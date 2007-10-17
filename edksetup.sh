#
# Copyright (c) 2006 - 2007, Intel Corporation
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
     "$1" = "-?" -o \
     "$1" = "-h" -o \
     "$1" = "--help" \
   ]
then
  echo BaseTools Usage: \'. edksetup.sh NewBuild\'
  echo Ant Tools Usage: \'. edksetup.sh [ForceRebuild]\'
  echo
  echo Please note: This script must be \'sourced\' so the environment can be changed.
  echo \(Either \'. edksetup.sh\' or \'source edksetup.sh\'\)
  return
fi

if [ "$1" = NewBuild ]
then
  echo To utilize the new build system, run
  echo "  . BaseTools/BuildEnv [options]"
  echo Try \'. BaseTools/BuildEnv --help\' for more information.
  return
else
  . $WORKSPACE/Tools/OldBuildEnv $*
fi


