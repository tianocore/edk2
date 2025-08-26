## @file
# Bootloader Payload Package
#
# Provides drivers and definitions to create uefi payload for bootloaders.
#
# Copyright (c) 2014 - 2023, Intel Corporation. All rights reserved.<BR>
# Copyright (c) Microsoft Corporation.
# Copyright 2024 Google LLC
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                       = UefiPayloadPkg
  PLATFORM_GUID                       = F71608AB-D63D-4491-B744-A99998C8CD96
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  SUPPORTED_ARCHITECTURES             = IA32|X64|AARCH64
  BUILD_TARGETS                       = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER                    = DEFAULT
  BUILD_ARCH                          = Legacy
  OUTPUT_DIRECTORY                    = Build/UefiPayloadPkg$(BUILD_ARCH)
  FLASH_DEFINITION                    = UefiPayloadPkg/UefiPayloadPkg.fdf
  PCD_DYNAMIC_AS_DYNAMICEX            = TRUE

  DEFINE SOURCE_DEBUG_ENABLE          = FALSE
  DEFINE PS2_KEYBOARD_ENABLE          = FALSE
  DEFINE RAM_DISK_ENABLE              = FALSE
  DEFINE SIO_BUS_ENABLE               = FALSE
  DEFINE SECURITY_STUB_ENABLE         = TRUE
  DEFINE SMM_SUPPORT                  = FALSE
  DEFINE PLATFORM_BOOT_TIMEOUT        = 3
  DEFINE BOOT_MANAGER_ESCAPE          = FALSE
  DEFINE ATA_ENABLE                   = TRUE
  DEFINE SD_ENABLE                    = TRUE
  DEFINE PS2_MOUSE_ENABLE             = TRUE
  DEFINE SD_MMC_TIMEOUT               = 1000000
  DEFINE USE_CBMEM_FOR_CONSOLE        = FALSE
  DEFINE BOOTSPLASH_IMAGE             = FALSE
  DEFINE NVME_ENABLE                  = TRUE
  DEFINE LOCKBOX_SUPPORT              = FALSE
  DEFINE LOAD_OPTION_ROMS             = FALSE

  #
  # Capsule updates
  #
  # CAPSULE_MAIN_FW_GUID specifies GUID to be used by FmpDxe when
  # CAPSULE_SUPPORT is set to TRUE
  #
  DEFINE CAPSULE_SUPPORT              = FALSE
  DEFINE CAPSULE_MAIN_FW_GUID         =

  #
  # Crypto Support
  #
  DEFINE CRYPTO_PROTOCOL_SUPPORT        = FALSE
  DEFINE CRYPTO_DRIVER_EXTERNAL_SUPPORT = FALSE

  #
  # Setup Universal Payload
  #
  # ELF: Build UniversalPayload file as UniversalPayload.elf
  # FIT: Build UniversalPayload file as UniversalPayload.fit
  #
  DEFINE UNIVERSAL_PAYLOAD            = FALSE
  DEFINE UNIVERSAL_PAYLOAD_FORMAT     = ELF

  #
  # NULL:    NullMemoryTestDxe
  # GENERIC: GenericMemoryTestDxe
  #
  DEFINE MEMORY_TEST                  = NULL
  #
  # SBL:      UEFI payload for Slim Bootloader
  # COREBOOT: UEFI payload for coreboot
  #
  DEFINE   BOOTLOADER                 = SBL

  #
  # CPU options
  #
  DEFINE MAX_LOGICAL_PROCESSORS       = 1024

  #
  # PCI options
  #
  DEFINE PCIE_BASE_SUPPORT            = TRUE

  #
  # Serial port set up
  #
  DEFINE BAUD_RATE                    = 115200
  DEFINE SERIAL_CLOCK_RATE            = 1843200
  DEFINE SERIAL_LINE_CONTROL          = 3 # 8-bits, no parity
  DEFINE SERIAL_HARDWARE_FLOW_CONTROL = FALSE
  DEFINE SERIAL_DETECT_CABLE          = FALSE
  DEFINE SERIAL_FIFO_CONTROL          = 7 # Enable FIFO
  DEFINE UART_DEFAULT_BAUD_RATE       = $(BAUD_RATE)
  DEFINE UART_DEFAULT_DATA_BITS       = 8
  DEFINE UART_DEFAULT_PARITY          = 1
  DEFINE UART_DEFAULT_STOP_BITS       = 1
  DEFINE DEFAULT_TERMINAL_TYPE        = 0

  # Enabling the serial terminal will slow down the boot menu redering!
  DEFINE DISABLE_SERIAL_TERMINAL      = FALSE

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
  # Vendor FFFF Device 0000 Prog Interface 1, BAR #0, Offset 0, Stride = 1, Clock 1843200 (0x1c2000)
  #
  #                                       [Vendor]   [Device]  [----ClockRate---]  [------------Offset-----------] [Bar] [Stride] [RxFifo] [TxFifo]   [Rsvd]   [Vendor]
  DEFINE PCI_SERIAL_PARAMETERS        = {0xff,0xff, 0x00,0x00, 0x0,0x20,0x1c,0x00, 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x00,    0x01, 0x0,0x0, 0x0,0x0, 0x0,0x0, 0xff,0xff}

  #
  # Shell options: [BUILD_SHELL, MIN_BIN, NONE, UEFI_BIN]
  #
  DEFINE SHELL_TYPE                   = BUILD_SHELL

  #
  # EMU:      UEFI payload with EMU variable
  # SPI:      UEFI payload with SPI NV variable support
  # SMMSTORE: UEFI payload with coreboot SMM NV variable support
  # NONE:     UEFI payload with no variable modules
  #
  DEFINE VARIABLE_SUPPORT      = EMU

  DEFINE DISABLE_RESET_SYSTEM  = FALSE
  DEFINE NETWORK_DRIVER_ENABLE = FALSE

  # Dfine the maximum size of the capsule image without a reset flag that the platform can support.
  DEFINE MAX_SIZE_NON_POPULATE_CAPSULE = 0xa00000

  # Define RTC related register.
  DEFINE RTC_INDEX_REGISTER = 0x70
  DEFINE RTC_TARGET_REGISTER = 0x71

  DEFINE SERIAL_DRIVER_ENABLE = TRUE
  DEFINE PERFORMANCE_MEASUREMENT_ENABLE  = FALSE

  # For recent X86 CPU, 0x15 CPUID instruction will return Time Stamp Counter Frequence.
  # This is how BaseCpuTimerLib works, and a recommended way to get Frequence, so set the default value as TRUE.
  # Note: for emulation platform such as QEMU, this may not work and should set it as FALSE
  DEFINE CPU_TIMER_LIB_ENABLE  = TRUE

  #
  # HPET:  UEFI Payload will use HPET timer
  # LAPIC: UEFI Payload will use local APIC timer
  #
  DEFINE TIMER_SUPPORT      = HPET

  DEFINE MULTIPLE_DEBUG_PORT_SUPPORT = FALSE

  #
  # Security
  #
  DEFINE SECURE_BOOT_ENABLE       = FALSE

  #
  # Flat DeviceTree handoff option:
  #   FALSE: Handover HobList to Payload
  #   TRUE:  Handover FDT to Payload
  #
  #
  HAND_OFF_FDT_ENABLE       = FALSE

[BuildOptions]
  *_*_*_CC_FLAGS                 = -D DISABLE_NEW_DEPRECATED_INTERFACES
!if $(USE_CBMEM_FOR_CONSOLE) == FALSE
  GCC:RELEASE_*_*_CC_FLAGS       = -DMDEPKG_NDEBUG
  INTEL:RELEASE_*_*_CC_FLAGS     = /D MDEPKG_NDEBUG
  MSFT:RELEASE_*_*_CC_FLAGS      = /D MDEPKG_NDEBUG
!endif
  *_*_*_CC_FLAGS                 = $(APPEND_CC_FLAGS)

[BuildOptions.AARCH64]
  GCC:*_*_*_CC_FLAGS         = -mcmodel=tiny -mstrict-align

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_*_DLINK_FLAGS      = -z common-page-size=0x1000
  XCODE:*_*_*_DLINK_FLAGS    = -seg1addr 0x1000 -segalign 0x1000
  XCODE:*_*_*_MTOC_FLAGS     = -align 0x1000
  CLANGPDB:*_*_*_DLINK_FLAGS = /ALIGN:4096
  MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096

[BuildOptions.IA32.EDKII.DXE_RUNTIME_DRIVER, BuildOptions.X64.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_*_DLINK_FLAGS      = -z common-page-size=0x1000

[BuildOptions.AARCH64.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_*_DLINK_FLAGS      = -z common-page-size=0x10000

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

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  #
  # Entry point
  #
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
!if $(PCIE_BASE_SUPPORT) == FALSE
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
!else
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
!endif
  PciSegmentLib|MdePkg/Library/PciSegmentLibSegmentInfo/BasePciSegmentLibSegmentInfo.inf
  PciSegmentInfoLib|UefiPayloadPkg/Library/PciSegmentInfoLibAcpiBoardInfo/PciSegmentInfoLibAcpiBoardInfo.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLib/DxeHobListLib.inf
!if $(CRYPTO_PROTOCOL_SUPPORT) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
!else
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
!endif
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  HobLib|UefiPayloadPkg/Library/DxeHobLib/DxeHobLib.inf
  CustomFdtNodeParserLib|UefiPayloadPkg/Library/CustomFdtNodeParserNullLib/CustomFdtNodeParserNullLib.inf

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
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  #
  # Generic Modules
  #
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  !if $(CAPSULE_SUPPORT) == TRUE
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeCapsuleLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
    !if $(BOOTSPLASH_IMAGE)
    # Note, however, that DisplayUpdateProgressLibGraphics aborts firmware
    # update if GOP is missing, so text progress works in more environments.
    DisplayUpdateProgressLib|MdeModulePkg/Library/DisplayUpdateProgressLibGraphics/DisplayUpdateProgressLibGraphics.inf
    !else
    DisplayUpdateProgressLib|MdeModulePkg/Library/DisplayUpdateProgressLibText/DisplayUpdateProgressLibText.inf
    !endif
  # If there are no specific checks to do, null-library suffices
  CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
  FmpDependencyCheckLib|FmpDevicePkg/Library/FmpDependencyCheckLib/FmpDependencyCheckLib.inf
  # No need to save/restore FMP dependencies unless they are utilized
  FmpDependencyDeviceLib|FmpDevicePkg/Library/FmpDependencyDeviceLibNull/FmpDependencyDeviceLibNull.inf
  FmpDependencyLib|FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf
  FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
  !else
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  !endif
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  ResetSystemLib|UefiPayloadPkg/Library/ResetSystemLib/ResetSystemLib.inf
!if $(USE_CBMEM_FOR_CONSOLE) == TRUE
  SerialPortLib|UefiPayloadPkg/Library/CbSerialPortLib/CbSerialPortLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
!else
  !if $(MULTIPLE_DEBUG_PORT_SUPPORT) == TRUE
    SerialPortLib|UefiPayloadPkg/Library/BaseSerialPortLibHob/DxeBaseSerialPortLibHob.inf
  !else
    SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  !endif
  PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
!endif
  PlatformBootManagerLib|UefiPayloadPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf

  #
  # Misc
  #
  DebugPrintErrorLevelLib|UefiPayloadPkg/Library/DebugPrintErrorLevelLibHob/DebugPrintErrorLevelLibHob.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf
!if $(SOURCE_DEBUG_ENABLE) == TRUE
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
!else
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
!endif
  PlatformSupportLib|UefiPayloadPkg/Library/PlatformSupportLibNull/PlatformSupportLibNull.inf
!if $(UNIVERSAL_PAYLOAD) == FALSE
  !if $(BOOTLOADER) == "COREBOOT"
    BlParseLib|UefiPayloadPkg/Library/CbParseLib/CbParseLib.inf
  !else
    BlParseLib|UefiPayloadPkg/Library/SblParseLib/SblParseLib.inf
  !endif
!endif

  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
!if $(LOCKBOX_SUPPORT) == TRUE
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
!else
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
!endif
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
  SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  PlatformPKProtectionLib|SecurityPkg/Library/PlatformPKProtectionLibVarPolicy/PlatformPKProtectionLibVarPolicy.inf
  SecureBootVariableProvisionLib|SecurityPkg/Library/SecureBootVariableProvisionLib/SecureBootVariableProvisionLib.inf
!else
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif

!if $(VARIABLE_SUPPORT) == "EMU"
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
!elseif $(VARIABLE_SUPPORT) == "SMMSTORE"
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
!elseif $(VARIABLE_SUPPORT) == "SPI"
  PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  S3BootScriptLib|MdePkg/Library/BaseS3BootScriptLibNull/BaseS3BootScriptLibNull.inf
  MmUnblockMemoryLib|MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf
!endif
!if $(VARIABLE_SUPPORT) == "SMMSTORE" || ($(CAPSULE_SUPPORT) && $(BOOTLOADER) == "COREBOOT")
  SmmStoreLib|UefiPayloadPkg/Library/SmmStoreLib/SmmStoreLib.inf
!endif
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  CcExitLib|UefiCpuPkg/Library/CcExitLibNull/CcExitLibNull.inf
  AmdSvsmLib|UefiCpuPkg/Library/AmdSvsmLibNull/AmdSvsmLibNull.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf
  SmmRelocationLib|UefiCpuPkg/Library/SmmRelocationLib/SmmRelocationLib.inf
  HobPrintLib|MdeModulePkg/Library/HobPrintLib/HobPrintLib.inf
  BuildFdtLib|UefiPayloadPkg/Library/BuildFdtLib/BuildFdtLib.inf
  MmSaveStateLib|UefiCpuPkg/Library/MmSaveStateLib/IntelMmSaveStateLib.inf
  SmmCpuSyncLib|UefiCpuPkg/Library/SmmCpuSyncLib/SmmCpuSyncLib.inf

[LibraryClasses.common]
!if $(BOOTSPLASH_IMAGE)
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
!endif
  UefiCpuBaseArchSupportLib|UefiCpuPkg/Library/BaseArchSupportLib/BaseArchSupportLib.inf

[LibraryClasses.X64]
  #
  # CPU
  #
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  MicrocodeLib|UefiCpuPkg/Library/MicrocodeLib/MicrocodeLib.inf
  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf
!if $(CPU_TIMER_LIB_ENABLE) == TRUE && $(UNIVERSAL_PAYLOAD) == TRUE
  TimerLib|UefiCpuPkg/Library/CpuTimerLib/BaseCpuTimerLib.inf
!else
  TimerLib|UefiPayloadPkg/Library/AcpiTimerLib/AcpiTimerLib.inf
!endif
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  CpuPageTableLib|UefiCpuPkg/Library/CpuPageTableLib/CpuPageTableLib.inf

[LibraryClasses.AARCH64]
  ArmHvcLib|ArmPkg/Library/ArmHvcLib/ArmHvcLib.inf
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|UefiCpuPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
  ArmSmcLib|MdePkg/Library/ArmSmcLib/ArmSmcLib.inf

  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  EfiResetSystemLib|ArmPkg/Library/ArmPsciResetSystemLib/ArmPsciResetSystemLib.inf
  PL011UartLib|ArmPlatformPkg/Library/PL011UartLib/PL011UartLib.inf
  PL011UartClockLib|ArmPlatformPkg/Library/PL011UartClockLib/PL011UartClockLib.inf
  SerialPortLib|ArmPlatformPkg/Library/PL011SerialPortLib/PL011SerialPortLib.inf

  QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgMmioDxeLib.inf
  QemuFwCfgS3Lib|OvmfPkg/Library/QemuFwCfgS3Lib/BaseQemuFwCfgS3LibNull.inf
  QemuFwCfgSimpleParserLib|OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf
  QemuLoadImageLib|OvmfPkg/Library/GenericQemuLoadImageLib/GenericQemuLoadImageLib.inf

  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  VirtNorFlashDeviceLib|OvmfPkg/Library/VirtNorFlashDeviceLib/VirtNorFlashDeviceLib.inf
  VirtNorFlashPlatformLib|OvmfPkg/Library/FdtNorFlashQemuLib/FdtNorFlashQemuLib.inf

  # ARM Architectural Libraries
  ArmGenericTimerCounterLib|ArmPkg/Library/ArmGenericTimerPhyCounterLib/ArmGenericTimerPhyCounterLib.inf
  CpuExceptionHandlerLib|ArmPkg/Library/ArmExceptionLib/ArmExceptionLib.inf
  DefaultExceptionHandlerLib|ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf

  # ARM PL031 RTC Driver
  RealTimeClockLib|ArmPlatformPkg/Library/PL031RealTimeClockLib/PL031RealTimeClockLib.inf
  TimeBaseLib|EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf

  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  DebugAgentTimerLib|EmbeddedPkg/Library/DebugAgentTimerLibNull/DebugAgentTimerLibNull.inf

  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
  PeiHardwareInfoLib|OvmfPkg/Library/HardwareInfoLib/PeiHardwareInfoLib.inf
  QemuBootOrderLib|OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf

  # PCI Libraries
  DxeHardwareInfoLib|OvmfPkg/Library/HardwareInfoLib/DxeHardwareInfoLib.inf
  PciCapLib|OvmfPkg/Library/BasePciCapLib/BasePciCapLib.inf
  PciCapPciSegmentLib|OvmfPkg/Library/BasePciCapPciSegmentLib/BasePciCapPciSegmentLib.inf
  PciCapPciIoLib|OvmfPkg/Library/UefiPciCapPciIoLib/UefiPciCapPciIoLib.inf
  PciPcdProducerLib|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf

  ArmPlatformLib|ArmVirtPkg/Library/ArmPlatformLibQemu/ArmPlatformLibQemu.inf
  VirtioMmioDeviceLib|OvmfPkg/Library/VirtioMmioDeviceLib/VirtioMmioDeviceLib.inf
  VirtioLib|OvmfPkg/Library/VirtioLib/VirtioLib.inf

[LibraryClasses.common.SEC]
  HobLib|UefiPayloadPkg/Library/PayloadEntryHobLib/HobLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLibNull/DxeHobListLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!if $(MULTIPLE_DEBUG_PORT_SUPPORT) == TRUE
  SerialPortLib|UefiPayloadPkg/Library/BaseSerialPortLibHob/BaseSerialPortLibHob.inf
!endif

[LibraryClasses.common.DXE_CORE]
  DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLibNull/DxeHobListLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[LibraryClasses.X64.DXE_CORE]
!if $(SOURCE_DEBUG_ENABLE)
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
!endif

[LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[LibraryClasses.X64.DXE_DRIVER]
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
!if $(SOURCE_DEBUG_ENABLE)
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
!if $(SECURE_BOOT_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
!endif
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
!endif
!if $(CAPSULE_SUPPORT) == TRUE
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibFmp/DxeRuntimeCapsuleLib.inf
!endif

[LibraryClasses.common.UEFI_DRIVER,LibraryClasses.common.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
!endif

[LibraryClasses.X64.SMM_CORE]
!if $(SMM_SUPPORT) == TRUE
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  SmmServicesTableLib|MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf

  MemoryAllocationLib|MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  PerformanceLib|MdeModulePkg/Library/SmmCorePerformanceLib/SmmCorePerformanceLib.inf
!endif
!endif

[LibraryClasses.X64.DXE_SMM_DRIVER]
!if $(SMM_SUPPORT) == TRUE
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf

  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  SmmCpuPlatformHookLib|UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
  SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  SmmCpuRendezvousLib|UefiCpuPkg/Library/SmmCpuRendezvousLib/SmmCpuRendezvousLib.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  PerformanceLib|MdeModulePkg/Library/SmmPerformanceLib/SmmPerformanceLib.inf
!endif
!endif
!if $(VARIABLE_SUPPORT) == "SPI"
  SpiFlashLib|UefiPayloadPkg/Library/SpiFlashLib/SpiFlashLib.inf
  FlashDeviceLib|UefiPayloadPkg/Library/FlashDeviceLib/FlashDeviceLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform.
#
################################################################################
[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE
  ## This PCD specified whether ACPI SDT protocol is installed.
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdHiiOsRuntimeSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDegradeResourceForOptionRom|FALSE
  ## Whether capsules are allowed to persist across reset.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|$(CAPSULE_SUPPORT)

[PcdsFeatureFlag.X64]
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|TRUE
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmEnableBspElection|FALSE

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdHardwareErrorRecordLevel|1
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize|0x8000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x10000
!if $(VARIABLE_SUPPORT) == "EMU"
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable        |TRUE
!elseif $(VARIABLE_SUPPORT) == "SPI" || $(VARIABLE_SUPPORT) == "SMMSTORE"
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable        |FALSE
!endif

  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseMemory|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdUse1GPageTable|TRUE
  gUefiPayloadPkgTokenSpaceGuid.PcdHandOffFdtEnable|$(HAND_OFF_FDT_ENABLE)

  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }
  gUefiPayloadPkgTokenSpaceGuid.PcdPcdDriverFile|{ 0x57, 0x72, 0xcf, 0x80, 0xab, 0x87, 0xf9, 0x47, 0xa3, 0xfe, 0xD5, 0x0B, 0x76, 0xd8, 0x95, 0x41 }

!if $(SOURCE_DEBUG_ENABLE)
  gEfiSourceLevelDebugPkgTokenSpaceGuid.PcdDebugLoadImageMethod|0x2
!endif
  gUefiCpuPkgTokenSpaceGuid.PcdCpuSmmStackSize|0x4000
  gEfiMdeModulePkgTokenSpaceGuid.PcdEdkiiFpdtStringRecordEnableOnly| TRUE
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask       | 0x1
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdSdMmcGenericTimeoutValue|$(SD_MMC_TIMEOUT)

  gUefiPayloadPkgTokenSpaceGuid.PcdBootManagerEscape|$(BOOT_MANAGER_ESCAPE)

  gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength|1800000

  ## Whether FMP capsules are enabled.
  gEfiMdeModulePkgTokenSpaceGuid.PcdCapsuleFmpSupport|$(CAPSULE_SUPPORT)

!if $(CRYPTO_PROTOCOL_SUPPORT) == TRUE
!if $(CRYPTO_DRIVER_EXTERNAL_SUPPORT) == FALSE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Md5.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Dh.Family                                | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Random.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sm3.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Tls.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsSet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsGet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
!endif
!endif


!if $(SECURE_BOOT_ENABLE) == TRUE
  # Override the default values from SecurityPkg to ensure images from all sources are verified in secure boot
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04
!endif

[PcdsFixedAtBuild.AARCH64]
  # System Memory Base -- fixed at 0x4000_0000
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000

  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x4007c000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000

  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000

  # ARM General Interrupt Controller
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x8000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x80a0000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x8080000

  # Enable NX memory protection for all non-code regions, including OEM and OS
  # reserved ones, with the exception of LoaderData regions, of which OS loaders
  # (i.e., GRUB) may assume that its contents are executable.
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeNxMemoryProtectionPolicy|0xC000000000007FD1

  # initial location of the device tree blob passed by QEMU -- base of DRAM
  gUefiOvmfPkgTokenSpaceGuid.PcdDeviceTreeInitialBaseAddress|0x40000000

  # The maximum physical I/O addressability of the processor, set with BuildCpuHob().
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|16

  # Enable the non-executable DXE stack. (This gets set up by DxeIpl)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetNxForStack|TRUE

  # Shadowing PEI modules is absolutely pointless when the NOR flash is emulated
  gEfiMdeModulePkgTokenSpaceGuid.PcdShadowPeimOnBoot|FALSE

  # System Memory Size -- 128 MB initially, actual size will be fetched from DT
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x8000000

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize   | 0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize   | 0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize | 0x40000

  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x9000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|1
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciSerialParameters|$(PCI_SERIAL_PARAMETERS)

  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosEntryPointProvideMethod|0x2

  # AARCH64 use PL011 Serial Port device instead of UniversalPayload Serial
  gUefiPayloadPkgTokenSpaceGuid.PcdUseUniversalPayloadSerialPort|FALSE

[PcdsPatchableInModule.IA32, PcdsPatchableInModule.X64]
!if $(NETWORK_DRIVER_ENABLE) == TRUE
  gEfiNetworkPkgTokenSpaceGuid.PcdAllowHttpConnections|TRUE
!endif
  gUefiPayloadPkgTokenSpaceGuid.SizeOfIoSpace|16
  gUefiPayloadPkgTokenSpaceGuid.PcdFDTPageSize|8
  #
  # The following parameters are set by Library/PlatformHookLib
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x3F8
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|$(BAUD_RATE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|1

[PcdsPatchableInModule.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x7
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8000004F
!if $(USE_CBMEM_FOR_CONSOLE) == FALSE
  !if $(SOURCE_DEBUG_ENABLE)
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17
  !else
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2F
  !endif
!else
  !if $(TARGET) == DEBUG
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x07
  !else
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x03
  !endif
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|$(MAX_SIZE_NON_POPULATE_CAPSULE)

  #
  # Enable these parameters to be set on the command line
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|$(SERIAL_CLOCK_RATE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialLineControl|$(SERIAL_LINE_CONTROL)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|$(SERIAL_HARDWARE_FLOW_CONTROL)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialDetectCable|$(SERIAL_DETECT_CABLE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|$(SERIAL_FIFO_CONTROL)

  gEfiMdeModulePkgTokenSpaceGuid.PcdPciSerialParameters|$(PCI_SERIAL_PARAMETERS)

  gUefiCpuPkgTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber|$(MAX_LOGICAL_PROCESSORS)
  gUefiCpuPkgTokenSpaceGuid.PcdCpuNumberOfReservedVariableMtrrs|0
  gUefiPayloadPkgTokenSpaceGuid.PcdBootloaderParameter|0

[PcdsPatchableInModule.AARCH64]
  gUefiPayloadPkgTokenSpaceGuid.SizeOfIoSpace|16
  gUefiPayloadPkgTokenSpaceGuid.PcdFDTPageSize|8

  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x9000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|1

!if $(TARGET) == DEBUG
   gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x07
!else
   gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x03
!endif

################################################################################
#
# Pcd DynamicEx Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamicHii]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|$(PLATFORM_BOOT_TIMEOUT)

[PcdsDynamicExDefault]
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|$(DEFAULT_TERMINAL_TYPE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdAriSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMrIovSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPcieResizableBarSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSystemPageSize|0x1
  gUefiCpuPkgTokenSpaceGuid.PcdCpuApInitTimeOutInMicroSeconds|50000
  gUefiCpuPkgTokenSpaceGuid.PcdCpuApLoopMode|1
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMicrocodePatchAddress|0x0
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMicrocodePatchRegionSize|0x0
!if ($(TARGET) == DEBUG || $(USE_CBMEM_FOR_CONSOLE) == TRUE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|TRUE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvStoreReserved|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0
!if $(VARIABLE_SUPPORT) == "SPI" || $(VARIABLE_SUPPORT) == "SMMSTORE"
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize  |0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize  |0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase  |0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64|0
!endif
  # Disable SMM S3 script
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiS3Enable|FALSE

  ## This PCD defines the video horizontal resolution.
  #  This PCD could be set to 0 then video resolution could be at highest resolution.
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|0
  ## This PCD defines the video vertical resolution.
  #  This PCD could be set to 0 then video resolution could be at highest resolution.
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|0

  ## The PCD is used to specify the video horizontal resolution of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|0
  ## The PCD is used to specify the video vertical resolution of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|0

  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|31
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|100
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseSize|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdGhcbBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdTestKeyUsed|FALSE
  gUefiCpuPkgTokenSpaceGuid.PcdSevEsIsEnabled|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration|TRUE

[PcdsDynamicExDefault.IA32, PcdsDynamicExDefault.X64]
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcIndexRegister|$(RTC_INDEX_REGISTER)
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcTargetRegister|$(RTC_TARGET_REGISTER)

  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|$(UART_DEFAULT_BAUD_RATE)
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|$(UART_DEFAULT_DATA_BITS)
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|$(UART_DEFAULT_PARITY)
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|$(UART_DEFAULT_STOP_BITS)

[PcdsDynamicExDefault.AARCH64]

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase     | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64   | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64   | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase     | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase   | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64 | 0

  # Timer IRQs
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|29
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|30
  # Not used in QEMU platform
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|0
  gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|26
  gArmTokenSpaceGuid.PcdArmArchTimerHypVirtIntrNum|0x0

  # PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x9010000

  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0x4010000000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseSize|0xfffffff
  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation|0x0

  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|1280
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|800
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|640
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|480
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0

  # SMBIOS entry point version
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0300
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev|0x0
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuSmbiosValidated|FALSE

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform.
#
################################################################################

!if "IA32" in "$(ARCH)"
  [Components.IA32]
  !if $(UNIVERSAL_PAYLOAD) == TRUE
    !if $(UNIVERSAL_PAYLOAD_FORMAT) == "ELF"
      UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry.inf
    !elseif $(UNIVERSAL_PAYLOAD_FORMAT) == "FIT"
      UefiPayloadPkg/UefiPayloadEntry/FitUniversalPayloadEntry.inf {
        <LibraryClasses>
          !if gUefiPayloadPkgTokenSpaceGuid.PcdHandOffFdtEnable == TRUE
            FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf
            CustomFdtNodeParserLib|UefiPayloadPkg/Library/CustomFdtNodeParserLib/CustomFdtNodeParserLib.inf
            NULL|UefiPayloadPkg/Library/FdtParserLib/FdtParseLib.inf
          !endif
          NULL|UefiPayloadPkg/Library/HobParseLib/HobParseLib.inf
      }
    !else
      UefiPayloadPkg/UefiPayloadEntry/UefiPayloadEntry.inf
    !endif
  !else
    UefiPayloadPkg/UefiPayloadEntry/UefiPayloadEntry.inf
  !endif
!else
  [Components.X64, Components.AARCH64]
  !if $(UNIVERSAL_PAYLOAD) == TRUE
    !if $(UNIVERSAL_PAYLOAD_FORMAT) == "ELF"
      UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry.inf
    !elseif $(UNIVERSAL_PAYLOAD_FORMAT) == "FIT"
      UefiPayloadPkg/UefiPayloadEntry/FitUniversalPayloadEntry.inf {
        <LibraryClasses>
          !if gUefiPayloadPkgTokenSpaceGuid.PcdHandOffFdtEnable == TRUE
            FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf
            CustomFdtNodeParserLib|UefiPayloadPkg/Library/CustomFdtNodeParserLib/CustomFdtNodeParserLib.inf
            NULL|UefiPayloadPkg/Library/FdtParserLib/FdtParseLib.inf
          !endif
          NULL|UefiPayloadPkg/Library/HobParseLib/HobParseLib.inf
      }
    !else
      UefiPayloadPkg/UefiPayloadEntry/UefiPayloadEntry.inf
    !endif
  !else
    UefiPayloadPkg/UefiPayloadEntry/UefiPayloadEntry.inf
  !endif
!endif

#
# UEFI network modules
#
!if $(NETWORK_DRIVER_ENABLE) == TRUE
[Defines]
  DEFINE PLATFORMX64_ENABLE = TRUE
  !include NetworkPkg/Network.dsc.inc
!endif

[Components.X64, Components.AARCH64]
  #
  # DXE Core
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      !if $(MULTIPLE_DEBUG_PORT_SUPPORT) == TRUE
        DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
        SerialPortLib|UefiPayloadPkg/Library/BaseSerialPortLibHob/DxeBaseSerialPortLibHob.inf
      !endif
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  #
  # Components that produce the architectural protocols
  #
!if $(SECURITY_STUB_ENABLE) == TRUE
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
      <LibraryClasses>
!if $(SECURE_BOOT_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
!endif
  }
!endif

!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!endif

  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
!if $(BOOTSPLASH_IMAGE)
  MdeModulePkg/Logo/LogoDxe.inf
!endif
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf
!if $(CAPSULE_SUPPORT) == TRUE
  # Build FmpDxe meant for the inclusion into an update capsule as an embedded driver.
  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      # FmpDxe interprets its FILE_GUID as firmware GUID.  This allows including
      # multiple FmpDxe instances along each other targeting different
      # components.
      FILE_GUID = $(CAPSULE_MAIN_FW_GUID)
    <PcdsFixedAtBuild>
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"System Firmware"
      # Public certificate used for validation of UEFI capsules
      #
      # See BaseTools/Source/Python/Pkcs7Sign/Readme.md for more details on such
      # PCDs and include files.
      !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
    <LibraryClasses>
!if $(BOOTLOADER) == "COREBOOT"
      FmpDeviceLib|UefiPayloadPkg/Library/FmpDeviceSmmLib/FmpDeviceSmmLib.inf
!else
      # TODO: provide platform-specific implementation of firmware flashing
      FmpDeviceLib|FmpDevicePkg/Library/FmpDeviceLibNull/FmpDeviceLibNull.inf
!endif
  }
  MdeModulePkg/Universal/EsrtDxe/EsrtDxe.inf
!endif


  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
!if $(DISABLE_RESET_SYSTEM) == FALSE
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
!endif
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
!if $(EMU_VARIABLE_ENABLE) == TRUE
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
!endif
  #
  # Following are the DXE drivers
  #
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
!if $(MEMORY_TEST) == "GENERIC"
  MdeModulePkg/Universal/MemoryTest/GenericMemoryTestDxe/GenericMemoryTestDxe.inf
!elseif $(MEMORY_TEST) == "NULL"
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
!endif
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/PlatformDriOverrideDxe/PlatformDriOverrideDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf

  UefiPayloadPkg/BlSupportDxe/BlSupportDxe.inf

  #
  # SMBIOS Support
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

  #
  # ACPI Support
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
!if $(BOOTSPLASH_IMAGE)
  MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf
!endif

  #
  # PCI Support
  #
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      PciHostBridgeLib|UefiPayloadPkg/Library/PciHostBridgeLib/PciHostBridgeLib.inf
  }

  #
  # SCSI/ATA/IDE/DISK Support
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
!if $(ATA_ENABLE) == TRUE
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
!endif
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
!if $(NVME_ENABLE) == TRUE
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
!endif

!if $(RAM_DISK_ENABLE) == TRUE
  MdeModulePkg/Universal/Disk/RamDiskDxe/RamDiskDxe.inf
!endif
  #
  # SD/eMMC Support
  #
!if $(SD_ENABLE) == TRUE
  MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.inf
  MdeModulePkg/Bus/Sd/EmmcDxe/EmmcDxe.inf
  MdeModulePkg/Bus/Sd/SdDxe/SdDxe.inf
!endif

  #
  # Support for loading Option ROMs from PCI-Express devices
  #
!if $(LOAD_OPTION_ROMS) == TRUE
  UefiPayloadPkg/PciPlatformDxe/PciPlatformDxe.inf
!endif

  #
  # Usb Support
  #
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf

  #
  # ISA Support
  #
!if $(SERIAL_DRIVER_ENABLE) == TRUE
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
!endif
!if $(SIO_BUS_ENABLE) == TRUE
  OvmfPkg/SioBusDxe/SioBusDxe.inf
!endif
!if $(PS2_KEYBOARD_ENABLE) == TRUE
  MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KeyboardDxe.inf
!endif
!if $(PS2_MOUSE_ENABLE) == TRUE
  MdeModulePkg/Bus/Isa/Ps2MouseDxe/Ps2MouseDxe.inf
!endif

  #
  # Console Support
  #
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
!if $(DISABLE_SERIAL_TERMINAL) == FALSE
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
!endif
  UefiPayloadPkg/GraphicsOutputDxe/GraphicsOutputDxe.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableDxe/FirmwarePerformanceDxe.inf
!endif
  #
  # SMM Support
  #
!if $(SMM_SUPPORT) == TRUE
  UefiPayloadPkg/SmmAccessDxe/SmmAccessDxe.inf
  UefiPayloadPkg/SmmControlRuntimeDxe/SmmControlRuntimeDxe.inf
  UefiPayloadPkg/BlSupportSmm/BlSupportSmm.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf
  UefiPayloadPkg/PchSmiDispatchSmm/PchSmiDispatchSmm.inf
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf
  UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
!if $(PERFORMANCE_MEASUREMENT_ENABLE)
  MdeModulePkg/Universal/Acpi/FirmwarePerformanceDataTableSmm/FirmwarePerformanceSmm.inf
!endif
!endif

!if $(VARIABLE_SUPPORT) == "EMU"
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
!elseif $(VARIABLE_SUPPORT) == "SMMSTORE"
  UefiPayloadPkg/SmmStoreFvb/SmmStoreFvbRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|EmbeddedPkg/Library/NvVarStoreFormattedLib/NvVarStoreFormattedLib.inf
  }
!elseif $(VARIABLE_SUPPORT) == "SPI"
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|MdeModulePkg/Library/VarCheckHiiLib/VarCheckHiiLib.inf
      NULL|MdeModulePkg/Library/VarCheckPcdLib/VarCheckPcdLib.inf
      NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
  }

  UefiPayloadPkg/FvbRuntimeDxe/FvbSmm.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
!endif

  #
  # Misc
  #
!if $(CRYPTO_PROTOCOL_SUPPORT) == TRUE
!if $(CRYPTO_DRIVER_EXTERNAL_SUPPORT) == FALSE
  CryptoPkg/Driver/CryptoDxe.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  }
!endif
!endif

[Components.X64]
  UefiCpuPkg/CpuDxe/CpuDxe.inf

!if $(TIMER_SUPPORT) == "HPET"
  PcAtChipsetPkg/HpetTimerDxe/HpetTimerDxe.inf
!elseif $(TIMER_SUPPORT) == "LAPIC"
  OvmfPkg/LocalApicTimerDxe/LocalApicTimerDxe.inf
!else
  !error "Invalid TIMER_SUPPORT"
!endif

[Components.AARCH64]
  ArmPkg/Drivers/ArmPciCpuIo2Dxe/ArmPciCpuIo2Dxe.inf
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  UefiCpuPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf

  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  OvmfPkg/VirtNorFlashDxe/VirtNorFlashDxe.inf {
    <LibraryClasses>
      # don't use unaligned CopyMem () on the UEFI varstore NOR flash region
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }

  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf

  OvmfPkg/PlatformDxe/Platform.inf
  OvmfPkg/Fdt/VirtioFdtDxe/VirtioFdtDxe.inf
  OvmfPkg/Fdt/HighMemDxe/HighMemDxe.inf
  OvmfPkg/VirtioBlkDxe/VirtioBlk.inf
  OvmfPkg/VirtioScsiDxe/VirtioScsi.inf
  OvmfPkg/VirtioNetDxe/VirtioNet.inf
  OvmfPkg/VirtioRngDxe/VirtioRng.inf
  OvmfPkg/VirtioSerialDxe/VirtioSerial.inf

  MdeModulePkg/Universal/Disk/UdfDxe/UdfDxe.inf
  OvmfPkg/VirtioFsDxe/VirtioFsDxe.inf

  # SMBIOS Support
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

  # PCI support
  UefiCpuPkg/CpuMmio2Dxe/CpuMmio2Dxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
  OvmfPkg/PciHotPlugInitDxe/PciHotPlugInit.inf
  OvmfPkg/VirtioPciDeviceDxe/VirtioPciDeviceDxe.inf
  OvmfPkg/Virtio10Dxe/Virtio10.inf

  # Video support
  OvmfPkg/QemuRamfbDxe/QemuRamfbDxe.inf
  OvmfPkg/VirtioGpuDxe/VirtioGpu.inf

  # Hash2 Protocol Support
  SecurityPkg/Hash2DxeCrypto/Hash2DxeCrypto.inf

  # ACPI Support
  OvmfPkg/PlatformHasAcpiDtDxe/PlatformHasAcpiDtDxe.inf

  #------------------------------
  #  Build the shell
  #------------------------------

!if $(SHELL_TYPE) == BUILD_SHELL

  #
  # Shell Lib
  #
[LibraryClasses]
  BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  !include NetworkPkg/NetworkLibs.dsc.inc

[Components.X64, Components.AARCH64]
  ShellPkg/DynamicCommand/TftpDynamicCommand/TftpDynamicCommand.inf {
    <PcdsFixedAtBuild>
      ## This flag is used to control initialization of the shell library
      #  This should be FALSE for compiling the dynamic command.
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }
!if $(PERFORMANCE_MEASUREMENT_ENABLE) == TRUE
  ShellPkg/DynamicCommand/DpDynamicCommand/DpDynamicCommand.inf {
    <PcdsFixedAtBuild>
      ## This flag is used to control initialization of the shell library
      #  This should be FALSE for compiling the dynamic command.
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }
!endif
  ShellPkg/Application/Shell/Shell.inf {
    <PcdsFixedAtBuild>
      ## This flag is used to control initialization of the shell library
      #  This should be FALSE for compiling the shell application itself only.
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE

    #------------------------------
    #  Basic commands
    #------------------------------

    <LibraryClasses>
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf

    #------------------------------
    #  Networking commands
    #------------------------------

    <LibraryClasses>
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf

    #------------------------------
    #  Support libraries
    #------------------------------

    <LibraryClasses>
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  }

!endif
