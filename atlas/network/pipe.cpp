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
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>

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
        close(sock);
    }
    if (recvThread.joinable()) {
        recvThread.join();
    }
}

void NetworkPipe::connectLoop() {
    bool connected = false;
    bool messageShown = false;

    while (running && !connected) {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            perror("socket");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, serverAddress.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            close(clientSocket);
            clientSocket = -1;
            return;
        }

        if (connect(clientSocket, reinterpret_cast<sockaddr *>(&addr),
                    sizeof(addr)) < 0) {
            if (!messageShown) {
                std::cout
                    << "\033[1;3;32mWaiting for a tracer to connect...\033[0m"
                    << std::endl;
                messageShown = true;
            }
            close(clientSocket);
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
        ssize_t received = recv(sock, buffer, sizeof(buffer), 0);
        if (received > 0) {
            std::string msg(buffer, received);

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
                close(sock);
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
        ssize_t sent = ::send(sock, message.c_str(), message.size(), 0);
        if (sent < 0) {
            perror("send");
        }
    }
}

std::vector<std::string> NetworkPipe::getMessages() {
    std::scoped_lock lock(messagesMutex);
    return messages;
}
