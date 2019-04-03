
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent


Module Name:

  VlvPeiInit.h

Abstract:

  Interface definition between ValleyView MRC and VlvInitPeim driver..

--*/

#ifndef _VLV_PEI_INIT_H_
#define _VLV_PEI_INIT_H_

//
// Define the VLV PEI Init PPI GUID
//
#define VLV_PEI_INIT_PPI_GUID \
  { \
    0x9ea8911, 0xbe0d, 0x4230, 0xa0, 0x3, 0xed, 0xc6, 0x93, 0xb4, 0x8e, 0x11 \
  }

//
// Extern the GUID for PPI users.
//
extern EFI_GUID     gVlvPeiInitPpiGuid;

#endif
