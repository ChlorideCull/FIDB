cmake_minimum_required(VERSION 2.6)
project(FIDB CXX)

set(CMAKE_CXX_FLAGS "-std=c++11")

add_library(FIDB SHARED database.cpp)

install(TARGETS FIDB DESTINATION lib)
install(FILES database.hpp DESTINATION include RENAME FIDB.hpp)
