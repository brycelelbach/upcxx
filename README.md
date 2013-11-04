### UPC\+\+ is a PGAS extension for C\+\+. ###

## Installing UPC\+\+ ##

###  1. Obtain the upcxx source code from the git repo ###

```
#!sh
     git clone git@bitbucket.org:upcxx/upcxx.git
```


### 2. Install GASNet.  Please see https://bitbucket.org/upcxx/upcxx/wiki/GASNet%20Installation ###

### 3. Configure and build UPC\+\+ ###

``` 
#!sh
## Generate the configure script 
cd upcxx
autoreconf -i
cd ..

## Run the configure script in the build directory and make
## /path/to/${conduit}-seq.mak is the absolute path to the GASNet
## Makefile fragment. E.g, use
## "bupc_path/opt/opt/include/aries-conduit/aries-seq.mak" for the
## GASNet aries conduit on Edison (Cray XC30).  Or use
## "gasnet_install_path/include/mpi-conduit/mpi-seq.mak" for the
## GASNet mpi conduit.  
## Change "-seq" to "-par" to use the thread-safe version of gasnet

mkdir upcxx_build
cd upcxx_build
../upcxx/configure --with-gasnet=/path/to/${conduit}-seq.mak --prefix=/path/to/install
make
make install

## Check the UPC++ installation
cd /path/to/install
```