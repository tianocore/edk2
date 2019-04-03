//
//

/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



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

struct _PEI_SMBUS_POLICY_PPI {
  UINTN   BaseAddress;
  UINT32  PciAddress;
  UINT8   NumRsvdAddress;
  UINT8   *RsvdAddress;
};

extern EFI_GUID gPeiSmbusPolicyPpiGuid;

#endif
