include_directories(${GASNET_INCLUDE_PATH} ${GASNET_CONDUIT_INCLUDE_PATH})

find_package(BLAS)
find_package(FFTW)
find_package(OpenMP)

link_directories(${FFTW_LIBRARY_PATH})
link_directories(${BLAS_LINKER_FLAGS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

add_executable(fft2d fft2d.cpp myfft_fftw3.c)
target_link_libraries(fft2d ${GASNET_LIBRARIES} fftw3 fftw3_threads ${BLAS_LIBRARIES} upcxx)
