## @file
# Standalone MM Platform.
#
# Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.<BR>
# Copyright (C) Microsoft Corporation<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = StandaloneMm
  PLATFORM_GUID                  = 9A4BBA60-B4F9-47C7-9258-3BD77CAE9322
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 0x00010011
  OUTPUT_DIRECTORY               = Build/StandaloneMm
  SUPPORTED_ARCHITECTURES        = AARCH64|X64|ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

  # LzmaF86
  DEFINE COMPRESSION_TOOL_GUID   = D42AE6BD-1352-4bfb-909A-CA72A6EAE889

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  ExtractGuidedSectionLib|StandaloneMmPkg/Library/StandaloneMmExtractGuidedSectionLib/StandaloneMmExtractGuidedSectionLib.inf
  FvLib|StandaloneMmPkg/Library/FvLib/FvLib.inf
  HobLib|StandaloneMmPkg/Library/StandaloneMmHobLib/StandaloneMmHobLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MemLib|StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf
  MemoryAllocationLib|StandaloneMmPkg/Library/StandaloneMmCoreMemoryAllocationLib/StandaloneMmCoreMemoryAllocationLib.inf
  MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
  VariableMmDependency|StandaloneMmPkg/Library/VariableMmDependency/VariableMmDependency.inf
  HobPrintLib|MdeModulePkg/Library/HobPrintLib/HobPrintLib.inf
  MmPlatformHobProducerLib|StandaloneMmPkg/Library/MmPlatformHobProducerLibNull/MmPlatformHobProducerLibNull.inf
  MmIplPlatformHookLib|StandaloneMmPkg/Library/MmIplPlatformHookLibNull/MmIplPlatformHookLibNull.inf
  ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

[LibraryClasses.common.PEIM]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  MmUnblockMemoryLib|MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf

[LibraryClasses.X64]
  StandaloneMmCoreEntryPoint|MdePkg/Library/StandaloneMmCoreEntryPoint/StandaloneMmCoreEntryPoint.inf

[LibraryClasses.AARCH64, LibraryClasses.ARM]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  StandaloneMmCoreEntryPoint|ArmPkg/Library/ArmStandaloneMmCoreEntryPoint/ArmStandaloneMmCoreEntryPoint.inf
  StandaloneMmMmuLib|ArmPkg/Library/StandaloneMmMmuLib/ArmMmuStandaloneMmLib.inf
  ArmSmcLib|MdePkg/Library/ArmSmcLib/ArmSmcLib.inf
  ArmSvcLib|MdePkg/Library/ArmSvcLib/ArmSvcLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  PeCoffExtraActionLib|StandaloneMmPkg/Library/StandaloneMmPeCoffExtraActionLib/StandaloneMmPeCoffExtraActionLib.inf
  ArmTransferListLib|ArmPkg/Library/ArmTransferListLib/ArmTransferListLib.inf

[LibraryClasses.common.MM_CORE_STANDALONE]
  HobLib|StandaloneMmPkg/Library/StandaloneMmCoreHobLib/StandaloneMmCoreHobLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf

[LibraryClasses.AARCH64.MM_CORE_STANDALONE, LibraryClasses.ARM.MM_CORE_STANDALONE]
  ArmFfaLib|MdeModulePkg/Library/ArmFfaLib/ArmFfaStandaloneMmCoreLib.inf

[LibraryClasses.common.MM_STANDALONE]
  MemoryAllocationLib|StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_DRIVER]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x800000CF
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0xff
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0f

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################
[Components.common]
  #
  # MM Core
  #
  StandaloneMmPkg/Core/StandaloneMmCore.inf
  StandaloneMmPkg/Library/FvLib/FvLib.inf
  StandaloneMmPkg/Library/StandaloneMmCoreHobLib/StandaloneMmCoreHobLib.inf
  StandaloneMmPkg/Library/StandaloneMmCoreMemoryAllocationLib/StandaloneMmCoreMemoryAllocationLib.inf
  StandaloneMmPkg/Library/StandaloneMmHobLib/StandaloneMmHobLib.inf
  StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf
  StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib.inf
  StandaloneMmPkg/Library/VariableMmDependency/VariableMmDependency.inf
  StandaloneMmPkg/Library/SmmLockBoxMmDependency/SmmLockBoxMmDependency.inf
  StandaloneMmPkg/Library/MmPlatformHobProducerLibNull/MmPlatformHobProducerLibNull.inf
  StandaloneMmPkg/Library/MmIplPlatformHookLibNull/MmIplPlatformHookLibNull.inf
  StandaloneMmPkg/Drivers/MmCommunicationDxe/MmCommunicationDxe.inf
  StandaloneMmPkg/Drivers/MmCommunicationNotifyDxe/MmCommunicationNotifyDxe.inf
  StandaloneMmPkg/Library/StandaloneMmExtractGuidedSectionLib/StandaloneMmExtractGuidedSectionLib.inf

[Components.AARCH64, Components.ARM]
  StandaloneMmPkg/Library/StandaloneMmPeCoffExtraActionLib/StandaloneMmPeCoffExtraActionLib.inf

[Components.X64]
  StandaloneMmPkg/Drivers/StandaloneMmIplPei/StandaloneMmIplPei.inf
  StandaloneMmPkg/Drivers/StandaloneMmIplPei/StandaloneMmIplDxe.inf

###################################################################################################
#
# BuildOptions Section - Define the module specific tool chain flags that should be used as
#                        the default flags for a module. These flags are appended to any
#                        standard flags that are defined by the build process. They can be
#                        applied for any modules or only those modules with the specific
#                        module style (EDK or EDKII) specified in [Components] section.
#
###################################################################################################
[BuildOptions.AARCH64]
GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000 -march=armv8-a+nofp -mstrict-align
GCC:*_*_*_CC_FLAGS = -mstrict-align

[BuildOptions.ARM]
GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000 -march=armv7-a
GCC:*_*_*_CC_FLAGS = -fno-stack-protector

[BuildOptions.X64]
  MSFT:*_*_*_DLINK_FLAGS = /ALIGN:4096
  GCC:*_GCC*_*_DLINK_FLAGS = -z common-page-size=0x1000
