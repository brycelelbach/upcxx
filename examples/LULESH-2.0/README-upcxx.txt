README for the UPC++ version of LULESH

To build the UPC++ version of LULESH, please first specify the
installation path of UPC++, e.g.,
export UPCXX_INSTALL_DIR=/home/yzheng/upcxx_install

Then "make" and it should build both the MPI and UPC++ version of
LULESH.

If you want to use OpenMP or other features in LULESH, please modify
the compiler settings in the Makefile.

For example, to use Intel compilers with OpenMP, set the following in
Makefile:

CXX = icpc -fast -DUSE_OMP=1 -fopenmp 
MPICXX = mpicxx -fast -DUSE_OMP=1 -fopenmp
UPCXX = mpicxx -fast -DUSE_OMP=1 -fopenmp
