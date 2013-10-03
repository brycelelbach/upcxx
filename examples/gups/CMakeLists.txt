include_directories(${GASNET_INCLUDE_PATH} ${GASNET_CONDUIT_INCLUDE_PATH})

add_executable(gups gups.cpp)
target_link_libraries(gups ${GASNET_LIBRARIES} upcxx)
