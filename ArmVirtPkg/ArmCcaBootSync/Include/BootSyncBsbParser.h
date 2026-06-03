/** @file
  Boot Sync BSB parser.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BOOT_SYNC_BSB_PARSER_H_
#define BOOT_SYNC_BSB_PARSER_H_

/**
  Search the BSB for an element that matches the specified GUID and return
  a pointer to the element if found.

  @param[in]  BsbHdr        Pointer to the BSB header.
  @param[in]  ElementGuid   GUID of the element to search.
  @param[out] BsbElement    Returns a pointer to the BSB element within the BSB,
                            if an element matching the ElementGuid is found
                            otherwise NULL.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_NOT_FOUND           BSB element matching the specified
                                  GUID was not found.
  @retval EFI_SUCCESS             BSB element found.
**/
EFI_STATUS
EFIAPI
BsbGetElement (
  IN  BOOT_SYNC_BSB_HEADER   *BsbHdr,
  IN  EFI_GUID               *ElementGuid,
  OUT BOOT_SYNC_BSB_ELEMENT  **BsbElement
  );

#endif // BOOT_SYNC_BSB_PARSER_H_
