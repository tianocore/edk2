#!/bin/bash
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

set -e
shopt -s nocasematch


#
# Setup workspace if it is not set
#
if [ -z "$WORKSPACE" ]
then
  echo Initializing workspace
  if [ ! -e `pwd`/edksetup.sh ]
  then
    cd ..
  fi
# This version is for the tools in the BaseTools project.
# this assumes svn pulls have the same root dir
#  export EDK_TOOLS_PATH=`pwd`/../BaseTools
# This version is for the tools source in edk2
  export EDK_TOOLS_PATH=`pwd`/BaseTools
  echo $EDK_TOOLS_PATH
  source edksetup.sh BaseTools
else
  echo Building from: $WORKSPACE
fi

PROCESSOR=X64

#
# Pick a default tool type for a given OS
#
TARGET_TOOLS=MYTOOLS
case `uname` in
  CYGWIN*) echo Cygwin not fully supported yet. ;;
  Darwin*)
      Major=$(uname -r | cut -f 1 -d '.')
      if [[ $Major == 9 ]]
      then
        echo DuetPkg requires Snow Leopard or later OS
        exit 1
      else
        TARGET_TOOLS=XCODE32
      fi
      ;;
  Linux*)
    TARGET_TOOLS=GCC44
    ;;

esac

BUILD_ROOT_ARCH=$WORKSPACE/Build/DuetPkg$PROCESSOR/DEBUG_"$TARGET_TOOLS"/$PROCESSOR
FLOPPY_IMAGE=$WORKSPACE/Build/DuetPkg$PROCESSOR/floppy.img

if  [[ ! -f `which build` || ! -f `which GenFv` ]];
then
  # build the tools if they don't yet exist. Bin scheme
  echo Building tools as they are not in the path
  make -C $WORKSPACE/BaseTools
elif [[ ( -f `which build` ||  -f `which GenFv` )  && ! -d  $EDK_TOOLS_PATH/Source/C/bin ]];
then
  # build the tools if they don't yet exist. BinWrapper scheme
  echo Building tools no $EDK_TOOLS_PATH/Source/C/bin directory
  make -C $WORKSPACE/BaseTools
else
  echo using prebuilt tools
fi


for arg in "$@"
do
  if [[ $arg == qemu ]]; then
    shift
    qemu-system-x86_64 -fda $FLOPPY_IMAGE -boot a $*
    exit
  fi

  if [[ $arg == cleanall ]]; then
    make -C $WORKSPACE/BaseTools clean
    build -p $WORKSPACE/DuetPkg/DuetPkg$PROCESSOR.dsc -a $PROCESSOR -t $TARGET_TOOLS -n 3 clean
    exit $?
  fi

  if [[ $arg == clean ]]; then
    build -p $WORKSPACE/DuetPkg/DuetPkg$PROCESSOR.dsc -a $PROCESSOR -t $TARGET_TOOLS -n 3 clean
    exit $?
  fi
done


#
# Build the edk2 DuetPkg
#
echo Running edk2 build for DuetPkg$PROCESSOR
build -p $WORKSPACE/DuetPkg/DuetPkg$PROCESSOR.dsc -a $PROCESSOR -t $TARGET_TOOLS -n 3 $*
echo Running DuetPkg/PostBuild.sh
$WORKSPACE/DuetPkg/PostBuild.sh $PROCESSOR $TARGET_TOOLS
echo Running DuetPkg/CreateBootDisk.sh

$WORKSPACE/DuetPkg/CreateBootDisk.sh file $FLOPPY_IMAGE /dev/null FAT12 $PROCESSOR $TARGET_TOOLS
exit $?

