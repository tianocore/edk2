#!/bin/bash
#
# Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
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

PROCESSOR=
BUILDTARGET=DEBUG
BUILD_OPTIONS=
PLATFORMFILE=
LAST_ARG=
RUN_EMULATOR=no
CLEAN_TYPE=none
TARGET_TOOLS=GCC44
NETWORK_SUPPORT=
BUILD_NEW_SHELL=
BUILD_FAT=
HOST_PROCESSOR=X64

case `uname` in
  CYGWIN*) echo Cygwin not fully supported yet. ;;
  Darwin*)
      Major=$(uname -r | cut -f 1 -d '.')
      if [[ $Major == 9 ]]
      then
        echo UnixPkg requires Snow Leopard or later OS
        exit 1
      else
        CLANG_VER=$(clang -ccc-host-triple x86_64-pc-win32-macho 2>&1 >/dev/null) || true
        if [[ "$CLANG_VER" == *-ccc-host-triple* ]]
        then
        # only older versions of Xcode support -ccc-host-tripe, for newer versions
        # it is -target
          HOST_TOOLS=XCODE32
          TARGET_TOOLS=XCODE5
        else
          HOST_TOOLS=XCODE32
          TARGET_TOOLS=XCLANG
        fi
      fi
      BUILD_NEW_SHELL="-D BUILD_NEW_SHELL"
      BUILD_FAT="-D BUILD_FAT"
      ;;
  Linux*)
    case `uname -m` in
      i386)
        HOST_PROCESSOR=IA32
        ;;
      i686)
        HOST_PROCESSOR=IA32
        ;;
      x86_64)
        HOST_PROCESSOR=X64
        ;;
    esac

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
      4.8.*)
        TARGET_TOOLS=GCC48
        ;;
      4.9.*|4.1[0-9].*)
        TARGET_TOOLS=GCC49
        ;;
      *)
        TARGET_TOOLS=GCC44
        ;;
    esac
    ;;
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
      run)
        RUN_EMULATOR=yes
        shift
        break
        ;;
      clean|cleanall)
        CLEAN_TYPE=$arg
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
        HOST_TOOLS=$arg
        ;;
      *)
        BUILD_OPTIONS="$BUILD_OPTIONS $arg"
        ;;
    esac
    LAST_ARG=
  fi
  shift
done
if [ -z "$HOST_TOOLS" ]
then
  HOST_TOOLS=$TARGET_TOOLS
fi

if [ -z "$PROCESSOR" ]
then
  PROCESSOR=$HOST_PROCESSOR
fi

case $PROCESSOR in
  IA32)
    ARCH_SIZE=32
    BUILD_OUTPUT_DIR=$WORKSPACE/Build/Emulator32
    LIB_NAMES="ld-linux.so.2 libdl.so.2 crt1.o crti.o crtn.o"
    LIB_SEARCH_PATHS="/usr/lib/i386-linux-gnu /usr/lib32 /lib32 /usr/lib /lib"
    ;;
  X64)
    ARCH_SIZE=64
    BUILD_OUTPUT_DIR=$WORKSPACE/Build/Emulator
    LIB_NAMES="ld-linux-x86-64.so.2 libdl.so.2 crt1.o crti.o crtn.o"
    LIB_SEARCH_PATHS="/usr/lib/x86_64-linux-gnu /usr/lib64 /lib64 /usr/lib /lib"
    ;;
esac

for libname in $LIB_NAMES
do
  for dirname in $LIB_SEARCH_PATHS
  do
    if [ -e $dirname/$libname ]; then
      export HOST_DLINK_PATHS="$HOST_DLINK_PATHS $dirname/$libname"
      break
    fi
  done
done

PLATFORMFILE=$WORKSPACE/EmulatorPkg/EmulatorPkg.dsc
BUILD_DIR=$BUILD_OUTPUT_DIR/DEBUG_"$TARGET_TOOLS"
BUILD_ROOT_ARCH=$BUILD_DIR/$PROCESSOR

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


if [[ "$RUN_EMULATOR" == "yes" ]]; then
  case `uname` in
    Darwin*)
      #
      # On Darwin we can't use dlopen, so we have to load the real PE/COFF images.
      # This .gdbinit script sets a breakpoint that loads symbols for the PE/COFFEE
      # images that get loaded in Host
      #
      if [[ "$CLANG_VER" == *-ccc-host-triple* ]]
      then
      # only older versions of Xcode support -ccc-host-tripe, for newer versions
      # it is -target
        cp $WORKSPACE/EmulatorPkg/Unix/lldbefi.py $BUILD_OUTPUT_DIR/DEBUG_"$TARGET_TOOLS"/$PROCESSOR
        cd $BUILD_ROOT_ARCH; /usr/bin/lldb --source $WORKSPACE/EmulatorPkg/Unix/lldbinit Host
        exit $? 
      else
        cp $WORKSPACE/EmulatorPkg/Unix/.gdbinit $BUILD_OUTPUT_DIR/DEBUG_"$TARGET_TOOLS"/$PROCESSOR
      fi
      ;;
  esac

  /usr/bin/gdb $BUILD_ROOT_ARCH/Host -q -cd=$BUILD_ROOT_ARCH -x $WORKSPACE/EmulatorPkg/Unix/GdbRun
  exit
fi

case $CLEAN_TYPE in
  clean)
    build -p $WORKSPACE/EmulatorPkg/EmulatorPkg.dsc -a $PROCESSOR -b $BUILDTARGET -t $HOST_TOOLS -D UNIX_SEC_BUILD -n 3 clean
    build -p $WORKSPACE/EmulatorPkg/EmulatorPkg.dsc -a $PROCESSOR -b $BUILDTARGET -t $TARGET_TOOLS -n 3 clean
    exit $?
    ;;
  cleanall)
    make -C $WORKSPACE/BaseTools clean
    build -p $WORKSPACE/EmulatorPkg/EmulatorPkg.dsc -a $PROCESSOR -b $BUILDTARGET -t $HOST_TOOLS -D UNIX_SEC_BUILD -n 3 clean
    build -p $WORKSPACE/EmulatorPkg/EmulatorPkg.dsc -a $PROCESSOR -b $BUILDTARGET -t $TARGET_TOOLS -n 3 clean
    build -p $WORKSPACE/ShellPkg/ShellPkg.dsc -a IA32 -b $BUILDTARGET -t $TARGET_TOOLS -n 3 clean
    exit $?
    ;;
esac


#
# Build the edk2 EmulatorPkg
#
if [[ $HOST_TOOLS == $TARGET_TOOLS ]]; then
  build -p $WORKSPACE/EmulatorPkg/EmulatorPkg.dsc $BUILD_OPTIONS -a $PROCESSOR -b $BUILDTARGET -t $TARGET_TOOLS -D BUILD_$ARCH_SIZE -D UNIX_SEC_BUILD $NETWORK_SUPPORT $BUILD_NEW_SHELL $BUILD_FAT -n 3
else
  build -p $WORKSPACE/EmulatorPkg/EmulatorPkg.dsc $BUILD_OPTIONS -a $PROCESSOR -b $BUILDTARGET -t $HOST_TOOLS  -D BUILD_$ARCH_SIZE -D UNIX_SEC_BUILD -D SKIP_MAIN_BUILD -n 3 modules
  build -p $WORKSPACE/EmulatorPkg/EmulatorPkg.dsc $BUILD_OPTIONS -a $PROCESSOR -b $BUILDTARGET -t $TARGET_TOOLS -D BUILD_$ARCH_SIZE $NETWORK_SUPPORT $BUILD_NEW_SHELL $BUILD_FAT -n 3
  cp $BUILD_OUTPUT_DIR/DEBUG_"$HOST_TOOLS"/$PROCESSOR/Host $BUILD_ROOT_ARCH
fi
exit $?

