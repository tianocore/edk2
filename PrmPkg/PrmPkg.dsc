## @file
# Build description file for PrmPkg
#
# Copyright (C) Microsoft Corporation
# Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = Prm
  PLATFORM_GUID                  = C29BB610-84F9-448D-A7DD-5A04C5A54F52
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  DEFINE  PLATFORM_PACKAGE       = $(PLATFORM_NAME)Pkg

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  #
  # EDK II Packages
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/BaseMemoryAllocationLibNull/BaseMemoryAllocationLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

# StackCheckLib is not linked for SEC modules by default, this package can link it against its SEC modules
[LibraryClasses.common.SEC]
  NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf

[LibraryClasses.IA32, LibraryClasses.X64]
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  #
  # EDK II Packages
  #
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

  #
  # PRM Package
  #
  PrmContextBufferLib|$(PLATFORM_PACKAGE)/Library/DxePrmContextBufferLib/DxePrmContextBufferLib.inf
  PrmModuleDiscoveryLib|$(PLATFORM_PACKAGE)/Library/DxePrmModuleDiscoveryLib/DxePrmModuleDiscoveryLib.inf
  PrmPeCoffLib|$(PLATFORM_PACKAGE)/Library/DxePrmPeCoffLib/DxePrmPeCoffLib.inf


[LibraryClasses.common.UEFI_APPLICATION]
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

################################################################################
#
# Pcd Section - List of PCD entries modified by this package
#
################################################################################

[PcdsFixedAtBuild.common]
  gPrmPkgTokenSpaceGuid.PcdPrmInfoPrintHandlerExecutionTime|FALSE

###################################################################################################
#
# Components Section - List of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image file.
#
###################################################################################################

[Components]
  #
  # PRM Libraries
  #
  $(PLATFORM_PACKAGE)/Library/DxePrmContextBufferLib/DxePrmContextBufferLib.inf
  $(PLATFORM_PACKAGE)/Samples/PrmSampleAcpiParameterBufferModule/Library/DxeAcpiParameterBufferModuleConfigLib/DxeAcpiParameterBufferModuleConfigLib.inf
  $(PLATFORM_PACKAGE)/Samples/PrmSampleContextBufferModule/Library/DxeContextBufferModuleConfigLib/DxeContextBufferModuleConfigLib.inf
  $(PLATFORM_PACKAGE)/Samples/PrmSampleHardwareAccessModule/Library/DxeHardwareAccessModuleConfigLib/DxeHardwareAccessModuleConfigLib.inf

  #
  # PRM Module Discovery Library
  #
  $(PLATFORM_PACKAGE)/Library/DxePrmModuleDiscoveryLib/DxePrmModuleDiscoveryLib.inf

  #
  # PRM PE/COFF Library
  #
  $(PLATFORM_PACKAGE)/Library/DxePrmPeCoffLib/DxePrmPeCoffLib.inf

  #
  # PRM Configuration Driver
  #
  $(PLATFORM_PACKAGE)/PrmConfigDxe/PrmConfigDxe.inf {
    <LibraryClasses>
      NULL|$(PLATFORM_PACKAGE)/Samples/PrmSampleAcpiParameterBufferModule/Library/DxeAcpiParameterBufferModuleConfigLib/DxeAcpiParameterBufferModuleConfigLib.inf
      NULL|$(PLATFORM_PACKAGE)/Samples/PrmSampleContextBufferModule/Library/DxeContextBufferModuleConfigLib/DxeContextBufferModuleConfigLib.inf
      NULL|$(PLATFORM_PACKAGE)/Samples/PrmSampleHardwareAccessModule/Library/DxeHardwareAccessModuleConfigLib/DxeHardwareAccessModuleConfigLib.inf
  }

  #
  # PRM Module Loader Driver
  #
  $(PLATFORM_PACKAGE)/PrmLoaderDxe/PrmLoaderDxe.inf

  #
  # PRM SSDT Installation Driver
  #
  $(PLATFORM_PACKAGE)/PrmSsdtInstallDxe/PrmSsdtInstallDxe.inf

  #
  # PRM Sample Modules
  #
  $(PLATFORM_PACKAGE)/Samples/PrmSampleAcpiParameterBufferModule/PrmSampleAcpiParameterBufferModule.inf
  $(PLATFORM_PACKAGE)/Samples/PrmSampleContextBufferModule/PrmSampleContextBufferModule.inf

  #
  # PRM Information UEFI Application
  #
  $(PLATFORM_PACKAGE)/Application/PrmInfo/PrmInfo.inf

[Components.IA32, Components.X64]
  #
  # PRM Sample Modules for IA32 and X64
  #
  $(PLATFORM_PACKAGE)/Samples/PrmSampleHardwareAccessModule/PrmSampleHardwareAccessModule.inf

[BuildOptions]
# Force deprecated interfaces off
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
