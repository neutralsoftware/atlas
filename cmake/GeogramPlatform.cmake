if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(VORPALINE_PLATFORM "Linux64-gcc-aarch64")
endif()
