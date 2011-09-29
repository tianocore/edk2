/** @file
  Function and Macro defintions for to extract default values from UEFI Form package.

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"
#include "UefiIfrParser.h"
#include "UefiIfrDefault.h"

//
// Extern Variables
//
extern CONST EFI_HII_DATABASE_PROTOCOL            *mHiiDatabase;
extern CONST EFI_HII_IMAGE_PROTOCOL               *mHiiImageProtocol;
extern CONST EFI_HII_STRING_PROTOCOL              *mHiiStringProtocol;
extern CONST EFI_HII_CONFIG_ROUTING_PROTOCOL      *mHiiConfigRoutingProtocol;

/**
  Set the data position at Offset with Width in Node->Buffer based 
  the value passed in.

  @param  Node                    The Buffer Storage Node.
  @param Value                    The input value.
  @param Offset                   The offset in Node->Buffer for the update.
  @param Width                    The length of the Value.

**/
VOID
SetNodeBuffer (
  OUT UEFI_IFR_BUFFER_STORAGE_NODE        *Node,
  IN  CONST   EFI_HII_VALUE               *Value,
  IN  UINTN                               Offset,
  IN  UINTN                               Width
  )
{
  ASSERT (Node->Signature == UEFI_IFR_BUFFER_STORAGE_NODE_SIGNATURE);
  ASSERT (Offset + Width <= Node->Size);

  CopyMem (Node->Buffer + Offset, &Value->Value.u8, Width);
}


/**
  Get question default value, and set it into the match var storage.

  Note Framework 0.92's HII Implementation does not support for default value for these opcodes:
  EFI_IFR_ORDERED_LIST_OP:
  EFI_IFR_PASSWORD_OP:
  EFI_IFR_STRING_OP:

  @param  Question               Question to be set to its default value.
  @param  DefaultId              The Class of the default.
  @param  VarStoreId             Id of var storage. 
  @param  Node                   Var storage buffer to store the got default value.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetQuestionDefault (
  IN FORM_BROWSER_STATEMENT           *Question,
  IN UINT16                           DefaultId,
  IN UINT16                           VarStoreId,
  OUT UEFI_IFR_BUFFER_STORAGE_NODE    *Node
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  QUESTION_DEFAULT        *Default;
  QUESTION_OPTION         *Option;
  EFI_HII_VALUE           *HiiValue;

  Status = EFI_SUCCESS;

  //
  // Statement don't have storage, skip them
  //
  if (Question->QuestionId == 0) {
    return Status;
  }

  if (Question->VarStoreId != VarStoreId) {
    return Status;
  }

  ASSERT (Question->Storage->Type == EFI_HII_VARSTORE_BUFFER);

  //
  // There are three ways to specify default value for a Question:
  //  1, use nested EFI_IFR_DEFAULT (highest priority)
  //  2, set flags of EFI_ONE_OF_OPTION (provide Standard and Manufacturing default)
  //  3, set flags of EFI_IFR_CHECKBOX (provide Standard and Manufacturing default) (lowest priority)
  //
  HiiValue = &Question->HiiValue;

  //
  // EFI_IFR_DEFAULT has highest priority
  //
  if (!IsListEmpty (&Question->DefaultListHead)) {
    Link = GetFirstNode (&Question->DefaultListHead);
    while (!IsNull (&Question->DefaultListHead, Link)) {
      Default = QUESTION_DEFAULT_FROM_LINK (Link);

      if (Default->DefaultId == DefaultId) {
        //
        // Default value is embedded in EFI_IFR_DEFAULT
        //
        CopyMem (HiiValue, &Default->Value, sizeof (EFI_HII_VALUE));
       
        SetNodeBuffer (Node, HiiValue, Question->VarStoreInfo.VarOffset, Question->StorageWidth);
        return EFI_SUCCESS;
      }

      Link = GetNextNode (&Question->DefaultListHead, Link);
    }
  }

  //
  // EFI_ONE_OF_OPTION
  //
  if ((Question->Operand == EFI_IFR_ONE_OF_OP) && !IsListEmpty (&Question->OptionListHead)) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING)  {
      //
      // OneOfOption could only provide Standard and Manufacturing default
      //
      Link = GetFirstNode (&Question->OptionListHead);
      while (!IsNull (&Question->OptionListHead, Link)) {
        Option = QUESTION_OPTION_FROM_LINK (Link);

        if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT) == EFI_IFR_OPTION_DEFAULT)) ||
            ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT_MFG) == EFI_IFR_OPTION_DEFAULT_MFG))
           ) {
          CopyMem (HiiValue, &Option->Value, sizeof (EFI_HII_VALUE));

          SetNodeBuffer (Node, HiiValue, Question->VarStoreInfo.VarOffset, Question->StorageWidth);
          return EFI_SUCCESS;
        }

        Link = GetNextNode (&Question->OptionListHead, Link);
      }
    }
  }

  //
  // EFI_IFR_CHECKBOX - lowest priority
  //
  if (Question->Operand == EFI_IFR_CHECKBOX_OP) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING)  {
      //
      // Checkbox could only provide Standard and Manufacturing default
      //
      if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Question->Flags & EFI_IFR_CHECKBOX_DEFAULT) == EFI_IFR_CHECKBOX_DEFAULT)) ||
          ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Question->Flags & EFI_IFR_CHECKBOX_DEFAULT_MFG) == EFI_IFR_CHECKBOX_DEFAULT_MFG))
         ) {
        HiiValue->Value.b = TRUE;
      } else {
        HiiValue->Value.b = FALSE;
      }

      SetNodeBuffer (Node, HiiValue, Question->VarStoreInfo.VarOffset, Question->StorageWidth);
      return EFI_SUCCESS;
    }
  }

  return Status;
}


/**
  Extract the default values from all questions in the input Form, 
  and set default value into the matched var storage.

  @param  Form                   The Form which to be reset.
  @param  DefaultId              The Class of the default.
  @param  VarStoreId             Id of var storage. 
  @param  Node                   Var storage buffer to store the got default value.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
ExtractFormDefault (
  IN FORM_BROWSER_FORM                *Form,
  IN UINT16                           DefaultId,
  IN UINT16                           VarStoreId,
  OUT UEFI_IFR_BUFFER_STORAGE_NODE    *Node
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  FORM_BROWSER_STATEMENT  *Question;

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Question = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    //
    // Reset Question to its default value
    //
    Status = GetQuestionDefault (Question, DefaultId, VarStoreId, Node);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Link = GetNextNode (&Form->StatementListHead, Link);
  }
  return EFI_SUCCESS;
}


/**
  Destroy all the buffer allocated for the fileds of
  UEFI_IFR_BUFFER_STORAGE_NODE. The Node itself
  will be freed too.

  @param  Node                Var storage buffer.

**/
VOID
DestroyDefaultNode (
  IN UEFI_IFR_BUFFER_STORAGE_NODE        *Node
  )
{
  FreePool (Node->Buffer);
  FreePool (Node->Name);
  FreePool (Node);
}


/**
  Get the default value for Buffer Type storage named by
  a Default Store and a Storage Store from a FormSet.
  The result is in the a instance of UEFI_IFR_BUFFER_STORAGE_NODE
  allocated by this function. It is inserted to the link list.
  
  @param  DefaultStore           The Default Store.
  @param  Storage                The Storage.
  @param  FormSet                The Form Set.
  @param  UefiDefaultsListHead   The head of link list for the output.

  @retval   EFI_SUCCESS          Successful.
  
**/
EFI_STATUS
GetBufferTypeDefaultIdAndStorageId (
  IN        FORMSET_DEFAULTSTORE        *DefaultStore,
  IN        FORMSET_STORAGE             *Storage,
  IN        FORM_BROWSER_FORMSET        *FormSet,
  OUT       LIST_ENTRY                  *UefiDefaultsListHead
 )
{
  UEFI_IFR_BUFFER_STORAGE_NODE        *Node;
  LIST_ENTRY              *Link;
  FORM_BROWSER_FORM       *Form;
  EFI_STATUS              Status;

  Node = AllocateZeroPool (sizeof (UEFI_IFR_BUFFER_STORAGE_NODE));
  ASSERT (Node != NULL);

  Node->Signature = UEFI_IFR_BUFFER_STORAGE_NODE_SIGNATURE;
  Node->Name      = AllocateCopyPool (StrSize (Storage->Name), Storage->Name);
  Node->DefaultId = DefaultStore->DefaultId;
  Node->StoreId   = Storage->VarStoreId;
  CopyGuid (&Node->Guid, &Storage->Guid);
  Node->Size      = Storage->Size;
  Node->Buffer    = AllocateZeroPool (Node->Size);
  //
  // Extract default from IFR binary
  //
  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (Link);

    Status = ExtractFormDefault (Form, DefaultStore->DefaultId, Storage->VarStoreId, Node);
    ASSERT_EFI_ERROR (Status);

    Link = GetNextNode (&FormSet->FormListHead, Link);
  }

  InsertTailList (UefiDefaultsListHead, &Node->List);
  
  return EFI_SUCCESS;
}


/**
  Get the default value for Buffer Type storage named by
  a Default Store from a FormSet.
  The result is in the a instance of UEFI_IFR_BUFFER_STORAGE_NODE
  allocated by this function. The output can be multiple instances
  of UEFI_IFR_BUFFER_STORAGE_NODE. It is inserted to the link list.
  
  @param  DefaultStore            The Default Store.
  @param  FormSet                  The Form Set.
  @param  UefiDefaultsListHead The head of link list for the output.

  @retval   EFI_SUCCESS          Successful.
  
**/
EFI_STATUS
GetBufferTypeDefaultId (
  IN  FORMSET_DEFAULTSTORE  *DefaultStore,
  IN  FORM_BROWSER_FORMSET  *FormSet,
  OUT       LIST_ENTRY      *UefiDefaultsListHead
  )
{
  LIST_ENTRY                  *StorageLink;
  FORMSET_STORAGE             *Storage;
  EFI_STATUS                  Status;

  StorageLink = GetFirstNode (&FormSet->StorageListHead);

  while (!IsNull (&FormSet->StorageListHead, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK(StorageLink);

    if (Storage->Type == EFI_HII_VARSTORE_BUFFER) {
      Status = GetBufferTypeDefaultIdAndStorageId (DefaultStore, Storage, FormSet, UefiDefaultsListHead);
      ASSERT_EFI_ERROR (Status);
    }

    StorageLink = GetNextNode (&FormSet->StorageListHead, StorageLink);
  }
  
  return EFI_SUCCESS;
}


/**
  Get the default value for Buffer Type storage from the FormSet in ThunkContext.
  
  The results can be multiple instances of UEFI_IFR_BUFFER_STORAGE_NODE. 
  They are inserted to the link list.
  
  @param  ThunkContext  Hii thunk context.
  @param  UefiDefaults  The head of link list for the output.

  @retval   EFI_SUCCESS          Successful.
  
**/
EFI_STATUS
UefiIfrGetBufferTypeDefaults (
  IN  HII_THUNK_CONTEXT   *ThunkContext,
  OUT LIST_ENTRY          **UefiDefaults
  )
{
  LIST_ENTRY            *DefaultLink;
  FORMSET_DEFAULTSTORE  *DefaultStore;
  EFI_STATUS            Status;

  ASSERT (UefiDefaults != NULL);

  *UefiDefaults = AllocateZeroPool (sizeof (LIST_ENTRY));
  ASSERT (*UefiDefaults != NULL);
  InitializeListHead (*UefiDefaults);

  DefaultLink = GetFirstNode (&ThunkContext->FormSet->DefaultStoreListHead);
  while (!IsNull (&ThunkContext->FormSet->DefaultStoreListHead, DefaultLink)) {
    DefaultStore = FORMSET_DEFAULTSTORE_FROM_LINK(DefaultLink);

    Status = GetBufferTypeDefaultId (DefaultStore, ThunkContext->FormSet, *UefiDefaults);
    ASSERT_EFI_ERROR (Status);

    DefaultLink = GetNextNode (&ThunkContext->FormSet->DefaultStoreListHead, DefaultLink);    
  }

  return EFI_SUCCESS;
}


/**
  Convert the UEFI Buffer Type default values to a Framework HII default
  values specified by a EFI_HII_VARIABLE_PACK_LIST structure.
  
  @param  ListHead             The link list of UEFI_IFR_BUFFER_STORAGE_NODE
                               which contains the default values retrived from a UEFI form set.
  @param  DefaultMask          The default mask.
                               The valid values are EFI_IFR_FLAG_DEFAULT and EFI_IFR_FLAG_MANUFACTURING.
                               UEFI spec only map EFI_IFR_FLAG_DEFAULT and EFI_IFR_FLAG_MANUFACTURING 
                               from specification to valid default class.
  @param  UefiFormSetDefaultVarStoreId
                               ID of the default varstore in FormSet.
  @param  VariablePackList     The output default value in a format defined in Framework.

  @retval   EFI_SUCCESS                Successful.
  @retval   EFI_INVALID_PARAMETER      The default mask is not EFI_IFR_FLAG_DEFAULT or 
                                       EFI_IFR_FLAG_MANUFACTURING.
**/
EFI_STATUS
UefiDefaultsToFwDefaults (
  IN     LIST_ENTRY                  *ListHead,
  IN     UINTN                       DefaultMask,
  IN     EFI_VARSTORE_ID             UefiFormSetDefaultVarStoreId,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  )
{
  LIST_ENTRY                        *List;
  UEFI_IFR_BUFFER_STORAGE_NODE      *Node;
  UINTN                             Size;
  UINTN                             Count;
  UINT16                            DefaultId;
  EFI_HII_VARIABLE_PACK             *Pack;
  EFI_HII_VARIABLE_PACK_LIST        *PackList;
  UINTN                             Index;

  if (DefaultMask == EFI_IFR_FLAG_DEFAULT) {
    DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
  } else if (DefaultMask == EFI_IFR_FLAG_MANUFACTURING) {
    DefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
  } else {
    //
    // UEFI spec only map EFI_IFR_FLAG_DEFAULT and EFI_IFR_FLAG_MANUFACTURING 
    // from specification to valid default class.
    //
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Calculate the size of the output EFI_HII_VARIABLE_PACK_LIST structure
  //
  Size = 0;
  Count = 0;
  List = GetFirstNode (ListHead);
  while (!IsNull (ListHead, List)) {
    Node = UEFI_IFR_BUFFER_STORAGE_NODE_FROM_LIST(List);

    if (Node->DefaultId == DefaultId) {
      Size += Node->Size;
      Size += StrSize (Node->Name);

      Count++;
    }
    
    List = GetNextNode (ListHead, List);  
  }

  if (Count == 0) {
    *VariablePackList = NULL;
    return EFI_NOT_FOUND;
  }

  Size = Size + Count * (sizeof (EFI_HII_VARIABLE_PACK_LIST) + sizeof (EFI_HII_VARIABLE_PACK));
  
  *VariablePackList = AllocateZeroPool (Size);
  ASSERT (*VariablePackList != NULL);

  List = GetFirstNode (ListHead);

  PackList = (EFI_HII_VARIABLE_PACK_LIST *) *VariablePackList;
  Pack     = (EFI_HII_VARIABLE_PACK *) (PackList + 1);
  Index = 0;
  while (!IsNull (ListHead, List)) {
    Node = UEFI_IFR_BUFFER_STORAGE_NODE_FROM_LIST(List);

    Size = 0;    
    if (Node->DefaultId == DefaultId) {
      Size += Node->Size;
      Size += sizeof (EFI_HII_VARIABLE_PACK);      

      Pack->VariableNameLength = (UINT32) StrSize (Node->Name);

      if (Node->StoreId == UefiFormSetDefaultVarStoreId) {
        //
        // The default VARSTORE in VFR from a Framework module has Varstore ID of 0.
        //
        Pack->VariableId = 0;
      } else {
        Pack->VariableId = Node->StoreId;
      }

      CopyMem ((UINT8 *) Pack + sizeof (EFI_HII_VARIABLE_PACK), Node->Name, StrSize (Node->Name));
      Size += Pack->VariableNameLength;

      //
      // Initialize EFI_HII_VARIABLE_PACK
      //
      Pack->Header.Type   = 0;
      Pack->Header.Length = (UINT32) Size;
      CopyMem (&Pack->VariableGuid, &Node->Guid, sizeof (EFI_GUID));
      
      CopyMem ((UINT8 *) Pack + sizeof (EFI_HII_VARIABLE_PACK) + Pack->VariableNameLength, Node->Buffer, Node->Size);

      Size += sizeof (EFI_HII_VARIABLE_PACK_LIST);

      //
      // Initialize EFI_HII_VARIABLE_PACK_LIST
      //
      PackList->VariablePack = Pack;
      Index++;
      if (Index < Count) {
        PackList->NextVariablePack = (EFI_HII_VARIABLE_PACK_LIST *)((UINT8 *) PackList + Size);

        PackList = PackList->NextVariablePack;
        Pack     = (EFI_HII_VARIABLE_PACK *) (PackList + 1);
      }
            
    }
    
    List = GetNextNode (ListHead, List);  
  }
  
  
  return EFI_SUCCESS;
}


/**
  Free up all buffer allocated for the link list of UEFI_IFR_BUFFER_STORAGE_NODE.
    
  @param  ListHead                  The link list of UEFI_IFR_BUFFER_STORAGE_NODE
                                    which contains the default values retrived from
                                    a UEFI form set.

**/
VOID
FreeDefaultList (
  IN     LIST_ENTRY                  *ListHead
  )
{
  LIST_ENTRY *Link;
  UEFI_IFR_BUFFER_STORAGE_NODE *Default;

  while (!IsListEmpty (ListHead)) {
    Link = GetFirstNode (ListHead);
    
    Default = UEFI_IFR_BUFFER_STORAGE_NODE_FROM_LIST(Link);

    RemoveEntryList (Link);
   
    DestroyDefaultNode (Default);
  }

  FreePool (ListHead);
}

