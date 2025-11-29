#pragma once

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include "protocol.h"

//////////////////////////////////////////////////////////////////////////

class Client
{
public:
    Client(boost::asio::io_context& io, const std::string& host, uint16_t port, ESignalType signal_type);
    virtual ~Client();

    // disable copying
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void Start();
    void Stop();

private:
    void connect();
    void send_subscribe();
    void start_read_header();
    void start_read_body(uint32_t len, uint8_t data_type);
    void process_body(uint8_t type, const std::vector<uint8_t>& body);

    void schedule_reconnect();

private:
    boost::asio::io_context& m_io;

    // The declaration order is important: socket/timers must be destroyed before io_context (it's external, so that's ok)
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::steady_timer m_reconnect_timer;

    std::string m_host;
    uint16_t m_port;
    ESignalType m_signal_type;

    // inbound buffers/state
    SSignalProtocolHeader m_header;
    std::vector<uint8_t> m_body;
};

