## @file
# Network DSC include file for [LibraryClasses*] section of all Architectures.
#
# This file can be included to the [LibraryClasses*] section(s) of a platform DSC file
# by using "!include NetworkPkg/NetworkLibs.dsc.inc" to specify the library instances
# of EDKII network library classes.
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

  DpcLib|NetworkPkg/Library/DxeDpcLib/DxeDpcLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|NetworkPkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|NetworkPkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  TcpIoLib|NetworkPkg/Library/DxeTcpIoLib/DxeTcpIoLib.inf
  # HttpLib and HttpIoLib are used for Http Boot and other
  # HTTP applications.
  HttpLib|NetworkPkg/Library/DxeHttpLib/DxeHttpLib.inf
  HttpIoLib|NetworkPkg/Library/DxeHttpIoLib/DxeHttpIoLib.inf
