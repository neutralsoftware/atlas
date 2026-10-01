if(UNIX AND NOT APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(VORPALINE_PLATFORM "Linux64-gcc-aarch64")
endif()

foreach(ATLAS_GEOGRAM_PLATFORM IN ITEMS Darwin-clang Linux-clang Linux-gcc)
    set(ATLAS_GEOGRAM_PLATFORM_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms/${ATLAS_GEOGRAM_PLATFORM}.cmake")
    if(EXISTS "${ATLAS_GEOGRAM_PLATFORM_FILE}")
        file(READ "${ATLAS_GEOGRAM_PLATFORM_FILE}" ATLAS_GEOGRAM_PLATFORM_CONTENT)
        string(REPLACE
            "add_definitions(\${FULL_WARNINGS})"
            "add_definitions(\${NORMAL_WARNINGS})"
            ATLAS_GEOGRAM_PLATFORM_CONTENT
            "${ATLAS_GEOGRAM_PLATFORM_CONTENT}")
        file(WRITE "${ATLAS_GEOGRAM_PLATFORM_FILE}" "${ATLAS_GEOGRAM_PLATFORM_CONTENT}")
    endif()
endforeach()
