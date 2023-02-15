## @file
# List of Core Components.
#
# Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

  UsbNetworkPkg/NetworkCommon/NetworkCommon.inf

!if gUsbNetworkPkgTokenSpaceGuid.UsbCdcEcmSupport
  UsbNetworkPkg/UsbCdcEcm/UsbCdcEcm.inf
!endif

!if gUsbNetworkPkgTokenSpaceGuid.UsbCdcNcmSupport
  UsbNetworkPkg/UsbCdcNcm/UsbCdcNcm.inf
!endif

!if gUsbNetworkPkgTokenSpaceGuid.UsbRndisSupport
  UsbNetworkPkg/UsbRndis/UsbRndis.inf
!endif
