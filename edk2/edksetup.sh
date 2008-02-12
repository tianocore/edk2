#
# Copyright (c) 2006 - 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

# Setup the environment for unix-like systems running a bash-like shell.
# This file must be "sourced" not merely executed. For example: ". edksetup.sh"

# CYGWIN users: Your path and filename related environment variables should be
# set up in the unix style.  This script will make the necessary conversions to
# windows style.

if [ \
     -z "$1" -o \
     "$1" = "-?" -o \
     "$1" = "-h" -o \
     "$1" = "--help" \
   ]
then
  echo BaseTools Usage: \'. edksetup.sh BaseTools\'
  echo Ant Tools Usage: \'. edksetup.sh [AntBuild \| ForceRebuild]\'
  echo
  echo Please note: This script must be \'sourced\' so the environment can be changed.
  echo \(Either \'. edksetup.sh\' or \'source edksetup.sh\'\)
  return
fi

if [ "$1" = BaseTools ]
then
  if [ -z "$WORKSPACE" ]
  then
    . BaseTools/BuildEnv $*
  else
    . $WORKSPACE/BaseTools/BuildEnv $*
  fi
else
  if [ "$1" = AntBuild -o "$1" = ForceRebuild ]
  then
    if [ -z "$WORKSPACE" ]
    then
      if [ "$1" = AntBuild ]
      then
        shift
      fi
      . Tools/OldBuildEnv $*
    else
      . $WORKSPACE/Tools/OldBuildEnv $*
    fi
  else
    echo Please run \'. edksetup.sh --help\' for help.
  fi
fi


