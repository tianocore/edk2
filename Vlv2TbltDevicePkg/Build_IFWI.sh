#!/usr/bin/env bash
##**********************************************************************
## Function define
##**********************************************************************
function Usage ( ) {
  echo
  echo "Script to build BIOS firmware and stitch the entire IFWI."
  echo
  echo "Usage: Build_IFWI.bat   PlatformType  BuildTarget  "
  echo
  echo 
  echo "       Platform Types:   MNW2"
  echo "       Build Targets:    Release, Debug"
  echo
  echo "       See  Stitch/Stitch_Config.txt  for additional stitching settings."
  echo
  echo
  exit 0
}

## Assign initial values
exitCode=0
Build_Flags=
Stitch_Flags=
Arch=X64
PLATFORM_PACKAGE=Vlv2TbltDevicePkg

## Parse Optional arguments
if [ "$1" == "/?" ]; then
  Usage
fi

for (( i=1; i<=$#; ))
  do
    if [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/Q" ]; then
      Build_Flags="$Build_Flags /q"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/L" ]; then
      Build_Flags="$Build_Flags /l"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/C" ]; then
      Build_Flags="$Build_Flags /c"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/ECP" ]; then
      Build_Flags="$Build_Flags /ecp"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/X64" ]; then
      Arch=X64
      Build_Flags="$Build_Flags /x64"
      shift
    elif [ "$1" == "/nG" ]; then
      Stitch_Flags="$Stitch_Flags /nG"
      shift
    elif [ "$1" == "/nM" ]; then
      Stitch_Flags="$Stitch_Flags /nM"
      shift
    elif [ "$1" == "/nB" ]; then
      Stitch_Flags="$Stitch_Flags /nB"
      shift
    elif [ "$1" == "/nV" ]; then
      Stitch_Flags="$Stitch_Flags /nV"
      shift
    else
      break
    fi
  done

## Require 2 input parameters
if [ "$2" == "" ]; then
  Usage
fi

## Assign required arguments
Platform_Type=$1
Build_Target=$2
if [ "$3" == "" ]; then
  IFWI_Suffix=
else
  IFWI_Suffix="/S $3"
fi

## Go to root directory
cd ..

## Build BIOS
echo "======================================================================"
echo "Build_IFWI:  Calling BIOS build Script..."
./$PLATFORM_PACKAGE/bld_vlv.sh $Build_Flags $Platform_Type $Build_Target

echo
echo Finished Building BIOS.

## Start Integration process
echo ======================================================================
echo Skip "Build_IFWI:  Calling IFWI Stitching Script..."

echo
echo Build_IFWI is finished.
echo The final IFWI file is located in Stitch
echo ======================================================================
