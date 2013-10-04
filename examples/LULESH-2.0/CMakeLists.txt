find_package(UPCXX REQUIRED)
find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)

include_directories(${UPCXX_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

add_definitions(${UPCXX_DEFINES} -DUSE_MPI=0 -DUSE_UPCXX=1 -DUSE_OMP=1)

add_executable(lulesh2.0
   lulesh.cc
   lulesh-comm.cc
   lulesh-viz.cc
   lulesh-util.cc
   lulesh-init.cc
)

add_executable(lulesh2.0-upcxx
   lulesh.cc
   lulesh-comm-upcxx.cc
   lulesh-viz.cc
   lulesh-util.cc
   lulesh-init.cc
)

target_link_libraries(lulesh2.0 ${UPCXX_LIBRARIES})
target_link_libraries(lulesh2.0-upcxx ${UPCXX_LIBRARIES})
