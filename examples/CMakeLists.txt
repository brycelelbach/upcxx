message(STATUS "Skipping directory LULESH: undefined upcxx-related functions when linking 'lulesh2.0'.")
#add_subdirectory(LULESH-2.0)

message(STATUS "Skipping directory axpy: requires missing degas.h file.")
#add_subdirectory(axpy)

add_subdirectory(fft2d)
add_subdirectory(basic)
add_subdirectory(gups)
