/** @file
  Convert between GUID and UUID RFC4122 format.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BaseLibInternals.h"

/**
  Convert GUID to RFC4122 UUID format.

  For example, If there is GUID named
  "378daedc-f06b-4446-8314-40ab933c87a3",

  GUID is saved in memory like:
     dc ae 8d 37
     6b f0 46 44
     83 14 40 ab
     93 3c 87 a3

  However, UUID should be saved like:
     37 8d ae dc
     f0 6b 44 46
     83 14 40 ab
     93 3c 87 a3

  Other software components (i.e. linux-kernel) uses RFC4122 UUID format.

  @param [in] Guid            GUID
  @param [out] Uuid           Uuid

**/
VOID
EFIAPI
ConvertGuidToUuid (
  IN   GUID    *Guid,
  OUT  UINT64  *Uuid
  )
{
  UINT32  *Data32;
  UINT16  *Data16;

  CopyGuid ((GUID *)Uuid, Guid);
  Data32    = (UINT32 *)Uuid;
  Data32[0] = SwapBytes32 (Data32[0]);
  Data16    = (UINT16 *)&Data32[1];
  Data16[0] = SwapBytes16 (Data16[0]);
  Data16[1] = SwapBytes16 (Data16[1]);
}

/**
  Convert UUID to GUID to RFC4122 UUID format, which is the inverse of
  ConvertEfiGuidToUuid.

  @param [in] Uuid            Uuid
  @param [out] Guid           GUID

**/
VOID
EFIAPI
ConvertUuidToGuid (
  IN   UINT64  *Uuid,
  OUT  GUID    *Guid
  )
{
  // The conversion is symmetric, so we can use the same function.
  // The only difference is the order of the parameters.
  ConvertGuidToUuid (
    (GUID *)Uuid,
    (UINT64 *)Guid
    );
}
