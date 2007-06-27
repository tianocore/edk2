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
  
################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  $(WORKSPACE)\IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  $(WORKSPACE)\IntelFrameworkModulePkg\Library\OemHookStatusCodeLibNull\OemHookStatusCodeLibNull.inf
  $(WORKSPACE)\IntelFrameworkModulePkg\Library\PciIncompatibleDeviceSupportLib\PciIncompatibleDeviceSupportLib.inf
  #$(WORKSPACE)\IntelFrameworkModulePkg\Bus\Pci\PciBus\Dxe\PciBus.inf
  #$(WORKSPACE)\IntelFrameworkModulePkg\Bus\Pci\IdeBus\Dxe\IdeBus.inf