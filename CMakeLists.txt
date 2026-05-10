cmake_minimum_required(VERSION 3.14)
project(collect
    VERSION 2.0.0
    LANGUAGES C CXX
    DESCRIPTION "Lightweight system resource monitor for embedded Linux"
)

# ─── C++ Standard ───────────────────────────────────────────
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ─── Build Type Default ─────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# ─── Output Directories ─────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─── Compile Options ────────────────────────────────────────
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(COLLECT_DEBUG=1)
endif()

# ─── Core Library (linked into main executable) ─────────────
set(COLLECT_CORE_SOURCES
    # app
    src/app/CollectDaemon.cpp
    src/app/SignalHandler.cpp

    # config
    src/config/ConfigManager.cpp
    src/config/ConfigParser.cpp
    src/config/TypesDb.cpp

    # core
    src/core/PluginManager.cpp
    src/core/Dispatcher.cpp
    src/core/WriteQueue.cpp
    src/core/PluginLoader.cpp

    # snapshot
    src/snapshot/SnapshotManager.cpp

    # output
    src/output/JsonFormatter.cpp
    src/output/CsvFormatter.cpp

    # interact
    src/interact/AppConfigManager.cpp
    src/interact/IpcServer.cpp

    # utils
    src/utils/Logger.cpp
    src/utils/cJSON.c
)

add_library(collect_core STATIC ${COLLECT_CORE_SOURCES})

target_include_directories(collect_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(collect_core
    PUBLIC
        dl          # dlopen/dlsym
        pthread     # mutex/condvar (even single-threaded, needed for some APIs)
)

# ─── Main Executable ────────────────────────────────────────
add_executable(collect src/main.cpp)

target_link_libraries(collect
    PRIVATE
        collect_core
        -rdynamic   # export symbols for plugins
)

# ─── Plugins (each built as a shared library) ───────────────
add_subdirectory(plugins)

# ─── Copy share/ to build output ────────────────────────────
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/share/
    DESTINATION ${CMAKE_BINARY_DIR}/bin/share
)

# ─── Testing ────────────────────────────────────────────────
option(COLLECT_BUILD_TESTS "Build unit and integration tests" ON)
if(COLLECT_BUILD_TESTS)
    enable_testing()
    include(cmake/FetchGTest.cmake)
    add_subdirectory(tests)
endif()

# ─── Install Rules ──────────────────────────────────────────
install(TARGETS collect RUNTIME DESTINATION bin)
install(DIRECTORY share/ DESTINATION share)
