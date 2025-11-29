#include <boost/asio.hpp>
#include <iostream>
#include "server.h"

namespace io = boost::asio;

void thread_setsignals(Server& server, bool& isStop)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10 * 1000));

    while (!isStop) 
    {
        //// real - for send to gRPCServer 
        std::vector<Signal> vecSignal;
        for (int i = 0; i < 1000; i += 2)
        {
            vecSignal.push_back(Signal(i, ESignalType::discret));
            vecSignal.push_back(Signal(i + 1, ESignalType::analog));
        }

        server.SetSignals(vecSignal);

        std::cout << "set signals" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5*1000)); 
    }
}


int main() 
{
    try
    {
        io::io_context io;

        Server server(io, 5000);

        // for console test client 
        std::vector<Signal> vecSignal = 
        {   Signal{ 1, ESignalType::discret } ,
            Signal{ 2, ESignalType::discret },
            Signal{ 3, ESignalType::analog },
            Signal{ 4, ESignalType::analog },
        };

        //// real - for send to gRPCServer 
        //std::vector<SSignal> vecSignal;
        //for (int i = 0; i < 2000; i += 2)
        //{
        //    vecSignal.push_back(SSignal(i, ESignalType::discret));
        //    vecSignal.push_back(SSignal(i + 1, ESignalType::analog));
        //}

        server.SetSignals(vecSignal);

        bool isStop = false;
        std::thread ts(thread_setsignals, std::ref(server), std::ref(isStop));

        io.run();

        isStop = true;
        ts.join();

    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << "\n";
    }

    return 0;
}