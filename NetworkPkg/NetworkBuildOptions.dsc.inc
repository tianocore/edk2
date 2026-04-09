## @file
# Network DSC include file for [BuildOptions] sections of all Architectures.
#
# This file can be included in the [BuildOptions*] section(s) of a platform DSC file
# by using "!include NetworkPkg/NetworkBuildOptions.dsc.inc", to specify the C language
# feature test macros (eg., API deprecation macros) according to the flags described
# in "NetworkDefines.dsc.inc".
#
# Supported tool chain families: "GCC", "INTEL", "MSFT".
#
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!if $(NETWORK_ISCSI_ENABLE) == TRUE && $(NETWORK_ISCSI_MD5_ENABLE) == TRUE
  MSFT:*_*_*_CC_FLAGS = /D ENABLE_MD5_DEPRECATED_INTERFACES
  INTEL:*_*_*_CC_FLAGS = /D ENABLE_MD5_DEPRECATED_INTERFACES
  GCC:*_*_*_CC_FLAGS = -D ENABLE_MD5_DEPRECATED_INTERFACES
!endif
