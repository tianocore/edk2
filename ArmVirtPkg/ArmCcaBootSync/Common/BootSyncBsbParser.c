/** @file
  Boot Sync BSB parser.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>

#include "Include/BootSyncProtocolDefines.h"

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
  )
{
  UINT32               BytesRemining;
  UINT32               ElementCount;
  BOOT_SYNC_GUID_BLOB  *Elements;

  if ((BsbHdr == NULL) || (ElementGuid == NULL) || (BsbElement == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ElementCount  = BsbHdr->ElementCount;
  BytesRemining = BsbHdr->Header.Length;

  if (BytesRemining < sizeof (BOOT_SYNC_BSB_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }

  // Skip the Bsb Header
  BytesRemining -= sizeof (BOOT_SYNC_BSB_HEADER);
  Elements       = (BOOT_SYNC_GUID_BLOB *)(BsbHdr + 1);
  while ((ElementCount > 0) &&
         (BytesRemining >= sizeof (BOOT_SYNC_BSB_ELEMENT)))
  {
    if ((Elements->Length < sizeof (BOOT_SYNC_BSB_ELEMENT)) ||
        (Elements->Length > BytesRemining))
    {
      *BsbElement = NULL;
      return EFI_INVALID_PARAMETER;
    }

    if (CompareGuid (&Elements->Name, ElementGuid)) {
      *BsbElement = (BOOT_SYNC_BSB_ELEMENT *)Elements;
      return EFI_SUCCESS;
    }

    BytesRemining -= Elements->Length;
    Elements       = (BOOT_SYNC_GUID_BLOB *)((UINT8 *)Elements + Elements->Length);
    ElementCount--;
  }

  *BsbElement = NULL;
  return EFI_NOT_FOUND;
}
