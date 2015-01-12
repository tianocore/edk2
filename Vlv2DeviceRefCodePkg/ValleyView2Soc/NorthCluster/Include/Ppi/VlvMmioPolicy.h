
/*++

Copyright (c)  2010  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  VlvMmioPolicy.h

Abstract:

  Interface definition details between ValleyView platform drivers during PEI phase.

--*/

#ifndef _VLV_MMIO_POLICY_PPI_H_
#define _VLV_MMIO_POLICY_PPI_H_

#define VLV_MMIO_POLICY_PPI_GUID \
  { \
    0xE767BF7F, 0x4DB6, 0x5B34, 0x10, 0x11, 0x4F, 0xBE, 0x4C, 0xA7, 0xAF, 0xD2 \
  }

extern EFI_GUID gVlvMmioPolicyPpiGuid;


//
// MRC Platform Policiy PPI
//
typedef struct _VLV_MMIO_POLICY_PPI {
  UINT16                 MmioSize;
} VLV_MMIO_POLICY_PPI;

#pragma pack()

#endif // _VLV_MMIO_POLICY_PPI_H_
