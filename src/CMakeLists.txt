cmake_minimum_required(VERSION 3.8)
project(transport_catalogue)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif()

file(GLOB SOURCES *.cpp *.h)

add_executable(${PROJECT_NAME} ${SOURCES})