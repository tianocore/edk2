/** @file                                                                           // [CODE_FIRST] 13357
  Private header for RedfishResourceDxe.                                            // [CODE_FIRST] 13357
                                                                                    // [CODE_FIRST] 13357
  Copyright (C) 2026 Jabil Inc. All rights reserved.<BR>                            // [CODE_FIRST] 13357
  SPDX-License-Identifier: BSD-2-Clause-Patent                                      // [CODE_FIRST] 13357
                                                                                    // [CODE_FIRST] 13357
**/// [CODE_FIRST] 13357
// [CODE_FIRST] 13357
#pragma once                                                                        // [CODE_FIRST] 13357
                                                                                    // [CODE_FIRST] 13357
#include <Uefi.h>                                                                   // [CODE_FIRST] 13357
#include <Library/PcdLib.h>                                                         // [CODE_FIRST] 13357
#include <Protocol/RedfishResourceProtocol.h>                                       // [CODE_FIRST] 13357
                                                                                    // [CODE_FIRST] 13357
//                                                                                  // [CODE_FIRST] 13357
// FixedPcdGet32 () is a build-time constant, so it can size a static array.        // [CODE_FIRST] 13357
//                                                                                  // [CODE_FIRST] 13357
#define REDFISH_RESOURCE_MAX_ENTRIES  FixedPcdGet32 (PcdRedfishResourceMaxEntries)  // [CODE_FIRST] 13357
                                                                                    // [CODE_FIRST] 13357
typedef struct {
  // [CODE_FIRST] 13357
  CHAR8    *TypeName;      // [CODE_FIRST] 13357
  CHAR8    *MajorVersion;  // [CODE_FIRST] 13357
  CHAR8    *MinorVersion;  // [CODE_FIRST] 13357
  CHAR8    *ErrataVersion; // [CODE_FIRST] 13357
  UINTN    DataSize;       // [CODE_FIRST] 13357
  VOID     *Data;          // Owned by caller - protocol only stores the pointer     // [CODE_FIRST] 13357
} RESOURCE_DATA_ENTRY;                                                              // [CODE_FIRST] 13357
                                                                                    // [CODE_FIRST] 13357
typedef struct {
  // [CODE_FIRST] 13357
  UINTN                              EntryCount;                                    // [CODE_FIRST] 13357
  RESOURCE_DATA_ENTRY                Entries[REDFISH_RESOURCE_MAX_ENTRIES];         // [CODE_FIRST] 13357
  EDKII_REDFISH_RESOURCE_PROTOCOL    Protocol;                                      // [CODE_FIRST] 13357
} REDFISH_RESOURCE_PRIVATE;                                                         // [CODE_FIRST] 13357
