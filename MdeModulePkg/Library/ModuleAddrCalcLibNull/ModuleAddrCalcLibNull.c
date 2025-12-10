/** @file
  Calculate Module Offset

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>

/**
  Only record the EntryPoint now
  Consumed by ModuleOffsetCalculator only
**/
typedef struct _MODULE_OFFSET_HOB {
  UINTN    EntryPoint;
} MODULE_OFFSET_HOB;

/**
  Calculate Module Offset when Module Migration

  @param[in, out] ModuleOffset Offset before and after PeiCore migrate PEIM
  @param[in, out] ModulePositive Migrate to high address will be TRUE. Otherwise will be FALSE.

  Security haven't be considered yet.

  It support 32 and 64. So it can support 64BIT PEIM

  @retval EFI_SUCCESS         Indicate that first run. There are no valid ModuleOffset and ModulePositive.
                              Should ignore these.
  @retval EFI_ALREADY_STARTED Indicate the logical have done and have valid ModuleOffset and ModulePositive

  @return Status It can only be Success or AlreadyStarted. The reason is that PEIM can only run twice at most. First is
          Success and Second is AlreadyStarted.
**/
EFI_STATUS
ModuleOffsetCalculator (
  IN     EFI_PEI_FILE_HANDLE  FileHandle,
  IN     EFI_GUID             *CallerGuid,
  IN OUT UINTN                *ModuleOffset,
  IN OUT BOOLEAN              *ModulePositive
  )
{
  EFI_STATUS         Status;
  MODULE_OFFSET_HOB  *ModuleOffsetHob;

  Status = PeiServicesRegisterForShadow (FileHandle);

  if (Status == EFI_SUCCESS) {
    ModuleOffsetHob = BuildGuidHob (CallerGuid, sizeof (MODULE_OFFSET_HOB));

    if (ModuleOffsetHob != NULL) {
      ModuleOffsetHob->EntryPoint = (UINTN)_ModuleEntryPoint;
    } else {
      // This branch mean that fail to create HOB previously. HOB must be created successfully to calculate offset.
      ASSERT (ModuleOffsetHob != NULL);

      // Should never arrive here and unpredictable behavior may happened.
      return Status;
    }
  } else if (Status == EFI_ALREADY_STARTED) {
    ModuleOffsetHob = GetFirstGuidHob (CallerGuid);

    if (ModuleOffsetHob != NULL) {
      ModuleOffsetHob = GET_GUID_HOB_DATA (ModuleOffsetHob);
    } else {
      // This branch mean that fail to create HOB previously. HOB must be created successfully to calculate offset.
      ASSERT (ModuleOffsetHob != NULL);

      // Use 0 as return for safe.
      *ModulePositive = TRUE;
      *ModuleOffset   = 0;

      // Should never arrive here and unpredictable behavior may happened.
      return Status;
    }

    DEBUG ((DEBUG_INFO, "Address before and after Migration: 0x%X -> 0x%X\n", ModuleOffsetHob->EntryPoint, _ModuleEntryPoint));
  } else {
    // Status can only be Success or AlreadyStarted
    ASSERT (FALSE);

    // Should never arrive here and unpredictable behavior may happened.
    return Status;
  }

  if ((UINTN)_ModuleEntryPoint >= ModuleOffsetHob->EntryPoint) {
    *ModulePositive = TRUE;
    *ModuleOffset   = (UINTN)_ModuleEntryPoint - ModuleOffsetHob->EntryPoint;
  } else {
    *ModulePositive = FALSE;
    *ModuleOffset   = ModuleOffsetHob->EntryPoint - (UINTN)_ModuleEntryPoint;
  }

  return Status;
}
