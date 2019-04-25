## @file
# Network DSC include file for [LibraryClasses*] section of all Architectures.
#
# This file can be included to the [LibraryClasses*] section(s) of a platform DSC file
# by using "!include NetworkPkg/NetworkLibs.dsc.inc" to specify the library instances
# of EDKII network library classes.
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  TcpIoLib|MdeModulePkg/Library/DxeTcpIoLib/DxeTcpIoLib.inf
  # HttpLib is used for Http Boot
  HttpLib|MdeModulePkg/Library/DxeHttpLib/DxeHttpLib.inf
