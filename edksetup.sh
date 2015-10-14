#
# Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# In *inux environment, the build tools's source is required and need to be compiled
# firstly, please reference https://edk2.tianocore.org/unix-getting-started.html to 
# to get how to setup build tool.
#
# After build tool is downloaded and compiled, a soft symbol linker need to be created
# at <workspace>/Conf. For example: ln -s /work/BaseTools /work/edk2/Conf/BaseToolsSource.
#
# Setup the environment for unix-like systems running a bash-like shell.
# This file must be "sourced" not merely executed. For example: ". edksetup.sh"
#
# CYGWIN users: Your path and filename related environment variables should be
# set up in the unix style.  This script will make the necessary conversions to
# windows style.
#
# Please reference edk2 user manual for more detail descriptions at https://edk2.tianocore.org/files/documents/64/494/EDKII_UserManual.pdf
#

function HelpMsg()
{
  echo Please note: This script must be \'sourced\' so the environment can be changed.
  echo ". edksetup.sh" 
  echo "source edksetup.sh"
  return 1
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

  if [ ! ${BASH_SOURCE[0]} -ef ./edksetup.sh ]
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
    . $EDK_TOOLS_PATH/BuildEnv $*
  elif [ -f "$WORKSPACE/BaseTools/BuildEnv" ]
  then
    . $WORKSPACE/BaseTools/BuildEnv $*
  elif [ -n "$PACKAGES_PATH" ]
  then 
    PATH_LIST=$PACKAGES_PATH
    PATH_LIST=${PATH_LIST//:/ }
    for DIR in $PATH_LIST
    do
      if [ -f "$DIR/BaseTools/BuildEnv" ]
      then
        export EDK_TOOLS_PATH=$DIR/BaseTools
        . $DIR/BaseTools/BuildEnv $*
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
  if [ \
       "$1" = "-?" -o \
       "$1" = "-h" -o \
       "$1" = "--help" \
     ]
  then
    HelpMsg
  else
    SetWorkspace &&
    SetupEnv "$*"
  fi
}

if [ $# -gt 1 ]
then
  HelpMsg
elif [ $# -eq 1 ] && [ "$1" != "BaseTools" ]
then
  HelpMsg
fi

RETVAL=$?
if [ $RETVAL -ne 0 ]
then
  return $RETVAL
fi

SourceEnv "$*"

