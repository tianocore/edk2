#!/bin/bash
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
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

#
# Configure defaults for various options
#

PROCESSOR=X64
BUILDTARGET=DEBUG
BUILD_OPTIONS=
PLATFORMFILE=
LAST_ARG=
RUN_QEMU=no

#
# Pick a default tool type for a given OS
#
TARGET_TOOLS=MYTOOLS
case `uname` in
  CYGWIN*)
    echo Cygwin not fully supported yet.
    ;;
  Darwin*)
      Major=$(uname -r | cut -f 1 -d '.')
      if [[ $Major == 9 ]]
      then
        echo OvmfPkg requires Snow Leopard or later OS
        exit 1
      else
        TARGET_TOOLS=XCODE32
      fi
      ;;
  Linux*)
    gcc_version=$(gcc -v 2>&1 | tail -1 | awk '{print $3}')
    case $gcc_version in
      4.5.*)
        TARGET_TOOLS=GCC45
        ;;
      4.6.*)
        TARGET_TOOLS=GCC46
        ;;
      *)
        TARGET_TOOLS=GCC44
        ;;
    esac
esac

#
# Scan command line to override defaults
#

for arg in "$@"
do
  if [ -z "$LAST_ARG" ]; then
    case $arg in
      -a|-b|-t|-p)
        LAST_ARG=$arg
        ;;
      qemu)
        RUN_QEMU=yes
        shift
        break
        ;;
      *)
        BUILD_OPTIONS="$BUILD_OPTIONS $arg"
        ;;
    esac
  else
    case $LAST_ARG in
      -a)
        PROCESSOR=$arg
        ;;
      -b)
        BUILDTARGET=$arg
        ;;
      -p)
        PLATFORMFILE=$arg
        ;;
      -t)
        TARGET_TOOLS=$arg
        ;;
      *)
        BUILD_OPTIONS="$BUILD_OPTIONS $arg"
        ;;
    esac
    LAST_ARG=
  fi
  shift
done

case $PROCESSOR in
  IA32)
    Processor=Ia32
    QEMU_COMMAND=qemu
    ;;
  X64)
    Processor=X64
    QEMU_COMMAND=qemu-system-x86_64
    ;;
  *)
    echo Unsupported processor architecture: $PROCESSOR
    echo Only IA32 or X64 is supported
    exit 1
    ;;
esac

if [ -z "$PLATFORMFILE" ]; then
  PLATFORMFILE=$WORKSPACE/OvmfPkg/OvmfPkg$Processor.dsc
fi

ADD_QEMU_HDA=yes
for arg in "$@"
do
  case $arg in
    -hd[a-d]|-fd[ab]|-cdrom)
      ADD_QEMU_HDA=no
      break
      ;;
  esac
done

#
# Uncomment this block for parameter parsing debug
#
#echo RUN_QEMU=$RUN_QEMU
#echo BUILD_OPTIONS=$BUILD_OPTIONS
#echo BUILDTARGET=$BUILDTARGET
#echo TARGET_TOOLS=$TARGET_TOOLS
#echo PROCESSOR=$PROCESSOR
#echo Remaining for qemu: $*
#exit 1

BUILD_ROOT=$WORKSPACE/Build/Ovmf$Processor/"$BUILDTARGET"_"$TARGET_TOOLS"
FV_DIR=$BUILD_ROOT/FV
BUILD_ROOT_ARCH=$BUILD_ROOT/$PROCESSOR
QEMU_FIRMWARE_DIR=$BUILD_ROOT/QEMU

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


if [[ "$RUN_QEMU" == "yes" ]]; then
  if [[ ! -d $QEMU_FIRMWARE_DIR ]]; then
    mkdir $QEMU_FIRMWARE_DIR
  fi
  ln -sf $FV_DIR/OVMF.fd $QEMU_FIRMWARE_DIR/bios.bin
  ln -sf $FV_DIR/OvmfVideo.rom $QEMU_FIRMWARE_DIR/vgabios-cirrus.bin
  if [[ "$ADD_QEMU_HDA" == "yes" ]]; then
    AUTO_QEMU_HDA="-hda fat:$BUILD_ROOT_ARCH"
  else
    AUTO_QEMU_HDA=
  fi
  QEMU_COMMAND="$QEMU_COMMAND -L $QEMU_FIRMWARE_DIR $AUTO_QEMU_HDA $*"
  echo Running: $QEMU_COMMAND
  $QEMU_COMMAND
  exit $?
fi

#
# Build the edk2 OvmfPkg
#
echo Running edk2 build for OvmfPkg$Processor
build -p $PLATFORMFILE $BUILD_OPTIONS -a $PROCESSOR -b $BUILDTARGET -t $TARGET_TOOLS
exit $?

