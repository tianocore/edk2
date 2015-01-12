
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


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
