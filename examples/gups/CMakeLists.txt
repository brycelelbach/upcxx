find_package(UPCXX REQUIRED)

include_directories(${UPCXX_INCLUDE_DIR})
link_directories(${UPCXX_LIBRARY_PATH})
add_definitions(${UPCXX_DEFINES})

add_executable(gups gups.cpp)
target_link_libraries(gups ${UPCXX_LIBRARIES})
