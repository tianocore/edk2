## @file
# Network DSC include file for Platform DSC
#
# This file includes all required information to enable Network features.
# It can be included to a platform DSC file by using "!include NetworkPkg/Network.dsc.inc".
#
# This file defines one build flag PLATFORMX64_ENABLE to support
# IA32 PEI and X64 DXE platform. Its default value is FALSE.
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
!include NetworkPkg/NetworkDefines.dsc.inc

!ifndef PLATFORMX64_ENABLE
  #
  # PLATFORMX64_ENABLE is set to TRUE when PEI is IA32 and DXE is X64 platform
  #
  DEFINE PLATFORMX64_ENABLE = FALSE
!endif

[PcdsFixedAtBuild]
!include NetworkPkg/NetworkPcds.dsc.inc

[LibraryClasses]
!include NetworkPkg/NetworkLibs.dsc.inc

[BuildOptions]
!include NetworkPkg/NetworkBuildOptions.dsc.inc

!if $(PLATFORMX64_ENABLE) == TRUE
[Components.X64]
!include NetworkPkg/NetworkComponents.dsc.inc

!else
[Components.IA32, Components.X64, Components.ARM, Components.AARCH64, Components.RISCV64]
!include NetworkPkg/NetworkComponents.dsc.inc

!endif
