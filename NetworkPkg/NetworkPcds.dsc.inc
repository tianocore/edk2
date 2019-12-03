## @file
# Network DSC include file for [Pcds*] section of all Architectures.
#
# This file can be included to the [Pcds*] section(s) of a platform DSC file
# by using "!include NetworkPkg/NetworkPcds.dsc.inc" to specify PCD settings
# according to the value of flags described in "NetworkDefines.dsc.inc".
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!if $(NETWORK_ALLOW_HTTP_CONNECTIONS) == TRUE
  gEfiNetworkPkgTokenSpaceGuid.PcdAllowHttpConnections|TRUE
!endif
