/** @file
  Metadata Proximity Domain handlers.

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <ConfigurationManagerObject.h>

#include <Library/MetadataObjLib.h>
#include "MetadataHandler.h"

/** Uid Entry

  One entry is allocated for each HID/CID/EISAID. Each entry
  contains a counter used for the generation of UID values.
**/
typedef struct MetadataUidEntry {
  LIST_ENTRY    List;

  /// _HID or _CID of the device (NULL-terminated string).
  /// This provides a mean to uniquely identify a device type.
  /// If not populated, EisaId must be set.
  CHAR8         NameId[9];

  /// EisaId of the device.
  /// This provides a mean to uniquely identify a device type.
  /// If not populated, NameId must be set.
  UINT32        EisaId;

  /// Current Id used for the generation of a HID/CID/EisaId.
  UINT32        CurrId;
} METADATA_UID_ENTRY;

// List of METADATA_UID_ENTRY
STATIC LIST_ENTRY  mUidList = INITIALIZE_LIST_HEAD_VARIABLE (mUidList);

/** Allocate a METADATA_UID_ENTRY.

  @param [in]  Metadata   Metadata containing the NameId/EisaId to use.
  @param [out] OutEntry   Allocated Entry.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
AllocateUidEntry (
  IN   METADATA_OBJ_UID   *Metadata,
  OUT METADATA_UID_ENTRY  **OutEntry
  )
{
  METADATA_UID_ENTRY  *Entry;

  if ((Metadata == NULL) || (OutEntry == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Entry = AllocateZeroPool (sizeof (METADATA_UID_ENTRY));
  if (Entry == NULL) {
    ASSERT (Entry != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&Entry->List);
  Entry->CurrId = 0;

  if (Metadata->NameId[0] != 0) {
    AsciiStrCpyS (Entry->NameId, sizeof (Metadata->NameId), Metadata->NameId);
  } else {
    Entry->EisaId = Metadata->EisaId;
  }

  *OutEntry = Entry;
  return EFI_SUCCESS;
}

/** Find a matching METADATA_UID_ENTRY.
    If no Entry is found, allocate one.

  @param [in]  Metadata   Metadata containing the NameId/EisaId to search.
  @param [out] OutEntry   Matching or allocated Entry.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FindEntry (
  IN   METADATA_OBJ_UID   *Metadata,
  OUT METADATA_UID_ENTRY  **OutEntry
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Link;
  METADATA_UID_ENTRY  *Entry;

  if ((Metadata == NULL) || (OutEntry == NULL)) {
    ASSERT (Metadata != NULL);
    ASSERT (OutEntry != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Link = GetNextNode (&mUidList, &mUidList);

  while (Link != &mUidList) {
    Entry = (METADATA_UID_ENTRY *)Link;

    if (Entry->NameId[0] != 0) {
      if (AsciiStrnCmp (Entry->NameId, Metadata->NameId, sizeof (Metadata->NameId)) == 0) {
        break;
      }
    } else if (Metadata->EisaId != 0) {
      if (Entry->EisaId == Metadata->EisaId) {
        break;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "MetadatUid: Empty NameId and EisaId\n"));
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Link = GetNextNode (&mUidList, Link);
  } // while

  // No matching METADATA_UID_ENTRY found.
  if (Link == &mUidList) {
    Status = AllocateUidEntry (Metadata, &Entry);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    InsertTailList (&mUidList, &Entry->List);
  }

  *OutEntry = Entry;
  return EFI_SUCCESS;
}

/** Query the MetadataObjLib for metadata matching the input (Type/Token).
    If the metadata exists, return it.
    Otherwise:
    - Generate a new metadata object
    - Add it to the MetadataObjLib
    - return it

  @param[in]       Root          Root of the Metadata information.
  @param[in]       Type            METADATA_TYPE of the entry to generate.
  @param[in]       Token         Token uniquely identifying an entry among other
                                 objects with the input METADATA_TYPE.
  @param[in]       Context       Optional context to use during the Metadata generation.
  @param[in, out]  Metadata      On input, can contain METADATA_TYPE-specific information.
                                 On output and if success, contains the generated
                                 Metadata object.
  @param[in]       MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
MetadataGenerateUid (
  IN      METADATA_ROOT_HANDLE  Root,
  IN      METADATA_TYPE         Type,
  IN      CM_OBJECT_TOKEN       Token,
  IN      VOID                  *Context,
  IN OUT  VOID                  *Metadata,
  IN      UINT32                MetadataSize
  )
{
  EFI_STATUS          Status;
  METADATA_OBJ_UID    *Uid;
  METADATA_UID_ENTRY  *Entry;

  if ((Type != MetadataTypeUid) ||
      (Token == CM_NULL_TOKEN)  ||
      (Metadata == NULL))
  {
    ASSERT (Type == MetadataTypeUid);
    ASSERT (Token != CM_NULL_TOKEN);
    ASSERT (Metadata != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = MetadataGet (Root, Type, Token, Metadata, MetadataSize);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  } else if (Status == EFI_SUCCESS) {
    // Metadata for this (Type/Token) already exists.
    return EFI_SUCCESS;
  }

  // Generate new Metadata (i.e. Status == EFI_NOT_FOUND).

  // Get the Current Type for this HID/CID/EISAID
  Status = FindEntry (Metadata, &Entry);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Uid      = (METADATA_OBJ_UID *)Metadata;
  Uid->Uid = Entry->CurrId++;

  Status = MetadataAdd (Root, Type, Token, Metadata, MetadataSize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return EFI_SUCCESS;
}

/** Validate the Metadata.

  @param[in]  Root    Root of the Metadata information.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
MetadataValidateUid (
  IN  METADATA_ROOT_HANDLE  Root
  )
{
  METADATA_HANDLE   Handle0;
  METADATA_HANDLE   Handle1;
  METADATA_OBJ_UID  Metadata0;
  METADATA_OBJ_UID  Metadata1;

  Handle0 = NULL;

  while (TRUE) {
    Handle0 = MetadataIterate (
                Root,
                MetadataTypeUid,
                Handle0,
                &Metadata0,
                sizeof (METADATA_OBJ_UID)
                );
    if (Handle0 == NULL) {
      break;
    }

    // Loop starting from Handle0
    Handle1 = Handle0;

    while (TRUE) {
      Handle1 = MetadataIterate (
                  Root,
                  MetadataTypeUid,
                  Handle1,
                  &Metadata1,
                  sizeof (METADATA_OBJ_UID)
                  );

      if (Handle1 == NULL) {
        break;
      }

      if (CompareMem (&Metadata0, &Metadata1, sizeof (METADATA_OBJ_UID)) == 0) {
        DEBUG ((
          DEBUG_ERROR,
          "Metadata: Uid: Same Type: EisaId=%d NameId=%a Uid=%d \n",
          Metadata0.EisaId,
          Metadata0.NameId,
          Metadata0.Uid
          ));
        ASSERT (0);
      }
    }
  }

  return EFI_SUCCESS;
}
