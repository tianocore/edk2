#!/bin/bash
#
# External makefile Xcode project project uses this script to build and clean from the Xcode GUI
#

# force exit on error
set -e

#
# Source the workspace and set up the environment varaibles we need
#
cd ../..
echo `pwd`
./build.sh
