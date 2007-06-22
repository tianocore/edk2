#/** @file
# EFI/PI MdePkg Package
#
# This FPD file is used for Package Level build.
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
  PLATFORM_NAME                  = MdePkgAll
  PLATFORM_GUID                  = 082F8BFC-0455-4859-AE3C-ECD64FB81642
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = $(WORKSPACE)\Build/Mde
  SUPPORTED_ARCHITECTURES        = IA32 IPF X64 EBC
  BUILD_TARGETS                  = DEBUG,RELEASE
  SKUID_IDENTIFIER               = DEFAULT


################################################################################

[PcdsFeatureFlag.common]
  PcdComponentNameDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnosticsDisable|gEfiMdePkgTokenSpaceGuid|FALSE

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

[PcdsPatchableInModule.common]
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000
  PcdPciExpressBaseAddress|gEfiMdePkgTokenSpaceGuid|0xE0000000
  PcdFSBClock|gEfiMdePkgTokenSpaceGuid|200000000


[PcdsFixedAtBuild.IPF]
  PcdIoBlockBaseAddressForIpf|gEfiMdePkgTokenSpaceGuid|0x0ffffc000000

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  ${WORKSPACE}\MdePkg\Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  ${WORKSPACE}\MdePkg\Library/BaseDebugLibNull/BaseDebugLibNull.inf
  ${WORKSPACE}\MdePkg\Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  ${WORKSPACE}\MdePkg\Library/BaseLib/BaseLib.inf
  ${WORKSPACE}\MdePkg\Library/BaseMemoryLib/BaseMemoryLib.inf
  ${WORKSPACE}\MdePkg\Library/BasePciCf8Lib/BasePciCf8Lib.inf
  ${WORKSPACE}\MdePkg\Library/BasePciExpressLib/BasePciExpressLib.inf
  ${WORKSPACE}\MdePkg\Library/BasePciLibCf8/BasePciLibCf8.inf
  ${WORKSPACE}\MdePkg\Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  ${WORKSPACE}\MdePkg\Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  ${WORKSPACE}\MdePkg\Library/BasePeCoffLib/BasePeCoffLib.inf
  ${WORKSPACE}\MdePkg\Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  ${WORKSPACE}\MdePkg\Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  ${WORKSPACE}\MdePkg\Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  ${WORKSPACE}\MdePkg\Library/BasePrintLib/BasePrintLib.inf
  ${WORKSPACE}\MdePkg\Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  ${WORKSPACE}\MdePkg\Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  ${WORKSPACE}\MdePkg\Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  ${WORKSPACE}\MdePkg\Library/DxeCoreHobLib/DxeCoreHobLib.inf
  ${WORKSPACE}\MdePkg\Library/DxeHobLib/DxeHobLib.inf
  ${WORKSPACE}\MdePkg\Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  ${WORKSPACE}\MdePkg\Library/DxePcdLib/DxePcdLib.inf
  ${WORKSPACE}\MdePkg\Library/DxeServicesTableLib/DxeServicesTableLib.inf
  ${WORKSPACE}\MdePkg\Library/DxeSmbusLib/DxeSmbusLib.inf
  ${WORKSPACE}\MdePkg\Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  ${WORKSPACE}\MdePkg\Library/PeiHobLib/PeiHobLib.inf
  ${WORKSPACE}\MdePkg\Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ${WORKSPACE}\MdePkg\Library/PeimEntryPoint/PeimEntryPoint.inf
  ${WORKSPACE}\MdePkg\Library/PeiPcdLib/PeiPcdLib.inf
  ${WORKSPACE}\MdePkg\Library/PeiResourcePublicationLib/PeiResourcePublicationLib.inf
  ${WORKSPACE}\MdePkg\Library/PeiServicesLib/PeiServicesLib.inf
  ${WORKSPACE}\MdePkg\Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ${WORKSPACE}\MdePkg\Library/PeiSmbusLib/PeiSmbusLib.inf
  ${WORKSPACE}\MdePkg\Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf
  ${WORKSPACE}\MdePkg\Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  ${WORKSPACE}\MdePkg\Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  ${WORKSPACE}\MdePkg\Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  ${WORKSPACE}\MdePkg\Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  ${WORKSPACE}\MdePkg\Library/UefiDevicePathLib/UefiDevicePathLib.inf
  ${WORKSPACE}\MdePkg\Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
  ${WORKSPACE}\MdePkg\Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  ${WORKSPACE}\MdePkg\Library/UefiLib/UefiLib.inf
  ${WORKSPACE}\MdePkg\Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf


[Components.IA32]
  ${WORKSPACE}\MdePkg\Library/BaseMemoryLibSse2/BaseMemoryLibSse2.inf
  ${WORKSPACE}\MdePkg\Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf

[Components.X64]
  ${WORKSPACE}\MdePkg\Library/BaseMemoryLibSse2/BaseMemoryLibSse2.inf
  ${WORKSPACE}\MdePkg\Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf


