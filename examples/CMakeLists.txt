message(STATUS "Skipping directory LULESH: undefined upcxx-related functions when linking 'lulesh2.0'.")
#add_subdirectory(LULESH-2.0)

message(STATUS "Skipping directory axpy: requires missing degas.h file.")
#add_subdirectory(axpy)

message(STATUS "Skipping directory fft2d: requires MKL and FFTW support in build system.")
#add_subdirectory(fft2d)

add_subdirectory(basic)
add_subdirectory(gups)
