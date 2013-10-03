include_directories(${GASNET_INCLUDE_PATH} ${GASNET_CONDUIT_INCLUDE_PATH})

add_executable(axpy axpy.cpp)
target_link_libraries(axpy ${GASNET_LIBRARIES} upcxx)
