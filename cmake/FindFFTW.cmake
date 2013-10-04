# Try to find UPCXX library and include path.
# Once done this will define
#
# FFTW_FOUND
# FFTW_CONDUIT_INCLUDE_DIR
# FFTW_INCLUDE_DIR
# FFTW_LIBRARIES
#

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

find_package_handle_standard_args(FFTW DEFAULT_MSG
    FFTW_INCLUDE_DIR
    FFTW_LIBRARY_PATH
)
