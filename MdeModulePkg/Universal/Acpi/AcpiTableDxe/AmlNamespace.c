/** @file
  ACPI Sdt Protocol Driver

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiTable.h"

/**
  Construct node list according to the AML handle.

  @param[in]    AmlHandle            AML handle.
  @param[in]    AmlRootNodeList      AML root node list.
  @param[in]    AmlParentNodeList    AML parent node list.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER AML handle does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlConstructNodeList (
  IN EFI_AML_HANDLE     *AmlHandle,
  IN EFI_AML_NODE_LIST  *AmlRootNodeList,
  IN EFI_AML_NODE_LIST  *AmlParentNodeList
  );

/**
  Create AML Node.

  @param[in]    NameSeg              AML NameSeg.
  @param[in]    Parent               AML parent node list.
  @param[in]    AmlByteEncoding      AML Byte Encoding.

  @return       AML Node.
**/
EFI_AML_NODE_LIST *
AmlCreateNode (
  IN UINT8              *NameSeg,
  IN EFI_AML_NODE_LIST  *Parent,
  IN AML_BYTE_ENCODING  *AmlByteEncoding
  )
{
  EFI_AML_NODE_LIST  *AmlNodeList;

  AmlNodeList = AllocatePool (sizeof (*AmlNodeList));
  ASSERT (AmlNodeList != NULL);

  AmlNodeList->Signature = EFI_AML_NODE_LIST_SIGNATURE;
  CopyMem (AmlNodeList->Name, NameSeg, AML_NAME_SEG_SIZE);
  AmlNodeList->Buffer = NULL;
  AmlNodeList->Size   = 0;
  InitializeListHead (&AmlNodeList->Link);
  InitializeListHead (&AmlNodeList->Children);
  AmlNodeList->Parent          = Parent;
  AmlNodeList->AmlByteEncoding = AmlByteEncoding;

  return AmlNodeList;
}

/**
  Find the AML NameSeg in the children of AmlParentNodeList.

  @param[in]    NameSeg              AML NameSeg.
  @param[in]    AmlParentNodeList    AML parent node list.
  @param[in]    Create               TRUE means to create node if not found.

  @return       AmlChildNode whoes name is same as NameSeg.
**/
EFI_AML_NODE_LIST *
AmlFindNodeInThis (
  IN UINT8              *NameSeg,
  IN EFI_AML_NODE_LIST  *AmlParentNodeList,
  IN BOOLEAN            Create
  )
{
  EFI_AML_NODE_LIST  *CurrentAmlNodeList;
  LIST_ENTRY         *CurrentLink;
  LIST_ENTRY         *StartLink;
  EFI_AML_NODE_LIST  *AmlNodeList;

  StartLink   = &AmlParentNodeList->Children;
  CurrentLink = StartLink->ForwardLink;

  while (CurrentLink != StartLink) {
    CurrentAmlNodeList = EFI_AML_NODE_LIST_FROM_LINK (CurrentLink);
    //
    // AML name is same as the one stored
    //
    if (CompareMem (CurrentAmlNodeList->Name, NameSeg, AML_NAME_SEG_SIZE) == 0) {
      //
      // Good! Found it
      //
      return CurrentAmlNodeList;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // Not found
  //
  if (!Create) {
    return NULL;
  }

  //
  // Create new node with NULL buffer - it means namespace not be returned.
  //
  AmlNodeList = AmlCreateNode (NameSeg, AmlParentNodeList, NULL);
  InsertTailList (&AmlParentNodeList->Children, &AmlNodeList->Link);

  return AmlNodeList;
}

/**
  Find the AML NameString in the children of AmlParentNodeList or AmlRootNodeList.

  @param[in]    NameString           AML NameString.
  @param[in]    AmlRootNodeList      AML root node list.
  @param[in]    AmlParentNodeList    AML parent node list.
  @param[in]    Create               TRUE means to create node if not found.

  @return       AmlChildNode whoes name is same as NameSeg.
**/
EFI_AML_NODE_LIST *
AmlFindNodeInTheTree (
  IN UINT8              *NameString,
  IN EFI_AML_NODE_LIST  *AmlRootNodeList,
  IN EFI_AML_NODE_LIST  *AmlParentNodeList,
  IN BOOLEAN            Create
  )
{
  UINT8              *Buffer;
  EFI_AML_NODE_LIST  *AmlNodeList;
  EFI_AML_NODE_LIST  *AmlCurrentNodeList;
  UINT8              Index;
  UINT8              SegCount;

  Buffer = NameString;

  //
  // Handle root or parent prefix
  //
  if (*Buffer == AML_ROOT_CHAR) {
    AmlCurrentNodeList = AmlRootNodeList;
    Buffer            += 1;
  } else if (*Buffer == AML_PARENT_PREFIX_CHAR) {
    AmlCurrentNodeList = AmlParentNodeList;
    do {
      if (AmlCurrentNodeList->Parent != NULL) {
        AmlCurrentNodeList = AmlCurrentNodeList->Parent;
      } else {
        //
        // Only root has no parent
        //
        ASSERT (AmlCurrentNodeList == AmlRootNodeList);
      }

      Buffer += 1;
    } while (*Buffer == AML_PARENT_PREFIX_CHAR);
  } else {
    AmlCurrentNodeList = AmlParentNodeList;
  }

  //
  // Handle name segment
  //
  if (*Buffer == AML_DUAL_NAME_PREFIX) {
    Buffer  += 1;
    SegCount = 2;
  } else if (*Buffer == AML_MULTI_NAME_PREFIX) {
    Buffer  += 1;
    SegCount = *Buffer;
    Buffer  += 1;
  } else if (*Buffer == 0) {
    //
    // NULL name, only for Root
    //
    ASSERT (AmlCurrentNodeList == AmlRootNodeList);
    return AmlCurrentNodeList;
  } else {
    SegCount = 1;
  }

  //
  // Handle NamePath
  //
  Index = 0;
  do {
    AmlNodeList = AmlFindNodeInThis (Buffer, AmlCurrentNodeList, Create);
    if (AmlNodeList == NULL) {
      return NULL;
    }

    AmlCurrentNodeList = AmlNodeList;
    Buffer            += AML_NAME_SEG_SIZE;
    Index++;
  } while (Index < SegCount);

  return AmlNodeList;
}

/**
  Insert the NameString to the AmlNodeList.

  @param[in]    NameString           AML NameString.
  @param[in]    Buffer               Buffer for the Node.
  @param[in]    Size                 Size for the Node.
  @param[in]    AmlRootNodeList      AML root node list.
  @param[in]    AmlParentNodeList    AML parent node list.

  @return       AmlChildNode whoes name is NameString.
**/
EFI_AML_NODE_LIST *
AmlInsertNodeToTree (
  IN UINT8              *NameString,
  IN VOID               *Buffer,
  IN UINTN              Size,
  IN EFI_AML_NODE_LIST  *AmlRootNodeList,
  IN EFI_AML_NODE_LIST  *AmlParentNodeList
  )
{
  EFI_AML_NODE_LIST  *AmlNodeList;

  AmlNodeList = AmlFindNodeInTheTree (
                  NameString,
                  AmlRootNodeList,
                  AmlParentNodeList,
                  TRUE  // Find and Create
                  );
  ASSERT (AmlNodeList != NULL);
  if (AmlNodeList == NULL) {
    return NULL;
  }

  //
  // Check buffer
  //
  if (AmlNodeList->Buffer == NULL) {
    //
    // NULL means new added one or SCOPE_OP
    //
    if (*(UINT8 *)Buffer != AML_SCOPE_OP) {
      //
      // We need check if new one is SCOPE_OP, because SCOPE_OP just means namespace, not a real device.
      // We should not return SCOPE_OP.
      //
      AmlNodeList->Buffer          = Buffer;
      AmlNodeList->Size            = Size;
      AmlNodeList->AmlByteEncoding = AmlSearchByOpByte (Buffer);
    }

    return AmlNodeList;
  }

  //
  // Already added
  //
  if (*(UINT8 *)Buffer == AML_SCOPE_OP) {
    //
    // The new one is SCOPE_OP, OK just return;
    //
    return AmlNodeList;
  }

  //
  // Oops!!!, There must be something wrong.
  //
  DEBUG ((DEBUG_ERROR, "AML: Override Happen - %a!\n", NameString));
  DEBUG ((DEBUG_ERROR, "AML: Existing Node - %x\n", AmlNodeList->Buffer));
  DEBUG ((DEBUG_ERROR, "AML: New Buffer - %x\n", Buffer));

  return NULL;
}

/**
  Construct child node list according to the AML handle.

  @param[in]    AmlHandle            AML handle.
  @param[in]    AmlRootNodeList      AML root node list.
  @param[in]    AmlParentNodeList    AML parent node list.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER AML handle does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlConstructNodeListForChild (
  IN EFI_AML_HANDLE     *AmlHandle,
  IN EFI_AML_NODE_LIST  *AmlRootNodeList,
  IN EFI_AML_NODE_LIST  *AmlParentNodeList
  )
{
  AML_BYTE_ENCODING  *AmlByteEncoding;
  UINT8              *Buffer;
  UINTN              BufferSize;
  UINT8              *CurrentBuffer;
  EFI_AML_HANDLE     *AmlChildHandle;
  EFI_STATUS         Status;

  CurrentBuffer   = NULL;
  AmlChildHandle  = NULL;
  AmlByteEncoding = AmlHandle->AmlByteEncoding;
  Buffer          = AmlHandle->Buffer;
  BufferSize      = AmlHandle->Size;

  //
  // Check if we need recursively add node
  //
  if ((AmlByteEncoding->Attribute & AML_HAS_CHILD_OBJ) == 0) {
    //
    // No more node need to be added
    //
    return EFI_SUCCESS;
  }

  //
  // Do we need add node within METHOD?
  // Yes, just add Object is OK. But we need filter NameString for METHOD invoke.
  //

  //
  // Now, we get the last node.
  //
  Status = AmlGetOffsetAfterLastOption (AmlHandle, &CurrentBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Go through all the reset buffer.
  //
  while ((UINTN)CurrentBuffer < (UINTN)Buffer + BufferSize) {
    //
    // Find the child node.
    //
    Status = SdtOpenEx (CurrentBuffer, (UINTN)Buffer + BufferSize - (UINTN)CurrentBuffer, (EFI_ACPI_HANDLE *)&AmlChildHandle);
    if (EFI_ERROR (Status)) {
      //
      // No child found, break now.
      //
      break;
    }

    //
    // Good, find the child. Construct node recursively
    //
    Status = AmlConstructNodeList (
               AmlChildHandle,
               AmlRootNodeList,
               AmlParentNodeList
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Parse next one
    //
    CurrentBuffer += AmlChildHandle->Size;

    Close ((EFI_ACPI_HANDLE)AmlChildHandle);
  }

  return EFI_SUCCESS;
}

/**
  Construct node list according to the AML handle.

  @param[in]    AmlHandle            AML handle.
  @param[in]    AmlRootNodeList      AML root node list.
  @param[in]    AmlParentNodeList    AML parent node list.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_INVALID_PARAMETER AML handle does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlConstructNodeList (
  IN EFI_AML_HANDLE     *AmlHandle,
  IN EFI_AML_NODE_LIST  *AmlRootNodeList,
  IN EFI_AML_NODE_LIST  *AmlParentNodeList
  )
{
  VOID               *NameString;
  EFI_AML_NODE_LIST  *AmlNodeList;

  //
  // 1. Check if there is need to construct node for this OpCode.
  //
  if ((AmlHandle->AmlByteEncoding->Attribute & AML_IN_NAMESPACE) == 0) {
    //
    // No need to construct node, so we just skip this OpCode.
    //
    return EFI_SUCCESS;
  }

  //
  // 2. Now, we need construct node for this OpCode.
  //
  NameString = AmlGetObjectName (AmlHandle);
  if (NameString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now, we need to insert node to the node list.
  // NOTE: The name here could be AML NameString. So the callee need parse it.
  //
  AmlNodeList = AmlInsertNodeToTree (NameString, AmlHandle->Buffer, AmlHandle->Size, AmlRootNodeList, AmlParentNodeList);
  ASSERT (AmlNodeList != NULL);

  //
  // 3. Ok, we need to parse the object list to see if there are more node to be added.
  //
  return AmlConstructNodeListForChild (AmlHandle, AmlRootNodeList, AmlNodeList);
}

/**
  Destruct node list

  @param[in]    AmlParentNodeList    AML parent node list.
**/
VOID
AmlDestructNodeList (
  IN EFI_AML_NODE_LIST  *AmlParentNodeList
  )
{
  EFI_AML_NODE_LIST  *CurrentAmlNodeList;
  LIST_ENTRY         *CurrentLink;
  LIST_ENTRY         *StartLink;

  //
  // Get the children link
  //
  StartLink   = &AmlParentNodeList->Children;
  CurrentLink = StartLink->ForwardLink;

  //
  // Go through all the children
  //
  while (CurrentLink != StartLink) {
    //
    // Destruct the child's list recursively
    //
    CurrentAmlNodeList = EFI_AML_NODE_LIST_FROM_LINK (CurrentLink);
    CurrentLink        = CurrentLink->ForwardLink;

    //
    // Remove this child from list and free the node
    //
    RemoveEntryList (&(CurrentAmlNodeList->Link));

    AmlDestructNodeList (CurrentAmlNodeList);
  }

  //
  // Done.
  //
  FreePool (AmlParentNodeList);
  return;
}

/**
  Dump node list

  @param[in]    AmlParentNodeList    AML parent node list.
  @param[in]    Level                Output debug level.
**/
VOID
AmlDumpNodeInfo (
  IN EFI_AML_NODE_LIST  *AmlParentNodeList,
  IN UINTN              Level
  )
{
  EFI_AML_NODE_LIST    *CurrentAmlNodeList;
  volatile LIST_ENTRY  *CurrentLink;
  UINTN                Index;

  CurrentLink = AmlParentNodeList->Children.ForwardLink;

  if (Level == 0) {
    DEBUG ((DEBUG_ERROR, "\\"));
  } else {
    for (Index = 0; Index < Level; Index++) {
      DEBUG ((DEBUG_ERROR, "    "));
    }

    AmlPrintNameSeg (AmlParentNodeList->Name);
  }

  DEBUG ((DEBUG_ERROR, "\n"));

  while (CurrentLink != &AmlParentNodeList->Children) {
    CurrentAmlNodeList = EFI_AML_NODE_LIST_FROM_LINK (CurrentLink);
    AmlDumpNodeInfo (CurrentAmlNodeList, Level + 1);
    CurrentLink = CurrentLink->ForwardLink;
  }

  return;
}

/**
  Returns the handle of the ACPI object representing the specified ACPI AML path

  @param[in]    AmlHandle   Points to the handle of the object representing the starting point for the path search.
  @param[in]    AmlPath     Points to the ACPI AML path.
  @param[out]   Buffer      On return, points to the ACPI object which represents AcpiPath, relative to
                            HandleIn.
  @param[in]    FromRoot    TRUE means to find AML path from \ (Root) Node.
                            FALSE means to find AML path from this Node (The HandleIn).

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER HandleIn does not refer to a valid ACPI object.
**/
EFI_STATUS
AmlFindPath (
  IN    EFI_AML_HANDLE  *AmlHandle,
  IN    UINT8           *AmlPath,
  OUT   VOID            **Buffer,
  IN    BOOLEAN         FromRoot
  )
{
  EFI_AML_NODE_LIST  *AmlRootNodeList;
  EFI_STATUS         Status;
  EFI_AML_NODE_LIST  *AmlNodeList;
  UINT8              RootNameSeg[AML_NAME_SEG_SIZE];
  EFI_AML_NODE_LIST  *CurrentAmlNodeList;
  LIST_ENTRY         *CurrentLink;

  //
  // 1. create tree
  //

  //
  // Create root handle
  //
  RootNameSeg[0]  = AML_ROOT_CHAR;
  RootNameSeg[1]  = 0;
  AmlRootNodeList = AmlCreateNode (RootNameSeg, NULL, AmlHandle->AmlByteEncoding);

  Status = AmlConstructNodeList (
             AmlHandle,
             AmlRootNodeList, // Root
             AmlRootNodeList  // Parent
             );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG_CODE_BEGIN ();
  DEBUG ((DEBUG_ERROR, "AcpiSdt: NameSpace:\n"));
  AmlDumpNodeInfo (AmlRootNodeList, 0);
  DEBUG_CODE_END ();

  //
  // 2. Search the node in the tree
  //
  if (FromRoot) {
    //
    // Search from Root
    //
    CurrentAmlNodeList = AmlRootNodeList;
  } else {
    //
    // Search from this node, NOT ROOT.
    // Since we insert node to ROOT one by one, we just get the first node and search from it.
    //
    CurrentLink = AmlRootNodeList->Children.ForwardLink;
    if (CurrentLink != &AmlRootNodeList->Children) {
      //
      // First node
      //
      CurrentAmlNodeList = EFI_AML_NODE_LIST_FROM_LINK (CurrentLink);
    } else {
      //
      // No child
      //
      CurrentAmlNodeList = NULL;
    }
  }

  //
  // Search
  //
  if (CurrentAmlNodeList != NULL) {
    DEBUG_CODE_BEGIN ();
    DEBUG ((DEBUG_ERROR, "AcpiSdt: Search from: \\"));
    AmlPrintNameSeg (CurrentAmlNodeList->Name);
    DEBUG ((DEBUG_ERROR, "\n"));
    DEBUG_CODE_END ();
    AmlNodeList = AmlFindNodeInTheTree (
                    AmlPath,
                    AmlRootNodeList,    // Root
                    CurrentAmlNodeList, // Parent
                    FALSE
                    );
  } else {
    AmlNodeList = NULL;
  }

  *Buffer = NULL;
  Status  = EFI_SUCCESS;
  if ((AmlNodeList != NULL) && (AmlNodeList->Buffer != NULL)) {
    *Buffer = AmlNodeList->Buffer;
  }

  //
  // 3. free the tree
  //
  AmlDestructNodeList (AmlRootNodeList);

  return Status;
}
