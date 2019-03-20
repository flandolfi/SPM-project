cmake_minimum_required(VERSION 3.10)
project(SPM_project)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -fsanitize=thread -fstack-check -fstack-protector-strong \
                           -fno-omit-frame-pointer -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_STANDARD 14)
set(THREADS_PREFER_PTHREAD_FLAG ON)

include_directories(include)

add_subdirectory(src/dac)
add_subdirectory(src/test)
add_subdirectory(external/)
