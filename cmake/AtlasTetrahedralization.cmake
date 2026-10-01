FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://eigen.googlesource.com/mirror
        GIT_TAG 3.4.0
        GIT_SHALLOW TRUE
)

if(UNIX AND NOT APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(
        CMAKE_PROJECT_Geogram_INCLUDE
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GeogramPlatform.cmake"
    )
endif()

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/fTetWild
    ${CMAKE_CURRENT_BINARY_DIR}/extern/fTetWild
    EXCLUDE_FROM_ALL
)

if(UNIX AND NOT APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    target_compile_options(FloatTetwild PUBLIC -fsigned-char)
endif()

set(GEOGRAM_MEMORY_HEADER
    "${CMAKE_BINARY_DIR}/_deps/geogram-src/src/lib/geogram/basic/memory.h"
)

if(EXISTS "${GEOGRAM_MEMORY_HEADER}")
    file(READ "${GEOGRAM_MEMORY_HEADER}" GEOGRAM_MEMORY_CONTENT)

    if(NOT GEOGRAM_MEMORY_CONTENT MATCHES "#include <new>")
        string(
            REPLACE
            "#include <vector>"
            "#include <new>\n#include <vector>"
            GEOGRAM_MEMORY_CONTENT
            "${GEOGRAM_MEMORY_CONTENT}"
        )
        file(WRITE "${GEOGRAM_MEMORY_HEADER}" "${GEOGRAM_MEMORY_CONTENT}")
        message(STATUS "Patched Geogram memory header")
    endif()
endif()

set(GEOGRAM_XATLAS_SOURCE
    "${CMAKE_BINARY_DIR}/_deps/geogram-src/src/lib/geogram/third_party/xatlas/xatlas.cpp"
)

if(EXISTS "${GEOGRAM_XATLAS_SOURCE}")
    file(READ "${GEOGRAM_XATLAS_SOURCE}" GEOGRAM_XATLAS_CONTENT)

    if(NOT GEOGRAM_XATLAS_CONTENT MATCHES "#include <stdlib.h>")
        string(
            REPLACE
            "#include <stdint.h>"
            "#include <stdint.h>\n#include <stdlib.h>"
            GEOGRAM_XATLAS_CONTENT
            "${GEOGRAM_XATLAS_CONTENT}"
        )
        file(WRITE "${GEOGRAM_XATLAS_SOURCE}" "${GEOGRAM_XATLAS_CONTENT}")
        message(STATUS "Patched Geogram xatlas source")
    endif()
endif()

if(MSVC)
    set(GEOGRAM_POISSON_HASH
        "${CMAKE_BINARY_DIR}/_deps/geogram-src/src/lib/geogram/third_party/PoissonRecon/Hash.h"
    )

    if(EXISTS "${GEOGRAM_POISSON_HASH}")
        file(READ "${GEOGRAM_POISSON_HASH}" GEOGRAM_HASH_CONTENT)

        if(GEOGRAM_HASH_CONTENT MATCHES "#include <hash_map>")
            string(
                REPLACE
                "#include <hash_map>"
                "#include <unordered_map>"
                GEOGRAM_HASH_CONTENT
                "${GEOGRAM_HASH_CONTENT}"
            )

            string(
                REPLACE
                "using stdext::hash_map;"
                "template<class... Args> using hash_map = std::unordered_map<Args...>;"
                GEOGRAM_HASH_CONTENT
                "${GEOGRAM_HASH_CONTENT}"
            )

            file(
                WRITE
                "${GEOGRAM_POISSON_HASH}"
                "${GEOGRAM_HASH_CONTENT}"
            )

            message(STATUS "Patched Geogram PoissonRecon <hash_map>")
        else()
            message(STATUS "Geogram PoissonRecon does not require hash_map patch")
        endif()
    endif()
endif()

get_target_property(ATLAS_TETRAHEDRAL_LINK_LIBRARIES FloatTetwild LINK_LIBRARIES)
list(REMOVE_ITEM ATLAS_TETRAHEDRAL_LINK_LIBRARIES warnings::all)
set_property(TARGET FloatTetwild PROPERTY LINK_LIBRARIES "${ATLAS_TETRAHEDRAL_LINK_LIBRARIES}")

foreach(ATLAS_TETRAHEDRAL_TARGET IN ITEMS FloatTetwild geogram)
    if(TARGET ${ATLAS_TETRAHEDRAL_TARGET})
        get_target_property(ATLAS_TETRAHEDRAL_INCLUDES ${ATLAS_TETRAHEDRAL_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
        if(ATLAS_TETRAHEDRAL_INCLUDES)
            set_property(TARGET ${ATLAS_TETRAHEDRAL_TARGET} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${ATLAS_TETRAHEDRAL_INCLUDES}")
        endif()
    endif()
endforeach()
