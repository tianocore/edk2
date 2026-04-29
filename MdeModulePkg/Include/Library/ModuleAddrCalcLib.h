/** @file
  Calculate Module Offset

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
  Calculate the module entry point offset after PEIM shadowing.

  @param[in, out] ModuleOffset    Absolute offset between the original PEIM
                                  entry point and the shadowed entry point.
  @param[in, out] ModulePositive  TRUE if the PEIM moved to a higher address.
                                  FALSE if it moved to a lower address.

  This helper supports PEIMs that may run twice: once from temporary storage and
  once after shadowing into permanent memory.

  This helper supports both 32-bit and 64-bit PEIMs.

  @retval EFI_SUCCESS          The PEIM is running before shadowing. Ignore the
                               returned offset and direction values.
  @retval EFI_ALREADY_STARTED  The PEIM is running after shadowing. ModuleOffset
                               and ModulePositive contain valid results.

  @return Status  The return value is expected to be either EFI_SUCCESS or
                  EFI_ALREADY_STARTED because a PEIM is shadowed at most once.
**/
EFI_STATUS
ModuleOffsetCalculator (
  IN     EFI_PEI_FILE_HANDLE  FileHandle,
  IN     EFI_GUID             *CallerGuid,
  IN OUT UINTN                *ModuleOffset,
  IN OUT BOOLEAN              *ModulePositive
  );
