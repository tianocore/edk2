## @file
#
# EFI/Framework Emulation Platform with UEFI HII interface supported.
#
# The Emulation Platform can be used to debug individual modules, prior to creating
#       a real platform. This also provides an example for how an DSC is created.
# Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
# Portions copyright (c) 2010, Apple, Inc. All rights reserved.<BR>
#
#    This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = Unix
  PLATFORM_GUID                  = 7b3c1fb4-8986-11db-b5b2-0040d02b1835
  PLATFORM_VERSION               = 0.3
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/UnixX64
  SUPPORTED_ARCHITECTURES        = X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = UnixPkg/UnixPkgX64.fdf

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses]
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  #
  # Generic Modules
  #
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  GenericBdsLib|IntelFrameworkModulePkg/Library/GenericBdsLib/GenericBdsLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  #
  # Platform
  #
  PlatformBdsLib|UnixPkg/Library/UnixBdsLib/PlatformBds.inf
  #
  # Misc
  #
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  SecDispatchTableLib|UnixPkg/Library/SecDispatchTableLib/SecDispatchTableLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf

[LibraryClasses.common.USER_DEFINED]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/PeiUnixOemHookStatusCodeLib/PeiUnixOemHookStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf

[LibraryClasses.common.PEIM, LibraryClasses.common.PEI_CORE]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeCoffGetEntryPointLib|UnixPkg/Library/EdkUnixPeiPeCoffGetEntryPointLib/EdkUnixPeiPeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|UnixPkg/Library/PeiUnixPeCoffExtraActionLib/PeiUnixPeCoffExtraActionLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

[LibraryClasses.common.PEIM]
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/PeiUnixOemHookStatusCodeLib/PeiUnixOemHookStatusCodeLib.inf

[LibraryClasses.common.PEI_CORE]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PeCoffExtraActionLib|UnixPkg/Library/DxeUnixPeCoffExtraActionLib/DxeUnixPeCoffExtraActionLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/DxeUnixOemHookStatusCodeLib/DxeUnixOemHookStatusCodeLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  UnixLib|UnixPkg/Library/DxeUnixLib/DxeUnixLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/DxeUnixOemHookStatusCodeLib/DxeUnixOemHookStatusCodeLib.inf
  PeCoffExtraActionLib|UnixPkg/Library/DxeUnixPeCoffExtraActionLib/DxeUnixPeCoffExtraActionLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  PrintLib|MdeModulePkg/Library/DxePrintLibPrint2Protocol/DxePrintLibPrint2Protocol.inf

[LibraryClasses.X64]
  # turn off CR3 write so that DXE IPL will not crash emulator
  BaseLib|UnixPkg/Library/UnixBaseLib/UnixBaseLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform.
#
################################################################################
[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreImageLoaderSearchTeSectionFirst|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseOEM|TRUE

[PcdsFixedAtBuild]
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000040
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1f
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizePopulateCapsule|0x0
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFirmwareFdSize|0x002a0000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFirmwareBlockSize|0x10000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFirmwareVolume|L"../FV/FV_RECOVERY.fd"

################################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamicDefault.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0
  gEfiUnixPkgTokenSpaceGuid.PcdUnixConsole|L"Bus Driver Console Window"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixUga|L"UGA Window"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFileSystem|L".!../../../../EdkShellBinPkg/Bin/Ia32/Apps"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixVirtualDisk|L"disk1.img:FW"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixPhysicalDisk|L"E:RW;245760;512"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixCpuModel|L"Intel(R) Processor Model"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixCpuSpeed|L"3000"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixMemorySize|L"128!128"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixSerialPort|L"/dev/ttyS0!/dev/ttyS1"
  gEfiUnixPkgTokenSpaceGuid.PcdUnixNetworkInterface|L"en0"

[PcdsDynamicHii.common.DEFAULT]
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdSetupConOutColumn|L"SetupConsoleConfig"|gEfiGlobalVariableGuid|0x0|80
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdSetupConOutRow|L"SetupConsoleConfig"|gEfiGlobalVariableGuid|0x4|25
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|10

[BuildOptions]
  # Magic to make system include files work
  XCODE:*_XCLANG_*_CC_FLAGS = -U __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ -D __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=1060 -D __APPLE__ -I/System/Library/Frameworks -I/Library/Frameworks
  XCODE:*_XCLANG_*_PP_FLAGS = -U __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ -D __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=1060 -D __APPLE__ -I/System/Library/Frameworks -I/Library/Frameworks


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
!if $(SEC_ONLY)
  ##
  #  SEC Phase modules
  ##
  UnixPkg/Sec/SecMain.inf
!else
  ##
  #  PEI Phase modules
  ##
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
   <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  IntelFrameworkModulePkg/Universal/StatusCode/Pei/StatusCodePei.inf
  UnixPkg/BootModePei/BootModePei.inf
  UnixPkg/UnixFlashMapPei/FlashMap.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  UnixPkg/UnixAutoScanPei/UnixAutoScan.inf
  UnixPkg/UnixFirmwareVolumePei/UnixFwh.inf
  UnixPkg/UnixThunkPpiToProtocolPei/UnixThunkPpiToProtocol.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf

  ##
  #  DXE Phase modules
  ##
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
      NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  UnixPkg/MetronomeDxe/Metronome.inf
  UnixPkg/RealTimeClockRuntimeDxe/RealTimeClock.inf
  UnixPkg/ResetRuntimeDxe/Reset.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  UnixPkg/FvbServicesRuntimeDxe/UnixFwh.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  UnixPkg/UnixThunkDxe/UnixThunk.inf
  UnixPkg/CpuRuntimeDxe/Cpu.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  UnixPkg/MiscSubClassPlatformDxe/MiscSubClassDriver.inf
  UnixPkg/TimerDxe/Timer.inf
  IntelFrameworkModulePkg/Universal/StatusCode/RuntimeDxe/StatusCodeRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  IntelFrameworkModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  IntelFrameworkModulePkg/Bus/Pci/IdeBusDxe/IdeBusDxe.inf
  UnixPkg/UnixBusDriverDxe/UnixBusDriver.inf

  UnixPkg/UnixBlockIoDxe/UnixBlockIo.inf
  UnixPkg/UnixSerialIoDxe/UnixSerialIo.inf
  UnixPkg/UnixUgaDxe/UnixUga.inf
  UnixPkg/UnixGopDxe/UnixGop.inf
  UnixPkg/UnixConsoleDxe/UnixConsole.inf
  UnixPkg/UnixSimpleFileSystemDxe/UnixSimpleFileSystem.inf
  MdeModulePkg/Application/HelloWorld/HelloWorld.inf

  #
  # Network stack drivers
  #
!if $(NETWORK_SUPPORT)
  UnixPkg/UnixSnpDxe/UnixSnpDxe.inf
!endif
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

  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  MdeModulePkg/Universal/DriverSampleDxe/DriverSampleDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

!if $(COMPILE_BINS)
  FatPkg/EnhancedFatDxe/Fat.inf
!endif
!endif  
