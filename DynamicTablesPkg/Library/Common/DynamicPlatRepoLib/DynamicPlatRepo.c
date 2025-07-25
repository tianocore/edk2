/** @file
  Dynamic Platform Info Repository

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#include <Protocol/ConfigurationManagerProtocol.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "CmObjectTokenFixer.h"
#include "DynamicPlatRepoInternal.h"
#include "TokenGenerator.h"

/** Allocate a CM_OBJ_NODE.

  @param [in]  CmObjDesc  CmObj to wrap in a node.
                          All the fields of the CmObj (Data field included),
                          are copied.
  @param [in]  Token      Token to assign to this CmObj/node.
  @param [out] ObjNode    Allocated ObjNode.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
STATIC
EFI_STATUS
EFIAPI
AllocCmObjNode (
  IN  CONST CM_OBJ_DESCRIPTOR  *CmObjDesc,
  IN        CM_OBJECT_TOKEN    Token,
  OUT       CM_OBJ_NODE        **ObjNode
  )
{
  CM_OBJ_NODE        *Node;
  CM_OBJ_DESCRIPTOR  *Desc;

  if ((CmObjDesc == NULL) || (ObjNode == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Node = AllocateZeroPool (sizeof (CM_OBJ_NODE));
  if (Node == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialise the list head.
  InitializeListHead (&Node->Link);
  Node->Token    = Token;
  Desc           = &Node->CmObjDesc;
  Desc->ObjectId = CmObjDesc->ObjectId;
  Desc->Size     = CmObjDesc->Size;
  Desc->Count    = CmObjDesc->Count;

  // Allocate and copy the CmObject Data.
  Desc->Data = AllocateCopyPool (CmObjDesc->Size, CmObjDesc->Data);
  if (Desc->Data == NULL) {
    FreePool (Node);
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  *ObjNode = Node;
  return EFI_SUCCESS;
}

/** Free a CM_OBJ_NODE.

  @param [in]  ObjNode    ObjNode to free.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeCmObjNode (
  IN  CM_OBJ_NODE  *ObjNode
  )
{
  CM_OBJ_DESCRIPTOR  *Desc;

  if (ObjNode == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Unlink Node
  RemoveEntryList (&ObjNode->Link);

  Desc = &ObjNode->CmObjDesc;
  if (Desc->Data != NULL) {
    FreePool (Desc->Data);
  }

  FreePool (ObjNode);
  return EFI_SUCCESS;
}

/** Add an object to the dynamic platform repository.

  @param [in]  This       This dynamic platform repository.
  @param [in]  CmObjDesc  CmObj to add. The data is copied.
  @param [out] Token      If not NULL, token allocated to this CmObj.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
EFI_STATUS
EFIAPI
DynPlatRepoAddObject (
  IN        DYNAMIC_PLATFORM_REPOSITORY_INFO  *This,
  IN  CONST CM_OBJ_DESCRIPTOR                 *CmObjDesc,
  OUT       CM_OBJECT_TOKEN                   *Token OPTIONAL
  )
{
  EFI_STATUS            Status;
  CM_OBJ_NODE           *ObjNode;
  CM_OBJECT_ID          ObjId;
  CM_OBJECT_TOKEN       NewToken;
  LIST_ENTRY            *ObjList;
  EOBJECT_NAMESPACE_ID  NamespaceId;

  // The dynamic repository must be able to receive objects.
  if ((This == NULL)      ||
      (CmObjDesc == NULL) ||
      (This->RepoState != DynRepoTransient))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Check the CmObjDesc:
  //  - only Arm objects and Arch Common objects are supported for now.
  //  - only EArchCommonObjCmRef objects can be added as arrays.
  if ((CmObjDesc->Size == 0) || (CmObjDesc->Count == 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ObjId       = GET_CM_OBJECT_ID (CmObjDesc->ObjectId);
  NamespaceId = GET_CM_NAMESPACE_ID (CmObjDesc->ObjectId);

  if (EObjNameSpaceArm == NamespaceId) {
    if (ObjId >= EArmObjMax) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    ObjList = &This->ArmCmObjList[ObjId];
  } else if (EObjNameSpaceRiscV == NamespaceId) {
    if (ObjId >= ERiscVObjMax) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    ObjList = &This->RiscVCmObjList[ObjId];
  } else if (EObjNameSpaceArchCommon == NamespaceId) {
    if ((ObjId >= EArchCommonObjMax) ||
        ((CmObjDesc->Count > 1)  && (ObjId != EArchCommonObjCmRef)))
    {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    ObjList = &This->ArchCommonCmObjList[ObjId];
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Generate a token.
  NewToken = GenerateToken ();

  // Create an ObjNode.
  Status = AllocCmObjNode (CmObjDesc, NewToken, &ObjNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Fixup self-token if necessary.
  if (EObjNameSpaceArm == NamespaceId) {
    Status = FixupCmObjectSelfToken (&ObjNode->CmObjDesc, NewToken);
    if (EFI_ERROR (Status)) {
      FreeCmObjNode (ObjNode);
      ASSERT (0);
      return Status;
    }
  }

  // Add to link list.
  InsertTailList (ObjList, &ObjNode->Link);
  This->ObjectCount += 1;

  if (Token != NULL) {
    *Token = NewToken;
  }

  return EFI_SUCCESS;
}

/** Group lists of CmObjNode from the Arm Namespace or ArchCommon namespace
    to one array.

  @param [in]  This         This dynamic platform repository.
  @param [in]  NamespaceId  The namespace ID which can be EObjNameSpaceArm or
                            EObjNameSpaceArchCommon.
  @param [in]  ObjIndex     Index in EARM_OBJECT_ID (must be < EArmObjMax) or
                            EARCH_COMMON_OBJECT_ID (must be <EArchCommonObjMax).

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_BUFFER_TOO_SMALL    Buffer too small.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
STATIC
EFI_STATUS
EFIAPI
GroupCmObjNodes (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *This,
  IN  EOBJECT_NAMESPACE_ID              NamespaceId,
  IN  UINT32                            ObjIndex
  )
{
  EFI_STATUS         Status;
  UINTN              Count;
  UINTN              Size;
  UINT32             CmObjId;
  UINT8              *GroupedData;
  UINT8              *Data;
  CM_OBJ_DESCRIPTOR  *CmObjDesc;
  LIST_ENTRY         *ListHead;
  LIST_ENTRY         *Link;
  CM_OBJ_DESCRIPTOR  *ObjArray;

  if (This == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (NamespaceId == EObjNameSpaceArm) {
    if (ObjIndex >= EArmObjMax) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    ListHead = &This->ArmCmObjList[ObjIndex];
    ObjArray = &This->ArmCmObjArray[ObjIndex];
  } else if (NamespaceId == EObjNameSpaceRiscV) {
    if (ObjIndex >= ERiscVObjMax) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    ListHead = &This->RiscVCmObjList[ObjIndex];
    ObjArray = &This->RiscVCmObjArray[ObjIndex];
  } else if (NamespaceId == EObjNameSpaceArchCommon) {
    if (ObjIndex >= EArchCommonObjMax) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    ListHead = &This->ArchCommonCmObjList[ObjIndex];
    ObjArray = &This->ArchCommonCmObjArray[ObjIndex];
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Count   = 0;
  Size    = 0;
  CmObjId = CREATE_CM_OBJECT_ID (NamespaceId, ObjIndex);
  Link    = GetFirstNode (ListHead);

  // Compute the total count and size of the CmObj in the list.
  while (Link != ListHead) {
    CmObjDesc = &((CM_OBJ_NODE *)Link)->CmObjDesc;

    if (CmObjDesc->ObjectId != CmObjId) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    if ((CmObjDesc->Count != 1) &&
        ((NamespaceId != EObjNameSpaceArchCommon) ||
         (ObjIndex != EArchCommonObjCmRef)))
    {
      // We expect each descriptor to contain an individual object.
      // EArchCommonObjCmRef objects are counted as groups, so +1 as well.
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Count++;
    Size += CmObjDesc->Size;

    // Next Link
    Link = GetNextNode (ListHead, Link);
  } // while

  if (Count == 0) {
    // No objects found.
    return EFI_SUCCESS;
  }

  GroupedData = AllocateZeroPool (Size);
  if (GroupedData == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Copy the Object Data and add to the TokenMapper.
  Data = GroupedData;
  Link = GetFirstNode (ListHead);
  while (Link != ListHead) {
    CmObjDesc = &((CM_OBJ_NODE *)Link)->CmObjDesc;
    CopyMem (Data, CmObjDesc->Data, CmObjDesc->Size);

    // Add the object to the Token Mapper.
    // Note: The CmObject Data field of objects in the Token Mapper point
    //       to the memory in the GroupedData array.
    Status = TokenMapperAddObject (
               &This->TokenMapper,
               ((CM_OBJ_NODE *)Link)->Token,
               CmObjDesc->ObjectId,
               CmObjDesc->Size,
               Data
               );
    if (EFI_ERROR (Status)) {
      FreePool (GroupedData);
      return Status;
    }

    Data += CmObjDesc->Size;
    Link  = GetNextNode (ListHead, Link);
  } // while

  CmObjDesc           = ObjArray;
  CmObjDesc->ObjectId = CmObjId;
  CmObjDesc->Size     = (UINT32)Size;
  CmObjDesc->Count    = (UINT32)Count;
  CmObjDesc->Data     = GroupedData;

  return Status;
}

/** Finalise the dynamic repository.

  Finalising means:
   - Preventing any further objects from being added.
   - Allowing to get objects from the dynamic repository
     (not possible before a call to this function).

  @param [in]  This       This dynamic platform repository.

  @retval EFI_SUCCESS           Success.
  @retval EFI_ALREADY_STARTED   Instance already initialised.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_BUFFER_TOO_SMALL  Buffer too small.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoFinalise (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *This
  )
{
  EFI_STATUS  Status;
  UINTN       ObjIndex;

  if ((This == NULL)  ||
      (This->RepoState != DynRepoTransient))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Prevent any further objects from being added.
  This->RepoState = DynRepoFinalized;

  // Initialise the token mapper.
  Status = TokenMapperInitialise (&This->TokenMapper, This->ObjectCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // For each CM_OBJECT_ID:
  //  - Convert the list of nodes to an array
  //    (the array is wrapped in a CmObjDesc).
  //  - Add the Token/CmObj binding to the token mapper.
  for (ObjIndex = 0; ObjIndex < EArmObjMax; ObjIndex++) {
    Status = GroupCmObjNodes (This, EObjNameSpaceArm, (UINT32)ObjIndex);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }
  } // for

  for (ObjIndex = 0; ObjIndex < ERiscVObjMax; ObjIndex++) {
    Status = GroupCmObjNodes (This, EObjNameSpaceRiscV, (UINT32)ObjIndex);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }
  } // for

  for (ObjIndex = 0; ObjIndex < EArchCommonObjMax; ObjIndex++) {
    Status = GroupCmObjNodes (This, EObjNameSpaceArchCommon, (UINT32)ObjIndex);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }
  } // for

  return EFI_SUCCESS;

error_handler:
  // Free the TokenMapper.
  // Ignore the returned Status since we already failed.
  TokenMapperShutdown (&This->TokenMapper);
  return Status;
}

/** Get a CmObj from the dynamic repository.

  @param [in]      This        Pointer to the Dynamic Platform Repository.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObjDesc   Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoGetObject (
  IN      DYNAMIC_PLATFORM_REPOSITORY_INFO  *This,
  IN      CM_OBJECT_ID                      CmObjectId,
  IN      CM_OBJECT_TOKEN                   Token OPTIONAL,
  IN  OUT CM_OBJ_DESCRIPTOR                 *CmObjDesc
  )
{
  EFI_STATUS            Status;
  CM_OBJ_DESCRIPTOR     *Desc;
  CM_OBJECT_ID          ObjId;
  EOBJECT_NAMESPACE_ID  NamespaceId;

  if ((This == NULL)      ||
      (CmObjDesc == NULL) ||
      (This->RepoState != DynRepoFinalized))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  NamespaceId = GET_CM_NAMESPACE_ID (CmObjectId);
  ObjId       = GET_CM_OBJECT_ID (CmObjectId);

  if (NamespaceId == EObjNameSpaceArm) {
    if (ObjId >= EArmObjMax) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Desc = &This->ArmCmObjArray[ObjId];
  } else if (NamespaceId == EObjNameSpaceRiscV) {
    if (ObjId >= ERiscVObjMax) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Desc = &This->RiscVCmObjArray[ObjId];
  } else if (NamespaceId == EObjNameSpaceArchCommon) {
    if ((ObjId >= EArchCommonObjMax) ||
        ((ObjId == EArchCommonObjCmRef) &&
         (Token == CM_NULL_TOKEN)))
    {
      // EArchCommonObjCmRef object must be requested using a valid token.
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Desc = &This->ArchCommonCmObjArray[ObjId];
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (Token != CM_NULL_TOKEN) {
    // Search in the Token Mapper and return the object.
    Status = TokenMapperGetObject (
               &This->TokenMapper,
               Token,
               CmObjectId,
               CmObjDesc
               );
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Nothing here.
  if (Desc->Count == 0) {
    return EFI_NOT_FOUND;
  } else {
    // Return the full array.
    CmObjDesc->ObjectId = Desc->ObjectId;
    CmObjDesc->Size     = Desc->Size;
    CmObjDesc->Data     = Desc->Data;
    CmObjDesc->Count    = Desc->Count;
  }

  return EFI_SUCCESS;
}

/** Initialize the dynamic platform repository.

  @param [out]  DynPlatRepo   If success, contains the initialised dynamic
                              platform repository.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  An allocation has failed.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoInit (
  OUT DYNAMIC_PLATFORM_REPOSITORY_INFO  **DynPlatRepo
  )
{
  UINTN                             Index;
  DYNAMIC_PLATFORM_REPOSITORY_INFO  *Repo;

  if (DynPlatRepo == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Repo = AllocateZeroPool (sizeof (DYNAMIC_PLATFORM_REPOSITORY_INFO));
  if (Repo == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialise the CmObject List.
  for (Index = 0; Index < EArmObjMax; Index++) {
    InitializeListHead (&Repo->ArmCmObjList[Index]);
  }

  for (Index = 0; Index < ERiscVObjMax; Index++) {
    InitializeListHead (&Repo->RiscVCmObjList[Index]);
  }

  for (Index = 0; Index < EArchCommonObjMax; Index++) {
    InitializeListHead (&Repo->ArchCommonCmObjList[Index]);
  }

  Repo->ObjectCount = 0;
  Repo->RepoState   = DynRepoTransient;

  *DynPlatRepo = Repo;

  return EFI_SUCCESS;
}

/** Free Arm Namespace objects.

  Free all the memory allocated for the Arm namespace objects in the
  dynamic platform repository.

  @param [in]  DynPlatRepo    The dynamic platform repository.

**/
STATIC
VOID
EFIAPI
DynamicPlatRepoFreeArmObjects (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *DynPlatRepo
  )
{
  UINT32             Index;
  LIST_ENTRY         *ListHead;
  CM_OBJ_DESCRIPTOR  *CmObjDesc;
  VOID               *Data;

  ASSERT (DynPlatRepo != NULL);

  // Free the list of objects.
  for (Index = 0; Index < EArmObjMax; Index++) {
    // Free all the nodes with this object Id.
    ListHead = &DynPlatRepo->ArmCmObjList[Index];
    while (!IsListEmpty (ListHead)) {
      FreeCmObjNode ((CM_OBJ_NODE *)GetFirstNode (ListHead));
    } // while
  } // for

  // Free the arrays.
  CmObjDesc = DynPlatRepo->ArmCmObjArray;
  for (Index = 0; Index < EArmObjMax; Index++) {
    Data = CmObjDesc[Index].Data;
    if (Data != NULL) {
      FreePool (Data);
    }
  } // for
}

/** Free RISC-V Namespace objects.

  Free all the memory allocated for the Arm namespace objects in the
  dynamic platform repository.

  @param [in]  DynPlatRepo    The dynamic platform repository.

**/
STATIC
VOID
EFIAPI
DynamicPlatRepoFreeRiscVObjects (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *DynPlatRepo
  )
{
  UINT32             Index;
  LIST_ENTRY         *ListHead;
  CM_OBJ_DESCRIPTOR  *CmObjDesc;
  VOID               *Data;

  ASSERT (DynPlatRepo != NULL);

  // Free the list of objects.
  for (Index = 0; Index < ERiscVObjMax; Index++) {
    // Free all the nodes with this object Id.
    ListHead = &DynPlatRepo->RiscVCmObjList[Index];
    while (!IsListEmpty (ListHead)) {
      FreeCmObjNode ((CM_OBJ_NODE *)GetFirstNode (ListHead));
    } // while
  } // for

  // Free the arrays.
  CmObjDesc = DynPlatRepo->RiscVCmObjArray;
  for (Index = 0; Index < ERiscVObjMax; Index++) {
    Data = CmObjDesc[Index].Data;
    if (Data != NULL) {
      FreePool (Data);
    }
  } // for
}

/** Free Arch Common Namespace objects.

  Free all the memory allocated for the Arch Common namespace objects in the
  dynamic platform repository.

  @param [in]  DynPlatRepo    The dynamic platform repository.

**/
STATIC
VOID
EFIAPI
DynamicPlatRepoFreeArchCommonObjects (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *DynPlatRepo
  )
{
  UINT32             Index;
  LIST_ENTRY         *ListHead;
  CM_OBJ_DESCRIPTOR  *CmObjDesc;
  VOID               *Data;

  ASSERT (DynPlatRepo != NULL);

  // Free the list of objects.
  for (Index = 0; Index < EArchCommonObjMax; Index++) {
    // Free all the nodes with this object Id.
    ListHead = &DynPlatRepo->ArchCommonCmObjList[Index];
    while (!IsListEmpty (ListHead)) {
      FreeCmObjNode ((CM_OBJ_NODE *)GetFirstNode (ListHead));
    } // while
  } // for

  // Free the arrays.
  CmObjDesc = DynPlatRepo->ArchCommonCmObjArray;
  for (Index = 0; Index < EArchCommonObjMax; Index++) {
    Data = CmObjDesc[Index].Data;
    if (Data != NULL) {
      FreePool (Data);
    }
  } // for
}

/** Shutdown the dynamic platform repository.

  Free all the memory allocated for the dynamic platform repository.

  @param [in]  DynPlatRepo    The dynamic platform repository.

  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
EFIAPI
DynamicPlatRepoShutdown (
  IN  DYNAMIC_PLATFORM_REPOSITORY_INFO  *DynPlatRepo
  )
{
  EFI_STATUS  Status;

  if (DynPlatRepo == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  DynamicPlatRepoFreeArmObjects (DynPlatRepo);
  DynamicPlatRepoFreeRiscVObjects (DynPlatRepo);
  DynamicPlatRepoFreeArchCommonObjects (DynPlatRepo);

  // Free the TokenMapper
  Status = TokenMapperShutdown (&DynPlatRepo->TokenMapper);
  ASSERT_EFI_ERROR (Status);
  FreePool (DynPlatRepo);
  return Status;
}
