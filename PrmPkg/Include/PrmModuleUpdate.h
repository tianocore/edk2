/** @file

  Definition for the Platform Runtime Mechanism (PRM) module update structures.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_MODULE_UPDATE_H_
#define PRM_MODULE_UPDATE_H_

#include <Uefi.h>

#define PRM_MODULE_UPDATE_LOCK_DESCRIPTOR_NAME        PrmModuleUpdateLock
#define PRM_MODULE_UPDATE_LOCK_DESCRIPTOR_SIGNATURE   SIGNATURE_64 ('P', 'R', 'M', '_', 'M', 'U', 'L', '_')
#define PRM_MODULE_UPDATE_LOCK_REVISION               0x0

#pragma pack(push, 1)

///
/// Maintains the PRM Module Update Lock state
///
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT8 Acquired          : 1;  ///< [0] - If 1 lock is acquired. If 0 lock is released.
    UINT8 Reserved          : 7;  ///< [7:1] - Reserved
  } Bits;
  ///
  /// All bit fields as an 8-bit value
  ///
  UINT8 Uint8;
} PRM_MODULE_UPDATE_LOCK;

typedef struct {
  UINT64                                Signature;
  UINT16                                Revision;
  PRM_MODULE_UPDATE_LOCK                Lock;
} PRM_MODULE_UPDATE_LOCK_DESCRIPTOR;

#pragma pack(pop)

#endif
