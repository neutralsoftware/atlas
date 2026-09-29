//
// pipe.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipe implementation for C++
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/network/pipe.h"
#include "atlas/tracer/log.h"
#include <cerrno>
#include <cstring>
#include <iostream>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>

namespace {
#ifdef _WIN32
// Winsock handles are kernel handles, which fit in 32 bits, so they can be
// stored in the pipe's std::atomic<int> like POSIX file descriptors.
using SocketLength = int;

bool ensureSocketsInitialized() {
    static const bool initialized = [] {
        WSADATA data{};
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    return initialized;
}

int openSocket() {
    if (!ensureSocketsInitialized()) {
        return -1;
    }
    SOCKET handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return handle == INVALID_SOCKET ? -1 : static_cast<int>(handle);
}

void closeSocket(int socket) { ::closesocket(static_cast<SOCKET>(socket)); }

long long receiveFromSocket(int socket, char *buffer, std::size_t size) {
    return ::recv(static_cast<SOCKET>(socket), buffer, static_cast<int>(size),
                  0);
}

long long sendToSocket(int socket, const char *data, std::size_t size) {
    return ::send(static_cast<SOCKET>(socket), data, static_cast<int>(size), 0);
}

int connectSocket(int socket, const sockaddr_in &address) {
    return ::connect(static_cast<SOCKET>(socket),
                     reinterpret_cast<const sockaddr *>(&address),
                     static_cast<SocketLength>(sizeof(address)));
}
#else
int openSocket() { return ::socket(AF_INET, SOCK_STREAM, 0); }

void closeSocket(int socket) { ::close(socket); }

long long receiveFromSocket(int socket, char *buffer, std::size_t size) {
    return ::recv(socket, buffer, size, 0);
}

long long sendToSocket(int socket, const char *data, std::size_t size) {
    return ::send(socket, data, size, 0);
}

int connectSocket(int socket, const sockaddr_in &address) {
    return ::connect(socket, reinterpret_cast<const sockaddr *>(&address),
                     sizeof(address));
}
#endif
} // namespace

NetworkPipe::NetworkPipe() = default;

NetworkPipe::~NetworkPipe() { stop(); }

void NetworkPipe::setPort(int newPort) { this->port = newPort; }

void NetworkPipe::onReceive(const PipeCallback &callback) {
    this->dispatcher = callback;
}

void NetworkPipe::start() {
    if (port == 0) {
        atlas_warning("Port not set. Cannot start NetworkPipe.");
        std::cerr << "Port not set. Cannot start NetworkPipe." << std::endl;
        return;
    }
    atlas_log("Starting network pipe on port " + std::to_string(port));
    running = true;

    connectLoop();
}

void NetworkPipe::stop() {
    running = false;
    int sock = clientSocket.exchange(-1);
    if (sock != -1) {
        closeSocket(sock);
    }
    if (recvThread.joinable()) {
        recvThread.join();
    }
}

void NetworkPipe::connectLoop() {
    bool connected = false;
    bool messageShown = false;

    while (running && !connected) {
        clientSocket = openSocket();
        if (clientSocket == -1) {
            perror("socket");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(port));

        if (inet_pton(AF_INET, serverAddress.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            closeSocket(clientSocket);
            clientSocket = -1;
            return;
        }

        if (connectSocket(clientSocket, addr) < 0) {
            if (!messageShown) {
                std::cout
                    << "\033[1;3;32mWaiting for a tracer to connect...\033[0m"
                    << std::endl;
                messageShown = true;
            }
            closeSocket(clientSocket);
            clientSocket = -1;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        connected = true;
        if (messageShown) {
            atlas_log("Connected to tracer on port " + std::to_string(port));
            std::cout << "\rConnected to tracer on port " << port << "!"
                      << std::string(20, ' ') << std::endl;
        } else {
            atlas_log("Connected to tracer on port " + std::to_string(port));
            std::cout << "Connected to tracer on port " << port << "!"
                      << std::endl;
        }
    }

    if (!connected) {
        return;
    }

    recvThread = std::thread(&NetworkPipe::receiveLoop, this);
}

void NetworkPipe::receiveLoop() {
    char buffer[4096];
    while (running) {
        int sock = clientSocket.load();
        if (sock == -1) {
            break;
        }

        std::memset(buffer, 0, sizeof(buffer));
        const long long received =
            receiveFromSocket(sock, buffer, sizeof(buffer));
        if (received > 0) {
            std::string msg(buffer, static_cast<std::size_t>(received));

            {
                std::scoped_lock lock(messagesMutex);
                messages.push_back(msg);
            }

            if (dispatcher) {
                dispatcher(msg);
            }
        } else if (received == 0) {
            atlas_log("Tracer disconnected");
            std::cout << "Tracer disconnected\n";
            int expected = sock;
            if (clientSocket.compare_exchange_strong(expected, -1)) {
                closeSocket(sock);
            }
            break;
        } else {
            perror("recv");
            break;
        }
    }
}

void NetworkPipe::send(const std::string &message) const {
    int sock = clientSocket.load();
    if (sock != -1) {
        const long long sent =
            sendToSocket(sock, message.c_str(), message.size());
        if (sent < 0) {
            perror("send");
        }
    }
}

std::vector<std::string> NetworkPipe::getMessages() {
    std::scoped_lock lock(messagesMutex);
    return messages;
}
