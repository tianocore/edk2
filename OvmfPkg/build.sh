#!/bin/bash
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
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
THREADNUMBER=1
LAST_ARG=
RUN_QEMU=no
ENABLE_FLASH=no

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
    case $Major in
      10)
        TARGET_TOOLS=XCODE32
        ;;
      1[12])
        TARGET_TOOLS=XCLANG
        ;;
       *)
        echo OvmfPkg requires Snow Leopard or later OS
        exit 1
        ;;
    esac
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
      4.7.*)
        TARGET_TOOLS=GCC47
        ;;
      4.[8-9].*)
        TARGET_TOOLS=GCC48
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
      -a|-b|-t|-p|-n)
        LAST_ARG=$arg
        ;;
      qemu)
        RUN_QEMU=yes
        shift
        break
        ;;
      --enable-flash)
        ENABLE_FLASH=yes
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
      -n)
        THREADNUMBER=$arg
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
    if [ -n "$QEMU_COMMAND" ]; then
      #
      # The user set the QEMU_COMMAND variable. We'll use it to run QEMU.
      #
      :
    elif  [ -x `which qemu-system-i386` ]; then
      QEMU_COMMAND=qemu-system-i386
    elif  [ -x `which qemu-system-x86_64` ]; then
      QEMU_COMMAND=qemu-system-x86_64
    elif  [ -x `which qemu` ]; then
      QEMU_COMMAND=qemu
    else
      echo Unable to find QEMU for IA32 architecture!
      exit 1
    fi
    ;;
  X64)
    Processor=X64
    if [ -z "$QEMU_COMMAND" ]; then
      #
      # The user didn't set the QEMU_COMMAND variable.
      #
      QEMU_COMMAND=qemu-system-x86_64
    fi
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

if [[ "$RUN_QEMU" == "yes" ]]; then
  qemu_version=$($QEMU_COMMAND -version 2>&1 | tail -1 | awk '{print $4}')
  case $qemu_version in
    1.[6-9].*|1.[1-9][0-9].*|2.*.*)
      ENABLE_FLASH=yes
      ;;
  esac

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
fi

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
  if [[ "$ENABLE_FLASH" == "yes" ]]; then
    QEMU_COMMAND="$QEMU_COMMAND -pflash $QEMU_FIRMWARE_DIR/bios.bin"
  else
    QEMU_COMMAND="$QEMU_COMMAND -L $QEMU_FIRMWARE_DIR"
  fi
  if [[ "$ADD_QEMU_HDA" == "yes" ]]; then
    QEMU_COMMAND="$QEMU_COMMAND -hda fat:$BUILD_ROOT_ARCH"
  fi
  QEMU_COMMAND="$QEMU_COMMAND $*"
  echo Running: $QEMU_COMMAND
  $QEMU_COMMAND
  exit $?
fi

#
# Build the edk2 OvmfPkg
#
echo Running edk2 build for OvmfPkg$Processor
build -p $PLATFORMFILE $BUILD_OPTIONS -a $PROCESSOR -b $BUILDTARGET -t $TARGET_TOOLS -n $THREADNUMBER
exit $?

