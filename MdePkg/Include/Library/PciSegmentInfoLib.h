/** @file
  Provides services to return segment information on a platform with multiple PCI segments.

  This library is consumed by PciSegmentLib to support multiple segment PCI configuration access.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PCI_SEGMENT_INFO_LIB__
#define __PCI_SEGMENT_INFO_LIB__

typedef struct {
  UINT16               SegmentNumber;   ///< Segment number.
  UINT64               BaseAddress;     ///< ECAM Base address.
  UINT8                StartBusNumber;  ///< Start BUS number, for verifying the PCI Segment address.
  UINT8                EndBusNumber;    ///< End BUS number, for verifying the PCI Segment address.
} PCI_SEGMENT_INFO;

/**
  Return an array of PCI_SEGMENT_INFO holding the segment information.

  Note: The returned array/buffer is owned by callee.

  @param  Count  Return the count of segments.

  @retval A callee owned array holding the segment information.
**/
PCI_SEGMENT_INFO *
GetPciSegmentInfo (
  UINTN  *Count
  );

#endif
