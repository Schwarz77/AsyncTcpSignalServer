// server.cpp

#include "Server.h"
#include "Session.h"
#include <iostream>
#include <chrono>
#include <Utils.h>


namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using error_code = boost::system::error_code;
using time_point = std::chrono::steady_clock::time_point;
using steady_clock = std::chrono::steady_clock;



Server::Server(asio::io_context& io, uint16_t port)
    : m_io(io), m_acceptor(io, tcp::endpoint(tcp::v4(), port), true/*false*/)
{
    //Start();

    m_dispatcher = std::thread(&Server::dispatcher_loop, this);
    m_producer = std::thread(&Server::producer_loop, this);
}

Server::~Server() 
{
    Stop();
}

void Server::Start() 
{
    do_accept();

    if (m_show_log_msg)
        std::cout << "Server started\n";
}

void Server::do_accept() 
{
    m_acceptor.async_accept([this](error_code ec, tcp::socket socket) 
        {
            if (!ec) 
            {
                if (m_show_log_msg)
                    std::cout << "Accepted connection\n";

                auto s = std::make_shared<Session>(std::move(socket), *this);
                s->Start();

                do_accept();
            }
            else if (ec == boost::asio::error::operation_aborted)
            {
                // Normal termination (cancellation)
                
                //std::cout << "Acceptor stopped gracefully." << std::endl;
            }
            else if (   ec == boost::asio::error::would_block   ||
                        ec == boost::asio::error::interrupted)
            {
                // Recoverable errors (need to try again).

                write_error("Accept error", ec);
                do_accept();
            }
            else
            {
                // Unrecoverable error (including 10009 Bad File Descriptor)

                write_error("Accept error, STOP ACCEPT!", ec);

                // do_accept() should never be called !

            }
        });
}

void Server::Stop() 
{
    m_running = false;

    // wake dispatcher
    m_cv_queue.notify_all();

    // close acceptor
    error_code ec;

    if (m_acceptor.is_open())
    {
        m_acceptor.cancel(ec);
    }

    if (m_acceptor.is_open())
    {
        m_acceptor.close(ec);
    }

    if (m_producer.joinable())
    {
        m_producer.join();
    }

    if (m_dispatcher.joinable())
    {
        m_dispatcher.join();
    }

    // just in case - for guaranteed absence of leaks
    clear_sessions(); 
}

void Server::SetSignals(const std::vector<Signal> vecSignal)
{
    asio::post(m_io, [this, vecSignal]()
        {
            // Closing all client connections so that clients can reconnect and receive the changed count of signals.

            std::lock_guard<std::mutex> lk(m_mtx_subscribers);

            for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
            {
                if (auto sp = it->lock())
                {
                    sp->ForceClose();
                    ++it;
                }
                else
                {
                    it = m_subscribers.erase(it);
                }
            }

            // set signals

            {
                std::lock_guard<std::mutex> lk(m_mtx_state);

                m_state.clear();

                auto now = steady_clock::now();

                for (auto s : vecSignal)
                {
                    m_state[s.id] = s;
                }
            }
        });
}

void Server::RegisterSession(std::shared_ptr<Session> s) 
{
    std::lock_guard<std::mutex> lk(m_mtx_subscribers);

    m_subscribers.push_back(s);
}

void Server::UnregisterExpired() 
{
    std::lock_guard<std::mutex> lk(m_mtx_subscribers);

    m_subscribers.erase(std::remove_if(m_subscribers.begin(), m_subscribers.end(),
        [](const std::weak_ptr<Session>& w) 
        { 
            return w.expired(); 
        }),
        m_subscribers.end());
}

bool Server::PushSignal(const Signal& s) 
{
    bool pushed = false;

    {
        std::lock_guard<std::mutex> lk_state(m_mtx_state);

        auto it = m_state.find(s.id);

        if (it != m_state.end() && s.ts >= it->second.ts)
        {
            m_state[s.id] = s;
            pushed = true;
        }
    }

    if (pushed)
    {
        {
            std::lock_guard<std::mutex> lk_queue(m_mtx_queue);
            m_queue.push_back(s);
        }
        m_cv_queue.notify_one();
    }

    return pushed;
}

bool Server::GetSignal(int id, Signal& s)
{
    std::lock_guard<std::mutex> lk(m_mtx_state);

    auto it = m_state.find(id);
    if (it != m_state.end())
    {
        s = it->second;
        return true;
    }

    return false;
}

std::vector<Signal> Server::GetSnapshot(uint8_t type) 
{
    std::vector<Signal> out;
    std::lock_guard<std::mutex> lk(m_mtx_state);

    for (auto& p : m_state)
    {
        if ((uint8_t)p.second.type & type)
        {
            out.push_back(p.second);
        }
    }

    return out;
}

void Server::dispatcher_loop() 
{
    while (m_running) 
    {
        std::vector<Signal> batch;

        {
            std::unique_lock<std::mutex> lk(m_mtx_queue);

            m_cv_queue.wait(lk, [&] 
                {
                    return !m_queue.empty() || !m_running; 
                });

            while (!m_queue.empty()) 
            {
                batch.push_back(m_queue.front()); 
                m_queue.pop_front(); 
            }
        }

        if (!batch.empty()) 
        {
            // delivery: broadcast to subscribers
            std::lock_guard<std::mutex> lk(m_mtx_subscribers);

            for (auto it = m_subscribers.begin(); it != m_subscribers.end();) 
            {
                if (auto sp = it->lock()) 
                {
                    sp->DeliverUpdates(batch);
                    ++it;
                }
                else
                {
                    it = m_subscribers.erase(it);
                }
            }
        }
    }
}

void Server::producer_loop()
{
    std::mt19937 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());

    std::uniform_int_distribution<int> discret_val(0, 1);
    std::uniform_real_distribution<double> delta(-0.5, 0.5);

    while (m_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(700 + (rng() % 800)));

        if (!m_data_emulation || m_state.empty())
        {
            continue;
        }

        int state_size;
        {
            std::lock_guard<std::mutex> lk(m_mtx_state);
            state_size = m_state.size();
        }

        std::uniform_int_distribution<int> ids(1, state_size);

        std::uniform_int_distribution<int> cnt_rnd(1, state_size);
        int cnt = cnt_rnd(rng);

        for (int i = 0; i < cnt; i++)
        {
            int id = ids(rng);

            Signal current_s;
            if (!GetSignal(id, current_s))
            {
                continue;
            }

            // generate new signal
            Signal new_s = current_s;
            new_s.value = (new_s.type == ESignalType::discret) ? (discret_val(rng)) : new_s.value + delta(rng);
            new_s.ts = steady_clock::now();

            PushSignal(new_s);
        }
    }
}


void Server::clear_sessions()
{
    std::lock_guard<std::mutex> lk(m_mtx_subscribers);

    for (auto& it : m_subscribers)
    {
        if (auto sp = it.lock())
        {
            sp->ForceClose();
        }
    }

    m_subscribers.clear();
}