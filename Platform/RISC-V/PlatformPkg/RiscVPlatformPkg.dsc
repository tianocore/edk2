#/** @file
# RISC-V platform package.
#
# Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#**/

################################################################################
#
# Defines Section
#
################################################################################
[Defines]
  PLATFORM_NAME                  = RiscVPlatform
  PLATFORM_GUID                  = 840A9576-5869-491E-9210-89769DED4650
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001c
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

[BuildOptions]
  GCC:RELEASE_*_*_CC_FLAGS       = -DMDEPKG_NDEBUG
!ifdef $(SOURCE_DEBUG_ENABLE)
  GCC:*_*_RISCV64_GENFW_FLAGS    = --keepexceptiontable
!endif

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  FdtLib|EmbeddedPkg/Library/FdtLib/FdtLib.inf
  RiscVOpensbiPlatformLib|Platform/RISC-V/PlatformPkg/Library/OpensbiPlatformLib/OpensbiPlatformLib.inf
  RiscVCpuLib|Silicon/RISC-V/ProcessorPkg/Library/RiscVCpuLib/RiscVCpuLib.inf
  RiscVEdk2SbiLib|Silicon/RISC-V/ProcessorPkg/Library/RiscVEdk2SbiLib/RiscVEdk2SbiLib.inf
  RiscVOpensbiLib|Silicon/RISC-V/ProcessorPkg/Library/RiscVOpensbiLib/RiscVOpensbiLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

[LibraryClasses.common.PEI_CORE]
  # RISC-V platform PEI core entry point.
  PeiCoreEntryPoint|Platform/RISC-V/PlatformPkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PlatformSecPpiLib|Platform/RISC-V/PlatformPkg/Library/PlatformSecPpiLibNull/PlatformSecPpiLibNull.inf

[LibraryClasses.common.PEIM]
  FirmwareContextProcessorSpecificLib|Platform/RISC-V/PlatformPkg/Library/FirmwareContextProcessorSpecificLib/FirmwareContextProcessorSpecificLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf

[LibraryClasses.common.SEC]
  ExtractGuidedSectionLib|MdePkg/Library/BaseExtractGuidedSectionLib/BaseExtractGuidedSectionLib.inf
  RiscVSpecialPlatformLib|Platform/RISC-V/PlatformPkg/Library/RiscVSpecialPlatformLibNull/RiscVSpecialPlatformLibNull.inf

[LibraryClasses.common.DXE_DRIVER]
  PlatformBootManagerLib|Platform/RISC-V/PlatformPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf

[Components.common]
  Platform/RISC-V/PlatformPkg/Library/OpensbiPlatformLib/OpensbiPlatformLib.inf
  Platform/RISC-V/PlatformPkg/Library/PlatformMemoryTestLibNull/PlatformMemoryTestLibNull.inf
  Platform/RISC-V/PlatformPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  Platform/RISC-V/PlatformPkg/Library/PlatformUpdateProgressLibNull/PlatformUpdateProgressLibNull.inf
  Platform/RISC-V/PlatformPkg/Library/FirmwareContextProcessorSpecificLib/FirmwareContextProcessorSpecificLib.inf
  Platform/RISC-V/PlatformPkg/Library/RiscVPlatformTempMemoryInitLibNull/RiscVPlatformTempMemoryInitLibNull.inf
  Platform/RISC-V/PlatformPkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  Platform/RISC-V/PlatformPkg/Library/PlatformSecPpiLibNull/PlatformSecPpiLibNull.inf
  Platform/RISC-V/PlatformPkg/Library/RiscVSpecialPlatformLibNull/RiscVSpecialPlatformLibNull.inf

[Components.common.SEC]
  Platform/RISC-V/PlatformPkg/Universal/Sec/SecMain.inf

