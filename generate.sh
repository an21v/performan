#! /bin/bash

CURRENT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BUILD_DIR=$CURRENT_DIR/build

pushd $CURRENT_DIR
rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cmake -B $BUILD_DIR
popd
