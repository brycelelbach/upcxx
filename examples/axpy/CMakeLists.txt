find_package(UPCXX)

include_directories(${UPCXX_INCLUDE_PATH})
link_directories(${UPCXX_LIBRARY_PATH})
add_definitions(${UPCXX_DEFINES})

add_executable(axpy axpy.cpp)
target_link_libraries(axpy ${UPCXX_LIBRARIES})
