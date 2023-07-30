cmake_minimum_required(VERSION 3.7)
project(epoll-http-server)

set(CMAKE_CXX_STANDARD 17)	
set(BINARY server)

enable_testing()

include_directories(
    src
    )

file(GLOB sources src/*.cpp src/*.h)
add_executable(${BINARY} ${sources})	

