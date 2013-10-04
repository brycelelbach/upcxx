find_package(UPCXX)
find_package(BLAS)
find_package(FFTW)
find_package(OpenMP)

include_directories(${UPCXX_INCLUDE_PATH})

link_directories(${UPCXX_LIBRARY_PATH})
link_directories(${FFTW_LIBRARY_PATH})
link_directories(${BLAS_LINKER_FLAGS})

add_definitions(${UPCXX_DEFINES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

add_executable(fft2d fft2d.cpp myfft_fftw3.c)
target_link_libraries(fft2d ${UPCXX_LIBRARIES} ${FFTW_LIBRARIES} ${BLAS_LIBRARIES})
