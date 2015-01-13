################################################################
# Boilerplate text to be included by any DSC file using the
# Standard Libraries.
#
# The including DSC file must DEFINE the EMULATE macro if
# the application is to be run in an emulation environment.
#
#  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
################################################################

##########
#  Socket Support Libraries
##########

[LibraryClasses.common]
  BsdSocketLib|StdLib/BsdSocketLib/BsdSocketLib.inf
  EfiSocketLib|StdLib/EfiSocketLib/EfiSocketLib.inf
  UseSocketDxe|StdLib/UseSocketDxe/UseSocketDxe.inf

[LibraryClasses.Common.UEFI_APPLICATION]
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf

  #
  # C Standard Libraries
  #
  LibC|StdLib/LibC/LibC.inf
  LibCType|StdLib/LibC/Ctype/Ctype.inf
  LibLocale|StdLib/LibC/Locale/Locale.inf
  LibMath|StdLib/LibC/Math/Math.inf
  LibSignal|StdLib/LibC/Signal/Signal.inf
  LibStdio|StdLib/LibC/Stdio/Stdio.inf
  LibStdLib|StdLib/LibC/StdLib/StdLib.inf
  LibString|StdLib/LibC/String/String.inf
  LibTime|StdLib/LibC/Time/Time.inf
  LibUefi|StdLib/LibC/Uefi/Uefi.inf
  LibWchar|StdLib/LibC/Wchar/Wchar.inf

# Common Utilities for Networking Libraries
  LibNetUtil|StdLib/LibC/NetUtil/NetUtil.inf

# Additional libraries for POSIX functionality.
  LibPosix|StdLib/PosixLib/PosixLib.inf   # Combines LibErr, LibGen, LibGlob, LibStringlist, GetPass into one library

  LibErr|StdLib/PosixLib/Err/LibErr.inf
  LibGen|StdLib/PosixLib/Gen/LibGen.inf
  LibGlob|StdLib/PosixLib/Glob/LibGlob.inf
  LibStringlist|StdLib/PosixLib/Stringlist/LibStringlist.inf
  LibIIO|StdLib/LibC/Uefi/InteractiveIO/IIO.inf

# Additional, non-standard, libraries
  LibContainer|StdLib/LibC/Containers/ContainerLib.inf

# Libraries for device abstractions within the Standard C Library
# Applications should not directly access any functions defined in these libraries.
  LibGdtoa|StdLib/LibC/gdtoa/gdtoa.inf
  DevConsole|StdLib/LibC/Uefi/Devices/daConsole.inf
  DevShell|StdLib/LibC/Uefi/Devices/daShell.inf       # DEPRECATED!  Please use DevMedia for new code.
  DevMedia|StdLib/LibC/Uefi/Devices/daShell.inf
  DevUtility|StdLib/LibC/Uefi/Devices/daUtility.inf

  LuaLib|AppPkg/Applications/Lua/LuaLib.inf           # Lua language library

[LibraryClasses.ARM]
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

[LibraryClasses.AArch64]
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

[Components]
# BaseLib and BaseMemoryLib need to be built with the /GL- switch when using the Microsoft
# tool chain.  This is required so that the library functions can be resolved during
# the second pass of the linker during Link-time-code-generation.
###
  MdePkg/Library/BaseLib/BaseLib.inf {
    <BuildOptions>
      MSFT:*_*_*_CC_FLAGS    = /X /Zc:wchar_t /GL-
  }

  MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf {
    <BuildOptions>
      MSFT:*_*_*_CC_FLAGS    = /X /Zc:wchar_t /GL-
  }

##########
#  Socket Layer
##########
  StdLib/SocketDxe/SocketDxe.inf


##############################################################################
#
# See the additional comments below if you plan to run applications under an
# emulation environment.
#

[BuildOptions]
!ifdef $(EMULATE)
  # The Build Options, below, are only used when building the Standard Libraries
  # to be run under an emulation environment; such as NT32Pkg.
  # They disable optimization which facillitates debugging under the Emulation environment.
  INTEL:*_*_IA32_CC_FLAGS   = /Od /D UEFI_C_SOURCE
   MSFT:*_*_IA32_CC_FLAGS   = /Od /D UEFI_C_SOURCE
    GCC:*_*_IA32_CC_FLAGS   = -O0 -DUEFI_C_SOURCE
    RVCT:*_*_*_CC_FLAGS     = --library_interface=none -DUEFI_C_SOURCE -J$(WORKSPACE)/StdLib/Include -J$(WORKSPACE)/StdLib/Include/Arm
  ARMGCC:*_*_*_CC_FLAGS     = -O0 -DUEFI_C_SOURCE -Wno-unknown-pragmas -Wno-unused -Wno-format-zero-length
   XCODE:*_*_*_CC_FLAGS     = -O0 -DUEFI_C_SOURCE -Wno-unused-const-variable -Wno-string-compare -Wno-sometimes-uninitialized

!else
  # These Build Options are used when building the Standard Libraries to be run
  # on real hardware.
   INTEL:*_*_*_CC_FLAGS     = /Qfreestanding /D UEFI_C_SOURCE
    MSFT:*_*_*_CC_FLAGS     = /X /Zc:wchar_t /D UEFI_C_SOURCE
     GCC:*_*_*_CC_FLAGS     = -nostdinc -nostdlib -DUEFI_C_SOURCE
    RVCT:*_*_*_CC_FLAGS     = --library_interface=none -DUEFI_C_SOURCE -J$(WORKSPACE)/StdLib/Include -J$(WORKSPACE)/StdLib/Include/Arm
  ARMGCC:*_*_*_CC_FLAGS     = -nostdinc -nostdlib -DUEFI_C_SOURCE -Wno-unknown-pragmas -Wno-unused -Wno-format-zero-length
   XCODE:*_*_*_CC_FLAGS     = -nostdinc -nostdlib -DUEFI_C_SOURCE -Wno-unused-const-variable -Wno-string-compare -Wno-sometimes-uninitialized
!endif
