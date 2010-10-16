#!/bin/sh

## @file
#
#  Currently, Build system does not provide post build mechanism for module 
#  and platform building, so just use a sh file to do post build commands.
#  Originally, following post building command is for EfiLoader module.
#
#  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

if [ -z "$EDK_TOOLS_PATH" ]
then
export BASETOOLS_DIR=$WORKSPACE/Conf/BaseToolsSource/Source/C/bin
else
export BASETOOLS_DIR=$EDK_TOOLS_PATH/Source/C/bin
fi

export BOOTSECTOR_BIN_DIR=$WORKSPACE/DuetPkg/BootSector/bin
export PROCESSOR=""
if [ \
     -z "$1" -o \
     "$1" = "-?" -o \
     "$1" = "-h" -o \
     "$1" = "--help" \
   ]
then
	echo Error! Please specific the architecture. 
	echo Usage: "./PostBuild.sh [IA32|X64] [UNIXGCC|GCC44]"
fi

case "$1" in
   IA32)
     export PROCESSOR=IA32
     ;;
   X64)
     export PROCESSOR=X64
     ;;
   *)
     echo Invalid Architecture string, should be only IA32 or X64
     return 1
esac

case "$2" in
   UNIXGCC)
     export TOOLTAG=UNIXGCC
     ;;
   GCC44)
     export TOOLTAG=GCC44
     ;;
   *)
     echo Invalid tool tag, should be only UNIXGCC or GCC44
     return 1
esac

export BUILD_DIR=$WORKSPACE/Build/DuetPkg$PROCESSOR/DEBUG_$TOOLTAG


#
# Boot sector module could only be built under IA32 tool chain
#

echo Compressing DUETEFIMainFv.FV ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DUETEFIMAINFV.z $BUILD_DIR/FV/DUETEFIMAINFV.Fv

echo Compressing DxeMain.efi ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/$PROCESSOR/DxeCore.efi

echo Compressing DxeIpl.efi ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/$PROCESSOR/DxeIpl.efi	

echo Generate Loader Image ...

if [ $PROCESSOR = IA32 ]
then
        $BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
	$BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr32 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/DUETEFIMAINFV.z
	cat $BOOTSECTOR_BIN_DIR/Start.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32   > $BUILD_DIR/FV/Efildr
	#
	# It is safe to use "bcat" to cat following binary file, if bcat command is avaiable for your system
	#
	#bcat -o $BUILD_DIR/FV/Efildr.bcat $BOOTSECTOR_BIN_DIR/start.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32
	cat $BOOTSECTOR_BIN_DIR/Start16.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr16
	#bcat -o $BUILD_DIR/FV/Efildr16.bcat $BOOTSECTOR_BIN_DIR/start16.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32
	cat $BOOTSECTOR_BIN_DIR/Start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr20	
	#bcat -o $BUILD_DIR/FV/Efildr20.bcat $BOOTSECTOR_BIN_DIR/start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32
	echo Done!
fi

if [ $PROCESSOR = X64 ]
then
         $BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
	$BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr64 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/DUETEFIMAINFV.z
	cat $BOOTSECTOR_BIN_DIR/Start64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/EfildrPure
	#bcat -o $BUILD_DIR/FV/EfildrPure $BOOTSECTOR_BIN_DIR/start64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 
	$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/EfildrPure -o $BUILD_DIR/FV/Efildr
	cat $BOOTSECTOR_BIN_DIR/St16_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr16Pure
	#bcat -o $BUILD_DIR/FV/Efildr16Pure $BOOTSECTOR_BIN_DIR/st16_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64
	$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/Efildr16Pure -o $BUILD_DIR/FV/Efildr16
	cat $BOOTSECTOR_BIN_DIR/St32_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
	#bcat -o $BUILD_DIR/FV/Efildr20Pure $BOOTSECTOR_BIN_DIR/st32_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64
	$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildr20
	
	echo Done!
fi


