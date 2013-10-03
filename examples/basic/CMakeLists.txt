include_directories(${GASNET_INCLUDE_PATH} ${GASNET_CONDUIT_INCLUDE_PATH})

add_executable(hello hello.cpp)
target_link_libraries(hello ${GASNET_LIBRARIES} upcxx)

add_executable(test_shared_var test_shared_var.cpp)
target_link_libraries(test_shared_var ${GASNET_LIBRARIES} upcxx)

add_executable(test_ptr_to_shared test_ptr_to_shared.cpp)
target_link_libraries(test_ptr_to_shared ${GASNET_LIBRARIES} upcxx)

add_executable(testperf2 testperf2.cpp)
target_link_libraries(testperf2 ${GASNET_LIBRARIES} upcxx)

add_executable(test_async test_async.cpp)
target_link_libraries(test_async ${GASNET_LIBRARIES} upcxx)

add_executable(test_event test_event.cpp)
target_link_libraries(test_event ${GASNET_LIBRARIES} upcxx)

add_executable(test_am_bcast test_am_bcast.cpp)
target_link_libraries(test_am_bcast ${GASNET_LIBRARIES} upcxx)

add_executable(test_shared_array test_shared_array.cpp)
target_link_libraries(test_shared_array ${GASNET_LIBRARIES} upcxx)

add_executable(test_reduce test_reduce.cpp)
target_link_libraries(test_reduce ${GASNET_LIBRARIES} upcxx)
