//
// pipe.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipe implementation for networking
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef NETWORK_PIPE_H
#define NETWORK_PIPE_H

// TCP Pipe
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**
 * @file atlas/network/pipe.h
 * @brief Simple TCP client pipe used by Atlas tooling (e.g. Tracer).
 *
 * \note This is an alpha API and may change.
 */

/**
 * @brief Callback invoked for each received message (raw string payload).
 */
using PipeCallback = std::function<void(const std::string &)>;

/**
 * @brief TCP client that connects to a server and streams newline-delimited
 * messages.
 *
 * The pipe owns a background receive thread. Use `start()` / `stop()` to
 * control the connection loop.
 *
 * \subsection networkpipe-example Example
 * ```cpp
 * NetworkPipe pipe;
 * pipe.setPort(5123);
 * pipe.onRecieve([](const std::string& msg) {
 *     std::cout << "Tracer: " << msg << std::endl;
 * });
 * pipe.start();
 *
 * pipe.send("hello\n");
 * // ... later
 * pipe.stop();
 * ```
 */
class NetworkPipe {
  private:
    int port = 0;
    std::string serverAddress = "127.0.0.1";
    std::atomic<std::uintptr_t> clientSocket{UINTPTR_MAX};

    std::thread recvThread;
    bool running = false;

    std::vector<std::string> messages;
    std::mutex messagesMutex;

    PipeCallback dispatcher;

    void connectLoop();
    void receiveLoop();

  public:
    /** @brief Constructs a disconnected pipe. */
    NetworkPipe();
    /** @brief Stops the receive thread and closes the socket. */
    ~NetworkPipe();

    /** @brief Sets the server port (defaults to 0 until set). */
    void setPort(int newPort);
    /** @brief Starts the background connection and receive loops. */
    void start();
    /** @brief Stops background threads and disconnects. */
    void stop();
    /** @brief Registers a callback to receive messages. */
    void onReceive(const PipeCallback &callback);
    /** @brief Sends a raw message to the server. */
    void send(const std::string &message) const;

    /** @brief Returns a snapshot of all received messages. */
    std::vector<std::string> getMessages();
};

#endif // NETWORK_PIPE_H
