file(GLOB ATLAS_TEST_MAINS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/main.cpp")

foreach (test_main IN LISTS ATLAS_TEST_MAINS)
    get_filename_component(test_dir "${test_main}" DIRECTORY)
    get_filename_component(test_name "${test_dir}" NAME)
    if (NOT test_name MATCHES "^[A-Za-z0-9][A-Za-z0-9_-]*$")
        message(FATAL_ERROR "Invalid native test directory name: ${test_name}")
    endif ()
    set(test_target "atlas_test_${test_name}")
    file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "${test_dir}/*.cpp")
    add_executable(${test_target} EXCLUDE_FROM_ALL ${test_sources})
    target_link_libraries(${test_target} PRIVATE Atlas::Atlas)
    target_include_directories(${test_target} PRIVATE "${test_dir}")
    set_target_properties(${test_target} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${test_dir}")
    target_compile_definitions(${test_target} PRIVATE
        TEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    if (WIN32)
        set(test_dlls "$<TARGET_RUNTIME_DLLS:${test_target}>")
        if (TARGET slang)
            list(APPEND test_dlls "$<TARGET_FILE:slang>")
        endif ()
        file(GENERATE
                OUTPUT "${CMAKE_BINARY_DIR}/atlas-test-${test_name}-dlls-$<CONFIG>.txt"
                CONTENT "$<JOIN:${test_dlls},\n>\n")
    endif ()
endforeach ()
