#!/bin/bash
#
# External makefile Xcode project project uses this script to build and clean from the Xcode GUI
#
# Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

# force exit on error
set -e

#
# Source the workspace and set up the environment variables we need
#
cd ../../..
./build.sh -a IA32 $1 $2 $3 $4 $5 $6 $8
