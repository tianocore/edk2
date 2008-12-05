#/** @file
# EFI/PI Reference Module Package for All Architectures
#
# Copyright (c) 2007 - 2008, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

[Defines]
  PLATFORM_NAME                  = MdeModule
  PLATFORM_GUID                  = 587CE499-6CBE-43cd-94E2-186218569478
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MdeModule
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT



[LibraryClasses.common]
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PostCodeLib|MdePkg/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  PostCodeLib|MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  S3Lib|MdeModulePkg/Library/PeiS3LibNull/PeiS3LibNull.inf
  RecoveryLib|MdeModulePkg/Library/PeiRecoveryLibNull/PeiRecoveryLibNull.inf

  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PalLib|MdePkg/Library/BasePalLibNull/BasePalLibNull.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  ExtendedHiiLib|MdeModulePkg/Library/ExtendedHiiLib/ExtendedHiiLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  IfrSupportLib|MdeModulePkg/Library/UefiIfrSupportLib/UefiIfrSupportLib.inf
  ExtendedIfrSupportLib|MdeModulePkg/Library/ExtendedIfrSupportLib/ExtendedIfrSupportLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  PlatformBdsLib|MdeModulePkg/Library/PlatformBdsLibNull/PlatformBdsLibNull.inf
  GenericBdsLib|MdeModulePkg/Library/GenericBdsLib/GenericBdsLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  PlatformBdsLib|MdeModulePkg/Library/PlatformBdsLibNull/PlatformBdsLibNull.inf
  GenericBdsLib|MdeModulePkg/Library/GenericBdsLib/GenericBdsLib.inf
  PlatDriOverLib|MdeModulePkg/Library/DxePlatDriOverLib/DxePlatDriOverLib.inf
  OemHookStatusCodeLib|IntelFrameworkModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf

[LibraryClasses.IA32]
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.X64]
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.IPF]
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.EBC.DXE_DRIVER]
  IoLib|IntelFrameworkPkg/Library/DxeIoLibCpuIo/DxeIoLibCpuIo.inf

[LibraryClasses.EBC.PEIM]
  IoLib|MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf

[LibraryClasses.common.PEI_CORE]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

[LibraryClasses.common.PEIM]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.DXE_SAL_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf

[PcdsFeatureFlag.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleRest|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleRest|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxePcdDatabaseTraverseEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseTraverseEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseSetEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseGetSizeEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseCallbackOnSetEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseExEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdNtEmulatorEnable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathFromText|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathToText|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportCustomDecompress|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplBuildShareCodeHobs|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportEfiDecompress|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportTianoDecompress|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportCustomDecompress|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableCollectStatistics|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdUnicodeCollationSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdUnicodeCollation2Support|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport|TRUE

[PcdsFeatureFlag.IA32]
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplEnableIdt|FALSE

[PcdsFixedAtBuild.common]
  gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout|10000000
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06
  gEfiMdePkgTokenSpaceGuid.PcdDebugClearMemoryValue|0xAF
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
  gEfiMdePkgTokenSpaceGuid.PcdFSBClock|200000000
  gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|320
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizePopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|28
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPcdCallBackNumberPerPcdEntry|0x08
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueEfiWatchDogTimerExpired|0x00011003   # EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_TIMER_EXPIRED
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueMemoryTestStarted|0x00051006         # EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_RS_PC_SET_VIRTUAL_ADDRESS_MAP
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueSetVirtualAddressMap|0x03101004      # EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TEST
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueUncorrectableMemoryError|0x0005100   # EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UNCORRECTABLE3
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleError|0x01040006        # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleReset|0x01040001        # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_PC_RESET
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleInputError|0x01040007   # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_INPUT_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleOutputError|0x01040008  # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_OUTPUT_ERROR

  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueMouseInterfaceError|0x01020005       # EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueMouseEnable|0x01020004               # EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueMouseDisable|0x01020002              # EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardEnable|0x01010004            # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardPresenceDetect|0x01010003    # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardDisable|0x01010002           # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardReset|0x01010001             # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_RESET
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardClearBuffer|0x01011000       # EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_CLEAR_BUFFER
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardSelfTest|0x01011001          # EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardInterfaceError|0x01010005    # EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INTERFACE_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueKeyboardInputError|0x01010007        # EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INPUT_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueMouseInputError|0x01020007           # EFI_PERIPHERAL_MOUSE | EFI_P_EC_INPUT_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueMouseReset|0x01020001                # EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET

  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|8
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|1
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|1
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultTerminalType|0
  gEfiMdePkgTokenSpaceGuid.PcdMaximumGuidedExtractHandler|0x10

  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxFvSupported|6
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeimPerFv|32
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeiStackSize|0x20000


[PcdsFixedAtBuild.IPF]
  gEfiMdePkgTokenSpaceGuid.PcdIoBlockBaseAddressForIpf|0x0ffffc000000

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  MdeModulePkg/Application/HelloWorld/HelloWorld.inf

  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/UndiRuntimeDxe/UndiRuntimeDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseAbsolutePointerDxe/UsbMouseAbsolutePointerDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf

  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf

  MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
  MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  MdeModulePkg/Library/DxePlatDriOverLib/DxePlatDriOverLib.inf
  MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  MdeModulePkg/Library/EdkDxePrintLib/EdkDxePrintLib.inf
  MdeModulePkg/Library/ExtendedHiiLib/ExtendedHiiLib.inf
  MdeModulePkg/Library/ExtendedIfrSupportLib/ExtendedIfrSupportLib.inf
  MdeModulePkg/Library/GenericBdsLib/GenericBdsLib.inf
  MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  MdeModulePkg/Library/PeiRecoveryLibNull/PeiRecoveryLibNull.inf
  MdeModulePkg/Library/PeiS3LibNull/PeiS3LibNull.inf
  MdeModulePkg/Library/PlatformBdsLibNull/PlatformBdsLibNull.inf
  MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  MdeModulePkg/Library/UefiIfrSupportLib/UefiIfrSupportLib.inf

  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/DebugPortDxe/DebugPortDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/DriverSampleDxe/DriverSampleDxe.inf
  MdeModulePkg/Universal/FirmwareVolume/FaultTolerantWriteDxe/FtwLite.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/MemoryTest/BaseMemoryTestPei/BaseMemoryTestPei.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf

  MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
  MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
  MdeModulePkg/Universal/Network/DpcDxe/DpcDxe.inf
  MdeModulePkg/Universal/Network/Ip4ConfigDxe/Ip4ConfigDxe.inf
  MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
  MdeModulePkg/Universal/Network/IScsiDxe/IScsiDxe.inf
  MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf
  MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
  MdeModulePkg/Universal/Network/PxeBcDxe/PxeBcDxe.inf
  MdeModulePkg/Universal/Network/PxeDhcp4Dxe/PxeDhcp4Dxe.inf
  MdeModulePkg/Universal/Network/SnpDxe/SnpDxe.inf
  MdeModulePkg/Universal/Network/Tcp4Dxe/Tcp4Dxe.inf
  MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf
  MdeModulePkg/Universal/Network/UefiPxeBcDxe/UefiPxeBcDxe.inf

  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf
  MdeModulePkg/Universal/PlatformDriverOverride/PlatformDriOverrideDxe/PlatformDriOverrideDxe.inf
  MdeModulePkg/Universal/PlatformDriverOverride/PlatOverMngr/PlatOverMngr.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/Variable/Application/VariableInfo.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf


[Components.IA32, Components.X64]
  MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf


[Components.IPF]
  MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf


[Components.EBC]
  #
  # PcatRealTimeClockRuntimeDxe.inf needs DXE I/O Library instance.
  #
  #MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf
