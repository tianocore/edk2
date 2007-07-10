#/** @file
# Intel Framework Reference Module Package for All Architectures
#
# This file is used to build all modules in IntelFrameworkModulePkg.
#
# Copyright (c) 2007, Intel Corporation
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

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = IntelFrameworkModuleAll
  PLATFORM_GUID                  = FFF87D9A-E5BB-4AFF-9ADE-8645492E8087
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = $(WORKSPACE)/Build/IntelFrameworkModuleAll
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

[LibraryClasses.common]
  CacheMaintenanceLib|$(WORKSPACE)/MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|$(WORKSPACE)/MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciCf8Lib|$(WORKSPACE)/MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|$(WORKSPACE)/MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciLib|$(WORKSPACE)/MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PeCoffGetEntryPoint|$(WORKSPACE)/MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|$(WORKSPACE)/MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PerformanceLib|$(WORKSPACE)/MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PostCodeLib|$(WORKSPACE)/MdePkg/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  PostCodeLib|$(WORKSPACE)/MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiDecompressLib|$(WORKSPACE)/MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  OemHookStatusCodeLib|$(WORKSPACE)/IntelFrameworkModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  SerialPortLib|$(WORKSPACE)/MdePkg/Library/SerialPortLibNull/SerialPortLibNull.inf
  GraphicsLib|$(WORKSPACE)/IntelFrameworkModulePkg/Library/GraphicsLib/GraphicsLib.inf

[LibraryClasses.common.PEIM]
  HobLib|$(WORKSPACE)/MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  IoLib|$(WORKSPACE)/MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PeimEntryPoint|$(WORKSPACE)/MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|$(WORKSPACE)/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|$(WORKSPACE)/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  BootScriptLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeBootScriptLibNull/DxeBootScriptLibNull.inf
  HiiLibFramework|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLibFramework/HiiLib.inf
  IfrSupportLibFramework|$(WORKSPACE)/IntelFrameworkPkg/Library/IfrSupportLibFramework/IfrSupportLib.inf
  PciIncompatibleDeviceSupportLib|$(WORKSPACE)/IntelFrameworkModulePkg/Library/PciIncompatibleDeviceSupportLib/PciIncompatibleDeviceSupportLib.inf
  IoLib|${WORKSPACE}/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf


[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  HiiLibFramework|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLibFramework/HiiLib.inf
  IfrSupportLibFramework|$(WORKSPACE)/IntelFrameworkPkg/Library/IfrSupportLibFramework/IfrSupportLib.inf
  IoLib|${WORKSPACE}/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag.common]
  PcdComponentNameDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnosticsDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdComponentName2Disable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnostics2Disable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseSerial|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseMemory|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseOEM|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseEfiSerial|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseHardSerial|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseRuntimeMemory|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseDataHub|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeReplayInSerial|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeReplayInRuntimeMemory|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeReplayInDataHub|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeReplayInOEM|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdPciIsaEnable|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdPciVgaEnable|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdPciBusHotplugDeviceSupport|gEfiIntelFrameworkModulePkgTokenSpaceGuid|TRUE
  PcdIsaBusSupportDma|gEfiIntelFrameworkModulePkgTokenSpaceGuid|TRUE
  PcdIsaBusOnlySupportSlaveDma|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE
  PcdIsaBusSupportIsaMemory|gEfiIntelFrameworkModulePkgTokenSpaceGuid|TRUE

[PcdsFixedAtBuild.common]
  PcdMaximumUnicodeStringLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdMaximumAsciiStringLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdMaximumLinkedListLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdSpinLockTimeout|gEfiMdePkgTokenSpaceGuid|10000000
  PcdDebugPropertyMask|gEfiMdePkgTokenSpaceGuid|0x0f
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000
  PcdReportStatusCodePropertyMask|gEfiMdePkgTokenSpaceGuid|0x06
  PcdDebugClearMemoryValue|gEfiMdePkgTokenSpaceGuid|0xAF
  PcdPerformanceLibraryPropertyMask|gEfiMdePkgTokenSpaceGuid|0
  PcdPostCodePropertyMask|gEfiMdePkgTokenSpaceGuid|0
  PcdPciExpressBaseAddress|gEfiMdePkgTokenSpaceGuid|0xE0000000
  PcdFSBClock|gEfiMdePkgTokenSpaceGuid|200000000
  PcdUefiLibMaxPrintBufferSize|gEfiMdePkgTokenSpaceGuid|320
  PcdPciIncompatibleDeviceSupportMask|gEfiIntelFrameworkModulePkgTokenSpaceGuid|0

[PcdsPatchableInModule.common]
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000
  PcdPciExpressBaseAddress|gEfiMdePkgTokenSpaceGuid|0xE0000000
  PcdFSBClock|gEfiMdePkgTokenSpaceGuid|200000000

[PcdsDynamicDefault.common.DEFAULT]
  PcdStatusCodeMemorySize|gEfiIntelFrameworkModulePkgTokenSpaceGuid|1
  PcdStatusCodeRuntimeMemorySize|gEfiIntelFrameworkModulePkgTokenSpaceGuid|4

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  $(WORKSPACE)/IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Library/PciIncompatibleDeviceSupportLib/PciIncompatibleDeviceSupportLib.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Library/GraphicsLib/GraphicsLib.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Bus/Pci/PciBus/Dxe/PciBus.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Bus/Pci/IdeBus/Dxe/IdeBus.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Bus/Isa/IsaBusDxe/IsaBus.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Bus/Isa/IsaFloppy/Dxe/IsaFloppy.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Bus/Isa/IsaFloppy/Pei/FloppyPeim.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2keyboard.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Bus/Isa/Ps2MouseDxe/Ps2Mouse.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Universal/DataHub/DataHub/Dxe/DataHub.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Universal/DataHub/DataHubStdErr/Dxe/DataHubStdErr.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Universal/StatusCode/Dxe/DxeStatusCode.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Universal/StatusCode/Pei/PeiStatusCode.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Universal/HiiDataBaseDxe/HiiDatabase.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Universal/SetupBrowserDxe/SetupBrowser.inf
  $(WORKSPACE)/IntelFrameworkModulePkg/Universal/DriverSampleDxe/DriverSample.inf

[Components.IA32]  
  $(WORKSPACE)/IntelFrameworkModulePkg/PcatCompatible/RealTimeClockRuntimeDxe/PcRtc.inf

[Components.X64]  
  $(WORKSPACE)/IntelFrameworkModulePkg/PcatCompatible/RealTimeClockRuntimeDxe/PcRtc.inf

