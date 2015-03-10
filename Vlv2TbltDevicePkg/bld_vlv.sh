#!/usr/bin/env bash
##**********************************************************************
## Function define
##**********************************************************************
function Usage() {
  echo
  echo "***************************************************************************"
  echo "Build BIOS rom for VLV platforms."
  echo
  echo "Usage: bld_vlv.bat  PlatformType [Build Target]"
  echo
  echo
  echo "       Platform Types:  MNW2"
  echo "       Build Targets:   Debug, Release  (default: Debug)"
  echo
  echo "***************************************************************************"
  echo "Press any key......"
  read
  exit 0
}


echo -e $(date)
##**********************************************************************
## Initial Setup
##**********************************************************************
#WORKSPACE=$(pwd)
#build_threads=($NUMBER_OF_PROCESSORS)+1
Build_Flags=
exitCode=0
Arch=X64

## Clean up previous build files.
if [ -e $(pwd)/EDK2.log ]; then
  rm $(pwd)/EDK2.log
fi

if [ -e $(pwd)/Unitool.log ]; then
  rm $(pwd)/Unitool.log
fi

if [ -e $(pwd)/Conf/target.txt ]; then
  rm $(pwd)/Conf/target.txt
fi

if [ -e $(pwd)/Conf/BiosId.env ]; then
  rm $(pwd)/Conf/BiosId.env
fi

if [ -e $(pwd)/Conf/tools_def.txt ]; then
  rm $(pwd)/Conf/tools_def.txt
fi

if [ -e $(pwd)/Conf/build_rule.txt ]; then
  rm $(pwd)/Conf/build_rule.txt
fi


## Setup EDK environment. Edksetup puts new copies of target.txt, tools_def.txt, build_rule.txt in WorkSpace\Conf
## Also run edksetup as soon as possible to avoid it from changing environment variables we're overriding
. edksetup.sh BaseTools
make -C BaseTools

## Define platform specific environment variables.
PLATFORM_PACKAGE=Vlv2TbltDevicePkg
config_file=$WORKSPACE/$PLATFORM_PACKAGE/PlatformPkgConfig.dsc
auto_config_inc=$WORKSPACE/$PLATFORM_PACKAGE/AutoPlatformCFG.txt

## default ECP (override with /ECP flag)
EDK_SOURCE=$WORKSPACE/EdkCompatibilityPkg

## create new AutoPlatformCFG.txt file
if [ -f "$auto_config_inc" ]; then
  rm $auto_config_inc
fi
touch $auto_config_inc

##**********************************************************************
## Parse command line arguments
##**********************************************************************

## Optional arguments
for (( i=1; i<=$#; ))
  do
    if [ "$1" == "/?" ]; then
      Usage
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/Q" ]; then
      Build_Flags="$Build_Flags --quiet"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/L" ]; then
      Build_Flags="$Build_Flags -j EKD2.log"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/C" ]; then
      echo Removing previous build files ...
      if [ -d "Build" ]; then
        rm -r Build
      fi
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/ECP" ]; then
      ECP_SOURCE=$WORKSPACE/EdkCompatibilityPkgEcp
      EDK_SOURCE=$WORKSPACE/EdkCompatibilityPkgEcp
      echo DEFINE ECP_BUILD_ENABLE = TRUE >> $auto_config_inc
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/X64" ]; then
      Arch=X64
      shift
    else
      break
    fi
  done





## Required argument(s)
if [ "$2" == "" ]; then
  Usage
fi

## Remove the values for Platform_Type and Build_Target from BiosIdX.env and stage in Conf
if [ $Arch == "IA32" ]; then
  cp $PLATFORM_PACKAGE/BiosIdR.env    Conf/BiosId.env
  echo DEFINE X64_CONFIG = FALSE      >> $auto_config_inc
else
  cp $PLATFORM_PACKAGE/BiosIdx64R.env  Conf/BiosId.env
  echo DEFINE X64_CONFIG = TRUE       >> $auto_config_inc
fi
sed -i '/^BOARD_ID/d' Conf/BiosId.env
sed -i '/^BUILD_TYPE/d' Conf/BiosId.env



## -- Build flags settings for each Platform --
##    AlpineValley (ALPV):  SVP_PF_BUILD = TRUE,   ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
##       BayleyBay (BBAY):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = TRUE,   TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
##         BayLake (BLAK):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = TRUE,   BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
##      Bakersport (BYTI):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = TRUE, IVI_PF_BUILD = FALSE
## Crestview Hills (CVHS):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = TRUE, IVI_PF_BUILD = TRUE
##            FFD8 (BLAK):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = TRUE,   BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
echo "Setting  $1  platform configuration and BIOS ID..."
if [ "$(echo $1 | tr 'a-z' 'A-Z')" == "MNW2" ]; then
  echo BOARD_ID = MNW2MAX             >> Conf/BiosId.env
  echo DEFINE ENBDT_PF_BUILD = TRUE  >> $auto_config_inc
else
  echo "Error - Unsupported PlatformType: $1"
  Usage
fi

Platform_Type=$1

if [ "$(echo $2 | tr 'a-z' 'A-Z')" == "RELEASE" ]; then
  TARGET=RELEASE
  BUILD_TYPE=R
  echo BUILD_TYPE = R >> Conf/BiosId.env
else
  TARGET=DEBUG
  BUILD_TYPE=D
  echo BUILD_TYPE = D >> Conf/BiosId.env
fi


##**********************************************************************
## Additional EDK Build Setup/Configuration
##**********************************************************************
echo "Ensuring correct build directory is present for GenBiosId..."

echo Modifing Conf files for this build...
## Remove lines with these tags from target.txt
sed -i '/^ACTIVE_PLATFORM/d' Conf/target.txt
sed -i '/^TARGET /d' Conf/target.txt
sed -i '/^TARGET_ARCH/d' Conf/target.txt
sed -i '/^TOOL_CHAIN_TAG/d' Conf/target.txt
sed -i '/^MAX_CONCURRENT_THREAD_NUMBER/d' Conf/target.txt

ACTIVE_PLATFORM=$PLATFORM_PACKAGE/PlatformPkgGcc"$Arch".dsc
TOOL_CHAIN_TAG=GCC46
MAX_CONCURRENT_THREAD_NUMBER=1
echo ACTIVE_PLATFORM = $ACTIVE_PLATFORM                           >> Conf/target.txt
echo TARGET          = $TARGET                                    >> Conf/target.txt
echo TOOL_CHAIN_TAG  = $TOOL_CHAIN_TAG                            >> Conf/target.txt
echo MAX_CONCURRENT_THREAD_NUMBER = $MAX_CONCURRENT_THREAD_NUMBER >> Conf/target.txt
if [ $Arch == "IA32" ]; then
  echo TARGET_ARCH   = IA32                                       >> Conf/target.txt
else
  echo TARGET_ARCH   = IA32 X64                                   >> Conf/target.txt
fi

##**********************************************************************
## Build BIOS
##**********************************************************************
echo Skip "Running UniTool..."
echo "Make GenBiosId Tool..."
BUILD_PATH=Build/$PLATFORM_PACKAGE/"$TARGET"_"$TOOL_CHAIN_TAG"
if [ ! -d "$BUILD_PATH/$Arch" ]; then
  mkdir -p $BUILD_PATH/$Arch
fi
if [ -e "$BUILD_PATH/$Arch/BiosId.bin" ]; then
  rm -f $BUILD_PATH/$Arch/BiosId.bin
fi


./$PLATFORM_PACKAGE/GenBiosId -i Conf/BiosId.env -o $BUILD_PATH/$Arch/BiosId.bin


echo "Invoking EDK2 build..."
build


##**********************************************************************
## Post Build processing and cleanup
##**********************************************************************

echo Skip "Running fce..."

echo Skip "Running KeyEnroll..."

## Set the Board_Id, Build_Type, Version_Major, and Version_Minor environment variables
VERSION_MAJOR=$(grep '^VERSION_MAJOR' Conf/BiosId.env | cut -d ' ' -f 3 | cut -c 1-4)
VERSION_MINOR=$(grep '^VERSION_MINOR' Conf/BiosId.env | cut -d ' ' -f 3 | cut -c 1-2)
BOARD_ID=$(grep '^BOARD_ID' Conf/BiosId.env | cut -d ' ' -f 3 | cut -c 1-7)
BIOS_Name="$BOARD_ID"_"$Arch"_"$BUILD_TYPE"_"$VERSION_MAJOR"_"$VERSION_MINOR".ROM
BIOS_ID="$BOARD_ID"_"$Arch"_"$BUILD_TYPE"_"$VERSION_MAJOR"_"$VERSION_MINOR"_GCC.bin
cp -f $BUILD_PATH/FV/VLV.fd  $WORKSPACE/$BIOS_Name
SEC_VERSION=1.0.2.1067
cat ./$PLATFORM_PACKAGE/Stitch/IFWIHeader/IFWI_HEADER.bin ./Vlv2MiscBinariesPkg/SEC/$SEC_VERSION/VLV_SEC_REGION.bin ./$PLATFORM_PACKAGE/Stitch/IFWIHeader/Vacant.bin $BIOS_Name > ./$PLATFORM_PACKAGE/Stitch/$BIOS_ID


echo Skip "Running BIOS_Signing ..."

echo
echo Build location:     $BUILD_PATH
echo BIOS ROM Created:   $BIOS_Name
echo
echo -------------------- The EDKII BIOS build has successfully completed. --------------------
echo
