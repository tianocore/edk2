## @file
# Network DSC include file for [PcdsFixed*] section of all Architectures.
#
# This file can be included to the [PcdsFixed*] section(s) of a platform DSC file
# by using "!include NetworkPkg/NetworkFixedPcds.dsc.inc" to specify PCD settings
# according to the value of flags described in "NetworkDefines.dsc.inc".
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!if ($(NETWORK_ENABLE) == TRUE) AND ($(NETWORK_ALLOW_HTTP_CONNECTIONS) == TRUE)
  gEfiNetworkPkgTokenSpaceGuid.PcdAllowHttpConnections|TRUE
!endif
