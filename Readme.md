# Asynchronous TCP Signal Server (C++ / Boost.Asio)

## Overview

This project implements a high-performance, real-time asynchronous data distribution system using modern C++ and the Boost.Asio library. 
The codebase demonstrates a robust, push-based architecture designed for low-latency delivery of signal states over TCP/IP, eliminating the overhead of continuous client polling.

## Architecture & Design

The server is built around a centralized **Dispatcher Core** and relies entirely on **Boost.Asio's asynchronous I/O** primitives for maximum throughput and scalability.

1.  **Asynchronous I/O:** Utilizes non-blocking operations to manage thousands of concurrent connections on a limited thread pool, ensuring efficient use of system resources.
2.  **Dispatcher Pattern:** Manages active connections and broadcasts state updates to all subscribed clients immediately upon data change.
3.  **Thread Safety:** The write path for each client session is strictly protected by **Boost.Asio Strands** (serialized execution contexts) to ensure thread-safe access to the output buffers and guarantee in-order message delivery. 

## Key Features

* **Push-Model Delivery:** Messages are actively pushed from the server upon data change (no client polling).
* **Minimal Protocol:** Uses a custom, lightweight binary protocol designed for low-overhead delta transmission.
* **Resilience:** The client features robust automatic reconnection and connection heartbeats.
* **Comprehensive Testing:** Includes unit, integration, and load tests using GoogleTest.
* **CI/CD:** Continuous integration is configured via GitHub Actions to ensure cross-platform compatibility.

## Performance and Optimization

Design priorities are throughput and minimal P99 latency. Key optimizations include:

* **Binary Protocol:** Uses network byte order (Big-Endian) for efficient cross-platform serialization of fixed-size headers.
* **Delta Updates:** After initial state synchronization, only differences in data (deltas) are transmitted, significantly reducing network traffic.
* **Non-Blocking Logic:** The use of strands ensures that no single session can block the entire I/O loop, maintaining predictable latency for all connected clients.

## Protocol Specification

The communication is based on a structured binary frame:

| Field | Type | Size (Bytes) | Description |
| :--- | :--- | :--- | :--- |
| Signature | UINT16 | 2 | Magic number for protocol identification. |
| Version | UINT8 | 1 | Protocol version. |
| Message Type | UINT8 | 1 | Command or Data type identifier. |
| Message Number | UINT8 | 1 | Current Message Number. |
| Payload Size | UINT32 | 4 | Size of the dynamic payload that follows. |
| Data | UINT8 | Payload Size | Signals Data. |


## Build

The project uses **CMake** for build configuration.

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

Comprehensive testing, including unit, integration, performance, and stress tests, is critical for verifying the protocol handling, thread safety logic, and high throughput. 
Test files are located in /tests.

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
├── CMakeLists.txt 
├── .github/workflows/
│   └── ci.yml             (CI configuration)
├── Server/
│   ├── CMakeLists.txt 
│   ├── Session.h
│   ├── Session.cpp
│   ├── Server.h
│   ├── Server.cpp
│   └── main.cpp
├── Client/
│   ├── CMakeLists.txt 
│   ├── Client.h
│   ├── Client.cpp
│   └── main.cpp
├── Include/
│   └── Protocol.h
├── Utils/
│   ├── Utils.h
│   └── Utils.cpp
├── Tests/
│   ├── CMakeLists.txt 
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