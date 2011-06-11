#
#  Copyright (c) 2011, ARM Limited. All rights reserved.
#  
#  This program and the accompanying materials                          
#  are licensed and made available under the terms and conditions of the BSD License         
#  which accompanies this distribution.  The full text of the license may be found at        
#  http://opensource.org/licenses/bsd-license.php                                            
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ArmRealViewEbPkg
  PLATFORM_GUID                  = F4C1AD3E-9D3E-4F61-8791-B3BB1C43D04C
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ArmRealViewEb-RTSM-A8
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = ArmPlatformPkg/ArmRealViewEbPkg/ArmRealViewEb-RTSM-UniCore.fdf

[LibraryClasses.common]
!if $(BUILD_TARGETS) == RELEASE
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  UncachedMemoryAllocationLib|ArmPkg/Library/UncachedMemoryAllocationLib/UncachedMemoryAllocationLib.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  UncachedMemoryAllocationLib|ArmPkg/Library/UncachedMemoryAllocationLib/UncachedMemoryAllocationLib.inf
#  UncachedMemoryAllocationLib|ArmPkg/Library/DebugUncachedMemoryAllocationLib/DebugUncachedMemoryAllocationLib.inf
!endif
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf

  ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7Lib.inf
  ArmPlatformLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/ArmRealViewEbLibRTSM/ArmRealViewEbLib.inf
  
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|ArmPkg/Library/BaseMemoryLibStm/BaseMemoryLibStm.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf

  EfiResetSystemLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/ResetSystemLib/ResetSystemLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  
  EblCmdLib|ArmPlatformPkg/Library/EblCmdLib/EblCmdLib.inf
  EfiFileLib|EmbeddedPkg/Library/EfiFileLib/EfiFileLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

  #
  # Uncomment (and comment out the next line) For RealView Debugger. The Standard IO window 
  # in the debugger will show load and unload commands for symbols. You can cut and paste this
  # into the command window to load symbols. We should be able to use a script to do this, but
  # the version of RVD I have does not support scripts accessing system memory.
  #
#  PeCoffExtraActionLib|ArmPkg/Library/RvdPeCoffExtraActionLib/RvdPeCoffExtraActionLib.inf
  PeCoffExtraActionLib|ArmPkg/Library/DebugPeCoffExtraActionLib/DebugPeCoffExtraActionLib.inf
#  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  DefaultExceptioHandlerLib|ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf
  
  SemihostLib|ArmPkg/Library/SemihostLib/SemihostLib.inf
  
  RealTimeClockLib|ArmPlatformPkg/Library/PL031RealTimeClockLib/PL031RealTimeClockLib.inf

  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf

  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

#
# Assume everything is fixed at build
#
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

  EblAddExternalCommandLib|EmbeddedPkg/Library/EblAddExternalCommandLib/EblAddExternalCommandLib.inf

  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

  EblNetworkLib|EmbeddedPkg/Library/EblNetworkLib/EblNetworkLib.inf
  
  ArmDisassemblerLib|ArmPkg/Library/ArmDisassemblerLib/ArmDisassemblerLib.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  DebugAgentTimerLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/DebugAgentTimerLib/DebugAgentTimerLib.inf

  SerialPortLib|ArmPlatformPkg/Library/PL011SerialPortLib/PL011SerialPortLib.inf
  TimerLib|ArmPlatformPkg/Library/SP804TimerLib/SP804TimerLib.inf  
  GdbSerialLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/GdbSerialLib/GdbSerialLib.inf
  DmaLib|ArmPkg/Library/ArmDmaLib/ArmDmaLib.inf
  
  # ARM PL011 UART Driver
  PL011UartLib|ArmPlatformPkg/Drivers/PL011Uart/PL011Uart.inf

  BdsLib|ArmPkg/Library/BdsLib/BdsLib.inf
  
  ArmPlatformSysConfigLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/SysConfigArmRealViewEbLib/SysConfigArmRealViewEbLib.inf
  NorFlashPlatformLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/NorFlashArmRealViewEbLib/NorFlashArmRealViewEbLib.inf
  LcdPlatformLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/PL111LcdArmRealViewEbLib/PL111LcdArmRealViewEbLib.inf

[LibraryClasses.common.SEC]
  ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7LibSec.inf
  ArmPlatformLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/ArmRealViewEbLibRTSM/ArmRealViewEbSecLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  
  # 1/123 faster than Stm or Vstm version
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  # Uncomment to turn on GDB stub in SEC. 
  #DebugAgentLib|EmbeddedPkg/Library/GdbDebugAgent/GdbDebugAgent.inf
  
  # L2 Cache Driver
  L2X0CacheLib|ArmPkg/Library/L2X0CacheLibNull/L2X0CacheLibNull.inf
  # ARM PL390 General Interrupt Driver in Secure and Non-secure
  PL390GicSecLib|ArmPkg/Drivers/PL390Gic/PL390GicSec.inf

[LibraryClasses.common.PEI_CORE]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  # note: this won't actually work since globals in PEI are not writeable
  # need to generate an ARM PEI services table pointer implementation
  PeiServicesTablePointerLib|ArmPlatformPkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

[LibraryClasses.common.PEIM]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  # note: this won't actually work since globals in PEI are not writeable
  # need to generate an ARM PEI services table pointer implementation
  PeiServicesTablePointerLib|ArmPlatformPkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeiResourcePublicationLib|MdePkg/Library/PeiResourcePublicationLib/PeiResourcePublicationLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf

[LibraryClasses.common.DXE_DRIVER]
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  PerformanceLib|MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf

[LibraryClasses.ARM]
  #
  # It is not possible to prevent the ARM compiler for generic intrinsic functions.
  # This library provides the instrinsic functions generate by a given compiler.
  # [LibraryClasses.ARM] and NULL mean link this library into all ARM images.
  #
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

[BuildOptions]
  RVCT:*_*_ARM_ARCHCC_FLAGS  == --cpu Cortex-A8 --thumb --fpu=softvfp -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform
  RVCT:*_*_ARM_ARCHASM_FLAGS == --cpu Cortex-A8 -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform
  RVCT:RELEASE_*_*_CC_FLAGS  = -DMDEPKG_NDEBUG -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform

  GCC:*_*_ARM_ARCHCC_FLAGS    == -march=armv7-a -mthumb -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform 
  GCC:*_*_ARM_ARCHASM_FLAGS   == -march=armv7-a -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform
  GCC:RELEASE_*_*_CC_FLAGS    = -DMDEPKG_NDEBUG -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform

  XCODE:*_*_ARM_ARCHCC_FLAGS     == -arch armv7 -march=armv7 -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform
  XCODE:*_*_ARM_ARCHASM_FLAGS    == -arch armv7 -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform
  XCODE:*_*_ARM_ARCHDLINK_FLAGS  == -arch armv7 -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform
  XCODE:RELEASE_*_*_CC_FLAGS     = -DMDEPKG_NDEBUG -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform


################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|TRUE
  
  gEmbeddedTokenSpaceGuid.PcdCacheEnable|TRUE
  
  # Use the Vector Table location in CpuDxe. We will not copy the Vector Table at PcdCpuVectorBaseAddress
  gArmTokenSpaceGuid.PcdRelocateVectorTable|FALSE
  
  gArmTokenSpaceGuid.PcdCpuDxeProduceDebugSupport|FALSE

  gEfiMdeModulePkgTokenSpaceGuid.PcdTurnOffUsbLegacySupport|TRUE
  
  gArmPlatformTokenSpaceGuid.PcdStandalone|TRUE

!if $(EDK2_SKIP_PEICORE) == 1
  gArmTokenSpaceGuid.PcdSkipPeiCore|TRUE
!endif

  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

[PcdsFixedAtBuild.common]
  gArmPlatformTokenSpaceGuid.PcdFirmwareVendor|"ARM RealView Emulation Board"
  
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPrompt|"ArmRealViewEb-A8"
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuMemorySize|32
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|0
  gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout|10000000
  gEfiMdePkgTokenSpaceGuid.PcdDebugClearMemoryValue|0xAF
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|1
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0
  gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|320

# DEBUG_ASSERT_ENABLED       0x01
# DEBUG_PRINT_ENABLED        0x02
# DEBUG_CODE_ENABLED         0x04
# CLEAR_MEMORY_ENABLED       0x08
# ASSERT_BREAKPOINT_ENABLED  0x10
# ASSERT_DEADLOOP_ENABLED    0x20
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f

#  DEBUG_INIT      0x00000001  // Initialization
#  DEBUG_WARN      0x00000002  // Warnings
#  DEBUG_LOAD      0x00000004  // Load events
#  DEBUG_FS        0x00000008  // EFI File system
#  DEBUG_POOL      0x00000010  // Alloc & Free's
#  DEBUG_PAGE      0x00000020  // Alloc & Free's
#  DEBUG_INFO      0x00000040  // Verbose
#  DEBUG_DISPATCH  0x00000080  // PEI/DXE Dispatchers
#  DEBUG_VARIABLE  0x00000100  // Variable
#  DEBUG_BM        0x00000400  // Boot Manager
#  DEBUG_BLKIO     0x00001000  // BlkIo Driver
#  DEBUG_NET       0x00004000  // SNI Driver
#  DEBUG_UNDI      0x00010000  // UNDI Driver
#  DEBUG_LOADFILE  0x00020000  // UNDI Driver
#  DEBUG_EVENT     0x00080000  // Event messages
#  DEBUG_ERROR     0x80000000  // Error
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8000000F

  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
  
  #
  # NV Storage PCDs. Use base of 0x43F00000 for NOR0
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x43F00000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x000C0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x43FC0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00020000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x43FE0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00020000

  gEmbeddedTokenSpaceGuid.PcdEmbeddedAutomaticBootCommand|""
  gEmbeddedTokenSpaceGuid.PcdEmbeddedDefaultTextColor|0x07
  gEmbeddedTokenSpaceGuid.PcdEmbeddedMemVariableStoreSize|0x10000
  
#
# Optional feature to help prevent EFI memory map fragments
# Turned on and off via: PcdPrePiProduceMemoryTypeInformationHob
# Values are in EFI Pages (4K). DXE Core will make sure that 
# at least this much of each type of memory can be allocated 
# from a single memory range. This way you only end up with
# maximum of two fragements for each type in the memory map
# (the memory used, and the free memory that was prereserved
# but not used).
#
  gArmTokenSpaceGuid.PcdCpuVectorBaseAddress|0x00000000
  
  # Stack for CPU Cores in Secure Mode
  gArmPlatformTokenSpaceGuid.PcdCPUCoresSecStackBase|0x4B000000     # Top of SEC Stack for Secure World
  gArmPlatformTokenSpaceGuid.PcdCPUCoreSecStackSize|0x2000          # Size of SEC Stack for Secure World

  # Stack for CPU Cores in Secure Monitor Mode
  gArmPlatformTokenSpaceGuid.PcdCPUCoresSecMonStackBase|0x4A000000  # Top of SEC Stack for Monitor World
  gArmPlatformTokenSpaceGuid.PcdCPUCoreSecMonStackSize|0x2000       # Size of SEC Stack for Monitor World

  # Stack for CPU Cores in Non Secure Mode
  gArmPlatformTokenSpaceGuid.PcdCPUCoresNonSecStackBase|0x48000000  # Top of SEC Stack for Normal World
  gArmPlatformTokenSpaceGuid.PcdCPUCoresNonSecStackSize|0x20000     # Size of SEC Stack for Normal World

  # Non Sec UEFI Firmware: These two PCDs must match PcdFlashFvMainBase/PcdFlashFvMainSize
  gArmTokenSpaceGuid.PcdNormalFdBaseAddress|0x40050000       # Must be equal to gEmbeddedTokenSpaceGuid.PcdFlashFvMainBase
  gArmTokenSpaceGuid.PcdNormalFdSize|0x00100000              # Must be equal to gEmbeddedTokenSpaceGuid.PcdFlashFvMainSize
  
  # System Memory (256MB) 
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x70000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x10000000
  
  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000
  
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPerformanceCounterFrequencyInHz|1000000
  gEmbeddedTokenSpaceGuid.PcdTimerPeriod|100000        # expressed in 100ns units, 100,000 x 100 ns = 10,000,000 ns = 10 ms
  gArmPlatformTokenSpaceGuid.PcdSP804Timer0InterruptNum|36
  
  #
  # ARM Pcds
  #
  gArmTokenSpaceGuid.PcdArmUncachedMemoryMask|0x0000000040000000
  
  #
  # ARM EB PCDS
  #
  gArmRealViewEbPkgTokenSpaceGuid.PcdGdbUartBase|0x1000a000
  
  #
  # ARM PL011 - Serial Terminal
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x10009000
  
  #
  # ARM PL390 General Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x10041000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x10040000
  gArmTokenSpaceGuid.PcdGicNumInterrupts|96

  #
  # ARM OS Loader
  #
  gArmTokenSpaceGuid.PcdArmMachineType|827
  gArmPlatformTokenSpaceGuid.PcdDefaultBootDescription|L"SemiHosting"
  gArmPlatformTokenSpaceGuid.PcdDefaultBootDevicePath|L"VenHw(C5B9C74A-6D72-4719-99AB-C59F199091EB)/zImage-RTSM"
  gArmPlatformTokenSpaceGuid.PcdDefaultBootArgument|"rdinit=/bin/ash debug earlyprintk console=ttyAMA0,38400 mem=128M"
  gArmPlatformTokenSpaceGuid.PcdDefaultBootType|1
  
  # Use the Serial console (ConIn & ConOut) and the Graphic driver (ConOut)
  gArmPlatformTokenSpaceGuid.PcdDefaultConOutPaths|L"VenHw(D3987D4B-971A-435F-8CAF-4967EB627241)/Uart(115200,8,N,1)/VenPcAnsi();VenHw(407B4008-BF5B-11DF-9547-CF16E0D72085)"
  gArmPlatformTokenSpaceGuid.PcdDefaultConInPaths|L"VenHw(D3987D4B-971A-435F-8CAF-4967EB627241)/Uart(115200,8,N,1)/VenPcAnsi()"
  gArmPlatformTokenSpaceGuid.PcdPlatformBootTimeOut|10
  
  #
  # ARM L2x0 PCDs
  #
  gArmTokenSpaceGuid.PcdL2x0ControllerBase|0x1F002000
 


################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]
  
#
# SEC
#
  ArmPlatformPkg/Sec/Sec.inf
  ArmPlatformPkg/PrePeiCore/PrePeiCoreUniCore.inf
  
#
# PEI Phase modules
#
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  ArmPlatformPkg/PlatformPei/PlatformPei.inf
  ArmPlatformPkg/MemoryInitPei/MemoryInitPei.inf
  IntelFrameworkModulePkg/Universal/StatusCode/Pei/StatusCodePei.inf
  Nt32Pkg/BootModePei/BootModePei.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

#
# DXE
#
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }

  #
  # Architectural Protocols
  #
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf  
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf  
  
  EmbeddedPkg/ResetRuntimeDxe/ResetRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe/PL111LcdGraphicsOutputDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  EmbeddedPkg/SerialDxe/SerialDxe.inf

  ArmPkg/Drivers/PL390Gic/PL390GicDxe.inf
  ArmPlatformPkg/Drivers/SP804TimerDxe/SP804TimerDxe.inf
  
  ArmPlatformPkg/Drivers/NorFlashDxe/NorFlashDxe.inf

  #
  # Semi-hosting filesystem
  #
  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf
  
  #
  # FAT filesystem + GPT/MBR partitioning
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  
  
  #
  # Application
  #  
  EmbeddedPkg/Ebl/Ebl.inf

  #
  # Bds
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  ArmPlatformPkg/Bds/Bds.inf
