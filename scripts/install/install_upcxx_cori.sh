#!/bin/bash
## shell script to build UPC++ on NERSC Cori Phase 1 (Cray XC40)

umask 022
set -x

## Collect basic system info
today=`date +%Y%m%d`
pe_env=`echo $PE_ENV | tr '[:upper:]' '[:lower:]'`
upcxx_ver=$today-$pe_env
gasnet_ver=$today-$pe_env

GASNET_SRC_DIR=$HOME/gasnet
UPCXX_SRC_DIR=$HOME/upcxx

BUILD_DIR=$HOME/build
GASNET_BUILD_DIR=$BUILD_DIR/gasnet-$NERSC_HOST/$gasnet_ver
UPCXX_BUILD_DIR=$BUILD_DIR/upcxx-$NERSC_HOST/$upcxx_ver

#PROJECT_DIR=/usr/common/usg/degas
PROJECT_DIR=$HOME/public
GASNET_INSTALL_DIR=$PROJECT_DIR/gasnet-$NERSC_HOST/$gasnet_ver
UPCXX_INSTALL_DIR=$PROJECT_DIR/upcxx-$NERSC_HOST/$upcxx_ver

INSTALL_GASNET="${INSTALL_GASNET:-yes}"
echo Install GASNet? $INSTALL_GASNET

UPDATE_BUILD_STATUS="${UPDATE_BUILD_STATUS:-no}"
echo Update Bitbucket Build Status? $UPDATE_BUILD_STATUS

## Build and install GASNet
if [ "$INSTALL_GASNET" == "yes" ]; then
    mkdir -p $GASNET_BUILD_DIR
    cd $GASNET_BUILD_DIR
    $GASNET_SRC_DIR/cross-configure-crayxc-linux MPIRUN_CMD="srun -K0 %V -m block:block --cpu_bind=cores -n%N %C" CC="cc -g" --prefix=${GASNET_INSTALL_DIR} --disable-pshm-hugetlbfs --enable-pshm-xpmem
    make
    make install
fi

## Build and install UPC++
mkdir -p $UPCXX_BUILD_DIR
cd $UPCXX_BUILD_DIR
$UPCXX_SRC_DIR/configure --with-gasnet=$GASNET_INSTALL_DIR/include/aries-conduit/aries-seq.mak --enable-short-names --enable-md-array --enable-64bit-global-ptr --prefix=$UPCXX_INSTALL_DIR CC="cc -g" CXX="CC -g"
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
export UPCXX_BUILD_ID=$NERSC_HOST-$pe_env-`date +%Y%m%d-%H%M%S`

## optional
export UPCXX_BUILD_NAME="$UPCXX_INSTALL_DIR" 

## Use the NERSC url for Edison
export UPCXX_BUILD_URL="http://www.nersc.gov/users/computational-systems/cori/cori-phase-i/"

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
