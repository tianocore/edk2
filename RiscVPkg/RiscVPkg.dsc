## @file
# RISC-V package.
#
# Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

################################################################################
#
# Defines Section
#
################################################################################
[Defines]
  PLATFORM_NAME                  = RiscV
  PLATFORM_GUID                  = 55D77916-B270-41B4-9325-2CE9DCE0926E
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
  CpuExceptionHandlerLib|RiscVPkg/Library/RiscVExceptionLib/CpuExceptionHandlerDxeLib.inf
  RiscVCpuLib|RiscVPkg/Library/RiscVCpuLib/RiscVCpuLib.inf
  RiscVEdk2SbiLib|RiscVPkg/Library/RiscVEdk2SbiLib/RiscVEdk2SbiLib.inf
  RiscVOpensbiLib|RiscVPkg/Library/RiscVOpensbiLib/RiscVOpensbiLib.inf
  TimerLib|RiscVPkg/Library/RiscVTimerLib/BaseRiscVTimerLib.inf
  MachineModeTimerLib|RiscVPkg/Library/RiscVReadMachineModeTimer/MachineModeTimerLib/MachineModeTimerLib.inf
  #MachineModeTimerLib|RiscVPkg/Library/RiscVReadMachineModeTimer/EmulatedMachineModeTimerLib/EmulatedMachineModeTimerLib.inf
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
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
  RiscVPlatformTimerLib|RiscVPkg/Library/RiscVPlatformTimerLibNull/RiscVPlatformTimerLib.inf
  FdtLib|EmbeddedPkg/Library/FdtLib/FdtLib.inf

[LibraryClasses.common.PEI_CORE]
  PeiServicesTablePointerLib|RiscVPkg/Library/PeiServicesTablePointerLibOpenSbi/PeiServicesTablePointerLibOpenSbi.inf
  RiscVFirmwareContextLib|RiscVPkg/Library/RiscVFirmwareContextSbiLib/RiscVFirmwareContextSbiLib.inf

[LibraryClasses.common.PEIM]
  PeiServicesTablePointerLib|RiscVPkg/Library/PeiServicesTablePointerLibOpenSbi/PeiServicesTablePointerLibOpenSbi.inf
  RiscVFirmwareContextLib|RiscVPkg/Library/RiscVFirmwareContextSbiLib/RiscVFirmwareContextSbiLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf

[LibraryClasses.common.DXE_CORE]
  TimerLib|RiscVPkg/Library/RiscVTimerLib/BaseRiscVTimerLib.inf

[LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  PlatformBootManagerLib|RiscVPlatformPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  TimerLib|RiscVPkg/Library/RiscVTimerLib/BaseRiscVTimerLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  TimerLib|RiscVPkg/Library/RiscVTimerLib/BaseRiscVTimerLib.inf

[Components]
  RiscVPkg/Library/RiscVTimerLib/BaseRiscVTimerLib.inf
  RiscVPkg/Library/RiscVExceptionLib/CpuExceptionHandlerDxeLib.inf
  RiscVPkg/Library/RiscVFirmwareContextSbiLib/RiscVFirmwareContextSbiLib.inf
  RiscVPkg/Library/PeiServicesTablePointerLibOpenSbi/PeiServicesTablePointerLibOpenSbi.inf
  RiscVPkg/Library/RiscVOpensbiLib/RiscVOpensbiLib.inf
  RiscVPkg/Library/RiscVPlatformTimerLibNull/RiscVPlatformTimerLib.inf
  RiscVPkg/Library/RiscVCpuLib/RiscVCpuLib.inf
  RiscVPkg/Library/RiscVEdk2SbiLib/RiscVEdk2SbiLib.inf

  RiscVPkg/Universal/CpuDxe/CpuDxe.inf
  RiscVPkg/Universal/SmbiosDxe/RiscVSmbiosDxe.inf
  RiscVPkg/Universal/FdtDxe/FdtDxe.inf
  RiscVPkg/Universal/PciCpuIo2Dxe/PciCpuIo2Dxe.inf
