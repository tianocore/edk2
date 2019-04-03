## @file
# Provides driver and definitions to build fsp in EDKII bios.
#
# Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = IntelFspPkg
  PLATFORM_GUID                  = 29C6791F-9EBC-4470-A126-2BB47431AE5E
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/IntelFspPkg
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

  # Dummy - test build only
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugDeviceLib|IntelFspPkg/Library/BaseDebugDeviceLibNull/BaseDebugDeviceLibNull.inf

  # FSP override
  DebugLib|IntelFspPkg/Library/BaseFspDebugLibSerialPort/BaseFspDebugLibSerialPort.inf

  # FSP specific lib
  CacheAsRamLib|IntelFspPkg/Library/BaseCacheAsRamLibNull/BaseCacheAsRamLibNull.inf
  CacheLib|IntelFspPkg/Library/BaseCacheLib/BaseCacheLib.inf
  FspCommonLib|IntelFspPkg/Library/BaseFspCommonLib/BaseFspCommonLib.inf
  FspPlatformLib|IntelFspPkg/Library/BaseFspPlatformLib/BaseFspPlatformLib.inf
  FspSwitchStackLib|IntelFspPkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf
  FspSecPlatformLib|IntelFspPkg/Library/SecFspSecPlatformLibNull/SecFspSecPlatformLibNull.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf

[Components]
  IntelFspPkg/Library/BaseCacheAsRamLibNull/BaseCacheAsRamLibNull.inf
  IntelFspPkg/Library/BaseCacheLib/BaseCacheLib.inf
  IntelFspPkg/Library/BaseFspCommonLib/BaseFspCommonLib.inf
  IntelFspPkg/Library/BaseFspDebugLibSerialPort/BaseFspDebugLibSerialPort.inf
  IntelFspPkg/Library/BaseFspPlatformLib/BaseFspPlatformLib.inf
  IntelFspPkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf

  IntelFspPkg/FspSecCore/FspSecCore.inf
  IntelFspPkg/FspDxeIpl/FspDxeIpl.inf

[PcdsFixedAtBuild.common]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80080046
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
