# mbd: this is a guess. Cmake will compain wihout a min version.
cmake_minimum_required(VERSION 2.6) # mbd: this is a guess

project(UPCXX)

# look for cmake modules in $ROOT/cmake directory
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# configure GASNet
find_package(GASNET REQUIRED)
include_directories(${GASNET_INCLUDE_DIR} ${GASNET_CONDUIT_INCLUDE_DIR})
link_directories(${GASNET_LIBRARY_PATH})

# include project headers
include_directories(${UPCXX_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(examples)
