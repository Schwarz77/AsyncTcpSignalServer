# Cross-Platform Async TCP Signal Server / Client (C++ / Boost.Asio)

## Overview

This project implements a high-performance, real-time asynchronous data distribution system using modern C++ and the Boost.Asio library. The codebase demonstrates a robust, push-based architecture designed for low-latency delivery of state updates (referred to as "signals") over TCP/IP, eliminating the overhead of client polling.

## Key Features

 - Fully Asynchronous I/O: Server and client are built entirely on Boost.Asio's asynchronous primitives.

 - Push-Model Delivery: Messages are actively pushed from the server upon update (no client polling).

 - Minimal Protocol Overhead: Uses a lightweight, efficient binary protocol.

 - Delta Updates: Sends the initial full state, followed by only the changed values (delta).

 - Automatic Reconnection: Client implements robust, automatic reconnection logic with a configurable delay.

 - Connection Health: Periodic keep-alive messages (heartbeats) maintain connection health during periods of inactivity.

 - Thread Safety: Ensures data integrity and reliable concurrent access using Boost.Asio Strands to protect the session's write queue.

 - Cross-Platform: Confirmed build on Linux (GCC) and Windows (MSVC).

 - Testing: Comprehensive unit tests using GoogleTest.

 - CI/CD: Continuous integration managed via GitHub Actions.

## Architecture

### Server (Dispatcher Core)

The server utilizes a centralized Dispatcher pattern to manage concurrency and message fan-out.

 1. Acceptor: Accepts and validates incoming TCP connections.

 2. Session: Manages a dedicated, thread-safe connection with a single client. Sessions own the asynchronous read and write logic, leveraging Strands for serialized access to the internal write queue.

 3. Dispatcher: Maintains the registry of all active sessions and broadcasts updates to them upon receiving a new signal from the internal Producer or an external source.

### Client

The client establishes a persistent connection, subscribes to the data stream, and handles asynchronous message processing, including automatic recovery from network interruptions.

### Protocol

The custom lightweight protocol ensures efficient transmission:

 - Fixed Header: Includes mandatory fields (signature, version, message type, and payload size).

 - Endianness: All multi-byte fields are transmitted in Network Byte Order (Big-Endian).

 - Payload: Binary data containing the signal identifier and its updated value.

## Build

The project uses CMake for build configuration.

### Dependencies

 - C++ Compiler: GCC/G++ (v11+) or MSVC (2019+).

 - CMake: Version 3.10 or higher.

 - Boost: Required for Asio (version 1.70+ recommended).

 - GoogleTest: Used for unit testing.

### Standard Build Instructions (Release Configuration)

First, clone the repository:
```
git clone <repo_url>
cd <repo>
```

### Linux (Using System Dependencies)
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Windows (Using Vcpkg)

Ensure that VCPKG is installed and the environment variable `%VCPKG_ROOT%` is correctly set.
```
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Binaries (Server and Client executables) are generated in the build/bin directory.

## Running

### Server

Start the server, listening on port 5000:
```
./bin/Server 5000
```

### Client

Start the client and connect to the server at 127.0.0.1:5000:
```
./bin/Client 127.0.0.1 5000
```

## Testing

Unit tests are crucial for verifying the protocol handling and thread safety logic. Test files are located in /tests.

### Executing Tests

To run the unit tests in Debug mode:
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest --output-on-failure --verbose
```

## Continuous Integration (CI)

CI workflows are managed via GitHub Actions to ensure build and test compatibility across target platforms.

Configuration file: .github/workflows/ci.yml

### CI tasks include:

 - Building and testing on Ubuntu and Windows.

 - Automatic dependency installation (Boost + GoogleTest).
 
 - Running CTest with verbose output. 
 
 - Generating and attaching platform-specific release artifacts on version tags (e.g., v1.0.0).

## Directory Structure
```
├── .github/workflows/
│   └── ci.yml             (CI configuration)
├── Server/
│   ├── Session.h
│   ├── Session.cpp
│   ├── Server.h
│   ├── Server.cpp
│   └── main.cpp
├── Client/
│   ├── Client.h
│   ├── Client.cpp
│   └── main.cpp
├── Include/
│   └── Protocol.h
├── Utils/
│   ├── Utils.h
│   └── Utils.cpp
├── Tests/
│   ├── server_test.cpp
│   ├── session_test.cpp
│   ├── utility_test.cpp
│   ├── perf_test.cpp
│   ├── integration_test.cpp
│   └── stress_test.cpp
└──build/
```


## License

This project is licensed under the **MIT License**.