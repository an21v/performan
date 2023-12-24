#! /bin/bash

CURRENT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BUILD_DIR=$CURRENT_DIR/build                                                    
                                                                                
pushd $CURRENT_DIR
cmake --build $BUILD_DIR
popd
