## @file
# Source Level Debug Package.
#
# Copyright (c) 2010 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2020, AMD Incorporated. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = SourceLevelDebugPkg
  PLATFORM_GUID                  = 38C85805-883F-4ee8-A854-95B966ED73AA
  PLATFORM_VERSION               = 0.96
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/SourceLevelDebugPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  TimerLib|UefiCpuPkg/Library/SecPeiDxeTimerLibUefiCpu/SecPeiDxeTimerLibUefiCpu.inf
  ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf

!ifdef $(SOURCE_DEBUG_USE_USB)
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibUsb/DebugCommunicationLibUsb.inf
!else
!ifndef $(SOURCE_DEBUG_USE_USB3)
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
!endif
!endif

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
!ifdef $(SOURCE_DEBUG_USE_USB3)
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibUsb3/DebugCommunicationLibUsb3Pei.inf
!endif

[LibraryClasses.common.DXE_DRIVER]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!ifdef $(SOURCE_DEBUG_USE_USB3)
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibUsb3/DebugCommunicationLibUsb3Dxe.inf
!endif

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

[Components.common]
  SourceLevelDebugPkg/Library/DebugCommunicationLibUsb3/DebugCommunicationLibUsb3Dxe.inf
  SourceLevelDebugPkg/Library/DebugCommunicationLibUsb3/DebugCommunicationLibUsb3Pei.inf
  SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
  SourceLevelDebugPkg/Library/DebugCommunicationLibUsb/DebugCommunicationLibUsb.inf
  SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
  SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
  SourceLevelDebugPkg/Library/DebugAgent/SmmDebugAgentLib.inf
  SourceLevelDebugPkg/DebugAgentPei/DebugAgentPei.inf
  SourceLevelDebugPkg/DebugAgentDxe/DebugAgentDxe.inf

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
