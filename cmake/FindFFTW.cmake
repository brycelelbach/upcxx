# Try to find FFTW library and include path.
# Once done this will define
#
# FFTW_FOUND
# FFTW_INCLUDE_DIR
# FFTW_LIBRARY_PATH
# FFTW_LIBRARIES
#

# mbd: I wrote this when I didn't have an internet connection,
# so there may be a production-quality one available online.

include(FindPackageHandleStandardArgs)

find_path(FFTW_INCLUDE_DIR
    NAMES
        fftw3.h
    PATHS
        /opt/local/include
        ${FFTW_LOCATION}/include
        $ENV{FFTW_LOCATION}/include
    DOC
        "The directory where fftw3.h resides"
)

find_path(FFTW_LIBRARY_PATH
    NAMES
        libfftw3.a
    PATHS
        /opt/local/lib
        ${FFTW_LOCATION}/lib
        $ENV{FFTW_LOCATION}/lib
    DOC
        "The fftw library"
)

set (FFTW_LIBRARIES fftw3 fftw3_threads)

find_package_handle_standard_args(FFTW DEFAULT_MSG
    FFTW_INCLUDE_DIR
    FFTW_LIBRARY_PATH
)
