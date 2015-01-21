//
//
/*++

Copyright (c)  2009  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



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
