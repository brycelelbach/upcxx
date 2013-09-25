README for building and running UPC++
Last Updated: 09/25/2013

************************************************************
Directory  structure for UPC++
************************************************************
 
We assume "upcxx" is the top directory of the UPC++ source

Files for the library
include
src

Examples for UPC++
examples

Makefile configuration examples:
examples/make_config

************************************************************
Installation instructions for GASNet:
************************************************************

Configure gasnet in the build directory:
   mkdir gasnet_build
   cd gasnet_build
  
   ## a configure example with the Intel compiler, assuming everything
   ## (nvcc, icc, mpicc) is already in you path.

configure examples (suppose gasnet is source directory for GASNet):
   ## 1) a basic configure example with the Intel compilers
   ../gasnet/configure --prefix="$PWD/../gasnet_install" --enable-segment-everything --disable-aligned-segments CC=icc CXX=icpc

   ## 2) configure with "-g" and MPICH2
   ../gasnet/configure --prefix=/home/yzheng/gasnet_gpu_install CC="icc -g" CXX="icpc -g" --enable-segment-everything --disable-aligned-segments MPI_CC=/home/yzheng/mpich2-install/bin/mpicc MPI_CFLAGS=-I/home/yzheng/mpich2-install/include MPI_LIBS="-L/home/yzheng/mpich2-install/lib -lmpich -lopa -lmpl -lrt -lpthread"

If the configure script runs without errors, then start the building
process:
   
   make 
   make install

************************************************************
Compiling and linking examples with UPC++
************************************************************

We assume "gasnet_install" is the installation
directory where you install gasnet to in the previous step.

cd examples/gasnet/basic

Modify the Makefile to set the variables matching your system:

# root path for UPC++
ROOT ?= $(HOME)/upcxx

# GASNet 
GASNET_PATH ?=  _install

# select a network conduit for your system
# smp, mpi, udp, ibv 
# smp conduit for single-node jobs, mpi conduit for portable parallel
# jobs, native conduits (e.g., ibv) for high-performance parallel jobs
GASNET_CONDUIT = ibv

Then you may build the examples by "make".

You may use the Makefile in each example directory and the
make_config directory as a template to create new ones.


************************************************************
Running UPC++ apps with GASNet
************************************************************

You may run a single node 

You may use "mpirun" or 'gasnetrun" in the gasnet_install/bin
directory to start up a parallel job.

mpirun -np 2 ./hello

gasnet_install_path/bin/gasnetrun_mpi -n 2 ./hello

************************************************************
Solving common problems
************************************************************

1) If you see a compile time error like the following:

"/home/yzheng/gasnet_install/include/gasnet_basic.h:397:6: error: #error "GCC's __may_alias__ attribute is required for correctness in gcc >= 4.4, but is disabled or unsupported."

  You may work around this by comment out the "#error" line in the
  header file, which in this case is "gasnet_basic.h:397".  This
  problem is because the GASNet configure script doesn't recognize the
  CUDA compiler and treats it as gcc.
