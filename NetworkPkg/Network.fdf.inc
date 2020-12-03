## @file
# Network FDF include file for All Architectures.
#
# This file can be included to a platform FDF by using
# "!include NetworkPkg/Network.fdf.inc" to add EDKII network stack drivers
# according to the value of flags described in "NetworkPkg/Network.dsc.inc".
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#

!if $(NETWORK_ENABLE) == TRUE
  INF  NetworkPkg/DpcDxe/DpcDxe.inf

  !if $(NETWORK_SNP_ENABLE) == TRUE
    INF  NetworkPkg/SnpDxe/SnpDxe.inf
  !endif

  !if $(NETWORK_VLAN_ENABLE) == TRUE
    INF  NetworkPkg/VlanConfigDxe/VlanConfigDxe.inf
  !endif

  INF  NetworkPkg/MnpDxe/MnpDxe.inf

  !if $(NETWORK_IP4_ENABLE) == TRUE
    INF  NetworkPkg/ArpDxe/ArpDxe.inf
    INF  NetworkPkg/Dhcp4Dxe/Dhcp4Dxe.inf
    INF  NetworkPkg/Ip4Dxe/Ip4Dxe.inf
    INF  NetworkPkg/Udp4Dxe/Udp4Dxe.inf
    INF  NetworkPkg/Mtftp4Dxe/Mtftp4Dxe.inf
  !endif

  !if $(NETWORK_IP6_ENABLE) == TRUE
    INF  NetworkPkg/Dhcp6Dxe/Dhcp6Dxe.inf
    INF  NetworkPkg/Ip6Dxe/Ip6Dxe.inf
    INF  NetworkPkg/Udp6Dxe/Udp6Dxe.inf
    INF  NetworkPkg/Mtftp6Dxe/Mtftp6Dxe.inf
  !endif

  INF  NetworkPkg/TcpDxe/TcpDxe.inf
  INF  NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf

  !if $(NETWORK_TLS_ENABLE) == TRUE
    INF  NetworkPkg/TlsDxe/TlsDxe.inf
    INF  NetworkPkg/TlsAuthConfigDxe/TlsAuthConfigDxe.inf
  !endif

  !if ($(NETWORK_HTTP_BOOT_ENABLE) == TRUE) OR ($(NETWORK_HTTP_ENABLE) == TRUE)
    INF  NetworkPkg/DnsDxe/DnsDxe.inf
    INF  NetworkPkg/HttpDxe/HttpDxe.inf
    INF  NetworkPkg/HttpUtilitiesDxe/HttpUtilitiesDxe.inf
  !endif

  !if $(NETWORK_HTTP_BOOT_ENABLE) == TRUE
    INF  NetworkPkg/HttpBootDxe/HttpBootDxe.inf
  !endif

  !if $(NETWORK_ISCSI_ENABLE) == TRUE
    INF  NetworkPkg/IScsiDxe/IScsiDxe.inf
  !endif

!endif
