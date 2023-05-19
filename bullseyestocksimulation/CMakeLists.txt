# Minimum CMake required
cmake_minimum_required(VERSION 3.15)

# Project
project(bullseyestocksimulation)

# mongocxx
find_package(libmongocxx REQUIRED)
find_package(libbsoncxx REQUIRED)
include_directories(${LIBMONGOCXX_INCLUDE_DIR})
include_directories(${LIBBSONCXX_INCLUDE_DIR})

# Targets (client|server)
foreach(_target
  stock_simulator)
  add_executable(${_target} "${_target}.cc")
  target_link_libraries(${_target}
    ${_REFLECTION}
    ${LIBMONGOCXX_LIBRARIES}
    ${LIBBSONCXX_LIBRARIES}
  )
endforeach()