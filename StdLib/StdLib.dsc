## @file
#   Intel(r) UEFI Standard Libraries for EDK II.
#   Build descriptions.
#
#   See the comments in the [LibraryClasses.IA32] and [BuildOptions] sections
#   for important information about configuring this package for your
#   environment.
#
# This package contains:
#       Standard C Library.
#       Sockets Library.
#       Posix Library.
#
#   Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
#   This program and the accompanying materials
#   are licensed and made available under the terms and conditions of the BSD License
#   which accompanies this distribution. The full text of the license may be found at
#   http://opensource.org/licenses/bsd-license.
#
#   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
##

[Defines]
  PLATFORM_NAME                  = StdLib
  PLATFORM_GUID                  = 6135e67b-d813-4e4a-93c3-945d6af41858
  PLATFORM_VERSION               = 0.01
  DSC_SPECIFICATION              = 0x00010006
  OUTPUT_DIRECTORY               = Build/StdLib
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

#
#  Debug output control
#
  DEFINE DEBUG_ENABLE_OUTPUT      = FALSE       # Set to TRUE to enable debug output
  DEFINE DEBUG_PRINT_ERROR_LEVEL  = 0x80000000  # Flags to control amount of debug output
  DEFINE DEBUG_PROPERTY_MASK      = 0x0f

[PcdsFeatureFlag]

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|$(DEBUG_PROPERTY_MASK)
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|$(DEBUG_PRINT_ERROR_LEVEL)

[PcdsFixedAtBuild.IPF]

[LibraryClasses]
  #
  # Entry Point Libraries
  #
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  #
  # Common Libraries
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  !if $(DEBUG_ENABLE_OUTPUT)
    DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
    DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  !else   ## DEBUG_ENABLE_OUTPUT
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  !endif  ## DEBUG_ENABLE_OUTPUT
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components]
# Standard C Libraries.
  StdLib/LibC/LibC.inf
  StdLib/LibC/StdLib/StdLib.inf
  StdLib/LibC/String/String.inf
  StdLib/LibC/Wchar/Wchar.inf
  StdLib/LibC/Ctype/Ctype.inf
  StdLib/LibC/Time/Time.inf
  StdLib/LibC/Stdio/Stdio.inf
  StdLib/LibC/Locale/Locale.inf
  StdLib/LibC/Uefi/Uefi.inf
  StdLib/LibC/Math/Math.inf
  StdLib/LibC/Signal/Signal.inf
  StdLib/LibC/NetUtil/NetUtil.inf

# Device Abstractions within the Standard C Library
# Applications should not directly access any functions defined in these libraries.
  StdLib/LibC/gdtoa/gdtoa.inf
  StdLib/LibC/Uefi/Devices/daUtility.inf
  StdLib/LibC/Uefi/Devices/daConsole.inf
  StdLib/LibC/Uefi/Devices/daShell.inf

# Additional, non-standard, libraries
  StdLib/LibC/Containers/ContainerLib.inf

# Additional libraries for POSIX functionality.
  StdLib/PosixLib/PosixLib.inf
  StdLib/PosixLib/Err/LibErr.inf
  StdLib/PosixLib/Gen/LibGen.inf
  StdLib/PosixLib/Glob/LibGlob.inf
  StdLib/PosixLib/Stringlist/LibStringlist.inf
  StdLib/LibC/Uefi/InteractiveIO/IIO.inf

#    Socket Libraries - LibC based
  StdLib/BsdSocketLib/BsdSocketLib.inf
  StdLib/EfiSocketLib/EfiSocketLib.inf
  StdLib/UseSocketDxe/UseSocketDxe.inf

##############################################################################
#
#  Include Boilerplate text required for building with the Standard Libraries.
#
##############################################################################
!include StdLib/StdLib.inc
