#!/bin/bash
#
# External makefile Xcode project project uses this script to build and clean from the Xcode GUI
#
# Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

# force exit on error
set -e

#
# Source the workspace and set up the environment variables we need
#
cd ../../..
./build.sh -a IA32 $1 $2 $3 $4 $5 $6 $8
