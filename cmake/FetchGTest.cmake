# Fetch Google Test via FetchContent
# Modified: Use URL download to avoid git dependency on build machine
include(FetchContent)

FetchContent_Declare(
    googletest
    URL            https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    URL_HASH       SHA256=1f357c27ca988c3f7c6b4bf68a9395005ac6761f034046e9dde0896e3aba00e4
)

# Prevent gtest from overriding compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Don't install gtest along with our project
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
