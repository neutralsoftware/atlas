//
// log.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Logging functions implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/tracer/log.h"
#include "atlas/network/pipe.h"
#include "json.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

using nlohmann::json;

TracerServices::TracerServices() : tracerPipe(nullptr) {}

void TracerServices::startTracing(int port) {
    if (tracerPipe != nullptr) {
        return;
    }
    tracerPipe = std::make_shared<NetworkPipe>();
    tracerPipe->setPort(port);
    tracerPipe->start();

    tracerPipe->onReceive([](const std::string &message) {
        std::cout << "Tracer received: " << message << std::endl;
    });
}

Logger::Logger() {}

void Logger::setConsoleFilter(bool showLogs, bool showWarnings,
                              bool showErrors) {
    showInfoLogs = showLogs;
    showWarningLogs = showWarnings;
    showErrorLogs = showErrors;
}

void Logger::log(const std::string &message, const std::string &file,
                 int line) const {
    if (!TracerServices::getInstance().isOk()) {
        if (!showInfoLogs) {
            return;
        }
        std::cout << "\033[1;32m[LOG]\033[0m " << message << " (" << file << ":"
                  << line << ")" << std::endl;
        return;
    }

    json log;
    log["type"] = "log";
    log["severity"] = "info";
    log["message"] = message;
    log["file"] = file;
    log["line"] = line;

    TracerServices::getInstance().tracerPipe->send(log.dump() + "\n");
}

void Logger::warning(const std::string &message, const std::string &file,
                     int line) const {
    if (!TracerServices::getInstance().isOk()) {
        if (!showWarningLogs) {
            return;
        }
        std::cout << "\033[1;33m[WARNING]\033[0m " << message << " (" << file
                  << ":" << line << ")" << std::endl;
        return;
    }

    json log;
    log["type"] = "log";
    log["severity"] = "warning";
    log["message"] = message;
    log["file"] = file;
    log["line"] = line;

    TracerServices::getInstance().tracerPipe->send(log.dump() + "\n");
}

void Logger::error(const std::string &message, const std::string &file,
                   int line) const {
    if (!TracerServices::getInstance().isOk()) {
        if (!showErrorLogs) {
            return;
        }
        std::cout << "\033[1;31m[ERROR]\033[0m " << message << " (" << file
                  << ":" << line << ")" << std::endl;
        return;
    }

    json log;
    log["type"] = "log";
    log["severity"] = "error";
    log["message"] = message;
    log["file"] = file;
    log["line"] = line;

    TracerServices::getInstance().tracerPipe->send(log.dump() + "\n");
}

DebugTimer::DebugTimer(const std::string &name) : name(name) {
    startTime = std::chrono::high_resolution_clock::now();
}

uint64_t DebugTimer::stop() {
    auto endTime = std::chrono::high_resolution_clock::now();
    uint64_t duration = std::chrono::duration_cast<std::chrono::microseconds>(
                            endTime - startTime)
                            .count();
    return duration;
}

DebugTimer::~DebugTimer() {}
