/*
 * process_usage.h
 * As part of the Atlas project
 * --------------------------------------
 * Description: Portable getrusage() subset for platforms without
 * <sys/resource.h>
 */

#ifndef ATLAS_PROCESS_USAGE_H
#define ATLAS_PROCESS_USAGE_H

#ifdef _WIN32

/**
 * @brief Minimal stand-in for POSIX `timeval` as used by `rusage`.
 */
struct AtlasUsageTime {
    long tv_sec = 0;
    long tv_usec = 0;
};

/**
 * @brief Subset of POSIX `rusage` populated on Windows.
 *
 * `ru_maxrss` is reported in kilobytes, matching Linux.
 */
struct rusage {
    AtlasUsageTime ru_utime;
    AtlasUsageTime ru_stime;
    long ru_maxrss = 0;
};

constexpr int RUSAGE_SELF = 0;

/**
 * @brief Fills `usage` with the current process CPU times and peak working
 * set. Only `RUSAGE_SELF` is supported.
 */
int getrusage(int who, rusage *usage);

#else
#include <sys/resource.h>
#endif

#endif // ATLAS_PROCESS_USAGE_H
