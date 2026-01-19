## @file
# UEFI/PI Emulation Platform with UEFI HII interface supported.
#
# The Emulation Platform can be used to debug individual modules, prior to creating
# a real platform. This also provides an example for how an DSC is created.
#
# Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
# Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.<BR>
# Copyright (c) Microsoft Corporation.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = EmulatorPkg
  PLATFORM_GUID                  = 05FD064D-1073-E844-936C-A0E16317107D
  PLATFORM_VERSION               = 0.3
  DSC_SPECIFICATION              = 0x00010005
!if $(WIN_MINGW32_BUILD)
  OUTPUT_DIRECTORY               = Build/Emulator$(ARCH)Mingw
!else
  OUTPUT_DIRECTORY               = Build/Emulator$(ARCH)
!endif

  SUPPORTED_ARCHITECTURES        = X64|IA32
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = EmulatorPkg/EmulatorPkg.fdf


  #
  # Network definition
  #
  DEFINE NETWORK_SNP_ENABLE       = FALSE
  DEFINE NETWORK_IP6_ENABLE       = FALSE
  DEFINE NETWORK_TLS_ENABLE       = FALSE
  DEFINE NETWORK_HTTP_BOOT_ENABLE = FALSE
  DEFINE NETWORK_HTTP_ENABLE      = FALSE
  DEFINE NETWORK_ISCSI_ENABLE     = FALSE
  DEFINE SECURE_BOOT_ENABLE       = FALSE

  #
  # Redfish definition
  #
  DEFINE REDFISH_ENABLE = FALSE

[SkuIds]
  0|DEFAULT

!include MdePkg/MdeLibs.dsc.inc
!include CryptoPkg/CryptoPkgFeatureFlagPcds.dsc.inc

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
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf

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
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf

  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbiosLib|EmulatorPkg/Library/SmbiosLib/SmbiosLib.inf

  #
  # Generic Modules
  #
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  #
  # Platform
  #
  PlatformBootManagerLib|EmulatorPkg/Library/PlatformBmLib/PlatformBmLib.inf
  KeyMapLib|EmulatorPkg/Library/KeyMapLibNull/KeyMapLibNull.inf
  !if $(REDFISH_ENABLE) == TRUE
    RedfishPlatformHostInterfaceLib|EmulatorPkg/Library/RedfishPlatformHostInterfaceLib/RedfishPlatformHostInterfaceLib.inf
    RedfishPlatformCredentialLib|EmulatorPkg/Library/RedfishPlatformCredentialLib/RedfishPlatformCredentialLib.inf
  !endif
  #
  # Misc
  #
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  PeiServicesTablePointerLib|EmulatorPkg/Library/PeiServicesTablePointerLibMagicPage/PeiServicesTablePointerLibMagicPage.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  CpuExceptionHandlerLib|MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  SortLib|MdeModulePkg/Library/BaseSortLib/BaseSortLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf
  RngLib|MdeModulePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
  SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  PlatformPKProtectionLib|SecurityPkg/Library/PlatformPKProtectionLibVarPolicy/PlatformPKProtectionLibVarPolicy.inf
  SecureBootVariableProvisionLib|SecurityPkg/Library/SecureBootVariableProvisionLib/SecureBootVariableProvisionLib.inf
!else
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif

[LibraryClasses.common.SEC]
  PeiServicesLib|EmulatorPkg/Library/SecPeiServicesLib/SecPeiServicesLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PeCoffGetEntryPointLib|EmulatorPkg/Library/PeiEmuPeCoffGetEntryPointLib/PeiEmuPeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|EmulatorPkg/Library/PeiEmuPeCoffExtraActionLib/PeiEmuPeCoffExtraActionLib.inf
  SerialPortLib|EmulatorPkg/Library/PeiEmuSerialPortLib/PeiEmuSerialPortLib.inf
  PpiListLib|EmulatorPkg/Library/SecPpiListLib/SecPpiListLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  TimerLib|EmulatorPkg/Library/PeiTimerLib/PeiTimerLib.inf

[LibraryClasses.common.HOST_APPLICATION, LibraryClasses.common.BASE]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PpiListLib|EmulatorPkg/Library/SecPpiListLib/SecPpiListLib.inf
  ThunkPpiList|EmulatorPkg/Library/ThunkPpiList/ThunkPpiList.inf
  ThunkProtocolList|EmulatorPkg/Library/ThunkProtocolList/ThunkProtocolList.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PpiListLib|EmulatorPkg/Library/SecPpiListLib/SecPpiListLib.inf
  PeiServicesLib|EmulatorPkg/Library/SecPeiServicesLib/SecPeiServicesLib.inf
  StackCheckLib|MdePkg/Library/StackCheckLibNull/StackCheckLibNullHostApplication.inf

[LibraryClasses.common.PEIM, LibraryClasses.common.PEI_CORE]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeCoffGetEntryPointLib|EmulatorPkg/Library/PeiEmuPeCoffGetEntryPointLib/PeiEmuPeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|EmulatorPkg/Library/PeiEmuPeCoffExtraActionLib/PeiEmuPeCoffExtraActionLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  SerialPortLib|EmulatorPkg/Library/PeiEmuSerialPortLib/PeiEmuSerialPortLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  TimerLib|EmulatorPkg/Library/PeiTimerLib/PeiTimerLib.inf

[LibraryClasses.common.PEI_CORE]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.common.PEIM]
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PeCoffExtraActionLib|EmulatorPkg/Library/DxeEmuPeCoffExtraActionLib/DxeEmuPeCoffExtraActionLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  TimerLib|EmulatorPkg/Library/DxeCoreTimerLib/DxeCoreTimerLib.inf
  EmuThunkLib|EmulatorPkg/Library/DxeEmuLib/DxeEmuLib.inf

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  EmuThunkLib|EmulatorPkg/Library/DxeEmuLib/DxeEmuLib.inf
  PeCoffExtraActionLib|EmulatorPkg/Library/DxeEmuPeCoffExtraActionLib/DxeEmuPeCoffExtraActionLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  TimerLib|EmulatorPkg/Library/DxeTimerLib/DxeTimerLib.inf

[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreImageLoaderSearchTeSectionFirst|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplBuildPageTables|FALSE
  gEmulatorPkgTokenSpaceGuid.PcdEmulatorLazyLoadSymbols|FALSE
!if $(WIN_MINGW32_BUILD)
  #
  # When WIN_MINGW32_BUILD is set, -target is set to build Windows application.
  # Set PcdOpensslLibAssemblySourceStyleNasm to TRUE to use Openssl NASM
  # source files that assume a Windows calling convention.
  #
  gEfiCryptoPkgTokenSpaceGuid.PcdOpensslLibAssemblySourceStyleNasm|TRUE
!endif

[PcdsFixedAtBuild]
  gEfiMdeModulePkgTokenSpaceGuid.PcdImageProtectionPolicy|0x00000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000040
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1f
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizePopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|TRUE

  gEmulatorPkgTokenSpaceGuid.PcdEmuFirmwareFdSize|0x002a0000
  gEmulatorPkgTokenSpaceGuid.PcdEmuFirmwareBlockSize|0x10000
  gEmulatorPkgTokenSpaceGuid.PcdEmuFirmwareVolume|L"../FV/FV_RECOVERY.fd"
!if $(SECURE_BOOT_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800
  gEfiSecurityPkgTokenSpaceGuid.PcdUserPhysicalPresence|TRUE
!endif

  gEmulatorPkgTokenSpaceGuid.PcdEmuMemorySize|L"64!64"

  # Change PcdBootManagerMenuFile to UiApp
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }


#define BOOT_WITH_FULL_CONFIGURATION                  0x00
#define BOOT_WITH_MINIMAL_CONFIGURATION               0x01
#define BOOT_ASSUMING_NO_CONFIGURATION_CHANGES        0x02
#define BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS 0x03
#define BOOT_WITH_DEFAULT_SETTINGS                    0x04
#define BOOT_ON_S4_RESUME                             0x05
#define BOOT_ON_S5_RESUME                             0x06
#define BOOT_ON_S2_RESUME                             0x10
#define BOOT_ON_S3_RESUME                             0x11
#define BOOT_ON_FLASH_UPDATE                          0x12
#define BOOT_IN_RECOVERY_MODE                         0x20
  gEmulatorPkgTokenSpaceGuid.PcdEmuBootMode|0

  gEmulatorPkgTokenSpaceGuid.PcdEmuApCount|L"1"

  # For a CD-ROM/DVD use L"diag.dmg:RO:2048"
  gEmulatorPkgTokenSpaceGuid.PcdEmuVirtualDisk|L"disk.dmg:FW"
  gEmulatorPkgTokenSpaceGuid.PcdEmuGop|L"GOP Window"
  gEmulatorPkgTokenSpaceGuid.PcdEmuFileSystem|L"."
  gEmulatorPkgTokenSpaceGuid.PcdEmuSerialPort|L"/dev/ttyS0"
  gEmulatorPkgTokenSpaceGuid.PcdEmuNetworkInterface|L"en0"

  gEmulatorPkgTokenSpaceGuid.PcdEmuCpuModel|L"Intel(R) Processor Model"
  gEmulatorPkgTokenSpaceGuid.PcdEmuCpuSpeed|L"3000"

  #  0-PCANSI, 1-VT100, 2-VT00+, 3-UTF8, 4-TTYTERM
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|1

!if $(REDFISH_ENABLE) == TRUE
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceDevicePath.DevicePathMatchMode|DEVICE_PATH_MATCH_MAC_NODE
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceDevicePath.DevicePathNum|1
  #
  # Below is the MAC address of network adapter on EDK2 Emulator platform.
  # You can use ifconfig under EFI shell to get the MAC address of network adapter on EDK2 Emulator platform.
  #
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceDevicePath.DevicePath|{DEVICE_PATH("MAC(000000000000,0x1)")}
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishRestExServiceAccessModeInBand|False
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishDiscoverAccessModeInBand|False

  gEmulatorPkgTokenSpaceGuid.PcdRedfishServiceStopIfSecureBootDisabled|False
  gEmulatorPkgTokenSpaceGuid.PcdRedfishServiceStopIfExitbootService|False

  gEfiRedfishClientPkgTokenSpaceGuid.PcdRedfishServiceEtagSupported|False

  #
  # Redfish Debug enablement
  #
  # 0x0000000000000001  RedfishPlatformConfigDxe driver debug enabled.
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishDebugCategory|0
  #   0x00000001  x-uefi-redfish string database message enabled
  #   0x00000002  Debug Message for dumping formset
  #   0x00000004  Debug Message for x-uefi-redfish searching result
  #   0x00000008  Debug Message for x-uefi-redfish Regular Expression searching result
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishPlatformConfigDebugProperty|0

  # Redfish Platform Configure DXE driver feature enablement
  #   0x00000001  Enable building Redfish Attribute Registry menu path.
  #   0x00000002  Allow supressed HII option to be exposed on Redfish.
  gEfiRedfishPkgTokenSpaceGuid.PcdRedfishPlatformConfigFeatureProperty|0
!endif

[PcdsDynamicDefault.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0

[PcdsDynamicHii.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|L"Setup"|gEmuSystemConfigGuid|0x0|80
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|L"Setup"|gEmuSystemConfigGuid|0x4|25
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|10

[Components]
!if "IA32" in $(ARCH) || "X64" in $(ARCH)
  !if $(WIN_HOST_BUILD)
    ##
    #  Emulator, OS WIN application
    #
    EmulatorPkg/Win/Host/WinHost.inf
  !else
    ##
    #  Emulator, OS POSIX application
    #
    EmulatorPkg/Unix/Host/Host.inf
  !endif
!endif

!ifndef $(SKIP_MAIN_BUILD)
  #
  # Generic SEC
  #
  EmulatorPkg/Sec/Sec.inf

  ##
  #  PEI Phase modules
  ##
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
   <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf

  EmulatorPkg/BootModePei/BootModePei.inf
  MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  EmulatorPkg/AutoScanPei/AutoScanPei.inf
  EmulatorPkg/FirmwareVolumePei/FirmwareVolumePei.inf
  EmulatorPkg/FlashMapPei/FlashMapPei.inf
  EmulatorPkg/ThunkPpiToProtocolPei/ThunkPpiToProtocolPei.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf

  ##
  #  DXE Phase modules
  ##
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      SerialPortLib|EmulatorPkg/Library/DxeEmuStdErrSerialPortLib/DxeEmuStdErrSerialPortLib.inf
      DxeEmuLib|EmulatorPkg/Library/DxeEmuLib/DxeEmuLib.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf {
   <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      SerialPortLib|EmulatorPkg/Library/DxeEmuStdErrSerialPortLib/DxeEmuStdErrSerialPortLib.inf
  }

  MdeModulePkg/Universal/Metronome/Metronome.inf
  EmulatorPkg/RealTimeClockRuntimeDxe/RealTimeClock.inf
  EmulatorPkg/ResetRuntimeDxe/Reset.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  EmulatorPkg/FvbServicesRuntimeDxe/FvbServicesRuntimeDxe.inf

  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
!if $(SECURE_BOOT_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
!endif
  }

  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  EmulatorPkg/EmuThunkDxe/EmuThunk.inf
  EmulatorPkg/CpuRuntimeDxe/Cpu.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  EmulatorPkg/PlatformSmbiosDxe/PlatformSmbiosDxe.inf
  EmulatorPkg/TimerDxe/Timer.inf

  #
  # Rng Protocol producer
  #
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf
  #
  # Hash2 Protocol producer
  #
  SecurityPkg/Hash2DxeCrypto/Hash2DxeCrypto.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!endif

  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
  }
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf {
   <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      SerialPortLib|EmulatorPkg/Library/DxeEmuSerialPortLib/DxeEmuSerialPortLib.inf
  }

  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
!if "XCODE5" not in $(TOOL_CHAIN_TAG)
  MdeModulePkg/Logo/LogoDxe.inf
!endif
  MdeModulePkg/Universal/LoadFileOnFv2/LoadFileOnFv2.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
   <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf

  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  #{
  #  <LibraryClasses>
  #    NULL|EmulatorPkg/Library/DevicePathTextLib/DevicePathTextLib.inf
  #}

  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  EmulatorPkg/EmuBusDriverDxe/EmuBusDriverDxe.inf
  EmulatorPkg/EmuGopDxe/EmuGopDxe.inf
  EmulatorPkg/EmuSimpleFileSystemDxe/EmuSimpleFileSystemDxe.inf
  EmulatorPkg/EmuBlockIoDxe/EmuBlockIoDxe.inf
  EmulatorPkg/EmuSnpDxe/EmuSnpDxe.inf

  MdeModulePkg/Application/HelloWorld/HelloWorld.inf

  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  MdeModulePkg/Universal/DriverSampleDxe/DriverSampleDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  FatPkg/EnhancedFatDxe/Fat.inf

!if "XCODE5" not in $(TOOL_CHAIN_TAG)
  ShellPkg/DynamicCommand/TftpDynamicCommand/TftpDynamicCommand.inf {
    <PcdsFixedAtBuild>
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }
!endif
  ShellPkg/Application/Shell/Shell.inf {
    <LibraryClasses>
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
      SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
      PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
#      SafeBlockIoLib|ShellPkg/Library/SafeBlockIoLib/SafeBlockIoLib.inf
#      SafeOpenProtocolLib|ShellPkg/Library/SafeOpenProtocolLib/SafeOpenProtocolLib.inf
      BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
      IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|8000
  }

!endif

!include NetworkPkg/Network.dsc.inc

!if $(REDFISH_ENABLE) == TRUE
  EmulatorPkg/Application/RedfishPlatformConfig/RedfishPlatformConfig.inf
!endif
!include RedfishPkg/Redfish.dsc.inc

#
# Fail with error message if the OS/Compiler combination is not supported
#
# Operating System and Compiler Compatibility Matrix for EmulatorPkg
#
# +--------------------+--------+----------+------------+-----+----+--------+
# | OS/Compiler        | VS2019 | CLANGPDB | CLANGDWARF |   GCC    | XCODE5 |
# |                    | VS2022 |          |            |   GCC5   |        |
# |                    |        |          |            | GCCNOLTO |        |
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
#   * EmulatorPkg builds depend on Visual Studio includes/libraries
# * Windows/LLVM/MSYS2: Windows environment with MSYS2 based LLVM 20.1.8 or
#   higher installed.
#   * https://github.com/tianocore/edk2-edkrepo/releases?q=clang
#   * EmulatorPkg builds do not use any Visual Studio includes/libraries
#   * MSYS2 based LLVM 20.1.8 does not provide IA32 includes/libraries
# * Windows/LLVM/MINGW: Windows environment with LLVM MINGW 20.1.8 or higher
#   installed with UCRT includes/libraries.
#   * https://github.com/mstorsjo/llvm-mingw/releases
#   * EmulatorPkg builds do not use any Visual Studio includes/libraries
#   * UCRT release provide IA32 and X64 includes/libraries
# * Linux/LLVM or Linux/GCC: Linux environment with GCC 13 or higher and
#   LLVM 20.1.8 or higher installed. Not all Linux distributions provide IA32
#   includes/libraries for Host-based unit tests.
#   * EmulatorPkg builds use GCC and LLVM includes/libraries
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
  !if $(TOOL_CHAIN_TAG) in "VS2019 VS2022"
    !error EmulatorPkg not supported for Mingw/VS20xx builds
  !endif
  !if $(TOOL_CHAIN_TAG) in "CLANGPDB"
    !error EmulatorPkg not supported for Mingw/CLANGPDB builds
  !endif
  !if $(TOOL_CHAIN_TAG) in "GCC GCC5 GCCNOLTO"
    !error EmulatorPkg not supported for Mingw/GCC builds
  !endif
!else
  !if $(WIN_HOST_BUILD)
    !if $(TOOL_CHAIN_TAG) in "GCC GCC5 GCCNOLTO"
      !error EmulatorPkg not supported for Windows/GCC builds
    !endif
    !if $(TOOL_CHAIN_TAG) in "CLANGDWARF"
      !error EmulatorPkg not supported for Windows/CLANGDWARF builds
    !endif
  !else
    !if $(TOOL_CHAIN_TAG) in "VS2019 VS2022"
      !error EmulatorPkg not supported for Linux/VS20xx builds
    !endif
    !if $(TOOL_CHAIN_TAG) in "CLANGPDB"
      !error EmulatorPkg not supported for Linux/CLANGPDB builds
    !endif
  !endif
!endif

[BuildOptions]
  #
  # Disable deprecated APIs.
  #
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES

  #
  # Windows/VS20xx using Visual Studio includes and libraries
  #
  MSFT:DEBUG_*_*_CC_FLAGS = /Od /Oy-
  MSFT:NOOPT_*_*_CC_FLAGS = /Od /Oy-

  MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096 /FILEALIGN:4096
  MSFT:*_*_IA32_DLINK_FLAGS  = /BASE:0x010000000
  MSFT:*_*_X64_DLINK_FLAGS   = /BASE:0x180000000

  #
  # Windows/CLANGPDB using Visual Studio includes and libraries
  #
  CLANGPDB:RELEASE_*_*_CC_FLAGS = -g0
  CLANGPDB:DEBUG_*_*_CC_FLAGS   = -g3 -fno-lto -O0
  CLANGPDB:NOOPT_*_*_CC_FLAGS   = -g3 -fno-lto

  CLANGPDB:*_*_*_DLINK_FLAGS     = /ALIGN:4096 /FILEALIGN:4096
  CLANGPDB:*_*_IA32_DLINK_FLAGS  = /BASE:0x010000000
  CLANGPDB:*_*_X64_DLINK_FLAGS   = /BASE:0x180000000

!if $(WIN_MINGW32_BUILD)
  #
  # Windows/Mingw CLANGDWARF using Mingw CLANG includes and libraries
  #
  GCC:*_CLANGDWARF_IA32_PP_FLAGS    = -target i686-w64-mingw32
  GCC:*_CLANGDWARF_X64_PP_FLAGS     = -target x86_64-w64-mingw32

  GCC:*_CLANGDWARF_X64_CC_FLAGS     = -target x86_64-w64-mingw32 -mno-stack-arg-probe
  GCC:*_CLANGDWARF_IA32_CC_FLAGS    = -target i686-w64-mingw32 -mno-stack-arg-probe
  GCC:RELEASE_CLANGDWARF_*_CC_FLAGS = -g0
  GCC:DEBUG_CLANGDWARF_*_CC_FLAGS   = -g3 -gcodeview -fdebug-macro -fno-omit-frame-pointer -fexceptions -fno-lto -O0
  GCC:NOOPT_CLANGDWARF_*_CC_FLAGS   = -g3 -gcodeview -fdebug-macro -fno-omit-frame-pointer -fexceptions -fno-lto

  GCC:*_CLANGDWARF_X64_NASM_FLAGS  = -f win64
  GCC:*_CLANGDWARF_IA32_NASM_FLAGS = -f win32

  #
  # Change Windows/Mingw CLANGDWARF to use llvm-rc instead of objcopy to
  # produce HII resources as a PE/COFF library
  #
  GCC:*_CLANGDWARF_*_GENFWHII_FLAGS == --hiipackage
  GCC:*_CLANGDWARF_*_RC_PATH         = llvm-rc
  GCC:*_CLANGDWARF_*_RC_FLAGS       ==

  #
  # Must override DLINK to use options compatible with Mingw CLANG that is
  # subset of GCC options for Windows application targets.
  #
  # DLINK action does not support -gcodeview that must be set in CC_FLAGS
  # to generate PDB symbol information. -Wno-unused-command-line-argument
  # is added to ignore the error generated by -gcodeview in DLINK action.
  #
  GCC:*_CLANGDWARF_*_DLINK_FLAGS     == -nostdlib -shared -Wl,--section-alignment=0x1000 -Wl,--file-alignment=0x1000
  GCC:*_CLANGDWARF_IA32_DLINK_FLAGS  = -target i686-w64-mingw32 -Wl,--entry,__ModuleEntryPoint
  GCC:*_CLANGDWARF_X64_DLINK_FLAGS   = -target x86_64-w64-mingw32 -Wl,--entry,_ModuleEntryPoint
  GCC:DEBUG_CLANGDWARF_*_DLINK_FLAGS = -g -Wl,--pdb,$(DEBUG_DIR)/$(BASE_NAME).pdb -Wno-unused-command-line-argument
  GCC:NOOPT_CLANGDWARF_*_DLINK_FLAGS = -g -Wl,--pdb,$(DEBUG_DIR)/$(BASE_NAME).pdb -Wno-unused-command-line-argument
  #
  # Set DLINK2_FLAGS to empyty string to disable use of linker script
  #
  GCC:*_CLANGDWARF_X64_DLINK2_FLAGS  ==
  GCC:*_CLANGDWARF_IA32_DLINK2_FLAGS ==
!endif

  GCC:RELEASE_*_*_CC_FLAGS = -g0
  GCC:DEBUG_*_*_CC_FLAGS   = -g3 -fno-lto -O0
  GCC:NOOPT_*_*_CC_FLAGS   = -g3 -fno-lto

  #
  # GCC IA32 CLANGDWARF does not work with -flto.
  # Disable LTO for this specific tool chain configuration
  #
  GCC:RELEASE_CLANGDWARF_IA32_CC_FLAGS = -fno-lto

[Defines]
  #
  # Defines for settings that are common between MSFT and CLANGPDB tool chain
  # families that must use Visual Studio specific defines and libraries when
  # building modules of type HOST_APPLICATION
  #
!if $(ARCH) in "X64"
  DEFINE VS_ARCH_DIR = x64
!endif
!if $(ARCH) in "IA32"
  DEFINE VS_ARCH_DIR = x86
!endif
  DEFINE VISUAL_STUDIO_DEFINES   = -D UNICODE -D _CRT_SECURE_NO_WARNINGS -D _CRT_SECURE_NO_DEPRECATE
  DEFINE VISUAL_STUDIO_LIB_PATHS = /LIBPATH:"%VCToolsInstallDir%lib\$(VS_ARCH_DIR)" /LIBPATH:"%UniversalCRTSdkDir%lib\%UCRTVersion%\ucrt\$(VS_ARCH_DIR)" /LIBPATH:"%WindowsSdkDir%lib\%WindowsSDKLibVersion%um\$(VS_ARCH_DIR)"
  DEFINE VISUAL_STUDIO_LIBS      = Kernel32.lib MSVCRTD.lib vcruntimed.lib ucrtd.lib Gdi32.lib User32.lib Winmm.lib Advapi32.lib

[BuildOptions.common.EDKII.HOST_APPLICATION]
  MSFT:*_*_*_CC_FLAGS        = $(VISUAL_STUDIO_DEFINES)
  #
  # Must ovveride DLINK_FLAGS to remove /DLL when linking .exe
  #
  MSFT:*_*_*_DLINK_FLAGS    == /out:"$(BIN_DIR)\$(BASE_NAME).exe" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /MAP /OPT:REF /LTCG
  MSFT:*_*_*_DLINK_FLAGS     = $(VISUAL_STUDIO_LIB_PATHS) $(VISUAL_STUDIO_LIBS)
  MSFT:DEBUG_*_*_DLINK_FLAGS = /DEBUG /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb"
  MSFT:NOOPT_*_*_DLINK_FLAGS = /DEBUG /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb"

  CLANGPDB:*_*_*_CC_FLAGS        = $(VISUAL_STUDIO_DEFINES)
  #
  # Must ovveride DLINK_FLAGS to remove /DLL when linking .exe
  #
  CLANGPDB:*_*_*_DLINK_FLAGS    == /OUT:"$(BIN_DIR)\$(BASE_NAME).exe" /NOLOGO /SUBSYSTEM:CONSOLE /IGNORE:4086 /OPT:REF /LLDMAP
  CLANGPDB:*_*_*_DLINK_FLAGS     = $(VISUAL_STUDIO_LIB_PATHS) $(VISUAL_STUDIO_LIBS)
  CLANGPDB:DEBUG_*_*_DLINK_FLAGS = /DEBUG /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb"
  CLANGPDB:NOOPT_*_*_DLINK_FLAGS = /DEBUG /pdb:"$(BIN_DIR)\$(BASE_NAME).pdb"

  #
  # GCC RELEASE X64 Host application does not work with -flto.
  # Disable LTO for this specific tool chain configuration
  #
  GCC:RELEASE_*_X64_CC_FLAGS = -fno-lto
  #
  # Must override DLINK flags to remove -nostdlib because Host application must
  # link against standard libraries
  #
  GCC:*_*_*_DLINK_FLAGS      == -o $(BIN_DIR)/Host -z noexecstack
  GCC:RELEASE_*_*_DLINK_FLAGS = -flto
  GCC:*_*_IA32_DLINK_FLAGS    = -m32
  GCC:*_*_X64_DLINK_FLAGS     = -m64

  GCC:*_*_*_DLINK2_FLAGS == -lpthread -ldl -lXext -lX11

!if $(WIN_MINGW32_BUILD)
  #
  # Windows Mingw CLANGDWARF
  #
  GCC:*_CLANGDWARF_*_CC_FLAGS       = -D UNICODE -D _CRT_SECURE_NO_DEPRECATE -Wno-incompatible-pointer-types
  GCC:RELEASE_CLANGDWARF_*_CC_FLAGS = -g0
  GCC:DEBUG_CLANGDWARF_*_CC_FLAGS   = -g3 -gcodeview -fdebug-macro -fno-omit-frame-pointer -fexceptions -fno-lto -O0
  GCC:NOOPT_CLANGDWARF_*_CC_FLAGS   = -g3 -gcodeview -fdebug-macro -fno-omit-frame-pointer -fexceptions -fno-lto

  #
  # Must override DLINK to use options compatible with Mingw CLANG that is
  # subset of GCC options for Windows application targets.
  #
  # DLINK action does not support -gcodeview that must be set in CC_FLAGS
  # to generate PDB symbol information. -Wno-unused-command-line-argument
  # is added to ignore the error generated by -gcodeview in DLINK action.
  #
  GCC:*_CLANGDWARF_*_DLINK_FLAGS    == -o $(BIN_DIR)/$(BASE_NAME).exe -Wl,--entry,main -lwinmm -lgdi32
  GCC:*_CLANGDWARF_IA32_DLINK_FLAGS  = -target i686-w64-mingw32
  GCC:*_CLANGDWARF_X64_DLINK_FLAGS   = -target x86_64-w64-mingw32
  GCC:DEBUG_CLANGDWARF_*_DLINK_FLAGS = -g -Wl,--pdb,$(BIN_DIR)/$(BASE_NAME).pdb -Wno-unused-command-line-argument
  GCC:NOOPT_CLANGDWARF_*_DLINK_FLAGS = -g -Wl,--pdb,$(BIN_DIR)/$(BASE_NAME).pdb -Wno-unused-command-line-argument
  #
  # DLINK2_FLAGS must be set to empty string to disable use of linker script
  #
  GCC:*_CLANGDWARF_IA32_DLINK2_FLAGS ==
  GCC:*_CLANGDWARF_X64_DLINK2_FLAGS  ==
!endif

  #
  # Need to do XCODE link via gcc and not ld as the pathing to libraries changes
  # from OS version to OS version
  #
  XCODE:*_*_IA32_DLINK_PATH == gcc
  XCODE:*_*_IA32_CC_FLAGS = -I$(WORKSPACE)/EmulatorPkg/Unix/Host/X11IncludeHack
  XCODE:*_*_IA32_DLINK_FLAGS == -arch i386 -o $(BIN_DIR)/Host -L/usr/X11R6/lib -lXext -lX11 -framework Carbon
  XCODE:*_*_IA32_ASM_FLAGS == -arch i386 -g

  XCODE:*_*_X64_DLINK_PATH == gcc
  XCODE:*_*_X64_DLINK_FLAGS == -o $(BIN_DIR)/Host -L/usr/X11R6/lib -lXext -lX11 -framework Carbon -Wl,-no_pie
  XCODE:*_*_X64_ASM_FLAGS == -g
  XCODE:*_*_X64_CC_FLAGS = -O0 -target x86_64-apple-darwin -I$(WORKSPACE)/EmulatorPkg/Unix/Host/X11IncludeHack "-DEFIAPI=__attribute__((ms_abi))"
