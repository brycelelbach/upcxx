# Try to find UPCXX library and include path.
# Once done this will define
#
# UPCXX_FOUND
# UPCXX_INCLUDE_DIR
# UPCXX_LIBRARY_PATH
# UPCXX_LIBRARIES
#

include(FindPackageHandleStandardArgs)

find_package(GASNET REQUIRED)

find_path(UPCXX_INCLUDE_DIR
    NAMES
        upcxx.h

    PATHS
        ${UPCXX_SOURCE_DIR}/include

        /Users/mbdriscoll/opt/upcxx-0.0.0/include
        ${UPCXX_LOCATION}/include
        $ENV{UPCXX_LOCATION}/include
    DOC
        "The directory where upcxx.h resides"
)

find_path( UPCXX_LIBRARY_PATH
    NAMES
        libupcxx.a

    PATHS
        ${UPCXX_BINARY_DIR}/src

        /Users/mbdriscoll/opt/upcxx-0.0.0/lib
        ${UPCXX_LOCATION}/lib
        ${UPCXX_LOCATION}/build
        $ENV{UPCXX_LOCATION}/lib
        $ENV{UPCXX_LOCATION}/build
    DOC
        "The upcxx core library"
)

find_package_handle_standard_args(UPCXX DEFAULT_MSG
    UPCXX_INCLUDE_DIR
    UPCXX_LIBRARY_PATH
)

set (UPCXX_LIBRARIES upcxx ${GASNET_LIBRARIES})
set (UPCXX_INCLUDE_DIR ${UPCXX_INCLUDE_DIR} ${GASNET_INCLUDE_DIR} ${GASNET_CONDUIT_INCLUDE_DIR})
set (UPCXX_LIBRARY_PATH ${UPCXX_LIBRARY_PATH} ${GASNET_LIBRARY_PATH})
set (UPCXX_DEFINES ${GASNET_DEFINES})
