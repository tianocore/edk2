## @file
# Provides driver and definitions to build fsp.
#
# Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = IntelFsp2Pkg
  PLATFORM_GUID                  = 55CA3D18-831B-469A-A1C3-7AE719EB6A97
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/IntelFsp2Pkg
  SUPPORTED_ARCHITECTURES        = IA32
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf

  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugDeviceLib|IntelFsp2Pkg/Library/BaseDebugDeviceLibNull/BaseDebugDeviceLibNull.inf

  # FSP override
  DebugLib|IntelFsp2Pkg/Library/BaseFspDebugLibSerialPort/BaseFspDebugLibSerialPort.inf

  # FSP specific lib
  CacheAsRamLib|IntelFsp2Pkg/Library/BaseCacheAsRamLibNull/BaseCacheAsRamLibNull.inf
  CacheLib|IntelFsp2Pkg/Library/BaseCacheLib/BaseCacheLib.inf
  FspCommonLib|IntelFsp2Pkg/Library/BaseFspCommonLib/BaseFspCommonLib.inf
  FspPlatformLib|IntelFsp2Pkg/Library/BaseFspPlatformLib/BaseFspPlatformLib.inf
  FspSwitchStackLib|IntelFsp2Pkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf
  FspSecPlatformLib|IntelFsp2Pkg/Library/SecFspSecPlatformLibNull/SecFspSecPlatformLibNull.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf

[Components]
  IntelFsp2Pkg/Library/BaseCacheAsRamLibNull/BaseCacheAsRamLibNull.inf
  IntelFsp2Pkg/Library/BaseCacheLib/BaseCacheLib.inf
  IntelFsp2Pkg/Library/BaseFspCommonLib/BaseFspCommonLib.inf
  IntelFsp2Pkg/Library/BaseFspDebugLibSerialPort/BaseFspDebugLibSerialPort.inf
  IntelFsp2Pkg/Library/BaseFspPlatformLib/BaseFspPlatformLib.inf
  IntelFsp2Pkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf
  IntelFsp2Pkg/Library/BaseDebugDeviceLibNull/BaseDebugDeviceLibNull.inf
  IntelFsp2Pkg/Library/SecFspSecPlatformLibNull/SecFspSecPlatformLibNull.inf

  IntelFsp2Pkg/FspSecCore/FspSecCoreT.inf
  IntelFsp2Pkg/FspSecCore/FspSecCoreM.inf
  IntelFsp2Pkg/FspSecCore/FspSecCoreS.inf
  IntelFsp2Pkg/FspNotifyPhase/FspNotifyPhasePeim.inf

[PcdsFixedAtBuild.common]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80080046
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
