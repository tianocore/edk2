## @file
# Global switches enable/disable project features.
#
# Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
!if "IA32" in $(ARCH) && "X64" in $(ARCH)
  DEFINE PEI=3DIA32
  DEFINE DXE=3DX64
!else
  DEFINE PEI=3DCOMMON
  DEFINE DXE=3DCOMMON
!endif

[Packages]
  UsbNetworkPkg/UsbNetworkPkg.dec

[PcdsFeatureFlag]
  gUsbNetworkPkgTokenSpaceGuid.UsbCdcEcmSupport|FALSE
  gUsbNetworkPkgTokenSpaceGuid.UsbCdcNcmSupport|FALSE
  gUsbNetworkPkgTokenSpaceGuid.UsbRndisSupport|TRUE
