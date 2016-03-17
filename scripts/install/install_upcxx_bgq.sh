#!/bin/bash
## shell script to build UPC++ on ALCF Mira (IBM BG/Q)

umask 022
set -x

## Collect basic system info
today=`date +%Y%m%d`

if test -n "$USE_GCC" ; then
    # GCC-based compilers
    compiler=gcc 
    CC='/bgsys/drivers/ppcfloor/gnu-linux/bin/powerpc64-bgq-linux-gcc -Wno-unused'
    CXX='/bgsys/drivers/ppcfloor/gnu-linux/bin/powerpc64-bgq-linux-g++ -Wno-unused'
elif test -n "$USE_CLANG" ; then
    # CLANG-based compilers 
    compiler=clang
    CC='bgclang -fno-integrated-as -Wno-unused -U__GNUC__'
    CXX='bgclang++11 -fno-integrated-as -Wno-unused -U__GNUC__'
else
    # XLC-based compilers 
    compiler=xlc 
    CC='bgxlc_r -qarch=qp -qtune=qp'
    CXX='bgxlC_r -qarch=qp -qtune=qp'
fi

upcxx_ver=upcxx-$compiler
gasnet_ver=gasnet-$compiler

GASNET_SRC_DIR=$HOME/gasnet
UPCXX_SRC_DIR=$HOME/upcxx

BUILD_DIR=$HOME/build/$today
GASNET_BUILD_DIR=$BUILD_DIR/$gasnet_ver
UPCXX_BUILD_DIR=$BUILD_DIR/$upcxx_ver

INSTALL_ROOT_DIR=$HOME/install/$today
GASNET_INSTALL_DIR=$INSTALL_ROOT_DIR/$gasnet_ver
UPCXX_INSTALL_DIR=$INSTALL_ROOT_DIR/$upcxx_ver

INSTALL_GASNET="${INSTALL_GASNET:-yes}"
echo Install GASNet? $INSTALL_GASNET

UPDATE_BUILD_STATUS="${UPDATE_BUILD_STATUS:-yes}"
echo Update Bitbucket Build Status? $UPDATE_BUILD_STATUS

## Build and install GASNet
if [ "$INSTALL_GASNET" == "yes" ]; then
    mkdir -p $GASNET_BUILD_DIR
    cd $GASNET_BUILD_DIR
    $GASNET_SRC_DIR/cross-configure-bgq --prefix=$GASNET_INSTALL_DIR
    make
    make install
fi

## Build and install UPC++
mkdir -p $UPCXX_BUILD_DIR
cd $UPCXX_BUILD_DIR
$UPCXX_SRC_DIR/configure --with-gasnet=$GASNET_INSTALL_DIR/include/pami-conduit/pami-seq.mak --enable-short-names --enable-md-array --enable-64bit-global-ptr --prefix=$UPCXX_INSTALL_DIR CC="$CC" CXX="$CXX"
make
make install

if [ ! -f $UPCXX_INSTALL_DIR/bin/upc++ ]; then
    echo "UPC++ installation failed!"
    export UPCXX_BUILD_STATE="FAILED"
else
    echo "UPC++ installation successful!"
    export UPCXX_BUILD_STATE="SUCCESSFUL"
fi

## find out the commit hash 
export UPCXX_GIT_COMMIT=`cd $UPCXX_SRC_DIR; git rev-parse HEAD`

## key
export UPCXX_BUILD_ID=$HOSTNAME-$compiler-`date +%Y%m%d-%H%M%S`

## optional
export UPCXX_BUILD_NAME="$UPCXX_INSTALL_DIR" 

## Use the NERSC url for Edison
export UPCXX_BUILD_URL="http://www.alcf.anl.gov/mira"

## optional
export UPCXX_BUILD_DESC=""

build_status_tmp_file="upcxx_build_status"-`date +%Y%m%d-%H%M%S`.json

echo "{" > $build_status_tmp_file
echo "	\"state\": \"$UPCXX_BUILD_STATE\"," >> $build_status_tmp_file
echo "	\"key\": \"$UPCXX_BUILD_ID\"," >> $build_status_tmp_file 
echo "	\"url\": \"$UPCXX_BUILD_URL\"" >> $build_status_tmp_file
echo "}" >> $build_status_tmp_file

if [ "$UPDATE_BUILD_STATUS" == "yes" ]; then
    curl -u $UPCXX_REPO_USER:$UPCXX_REPO_KEY -H "Content-Type: application/json" -X POST https://api.bitbucket.org/2.0/repositories/upcxx/upcxx/commit/$UPCXX_GIT_COMMIT/statuses/build -d @$build_status_tmp_file
fi

echo ""
