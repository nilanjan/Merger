cmake_minimum_required(VERSION 2.8)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.hpp")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

project(merger)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
