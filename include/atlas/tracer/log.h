//
// log.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Logging functions through the Tracer Protocol
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef TRACER_LOG_H
#define TRACER_LOG_H

#include <atlas/network/pipe.h>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>

/**
 * @file atlas/tracer/log.h
 * @brief Logging helpers for Atlas' Tracer tooling.
 *
 * Tracer is a lightweight TCP-based diagnostics sink. `Logger` formats
 * messages and forwards them to the tracer pipe.
 *
 * \note This is an alpha API and may change.
 */

/**
 * @brief Singleton services container that manages tracer connectivity.
 */
class TracerServices {
  private:
    TracerServices();

  public:
    static TracerServices &getInstance() {
        static TracerServices instance;
        return instance;
    }

    std::shared_ptr<NetworkPipe> tracerPipe;

    /** @brief Returns true when tracer connectivity exists. */
    bool isOk() const { return tracerPipe != nullptr; }

    /**
     * @brief Starts tracer logging by connecting to the given TCP port.
     */
    void startTracing(int port);
};

/**
 * @brief Simple logger that emits formatted messages via Tracer.
 */
class Logger {
  private:
    Logger();
    bool showInfoLogs = true;
    bool showWarningLogs = true;
    bool showErrorLogs = true;

  public:
    static Logger &getInstance() {
        static Logger instance;
        return instance;
    }

    /** @brief Logs an informational message. */
    void log(const std::string &message, const std::string &file,
             int line) const;
    /** @brief Logs a warning message. */
    void warning(const std::string &message, const std::string &file,
                 int line) const;
    /** @brief Logs an error message. */
    void error(const std::string &message, const std::string &file,
               int line) const;
    /**
     * @brief Configures which severities are echoed to the local console.
     */
    void setConsoleFilter(bool showLogs, bool showWarnings, bool showErrors);
};

/** @brief Convenience macro that logs an informational message with callsite.
 */
#define atlas_log(msg) Logger::getInstance().log(msg, __FILE__, __LINE__);
/** @brief Convenience macro that logs a warning message with callsite. */
#define atlas_warning(msg)                                                     \
    Logger::getInstance().warning(msg, __FILE__, __LINE__);
/** @brief Convenience macro that logs an error message with callsite. */
#define atlas_error(msg) Logger::getInstance().error(msg, __FILE__, __LINE__);

#define TRACER_PORT 5123

/**
 * @brief Small RAII helper that measures a scope duration and reports it on
 * destruction.
 *
 * \subsection debugtimer-example Example
 * ```cpp
 * {
 *   DebugTimer timer("ShadowPass");
 *   renderShadows();
 * } // timer stops and reports
 * ```
 */
class DebugTimer {
  public:
    /** @brief Starts timing a labeled scope. */
    DebugTimer(const std::string &name);
    /** @brief Stops timing and reports elapsed duration. */
    ~DebugTimer();

    /** @brief Stops timing manually and returns elapsed milliseconds. */
    uint64_t stop();

  private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
};

#endif // TRACER_LOG_H
