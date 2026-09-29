/*
 * process_usage.cpp
 * As part of the Atlas project
 * --------------------------------------
 * Description: Windows implementation of the getrusage() subset
 */

#include "atlas/core/process_usage.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>

namespace {
AtlasUsageTime toUsageTime(const FILETIME &time) {
    ULARGE_INTEGER value{};
    value.LowPart = time.dwLowDateTime;
    value.HighPart = time.dwHighDateTime;
    // FILETIME is expressed in 100-nanosecond intervals.
    const unsigned long long microseconds = value.QuadPart / 10ULL;
    AtlasUsageTime result;
    result.tv_sec = static_cast<long>(microseconds / 1'000'000ULL);
    result.tv_usec = static_cast<long>(microseconds % 1'000'000ULL);
    return result;
}
} // namespace

int getrusage(int who, rusage *usage) {
    if (who != RUSAGE_SELF || usage == nullptr) {
        return -1;
    }
    *usage = rusage{};

    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime,
                        &kernelTime, &userTime)) {
        usage->ru_utime = toUsageTime(userTime);
        usage->ru_stime = toUsageTime(kernelTime);
    }

    PROCESS_MEMORY_COUNTERS counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters,
                             sizeof(counters))) {
        usage->ru_maxrss =
            static_cast<long>(counters.PeakWorkingSetSize / 1024ULL);
    }
    return 0;
}

#endif
