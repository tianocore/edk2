/** @file

  Definitions for the Platform Runtime Mechanism (PRM) OS Services.

  Note: OS Services have been removed from POR. This file has been reduced to just debug print
        OS Service for use during PRM enabling.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_OS_SERVICES_H_
#define PRM_OS_SERVICES_H_

#include <Uefi.h>

typedef struct _PRM_OS_SERVICES PRM_OS_SERVICES;

//
// PRM OS Services function signatures
//
typedef
VOID
(EFIAPI *PRM_OS_SERVICE_DEBUG_PRINT) (
  IN  CONST CHAR8           *String
  );

#pragma pack(push, 1)

//
// PRM OS Services table
//
struct _PRM_OS_SERVICES {
    // Structure information
    UINT16                                MajorVersion;
    UINT16                                MinorVersion;

    // OS Services
    PRM_OS_SERVICE_DEBUG_PRINT            DebugPrint;
};

#pragma pack(pop)

#endif
