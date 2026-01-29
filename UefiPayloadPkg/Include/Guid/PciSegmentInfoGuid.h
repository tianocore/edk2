/** @file
  This file defines the hob structure for PCI Segment related information.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UPL_PCI_SEGMENT_INFO_GUID_H_
#define UPL_PCI_SEGMENT_INFO_GUID_H_

///
/// UPL Pcie Segment Information Hob GUID
///
extern EFI_GUID  gUplPciSegmentInfoHobGuid;

#pragma pack(1)
typedef struct {
  UINT16    SegmentNumber;              ///< Segment number.
  UINT64    BaseAddress;                ///< ECAM Base address.
} UPL_SEGMENT_INFO;

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  UINTN                               Count;
  UPL_SEGMENT_INFO                    SegmentInfo[0];
} UPL_PCI_SEGMENT_INFO_HOB;
#pragma pack()

#define UNIVERSAL_PAYLOAD_PCI_SEGMENT_INFO_REVISION  1

#endif
