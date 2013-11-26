## @file
# EDK Compatibility Package Build File
#
#
# Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
#
#    This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = EdkCompatibilityPkg
  PLATFORM_GUID                  = 6CBD2F63-BCF2-42b0-937E-869C67D2F734
  PLATFORM_VERSION               = 0.92
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/EdkCompatibilityPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|IPF|EBC
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
DEFINE MSFT_MACRO                = /D EFI_SPECIFICATION_VERSION=0x00020000 /D PI_SPECIFICATION_VERSION=0x00009000 /D TIANO_RELEASE_VERSION=0x00080006 /D PCD_EDKII_GLUE_PciExpressBaseAddress=0xE0000000 /D EFI_DEBUG
DEFINE INTEL_MACRO                = /D EFI_SPECIFICATION_VERSION=0x00020000 /D PI_SPECIFICATION_VERSION=0x00009000 /D TIANO_RELEASE_VERSION=0x00080006 /D PCD_EDKII_GLUE_PciExpressBaseAddress=0xE0000000 /D EFI_DEBUG
DEFINE GCC_MACRO                 = -DEFI_SPECIFICATION_VERSION=0x00020000 -DPI_SPECIFICATION_VERSION=0x00009000 -DTIANO_RELEASE_VERSION=0x00080006 -DPCD_EDKII_GLUE_PciExpressBaseAddress=0xE0000000 -DEFI_DEBUG

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf  
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  LanguageLib|EdkCompatibilityPkg/Compatibility/Library/UefiLanguageLib/UefiLanguageLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf

[LibraryClasses.common.PEIM]
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf

[LibraryClasses.common.DXE_DRIVER,LibraryClasses.common.DXE_RUNTIME_DRIVER]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[LibraryClasses.IA32.DXE_SMM_DRIVER,LibraryClasses.X64.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf

[BuildOptions.Common.EDK]
  GCC:*_*_IA32_CC_FLAGS     = -DEFI32 $(GCC_MACRO)
  GCC:*_*_IA32_ASM_FLAGS    =
  GCC:*_*_IA32_VFRPP_FLAGS  = -DEFI32 $(GCC_MACRO)
  GCC:*_*_IA32_APP_FLAGS    = -DEFI32 $(GCC_MACRO)
  GCC:*_*_IA32_PP_FLAGS     = -DEFI32 $(GCC_MACRO)

  GCC:*_*_X64_CC_FLAGS      = -DEFIX64 $(GCC_MACRO)
  GCC:*_*_X64_ASM_FLAGS     =
  GCC:*_*_X64_VFRPP_FLAGS   = -DEFIX64 $(GCC_MACRO)
  GCC:*_*_X64_APP_FLAGS     = -DEFIX64 $(GCC_MACRO)
  GCC:*_*_X64_PP_FLAGS      = -DEFIX64 $(GCC_MACRO)

  GCC:*_*_IPF_CC_FLAGS      = -DEFI64 $(GCC_MACRO)
  GCC:*_*_IPF_ASM_FLAGS     =
  GCC:*_*_IPF_VFRPP_FLAGS   = -DEFI64 $(GCC_MACRO)
  GCC:*_*_IPF_APP_FLAGS     = -DEFI64 $(GCC_MACRO)
  GCC:*_*_IPF_PP_FLAGS      = -DEFI64 $(GCC_MACRO)


  INTEL:*_*_IA32_CC_FLAGS    = /D EFI32 $(MSFT_MACRO)
  INTEL:*_*_IA32_ASM_FLAGS   = /DEFI32
  INTEL:*_*_IA32_VFRPP_FLAGS = /D EFI32 $(MSFT_MACRO)
  INTEL:*_*_IA32_APP_FLAGS   = /D EFI32 $(MSFT_MACRO)
  INTEL:*_*_IA32_PP_FLAGS    = /D EFI32 $(MSFT_MACRO)

  INTEL:*_*_EBC_CC_FLAGS    = /D EFI32 $(MSFT_MACRO)
  INTEL:*_*_EBC_PP_FLAGS    = /D EFI32 $(MSFT_MACRO)

  INTEL:*_*_X64_CC_FLAGS     = /D EFIX64 $(MSFT_MACRO)
  INTEL:*_*_X64_ASM_FLAGS    = /DEFIX64
  INTEL:*_*_X64_VFRPP_FLAGS  = /D EFIX64 $(MSFT_MACRO)
  INTEL:*_*_X64_APP_FLAGS    = /D EFIX64 $(MSFT_MACRO)
  INTEL:*_*_X64_PP_FLAGS     = /D EFIX64 $(MSFT_MACRO)

  INTEL:*_*_IPF_CC_FLAGS     = /D EFI64 $(MSFT_MACRO)
  INTEL:*_*_IPF_ASM_FLAGS    =
  INTEL:*_*_IPF_VFRPP_FLAGS  = /D EFI64 $(MSFT_MACRO)
  INTEL:*_*_IPF_APP_FLAGS    = /D EFI64 $(MSFT_MACRO)
  INTEL:*_*_IPF_PP_FLAGS     = /D EFI64 $(MSFT_MACRO)


  MSFT:*_*_IA32_CC_FLAGS    = /D EFI32 $(MSFT_MACRO)
  MSFT:*_*_IA32_ASM_FLAGS   = /DEFI32
  MSFT:*_*_IA32_VFRPP_FLAGS = /D EFI32 $(MSFT_MACRO)
  MSFT:*_*_IA32_APP_FLAGS   = /D EFI32 $(MSFT_MACRO)
  MSFT:*_*_IA32_PP_FLAGS    = /D EFI32 $(MSFT_MACRO)

  MSFT:*_*_X64_CC_FLAGS     = /D EFIX64 $(MSFT_MACRO)
  MSFT:*_*_X64_ASM_FLAGS    = /DEFIX64
  MSFT:*_*_X64_VFRPP_FLAGS  = /D EFIX64 $(MSFT_MACRO)
  MSFT:*_*_X64_APP_FLAGS    = /D EFIX64 $(MSFT_MACRO)
  MSFT:*_*_X64_PP_FLAGS     = /D EFIX64 $(MSFT_MACRO)

  MSFT:*_*_IPF_CC_FLAGS     = /Od /Os /D EFI64 $(MSFT_MACRO)
  MSFT:*_*_IPF_ASM_FLAGS    =
  MSFT:*_*_IPF_VFRPP_FLAGS  = /D EFI64 $(MSFT_MACRO)
  MSFT:*_*_IPF_APP_FLAGS    = /D EFI64 $(MSFT_MACRO)
  MSFT:*_*_IPF_PP_FLAGS     = /D EFI64 $(MSFT_MACRO)

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

[Components]

# All Libraries Instances listed in Alphabetic Order
  EdkCompatibilityPkg/Foundation/Core/Dxe/ArchProtocol/ArchProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Efi/Guid/EfiGuidLib.inf
  EdkCompatibilityPkg/Foundation/Efi/Protocol/EfiProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Guid/EdkFrameworkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Ppi/EdkFrameworkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Protocol/EdkFrameworkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Guid/EdkGuidLib.inf

  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/CustomizedDecompress/CustomizedDecompress.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiDriverLib/EfiDriverLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiIfrSupportLib/EfiIfrSupportLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiScriptLib/EfiScriptLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiUiLib/EfiUiLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Graphics/Graphics.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/GraphicsLite/Graphics.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Hob/HobLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Print/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/PrintLite/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/UefiEfiIfrSupportLib/UefiEfiIfrSupportLib.inf
  EdkCompatibilityPkg/Foundation/Library/Smm/SmmScriptLib/SmmScriptLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseLib/BaseLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseMemoryLib/BaseMemoryLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciExpressLib/BasePciExpressLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciLibCf8/BasePciLibCf8.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePeCoffLib/BasePeCoffLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePrintLib/BasePrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseTimerLibLocalApic/BaseTimerLibLocalApic.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeHobLib/DxeHobLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeIoLibCpuIo/DxeIoLibCpuIo.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf

  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/SmmRuntimeDxeReportStatusCodeLib/SmmRuntimeDxeReportStatusCodeLib.inf  
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeSmbusLib/DxeSmbusLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/EdkDxeRuntimeDriverLib/EdkDxeRuntimeDriverLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/HiiLib/HiiLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiDxePostCodeLibReportStatusCode/PeiDxePostCodeLibReportStatusCode.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiHobLib/PeiHobLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiResourcePublicationLib/PeiResourcePublicationLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesLib/PeiServicesLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiSmbusLib/PeiSmbusLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiDriverModelLib/UefiDriverModelLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiLib/UefiLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  EdkCompatibilityPkg/Foundation/Library/EfiCommonLib/EfiCommonLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/Hob/PeiHobLib.inf
  EdkCompatibilityPkg/Foundation/Library/RuntimeDxe/EfiRuntimeLib/EfiRuntimeLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/Thunk16/Thunk16Lib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Ppi/EdkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Protocol/EdkProtocolLib.inf
  EdkCompatibilityPkg/Sample/Platform/Generic/MonoStatusCode/Library/Pei/MemoryStatusCode/MemoryStatusCode.inf
  EdkCompatibilityPkg/Sample/Platform/Generic/RuntimeDxe/StatusCode/Lib/BsDataHubStatusCode/BsDataHubStatusCode.inf
  EdkCompatibilityPkg/Sample/Platform/Generic/RuntimeDxe/StatusCode/Lib/BsSerialStatusCode/BsSerialStatusCode.inf
  EdkCompatibilityPkg/Sample/Platform/Generic/RuntimeDxe/StatusCode/Lib/RtLedStatusCode/RtLedStatusCode.inf
  EdkCompatibilityPkg/Sample/Platform/Generic/RuntimeDxe/StatusCode/Lib/RtMemoryStatusCode/RtMemoryStatusCode.inf
  EdkCompatibilityPkg/Sample/Platform/Generic/RuntimeDxe/StatusCode/Lib/RtPlatformStatusCode/Nt32/RtPlatformStatusCode.inf
  EdkCompatibilityPkg/Sample/Platform/Generic/RuntimeDxe/StatusCode/Lib/RtPort80StatusCode/RtPort80StatusCode.inf
  EdkCompatibilityPkg/Sample/Platform/Nt32/Ppi/EdkNt32PpiLib.inf
  EdkCompatibilityPkg/Sample/Platform/Nt32/Protocol/EdkNt32ProtocolLib.inf

  #
  # Modules in Compatibility Directory
  #
  EdkCompatibilityPkg/Compatibility/DeviceIoOnPciRootBridgeIoThunk/DeviceIoOnPciRootBridgeIoThunk.inf
  EdkCompatibilityPkg/Compatibility/FrameworkHiiOnUefiHiiThunk/FrameworkHiiOnUefiHiiThunk.inf
  EdkCompatibilityPkg/Compatibility/FvFileLoaderOnLoadFileThunk/FvFileLoaderOnLoadFileThunk.inf
  EdkCompatibilityPkg/Compatibility/FvOnFv2Thunk/FvOnFv2Thunk.inf
  EdkCompatibilityPkg/Compatibility/Fv2OnFvThunk/Fv2OnFvThunk.inf
  EdkCompatibilityPkg/Compatibility/PciCfg2OnPciCfgThunk/PciCfg2OnPciCfgThunk.inf
  EdkCompatibilityPkg/Compatibility/PciCfgOnPciCfg2Thunk/PciCfgOnPciCfg2Thunk.inf
  EdkCompatibilityPkg/Compatibility/ReadOnlyVariable2OnReadOnlyVariableThunk/ReadOnlyVariable2OnReadOnlyVariableThunk.inf
  EdkCompatibilityPkg/Compatibility/ReadOnlyVariableOnReadOnlyVariable2Thunk/ReadOnlyVariableOnReadOnlyVariable2Thunk.inf
  EdkCompatibilityPkg/Compatibility/Uc2OnUcThunk/Uc2OnUcThunk.inf
  EdkCompatibilityPkg/Compatibility/UcOnUc2Thunk/UcOnUc2Thunk.inf
  EdkCompatibilityPkg/Compatibility/PrintThunk/PrintThunk.inf
  EdkCompatibilityPkg/Compatibility/LegacyRegion2OnLegacyRegionThunk/LegacyRegion2OnLegacyRegionThunk.inf
  EdkCompatibilityPkg/Compatibility/PiSmbiosRecordOnDataHubSmbiosRecordThunk/PiSmbiosRecordOnDataHubSmbiosRecordThunk.inf
  EdkCompatibilityPkg/Compatibility/CpuIo2OnCpuIoThunk/CpuIo2OnCpuIoThunk.inf

  #
  # User needs to turn on the compatibility switches for VFRC and EDK II build tool for Framework HII modules 
  # following the example shown below: 
  #
  #  $SomePackage/FrameworkHiiModule/FrameworkHiiModule.inf {
  #    <BuildOptions>
  #    *_*_*_VFR_FLAGS = -c # for VFR files in the module
  #    *_*_*_BUILD_FLAGS = -c # for UNI files in the module
  # }


[Components.IA32,Components.X64,Components.IPF]
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/PeiLib/PeiLib_Edk2.inf
  
[Components.IA32,Components.X64]
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxePerformanceLib/DxePerformanceLib.inf # Use IA32/X64 specific AsmReadTsc (). 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiPerformanceLib/PeiPerformanceLib.inf # Use IA32/X64 specific AsmReadTsc ().
  EdkCompatibilityPkg/Compatibility/AcpiVariableHobOnSmramReserveHobThunk/AcpiVariableHobOnSmramReserveHobThunk.inf
  EdkCompatibilityPkg/Compatibility/BootScriptThunkHelper/BootScriptThunkHelper.inf
  EdkCompatibilityPkg/Compatibility/MpServicesOnFrameworkMpServicesThunk/MpServicesOnFrameworkMpServicesThunk.inf
  EdkCompatibilityPkg/Compatibility/SmmBaseOnSmmBase2Thunk/SmmBaseOnSmmBase2Thunk.inf
  EdkCompatibilityPkg/Compatibility/SmmBaseHelper/SmmBaseHelper.inf
  EdkCompatibilityPkg/Compatibility/SmmAccess2OnSmmAccessThunk/SmmAccess2OnSmmAccessThunk.inf
  EdkCompatibilityPkg/Compatibility/SmmControl2OnSmmControlThunk/SmmControl2OnSmmControlThunk.inf
  EdkCompatibilityPkg/Compatibility/PiSmmStatusCodeOnFrameworkSmmStatusCodeThunk/PiSmmStatusCodeOnFrameworkSmmStatusCodeThunk.inf
  EdkCompatibilityPkg/Compatibility/FrameworkSmmStatusCodeOnPiSmmStatusCodeThunk/FrameworkSmmStatusCodeOnPiSmmStatusCodeThunk.inf
  EdkCompatibilityPkg/Compatibility/BootScriptSaveOnS3SaveStateThunk/BootScriptSaveOnS3SaveStateThunk.inf
  EdkCompatibilityPkg/Compatibility/DxeSmmReadyToLockOnExitPmAuthThunk/DxeSmmReadyToLockOnExitPmAuthThunk.inf

[Components.IPF]
  EdkCompatibilityPkg/Foundation/Cpu/Itanium/CpuIa64Lib/CpuIA64Lib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/EdkDxeSalLib/EdkDxeSalLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLibKr1/PeiServicesTablePointerLibKr1.inf

[Libraries]
  #
  # Libraries common to PEI and DXE
  #
  EdkCompatibilityPkg/Foundation/Efi/Guid/EfiGuidLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Guid/EdkFrameworkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Guid/EdkGuidLib.inf
  EdkCompatibilityPkg/Foundation/Library/EfiCommonLib/EfiCommonLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Cpu/Itanium/CpuIa64Lib/CpuIA64Lib.inf
  EdkCompatibilityPkg/Foundation/Library/CompilerStub/CompilerStubLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/CustomizedDecompress/CustomizedDecompress.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Hob/HobLib.inf
  #
  # PEI libraries
  #
  EdkCompatibilityPkg/Foundation/Framework/Ppi/EdkFrameworkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Ppi/EdkPpiLib.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/PeiLib/PeiLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/Hob/PeiHobLib.inf
  #
  # DXE libraries
  #
  EdkCompatibilityPkg/Foundation/Core/Dxe/ArchProtocol/ArchProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Efi/Protocol/EfiProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Framework/Protocol/EdkFrameworkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Protocol/EdkProtocolLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiDriverLib/EfiDriverLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/RuntimeDxe/EfiRuntimeLib/EfiRuntimeLib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/Graphics/Graphics.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiIfrSupportLib/EfiIfrSupportLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/UefiEfiIfrSupportLib/UefiEfiIfrSupportLib.inf   
  EdkCompatibilityPkg/Foundation/Library/Dxe/Print/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiScriptLib/EfiScriptLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/EfiUiLib/EfiUiLib.inf
  #
  # Print/Graphics Library consume SetupBrowser Print Protocol
  #
  EdkCompatibilityPkg/Foundation/Library/Dxe/PrintLite/PrintLib.inf
  EdkCompatibilityPkg/Foundation/Library/Dxe/GraphicsLite/Graphics.inf
  #
  # GlueLib
  #
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseLib/BaseLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseMemoryLib/BaseMemoryLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePrintLib/BasePrintLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseDebugLibNull/BaseDebugLibNull.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciCf8Lib/BasePciCf8Lib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciExpressLib/BasePciExpressLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciLibCf8/BasePciLibCf8.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BasePeCoffLib/BasePeCoffLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseTimerLibLocalApic/BaseTimerLibLocalApic.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiDxePostCodeLibReportStatusCode/PeiDxePostCodeLibReportStatusCode.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesLib/PeiServicesLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiHobLib/PeiHobLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiResourcePublicationLib/PeiResourcePublicationLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiSmbusLib/PeiSmbusLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeIoLibCpuIo/DxeIoLibCpuIo.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiLib/UefiLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeHobLib/DxeHobLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/SmmRuntimeDxeReportStatusCodeLib/SmmRuntimeDxeReportStatusCodeLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/HiiLib/HiiLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLibKr1/PeiServicesTablePointerLibKr1.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiDevicePathLib/UefiDevicePathLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/UefiDriverModelLib/UefiDriverModelLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeServicesTableLib/DxeServicesTableLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/EdkDxeSalLib/EdkDxeSalLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/EdkDxeRuntimeDriverLib/EdkDxeRuntimeDriverLib.inf 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxeSmbusLib/DxeSmbusLib.inf 

  EdkCompatibilityPkg/Foundation/Library/Thunk16/Thunk16Lib_Edk2.inf

[Libraries.IA32,Libraries.X64,Libraries.IPF]
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  EdkCompatibilityPkg/Foundation/Library/Pei/PeiLib/PeiLib_Edk2.inf
  
[Libraries.IA32,Libraries.X64]
  EdkCompatibilityPkg/Foundation/Cpu/Pentium/CpuIA32Lib/CpuIA32Lib_Edk2.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/DxePerformanceLib/DxePerformanceLib.inf # Use IA32/X64 specific AsmReadTsc (). 
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiPerformanceLib/PeiPerformanceLib.inf # Use IA32/X64 specific AsmReadTsc ().

[Libraries.IPF]
  EdkCompatibilityPkg/Foundation/Cpu/Itanium/CpuIa64Lib/CpuIA64Lib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/EdkDxeSalLib/EdkDxeSalLib.inf
  EdkCompatibilityPkg/Foundation/Library/EdkIIGlueLib/Library/PeiServicesTablePointerLibKr1/PeiServicesTablePointerLibKr1.inf

