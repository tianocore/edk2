#/** @file
# EFI/PI MdePkg Package
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

[Defines]
  PLATFORM_NAME                  = Mde
  PLATFORM_GUID                  = 082F8BFC-0455-4859-AE3C-ECD64FB81642
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/Mde
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT


[PcdsFeatureFlag.common]
  PcdComponentNameDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnosticsDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdComponentName2Disable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnostics2Disable|gEfiMdePkgTokenSpaceGuid|FALSE

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

[PcdsFixedAtBuild.IPF]
  PcdIoBlockBaseAddressForIpf|gEfiMdePkgTokenSpaceGuid|0x0ffffc000000

[Components.common]
  MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MdePkg/Library/BaseLib/BaseLib.inf
  MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  MdePkg/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  MdePkg/Library/BasePrintLib/BasePrintLib.inf
  MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  MdePkg/Library/PeiDxePostCodeLibReportStatusCode/PeiDxePostCodeLibReportStatusCode.inf
  MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  MdePkg/Library/PeiMemoryLib/PeiMemoryLib.inf
  MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  MdePkg/Library/PeiResourcePublicationLib/PeiResourcePublicationLib.inf
  MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  MdePkg/Library/PeiSmbusLibSmbus2Ppi/PeiSmbusLibSmbus2Ppi.inf
  MdePkg/Library/SerialPortLibNull/SerialPortLibNull.inf
  MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
  MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  MdePkg/Library/UefiLib/UefiLib.inf
  MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  MdePkg/Library/DxeMemoryLib/DxeMemoryLib.inf
  MdePkg/Library/BaseCustomDecompressLibNull/BaseCustomDecompressLibNull.inf
  MdePkg/Library/DxeDebugLibSerialPort/DxeDebugLibSerialPort.inf
  MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[Components.IA32]
  MdePkg/Library/BaseMemoryLibSse2/BaseMemoryLibSse2.inf
  MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  MdePkg/Library/BaseMemoryLibMmx/BaseMemoryLibMmx.inf
  MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf
  MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  MdePkg/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf

[Components.X64]
  MdePkg/Library/BaseMemoryLibSse2/BaseMemoryLibSse2.inf
  MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  MdePkg/Library/BaseMemoryLibMmx/BaseMemoryLibMmx.inf
  MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf
  MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  MdePkg/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf

[Components.IPF]
  MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf
  MdePkg/Library/PeiServicesTablePointerLibKr7/PeiServicesTablePointerLibKr7.inf
