## @file
# UnitTestFrameworkPkg DSC include file for host based test DSC
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc

[LibraryClasses.common.HOST_APPLICATION]
  BaseLib|MdePkg/Library/BaseLib/UnitTestHostBaseLib.inf
  UnitTestHostBaseLib|MdePkg/Library/BaseLib/UnitTestHostBaseLib.inf
  CpuLib|MdePkg/Library/BaseCpuLibNull/BaseCpuLibNull.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLibNull/BaseCacheMaintenanceLibNull.inf
  CmockaLib|UnitTestFrameworkPkg/Library/CmockaLib/CmockaLib.inf
  UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLibCmocka.inf
  DebugLib|UnitTestFrameworkPkg/Library/Posix/DebugLibPosix/DebugLibPosix.inf
  MemoryAllocationLib|UnitTestFrameworkPkg/Library/Posix/MemoryAllocationLibPosix/MemoryAllocationLibPosix.inf

[BuildOptions]
  GCC:*_*_*_CC_FLAGS = -fno-pie
!ifdef $(UNIT_TESTING_DEBUG)
  MSFT:*_*_*_CC_FLAGS  = -D UNIT_TESTING_DEBUG=1
  GCC:*_*_*_CC_FLAGS   = -D UNIT_TESTING_DEBUG=1
  XCODE:*_*_*_CC_FLAGS = -D UNIT_TESTING_DEBUG=1
!endif

[BuildOptions.common.EDKII.HOST_APPLICATION]
  #
  # MSFT
  #
  MSFT:*_*_*_DLINK_FLAGS            == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb" /IGNORE:4001 /NOLOGO /SUBSYSTEM:CONSOLE /DEBUG /STACK:0x40000,0x40000 /NODEFAULTLIB:libcmt.lib libcmtd.lib
  MSFT:*_*_IA32_DLINK_FLAGS         = /MACHINE:I386
  MSFT:*_*_X64_DLINK_FLAGS          = /MACHINE:AMD64

  MSFT:*_VS2015_IA32_DLINK_FLAGS    = /LIBPATH:"%VS2015_PREFIX%Lib" /LIBPATH:"%VS2015_PREFIX%VC\Lib" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"
  MSFT:*_VS2015x86_IA32_DLINK_FLAGS = /LIBPATH:"%VS2015_PREFIX%Lib" /LIBPATH:"%VS2015_PREFIX%VC\Lib" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"
  MSFT:*_VS2017_IA32_DLINK_FLAGS    = /LIBPATH:"%VCToolsInstallDir%lib\x86" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"
  MSFT:*_VS2019_IA32_DLINK_FLAGS    = /LIBPATH:"%VCToolsInstallDir%lib\x86" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"

  MSFT:*_VS2015_X64_DLINK_FLAGS     = /LIBPATH:"%VS2015_PREFIX%VC\Lib\AMD64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
  MSFT:*_VS2015x86_X64_DLINK_FLAGS  = /LIBPATH:"%VS2015_PREFIX%VC\Lib\AMD64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
  MSFT:*_VS2017_X64_DLINK_FLAGS     = /LIBPATH:"%VCToolsInstallDir%lib\x64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
  MSFT:*_VS2019_X64_DLINK_FLAGS     = /LIBPATH:"%VCToolsInstallDir%lib\x64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"

  #
  # GCC
  #
  GCC:*_*_IA32_DLINK_FLAGS == -o $(BIN_DIR)/$(BASE_NAME) -m32 -no-pie
  GCC:*_*_X64_DLINK_FLAGS  == -o $(BIN_DIR)/$(BASE_NAME) -m64 -no-pie
  GCC:*_*_*_DLINK2_FLAGS   == -lgcov

  #
  # Need to do this link via gcc and not ld as the pathing to libraries changes from OS version to OS version
  #
  XCODE:*_*_IA32_DLINK_PATH == gcc
  XCODE:*_*_IA32_CC_FLAGS = -I$(WORKSPACE)/EmulatorPkg/Unix/Host/X11IncludeHack
  XCODE:*_*_IA32_DLINK_FLAGS == -arch i386 -o $(BIN_DIR)/Host -L/usr/X11R6/lib -lXext -lX11 -framework Carbon
  XCODE:*_*_IA32_ASM_FLAGS == -arch i386 -g

  XCODE:*_*_X64_DLINK_PATH == gcc
  XCODE:*_*_X64_DLINK_FLAGS == -o $(BIN_DIR)/Host -L/usr/X11R6/lib -lXext -lX11 -framework Carbon -Wl,-no_pie
  XCODE:*_*_X64_ASM_FLAGS == -g
  XCODE:*_*_X64_CC_FLAGS = -O0 -target x86_64-apple-darwin -I$(WORKSPACE)/EmulatorPkg/Unix/Host/X11IncludeHack "-DEFIAPI=__attribute__((ms_abi))"
