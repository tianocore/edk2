#
# Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# In *inux environment, the build tools's source is required and need to be compiled
# firstly, please reference https://github.com/tianocore/tianocore.github.io/wiki/SourceForge-to-Github-Quick-Start
# to get how to setup build tool.
#
# Setup the environment for unix-like systems running a bash-like shell.
# This file must be "sourced" not merely executed. For example: ". edksetup.sh"
#
# CYGWIN users: Your path and filename related environment variables should be
# set up in the unix style.  This script will make the necessary conversions to
# windows style.
#
# Please reference edk2 user manual for more detail descriptions at https://github.com/tianocore-docs/Docs/raw/master/User_Docs/EDK_II_UserManual_0_7.pdf
#

SCRIPTNAME="edksetup.sh"
RECONFIG=FALSE

function HelpMsg()
{
  echo "Usage: $SCRIPTNAME [Options]"
  echo
  echo "The system environment variable, WORKSPACE, is always set to the current"
  echo "working directory."
  echo
  echo "Options: "
  echo "  --help, -h, -?        Print this help screen and exit."
  echo
  echo "  --reconfig            Overwrite the WORKSPACE/Conf/*.txt files with the"
  echo "                        template files from the BaseTools/Conf directory."
  echo
  echo Please note: This script must be \'sourced\' so the environment can be changed.
  echo ". $SCRIPTNAME"
  echo "source $SCRIPTNAME"
}

function SetWorkspace()
{
  #
  # If WORKSPACE is already set, then we can return right now
  #
  if [ -n "$WORKSPACE" ]
  then
    return 0
  fi

  if [ ! ${BASH_SOURCE[0]} -ef ./edksetup.sh ] && [ -z "$PACKAGES_PATH" ]
  then
    echo Run this script from the base of your tree.  For example:
    echo "  cd /Path/To/Edk/Root"
    echo "  . edksetup.sh"
    return 1
  fi

  #
  # Check for BaseTools/BuildEnv before dirtying the user's environment.
  #
  if [ ! -f BaseTools/BuildEnv ] && [ -z "$EDK_TOOLS_PATH" ]
  then
    echo BaseTools not found in your tree, and EDK_TOOLS_PATH is not set.
    echo Please point EDK_TOOLS_PATH at the directory that contains
    echo the EDK2 BuildEnv script.
    return 1
  fi

  #
  # Set $WORKSPACE
  #
  export WORKSPACE=`pwd`

  return 0
}

function SetupEnv()
{
  if [ -n "$EDK_TOOLS_PATH" ]
  then
    . $EDK_TOOLS_PATH/BuildEnv
  elif [ -f "$WORKSPACE/BaseTools/BuildEnv" ]
  then
    . $WORKSPACE/BaseTools/BuildEnv
  elif [ -n "$PACKAGES_PATH" ]
  then 
    PATH_LIST=$PACKAGES_PATH
    PATH_LIST=${PATH_LIST//:/ }
    for DIR in $PATH_LIST
    do
      if [ -f "$DIR/BaseTools/BuildEnv" ]
      then
        export EDK_TOOLS_PATH=$DIR/BaseTools
        . $DIR/BaseTools/BuildEnv
        break
      fi
    done
  else
    echo BaseTools not found in your tree, and EDK_TOOLS_PATH is not set.
    echo Please check that WORKSPACE or PACKAGES_PATH is not set incorrectly
    echo in your shell, or point EDK_TOOLS_PATH at the directory that contains
    echo the EDK2 BuildEnv script.
    return 1
  fi
}

function SourceEnv()
{
  SetWorkspace &&
  SetupEnv
}

I=$#
while [ $I -gt 0 ]
do
  case "$1" in
    BaseTools)
      # Ignore argument for backwards compatibility
      shift
    ;;
    --reconfig)
      RECONFIG=TRUE
      shift
    ;;
    -?|-h|--help|*)
      HelpMsg
      break
    ;;
  esac
  I=$(($I - 1))
done

if [ $I -gt 0 ]
then
  return 1
fi

SourceEnv

unset SCRIPTNAME RECONFIG

return $?
