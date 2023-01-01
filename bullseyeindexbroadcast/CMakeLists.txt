# Minimum CMake required
cmake_minimum_required(VERSION 3.15)

# Minimum C++ required
set(CMAKE_CXX_STANDARD 17)

# Project
project(bullseyeindexbroadcast)

# mongocxx
find_package(libmongocxx REQUIRED)
find_package(libbsoncxx REQUIRED)
include_directories(${LIBMONGOCXX_INCLUDE_DIR})
include_directories(${LIBBSONCXX_INCLUDE_DIR})

# cpprestsdk
find_package(cpprestsdk REQUIRED)

# Targets (client|server)
foreach(_target
  bullseyeindexbroadcast)
  add_executable(${_target} "${_target}.cc")
  target_link_libraries(${_target}
    ${_REFLECTION}
    ${LIBMONGOCXX_LIBRARIES}
    ${LIBBSONCXX_LIBRARIES}
    cpprestsdk::cpprest
  )
endforeach()
