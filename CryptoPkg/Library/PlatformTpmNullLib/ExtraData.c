/** @file
  Endoresment Seed generation part of PlatformTpmLib to use TpmLib.

  To see the plat_XXX interface in TPM reference library, see:
    - https://github.com/TrustedComputingGroup/TPM/tree/main/TPMCmd/Platform/src

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiMm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__GetPlatformManufactureData

  This function allows the platform to provide a small amount of data to be
  stored as part of the TPM's PERSISTENT_DATA structure during manufacture.
  Of course the platform can store data separately as well,
  but this allows a simple platform implementation to store
  a few bytes of data without implementing a multi-layer storage system.
  This function is called on manufacture and CLEAR.
  The buffer will contain the last value provided
  to the Core library.

  @param[out]  PlatformPersistentData          Platform data.
  @param[in]   Size                            Size of PlatformPersistentData.

**/
VOID
EFIAPI
PlatformTpmLibGetPlatformManufactureData (
  UINT8   *PlatformPersistentData,
  UINT32  Size
  )
{
  /*
   * Using TCG Tpm library's implementation.
   * See TPMCmd/Platform/src/ExtraData.c
   */
  if ((PlatformPersistentData != NULL) && (Size != 0)) {
    SetMem (PlatformPersistentData, Size, 0xFF);
  }
}
