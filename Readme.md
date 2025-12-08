Cross-Platform Async TCP Signal Server (Boost.Asio)

This project is a real-time system prototype for asynchronous data delivery over TCP/IP, built using C++ and the Boost.Asio library.

The architecture employs a Push Model to ensure minimal latency when delivering state updates (signals) to connected clients.

💡 Key Features

Boost.Asio: A fully asynchronous, multi-threaded server implementation for high performance and non-blocking I/O.

Dispatcher: The central component that manages client sessions and is responsible for reliable signal distribution to all connected clients.

Real-Time (Low Latency):

Push-Model: The server actively pushes changes to clients (Server -> Client), eliminating the need for client polling, which significantly reduces latency.

Asynchronous I/O: All network operations (read and write) are non-blocking thanks to Boost.Asio, preventing delays caused by waiting for a slow client.

State Synchronization: Upon connection, the client receives a full packet of signals with current states. Subsequently, the server sends only packets containing changes (deltas).

Packet Sequencing Control.

Client Reliability:

Implements automatic reconnection logic with exponential backoff.

The client monitors a timeout and initiates a reconnection if no message (including "Alive" messages from the server) has been received within the set interval.

Protocol: A custom, lightweight binary protocol is used for efficient data transmission.

📐 Project Architecture

The system consists of two primary components:

Server (Dispatcher Core):

Dispatcher: Registers active sessions and asynchronously distributes updates.

Session: Manages the connection in a thread-safe manner. The client requests a subscription to the required signal type(s) from the session upon connection establishment.

Heartbeat / Keep-Alive: If there are no data changes to send, the server periodically sends an "Alive" message to maintain the connection and reset the client's timeout.

Note: The Producer component (test data generator) is included purely for Push Model demonstration and must be replaced by an external data source in a production environment.

Client:

Maintains a stable connection and implements the Reliable Reconnection logic.

All I/O operations are non-blocking.

🛠️ Project Build (CMake)

The project uses CMake for building. Dependencies (Boost, GTest) are managed via apt on Linux and VCPKG on Windows.

1. Prerequisites

Platform

Requirements

Linux

GCC/G++ (v11+), CMake, libboost-dev, libgtest-dev (via apt).

Windows

Visual Studio 2019/2022, VCPKG. The VCPKG_ROOT environment variable must be set.

2. Build Instructions (Release)

First, clone the repository: git clone <your_repo_url> && cd <your_repo_name>

Linux

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .




Windows

mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .




The compiled binaries (Server, Client, Tests) will be located in the build/bin directory.