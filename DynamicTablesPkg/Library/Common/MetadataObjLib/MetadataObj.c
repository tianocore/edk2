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

#include "MetadataObj.h"

/** Array of METADATA_STATIC_INFO. */
STATIC METADATA_STATIC_INFO  mMetadataStaticInfo[MetadataTypeMax] = {
  // MetadataTypeUid
  {
    sizeof (METADATA_OBJ_UID),
  },
  // MetadataTypeProximityDomain
  {
    sizeof (METADATA_OBJ_PROXIMITY_DOMAIN),
  },
};

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
  UINT32         Index;

  if (Root == NULL) {
    ASSERT (Root != NULL);
    return EFI_INVALID_PARAMETER;
  }

  OutRoot = AllocateZeroPool (sizeof (*OutRoot));
  if (OutRoot == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < MetadataTypeMax; Index++) {
    InitializeListHead (&OutRoot->MetadataList[Index].List);
  }

  *Root = OutRoot;
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
  if (Root == NULL) {
    ASSERT (Root != NULL);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (Root);
  return EFI_SUCCESS;
}

/** Allocate a METADATA_ENTRY.

  @param[in]  Type          METADATA_TYPE of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.
  @param[in]  Metadata      Metadata to associate to the (Type/Token) pair.
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
  IN  METADATA_TYPE    Type,
  IN  CM_OBJECT_TOKEN  Token,
  IN  VOID             *Metadata,
  IN  UINT32           MetadataSize,
  OUT METADATA_ENTRY   **OutMdEntry
  )
{
  METADATA_ENTRY  *Entry;

  ASSERT (Type < MetadataTypeMax);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (Metadata != NULL);
  ASSERT (MetadataSize == mMetadataStaticInfo[Type].ExpectedSize);
  ASSERT (OutMdEntry != NULL);

  Entry = AllocateZeroPool (sizeof (*Entry));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Type     = Type;
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

/** Find a METADATA_ENTRY with a matching (Type/Token).

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.
  @param[out] OutMdEntry    If success, contains the METADATA_ENTRY with
                            a matching (Type/Token).

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
**/
STATIC
EFI_STATUS
EFIAPI
MetadataFindEntry (
  IN  METADATA_ROOT    *Root,
  IN  METADATA_TYPE    Type,
  IN  CM_OBJECT_TOKEN  Token,
  OUT METADATA_ENTRY   **OutMdEntry
  )
{
  LIST_ENTRY      *ListHead;
  LIST_ENTRY      *Link;
  METADATA_ENTRY  *Entry;

  if ((Type >= MetadataTypeMax) ||
      (Token == CM_NULL_TOKEN) ||
      (OutMdEntry == NULL))
  {
    ASSERT (Type < MetadataTypeMax);
    ASSERT (Token != CM_NULL_TOKEN);
    ASSERT (OutMdEntry != NULL);
    return EFI_INVALID_PARAMETER;
  }

  ListHead = &Root->MetadataList[Type].List;
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

/** Attach some Metadata to a (Type/Token) pair.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.
  @param[in]  Metadata      Metadata to associate to the (Type/Token) pair.
                            The data is copied.
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_ALREADY_STARTED   (Type/Token) pair is already present.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataAdd (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  CM_OBJECT_TOKEN       Token,
  IN  VOID                  *Metadata,
  IN  UINT32                MetadataSize
  )
{
  EFI_STATUS      Status;
  METADATA_ENTRY  *Entry;

  if ((Root == NULL)              ||
      (Type >= MetadataTypeMax)   ||
      (Token == CM_NULL_TOKEN)    ||
      (Metadata == NULL)          ||
      (MetadataSize != mMetadataStaticInfo[Type].ExpectedSize))
  {
    ASSERT (Root != NULL);
    ASSERT (Type < MetadataTypeMax);
    ASSERT (Token != CM_NULL_TOKEN);
    ASSERT (Metadata != NULL);
    ASSERT (MetadataSize == mMetadataStaticInfo[Type].ExpectedSize);
    return EFI_INVALID_PARAMETER;
  }

  Status = MetadataFindEntry (Root, Type, Token, &Entry);
  if (Status == EFI_SUCCESS) {
    // The (Type/Token) pair already exists.
    return EFI_ALREADY_STARTED;
  } else if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    // Error other than not-found.
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // If not-found, create an new entry.
  Status = AllocateMetadataEntry (Type, Token, Metadata, MetadataSize, &Entry);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  InsertTailList (&((METADATA_ROOT *)Root)->MetadataList[Type].List, &Entry->List);
  return Status;
}

/** Remove a (Type/Token) pair and its associated Metadata.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE of the entry to remove.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataRemove (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  CM_OBJECT_TOKEN       Token
  )
{
  EFI_STATUS      Status;
  METADATA_ENTRY  *Entry;

  if ((Root == NULL)              ||
      (Type >= MetadataTypeMax)   ||
      (Token == CM_NULL_TOKEN))
  {
    ASSERT (Root != NULL);
    ASSERT (Type < MetadataTypeMax);
    ASSERT (Token != CM_NULL_TOKEN);
    return EFI_INVALID_PARAMETER;
  }

  Status = MetadataFindEntry (Root, Type, Token, &Entry);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  RemoveEntryList (&Entry->List);
  if (Entry->Metadata != NULL) {
    FreePool (Entry->Metadata);
  }

  FreePool (Entry);

  return EFI_SUCCESS;
}

/** Get the Metadata associated with an (Type/Token).

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE of the entry to get.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.
  @param[out] Metadata      If success, contains the Metadata associated to the
                            input (Type/Token).
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataGet (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  CM_OBJECT_TOKEN       Token,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  )
{
  EFI_STATUS      Status;
  METADATA_ENTRY  *Entry;

  if ((Root == NULL)              ||
      (Type >= MetadataTypeMax)   ||
      (Token == CM_NULL_TOKEN)    ||
      (Metadata == NULL)          ||
      (MetadataSize != mMetadataStaticInfo[Type].ExpectedSize))
  {
    ASSERT (Root != NULL);
    ASSERT (Type < MetadataTypeMax);
    ASSERT (Token != CM_NULL_TOKEN);
    ASSERT (Metadata != NULL);
    ASSERT (MetadataSize != mMetadataStaticInfo[Type].ExpectedSize);
    return EFI_INVALID_PARAMETER;
  }

  Status = MetadataFindEntry (Root, Type, Token, &Entry);
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

/** Iterate over the existing Metadata with the same Type.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type            METADATA_TYPE to iterate over.
  @param[in]  PrevHandle    MetadataIterate () returns the Metadata handle
                            following PrevHandle.
                            If PrevHandle==NULL, the first Handle of the type
                            is returned.
                            If PrevHandle is the last Handle of the type,
                            NULL is returned.
  @param[out] Metadata      Metadata of the current Handle.
  @param[in]  MetadataSize  Size of the input Metadata.

  @return METADATA_HANDLE   The Metadata handle following PrevHandle.
**/
METADATA_HANDLE
EFIAPI
MetadataIterate (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  METADATA_HANDLE       PrevHandle,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  )
{
  METADATA_ENTRY  *Entry;
  LIST_ENTRY      *ListHead;
  LIST_ENTRY      *Link;

  if ((Root == NULL)              ||
      (Type >= MetadataTypeMax)   ||
      (Metadata == NULL)          ||
      (MetadataSize != mMetadataStaticInfo[Type].ExpectedSize))
  {
    ASSERT (Root != NULL);
    ASSERT (Type < MetadataTypeMax);
    ASSERT (Metadata != NULL);
    ASSERT (MetadataSize == mMetadataStaticInfo[Type].ExpectedSize);
    return NULL;
  }

  ListHead = &((METADATA_ROOT *)Root)->MetadataList[Type].List;

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
    CopyMem (Metadata, Entry->Metadata, mMetadataStaticInfo[Type].ExpectedSize);
  }

  return (METADATA_HANDLE)Link;
}
