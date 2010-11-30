#!/bin/bash
#
# External makefile Xcode project project uses this script to build and clean from the Xcode GUI
#

# force exit on error
set -e

#
# Source the workspace and set up the environment variables we need
#
cd ../..
./build.sh $1 $2 $3 $4 $5 $6 $8
