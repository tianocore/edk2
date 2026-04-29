/** @file
  Calculate Module Offset

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

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
  );
