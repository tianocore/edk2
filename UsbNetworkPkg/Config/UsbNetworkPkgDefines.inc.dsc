## @file
# Global switches enable/disable project features.
#
# Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
!if "IA32" in $(ARCH) && "X64" in $(ARCH)
  DEFINE PEI=IA32
  DEFINE DXE=X64
!else
  DEFINE PEI=COMMON
  DEFINE DXE=COMMON
!endif

[Packages]
  UsbNetworkPkg/UsbNetworkPkg.dec

[PcdsFeatureFlag]
  gUsbNetworkPkgTokenSpaceGuid.UsbCdcEcmSupport|FALSE
  gUsbNetworkPkgTokenSpaceGuid.UsbCdcNcmSupport|FALSE
  gUsbNetworkPkgTokenSpaceGuid.UsbRndisSupport|TRUE
