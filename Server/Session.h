#pragma once

#include <protocol.h>
#include <boost/asio.hpp>
#include <deque>
#include <vector>
#include <memory>



///////////////////////////////////////////////////////////////////////////


class Server;


class Session : public std::enable_shared_from_this<Session> 
{
    using tcp = boost::asio::ip::tcp;

public:
    Session(tcp::socket socket, Server& server);
    ~Session();

    void Start();
    void DeliverUpdates(const std::vector<Signal>& updates); // called by Server dispatcher

    bool Expired() const;

    void ForceClose();

private:
    void async_read_header();
    void async_read_body(std::size_t len, uint8_t dataType);
    void handle_subscribe(const std::vector<uint8_t>& payload);
    void do_write();
    void close();

private:
    using SocketExecutor = boost::asio::ip::tcp::socket::executor_type;
    using SessionStrand = boost::asio::strand<SocketExecutor>;
    using time_point = std::chrono::steady_clock::time_point;

    tcp::socket m_socket;

    SessionStrand m_strand;

    Server& m_server;

    std::array<uint8_t, sizeof(SSignalProtocolHeader)> m_buf_header;
    std::vector<uint8_t> m_buf_body;

    std::deque<std::shared_ptr<std::vector<uint8_t>>> m_que_write;
    bool m_writing{ false };

    uint8_t m_req_type{ 0 };

    time_point m_time_last_send;

    std::shared_ptr<Session> m_self;          // keep the self-pointer while the session is active
    std::atomic<bool> m_closing{ false };        // closing flag (to avoid trying to close multiple times)

};
