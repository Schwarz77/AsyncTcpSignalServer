// Session.cpp

#include "Session.h"
#include "Server.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include <utils.h>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using error_code = boost::system::error_code;
using time_point = std::chrono::steady_clock::time_point;
using steady_clock = std::chrono::steady_clock;


///////////////////////////////////////////////////////////////////////////


Session::Session(tcp::socket socket, Server& server)
    : m_socket(std::move(socket))
    , m_strand(asio::make_strand(m_socket.get_executor()))
    , m_server(server)
{
    m_time_last_send = steady_clock::now();
}

Session::~Session()
{
}

void Session::Start()
{
    async_read_header();
}

void Session::async_read_header()
{
    if (!m_socket.is_open())
    {
        return;
    }

    auto self = shared_from_this();

    // read exactly header size
    asio::async_read(m_socket, asio::buffer(m_buf_header),
        asio::bind_executor(m_strand,
            [this, self](error_code ec, std::size_t /*n*/)
            {
                if (ec)
                {
                    if (ec == asio::error::connection_reset ||
                        ec == asio::error::eof)
                    {
                        std::cout << "Client disconnected\n";
                    }
                    else if (ec == asio::error::operation_aborted)
                    {
                        // std::cout << "Read header aborted (Server::SetSignals)\n";
                    }
                    else 
                    {
                        write_error("Read header error", ec);
                    }

                    close();
                    return;
                }

                m_cnt_read_packet++;
                if (m_cnt_read_packet > 1)
                {
                    std::cout << "Client sent second query\n"; //  The client should not send a second request when working with the push protocol 
                    close();
                    return;
                }

                SSignalProtocolHeader hdr;
                std::memcpy(&hdr, m_buf_header.data(), sizeof(hdr));

                if (net_to_host_u16(hdr.signature) != SIGNAL_HEADER_SIGNATURE)
                {
                    std::cerr << "Session: bad signature, closing\n";
                    close();
                    return;
                }
                if (hdr.version != 1)
                {
                    std::cerr << "Session: bad version, closing\n";
                    close();
                    return;
                }

                uint8_t dataType = hdr.dataType;
                uint32_t len = net_to_host_u32(hdr.len);

                if (len > 10 * 1024 * 1024)
                {
                    std::cerr << "Session: payload too large (" << len << "), closing\n";
                    close();
                    return;
                }

                // read body of length 'len'
                async_read_body(len, dataType);
            }));
}

void Session::async_read_body(std::size_t len, uint8_t dataType)
{
    if (!m_socket.is_open())
    {
        return;
    }

    auto self = shared_from_this();

    if (len)
        m_buf_body.resize(len);
    else
        m_buf_body.clear();

    asio::async_read(m_socket, asio::buffer(m_buf_body),
        asio::bind_executor(m_strand,
            [this, self, dataType, len](error_code ec, std::size_t /*n*/)
            {
                if (ec)
                {
                    if (ec == asio::error::connection_reset ||
                        ec == asio::error::eof)
                    {
                        std::cout << "Client disconnected\n";
                    }
                    else if (ec == asio::error::operation_aborted)
                    {
                        //std::cout << "Read body aborted (server disconnects clients in Server::SetSignals)\n";
                    }
                    else
                    {
                        write_error("Read body error", ec);
                    }
                    close();
                    return;
                }

                if (dataType == 0x01)
                {
                    handle_subscribe(m_buf_body);
                }
                else
                {
                    std::cerr << "Session: unexpected dataType from client: " << int(dataType) << "\n";
                }


                async_read_header();  // <- // We don't really expect anything from the server.
                                            // But this is necessary so that the session is not closed when exiting the lambda (deleting Session when releasing "self"). 
                                            // If we remove this, we will have to work with std::shared_ptr<CSession> m_self.

            }));
}

void Session::handle_subscribe(const std::vector<uint8_t>& payload)
{
    if (payload.empty())
    {
        std::cerr << "Session: subscribe payload empty\n";
        close();
        return;
    }

    m_req_type = payload[0];
    std::cout << "Session: client subscribed to type=" << int(m_req_type) << "\n";


    // register session with server
    m_server.RegisterSession(shared_from_this());


    // send initial snapshot for this type
    auto snap = m_server.GetSnapshot(m_req_type);
    if (!snap.empty())
    {
        DeliverUpdates(snap);
    }
}

// DeliverUpdates mostly unchanged
void Session::DeliverUpdates(const std::vector<Signal>& updates)
{
    auto self = shared_from_this();
    asio::post(m_strand, [this, self, updates]() 
        {
            if (!m_socket.is_open()) 
            {
                // if socket already closed, drop updates
                return;
            }

            std::vector<uint8_t> payload;
            payload.reserve(updates.size() * (4 + 1 + 8));

            for (const auto& e : updates)
            {
                if (!((uint8_t)e.type & m_req_type))
                {
                    continue;
                }

                // id
                uint32_t idSignal = host_to_net_u32(e.id);
                payload.insert(payload.end(), (uint8_t*)&idSignal, (uint8_t*)&idSignal + 4);

                // type 
                payload.push_back(static_cast<uint8_t>(e.type));

                // value
                uint64_t value; 
                static_assert(sizeof(value) == sizeof(e.value), "double size mismatch");
                std::memcpy(&value, &e.value, sizeof(value));
                value = host_to_net_u64(value);
                payload.insert(payload.end(), (uint8_t*)&value, (uint8_t*)&value + 8);
            }

            SSignalProtocolHeader hdr;
            hdr.signature = host_to_net_u16(SIGNAL_HEADER_SIGNATURE);
            hdr.version = 1;
            hdr.dataType = 0x02;
            hdr.len = host_to_net_u32(static_cast<uint32_t>(payload.size()));

            auto frame = std::make_shared<std::vector<uint8_t>>();
            frame->resize(sizeof(hdr) + payload.size());
            std::memcpy(frame->data(), &hdr, sizeof(hdr));
            if (!payload.empty())
            {
                std::memcpy(frame->data() + sizeof(hdr), payload.data(), payload.size());
            }

            bool need_start = m_que_write.empty() && !m_writing;
            m_que_write.push_back(frame);
            if (need_start)
            {
                do_write();
            }

            m_time_last_send = steady_clock::now();
        });
}

void Session::do_write()
{
    // must run in strand
    if (!m_socket.is_open())
    {
        m_que_write.clear();
        m_writing = false;
        return;
    }

    if (m_que_write.empty())
    {
        m_writing = false;
        return;
    }

    m_writing = true;
    auto frame = m_que_write.front();
    auto self = shared_from_this();

    // Note: we already use bind_executor when posting so this call runs in strand.
    asio::async_write(m_socket, asio::buffer(*frame),
        asio::bind_executor(m_strand,
            [this, self, frame](error_code ec, std::size_t /*n*/) 
            {
                if (ec)
                {
                    if (ec == asio::error::connection_reset ||
                        ec == asio::error::eof)
                    {
                        std::cout << "Client disconnected\n";
                    }
                    else if(ec == asio::error::operation_aborted)
                    {
                        //std::cout << "Write aborted (server disconnects clients in Server::SetSignals)\n";
                    }
                    else
                    {
                        write_error("Write error", ec);
                    }

                    // If write fails, close once (safe)
                    close();
                    return;
                }

                // remove sent frame and continue
                m_que_write.pop_front();
                if (!m_que_write.empty())
                {
                    do_write();
                }
                else
                {
                    m_writing = false;
                }
            }));
}

void Session::close()
{
    bool expected = false;
    if (!m_closing.compare_exchange_strong(expected, true))
    {
        return; // already closing
    }

    // perform socket shutdown & close synchronously then clear queue on strand
    error_code ec;
    if (m_socket.is_open())
    {
        m_socket.shutdown(asio::socket_base::shutdown_both, ec);
        m_socket.close(ec);
    }

    // clear queued frames on the strand to avoid races
    auto self = shared_from_this();
    asio::post(m_strand, [this, self]() 
        {
            m_que_write.clear();
            m_writing = false;
        });

    //std::cout << "Session closed\n";
}

bool Session::Expired() const
{
    return !m_socket.is_open();
}

void Session::ForceClose()
{
    auto self = shared_from_this();
    asio::post(m_strand, [this, self]()
        {
            close();
        });
}