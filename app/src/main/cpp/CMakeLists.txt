cmake_minimum_required(VERSION 3.22.1)

project("quickmem")

# Fetch QuickJS-ng from GitHub
include(FetchContent)
FetchContent_Declare(
    quickjs
    GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
    GIT_TAG v0.14.0
)
FetchContent_MakeAvailable(quickjs)

# Create quickmem executable
add_executable(quickmem
    main.cpp
    quickmem.cpp
    quickmem.h
    memio.cpp
    memio.h
    linenoise.c
)

# Add QuickJS include directory
target_include_directories(quickmem PRIVATE ${quickjs_SOURCE_DIR})

target_link_libraries(quickmem
    qjs
    log
)
