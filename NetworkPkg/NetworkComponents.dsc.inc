## @file
# Network DSC include file for [Components*] section of all Architectures.
#
# This file can be included to the [Components*] section(s) of a platform DSC file
# by using "!include NetworkPkg/NetworkComponents.dsc.inc" to specify the INF files
# of EDKII network drivers according to the value of flags described in
# "NetworkDefines.dsc.inc".
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!if $(NETWORK_ENABLE) == TRUE
  MdeModulePkg/Universal/Network/DpcDxe/DpcDxe.inf

  !if $(NETWORK_SNP_ENABLE) == TRUE
    MdeModulePkg/Universal/Network/SnpDxe/SnpDxe.inf
  !endif

  !if $(NETWORK_VLAN_ENABLE) == TRUE
    MdeModulePkg/Universal/Network/VlanConfigDxe/VlanConfigDxe.inf
  !endif

  MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf

  !if $(NETWORK_IP4_ENABLE) == TRUE
    MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
    MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
    MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
    MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf
    MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
  !endif

  !if $(NETWORK_IP6_ENABLE) == TRUE
    NetworkPkg/Dhcp6Dxe/Dhcp6Dxe.inf
    NetworkPkg/Ip6Dxe/Ip6Dxe.inf
    NetworkPkg/Udp6Dxe/Udp6Dxe.inf
    NetworkPkg/Mtftp6Dxe/Mtftp6Dxe.inf
  !endif

  NetworkPkg/TcpDxe/TcpDxe.inf
  NetworkPkg/UefiPxeBcDxe/UefiPxeBcDxe.inf

  !if $(NETWORK_TLS_ENABLE) == TRUE
    NetworkPkg/TlsDxe/TlsDxe.inf
    NetworkPkg/TlsAuthConfigDxe/TlsAuthConfigDxe.inf
  !endif

  !if $(NETWORK_HTTP_BOOT_ENABLE) == TRUE
    NetworkPkg/DnsDxe/DnsDxe.inf
    NetworkPkg/HttpDxe/HttpDxe.inf
    NetworkPkg/HttpUtilitiesDxe/HttpUtilitiesDxe.inf
    NetworkPkg/HttpBootDxe/HttpBootDxe.inf
  !endif

  !if $(NETWORK_ISCSI_ENABLE) == TRUE
    NetworkPkg/IScsiDxe/IScsiDxe.inf
  !endif
!endif
