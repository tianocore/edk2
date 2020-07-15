## @file
# GDB startup script
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

#
# Gdb will set $_exitcode when the program exits.  Pre-init it to an unlikely
# return value.
#
set $_exitcode = 42

#
# Gdb will call hook-stop on each break.  Check to see if $_exitcode was
# changed from the value we pre-initialized it to.  If so, the program
# had exited, so gdb should now quit.
#
define hook-stop
  if $_exitcode != 42
    quit
  else
    source Host.gdb
  end
end

#
# We keep track of the number of symbol files we have loaded via gdb
# scripts in the $SymbolFilesAdded variable
#
set $SymbolFileChangesCount = 0

#
# This macro adds a symbols file for gdb
#
# @param  $arg0 - Symbol file changes number
# @param  $arg1 - Symbol file name
# @param  $arg2 - Image address
#
define AddFirmwareSymbolFile
  if $SymbolFileChangesCount < $arg0
    add-symbol-file $arg1 $arg2
    set $SymbolFileChangesCount = $arg0
  end
end

#
# This macro removes a symbols file for gdb
#
# @param  $arg0 - Symbol file changes number
# @param  $arg1 - Symbol file name
#
define RemoveFirmwareSymbolFile
  if $SymbolFileChangesCount < $arg0
    #
    # Currently there is not a method to remove a single symbol file
    #
    set $SymbolFileChangesCount = $arg0
  end
end

if gInXcode == 1
  # in Xcode the program is already running. Issuing a run command
  # will cause a fatal debugger error. The break point script that
  # is used to source this script sets gInCode to 1.
else
  #
  # Start the program running
  #
  run
end
