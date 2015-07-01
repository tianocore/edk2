#/** @file
# Platform description.
#
# Copyright (c) 2012  - 2015, Intel Corporation. All rights reserved.<BR>
#                                                                                  
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.  
# The full text of the license may be found at                                     
# http://opensource.org/licenses/bsd-license.php.                                  
#                                                                                  
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
#                                                                                  
#
#**/

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                       = Vlv2TbltDevicePkg
  PLATFORM_GUID                       = 465B0A0B-7AC1-443b-8F67-7B8DEC145F90
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005

  #
  # Set platform specific package/folder name, same as passed from PREBUILD script.
  # PLATFORM_PACKAGE would be the same as PLATFORM_NAME as well as package build folder
  # DEFINE only takes effect at R9 DSC and FDF.
  #
  DEFINE      PLATFORM_PACKAGE                = Vlv2TbltDevicePkg
  DEFINE      PLATFORM_RC_PACKAGE             = Vlv2DeviceRefCodePkg
  DEFINE      PLATFORM_BINARY_PACKAGE         = Vlv2BinaryPkg
  OUTPUT_DIRECTORY                    = Build/$(PLATFORM_PACKAGE)
  SUPPORTED_ARCHITECTURES             = IA32
  BUILD_TARGETS                       = DEBUG|RELEASE
  SKUID_IDENTIFIER                    = DEFAULT

  DEFINE CPU_ARCH                 =ValleyView2
  DEFINE PROJECT_SC_FAMILY        =IntelPch
  DEFINE PROJECT_SC_ROOT          =../$(PLATFORM_RC_PACKAGE)/ValleyView2Soc/SouthCluster
  DEFINE PROJECT_VLV_ROOT          =../$(PLATFORM_RC_PACKAGE)/ValleyView2Soc/NorthCluster

  DEFINE RC_BINARY_RELEASE        = TRUE
  #
  # Platform On/Off features are defined here
  #
  #
  # Platform Support:: Set only one token except Crestview Hills
  #
  #   3.BayleyBay
  #     ENBDT_PF_ENABLE  = TRUE
  #
  !include $(PLATFORM_PACKAGE)/AutoPlatformCFG.txt
  !include $(PLATFORM_PACKAGE)/PlatformPkgConfig.dsc

!if $(X64_CONFIG) == TRUE
  DEFINE      DXE_ARCHITECTURE        = X64
  DEFINE      EDK_DXE_ARCHITECTURE    = X64
  DEFINE      UNDI_DXE_ARCHITECTURE   = 64
!else
  DEFINE      DXE_ARCHITECTURE        = IA32
  DEFINE      EDK_DXE_ARCHITECTURE    = Ia32
  DEFINE      UNDI_DXE_ARCHITECTURE   = 32
!endif

  FLASH_DEFINITION                    = $(PLATFORM_PACKAGE)/PlatformPkg.fdf
!if $(LFMA_ENABLE) == TRUE
  FIX_LOAD_TOP_MEMORY_ADDRESS         = 0xFFFFFFFFFFFFFFFF
  DEFINE   TOP_MEMORY_ADDRESS         = 0xFFFFFFFFFFFFFFFF
!else
  FIX_LOAD_TOP_MEMORY_ADDRESS         = 0x0
  DEFINE   TOP_MEMORY_ADDRESS         = 0x0
!endif

  DEFINE   PLATFORM_PCIEXPRESS_BASE   = 0E0000000

  DEFINE SEC_ENABLE = TRUE
  DEFINE SEC_DEBUG_INFO_ENABLE = TRUE
  DEFINE FTPM_ENABLE = TRUE

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
[LibraryClasses.common]
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  DxeSmmDriverEntryPoint|IntelFrameworkPkg/Library/DxeSmmDriverEntryPoint/DxeSmmDriverEntryPoint.inf

  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
!if $(SSE2_ENABLE) == TRUE
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibSse2/BaseMemoryLibSse2.inf
!else
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
!endif
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
!if $(RC_BINARY_RELEASE) == TRUE
  PchPlatformLib|Vlv2TbltDevicePkg/Library/PchPlatformLib/PchPlatformLib.inf
!endif
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
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  GenericBdsLib|$(PLATFORM_PACKAGE)/Override/IntelFrameworkModulePkg/Library/GenericBdsLib/GenericBdsLib.inf
  PlatformBdsLib|$(PLATFORM_PACKAGE)/Library/PlatformBdsLib/PlatformBdsLib.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
  FlashDeviceLib|$(PLATFORM_PACKAGE)/Library/FlashDeviceLib/FlashDeviceLib.inf
  #
  # Framework
  #
!if $(S3_ENABLE) == TRUE
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
!else
  S3BootScriptLib|MdePkg/Library/BaseS3BootScriptLibNull/BaseS3BootScriptLibNull.inf
!endif
  S3IoLib|MdePkg/Library/BaseS3IoLib/BaseS3IoLib.inf
  S3PciLib|MdePkg/Library/BaseS3PciLib/BaseS3PciLib.inf

  #
  # Generic Modules
  #
!if $(USB_ENABLE) == TRUE
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
!endif
!if $(SCSI_ENABLE) == TRUE
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
!endif
!if $(NETWORK_ENABLE) == TRUE
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  TcpIoLib|MdeModulePkg/Library/DxeTcpIoLib/DxeTcpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
!endif
!if $(S3_ENABLE) == TRUE
  S3Lib|IntelFrameworkModulePkg/Library/PeiS3Lib/PeiS3Lib.inf
!endif

  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
!if $(CAPSULE_ENABLE) == TRUE
 CapsuleLib|IntelFrameworkModulePkg/Library/DxeCapsuleLib/DxeCapsuleLib.inf
!else
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
!endif
  LanguageLib|EdkCompatibilityPkg/Compatibility/Library/UefiLanguageLib/UefiLanguageLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf

  #
  # CPU
  #
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  CpuExceptionHandlerLib|MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf

  #
  # ICH
  #
  SmbusLib|$(PLATFORM_PACKAGE)/Library/SmbusLib/SmbusLib.inf
  SmmLib|$(PLATFORM_PACKAGE)/Library/PchSmmLib/PchSmmLib.inf

  #
  # Platform
  #
  TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
  ResetSystemLib|$(PLATFORM_PACKAGE)/Library/ResetSystemLib/ResetSystemLib.inf

  PlatformCmosLib|$(PLATFORM_PACKAGE)/Library/PlatformCmosLib/PlatformCmosLib.inf

  #
  # Misc
  #
  MonoStatusCodeLib|$(PLATFORM_PACKAGE)/MonoStatusCode/MonoStatusCode.inf
!if $(TARGET) == RELEASE
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  SerialPortLib|$(PLATFORM_PACKAGE)/Library/SerialPortLib/SerialPortLib.inf
!endif

  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!if $(TPM_ENABLED) == TRUE
  TpmCommLib|SecurityPkg/Library/TpmCommLib/TpmCommLib.inf
  Tpm12CommandLib|SecurityPkg/Library/Tpm12CommandLib/Tpm12CommandLib.inf
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibDTpm/Tpm12DeviceLibDTpm.inf

!endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
!else
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
!endif

  #
  # CryptLib
  #
!if $(TPM_ENABLED) == TRUE
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
!endif

 BiosIdLib|$(PLATFORM_PACKAGE)/Library/BiosIdLib/BiosIdLib.inf
 CpuIA32Lib|$(PLATFORM_PACKAGE)/Library/CpuIA32Lib/CpuIA32Lib.inf

  StallSmmLib|$(PLATFORM_PACKAGE)/Library/StallSmmLib/StallSmmLib.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
!else
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif
!if $(RC_BINARY_RELEASE) == TRUE
  I2cLib|Vlv2TbltDevicePkg/Library/I2CLib/I2CLibNull.inf
!endif
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
!if $(FTPM_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
!endif
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  TrEEPhysicalPresenceLib|SecurityPkg/Library/DxeTrEEPhysicalPresenceLib/DxeTrEEPhysicalPresenceLib.inf
!if $(FTPM_ENABLE) == TRUE  
  TrEEPpVendorLib|SecurityPkg/Library/TrEEPpVendorLibNull/TrEEPpVendorLibNull.inf
!endif  
  
  
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
!if $(MINNOW2_FSP_BUILD) == TRUE
  FspApiLib|IntelFspWrapperPkg/Library/BaseFspApiLib/BaseFspApiLib.inf
  FspPlatformInfoLib|IntelFspWrapperPkg/Library/BaseFspPlatformInfoLibSample/BaseFspPlatformInfoLibSample.inf
  FspPlatformSecLib|Vlv2TbltDevicePkg/FspSupport/Library/SecFspPlatformSecLibVlv2/FspPlatformSecLibVlv2.inf
  FspHobProcessLib|Vlv2TbltDevicePkg/FspSupport/Library/PeiFspHobProcessLibVlv2/FspHobProcessLibVlv2.inf
!endif

[LibraryClasses.IA32.SEC]
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
!endif
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.IA32.PEIM, LibraryClasses.IA32.PEI_CORE, LibraryClasses.IA32.SEC]
  #
  # PEI phase common
  #

  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  MultiPlatformLib|$(PLATFORM_PACKAGE)/Library/MultiPlatformLib/MultiPlatformLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf


!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
!endif

!if $(TARGET) == RELEASE
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
!else
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  SerialPortLib|$(PLATFORM_PACKAGE)/Library/SerialPortLib/SerialPortLib.inf
!endif

  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf

 !if $(MINNOW2_FSP_BUILD) == TRUE
 PlatformFspLib|Vlv2TbltDevicePkg/Library/PlatformFspLib/PlatformFspLib.inf
 !endif
!if $(FTPM_ENABLE) == TRUE 
  Tpm2DeviceLib|Vlv2TbltDevicePkg/Library/Tpm2DeviceLibSeCPei/Tpm2DeviceLibSeC.inf
!endif

[LibraryClasses.IA32]
  #
  # DXE phase common
  #
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

  TcgPhysicalPresenceLib|SecurityPkg/Library/DxeTcgPhysicalPresenceLib/DxeTcgPhysicalPresenceLib.inf
!if $(TPM_ENABLED) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif

  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  EfiRegTableLib|$(PLATFORM_PACKAGE)/Library/EfiRegTableLib/EfiRegTableLib.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif

  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf

[LibraryClasses.IA32.DXE_DRIVER]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
!endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif

[LibraryClasses.IA32.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
!endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif

[LibraryClasses.IA32.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf

  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  !if $(TARGET) != RELEASE
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SmmDebugAgentLib.inf
  TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
!endif
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf

[LibraryClasses.IA32.SMM_CORE]
  MemoryAllocationLib|MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
  SmmServicesTableLib|MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf

!if $(TPM_ENABLED) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif

  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf

!if $(TARGET) != RELEASE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
  TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
!endif

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
!endif
!if $(TPM_ENABLED) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
!endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif

[LibraryClasses.common.UEFI_DRIVER]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif

[LibraryClasses.IA32.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif


################################################################################
#
# Library Section - list of all EDK/Framework libraries
#
################################################################################
[Libraries.common]
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseLib/BaseLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseMemoryLib/BaseMemoryLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePrintLib/BasePrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciExpressLib/BasePciExpressLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciLibCf8/BasePciLibCf8.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePeCoffLib/BasePeCoffLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/SmmRuntimeDxeReportStatusCodeLib/SmmRuntimeDxeReportStatusCodeLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiHobLib/PeiHobLib.inf
  EdkCompatibilityPkg/Foundation/Ppi/EdkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/PeiLib/PeiLib.inf
  EdkCompatibilityPkg/Compatibility/Library/UefiLanguageLib/UefiLanguageLib.inf
  EdkCompatibilityPkg/Foundation/Guid/EdkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Efi/Protocol/EfiProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiDriverLib/EfiDriverLib.inf
  EdkCompatibilityPkg/Foundation/Protocol/EdkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Protocol/EdkFrameworkProtocolLib.inf
[Libraries.IA32]
  EdkCompatibilityPkg/Foundation/Efi/Guid/EfiGuidLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Guid/EdkFrameworkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Library/EfiCommonLib/EfiCommonLib.inf
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib.inf
  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Ppi/EdkFrameworkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/Hob/PeiHobLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesLib/PeiServicesLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  EdkCompatibilityPkg/Foundation/Core/Dxe/ArchProtocol/ArchProtocolLib.inf

  $(PLATFORM_PACKAGE)/Library/MultiPlatformLib/MultiPlatformLib.inf
[Libraries.IA32]

  EdkCompatibilityPkg/Foundation/Efi/Guid/EfiGuidLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Guid/EdkFrameworkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Library/EfiCommonLib/EfiCommonLib.inf
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib.inf
  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Ppi/EdkFrameworkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Core/Dxe/ArchProtocol/ArchProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Hob/HobLib.inf
  EdkCompatibilityPkg/Foundation/Library/RuntimeDxe/EfiRuntimeLib/EfiRuntimeLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiIfrSupportLib/EfiIfrSupportLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Print/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiScriptLib/EfiScriptLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/PrintLite/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/GraphicsLite/Graphics.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiLib/UefiLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeHobLib/DxeHobLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/HiiLib/HiiLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiDriverModelLib/UefiDriverModelLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/EdkDxeRuntimeDriverLib/EdkDxeRuntimeDriverLib.inf


################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag.common]
!if $(MINI_BIOS_ENABLE) == FALSE
  gPlatformModuleTokenSpaceGuid.PcdBdsDispatchAdditionalOprom|TRUE
!else
  gPlatformModuleTokenSpaceGuid.PcdBdsDispatchAdditionalOprom|FALSE
!endif
#
# If PcdDxeIplSwitchToLongMode is TRUE, DxeIpl will load a 64-bit DxeCore and switch to long mode to hand over to DxeCore.
#
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE

  gEfiMdeModulePkgTokenSpaceGuid.PcdBrowserGrayOutTextStatement|TRUE

!if $(CAPSULE_RESET_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|TRUE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdFrameworkCompatibilitySupport|TRUE
  gEfiCpuTokenSpaceGuid.PcdCpuSmmEnableBspElection|FALSE
!if $(DATAHUB_STATUS_CODE_ENABLE) == TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseDataHub|TRUE
!else
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseDataHub|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreImageLoaderSearchTeSectionFirst|FALSE
!if $(TARGET) == RELEASE
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseMemory|FALSE
!if $(ISA_SERIAL_STATUS_CODE_ENABLE) == TRUE
  gEfiSerialPortTokenSpaceGuid.PcdStatusCodeUseIsaSerial|TRUE
!else
  gEfiSerialPortTokenSpaceGuid.PcdStatusCodeUseIsaSerial|FALSE
!endif
!if $(USB_SERIAL_STATUS_CODE_ENABLE) == TRUE
  gEfiSerialPortTokenSpaceGuid.PcdStatusCodeUseUsbSerial|TRUE
!else
  gEfiSerialPortTokenSpaceGuid.PcdStatusCodeUseUsbSerial|FALSE
!endif
!if $(RAM_SERIAL_STATUS_CODE_ENABLE) == TRUE
  gEfiSerialPortTokenSpaceGuid.PcdStatusCodeUseRam|TRUE
!else
  gEfiSerialPortTokenSpaceGuid.PcdStatusCodeUseRam|FALSE
!endif


  ## This PCD specifies whether PS2 keyboard does a extended verification during start.
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPs2KbdExtendedVerification|FALSE

  ## This PCD specifies whether PS2 mouse does a extended verification during start.
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPs2MouseExtendedVerification|FALSE

!if $(VARIABLE_INFO_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableCollectStatistics|TRUE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableCollectStatistics|FALSE
!endif

  gEfiCpuTokenSpaceGuid.PcdCpuSmmBlockStartupThisAp|TRUE

[PcdsFixedAtBuild.common]
!if $(MINNOW2_FSP_BUILD) == TRUE
# $(FLASH_REGION_VLVMICROCODE_BASE)
  gFspWrapperTokenSpaceGuid.PcdCpuMicrocodePatchAddress|0xFFD00000
# $(FLASH_REGION_VLVMICROCODE_SIZE)
  gFspWrapperTokenSpaceGuid.PcdCpuMicrocodePatchRegionSize|0x00030000
  gFspWrapperTokenSpaceGuid.PcdFlashMicroCodeOffset|0x60
# $(FLASH_AREA_BASE_ADDRESS)
  gFspWrapperTokenSpaceGuid.PcdFlashCodeCacheAddress|0xFF800000
# $(FLASH_AREA_SIZE)
  gFspWrapperTokenSpaceGuid.PcdFlashCodeCacheSize|0x00800000
# $(FLASH_REGION_FSPBIN_BASE)
  gFspWrapperTokenSpaceGuid.PcdFlashFvFspBase|0xFFDB0000
!endif

!if $(PERFORMANCE_ENABLE) == TRUE
!if $(MINNOW2_FSP_BUILD) == TRUE
  # in FSP, when this got used, the memory already is up
  gEfiCpuTokenSpaceGuid.PcdTemporaryRamBase|0x00080000
!else
  gEfiCpuTokenSpaceGuid.PcdTemporaryRamBase|0xFEF80000
!endif
  gEfiCpuTokenSpaceGuid.PcdTemporaryRamSize|0x00010000

!else
  !if $(MINNOW2_FSP_BUILD) == TRUE
    gEfiCpuTokenSpaceGuid.PcdTemporaryRamBase|0x00080000
  !else
    gEfiCpuTokenSpaceGuid.PcdTemporaryRamBase|0xFEF80000
  !endif
  gEfiCpuTokenSpaceGuid.PcdTemporaryRamSize|0x00010000
  gEfiCpuTokenSpaceGuid.PcdPeiTemporaryRamStackSize|0x3C00
!endif


!if $(SECURE_BOOT_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x22000
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x4000
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdHwErrStorageSize|0x00000800
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize|0x400
  gEfiCpuTokenSpaceGuid.PcdCpuIEDRamSize|0x400000
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdS3AcpiReservedMemorySize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeimPerFv|50
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdAriSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPpiSupported|128
  gEfiCpuTokenSpaceGuid.PcdCpuSmmApSyncTimeout|1000
!if $(S4_ENABLE) == TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|TRUE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
!endif
!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x3
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2F
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
!endif
!if $(PERFORMANCE_ENABLE) == TRUE
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x1
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|60
!endif

  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdEbdaReservedMemorySize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdLoadModuleAtFixAddressEnable|$(TOP_MEMORY_ADDRESS)
  gEfiMdeModulePkgTokenSpaceGuid.PcdBrowserSubtitleTextColor|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdBrowserFieldTextColor|0x01
  gEfiCpuTokenSpaceGuid.PcdCpuIEDEnabled|TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBiosVideoCheckVbeEnable|TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBiosVideoCheckVgaEnable|TRUE

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|FALSE
!endif

[PcdsFixedAtBuild.IA32.PEIM, PcdsFixedAtBuild.IA32.PEI_CORE, PcdsFixedAtBuild.IA32.SEC]
!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x3
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2E
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
!endif

[PcdsPatchableInModule.common]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x803805c6
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x$(PLATFORM_PCIEXPRESS_BASE)
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdLegacyBiosCacheLegacyRegion|FALSE

  ## This PCD specifies whether to use the optimized timing for best PS2 detection performance.
  #  Note this PCD could be set to TRUE for best boot performance and set to FALSE for best device compatibility.
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdFastPS2Detection|TRUE

  #######################################################################################################
  #
  # Begin of MRC parameters
  #
  
  ## Memory Parameter Patchable.
  #  FALSE - MRC Parameters are fixed for MinnowBoard Max<BR>
  #  TRUE  - MRC Parameters are patchable by following PCDs<BR>
  # @Prompt Memory Parameter Patchable.
  # @ValidList 0x80000001 | 0, 1  
  gVlvRefCodePkgTokenSpaceGuid.PcdMemoryParameterPatchable|FALSE
  
  ## Memory Down or DIMM slot.
  #  0 - DIMM<BR>
  #  1 - Memory Down<BR>
  # @Prompt Enable Memory Down
  # @ValidList 0x80000001 | 0, 1
  gVlvRefCodePkgTokenSpaceGuid.PcdEnableMemoryDown|1
     
  ## The speed of DRAM.
  #  0 - 800 MHz<BR>
  #  1 - 1066 MHz<BR>
  #  2 - 1333 MHz<BR>
  #  3 - 1600 MHz<BR>
  # @Prompt DRAM Speed
  # @ValidList 0x80000001 | 0, 1, 2, 3
  gVlvRefCodePkgTokenSpaceGuid.PcdDramSpeed|1

  ## DRAM Type.
  #  0 - DDR3<BR>
  #  1 - DDR3L<BR>
  #  2 - DDR3U<BR>
  #  3 - DDR3All<BR>
  #  4 - LPDDR2<BR>
  #  5 - LPDDR3<BR>
  #  6 - DDR4<BR>
  # @Prompt DRAM Type
  # @ValidList 0x80000001 | 0, 1, 2, 3, 4, 5, 6
  gVlvRefCodePkgTokenSpaceGuid.PcdDramType|1
    
  ## Please populate DIMM slot 0 if only one DIMM is supported.
  #  0 - Disable<BR>
  #  1 - Enable<BR>
  # @Prompt DIMM 0 Enable 
  # @ValidList 0x80000001 | 0, 1
  gVlvRefCodePkgTokenSpaceGuid.PcdEnableDimm0|1

  ## DIMM 1 has to be identical to DIMM 0.
  #  0 - Disable<BR>
  #  1 - Enable<BR>
  # @Prompt DIMM 1 Enable Type
  # @ValidList 0x80000001 | 0, 1
  gVlvRefCodePkgTokenSpaceGuid.PcdEnableDimm1|0
  
  ## DRAM device data width.
  #  0 - x8<BR>
  #  1 - x16<BR>
  #  2 - x32<BR>
  # @Prompt DIMM_DWIDTH
  # @ValidList 0x80000001 | 0, 1, 2
  gVlvRefCodePkgTokenSpaceGuid.PcdDimmDataWidth|1

  ## DRAM device data density.
  #  0 - 1 Gbit<BR>
  #  1 - 2 Gbit<BR>
  #  2 - 4 Gbit<BR>
  #  3 - 8 Gbit<BR>
  # @Prompt DIMM_Density
  # @ValidList 0x80000001 | 0, 1, 2, 3
  gVlvRefCodePkgTokenSpaceGuid.PcdDimmDensity|2
  
  ## DRAM device data bus width.
  #  0 - 8 bits<BR>
  #  1 - 16 bits<BR>
  #  2 - 32 bits<BR>
  #  3 - 64 bits<BR>
  # @Prompt DIMM_BusWidth
  # @ValidList 0x80000001 | 0, 1, 2, 3
  gVlvRefCodePkgTokenSpaceGuid.PcdDimmBusWidth|3

  ## Ranks Per DIMM or Sides Per DIMM.
  #  0 - 1 Rank<BR>
  #  1 - 2 Ranks<BR>
  # @Prompt DIMM_Sides
  # @ValidList 0x80000001 | 0, 1
  gVlvRefCodePkgTokenSpaceGuid.PcdRankPerDimm|0

  ## tCL.<BR><BR>
  # @Prompt tCL
  gVlvRefCodePkgTokenSpaceGuid.PcdTcl|11

  ## tRP and tRCD in DRAM clk - 5:12.5ns, 6:15ns, etc.
  # @Prompt tRP_tRCD 
  gVlvRefCodePkgTokenSpaceGuid.PcdTrpTrcd|11

  ## tWR in DRAM clk. 
  # @Prompt tWR 
  gVlvRefCodePkgTokenSpaceGuid.PcdTwr|12
  
  ## tWTR in DRAM clk.  
  # @Prompt tWTR 
  gVlvRefCodePkgTokenSpaceGuid.PcdTwtr|6
  
  ## tRRD in DRAM clk. 
  # @Prompt tRRD 
  gVlvRefCodePkgTokenSpaceGuid.PcdTrrd|6
   
  ## tRTP in DRAM clk.  
  # @Prompt tRTP 
  gVlvRefCodePkgTokenSpaceGuid.PcdTrtp|6

  ## tFAW in DRAM clk.
  # @Prompt tFAW 
  gVlvRefCodePkgTokenSpaceGuid.PcdTfaw|32
  
  #
  # End of MRC parameters.
  # 
  ###############################################################################################

[PcdsDynamicHii.common.DEFAULT]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5 # Variable: L"Timeout"
  gEfiMdePkgTokenSpaceGuid.PcdHardwareErrorRecordLevel|L"HwErrRecSupport"|gEfiGlobalVariableGuid|0x0|1 # Variable: L"HwErrRecSupport"
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBootState|L"BootState"|gEfiBootStateGuid|0x0|TRUE

[PcdsDynamicDefault.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateDataPtr|0x0
  !if $(TPM_ENABLED) == TRUE
    gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{0x7b, 0x3a, 0xcd, 0x72, 0xA5, 0xFE, 0x5e, 0x4f, 0x91, 0x65, 0x4d, 0xd1, 0x21, 0x87, 0xbb, 0x13}
  !endif
  !if $(FTPM_ENABLE) == TRUE
    gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{0x7b, 0x3a, 0xcd, 0x72, 0xA5, 0xFE, 0x5e, 0x4f, 0x91, 0x65, 0x4d, 0xd1, 0x21, 0x87, 0xbb, 0x13}
  !endif

  ## This PCD defines the video horizontal resolution.
  #  This PCD could be set to 0 then video resolution could be at highest resolution.
  #gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|800
  ## This PCD defines the video vertical resolution.
  #  This PCD could be set to 0 then video resolution could be at highest resolution.
  #gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|600

  ## This PCD defines the Console output column and the default value is 25 according to UEFI spec.
  #  This PCD could be set to 0 then console output could be at max column and max row.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|31
  ## This PCD defines the Console output row and the default value is 80 according to UEFI spec.
  #  This PCD could be set to 0 then console output could be at max column and max row.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|100

  ## The PCD is used to specify the video horizontal resolution of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|800
  ## The PCD is used to specify the video vertical resolution of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|600
  ## The PCD is used to specify the console output column of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutColumn|100
  ## The PCD is used to specify the console output column of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutRow|31

!if $(TPM_ENABLED) == TRUE
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInitializationPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmScrtmPolicy|1
!endif

[PcdsDynamicExDefault.common.DEFAULT]
  gEfiVLVTokenSpaceGuid.PcdTCSmbaIoBaseAddress|0x1040
  gEfiVLVTokenSpaceGuid.PcdEmmcManufacturerId|0
  gEfiVLVTokenSpaceGuid.PcdProductSerialNumber|0
  gEfiVLVTokenSpaceGuid.PcdMeasuredBootEnable|TRUE
  gEfiVLVTokenSpaceGuid.PcdFTPMErrorOccur|FALSE
  gEfiVLVTokenSpaceGuid.PcdFTPMErrorSkip|FALSE
  gEfiVLVTokenSpaceGuid.PcdFTPMCommand|0
  gEfiVLVTokenSpaceGuid.PcdFTPMResponse|0
  gEfiVLVTokenSpaceGuid.PcdFTPMNotRespond|FALSE
  gEfiVLVTokenSpaceGuid.PcdFTPMStatus|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateSmmDataPtr|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateDataPtr|0
  gEfiCpuTokenSpaceGuid.PcdCpuS3DataAddress|0
  gEfiCpuTokenSpaceGuid.PcdCpuHotPlugDataAddress|0
  gEfiCpuTokenSpaceGuid.PcdCpuCallbackSignal|0
  gEfiCpuTokenSpaceGuid.PcdCpuConfigContextBuffer|0
  gEfiVLVTokenSpaceGuid.PcdCpuLockBoxDataAddress|0
  gEfiVLVTokenSpaceGuid.PcdCpuSmramCpuDataAddress|0
  gEfiVLVTokenSpaceGuid.PcdCpuLockBoxSize|0

[Components.IA32]

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/SecCore.inf

  !if $(MINNOW2_FSP_BUILD) == TRUE
  IntelFspWrapperPkg/FspWrapperSecCore/FspWrapperSecCore.inf {
    !if $(TARGET) == DEBUG

    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    !endif
  }
  Vlv2TbltDevicePkg/FspSupport/BootModePei/BootModePei.inf
  IntelFspWrapperPkg/FspInitPei/FspInitPei.inf {
    !if $(TARGET) == DEBUG
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    !endif
  }
  !endif

  MdeModulePkg/Core/Pei/PeiMain.inf {
!if $(TARGET) == DEBUG
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2E
!endif
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
  }

  $(PLATFORM_PACKAGE)/MonoStatusCode/MonoStatusCode.inf {
!if $(TARGET) == DEBUG
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2E
!endif
  }
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/MemoryInit.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
    <BuildOptions>
      !if $(FTPM_ENABLE)==TRUE
        *_*_IA32_CC_FLAGS = /D FTPM_ENABLE
      !endif
  }

!if $(RC_BINARY_RELEASE) == TRUE
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/SeCUma.inf
!endif

!if $(FTPM_ENABLE) == TRUE
$(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/fTPMInitPeim.inf
!endif

!if $(RC_BINARY_RELEASE) == TRUE
  $(PLATFORM_PACKAGE)/PlatformPei/PlatformPei.inf {
    <BuildOptions>
      *_*_IA32_CC_FLAGS      = /DRC_BINARY_RELEASE
  !if $(TARGET) == DEBUG
      <PcdsFixedAtBuild>
        gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2E
  !endif
  }
!endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  SourceLevelDebugPkg/DebugAgentPei/DebugAgentPei.inf{
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
      PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
      SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
    }
!endif

!if $(FTPM_ENABLE) == TRUE
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/Tpm2DeviceSeCPei.inf
!endif

!if $(TPM_ENABLED) == TRUE
  SecurityPkg/Tcg/PhysicalPresencePei/PhysicalPresencePei.inf
  SecurityPkg/Tcg/TcgPei/TcgPei.inf {
    <LibraryClasses>
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
}
!endif

 $(PLATFORM_PACKAGE)/PlatformInitPei/PlatformInitPei.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x803805c6
    <LibraryClasses>
!if $(TARGET) != RELEASE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  }
  $(PLATFORM_PACKAGE)/FvInfoPei/FvInfoPei.inf

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/VlvInitPeim.inf
!if $(PCIESC_ENABLE) == TRUE
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/PchEarlyInitPeim.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
  }
!endif
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/PchInitPeim.inf


  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/PchSmbusArpDisabled.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/PchSpiPeim.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/PeiSmmAccess.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/PeiSmmControl.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/CpuPeim.inf
  UefiCpuPkg/CpuIoPei/CpuIoPei.inf
  UefiCpuPkg/Universal/Acpi/S3Resume2Pei/S3Resume2Pei.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/MpS3.inf
  EdkCompatibilityPkg/Compatibility/AcpiVariableHobOnSmramReserveHobThunk/AcpiVariableHobOnSmramReserveHobThunk.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/IA32/PiSmmCommunicationPei.inf
!if $(CAPSULE_ENABLE) == TRUE
  MdeModulePkg/Universal/CapsulePei/CapsulePei.inf
!endif
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
!if $(LZMA_ENABLE) == TRUE
    NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
!endif
  }

 MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
 MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf

!if $(FTPM_ENABLE) == TRUE
   SecurityPkg/Tcg/TrEEPei/TrEEPei.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      NULL|SecurityPkg/Library\HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  }
!endif
!if $(TPM_ENABLED) == TRUE
  SecurityPkg/Tcg/TrEEConfig/TrEEConfigPei.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  }
!endif
!if $(ACPI50_ENABLE) == TRUE
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTablePei/FirmwarePerformancePei.inf{
    <LibraryClasses>
      TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
  }

!endif
!if $(PERFORMANCE_ENABLE) == TRUE
  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
!endif
[Components.IA32]
  !if $(MINNOW2_FSP_BUILD) == TRUE
  IntelFspWrapperPkg/FspNotifyDxe/FspNotifyDxe.inf {
    !if $(TARGET) == DEBUG
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    !endif
  }

  !endif
  #
  # EDK II Related Platform codes
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
    <LibraryClasses>
!if $(DXE_CRC32_SECTION_ENABLE) == TRUE
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
!endif
!if $(LZMA_ENABLE) == TRUE
      NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
!endif
!if $(TARGET) != RELEASE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }
  IntelFrameworkModulePkg/Universal/Acpi/AcpiS3SaveDxe/AcpiS3SaveDxe.inf {
    <PcdsPatchableInModule>
        gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xF0000043
    <PcdsFixedAtBuild>
        gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
    <LibraryClasses>
    !if $(TARGET) != RELEASE
          DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    !endif
       <BuildOptions>
        ICC:*_*_*_CC_FLAGS = /D MDEPKG_NDEBUG
        GCC:*_*_*_CC_FLAGS = -D MDEPKG_NDEBUG
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  IntelFrameworkModulePkg/Universal/CpuIoDxe/CpuIoDxe.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf

  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf  {
    <LibraryClasses>
!if $(TARGET) != RELEASE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
  }

!if $(CAPSULE_ENABLE) == TRUE
  MdeModulePkg/Universal/CapsulePei/CapsuleX64.inf {
    <LibraryClasses>
    PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
    MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
    HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  }
!endif

  MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterSmm.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf{
    <LibraryClasses>
!if $(SECURE_BOOT_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
!endif
!if $(USER_IDENTIFICATION_ENABLE)
      NULL|SecurityPkg/Library/DxeDeferImageLoadLib/DxeDeferImageLoadLib.inf
!endif
!if $(TPM_ENABLED) == TRUE
      NULL|SecurityPkg/Library/DxeTpmMeasureBootLib/DxeTpmMeasureBootLib.inf
!endif
!if $(FTPM_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
!endif
  }
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/MpCpu.inf
  $(PLATFORM_PACKAGE)/Metronome/Metronome.inf

  IntelFrameworkModulePkg/Universal/BdsDxe/BdsDxe.inf{
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      PlatformBdsLib|$(PLATFORM_PACKAGE)/Library/PlatformBdsLib/PlatformBdsLib.inf
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      SerialPortLib|$(PLATFORM_PACKAGE)/Library/SerialPortLib/SerialPortLib.inf
    !if $(FTPM_ENABLE) == TRUE  
      Tpm2DeviceLib|Vlv2TbltDevicePkg/Library/Tpm2DeviceLibSeCDxe/Tpm2DeviceLibSeC.inf
    !else
      TrEEPhysicalPresenceLib|$(PLATFORM_PACKAGE)/Library/DxeTrEEPhysicalPresenceLibNull/DxeTrEEPhysicalPresenceLibNull.inf
    !endif  
  }

  $(PLATFORM_PACKAGE)/UiApp/UiApp.inf

  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
    <LibraryClasses>
      SerialPortLib|$(PLATFORM_PACKAGE)/Library/SerialPortLib/SerialPortLib.inf
  }
  $(PLATFORM_PACKAGE)/FvbRuntimeDxe/FvbSmm.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchSpiSmm.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf {
    <LibraryClasses>
      PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
    <BuildOptions>
      #
      # Specify GUID gEfiIfrBootMaintenanceGuid, to install Secure Boot Configuration menu
      # into Boot Maintenance Manager menu
      #
      *_*_*_VFR_FLAGS   = -g b2dedc91-d59f-48d2-898a-12490c74a4e0
  }
!endif
   MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf {
    <LibraryClasses>
      FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  }

  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf

  $(PLATFORM_PACKAGE)/FvbRuntimeDxe/FvbRuntimeDxe.inf

  $(PLATFORM_PACKAGE)/PlatformSetupDxe/PlatformSetupDxe.inf

!if $(DATAHUB_ENABLE) == TRUE
  IntelFrameworkModulePkg/Universal/DataHubDxe/DataHubDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|0
  }
!endif
  IntelFrameworkModulePkg/Universal/StatusCode/DatahubStatusCodeHandlerDxe/DatahubStatusCodeHandlerDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchS3SupportDxe.inf
  !if $(USE_HPET_TIMER) == TRUE
    PcAtChipsetPkg/HpetTimerDxe/HpetTimerDxe.inf
  !else
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/SmartTimer.inf
  !endif
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/SmmControl.inf

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchSmbusDxe.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/IntelPchLegacyInterrupt.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchReset.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchInitDxe.inf{
    <PcdsPatchableInModule>
        gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xF0000043
  }
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchSmiDispatcher.inf

!if $(PCIESC_ENABLE) == TRUE
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchPcieSmm.inf
!endif
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchSpiRuntime.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchPolicyInitDxe.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PchBiosWriteProtect.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/SmmAccess.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PciHostBridge.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/VlvInitDxe.inf

  IntelFrameworkModulePkg/Universal/LegacyRegionDxe/LegacyRegionDxe.inf
  
  PerformancePkg/Dp_App/Dp.inf {
  <LibraryClasses>
  !if $(PERFORMANCE_ENABLE) == TRUE
    PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
    TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
  !endif
  }

  Vlv2TbltDevicePkg/VlvPlatformInitDxe/VlvPlatformInitDxe.inf{
    <LibraryClasses>
!if $(TARGET) != RELEASE
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/Dptf.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PnpDxe.inf

!if $(SEC_ENABLE) == TRUE
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/HeciDrv.inf {
!if $(SEC_DEBUG_INFO_ENABLE) == TRUE
    <BuildOptions>
      *_*_X64_CC_FLAGS      = /DSEC_DEBUG_INFO=1
!else
    <BuildOptions>
      *_*_X64_CC_FLAGS      = /DSEC_DEBUG_INFO=0
!endif
  }
  
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/SeCPolicyInitDxe.inf
!endif

!if $(FTPM_ENABLE) == TRUE
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/Tpm2DeviceSeCDxe.inf
  SecurityPkg/Tcg/MemoryOverwriteControl/TcgMor.inf
  SecurityPkg/Tcg/TrEEDxe/TrEEDxe.inf{
    <LibraryClasses>
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      PcdLib|MdePkg/Library\DxePcdLib/DxePcdLib.inf
      Tpm2DeviceLib|Vlv2TbltDevicePkg/Library/Tpm2DeviceLibSeCDxe/Tpm2DeviceLibSeC.inf
  }
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/FtpmSmm.inf
!endif
!if $(TPM_ENABLED) == TRUE
  SecurityPkg/Tcg/TrEEConfig/TrEEConfigPei.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  }

  SecurityPkg/Tcg/TcgConfigDxe/TcgConfigDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
    <BuildOptions>
      #
      # specify GUID gEfiIfrNotInTPVPageGuid, this page will not
      # be showed in TPV page.
      #
      *_*_*_VFR_FLAGS   = -g e58809f8-fbc1-48e2-883a-a30fdc4b441e
  }

  SecurityPkg/Tcg/TcgDxe/TcgDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  SecurityPkg/Tcg/TcgSmm/TcgSmm.inf
!endif
  #
  # EDK II Related Platform codes
  #
  $(PLATFORM_PACKAGE)/PlatformSmm/PlatformSmm.inf{
    <LibraryClasses>
    !if $(TARGET) != RELEASE
          DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    !endif
          PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  $(PLATFORM_PACKAGE)/PlatformInfoDxe/PlatformInfoDxe.inf
  $(PLATFORM_PACKAGE)/PlatformCpuInfoDxe/PlatformCpuInfoDxe.inf
  $(PLATFORM_PACKAGE)/PlatformDxe/PlatformDxe.inf

  $(PLATFORM_PACKAGE)/PciPlatform/PciPlatform.inf
  $(PLATFORM_PACKAGE)/SaveMemoryConfig/SaveMemoryConfig.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PlatformCpuPolicy.inf
  $(PLATFORM_PACKAGE)/PpmPolicy/PpmPolicy.inf
  $(PLATFORM_PACKAGE)/SmramSaveInfoHandlerSmm/SmramSaveInfoHandlerSmm.inf
!if $(GOP_DRIVER_ENABLE) == TRUE
  $(PLATFORM_PACKAGE)/PlatformGopPolicy/PlatformGopPolicy.inf

!endif


  #
  # SMM
  #
  MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PiSmmCpuDxeSmm.inf
  UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
  MdeModulePkg/Universal/LockBox/SmmLockBox/SmmLockBox.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PiSmmCommunicationSmm.inf
  $(PLATFORM_PACKAGE)/SmmSwDispatch2OnSmmSwDispatchThunk/SmmSwDispatch2OnSmmSwDispatchThunk.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PowerManagement2.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/DigitalThermalSensor.inf

  #
  # ACPI
  #
   MdeModulePkg/Universal/Acpi/BootScriptExecutorDxe/BootScriptExecutorDxe.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xF0000043
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }

  $(PLATFORM_PACKAGE)/BootScriptSaveDxe/BootScriptSaveDxe.inf
  IntelFrameworkModulePkg/Universal/Acpi/AcpiSupportDxe/AcpiSupportDxe.inf
  Vlv2DeviceRefCodePkg/ValleyView2Soc/CPU/PowerManagement/AcpiTables/PowerManagementAcpiTables.inf

  $(PLATFORM_RC_PACKAGE)/AcpiTablesPCAT/AcpiTables.inf

  $(PLATFORM_PACKAGE)/AcpiPlatform/AcpiPlatform.inf

  #
  # PCI
  #
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf


  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/ISPDxe.inf


#
# ISA
#
  $(PLATFORM_PACKAGE)/Wpce791/Wpce791.inf
  IntelFrameworkModulePkg/Bus/Isa/IsaBusDxe/IsaBusDxe.inf
  IntelFrameworkModulePkg/Bus/Isa/IsaIoDxe/IsaIoDxe.inf
  IntelFrameworkModulePkg/Bus/Isa/IsaSerialDxe/IsaSerialDxe.inf
  IntelFrameworkModulePkg/Bus/Isa/Ps2MouseDxe/Ps2MouseDxe.inf
  IntelFrameworkModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2keyboardDxe.inf
#
# SDIO
#
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/MmcHost.inf
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/MmcMediaDevice.inf
!if $(ACPI50_ENABLE) == TRUE
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.inf {
    <LibraryClasses>
      TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
  }
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceSmm.inf {
    <LibraryClasses>
      TimerLib|$(PLATFORM_PACKAGE)/Library/IntelPchAcpiTimerLib/IntelPchAcpiTimerLib.inf
  }
!endif

#
# IDE/SCSI/AHCI
#
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  IntelFrameworkModulePkg/Bus/Pci/IdeBusDxe/IdeBusDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
!if $(SATA_ENABLE) == TRUE
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/SataController.inf
!endif
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
!if $(SCSI_ENABLE) == TRUE
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
!endif
#
# Console
#
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  IntelFrameworkModulePkg/Universal/Console/VgaClassDxe/VgaClassDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf

  #
  # USB
  #
!if $(USB_ENABLE) == TRUE
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

!endif

  #
  #  ECP
  #
  EdkCompatibilityPkg/Compatibility/FrameworkHiiOnUefiHiiThunk/FrameworkHiiOnUefiHiiThunk.inf
  EdkCompatibilityPkg/Compatibility/LegacyRegion2OnLegacyRegionThunk/LegacyRegion2OnLegacyRegionThunk.inf
  EdkCompatibilityPkg/Compatibility/SmmBaseOnSmmBase2Thunk/SmmBaseOnSmmBase2Thunk.inf
  EdkCompatibilityPkg/Compatibility/SmmBaseHelper/SmmBaseHelper.inf
  EdkCompatibilityPkg/Compatibility/SmmAccess2OnSmmAccessThunk/SmmAccess2OnSmmAccessThunk.inf
  EdkCompatibilityPkg/Compatibility/SmmControl2OnSmmControlThunk/SmmControl2OnSmmControlThunk.inf
  EdkCompatibilityPkg/Compatibility/FrameworkSmmStatusCodeOnPiSmmStatusCodeThunk/FrameworkSmmStatusCodeOnPiSmmStatusCodeThunk.inf
  EdkCompatibilityPkg/Compatibility/FvOnFv2Thunk/FvOnFv2Thunk.inf
  #
  # SMBIOS
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  $(PLATFORM_PACKAGE)/SmBiosMiscDxe/SmBiosMiscDxe.inf

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/SmbiosMemory.inf
  #
  # CPU/FW Microde
  #
  Vlv2MiscBinariesPkg/Microcode/MicrocodeUpdates.inf {
    <BuildOptions>
      *_*_*_GENFW_FLAGS = -a 0x800 -p 0xFF
  }


  PcAtChipsetPkg/8259InterruptControllerDxe/8259.inf


!if $(NETWORK_ENABLE) == TRUE
  !if $(NETWORK_ISCSI_ENABLE) == TRUE
    !if $(NETWORK_IP6_ENABLE) == TRUE
      NetworkPkg/IScsiDxe/IScsiDxe.inf
    !else
      MdeModulePkg/Universal/Network/IScsiDxe/IScsiDxe.inf
    !endif
  !endif
  !if $(NETWORK_VLAN_ENABLE) == TRUE
    MdeModulePkg/Universal/Network/VlanConfigDxe/VlanConfigDxe.inf
  !endif
  !if $(CSM_ENABLE) == TRUE
    IntelFrameworkModulePkg/Csm/BiosThunk/Snp16Dxe/Snp16Dxe.inf
  !endif
!endif

!if $(NETWORK_ENABLE) == TRUE
  #
  # UEFI network modules
  #
    MdeModulePkg/Universal/Network/DpcDxe/DpcDxe.inf
    MdeModulePkg/Universal/Network/SnpDxe/SnpDxe.inf

    MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf
    MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
    MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
    MdeModulePkg/Universal/Network/Ip4ConfigDxe/Ip4ConfigDxe.inf
    MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
    MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
    MdeModulePkg/Universal/Network/Tcp4Dxe/Tcp4Dxe.inf {
      <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
    }
    MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf
    !if $(NETWORK_IP6_ENABLE) == TRUE
      NetworkPkg/Ip6Dxe/Ip6Dxe.inf
      NetworkPkg/Dhcp6Dxe/Dhcp6Dxe.inf
      NetworkPkg/IpSecDxe/IpSecDxe.inf
      NetworkPkg/TcpDxe/TcpDxe.inf
      NetworkPkg/Udp6Dxe/Udp6Dxe.inf
      NetworkPkg/Mtftp6Dxe/Mtftp6Dxe.inf
    !endif
    !if $(NETWORK_IP6_ENABLE) == TRUE
      NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf
    !else
      MdeModulePkg/Universal/Network/UefiPxeBcDxe/UefiPxeBcDxe.inf
    !endif
!endif

  Vlv2TbltDevicePkg/Application/FirmwareUpdate/FirmwareUpdate.inf

[BuildOptions]
#
# Define Build Options both for EDK and EDKII drivers.
#

#
# Define token for different Platform
#
!if $(MINNOW2_FSP_BUILD) == TRUE
  DEFINE MINNOW2_FSP_OPTION = /DMINNOW2_FSP_BUILD
!else
  DEFINE MINNOW2_FSP_OPTION =
!endif

!if $(ENBDT_PF_BUILD) == TRUE
  DEFINE ENBDT_PF_ENABLE = /DENBDT_PF_ENABLE=1
!else
  DEFINE ENBDT_PF_ENABLE = /DENBDT_PF_ENABLE=0
!endif


!if $(CLKGEN_CONFIG_EXTRA_ENABLE) == TRUE
  DEFINE CLKGEN_CONFIG_EXTRA_BUILD_OPTION = /DCLKGEN_CONFIG_EXTRA=1
!else
  DEFINE CLKGEN_CONFIG_EXTRA_BUILD_OPTION =
!endif



!if $(PCIESC_ENABLE) == TRUE
  DEFINE PCIESC_SUPPORT_BUILD_OPTION = /DPCIESC_SUPPORT=1
!else
  DEFINE PCIESC_SUPPORT_BUILD_OPTION =
!endif
!if $(SATA_ENABLE) == TRUE
  DEFINE SATA_SUPPORT_BUILD_OPTION = /DSATA_SUPPORT=1
!else
  DEFINE SATA_SUPPORT_BUILD_OPTION =
!endif
!if $(ENBDT_S3_SUPPORT) == TRUE
  DEFINE ENBDT_S3_SUPPORT_OPTIONS = /DNOCS_S3_SUPPORT
!else
  DEFINE ENBDT_S3_SUPPORT_OPTIONS =
!endif

!if $(X64_CONFIG) == TRUE
  DEFINE X64_BUILD_ENABLE = /DX64_BUILD_ENABLE=1
!else
  DEFINE X64_BUILD_ENABLE =
!endif

!if $(FTPM_ENABLE) == TRUE
  DEFINE DSC_FTPM_BUILD_OPTIONS = /DFTPM_ENABLE
!else
  DEFINE DSC_FTPM_BUILD_OPTIONS = 
!endif
!if $(TPM_ENABLED) == TRUE
  DEFINE DSC_TPM_BUILD_OPTIONS = /DTPM_ENABLED
!else
  DEFINE DSC_TPM_BUILD_OPTIONS =
!endif


  DEFINE EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS = $(MINNOW2_FSP_OPTION) $(MINNOW2_BUILD_OPTION) $(ENBDT_PF_ENABLE) $(EXTERNAL_VGA_BUILD_OPTION) $(PCIE_ENUM_WA_BUILD_OPTION) $(X0_WA_ENABLE_BUILD_OPTION) $(A0_WA_ENABLE_BUILD_OPTION) $(MICROCODE_FREE_BUILD_OPTIONS) $(SIMICS_BUILD_OPTIONS) $(HYBRID_BUILD_OPTIONS) $(COMPACT_BUILD_OPTIONS) $(VP_BUILD_OPTIONS) $(SYSCTL_ID_BUILD_OPTION) $(CLKGEN_CONFIG_EXTRA_BUILD_OPTION) $(SYSCTL_X0_CONVERT_BOARD_OPTION) $(ENBDT_S3_SUPPORT_OPTIONS) $(SATA_SUPPORT_BUILD_OPTION) $(PCIESC_SUPPORT_BUILD_OPTION) $(DSC_FTPM_BUILD_OPTIONS) $(DSC_FTPM_ERROR_WR_BUILD_OPTIONS) $(DSC_TPM_BUILD_OPTIONS) $(DSC_BYTI_SECURE_BOOT_BUILD_OPTIONS)
!if $(PERFORMANCE_ENABLE) == TRUE
  DEFINE PDB_BUILD_OPTION = /Zi
!endif

!if $(SOURCE_DEBUG_ENABLE) == TRUE
  MSFT:*_*_X64_GENFW_FLAGS  = --keepexceptiontable
  GCC:*_*_X64_GENFW_FLAGS   = --keepexceptiontable
  INTEL:*_*_X64_GENFW_FLAGS = --keepexceptiontable
!if $(TARGET) == DEBUG
  DEFINE SOURCE_LEVEL_DEBUG_BUILD_OPTIONS = /Od /Oy-
!endif
!else
  DEFINE SOURCE_LEVEL_DEBUG_BUILD_OPTIONS =

!endif

[BuildOptions.Common.EDK]

#
# Define token for different Platform
#
!if $(ENBDT_PF_BUILD) == TRUE
  DEFINE ENBDT_PF_ENABLE = /DENBDT_PF_ENABLE=1
!else
  DEFINE ENBDT_PF_ENABLE = /DENBDT_PF_ENABLE=0
!endif

!if $(PERFORMANCE_ENABLE) == TRUE
  RELEASE_*_*_DLINK_FLAGS = /DEBUG
!endif

!if $(S3_ENABLE) == TRUE
  DEFINE DSC_S3_BUILD_OPTIONS = /DEFI_S3_RESUME
!else
  DEFINE DSC_S3_BUILD_OPTIONS =
!endif

!if $(ENBDT_S3_SUPPORT) == TRUE
  DEFINE ENBDT_S3_SUPPORT_OPTIONS = /DNOCS_S3_SUPPORT
!else
  DEFINE ENBDT_S3_SUPPORT_OPTIONS =
!endif

!if $(X64_CONFIG) == TRUE
  DEFINE X64_BUILD_ENABLE = /DX64_BUILD_ENABLE=1
!else
  DEFINE X64_BUILD_ENABLE =
!endif


  DEFINE EDK_GLUE_LIB_DEBUG  =
  DEFINE DEBUG_BUILD_OPTIONS = /D EFI_DEBUG /D DEBUG_MODE=1  /GL- $(EDK_GLUE_LIB_DEBUG) /DEDKII_GLUE_DebugPrintErrorLevel=(EFI_D_ERROR)
  DEFINE EDK_DSC_FEATURE_BUILD_OPTIONS = $(DSC_S3_BUILD_OPTIONS) $(DSC_ACPI_BUILD_OPTIONS) $(DSC_SEC_BUILD_OPTIONS) $(DSC_FTPM_BUILD_OPTIONS) $(DSC_FTPM_ERROR_WR_BUILD_OPTIONS) $(DSC_TPM_BUILD_OPTIONS) $(SOFTSDV_BUILD_OPTIONS) $(SIMICS_BUILD_OPTIONS) $(HYBRID_BUILD_OPTIONS) $(COMPACT_BUILD_OPTIONS) $(VP_BUILD_OPTIONS) $(QT_BUILD_OPTIONS) $(DSC_BYTI_SECURE_BOOT_BUILD_OPTIONS) /D$(PROJECT_SC_CHIPSET)

  DEFINE EDK_DSC_OTHER_BUILD_OPTIONS = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS) $(SV_BUILD_OPTIONS) $(INTEL_FASTBOOT_BUILD_OPTION)
  DEFINE EDK_DSC_GLOBAL_BUILD_OPTIONS = $(ENBDT_PF_ENABLE) $(EDK_DSC_FEATURE_BUILD_OPTIONS) $(EDK_DSC_OTHER_BUILD_OPTIONS) /D EFI_SPECIFICATION_VERSION=0x00020000  /D PI_SPECIFICATION_VERSION=0x00000009  /D TIANO_RELEASE_VERSION=0x00080006 /D SUPPORT_DEPRECATED_PCI_CFG_PPI /D CSM_SMMENTRY_PORT8DATA8 /D EDKII_GLUE_PciExpressBaseAddress=0x$(PLATFORM_PCIEXPRESS_BASE) /D MAX_VARIABLE_SIZE=0x2000 /D EFI_FIRMWARE_VENDOR="L/"INTEL/"" /D EFI_BUILD_VERSION="L/"EDKII/"" /DEFI_PEI_REPORT_STATUS_CODE_ON $(ENBDT_S3_SUPPORT_OPTIONS)

  *_*_IA32_ASM_FLAGS         = /DEFI32 /D EDKII_GLUE_PciExpressBaseAddress=$(PLATFORM_PCIEXPRESS_BASE)h /DNOCS_S3_SUPPORT
  DEBUG_*_IA32_CC_FLAGS      = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_IA32_CC_FLAGS    = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  DEBUG_*_IA32_VFRPP_FLAGS   = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_IA32_VFRPP_FLAGS = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  DEBUG_*_IA32_APP_FLAGS     = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_IA32_APP_FLAGS   = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  DEBUG_*_IA32_PP_FLAGS      = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_IA32_PP_FLAGS    = /D EFI32 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_IA32_ASLPP_FLAGS       = /D EDKII_GLUE_PciExpressBaseAddress=0x$(PLATFORM_PCIEXPRESS_BASE)
  *_*_IA32_ASLCC_FLAGS       = /D EDKII_GLUE_PciExpressBaseAddress=0x$(PLATFORM_PCIEXPRESS_BASE)
  *_*_IA32_ASM16_FLAGS       = /D EDKII_GLUE_PciExpressBaseAddress=$(PLATFORM_PCIEXPRESS_BASE)h

  *_*_X64_ASM_FLAGS          = /DEFIX64 /D EDKII_GLUE_PciExpressBaseAddress=$(PLATFORM_PCIEXPRESS_BASE)h /DNOCS_S3_SUPPORT
  DEBUG_*_X64_CC_FLAGS       = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_X64_CC_FLAGS     = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  DEBUG_*_X64_VFRPP_FLAGS    = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_X64_VFRPP_FLAGS  = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  DEBUG_*_X64_APP_FLAGS      = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_X64_APP_FLAGS    = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  DEBUG_*_X64_PP_FLAGS       = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS) $(DEBUG_BUILD_OPTIONS)
  RELEASE_*_X64_PP_FLAGS     = /D EFIX64 $(EDK_DSC_GLOBAL_BUILD_OPTIONS)
  *_*_X64_ASLPP_FLAGS        = /D EDKII_GLUE_PciExpressBaseAddress=0x$(PLATFORM_PCIEXPRESS_BASE)
  *_*_X64_ASLCC_FLAGS        = /D EDKII_GLUE_PciExpressBaseAddress=0x$(PLATFORM_PCIEXPRESS_BASE)
  *_*_X64_ASM16_FLAGS        = /D EDKII_GLUE_PciExpressBaseAddress=$(PLATFORM_PCIEXPRESS_BASE)h
 # *_*_*_BUILD_FLAGS = -s
  *_*_*_VFR_FLAGS   = -c
  *_*_*_BUILD_FLAGS = -c

[BuildOptions.Common.EDKII]
  *_*_IA32_ASM_FLAGS     = $(VP_BUILD_OPTIONS) /D EDKII_GLUE_PciExpressBaseAddress=$(PLATFORM_PCIEXPRESS_BASE)h /DNOCS_S3_SUPPORT

  *_*_IA32_CC_FLAGS      = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_VFRPP_FLAGS   = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_APP_FLAGS     = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_PP_FLAGS      = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_IA32_ASLPP_FLAGS   = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)

  *_*_X64_CC_FLAGS       = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS) $(SOURCE_LEVEL_DEBUG_BUILD_OPTIONS)
  *_*_X64_VFRPP_FLAGS    = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_APP_FLAGS      = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_PP_FLAGS       = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)
  *_*_X64_ASLPP_FLAGS    = $(EDK_EDKII_DSC_FEATURE_BUILD_OPTIONS)


[Components.IA32]
 $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/SysFwUpdateCapsuleDxe.inf

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/I2cBus.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xF0000043
  }

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/I2cHost.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xF0000043
  }
  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/I2cPortA0Pio.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x00000043
  }

  $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/I2cMmioDeviceDxe.inf {
    <PcdsPatchableInModule>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x00000043
  }
