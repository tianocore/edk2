/** @file
  Metadata Object Library.

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
#include "MetadataObj.h"

/** Array of METADATA_STATIC_INFO. */
STATIC METADATA_STATIC_INFO  MetadataStaticInfo[MetadataMaxId] = {
  // MetadataUidId
  {
    sizeof (METADATA_OBJ_UID),
  },
  // MetadataProximityDomainId
  {
    sizeof (METADATA_OBJ_PROXIMITY_DOMAIN),
  },
};

/** Validate the Metadata Root.

  @param[in]  Root  Root of the Metadata information.

  @retval TRUE  The Root is valid.
  @retval FALSE Otherwise.
**/
STATIC
BOOLEAN
MetadataRootIsValid (
  IN METADATA_ROOT_HANDLE  Root
  )
{
  METADATA_ROOT  *InRoot;

  InRoot = Root;
  if ((InRoot == NULL) ||
      (InRoot->NumEntries != MetadataMaxId) ||
      (InRoot->MetadataList == NULL))
  {
    return FALSE;
  }

  return TRUE;
}

/** Initialize the Metadata Root.

  @param[out]  Root  If success, Root of the Metadata information.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
MetadataInitializeHandle (
  OUT METADATA_ROOT_HANDLE  *Root
  )
{
  METADATA_ROOT  *OutRoot;
  METADATA_LIST  *MetadataList;
  UINT32         Index;

  if (Root == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  OutRoot = AllocateZeroPool (sizeof (*OutRoot));
  if (OutRoot == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MetadataList = AllocateZeroPool (MetadataMaxId * sizeof (*MetadataList));
  if (MetadataList == NULL) {
    FreePool (OutRoot);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < MetadataMaxId; Index++) {
    InitializeListHead (&MetadataList[Index].List);
  }

  OutRoot->NumEntries   = MetadataMaxId;
  OutRoot->MetadataList = MetadataList;
  *Root                 = OutRoot;
  return EFI_SUCCESS;
}

/** Free the Metadata Root.

  @param[in]  Root  Root of the Metadata information to free.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
MetadataFreeHandle (
  IN METADATA_ROOT_HANDLE  Root
  )
{
  METADATA_ROOT  *InRoot;

  InRoot = (METADATA_ROOT *)Root;

  if (InRoot == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (InRoot->MetadataList);
  FreePool (InRoot);
  return EFI_SUCCESS;
}

/** Allocate a METADATA_ENTRY.

  @param[in]  Id            METADATA_ID of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.
  @param[in]  Metadata      Metadata to associate to the (Id/Token) pair.
                            The data is copied.
  @param[in]  MetadataSize  Size of the input Metadata.
  @param[out] OutMdEntry    If success, contains the created METADATA_ENTRY.

  @retval EFI_SUCCESS             Success.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
AllocateMetadataEntry (
  IN  METADATA_ID      Id,
  IN  CM_OBJECT_TOKEN  Token,
  IN  VOID             *Metadata,
  IN  UINT32           MetadataSize,
  OUT METADATA_ENTRY   **OutMdEntry
  )
{
  METADATA_ENTRY  *Entry;

  ASSERT (Id < MetadataMaxId);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (Metadata != NULL);
  ASSERT (MetadataSize == MetadataStaticInfo[Id].ExpectedSize);
  ASSERT (OutMdEntry != NULL);

  Entry = AllocateZeroPool (sizeof (*Entry));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Id       = Id;
  Entry->Token    = Token;
  Entry->Metadata = AllocateCopyPool (MetadataSize, Metadata);
  if (Entry->Metadata == NULL) {
    FreePool (Entry);
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&Entry->List);

  *OutMdEntry = Entry;
  return EFI_SUCCESS;
}

/** Find a METADATA_ENTRY with a matching (Id/Token).

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.
  @param[out] OutMdEntry    If success, contains the METADATA_ENTRY with
                            a matching (Id/Token).

  @retval EFI_SUCCESS     Success.
  @retval EFI_NOT_FOUND   Not found.
**/
STATIC
EFI_STATUS
EFIAPI
FindMdEntry (
  IN  METADATA_ROOT    *Root,
  IN  METADATA_ID      Id,
  IN  CM_OBJECT_TOKEN  Token,
  OUT METADATA_ENTRY   **OutMdEntry
  )
{
  LIST_ENTRY      *ListHead;
  LIST_ENTRY      *Link;
  METADATA_ENTRY  *Entry;

  ASSERT (Id < MetadataMaxId);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (OutMdEntry != NULL);

  ListHead = &Root->MetadataList[Id].List;
  Link     = GetFirstNode (ListHead);

  while (Link != ListHead) {
    Entry = (METADATA_ENTRY *)Link;

    if (Entry->Token == Token) {
      break;
    }

    Link = GetNextNode (ListHead, Link);
  }

  // No matching token.
  if (Link == ListHead) {
    return EFI_NOT_FOUND;
  }

  *OutMdEntry = Entry;
  return EFI_SUCCESS;
}

/** Attach some Metadata to a (Id/Token) pair.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.
  @param[in]  Metadata      Metadata to associate to the (Id/Token) pair.
                            The data is copied.
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_ALREADY_STARTED   (Id/Token) pair is already present.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataAdd (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  CM_OBJECT_TOKEN       Token,
  IN  VOID                  *Metadata,
  IN  UINT32                MetadataSize
  )
{
  EFI_STATUS      Status;
  METADATA_ENTRY  *Entry;

  if (!MetadataRootIsValid (Root) ||
      (Id >= MetadataMaxId)       ||
      (Token == CM_NULL_TOKEN)    ||
      (Metadata == NULL)          ||
      (MetadataSize != MetadataStaticInfo[Id].ExpectedSize))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = FindMdEntry (Root, Id, Token, &Entry);
  if (Status == EFI_SUCCESS) {
    // The (Id/Token) pair already exists.
    return EFI_ALREADY_STARTED;
  } else if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    // Error other than not-found.
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // If not-found, create an new entry.
  Status = AllocateMetadataEntry (Id, Token, Metadata, MetadataSize, &Entry);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  InsertTailList (&((METADATA_ROOT *)Root)->MetadataList[Id].List, &Entry->List);
  return Status;
}

/** Remove a (Id/Token) pair and its associated Metadata.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID of the entry to remove.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataRemove (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  CM_OBJECT_TOKEN       Token
  )
{
  EFI_STATUS      Status;
  METADATA_ENTRY  *Entry;

  if (!MetadataRootIsValid (Root) ||
      (Id >= MetadataMaxId)       ||
      (Token == CM_NULL_TOKEN))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = FindMdEntry (Root, Id, Token, &Entry);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  RemoveEntryList (&Entry->List);
  return EFI_SUCCESS;
}

/** Get the Metadata associated to an (Id/Token).

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID of the entry to get.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.
  @param[out] Metadata      If success, contains the Metadata associated to the
                            input (Id/Token).
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataGet (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  CM_OBJECT_TOKEN       Token,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  )
{
  EFI_STATUS      Status;
  METADATA_ENTRY  *Entry;

  if (!MetadataRootIsValid (Root) ||
      (Id >= MetadataMaxId)       ||
      (Token == CM_NULL_TOKEN)    ||
      (Metadata == NULL)          ||
      (MetadataSize != MetadataStaticInfo[Id].ExpectedSize))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = FindMdEntry (Root, Id, Token, &Entry);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    // Error other than not-found.
    ASSERT_EFI_ERROR (Status);
    return Status;
  } else if (Status == EFI_NOT_FOUND) {
    return Status;
  }

  CopyMem (Metadata, Entry->Metadata, MetadataSize);
  return EFI_SUCCESS;
}

/** Iterate over the existing Metadata with the same Id.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID to iterate over.
  @param[in]  PrevHandle    On entry: Handle to the Metadata structure previously
                            iterated over.
                            On exit: Handle to the Metadata structure currently
                            iterated over.
  @param[out] Metadata      Metadata of the current Handle.
  @param[in]  MetadataSize  Size of the input Metadata.

  @return METADATA_HANDLE   The handle.
**/
METADATA_HANDLE
EFIAPI
MetadataIterate (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  METADATA_HANDLE       PrevHandle,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  )
{
  METADATA_ENTRY  *Entry;
  LIST_ENTRY      *ListHead;
  LIST_ENTRY      *Link;

  if (!MetadataRootIsValid (Root) ||
      (Id >= MetadataMaxId)       ||
      (Metadata == NULL)          ||
      (MetadataSize != MetadataStaticInfo[Id].ExpectedSize))
  {
    ASSERT (FALSE);
    return NULL;
  }

  ListHead = &((METADATA_ROOT *)Root)->MetadataList[Id].List;

  if (PrevHandle == NULL) {
    Link = GetFirstNode (ListHead);
  } else {
    Link = GetNextNode (ListHead, (LIST_ENTRY *)PrevHandle);
  }

  // End of the list.
  if (Link == ListHead) {
    Link = NULL;
  }

  Entry = (METADATA_ENTRY *)Link;

  if ((Entry != NULL) && (Entry->Metadata != NULL)) {
    CopyMem (Metadata, Entry->Metadata, MetadataStaticInfo[Id].ExpectedSize);
  }

  return (METADATA_HANDLE)Link;
}
