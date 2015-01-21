//
//

/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  SmbusPolicy.h

Abstract:

  Smbus Policy PPI as defined in EFI 2.0

--*/
#ifndef _PEI_SMBUS_POLICY_PPI_H
#define _PEI_SMBUS_POLICY_PPI_H

#define PEI_SMBUS_POLICY_PPI_GUID \
  { \
    0x63b6e435, 0x32bc, 0x49c6, 0x81, 0xbd, 0xb7, 0xa1, 0xa0, 0xfe, 0x1a, 0x6c \
  }

typedef struct _PEI_SMBUS_POLICY_PPI PEI_SMBUS_POLICY_PPI;

typedef struct _PEI_SMBUS_POLICY_PPI {
  UINTN   BaseAddress;
  UINT32  PciAddress;
  UINT8   NumRsvdAddress;
  UINT8   *RsvdAddress;
} PEI_SMBUS_POLICY_PPI;

extern EFI_GUID gPeiSmbusPolicyPpiGuid;

#endif
