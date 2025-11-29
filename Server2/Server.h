#pragma once
#include "session.h"
#include <boost/asio.hpp>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <thread>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <random>


class Server 
{
public:

    Server(boost::asio::io_context& io, uint16_t port);
    virtual ~Server();

    // disable copying
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void Start();
    void Stop();

     // subscription
    void RegisterSession(std::shared_ptr<Session> s);
    void UnregisterExpired();

    // signal API
    void PushSignal(const Signal& s);
    std::vector<Signal> GetSnapshot(uint8_t type);
    void SetSignals(const std::vector<Signal> vecSignal);

    boost::asio::io_context& GetIoContext() { return m_io; }

private:
    void do_accept();
    void dispatcher_loop();
    void producer_loop();

    void clear_sessions();

private:
    boost::asio::io_context& m_io;
    boost::asio::ip::tcp::acceptor m_acceptor;

    std::mutex m_mtx_subscribers;
    std::vector<std::weak_ptr<Session>> m_subscribers;

    std::mutex m_mtx_state;
    std::unordered_map<uint32_t, Signal> m_state;

    // signal event queue
    std::mutex m_mtx_queue;
    std::condition_variable m_cv_queue;
    std::deque<Signal> m_queue;
    std::atomic<bool> m_running{ true };

    std::thread m_dispatcher;
    std::thread m_producer;
};
