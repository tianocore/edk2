# @file dsc_translator_test.py
# Tests for the translator for the EDK II DSC data model object
#
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import unittest
import os
import tempfile
from edk2toollib.uefi.edk2.parsers.dsc_parser import DscParser
# from edk2toollib.uefi.edk2.build_objects.dsc import dsc
from edk2toollib.uefi.edk2.build_objects.dsc_translator import DscTranslator


class TestDscTranslator(unittest.TestCase):
    test_dsc = """
## @file
# EFI/Framework Emulation Platform with UEFI HII interface supported.
#
# The Emulation Platform can be used to debug individual modules, prior
# to creating a real platform. This also provides an example for how
# an DSC is created.
#
# Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made
# available under the terms and conditions of the BSD License which
# accompanies this distribution.
# The full text of the license may be found at:
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS"
# BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER
# EXPRESS OR IMPLIED.
#
##
########################################################################
#
# Defines Section - statements that will be processed to create a
# Makefile.
#
########################################################################
[Defines]
  PLATFORM_NAME           = NT32
  PLATFORM_GUID           = EB216561-961F-47EE-9EF9-CA426EF547C2
  PLATFORM_VERSION        = 0.5
  DSC_SPECIFICATION       = 0x0001001C
  OUTPUT_DIRECTORY        = Build/NT32
  SUPPORTED_ARCHITECTURES = IA32
  BUILD_TARGETS           = DEBUG|RELEASE
  SKUID_IDENTIFIER        = DEFAULT
  FLASH_DEFINITION        = Nt32Pkg/Nt32Pkg.fdf
  #
  # Defines for default states. These can be changed on the command
  # line.
  # -D FLAG=VALUE
  #
!ifndef SECURE_BOOT_ENABLE
  DEFINE SECURE_BOOT_ENABLE = FALSE
!endif
  DEFINE SECRET_VAR = FALSE

########################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
# Platform.
#
########################################################################
[SkuIds]
  0|DEFAULT # The entry: 0|DEFAULT is reserved and always required.
  3|JOEY|DEFAULT
  5|JOEY2|JOEY

########################################################################
#
# Parser Class section - list of all Parser Classes needed by this
# Platform.
#
########################################################################
[LibraryClasses]
  DEFINE SECRET_VAR = TRUE
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Parser/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Parser/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Parser/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Parser/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Parser/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  #
  # Basic
  #
  BaseLib|MdePkg/Parser/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Parser/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PrintLib|MdePkg/Parser/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Parser/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Parser/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Parser/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Parser/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Parser/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Parser/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Parser/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Parser/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Parser/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Parser/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Parser/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Parser/UefiLib/UefiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Parser/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Parser/UefiHiiLib/UefiHiiLib.inf
  DevicePathLib|MdePkg/Parser/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Parser/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  PeiServicesTablePointerLib|MdePkg/Parser/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Parser/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Parser/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Parser/DxeServicesTableLib/DxeServicesTableLib.inf
  #
  # Generic Modules
  #
  UefiUsbLib|MdePkg/Parser/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Parser/UefiScsiLib/UefiScsiLib.inf
  NetLib|MdeModulePkg/Parser/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Parser/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Parser/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|MdeModulePkg/Parser/DxeDpcLib/DxeDpcLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Parser/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  GenericBdsLib|IntelFrameworkModulePkg/Parser/GenericBdsLib/GenericBdsLib.inf
  SecurityManagementLib|MdeModulePkg/Parser/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  TimerLib|MdePkg/Parser/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  SerialPortLib|MdePkg/Parser/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  CapsuleLib|MdeModulePkg/Parser/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  #
  # Platform
  #
  PlatformBdsLib|Nt32Pkg/Parser/Nt32BdsLib/Nt32BdsLib.inf
  #
  # Misc
  #
  DebugLib|IntelFrameworkModulePkg/Parser/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  DebugPrintErrorLevelLib|MdeModulePkg/Parser/DxeDebugPrintErrorLevelLib/DxeDebugPrintErrorLevelLib.inf
  PerformanceLib|MdePkg/Parser/BasePerformanceLibNull/BasePerformanceLibNull.inf
  DebugAgentLib|MdeModulePkg/Parser/DebugAgentLibNull/DebugAgentLibNull.inf
  CpuExceptionHandlerLib|MdeModulePkg/Parser/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  PlatformSecureLib|Nt32Pkg/Parser/PlatformSecureLib/PlatformSecureLib.inf
  IntrinsicLib|CryptoPkg/Parser/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Parser/OpensslLib/OpensslLib.inf
!endif

[LibraryClasses.common.USER_DEFINED]
  DebugLib|MdePkg/Parser/BaseDebugLibNull/BaseDebugLibNull.inf
  PeCoffExtraActionLib|MdePkg/Parser/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  ReportStatusCodeLib|MdeModulePkg/Parser/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  OemHookStatusCodeLib|Nt32Pkg/Parser/PeiNt32OemHookStatusCodeLib/PeiNt32OemHookStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Parser/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.common.PEIM,LibraryClasses.common.PEI_CORE]
  #
  # PEI phase common
  #
  HobLib|MdePkg/Parser/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Parser/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Parser/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Parser/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  BaseMemoryLib|MdePkg/Parser/BaseMemoryLibOptPei/BaseMemoryLibOptPei.inf
  IoLib|MdePkg/Parser/PeiIoLibCpuIo/PeiIoLibCpuIo.inf
  PeCoffGetEntryPointLib|Nt32Pkg/Parser/Nt32PeiPeCoffGetEntryPointLib/Nt32PeiPeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|Nt32Pkg/Parser/PeiNt32PeCoffExtraActionLib/PeiNt32PeCoffExtraActionLib.inf
  DebugPrintErrorLevelLib|MdePkg/Parser/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf

[LibraryClasses.common.PEI_CORE]
  PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf
  OemHookStatusCodeLib|MdeModulePkg/Parser/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf

[LibraryClasses.common.PEIM]
  PcdLib|MdePkg/Parser/PeiPcdLib/PeiPcdLib.inf
  OemHookStatusCodeLib|Nt32Pkg/Parser/PeiNt32OemHookStatusCodeLib/PeiNt32OemHookStatusCodeLib.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Parser/BaseCryptLib/PeiCryptLib.inf
!endif

[LibraryClasses.common]
  #
  # DXE phase common
  #
  BaseMemoryLib|MdePkg/Parser/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  HobLib|MdePkg/Parser/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Parser/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Parser/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Parser/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  OemHookStatusCodeLib|Nt32Pkg/Parser/DxeNt32OemHookStatusCodeLib/DxeNt32OemHookStatusCodeLib.inf
  PeCoffExtraActionLib|Nt32Pkg/Parser/DxeNt32PeCoffExtraActionLib/DxeNt32PeCoffExtraActionLib.inf
  ExtractGuidedSectionLib|MdePkg/Parser/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  WinNtLib|Nt32Pkg/Parser/DxeWinNtLib/DxeWinNtLib.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Parser/BaseCryptLib/BaseCryptLib.inf
!endif

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Parser/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Parser/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  DebugLib|MdePkg/Parser/BaseDebugLibNull/BaseDebugLibNull.inf

[LibraryClasses.common.UEFI_DRIVER]
  PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.common.UEFI_APPLICATION]
  PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdeModulePkg/Parser/DxePrintLibPrint2Protocol/DxePrintLibPrint2Protocol.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  #
  # Runtime
  #
!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Parser/BaseCryptLib/RuntimeCryptLib.inf
!endif

########################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
########################################################################
[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreImageLoaderSearchTeSectionFirst|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableCollectStatistics|TRUE

[PcdsFixedAtBuild]
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizePopulateCapsule|0x0
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000040
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtFirmwareFdSize|0x2a0000
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1f
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtFirmwareVolume|L"..FvNt32.fd"
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtFirmwareBlockSize|0x10000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0f
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange| FALSE
!if $(SECURE_BOOT_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
!endif
!if $(SECURE_BOOT_ENABLE) == TRUE
  # override the default values from SecurityPkg to ensure images from
  # all sources are verified in secure boot
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x05
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x05
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x05
!endif

[PcdsPatchableInModule.IA32, PcdsPatchableInModule.X64]
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtGop|L"UGA Window 1!UGA Window 2"|VOID*|52

########################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
########################################################################
[PcdsDynamicDefault.common.DEFAULT]
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtSerialPort|L"COM1!COM2"|VOID*|20
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtFileSystem|L".!.....EdkShellBinPkgBinIa32Apps"|VOID*|106
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtGop|L"UGA Window 1!UGA Window 2"|VOID*|52
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtConsole|L"Bus Driver Console Window"|VOID*|52
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtVirtualDisk|L"FW;40960;512"|VOID*|26
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtMemorySize|L"64!64"|VOID*|12
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtPhysicalDisk|L"a:RW;2880;512!d:RO;307200;2048!j:RW;262144;512"|VOID*|100
  gEfiNt32PkgTokenSpaceGuid.PcdWinNtUga|L"UGA Window 1!UGA Window 2"|VOID*|52
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0

[PcdsDynamicHii.common.DEFAULT]
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdSetupConOutColumn|L"SetupConsoleConfig"|gEfiGlobalVariableGuid|0x0|80
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdSetupConOutRow|L"SetupConsoleConfig"|gEfiGlobalVariableGuid|0x4|25
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|10
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdHardwareErrorRecordLevel|L"HwErrRecSupport"|gEfiGlobalVariableGuid|0x0|1
  gEfiMdeModulePkgTokenSpaceGuid.PcdValidRange|L"PcdValidRange"|gEfiGlobalVariableGuid|0x07|0|BS,RT,NV

########################################################################
#
# Components Section - list of the modules and components that will be
# processed by compilation tools and the EDK II
# tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary
# images get placed into firmware volume images. This section is
# just a list of modules to compile from source into
# UEFI-compliant binaries.
# It is the FDF file that contains information on combining binary
# files into firmware volume images, whose concept is beyond UEFI
# and is described in PI specification.
# Binary modules do not need to be listed in this section, as they
# should be specified in the FDF file. For example: Shell binary
# (Shell_Full.efi), FAT binary (Fat.efi), Logo (Logo.bmp), and etc.
# There may also be modules listed in this section that are not
# required in the FDF file,
# When a module listed here is excluded from FDF file, then
# UEFI-compliant binary will be generated for it, but the binary
# will not be put into any firmware volume.
#
########################################################################
[Components.IA32]
  ##
  # SEC Phase modules
  ##
  Nt32Pkg/Sec/SecMain.inf
  ##
  # PEI Phase modules
  ##
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf
  Nt32Pkg/WinNtOemHookStatusCodeHandlerPei/WinNtOemHookStatusCodeHandlerPei.inf
  Nt32Pkg/BootModePei/BootModePei.inf
  Nt32Pkg/StallPei/StallPei.inf
  Nt32Pkg/WinNtFlashMapPei/WinNtFlashMapPei.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/Pei/VariablePei.inf
!else
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
!endif

  Nt32Pkg/WinNtAutoScanPei/WinNtAutoScanPei.inf
  Nt32Pkg/WinNtFirmwareVolumePei/WinNtFirmwareVolumePei.inf
  Nt32Pkg/WinNtThunkPPIToProtocolPei/WinNtThunkPPIToProtocolPei.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf

[Components.X64]
  ##
  # DXE Phase modules
  ##
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL| MdeModulePkg/Parser/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
    <BuildOptions>
      *_*_IA32_CC_FLAGS =
  }

  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf
  }
  Nt32Pkg/MetronomeDxe/MetronomeDxe.inf
  Nt32Pkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  Nt32Pkg/ResetRuntimeDxe/ResetRuntimeDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  Nt32Pkg/FvbServicesRuntimeDxe/FvbServicesRuntimeDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
      !if $(SECURE_BOOT_ENABLE) == TRUE
        NULL|SecurityPkg/Parser/DxeImageVerificationLib/DxeImageVerificationLib.inf
      !endif
  }
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  Nt32Pkg/WinNtThunkDxe/WinNtThunkDxe.inf
  Nt32Pkg/CpuRuntimeDxe/CpuRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  Nt32Pkg/MiscSubClassPlatformDxe/MiscSubClassPlatformDxe.inf
  Nt32Pkg/TimerDxe/TimerDxe.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  Nt32Pkg/WinNtOemHookStatusCodeHandlerDxe/WinNtOemHookStatusCodeHandlerDxe.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/RuntimeDxe/VariableRuntimeDxe.inf
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/ SecureBootConfigDxe.inf
!else
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
!endif
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Parser/DxePcdLib/DxePcdLib.inf
  }
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Parser/DxePcdLib/DxePcdLib.inf
  }
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Parser/DxePcdLib/DxePcdLib.inf
  }
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf ## This driver follows UEFI
  ## specification definition
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf ## This driver follows UEFI
  ## specification definition
  IntelFrameworkModulePkg/Bus/Pci/IdeBusDxe/IdeBusDxe.inf
  Nt32Pkg/WinNtBusDriverDxe/WinNtBusDriverDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Parser/DxePcdLib/DxePcdLib.inf
  }
  Nt32Pkg/WinNtBlockIoDxe/WinNtBlockIoDxe.inf
  Nt32Pkg/WinNtSerialIoDxe/WinNtSerialIoDxe.inf
  Nt32Pkg/WinNtGopDxe/WinNtGopDxe.inf
  Nt32Pkg/WinNtSimpleFileSystemDxe/WinNtSimpleFileSystemDxe.inf
  MdeModulePkg/Application/HelloWorld/HelloWorld.inf
  #
  # Network stack drivers
  # To test network drivers, need network Io driver(SnpNt32Io.dll), please refer
  # to NETWORK-IO Subproject.
  #
  MdeModulePkg/Universal/Network/DpcDxe/DpcDxe.inf
  MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
  MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
  MdeModulePkg/Universal/Network/Ip4ConfigDxe/Ip4ConfigDxe.inf
  MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
  MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf
  MdeModulePkg/Universal/Network/VlanConfigDxe/VlanConfigDxe.inf
  MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
  MdeModulePkg/Universal/Network/Tcp4Dxe/Tcp4Dxe.inf
  MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf
  MdeModulePkg/Universal/Network/UefiPxeBcDxe/UefiPxeBcDxe.inf
  Nt32Pkg/SnpNt32Dxe/SnpNt32Dxe.inf
  MdeModulePkg/Universal/Network/IScsiDxe/IScsiDxe.inf
  IntelFrameworkModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  MdeModulePkg/Universal/DriverSampleDxe/DriverSampleDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Parser/BasePcdLibNull/BasePcdLibNull.inf
    <PcdsFeatureFlag>
      gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|TRUE
  }
  MdeModulePkg/Application/VariableInfo/VariableInfo.inf
  MdeModulePkg/Universal/PlatformDriOverrideDxe/PlatformDriOverrideDxe.inf

######################################################################## #
# BuildOptions Section - Define the module specific tool chain flags that
# should be used as the default flags for a
# module. These flags are appended to any
# standard flags that are defined by the build
# process. They can be applied for any modules or
# only those modules with the specific module
# style (EDK or EDKII) specified in [Components] # section.
#
########################################################################
[BuildOptions]
  DEBUG_*_IA32_DLINK_FLAGS = /BASE:0x10000 /ALIGN:4096 /FILEALIGN:4096 \\
                             /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) \\
                             /SUBSYSTEM:CONSOLE
  RELEASE_*_IA32_DLINK_FLAGS = /ALIGN:4096 /FILEALIGN:4096
  *_*_IA32_CC_FLAGS = /D EFI_SPECIFICATION_VERSION = 0x0002000A \\
                      /D TIANO_RELEASE_VERSION=0x00080006

"""

    def write_file(self, file_path, contents):
        temp_dir = tempfile.mkdtemp()
        file_path = os.path.join(temp_dir, file_path)
        f = open(file_path, "w")
        f.write(contents)
        f.close()
        return file_path

    def test_dsc_to_file_and_back_again(self):
        filepath = self.write_file("test.dsc", self.test_dsc)
        print(filepath)
        # parse the original DSC
        parser = DscParser()
        parser._ErrorLimit = 0
        # TODO: actually parse the file
        dsc_obj = []  # parser.ParseFile(filepath)
        # Write out to disk
        test_path = os.path.join(os.path.dirname(filepath), "test2.dsc")
        DscTranslator.dsc_to_file(dsc_obj, test_path)
        # parse in the outputted DSC
        parser2 = DscParser()
        parser2._ErrorLimit = 0
        print(test_path)
        # TODO: actually parse the file
        # dsc_obj2 = parser2.ParseFile(test_path)
        # self.assertNotEqual(dsc_obj, None)
        # self.assertNotEqual(dsc_obj2, None)
        # self.assertEqual(len(dsc_obj.defines), len(dsc_obj2.defines))
        # self.assertEqual(len(dsc_obj.library_classes), len(dsc_obj2.library_classes))
        # self.assertEqual(len(dsc_obj.components), len(dsc_obj2.components))
        # self.assertEqual(len(dsc_obj.build_options), len(dsc_obj2.build_options))
        # self.assertEqual(len(dsc_obj.pcds), len(dsc_obj2.pcds))
        # self.assertEqual(dsc_obj, dsc_obj2)
