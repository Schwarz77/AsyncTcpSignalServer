#include "Client.h"
#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <cstring>
#include <utils.h>


namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using error_code = boost::system::error_code;


//////////////////////////////////////////////////////////////////////////

Client::Client(asio::io_context& io, const std::string& host, uint16_t port, ESignalType signal_type)
    : m_io(io),
    m_socket(io),
    m_resolver(io),
    m_reconnect_timer(io),
    m_host(host),
    m_port(port),
    m_signal_type(signal_type)
{
}

Client::~Client()
{
    Stop();
}

void Client::Start()
{
    m_reconnect_timer.cancel();
    connect();

    std::cout << "Client started\n";
}

void Client::Stop()
{
    error_code ec;

    // Canceling timers will cause handlers to fail with error operation_aborted
    m_reconnect_timer.cancel(ec);

    // Closing a socket will also cause operation_aborted in pending read/writes
    if (m_socket.is_open())
    {
        m_socket.cancel(ec);
        m_socket.shutdown(tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
}

void Client::connect()
{
    tcp::resolver::query q(m_host, std::to_string(m_port));

    m_resolver.async_resolve(q,
        [this](const error_code& ec, tcp::resolver::results_type endpoints)
        {
            // Checking for cancellation of operation (Stop)
            if (ec == asio::error::operation_aborted)
            {
                return;
            }

            if (ec)
            {
                write_error("Resolve failed", ec);
                schedule_reconnect();
                return;
            }

            asio::async_connect(m_socket, endpoints,
                [this](const error_code& ec, const tcp::endpoint& ep)
                {
                    if (ec == asio::error::operation_aborted)
                    {
                        return;
                    }

                    if (ec)
                    {
                        write_error("Connect failed", ec);
                        schedule_reconnect();
                        return;
                    }

                    std::cout << "Connected to server\n";
                    send_subscribe();
                });
        });
}

void Client::send_subscribe()
{
    m_msg_num = 0;

    // Build subscribe payload
    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>(m_signal_type));

    // Header
    SSignalProtocolHeader hdr;
    hdr.signature = host_to_net_u16(SIGNAL_HEADER_SIGNATURE);
    hdr.version = 1;
    hdr.data_type = 0x01; // subscribe
    hdr.msg_num = 0;
    hdr.len = host_to_net_u32(static_cast<uint32_t>(payload.size()));  // data length 

    std::vector<uint8_t> frame(sizeof(hdr) + payload.size());
    std::memcpy(frame.data(), &hdr, sizeof(hdr));
    if (!payload.empty())
    {
        std::memcpy(frame.data() + sizeof(hdr), payload.data(), payload.size());
    }

    auto buf = std::make_shared<std::vector<uint8_t>>(std::move(frame));

    asio::async_write(m_socket, asio::buffer(*buf),
        [this, buf](const error_code& ec, std::size_t /*bytes_transferred*/)
        {
            if (ec == asio::error::operation_aborted)
            {
                return;
            }

            if (ec)
            {
                write_error("Write subscribe failed", ec);
                schedule_reconnect();
                return;
            }

            start_read_header();
        });
}

void Client::start_read_header()
{
    if (!m_socket.is_open())
    {
        return;
    }

    asio::async_read(m_socket, asio::buffer(&m_header, sizeof(m_header)),
        [this](const error_code& ec, std::size_t /*n*/)
        {
            if (ec == asio::error::operation_aborted)
            {
                return;
            }

            if (ec)
            {
                if (ec != asio::error::eof && ec != asio::error::connection_reset)
                {
                    write_error("Read header error", ec);
                }
                else
                {
                    std::cout << "Server closed connection\n";
                }

                schedule_reconnect();
                return;
            }

            // validate header
            SSignalProtocolHeader hdr = m_header;
            if (net_to_host_u16(hdr.signature) != SIGNAL_HEADER_SIGNATURE)
            {
                std::cerr << "Bad signature in header\n";
                schedule_reconnect();
                return;
            }

            if (hdr.version != 1)
            {
                std::cerr << "Bad version\n";
                schedule_reconnect();
                return;
            }

            if (hdr.msg_num != m_msg_num)
            {
                std::cerr << "Bad msg_num\n";
                schedule_reconnect();
                return;
            }

            m_msg_num++;

            uint32_t len = net_to_host_u32(hdr.len);

            // sanity cap
            if (len > 10 * 1024 * 1024)
            {
                std::cerr << "Packet too big, closing\n";
                schedule_reconnect();
                return;
            }

            start_read_body(len, hdr.data_type);
        });
}

void Client::start_read_body(uint32_t len, uint8_t data_type)
{
    m_body.resize(len);
    if (len == 0)
    {
        process_body(data_type, m_body);
        start_read_header();
        return;
    }

    asio::async_read(m_socket, asio::buffer(m_body),
        [this, data_type](const error_code& ec, std::size_t /*n*/)
        {
            if (ec == asio::error::operation_aborted)
            {
                return;
            }

            if (ec)
            {
                write_error("Read body error", ec);
                schedule_reconnect();
                return;
            }

            process_body(data_type, m_body);
            start_read_header();
        });
}

void Client::process_body(uint8_t data_type, const std::vector<uint8_t>& body)
{
    if (data_type == 0x02)
    {
        size_t pos = 0;
        while (pos + 13 <= body.size())
        {
            // id
            uint32_t id;
            std::memcpy(&id, body.data() + pos, 4);
            id = ntohl(id);
            pos += 4;

            // type
            uint8_t type = body[pos++];

            // value
            uint64_t ubits;
            std::memcpy(&ubits, body.data() + pos, 8);
            ubits = net_to_host_u64(ubits);
            pos += 8;

            double val;
            std::memcpy(&val, &ubits, sizeof(val));

            std::cout << "Update: id=" << id << " type=" << int(type) << " val=" << val << "\n";

            //Signal s(id, static_cast<ESignalType>(type), val);
            // ...

        }
    }
    else if (data_type == 0x03)
    {
        std::cout << "Alive msg\n";
    }
    else
    {
        std::cout << "Unknown msg_data_type=" << int(data_type)  << "\n";
    }
}

void Client::schedule_reconnect()
{
    // Close the socket to abort the current operation.
    error_code ec;
    if (m_socket.is_open())
    {
        m_socket.close(ec);
    }

    m_reconnect_timer.expires_after(std::chrono::seconds(2));

    m_reconnect_timer.async_wait(
        [this](const error_code& ec)
        {
            // If the timer is cancelled (at Stop) - exit
            if (ec == asio::error::operation_aborted)
            {
                return;
            }

            connect();
        });
}

