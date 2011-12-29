#!/bin/bash
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
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

function process_debug_scripts {
  if [[ -d $1 ]]; then
    for filename in `ls $1`
    do
      sed -e "s@ZZZZZZ@$BUILD_ROOT@g" -e "s@WWWWWW@$WORKSPACE@g" \
            "$1/$filename" \
            > "$BUILD_ROOT/$filename"

      #For ARMCYGWIN, we have to change /cygdrive/c to c:
      if [[ $TARGET_TOOLS == RVCT31CYGWIN ]]
      then
        mv "$BUILD_ROOT/$filename" "$BUILD_ROOT/$filename"_temp
        sed -e "s@/cygdrive/\(.\)@\1:@g" \
              "$BUILD_ROOT/$filename"_temp \
              > "$BUILD_ROOT/$filename"
        rm -f "$BUILD_ROOT/$filename"_temp
      fi
    done
  fi
}


#
# Setup workspace if it is not set
#
if [ -z "${WORKSPACE:-}" ]
then
  echo Initializing workspace
  cd ..
# Uses an external BaseTools project 
#  export EDK_TOOLS_PATH=`pwd`/../BaseTools
# Uses the BaseTools in edk2
  export EDK_TOOLS_PATH=`pwd`/BaseTools
  source edksetup.sh BaseTools
else
  echo Building from: $WORKSPACE
fi

#
# Pick a default tool type for a given OS if no toolchain already defined
#
if [ -z "${TARGET_TOOLS:-}" ]
then
  case `uname` in
    CYGWIN*) 
      TARGET_TOOLS=RVCT31CYGWIN 
      ;;
    Linux*)  
      if [[ ! -z `locate arm-linux-gnueabi-gcc` ]]; then
        TARGET_TOOLS=ARMLINUXGCC
      else 
        TARGET_TOOLS=ARMGCC 
      fi
      ;;
    Darwin*) 
      Major=$(uname -r | cut -f 1 -d '.')
      if [[ $Major == 9 ]]
      then
        # Not supported by this open source project
        TARGET_TOOLS=XCODE31
      else 
        TARGET_TOOLS=XCODE32
      fi  
      ;;
  esac
fi

TARGET=DEBUG
for arg in "$@"
do
  if [[ $arg == RELEASE ]]; 
  then
    TARGET=RELEASE
  fi
done

BUILD_ROOT=$WORKSPACE/Build/BeagleBoard/"$TARGET"_"$TARGET_TOOLS"
GENERATE_IMAGE=$WORKSPACE/BeagleBoardPkg/Tools/generate_image
FLASH_BOOT=$BUILD_ROOT/FV/BeagleBoard_EFI_flashboot.fd

if  [[ ! -e $EDK_TOOLS_PATH/Source/C/bin ]];
then
  # build the tools if they don't yet exist
  echo Building tools: $EDK_TOOLS_PATH
  make -C $EDK_TOOLS_PATH
else
  echo using prebuilt tools
fi

#
# Build the edk2 BeagleBoard code
#
if [[ $TARGET == RELEASE ]]; then
  build -p $WORKSPACE/BeagleBoardPkg/BeagleBoardPkg.dsc -a ARM -t $TARGET_TOOLS -b $TARGET -D DEBUG_TARGET=RELEASE ${2:-} ${3:-} ${4:-} ${5:-} ${6:-} ${7:-} ${8:-}
else
  build -p ${WORKSPACE:-}/BeagleBoardPkg/BeagleBoardPkg.dsc -a ARM -t $TARGET_TOOLS -b $TARGET ${1:-} ${2:-} ${3:-} ${4:-} ${5:-} ${6:-} ${7:-} ${8:-}
fi


for arg in "$@"
do
  if [[ $arg == clean ]]; then
    # no need to post process if we are doing a clean
    exit
  elif [[ $arg == cleanall ]]; then
    make -C $EDK_TOOLS_PATH clean
    make -C $WORKSPACE/BeagleBoardPkg/Tools clean
    exit
    
  fi
done


#
#  Build the tool used to patch the FLASH image to work with the Beagle board ROM
#
if [[ ! -e $GENERATE_IMAGE ]];
then
  make -C $WORKSPACE/BeagleBoardPkg/Tools
fi

echo Patching FD to work with BeagleBoard ROM
rm -f $FLASH_BOOT

#
# Ram starts at 0x80000000
# OMAP 3530 TRM defines 0x80008000 as the entry point
# The reset vector is caught by the mask ROM in the OMAP 3530 so that is why this entry 
# point looks so strange. 
# OMAP 3430 TRM section 26.4.8 has Image header information. (missing in OMAP 3530 TRM)
#
$GENERATE_IMAGE -D $WORKSPACE/BeagleBoardPkg/ConfigurationHeader.dat -E 0x80008000 -I $BUILD_ROOT/FV/BEAGLEBOARD_EFI.fd -O $FLASH_BOOT

echo Creating debugger scripts
process_debug_scripts $WORKSPACE/BeagleBoardPkg/Debugger_scripts

