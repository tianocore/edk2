/** @file
  Definition for structure & defines exported by Delayed Dispatch PPI

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DELAYED_DISPATCH_H_
#define DELAYED_DISPATCH_H_

// Delayed Dispatch table GUID
#define EFI_DELAYED_DISPATCH_TABLE_GUID  {\
  0x4b733449, 0x8eff, 0x488c, { 0x92, 0x1a, 0x15, 0x4a, 0xda, 0x25, 0x18, 0x07 } \
  }

//
// Maximal number of Delayed Dispatch entries supported
//
#define DELAYED_DISPATCH_MAX_ENTRIES  8

//
// Internal structure for delayed dispatch entries.
// Packing the structures here to save space as they will be stored as HOBs.
//
#pragma pack (push, 1)

typedef struct {
  EFI_GUID                         DelayedGroupId;
  UINT64                           Context;
  EFI_DELAYED_DISPATCH_FUNCTION    Function;
  UINT64                           DispatchTime;
  UINT32                           MicrosecondDelay;
} DELAYED_DISPATCH_ENTRY;

typedef struct {
  UINT32                    Count;
  UINT32                    DispCount;
  DELAYED_DISPATCH_ENTRY    Entry[DELAYED_DISPATCH_MAX_ENTRIES];
} DELAYED_DISPATCH_TABLE;

#pragma pack (pop)

extern EFI_GUID  gEfiDelayedDispatchTableGuid;

#endif
