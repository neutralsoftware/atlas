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
#include <cstdio>
#include <cstring>
#include <iostream>
#ifdef _WIN32
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
using SocketHandle = SOCKET;
constexpr SocketHandle InvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle InvalidSocket = -1;
#endif

std::uintptr_t storeSocket(SocketHandle socket) {
    return socket == InvalidSocket ? UINTPTR_MAX
                                   : static_cast<std::uintptr_t>(socket);
}

SocketHandle loadSocket(std::uintptr_t socket) {
    return socket == UINTPTR_MAX ? InvalidSocket
                                 : static_cast<SocketHandle>(socket);
}

void closeSocket(SocketHandle socket) {
    if (socket == InvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}
}

NetworkPipe::NetworkPipe() {
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        atlas_warning("Failed to initialize Winsock");
    }
#endif
}

NetworkPipe::~NetworkPipe() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

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
    SocketHandle sock = loadSocket(clientSocket.exchange(UINTPTR_MAX));
    closeSocket(sock);
    if (recvThread.joinable()) {
        recvThread.join();
    }
}

void NetworkPipe::connectLoop() {
    bool connected = false;
    bool messageShown = false;

    while (running && !connected) {
        SocketHandle socketHandle = socket(AF_INET, SOCK_STREAM, 0);
        clientSocket = storeSocket(socketHandle);
        if (socketHandle == InvalidSocket) {
            perror("socket");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<unsigned short>(port));

        if (inet_pton(AF_INET, serverAddress.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            closeSocket(socketHandle);
            clientSocket = UINTPTR_MAX;
            return;
        }

        if (connect(socketHandle, reinterpret_cast<sockaddr *>(&addr),
                    sizeof(addr)) < 0) {
            if (!messageShown) {
                std::cout
                    << "\033[1;3;32mWaiting for a tracer to connect...\033[0m"
                    << std::endl;
                messageShown = true;
            }
            closeSocket(socketHandle);
            clientSocket = UINTPTR_MAX;
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
        SocketHandle sock = loadSocket(clientSocket.load());
        if (sock == InvalidSocket) {
            break;
        }

        std::memset(buffer, 0, sizeof(buffer));
        const int received = recv(sock, buffer, static_cast<int>(sizeof(buffer)), 0);
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
            std::uintptr_t expected = storeSocket(sock);
            if (clientSocket.compare_exchange_strong(expected, UINTPTR_MAX)) {
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
    SocketHandle sock = loadSocket(clientSocket.load());
    if (sock != InvalidSocket) {
        const int sent = ::send(sock, message.c_str(),
                                static_cast<int>(message.size()), 0);
        if (sent < 0) {
            perror("send");
        }
    }
}

std::vector<std::string> NetworkPipe::getMessages() {
    std::scoped_lock lock(messagesMutex);
    return messages;
}
