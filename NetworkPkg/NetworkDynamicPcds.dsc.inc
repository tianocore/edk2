## @file
# Network DSC include file for [PcdsDynamic*] section of all Architectures.
#
# This file can be included to the [PcdsDynamic*] section(s) of a platform DSC file
# by using "!include NetworkPkg/NetworkDynamicPcds.dsc.inc" to specify PCD settings
# according to the value of flags described in "NetworkDefines.dsc.inc".
#
# Copyright (c) 2024, Aleksandr Goncharov. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

#
# IPv4 and IPv6 PXE Boot support.
#
!if ($(NETWORK_ENABLE) == TRUE) AND ($(NETWORK_PXE_BOOT_ENABLE) == TRUE)
  gEfiNetworkPkgTokenSpaceGuid.PcdIPv4PXESupport|0x01
  gEfiNetworkPkgTokenSpaceGuid.PcdIPv6PXESupport|0x01
!endif
