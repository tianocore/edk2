## @file
# Clanton Peak CRB platform with 32-bit DXE for 4MB/8MB flash devices.
#
# This package provides Clanton Peak CRB platform specific modules.
# Copyright (c) 2013 - 2014 Intel Corporation.
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  DSC_SPECIFICATION              = 0x00010005
  PLATFORM_NAME                  = QuarkMin
  PLATFORM_GUID                  = 2655F3CF-4CC7-4e17-A62D-77FE3F10AE7F
  PLATFORM_VERSION               = 0.1
  FLASH_DEFINITION               = QuarkPlatformPkg/QuarkMin.fdf
  OUTPUT_DIRECTORY               = Build/QuarkMin
  SUPPORTED_ARCHITECTURES        = IA32
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  VPD_TOOL_GUID                  = 8C3D856A-9BE6-468E-850A-24F7A8D38E08

  #
  # Platform On/Off features are defined here
  #
  DEFINE GALILEO             = GEN2
  DEFINE SECURE_BOOT_ENABLE  = FALSE
  DEFINE SOURCE_DEBUG_ENABLE = FALSE
  DEFINE PERFORMANCE_ENABLE  = FALSE
  DEFINE LOGGING             = FALSE

  !if $(TARGET) == "DEBUG"
    DEFINE LOGGING = TRUE
  !endif

  !if $(PERFORMANCE_ENABLE)
    DEFINE SOURCE_DEBUG_ENABLE = FALSE
    DEFINE LOGGING             = FALSE
  !endif

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

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
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
!if $(SOURCE_DEBUG_ENABLE)
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
!else
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
!endif
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  #
  # Generic Modules
  #
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  S3IoLib|MdePkg/Library/BaseS3IoLib/BaseS3IoLib.inf
  S3PciLib|MdePkg/Library/BaseS3PciLib/BaseS3PciLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
!if $(LOGGING)
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
!endif
!if $(PERFORMANCE_ENABLE)
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
!else
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
!endif
!if $(SECURE_BOOT_ENABLE)
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  PlatformSecureLib|QuarkPlatformPkg/Library/PlatformSecureLib/PlatformSecureLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
!else
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif

  #
  # CPU
  #
  MtrrLib|QuarkSocPkg/QuarkNorthCluster/Library/MtrrLib/MtrrLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf

  #
  # Quark North Cluster
  #
  SmmLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCSmmLib/QNCSmmLib.inf
  SmbusLib|QuarkSocPkg/QuarkNorthCluster/Library/SmbusLib/SmbusLib.inf
  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/DxeAcpiTimerLib.inf
  ResetSystemLib|QuarkSocPkg/QuarkNorthCluster/Library/ResetSystemLib/ResetSystemLib.inf
  IntelQNCLib|QuarkSocPkg/QuarkNorthCluster/Library/IntelQNCLib/IntelQNCLib.inf
  QNCAccessLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCAccessLib/QNCAccessLib.inf
  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf

  #
  # Quark South Cluster
  #
  IohLib|QuarkSocPkg/QuarkSouthCluster/Library/IohLib/IohLib.inf
  I2cLib|QuarkSocPkg/QuarkSouthCluster/Library/I2cLib/I2cLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf

  #
  # Quark Platform
  #
  RecoveryOemHookLib|QuarkPlatformPkg/Library/RecoveryOemHookLib/RecoveryOemHookLib.inf
  PlatformSecLib|QuarkPlatformPkg/Library/PlatformSecLib/PlatformSecLib.inf
  PlatformPcieHelperLib|QuarkPlatformPkg/Library/PlatformPcieHelperLib/PlatformPcieHelperLib.inf
  PlatformHelperLib|QuarkPlatformPkg/Library/PlatformHelperLib/DxePlatformHelperLib.inf

[LibraryClasses.common.SEC]
  #
  # SEC specific phase
  #
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf

[LibraryClasses.IA32.PEIM,LibraryClasses.IA32.PEI_CORE]
  #
  # PEI phase common
  #
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf
  PlatformHelperLib|QuarkPlatformPkg/Library/PlatformHelperLib/PeiPlatformHelperLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
!if $(SECURE_BOOT_ENABLE)
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
!endif
!if $(PERFORMANCE_ENABLE)
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
!endif

[LibraryClasses.IA32.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
!if $(PERFORMANCE_ENABLE)
  PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
!endif

[LibraryClasses.IA32.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
!if $(SECURE_BOOT_ENABLE)
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif
!if $(PERFORMANCE_ENABLE)
  PerformanceLib|MdeModulePkg/Library/SmmPerformanceLib/SmmPerformanceLib.inf
!endif

[LibraryClasses.IA32.SMM_CORE]
  SmmServicesTableLib|MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  MemoryAllocationLib|MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
!if $(SECURE_BOOT_ENABLE)
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif
!if $(PERFORMANCE_ENABLE)
  PerformanceLib|MdeModulePkg/Library/SmmCorePerformanceLib/SmmCorePerformanceLib.inf
!endif

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  QNCAccessLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCAccessLib/RuntimeQNCAccessLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
!if $(SECURE_BOOT_ENABLE)
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
!endif

[LibraryClasses.IA32.UEFI_DRIVER,LibraryClasses.IA32.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathToText|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathFromText|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciBusHotplugDeviceSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE
!if $(SOURCE_DEBUG_ENABLE)
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmDebug|TRUE
!endif

!if $(TARGET) == "RELEASE"
  gQuarkPlatformTokenSpaceGuid.WaitIfResetDueToError|FALSE
!else
  gQuarkPlatformTokenSpaceGuid.WaitIfResetDueToError|TRUE
!endif

[PcdsFixedAtBuild]
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber|1
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdS3AcpiReservedMemorySize|0x20000
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
!if $(LOGGING)
  !if $(SOURCE_DEBUG_ENABLE)
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17
  !else
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
  !endif
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0x0
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x3
!endif
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0x18
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
!if $(GALILEO) == GEN1
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|460800
!endif
!if $(GALILEO) == GEN2
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|921600
!endif
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|8
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|1
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|1
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|0
!if $(PERFORMANCE_ENABLE)
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x1
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|80
!else
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x00
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdLoadModuleAtFixAddressEnable|0
!if $(SECURE_BOOT_ENABLE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdHwErrStorageSize|0x00002000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize|0x1000
  ## RTC Update Timeout Value, need to increase timeout since also
  # waiting for RTC to be busy.
  gEfiMdeModulePkgTokenSpaceGuid.PcdRealTimeClockUpdateTimeout|500000

!if $(SECURE_BOOT_ENABLE)
  # override the default values from SecurityPkg to ensure images from all sources are verified in secure boot
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04
!endif
  gQuarkPlatformTokenSpaceGuid.PcdEsramStage1Base|0x80000000

!if $(SOURCE_DEBUG_ENABLE)
  gEfiSourceLevelDebugPkgTokenSpaceGuid.PcdDebugLoadImageMethod|0x2
!endif

  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x9000B000
!if $(GALILEO) == GEN1
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|460800
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
!endif
!if $(GALILEO) == GEN2
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|921600
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialLineControl|0x03
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0x07
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialDetectCable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|44236800
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialPciDeviceInfo|{0x14, 0x05, 0x84, 0x00, 0xFF}
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|4

  #
  #  typedef struct {
  #    UINT16  VendorId;          ///< Vendor ID to match the PCI device.  The value 0xFFFF terminates the list of entries.
  #    UINT16  DeviceId;          ///< Device ID to match the PCI device
  #    UINT32  ClockRate;         ///< UART clock rate.  Set to 0 for default clock rate of 1843200 Hz
  #    UINT64  Offset;            ///< The byte offset into to the BAR
  #    UINT8   BarIndex;          ///< Which BAR to get the UART base address
  #    UINT8   RegisterStride;    ///< UART register stride in bytes.  Set to 0 for default register stride of 1 byte.
  #    UINT16  ReceiveFifoDepth;  ///< UART receive FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  #    UINT16  TransmitFifoDepth; ///< UART transmit FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  #    UINT8   Reserved[2];
  #  } PCI_SERIAL_PARAMETER;
  #
  # Vendor 8086 Device 0936 Prog Interface 2, BAR #0, Offset 0, Stride = 4, Clock 44236800 (0x2a300000)
  # Vendor 8086 Device 0936 Prog Interface 2, BAR #0, Offset 0, Stride = 4, Clock 44236800 (0x2a300000)
  #
  #                                                       [Vendor]   [Device]  [---ClockRate---]  [------------Offset-----------] [Bar] [Stride] [RxFifo] [TxFifo]   [Rsvd]   [Vendor]
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciSerialParameters|{0x86,0x80, 0x36,0x09, 0x0,0x0,0xA3,0x02, 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x00,    0x04, 0x0,0x0, 0x0,0x0, 0x0,0x0, 0xff,0xff}

  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciBusNumber           |0
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciDeviceNumber        |31
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciFunctionNumber      |0
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciEnableRegisterOffset|0x4b
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoBarEnableMask          |0x80
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciBarRegisterOffset   |0x48
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPortBaseAddress        |0x1000
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiPm1TmrOffset             |0x0008

  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

  gEfiMdeModulePkgTokenSpaceGuid.PcdConInConnectOnDemand|FALSE

[PcdsPatchableInModule]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x803000C7
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress|0x0

[PcdsDynamicExHii.common.DEFAULT]
!if $(PERFORMANCE_ENABLE)
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|0
!else
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5
!endif
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBootState|L"BootState"|gQuarkPlatformTokenSpaceGuid|0x0|TRUE

[PcdsDynamicExDefault.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateDataPtr|0x0
  gQuarkPlatformTokenSpaceGuid.PcdEnableFastBoot|TRUE
  gQuarkPlatformTokenSpaceGuid.PcdUserIsPhysicallyPresent|FALSE
  gQuarkPlatformTokenSpaceGuid.PcdSpiFlashDeviceSize|0

[PcdsDynamicExVpd]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|*|32|L"EDK II"
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision|*|0x01000400
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|*|64|L"Galileo 1.0.4"

#
# ClantonPeakSVP
# gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0002
# gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"ClantonPeakSVP"
# gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x03, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
#
# KipsBay
# gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0003
# gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"KipsBay"
# gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
#
# CrossHill
# gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0004
# gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"CrossHill"
# gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x03, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
#
# ClantonHill
# gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0005
# gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"ClantonHill"
# gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x02, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
#
# Galileo
# gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0006
# gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"Galileo"
# gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
#
# GalileoGen2
# gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0008
# gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"GalileoGen2"
# gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
#
!if $(GALILEO) == GEN1
  gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0006
  gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"Galileo"
  gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
!endif
!if $(GALILEO) == GEN2
  gQuarkPlatformTokenSpaceGuid.PcdPlatformType|*|0x0008
  gQuarkPlatformTokenSpaceGuid.PcdPlatformTypeName|*|64|L"GalileoGen2"
  gEfiQuarkNcSocIdTokenSpaceGuid.PcdMrcParameters|*|40|{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x01, 0x7c, 0x92, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x40, 0x9c, 0x00, 0x00, 0x06}
!endif
  gEfiQuarkSCSocIdTokenSpaceGuid.PcdIohEthernetMac0|*|8|{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
  gEfiQuarkSCSocIdTokenSpaceGuid.PcdIohEthernetMac1|*|8|{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

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
#       specified in the FDF file. For example: Shell binary, FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components.IA32]
  #
  # SEC Core
  #
  UefiCpuPkg/SecCore/SecCore.inf {
    !if $(SOURCE_DEBUG_ENABLE)
      <LibraryClasses>
        PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
        DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
        ResetSystemLib|QuarkSocPkg/QuarkNorthCluster/Library/ResetSystemLib/ResetSystemLib.inf
    !endif
  }

  #
  # PEI Core
  #
  MdeModulePkg/Core/Pei/PeiMain.inf

  #
  # PEIM
  #
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf

  QuarkSocPkg/QuarkNorthCluster/MemoryInit/Pei/MemoryInitPei.inf
  QuarkPlatformPkg/Platform/Pei/PlatformInit/PlatformEarlyInit.inf
  QuarkPlatformPkg/Platform/Pei/PlatformConfig/PlatformConfigPei.inf

  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf

[Components.IA32]
  #
  # DXE Core
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf

!if $(SOURCE_DEBUG_ENABLE)
  SourceLevelDebugPkg/DebugAgentDxe/DebugAgentDxe.inf {
    <LibraryClasses>
      DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
      ResetSystemLib|QuarkSocPkg/QuarkNorthCluster/Library/ResetSystemLib/ResetSystemLib.inf
  }
!endif


  QuarkPlatformPkg/Platform/Dxe/PlatformInit/PlatformInitDxe.inf

  #
  # Components that produce the architectural protocols
  #
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  UefiCpuPkg/CpuDxe/CpuDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf {
    <LibraryClasses>
      CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  }
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf

  #
  # Following are the DXE drivers (alphabetical order)
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  QuarkPlatformPkg/Pci/Dxe/PciHostBridge/PciHostBridge.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf

  #
  # Platform
  #
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf {
    <LibraryClasses>
      UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
      PlatformBootManagerLib|QuarkPlatformPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  }

  QuarkSocPkg/QuarkNorthCluster/QNCInit/Dxe/QNCInitDxe.inf
  PcAtChipsetPkg/HpetTimerDxe/HpetTimerDxe.inf
  QuarkPlatformPkg/Platform/Dxe/Setup/DxePlatform.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf

  #
  # PCI
  #
  QuarkPlatformPkg/Pci/Dxe/PciPlatform/PciPlatform.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  QuarkSocPkg/QuarkSouthCluster/IohInit/Dxe/IohInitDxe.inf
  MdeModulePkg/Bus/Pci/PciSioSerialDxe/PciSioSerialDxe.inf

  #
  # Console
  #
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf {
    <LibraryClasses>
      CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  }
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  #
  # File System Modules
  #
  MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf

  #
  # Performance Application
  #
!if $(PERFORMANCE_ENABLE)
  PerformancePkg/Dp_App/Dp.inf {
    <LibraryClasses>
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
      FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
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
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
      FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
      PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
      BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf

    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xFF
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
      gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|8000
  }
