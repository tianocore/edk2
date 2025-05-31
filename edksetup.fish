#
# Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
# Copyright (c) 2025, VoltagedDebunked. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
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

set -gx EDK_TOOLS_PATH (pwd)/BaseTools
set -g SCRIPTNAME edksetup.fish
set -g RECONFIG FALSE

function HelpMsg
  echo "Usage: $SCRIPTNAME [Options]"
  echo
  echo "This script must be 'sourced' so the environment can be changed."
  echo "  source $SCRIPTNAME"
end

function SetWorkspace
  set -gx PYTHONHASHSEED 1

  if set -q WORKSPACE
    return 0
  end

  if not test -f "$SCRIPTNAME" -a -z "$PACKAGES_PATH"
    echo "Source this script from the base of your tree."
    echo "  cd /Path/To/Edk2/Clone"
    echo "  source $SCRIPTNAME"
    return 1
  end

  if not test -f "BaseTools/BuildEnv" -a -z "$EDK_TOOLS_PATH"
    echo "BaseTools not found, and EDK_TOOLS_PATH is not set."
    return 1
  end

  set -gx WORKSPACE (pwd)
  return 0
end

function SetupEnv
  if set -q EDK_TOOLS_PATH
    set buildenv "$EDK_TOOLS_PATH/BuildEnv"
  else if test -f "$WORKSPACE/BaseTools/BuildEnv"
    set buildenv "$WORKSPACE/BaseTools/BuildEnv"
  else if set -q PACKAGES_PATH
    for dir in (string split ":" $PACKAGES_PATH)
      if test -f "$dir/BaseTools/BuildEnv"
        set -gx EDK_TOOLS_PATH "$dir/BaseTools"
        set buildenv "$dir/BaseTools/BuildEnv"
        break
      end
    end
  else
    echo "Could not find BuildEnv."
    return 1
  end

  # Run BuildEnv in bash and capture EDK/WORKSPACE/PATH variables
  for line in (bash -c "source $buildenv > /dev/null 2>&1 && env" | grep -E '^(EDK_|WORKSPACE|PATH)=')
    set name (string match -r '^[^=]+' -- $line)
    set value (string replace "$name=" "" -- $line)
    # Only set valid variable names
    if string match -rq '^[a-zA-Z_][a-zA-Z0-9_]*$' -- $name
      set -gx $name $value
    end
  end
end

function SetupPythonCommand
  if not set -q PYTHON_COMMAND
    set -gx PYTHON_COMMAND python3
  end
end

function SourceEnv
  SetupPythonCommand
  SetWorkspace
  SetupEnv
end

# Parse args
for arg in $argv
  switch $arg
    case --help -h -\?
      HelpMsg
      return 0
    case --reconfig
      set RECONFIG TRUE
    case '*'
      HelpMsg
      return 1
  end
end

SourceEnv

# Clean up
set -e SCRIPTNAME
set -e RECONFIG
