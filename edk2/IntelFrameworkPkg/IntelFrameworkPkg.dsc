#/** @file
# Intel Framework Package Reference Implementations
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
  PLATFORM_NAME                  = IntelFramework
  PLATFORM_GUID                  = E76EB141-6EDB-43f3-A455-EF24A79673DD
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = $(WORKSPACE)/Build/IntelFramework
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT



################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild.common]
  PcdDebugPropertyMask|gEfiMdePkgTokenSpaceGuid|0x0f
  PcdReportStatusCodePropertyMask|gEfiMdePkgTokenSpaceGuid|0x06
  PcdDebugClearMemoryValue|gEfiMdePkgTokenSpaceGuid|0xAF
  PcdUefiLibMaxPrintBufferSize|gEfiMdePkgTokenSpaceGuid|320
[PcdsPatchableInModule.common]
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000


################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################


[Components.common]
  $(WORKSPACE)/IntelFrameworkPkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  $(WORKSPACE)/IntelFrameworkPkg/Library/DxeIoLibCpuIo/DxeIoLibCpuIo.inf
  $(WORKSPACE)/IntelFrameworkPkg/Library/UefiLibFramework/UefiLib.inf
  $(WORKSPACE)/IntelFrameworkPkg/Library/DxeSmmDriverEntryPoint/DxeSmmDriverEntryPoint.inf
  $(WORKSPACE)/IntelFrameworkPkg/Library/HiiLibFramework/HiiLib.inf
  $(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  $(WORKSPACE)/IntelFrameworkPkg/Library/IfrSupportLibFramework/IfrSupportLib.inf
  $(WORKSPACE)/IntelFrameworkPkg/Library/PeiSmbusLibSmbusPpi/PeiSmbusLibSmbusPpi.inf
