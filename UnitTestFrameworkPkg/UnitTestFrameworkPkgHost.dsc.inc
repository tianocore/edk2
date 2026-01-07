## @file
# UnitTestFrameworkPkg DSC include file for host based test DSC
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgCommon.dsc.inc

[Defines]
  HOST_BASED_UNIT_TESTING_ENABLED = TRUE

  #
  # Host based unit test builds only support NOOPT build target
  #
  BUILD_TARGETS = NOOPT

  #
  # Always enable code coverage and address sanitizers
  #
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
  StackCheckLib|MdePkg/Library/StackCheckLibNull/StackCheckLibNullHostApplication.inf

#
# Fail with error message if the OS/Compiler combination is not supported
#
# Operating System and Compiler Compatibility Matrix for Host-based unit tests
# NOTE: Only NOOPT build target is supported for unit test builds
#
# +--------------------+--------+----------+------------+-----+----+--------+
# | OS/Compiler        | VS2019 | CLANGPDB | CLANGDWARF |   GCC    | XCODE5 |
# |                    | VS2022 |          |            |   GCC5   |        |
# |                    | VS2026 |          |            | GCCNOLTO |        |
# +--------------------+--------+----------+------------+----------+--------+
# | Windows/VS         |IA32/X64|          |            |          |        |
# | Windows/LLVM/VS    |        | IA32/X64 |            |          |        |
# | Windows/LLVM/MSYS2 |        |          |    X64     |          |        |
# | Windows/LLVM/MINGW |        |          |  IA32/X64  |          |        |
# | Linux/LLVM         |        |          |  IA32/X64  |          |        |
# | Linux/GCC          |        |          |            | IA32/X64 |        |
# | macOS/XCODE5       |        |          |            |          |IA32/X64|
# +--------------------+--------+----------+------------+----------+--------+
#
# * Windows/VS: Windows environment with Visual Studio installed
# * Windows/LLVM/VS: Windows environment with Visual Studio and LLVM 20.1.8 or
#   higher installed. The default version of LLVM installed with Visual Studio
#   is not supported.
#   * https://github.com/llvm/llvm-project/releases
#   * Host-based unit tests depend on Visual Studio includes/libraries
# * Windows/LLVM/MSYS2: Windows environment with MSYS2 based LLVM 20.1.8 or
#   higher installed.
#   * https://github.com/tianocore/edk2-edkrepo/releases?q=clang
#   * Host-based unit tests do not use any Visual Studio includes/libraries
#   * MSYS2 based LLVM 20.1.8 does not provide IA32 includes/libraries
# * Windows/LLVM/MINGW: Windows environment with LLVM MINGW 20.1.8 or higher
#   installed with UCRT includes/libraries.
#   * https://github.com/mstorsjo/llvm-mingw/releases
#   * Host-based unit tests do not use any Visual Studio includes/libraries
#   * UCRT release provide IA32 and X64 includes/libraries
# * Linux/LLVM or Linux/GCC: Linux environment with GCC 13 or higher and
#   LLVM 20.1.8 or higher installed. Not all Linux distributions provide IA32
#   includes/libraries for Host-based unit tests.
#   * Host-based unit tests use GCC and LLVM includes/libraries
#   * Ubuntu 24.04 apt modules for IA32 and X64 includes/libraries:
#       build-essential uuid-dev lcov
#       g++-13 gcc-13
#       g++-13-x86-64-linux-gnux32 gcc-13-x86-64-linux-gnux32
#       llvm-20 clang-20 lld-20 libclang-rt-20-dev
#       gcc-multilib g++-multilib libx11-dev libx11-6 libxext6 libxext-dev
#       libc6-i386 libc6-dev-i386 libxext6:i386 libxext-dev:i386 linux-libc-dev:i386
# macOS/XCODE: macOS environment with XCODE5 installed.
#
!if $(WIN_MINGW32_BUILD)
  !if $(TOOL_CHAIN_TAG) in "VS2019 VS2022 VS2026"
    !error Host-based Unit Tests not supported for Mingw/VS20xx builds
  !endif
  !if $(TOOL_CHAIN_TAG) in "CLANGPDB"
    !error Host-based Unit Tests not supported for Mingw/CLANGPDB builds
  !endif
  !if $(TOOL_CHAIN_TAG) in "GCC GCC5 GCCNOLTO"
    !error Host-based Unit Tests not supported for Mingw/GCC builds
  !endif
!else
  !if $(WIN_HOST_BUILD)
    !if $(TOOL_CHAIN_TAG) in "GCC GCC5 GCCNOLTO"
      !error Host-based Unit Tests not supported for Windows/GCC builds
    !endif
    !if $(TOOL_CHAIN_TAG) in "CLANGDWARF"
      !error Host-based Unit Tests not supported for Windows/CLANGDWARF builds
    !endif
  !else
    !if $(TOOL_CHAIN_TAG) in "VS2019 VS2022 VS2026"
      !error Host-based Unit Tests not supported for Linux/VS20xx builds
    !endif
    !if $(TOOL_CHAIN_TAG) in "CLANGPDB"
      !error Host-based Unit Tests not supported for Linux/CLANGPDB builds
    !endif
  !endif
!endif

[BuildOptions]
!if $(WIN_MINGW32_BUILD)
  GCC:*_CLANGDWARF_IA32_PP_FLAGS = -target i686-w64-mingw32
  GCC:*_CLANGDWARF_X64_PP_FLAGS  = -target x86_64-w64-mingw32

  GCC:*_CLANGDWARF_X64_CC_FLAGS  = -target x86_64-w64-mingw32
  GCC:*_CLANGDWARF_IA32_CC_FLAGS = -target i686-w64-mingw32
  GCC:*_CLANGDWARF_*_CC_FLAGS    = -O0 -fno-omit-frame-pointer -fexceptions -g3 -gcodeview -fdebug-macro -fno-lto

  GCC:*_CLANGDWARF_X64_NASM_FLAGS  = -f win64
  GCC:*_CLANGDWARF_IA32_NASM_FLAGS = -f win32
!endif

  MSFT:*_*_*_CC_FLAGS     = /MTd /EHs
  GCC:*_*_*_CC_FLAGS      = -fpie
  CLANGPDB:*_*_*_CC_FLAGS = -fno-pie
  GCC:*_*_*_ASLCC_FLAGS   = -fpie

  #
  # Disable stack protector for unit tests because ASAN provides better coverage
  #
  GCC:*_*_*_CC_FLAGS             = -fno-stack-protector
  CLANGPDB:*_*_*_CC_FLAGS        = -fno-stack-protector

  #
  # Force long double to be 64-bits for CLANGDWARF IA32 builds to address
  # alignment errors from c++ math libraries
  #
  GCC:*_CLANGDWARF_IA32_CC_FLAGS = -mlong-double-64

!if $(UNIT_TESTING_ADDRESS_SANITIZER_ENABLE)
  #
  # Enable Address Sanitizer for VS2019, VS2022, VS2026, and GCC
  #
  MSFT:*_VS2019_*_CC_FLAGS = /fsanitize=address
  MSFT:*_VS2022_*_CC_FLAGS = /fsanitize=address
  MSFT:*_VS2026_*_CC_FLAGS = /fsanitize=address
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
!if $(TOOL_CHAIN_TAG) in "CLANGDWARF"
  GCC:*_CLANGDWARF_*_CC_FLAGS = -fprofile-instr-generate -fcoverage-mapping
!else
  GCC:*_*_*_CC_FLAGS          = --coverage
!endif
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

[Defines]
  #
  # Defines for settings that are common between MSFT and CLANGPDB tool chain
  # families that must use Visual Studio specific defines and libraries when
  # building modules of type HOST_APPLICATION
  #
  DEFINE VISUAL_STUDIO_DEFINES        = -D UNICODE -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE
  DEFINE VISUAL_STUDIO_IA32_LIB_PATHS = /LIBPATH:"%VCToolsInstallDir%lib\x86" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\x86" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%um\x86"
  DEFINE VISUAL_STUDIO_X64_LIB_PATHS  = /LIBPATH:"%VCToolsInstallDir%lib\X64" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\X64" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%um\X64"

[BuildOptions.common.EDKII.HOST_APPLICATION]
  #
  # Set MSFT HOST_APPLICATION options
  #
  MSFT:*_*_*_CC_FLAGS     = $(VISUAL_STUDIO_DEFINES)
  #
  # Must override DLINK_FLAGS to remove /DLL when linking .exe
  #
  MSFT:*_*_*_DLINK_FLAGS   == /out:"$(BIN_DIR)\$(MODULE_NAME_GUID).exe" /NOLOGO /SUBSYSTEM:CONSOLE /WHOLEARCHIVE /STACK:0x00800000,0x00800000 /SAFESEH:NO
  MSFT:*_*_IA32_DLINK_FLAGS = $(VISUAL_STUDIO_IA32_LIB_PATHS)
  MSFT:*_*_X64_DLINK_FLAGS  = $(VISUAL_STUDIO_X64_LIB_PATHS)
  MSFT:*_*_*_DLINK_FLAGS    = /DEBUG /pdb:"$(BIN_DIR)\$(MODULE_NAME_GUID).pdb"

  #
  # Set CLANGPDB HOST_APPLICATION options
  #
  CLANGPDB:*_*_*_CC_FLAGS     = $(VISUAL_STUDIO_DEFINES)
  #
  # Must override DLINK_FLAGS to remove /DLL when linking .exe
  #
  CLANGPDB:*_*_*_DLINK_FLAGS   == /out:"$(BIN_DIR)\$(MODULE_NAME_GUID).exe" /NOLOGO /SUBSYSTEM:CONSOLE /WHOLEARCHIVE /STACK:0x00800000,0x00800000 /IGNORE:4099 /SAFESEH:NO
  CLANGPDB:*_*_IA32_DLINK_FLAGS = $(VISUAL_STUDIO_IA32_LIB_PATHS)
  CLANGPDB:*_*_X64_DLINK_FLAGS  = $(VISUAL_STUDIO_X64_LIB_PATHS)
  CLANGPDB:*_*_*_DLINK_FLAGS    = /DEBUG /pdb:"$(BIN_DIR)\$(MODULE_NAME_GUID).pdb"
!if $(UNIT_TESTING_ADDRESS_SANITIZER_ENABLE)
  #
  # Remove default library clang_rt.profile.lib that is not includes with Visual Studio
  #
  CLANGPDB:*_*_IA32_DLINK_FLAGS = /NODEFAULTLIB:clang_rt.profile.lib
  #
  # Add Address Sanitizer runtime libraries for IA32/X64 architectures
  #
  CLANGPDB:*_*_IA32_DLINK_FLAGS = clang_rt.profile-i386.lib clang_rt.asan_dynamic-i386.lib clang_rt.asan_dynamic_runtime_thunk-i386.lib
  CLANGPDB:*_*_X64_DLINK_FLAGS  = clang_rt.profile-x86_64.lib clang_rt.asan_dynamic-x86_64.lib clang_rt.asan_dynamic_runtime_thunk-x86_64.lib
!endif

  #
  # Set GCC HOST_APPLICATION options
  #
  GCC:*_*_*_CC_FLAGS           =  -Wno-write-strings -fpie
  GCC:*_*_IA32_DLINK_FLAGS     == -o $(BIN_DIR)/$(MODULE_NAME_GUID) -m32
  GCC:*_*_X64_DLINK_FLAGS      == -o $(BIN_DIR)/$(MODULE_NAME_GUID) -m64
!if $(WIN_MINGW32_BUILD)
  GCC:*_CLANGDWARF_*_DLINK_FLAGS = -Wl,--pdb,$(BIN_DIR)\$(MODULE_NAME_GUID).pdb -g3
!else
  GCC:*_*_*_DLINK_FLAGS = -z noexecstack
!endif

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
!if $(TOOL_CHAIN_TAG) in "CLANGDWARF"
  GCC:*_CLANGDWARF_*_DLINK_FLAGS = -fprofile-instr-generate -fcoverage-mapping
!else
  GCC:*_*_*_DLINK_FLAGS          = --coverage
!endif
!endif
  #
  # Surround static libraries with whole-archive, so constructor-based test registration works properly.
  # Change to --no-whole-archive before linking system libraries.
  #
  GCC:*_*_*_DLINK_FLAGS    = -Wl,--whole-archive
  GCC:*_*_*_DLINK2_FLAGS   == -Wl,--no-whole-archive -lpthread -lstdc++ -lm

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
