#!/bin/bash
#
# Integration build test script for all components
#
#  * Build all components using example platform of the SyS-T library.
#  * Run the "hello" example and feed it's output into the printer.
#
#  The script will end with printing "Hello SyS-T" on success.
set -e
SELF_DIR="$(cd `dirname "${BASH_SOURCE[0]}"` && pwd)"

#BUILDMODE=Debug
BUILDMODE=MinSizeRel
BLD_ROOT=${BLD_ROOT:="$SELF_DIR/build"}
DEPLOY_DIR="$BLD_ROOT/sdk"
mkdir -p $DEPLOY_DIR

echo
echo ----- Building SyS-T library ... ----------------------------------------
echo

mkdir -p $BLD_ROOT/lib
pushd $BLD_ROOT/lib
cmake \
    -DSYST_BUILD_TEST=ON \
    -DSYST_BUILD_DOC=ON \
    -DSYST_BUILD_PLATFORM_NAME=example \
    -DCMAKE_INSTALL_PREFIX="$DEPLOY_DIR" \
    -DCMAKE_BUILD_TYPE=$BUILDMODE \
    "$SELF_DIR/../../library"

cmake --build . --target install
cmake --build . --target RUN_TEST_VERBOSE
pushd

echo
echo ----- Building Examples ... ---------------------------------------------
echo

mkdir -p $BLD_ROOT/examples
pushd $BLD_ROOT/examples

cmake \
    -DCMAKE_INSTALL_PREFIX="$DEPLOY_DIR" \
    -DSYST_SDK="$DEPLOY_DIR" \
    -DCMAKE_BUILD_TYPE=$BUILDMODE \
    "$SELF_DIR/.."

cmake --build . --target install
pushd

echo
echo ----- Building printer ... ----------------------------------------------
echo

mkdir -p $BLD_ROOT/printer
pushd $BLD_ROOT/printer

cmake \
    -DCMAKE_INSTALL_PREFIX="$DEPLOY_DIR" \
    -DCMAKE_BUILD_TYPE=$BUILDMODE \
    "$SELF_DIR/../../printer"

cmake --build . --target install
cmake --build . --target RUN_TEST_VERBOSE
pushd


echo
echo ----- Running the SyS-T 'hello world' example  --------------------------
echo
pushd "$DEPLOY_DIR/bin" > /dev/null
echo "$PWD/hello | $PWD/systprint -p -"
echo
./hello | ./systprint -p -
popd > /dev/null
exit 0
