#include <boost/asio.hpp>
#include <iostream>
#include "Server.h"

namespace io = boost::asio;

using time_point = std::chrono::steady_clock::time_point;
using steady_clock = std::chrono::steady_clock;


//#define TEST_SERVER_API

#ifdef TEST_SERVER_API
// test server API
void thread_set_signals(Server& server, size_t signal_count, bool& isStop, std::mutex& mtx, std::condition_variable& cv);
void thread_update_signals(Server& server, size_t signal_count, bool& isStop, std::mutex& mtx, std::condition_variable& cv);
#endif


int main() 
{
    try
    {
        io::io_context io;

        Server server(io, 5000);

        server.EnableDataEmulation(true);
        server.EnableShowLogMsg(true);

        std::vector<Signal> vecSignal = 
        {   Signal{ 1, ESignalType::discret } ,
            Signal{ 2, ESignalType::discret },
            Signal{ 3, ESignalType::analog },
            Signal{ 4, ESignalType::analog },
        };

        server.SetSignals(vecSignal);

        server.Start();


#ifdef TEST_SERVER_API
        /// test server API
        bool isStop = false;
        std::mutex mtx;
        std::condition_variable cv;
        std::thread tss(thread_set_signals, std::ref(server), vecSignal.size(), std::ref(isStop), std::ref(mtx), std::ref(cv));
        std::thread tsu(thread_update_signals, std::ref(server), vecSignal.size(), std::ref(isStop), std::ref(mtx), std::ref(cv));
#endif


        io.run();


#ifdef TEST_SERVER_API
        isStop = true;
        tss.join();
        tsu.join();
#endif

    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << "\n";
    }

    return 0;
}

#ifdef TEST_SERVER_API
void thread_set_signals(Server& server, size_t signal_count, bool& isStop, std::mutex& mtx, std::condition_variable& cv)
{
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(mtx);

            if (cv.wait_for(lock, std::chrono::seconds(5), [&isStop] { return isStop; }))
            {
                break;
            }
        }

        std::vector<Signal> vecSignal;
        for (int i = 0; i < signal_count; i += 2)
        {
            vecSignal.push_back(Signal(i, ESignalType::discret));
            vecSignal.push_back(Signal(i + 1, ESignalType::analog));
        }

        server.SetSignals(vecSignal);
    }
}

void thread_update_signals(Server& server, size_t signal_count, bool& isStop, std::mutex& mtx, std::condition_variable& cv)
{
    std::mt19937 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());

    // id
    std::uniform_int_distribution<int> ids(1, signal_count);

    // value
    std::uniform_int_distribution<int> discret_val(0, 1);
    std::uniform_real_distribution<double> delta(-0.5, 0.5);


    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(mtx);

            if (cv.wait_for(lock, std::chrono::milliseconds(100), [&isStop] { return isStop; }))
            {
                break;
            }
        }

        int id = ids(rng);

        Signal s;

        if (server.GetSignal(id, s))
        {
            s.value = (s.type == ESignalType::discret) ? (discret_val(rng)) : s.value + delta(rng);
            s.ts = steady_clock::now();

            server.PushSignal(s);
        }
    }
}
#endif