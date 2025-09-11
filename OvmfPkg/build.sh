#!/usr/bin/env bash
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
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

ARCH_IA32=no
ARCH_X64=no
BUILDTARGET=DEBUG
BUILD_OPTIONS=
PLATFORMFILE=
THREADNUMBER=0
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
    # Major is Darwin version, not OS X version.
    # OS X Yosemite 10.10.2 returns 14.
    case $Major in
      [156789])
        echo OvmfPkg requires OS X Snow Leopard 10.6 or newer OS
        exit 1
        ;;
      10)
        TARGET_TOOLS=XCODE32
        ;;
      1[12])
        TARGET_TOOLS=XCLANG
        ;;
       *)
        # Mavericks and future assume XCODE5 (clang + lldb)
        TARGET_TOOLS=XCODE5
        ;;
    esac
    ;;
  Linux*)
    gcc_version=$(gcc -v 2>&1 | tail -1 | awk '{print $3}')
    case $gcc_version in
      [1-3].*|4.[0-7].*)
        echo OvmfPkg requires GCC4.8 or later
        exit 1
        ;;
      4.8.*)
        TARGET_TOOLS=GCC48
        ;;
      4.9.*|6.[0-2].*)
        TARGET_TOOLS=GCC49
        ;;
      *)
        TARGET_TOOLS=GCC5
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
        if [[ x"$arg" != x"IA32" && x"$arg" != x"X64" ]]; then
          echo Unsupported processor architecture: $arg
          echo Only IA32 or X64 is supported
          exit 1
        fi
        eval ARCH_$arg=yes
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

if [[ "$ARCH_IA32" == "yes" && "$ARCH_X64" == "yes" ]]; then
  PROCESSOR=IA32X64
  Processor=Ia32X64
  BUILD_OPTIONS="$BUILD_OPTIONS -a IA32 -a X64"
  PLATFORM_BUILD_DIR=Ovmf3264
  BUILD_ROOT_ARCH=X64
else
  PROCESSOR=X64
  Processor=X64
  BUILD_OPTIONS="$BUILD_OPTIONS -a X64"
  PLATFORM_BUILD_DIR=Ovmf$Processor
  BUILD_ROOT_ARCH=X64
fi

case $PROCESSOR in
  X64|IA32X64)
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
  qemu_version=$($QEMU_COMMAND -version 2>&1 | \
                   grep -o -E 'version [0-9]+\.[0-9]+\.[0-9]+' | \
                     awk '{print $2}')
  case $qemu_version in
    1.[6-9].*|[2-9].*.*|[1-9][0-9]*.*.*)
      ENABLE_FLASH=yes
      ;;
  esac
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

BUILD_ROOT=$WORKSPACE/Build/$PLATFORM_BUILD_DIR/"$BUILDTARGET"_"$TARGET_TOOLS"
FV_DIR=$BUILD_ROOT/FV
BUILD_ROOT_ARCH=$BUILD_ROOT/$BUILD_ROOT_ARCH
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

#
# Run previously built OVMF image for current build options, in place.
# Do not rebuild first, rather allow multiple runs of a previously built
# image to start quickly (without rebuild), and with preserved NVRAM contents
# between runs (until the next rebuild).
#
if [[ "$RUN_QEMU" == "yes" ]]; then
  if [[ ! -d $QEMU_FIRMWARE_DIR ]]; then
    mkdir $QEMU_FIRMWARE_DIR
  fi
  ln -sf $FV_DIR/OVMF.fd $QEMU_FIRMWARE_DIR/bios.bin
  if [[ "$ENABLE_FLASH" == "yes" ]]; then
    QEMU_COMMAND="$QEMU_COMMAND -drive if=pflash,format=raw,file=$QEMU_FIRMWARE_DIR/bios.bin"
  else
    QEMU_COMMAND="$QEMU_COMMAND -L $QEMU_FIRMWARE_DIR"
  fi
  echo Running: $QEMU_COMMAND "$@"
  $QEMU_COMMAND "$@"
  exit $?
fi

#
# Build the edk2 OvmfPkg
#
echo Running edk2 build for OvmfPkg$Processor
build -p $PLATFORMFILE $BUILD_OPTIONS -b $BUILDTARGET -t $TARGET_TOOLS -n $THREADNUMBER
exit $?
