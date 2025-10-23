## @file
# UnitTestFrameworkPkg DSC include file for host based test DSC
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgCommon.dsc.inc

[Defines]
  UNIT_TESTING_CODE_COVERAGE_ENABLE     = TRUE
  UNIT_TESTING_ADDRESS_SANITIZER_ENABLE = TRUE

[LibraryClasses.common.HOST_APPLICATION]
  BaseLib|MdePkg/Library/BaseLib/UnitTestHostBaseLib.inf
  UnitTestHostBaseLib|MdePkg/Library/BaseLib/UnitTestHostBaseLib.inf
  CpuLib|MdePkg/Library/BaseCpuLibNull/BaseCpuLibNull.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLibNull/BaseCacheMaintenanceLibNull.inf
  CmockaLib|UnitTestFrameworkPkg/Library/CmockaLib/CmockaLib.inf
  GoogleTestLib|UnitTestFrameworkPkg/Library/GoogleTestLib/GoogleTestLib.inf
  SubhookLib|UnitTestFrameworkPkg/Library/SubhookLib/SubhookLib.inf
  FunctionMockLib|UnitTestFrameworkPkg/Library/FunctionMockLib/FunctionMockLib.inf
  UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLibCmocka.inf
  DebugLib|UnitTestFrameworkPkg/Library/Posix/DebugLibPosix/DebugLibPosix.inf
  MemoryAllocationLib|UnitTestFrameworkPkg/Library/Posix/MemoryAllocationLibPosix/MemoryAllocationLibPosix.inf
  HostMemoryAllocationBelowAddressLib|UnitTestFrameworkPkg/Library/Posix/MemoryAllocationLibPosix/MemoryAllocationLibPosix.inf
  UefiBootServicesTableLib|UnitTestFrameworkPkg/Library/UnitTestUefiBootServicesTableLib/UnitTestUefiBootServicesTableLib.inf
  PeiServicesTablePointerLib|UnitTestFrameworkPkg/Library/UnitTestPeiServicesTablePointerLib/UnitTestPeiServicesTablePointerLib.inf
  NULL|UnitTestFrameworkPkg/Library/UnitTestDebugAssertLib/UnitTestDebugAssertLibHost.inf
  NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNullHostApplication.inf

[BuildOptions]
  MSFT:*_*_*_CC_FLAGS     = /MTd /EHs
  GCC:*_*_*_CC_FLAGS      = -fno-pie
  CLANGPDB:*_*_*_CC_FLAGS = -fno-pie
  GCC:*_*_*_ASLCC_FLAGS   = -fpie
!if $(UNIT_TESTING_ADDRESS_SANITIZER_ENABLE)
  #
  # Enable Address Sanitizer for VS2019, VS2022, and GCC
  #
  MSFT:*_VS2019_*_CC_FLAGS = /fsanitize=address
  MSFT:*_VS2022_*_CC_FLAGS = /fsanitize=address
  GCC:*_*_*_CC_FLAGS       = -fsanitize=address
  CLANGPDB:*_*_*_CC_FLAGS  = -fsanitize=address
!endif
!ifdef $(UNIT_TESTING_DEBUG)
  MSFT:*_*_*_CC_FLAGS     = -D UNIT_TESTING_DEBUG=1
  GCC:*_*_*_CC_FLAGS      = -D UNIT_TESTING_DEBUG=1
  CLANGPDB:*_*_*_CC_FLAGS = -D UNIT_TESTING_DEBUG=1
  XCODE:*_*_*_CC_FLAGS    = -D UNIT_TESTING_DEBUG=1
!endif
  GCC:*_*_*_CC_FLAGS      = -fexceptions
  CLANGPDB:*_*_*_CC_FLAGS = -fexceptions

!if $(UNIT_TESTING_CODE_COVERAGE_ENABLE)
  #
  # Enable code coverage compiler options
  #
  GCC:*_GCC_*_CC_FLAGS        = --coverage
  GCC:*_GCC5_*_CC_FLAGS       = --coverage
  GCC:*_GCCNOLTO_*_CC_FLAGS   = --coverage
  GCC:*_CLANGDWARF_*_CC_FLAGS = -fprofile-instr-generate -fcoverage-mapping
  CLANGPDB:*_*_*_CC_FLAGS     = -fprofile-instr-generate -fcoverage-mapping
!endif

  #
  # Set C++ language standard version
  #
  MSFT:*_*_*_CXX_FLAGS         = /std:c++20 /Zc:strictStrings- /wd4244
  GCC:*_*_*_CXX_FLAGS          = -std=c++20
  GCC:*_CLANGDWARF_*_CXX_FLAGS = -std=c++17
  CLANGPDB:*_*_*_CXX_FLAGS     = -std=c++17
  #
  # CLANGDWARF and CLANGPDB: Some gtest c++ sources generate undefined
  # behavior that is never triggered. Map trap functions to abort() to
  # complete link and catch real undefined behavior issues.
  #
  GCC:*_CLANGDWARF_*_CXX_FLAGS = -ftrap-function=abort
  CLANGPDB:*_*_*_CXX_FLAGS     = -ftrap-function=abort
  #
  # CLANGPDB: Disable warning for microsoft-cast in gtest c++ sources
  #
  CLANGPDB:*_*_*_CXX_FLAGS = -Wno-microsoft-cast -Wno-write-strings

[BuildOptions.common.EDKII.HOST_APPLICATION]
  #
  # Set MSFT HOST_APPLICATION options
  #
  MSFT:*_*_*_DLINK_FLAGS            == /nologo /SUBSYSTEM:CONSOLE /DEBUG /out:"$(BIN_DIR)\$(MODULE_NAME_GUID).exe" /pdb:"$(BIN_DIR)\$(MODULE_NAME_GUID).pdb" /WHOLEARCHIVE /STACK:0x00800000,0x00800000

  MSFT:*_VS2015_IA32_DLINK_FLAGS    = /LIBPATH:"%VS2015_PREFIX%Lib" /LIBPATH:"%VS2015_PREFIX%VC\Lib" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"
  MSFT:*_VS2015x86_IA32_DLINK_FLAGS = /LIBPATH:"%VS2015_PREFIX%Lib" /LIBPATH:"%VS2015_PREFIX%VC\Lib" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"
  MSFT:*_VS2017_IA32_DLINK_FLAGS    = /LIBPATH:"%VCToolsInstallDir%lib\x86" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"
  MSFT:*_VS2019_IA32_DLINK_FLAGS    = /LIBPATH:"%VCToolsInstallDir%lib\x86" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"
  MSFT:*_VS2022_IA32_DLINK_FLAGS    = /LIBPATH:"%VCToolsInstallDir%lib\x86" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x86"

  MSFT:*_VS2015_X64_DLINK_FLAGS     = /LIBPATH:"%VS2015_PREFIX%VC\Lib\AMD64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
  MSFT:*_VS2015x86_X64_DLINK_FLAGS  = /LIBPATH:"%VS2015_PREFIX%VC\Lib\AMD64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
  MSFT:*_VS2017_X64_DLINK_FLAGS     = /LIBPATH:"%VCToolsInstallDir%lib\x64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
  MSFT:*_VS2019_X64_DLINK_FLAGS     = /LIBPATH:"%VCToolsInstallDir%lib\x64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
  MSFT:*_VS2022_X64_DLINK_FLAGS     = /LIBPATH:"%VCToolsInstallDir%lib\x64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"

  #
  # Set GCC HOST_APPLICATION options
  #
  GCC:*_*_*_CC_FLAGS        =  -Wno-write-strings
  GCC:*_*_IA32_DLINK_FLAGS  == -o $(BIN_DIR)/$(MODULE_NAME_GUID) -z noexecstack -m32 -no-pie
  GCC:*_*_X64_DLINK_FLAGS   == -o $(BIN_DIR)/$(MODULE_NAME_GUID) -z noexecstack -m64 -no-pie
!if $(UNIT_TESTING_ADDRESS_SANITIZER_ENABLE)
  #
  # Enable Address Sanitizer linker options
  #
  GCC:*_*_*_DLINK_FLAGS    = -fsanitize=address
!endif
!if $(UNIT_TESTING_CODE_COVERAGE_ENABLE)
  #
  # Enable code coverage linker options
  #
  GCC:*_GCC_*_DLINK_FLAGS        = --coverage
  GCC:*_GCC5_*_DLINK_FLAGS       = --coverage
  GCC:*_GCCNOLTO_*_DLINK_FLAGS   = --coverage
  GCC:*_CLANGDWARF_*_DLINK_FLAGS = -fprofile-instr-generate -fcoverage-mapping
!endif
  #
  # Surround our static libraries with whole-archive, so constructor-based test registration works properly.
  # Note that we need to --no-whole-archive before linking system libraries.
  #
  GCC:*_*_*_DLINK_FLAGS    = -Wl,--whole-archive
  GCC:*_*_*_DLINK2_FLAGS   == -Wl,--no-whole-archive -lpthread -lstdc++ -lm

  #
  # Set CLANGPDB HOST_APPLICATION options
  #
  CLANGPDB:*_*_*_DLINK_FLAGS   == /nologo /SUBSYSTEM:CONSOLE /DEBUG /out:"$(BIN_DIR)\$(MODULE_NAME_GUID).exe" /pdb:"$(BIN_DIR)\$(MODULE_NAME_GUID).pdb" /WHOLEARCHIVE /STACK:0x00800000,0x00800000
  CLANGPDB:*_*_X64_DLINK_FLAGS  = /LIBPATH:"%VCToolsInstallDir%lib\x64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%\um\x64"
!if $(UNIT_TESTING_ADDRESS_SANITIZER_ENABLE)
  CLANGPDB:*_*_X64_DLINK_FLAGS  = clang_rt.profile-x86_64.lib clang_rt.asan_dynamic-x86_64.lib clang_rt.asan_dynamic_runtime_thunk-x86_64.lib
!endif

  #
  # Need to do this link via gcc and not ld as the pathing to libraries changes from OS version to OS version
  #
  XCODE:*_*_IA32_DLINK_PATH == gcc
  XCODE:*_*_IA32_CC_FLAGS = -I$(WORKSPACE)/EmulatorPkg/Unix/Host/X11IncludeHack
  XCODE:*_*_IA32_DLINK_FLAGS == -arch i386 -o $(BIN_DIR)/$(MODULE_NAME_GUID) -L/usr/X11R6/lib -lXext -lX11 -framework Carbon
  XCODE:*_*_IA32_ASM_FLAGS == -arch i386 -g

  XCODE:*_*_X64_DLINK_PATH == gcc
  XCODE:*_*_X64_DLINK_FLAGS == -o $(BIN_DIR)/$(MODULE_NAME_GUID) -L/usr/X11R6/lib -lXext -lX11 -framework Carbon -Wl,-no_pie
  XCODE:*_*_X64_ASM_FLAGS == -g
  XCODE:*_*_X64_CC_FLAGS = -O0 -target x86_64-apple-darwin -I$(WORKSPACE)/EmulatorPkg/Unix/Host/X11IncludeHack "-DEFIAPI=__attribute__((ms_abi))"
