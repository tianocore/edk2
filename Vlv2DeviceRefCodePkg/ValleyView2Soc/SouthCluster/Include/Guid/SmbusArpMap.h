//
//
/*++

Copyright (c)  2009  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  SmbusArpMap.h

Abstract:

  GUID for use in describing SMBus devices that were ARPed during PEI.

--*/
#ifndef _EFI_SMBUS_ARP_MAP_GUID_H_
#define _EFI_SMBUS_ARP_MAP_GUID_H_

#define EFI_SMBUS_ARP_MAP_GUID \
  { \
    0x707be83e, 0x0bf6, 0x40a5, 0xbe, 0x64, 0x34, 0xc0, 0x3a, 0xa0, 0xb8, 0xe2 \
  }

extern EFI_GUID gEfiSmbusArpMapGuid;

#endif
