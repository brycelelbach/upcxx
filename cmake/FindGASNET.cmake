# Try to find UPCXX library and include path.
# Once done this will define
#
# GASNET_FOUND
# GASNET_CONDUIT_INCLUDE_DIR
# GASNET_INCLUDE_DIR
# GASNET_LIBRARIES
#

# TODO: handle different conduit/configurations

include(FindPackageHandleStandardArgs)

find_package(MPI REQUIRED)

find_path(GASNET_INCLUDE_DIR
    NAMES
        gasnet.h
    PATHS
        /Users/mbdriscoll/opt/gasnet-1.20.2/include
        ${GASNET_LOCATION}/include
        $ENV{GASNET_LOCATION}/include
    DOC
        "The directory where gasnet.h resides"
)

find_path(GASNET_CONDUIT_INCLUDE_DIR
    NAMES
        gasnet_core.h
    PATHS
        /Users/mbdriscoll/opt/gasnet-1.20.2/include/mpi-conduit
        ${GASNET_LOCATION}/include/mpi-conduit
        $ENV{GASNET_LOCATION}/include/mpi-conduit
    DOC
        "The directory where gasnet.h resides"
)

find_path(GASNET_LIBRARY_PATH
    NAMES
        libgasnet-mpi-seq.a
    PATHS
        /Users/mbdriscoll/opt/gasnet-1.20.2/lib
        ${GASNET_LOCATION}/lib
        $ENV{GASNET_LOCATION}/lib
    DOC
        "The gasnet library"
)

add_definitions( -DGASNET_SEQ -DGASNETT_USE_GCC_ATTRIBUTE_MAYALIAS )

SET(GASNET_LIBRARIES gasnet-mpi-seq ammpi ${MPI_CXX_LIBRARIES})

find_package_handle_standard_args(GASNET DEFAULT_MSG
    GASNET_INCLUDE_DIR
    GASNET_CONDUIT_INCLUDE_DIR
    GASNET_LIBRARY_PATH
)
