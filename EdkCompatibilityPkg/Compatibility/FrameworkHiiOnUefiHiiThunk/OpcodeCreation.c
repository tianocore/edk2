/** @file
Implement Functions to convert IFR Opcode in format defined in Framework HII specification to
format defined in UEFI HII Specification.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"
#include "UefiIfrDefault.h"

/**
  The dynamic creation of these opcodes is supported in Framework HII modules.
  Therefore, Framework HII Thunk module only map these opcode between Framework
  HII's definitions to UEFI HII's.
**/
typedef struct { 
  UINT8 FrameworkIfrOp;
  UINT8 UefiIfrOp;
} IFR_OPCODE_MAP;
  
IFR_OPCODE_MAP QuestionOpcodeMap[] = {
  { FRAMEWORK_EFI_IFR_ONE_OF_OP,        EFI_IFR_ONE_OF_OP},
  { FRAMEWORK_EFI_IFR_CHECKBOX_OP,      EFI_IFR_CHECKBOX_OP},
  { FRAMEWORK_EFI_IFR_NUMERIC_OP,       EFI_IFR_NUMERIC_OP},
  { FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP, EFI_IFR_ONE_OF_OPTION_OP},
  { FRAMEWORK_EFI_IFR_ORDERED_LIST_OP,  EFI_IFR_ORDERED_LIST_OP}
};

/**
  Translate a Framework Question Opcode to UEFI Question Opcode.

  @param FwOp     Framework Opcode.
  @param UefiOp   UEFI Opcode.

  @retval     EFI_SUCCESS     The UEFI opcode is found and returned.
  @retval     EFI_NOT_FOUND   The UEFI opcode is not found.
**/
EFI_STATUS
QuestionOpFwToUefi (
  IN            UINT8   FwOp,
  OUT           UINT8   *UefiOp
  )
{
  UINTN       Index;

  for (Index = 0; Index < sizeof (QuestionOpcodeMap) / sizeof (QuestionOpcodeMap[0]); Index++) {
    if (FwOp == QuestionOpcodeMap[Index].FrameworkIfrOp) {
      *UefiOp = QuestionOpcodeMap[Index].UefiIfrOp;
      return EFI_SUCCESS;
    }
  }

  *UefiOp = (UINT8) (EFI_IFR_LAST_OPCODE + 1);
  return EFI_NOT_FOUND;
}

/**
  Translate a Framework Question ID to UEFI Question ID.

  @param FormSet   FormSet context
  @param FwOpCode  Framework Opcode
  @param FwQId     Framework Question Id
  @param UefiQId   UEFI Question ID.

  @retval     EFI_SUCCESS     The UEFI Question Id is found and returned.
  @retval     EFI_NOT_FOUND   The UEFI Question Id is not found.
**/
EFI_STATUS
FwQIdToUefiQId (
  IN  CONST FORM_BROWSER_FORMSET *FormSet,
  IN  UINT8                      FwOpCode,
  IN  UINT16                     FwQId,
  OUT UINT16                     *UefiQId
  )
{
  LIST_ENTRY             *FormList;
  LIST_ENTRY             *StatementList;
  FORM_BROWSER_FORM      *Form;
  FORM_BROWSER_STATEMENT *Statement;
  FORM_BROWSER_STATEMENT *StatementFound;
  EFI_STATUS             Status;
  UINT8                  UefiOp;
  

  *UefiQId = 0;
  StatementFound = NULL;

  FormList = GetFirstNode (&FormSet->FormListHead);

  while (!IsNull (&FormSet->FormListHead, FormList)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (FormList);

    StatementList = GetFirstNode (&Form->StatementListHead);

    while (!IsNull (&Form->StatementListHead, StatementList)) {
      Statement = FORM_BROWSER_STATEMENT_FROM_LINK (StatementList);
      if (Statement->VarStoreId != 0 && Statement->Storage->Type == EFI_HII_VARSTORE_BUFFER) {
        if (FwQId == Statement->VarStoreInfo.VarOffset) {
          Status = QuestionOpFwToUefi (FwOpCode, &UefiOp);
          ASSERT_EFI_ERROR (Status);

          if ((UefiOp == Statement->Operand) && (FormSet->DefaultVarStoreId == Statement->VarStoreId)) {
            //
            // If ASSERT here, the Framework VFR file has two Questions with all three attibutes the same:
            // 1) Same Question Type, 
            // 2) Same Variable Storage
            // 3) Refering to the Same offset in Variable Map (NvMap). 
            // This is ambigurity as FwQIdToUefiQId () can't find which UEFI Question 
            // ID to return. 
            //
            // One possible solution is to remove the one of the duplicated questions in this Form Set.
            //
            ASSERT (StatementFound == NULL);
            StatementFound= Statement;

            //
            // Continue the search to check if the Form Set contains more than one questins that has the 3 attributes
            // with same value.
            //
          }
        }
      }

      StatementList = GetNextNode (&Form->StatementListHead, StatementList);
    }

    FormList = GetNextNode (&FormSet->FormListHead, FormList);
  }

  if (StatementFound != NULL) {
    *UefiQId = StatementFound->QuestionId;
    return EFI_SUCCESS;
  }
  
  return EFI_NOT_FOUND;
}

/**
  Assign a Question ID.

  If FwQuestionId is 0, then assign a new question ID. The new question ID
  is MaxQuestionId incremented by 1. The MaxQuestionId of FormSet is also
  incremented by 1.

  If FwQuestionId is not 0, then it is used as the Framework Question ID.

  @param FwQuestionId 
  @param FormSet      

  @return The Framework Question ID.
**/
EFI_QUESTION_ID
AssignQuestionId (
  IN  UINT16                      FwQuestionId,
  IN        FORM_BROWSER_FORMSET  *FormSet
  )
{
  if (FwQuestionId == 0) {
    FormSet->MaxQuestionId++;
    return FormSet->MaxQuestionId;
  } else {
    return FwQuestionId;
  }
}

/**
  Create UEFI HII Text Opcode from a Framework HII Text Opcode.

  @param UefiUpdateDataHandle  The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param FwOpcode              The input Framework Opcode.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateTextOpCode (
  IN OUT   VOID                        *UefiUpdateDataHandle,
  IN CONST FRAMEWORK_EFI_IFR_TEXT      *FwOpcode
  )
{
  EFI_IFR_TEXT      UTextOpCode;

  if ((FwOpcode->Flags & EFI_IFR_FLAG_INTERACTIVE) == 0) {
    ZeroMem (&UTextOpCode, sizeof(UTextOpCode));
    
    UTextOpCode.Header.OpCode = EFI_IFR_TEXT_OP;
    UTextOpCode.Header.Length = (UINT8) sizeof (EFI_IFR_TEXT);

    UTextOpCode.Statement.Help   = FwOpcode->Help;

    UTextOpCode.Statement.Prompt = FwOpcode->Text;
    UTextOpCode.TextTwo          = FwOpcode->TextTwo;
    
    return HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UTextOpCode, sizeof(UTextOpCode));
  } else {
    //
    // Iteractive Text Opcode is EFI_IFR_ACTION
    //
    return HiiCreateActionOpCode (UefiUpdateDataHandle, FwOpcode->Key, FwOpcode->Text, FwOpcode->Help, EFI_IFR_FLAG_CALLBACK, 0);
  }
}

/**
  Create UEFI HII Reference Opcode from a Framework HII Reference Opcode.

  @param UefiUpdateDataHandle  The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param FwOpcode              The input Framework Opcode.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateReferenceOpCode (
  IN OUT   VOID                        *UefiUpdateDataHandle,
  IN CONST FRAMEWORK_EFI_IFR_REF       *FwOpcode
  )
{
  EFI_IFR_REF UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = (UINT8) sizeof (UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_REF_OP;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;
  UOpcode.Question.QuestionId = FwOpcode->Key;

  UOpcode.FormId = FwOpcode->FormId;

  //
  // We only map EFI_IFR_FLAG_INTERACTIVE and EFI_IFR_FLAG_RESET_REQUIRED to 
  // UEFI IFR Opcode flags. The rest flags are obsolete.
  //
  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_RESET_REQUIRED));
  
  return HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcode, sizeof(UOpcode));
}

/**
  Create UEFI HII "One Of Option" Opcode from a Framework HII "One Of Option" Opcode.

  @param UefiUpdateDataHandle  The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param FwOpcode              The input Framework Opcode.
  @param Width                 The size of the One Of Option. 1 bytes or 2 bytes.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateOneOfOptionOpCode (
  IN OUT   VOID                               *UefiUpdateDataHandle,
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF_OPTION    *FwOpcode,
  IN       UINTN                              Width
  )
{
  EFI_IFR_ONE_OF_OPTION UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = (UINT8) sizeof (UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;

  UOpcode.Option        = FwOpcode->Option;
  CopyMem (&UOpcode.Value.u8, &FwOpcode->Value, Width);

  //
  // #define EFI_IFR_FLAG_DEFAULT           0x01
  // #define EFI_IFR_FLAG_MANUFACTURING     0x02
  // #define EFI_IFR_OPTION_DEFAULT                   0x10
  // #define EFI_IFR_OPTION_DEFAULT_MFG               0x20
  //
  UOpcode.Flags = (UINT8) (UOpcode.Flags  | (FwOpcode->Flags & (EFI_IFR_FLAG_DEFAULT | EFI_IFR_FLAG_MANUFACTURING)) << 4);

  switch (Width) {
    case 1:
      UOpcode.Type = EFI_IFR_TYPE_NUM_SIZE_8;
      break;
      
    case 2:
      UOpcode.Type = EFI_IFR_TYPE_NUM_SIZE_16;
      break;
      
    default:
      ASSERT (FALSE);
      return NULL;
  }

  return HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcode, sizeof(UOpcode));
}

/**
  Create a GUID Opcode EFI_IFR_GUID_OPTIONKEY to map the Framework One Of Option callback key
  to a UEFI Question ID. This information is used to invoke the Framework HII Browser Callback
  function. The opcode is appened to UefiUpdateDataHandle.

  @param    UefiUpdateDataHandle  The UEFI Update Data buffer.
  @param    QuestionId            The UEFI Question ID.
  @param    OptionValue           The value of the "One Of Option".
  @param    KeyValue              The Framework "One Of Option" callback key.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
**/
UINT8 *
CreateGuidOptionKeyOpCode (
  IN OUT   VOID                        *UefiUpdateDataHandle,
  IN EFI_QUESTION_ID                   QuestionId,
  IN UINT16                            OptionValue,
  IN EFI_QUESTION_ID                   KeyValue
  )
{
  EFI_IFR_GUID_OPTIONKEY              *UOpcode;
  
  UOpcode = (EFI_IFR_GUID_OPTIONKEY *) HiiCreateGuidOpCode (
                                         UefiUpdateDataHandle, 
                                         &gEfiIfrFrameworkGuid, 
                                         NULL,
                                         sizeof (EFI_IFR_GUID_OPTIONKEY)
                                         );

  UOpcode->ExtendOpCode = EFI_IFR_EXTEND_OP_OPTIONKEY;
  UOpcode->QuestionId  = QuestionId;
  CopyMem (&UOpcode->OptionValue, &OptionValue, sizeof (OptionValue)); 
  UOpcode->KeyValue = KeyValue;

  return (UINT8 *) UOpcode;
}

/**
  Create UEFI HII "One Of" Opcode from a Framework HII "One Of" Opcode.

  @param UefiUpdateDataHandle     The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param ThunkContext             The HII Thunk Context.
  @param FwOpcode                 The input Framework Opcode.
  @param NextFwOpcode             Returns the position of the next Framework Opcode after EFI_IFR_END_ONE_OF_OP of
                                  the "One Of Option".
  @param OpcodeCount              The number of Opcode for the complete Framework "One Of" Opcode.
                      
  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateOneOfOpCode (
  IN OUT   VOID                        *UefiUpdateDataHandle,
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF    *FwOpcode,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER **NextFwOpcode,
  OUT      UINTN                       *OpcodeCount
  )
{
  EFI_STATUS                          Status;
  EFI_IFR_ONE_OF                      UOpcode;
  FRAMEWORK_EFI_IFR_OP_HEADER         *FwOpHeader;
  FRAMEWORK_EFI_IFR_ONE_OF_OPTION     *FwOneOfOp;
  UINT8                               *OpCodeBuffer;
  UINT8                               *OneOfOpCodeBuffer;

  ASSERT (NextFwOpcode != NULL);
  ASSERT (OpcodeCount != NULL);

  ZeroMem (&UOpcode, sizeof(UOpcode));
  *OpcodeCount = 0;

  UOpcode.Header.Length = (UINT8) sizeof (UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_ONE_OF_OP;
  UOpcode.Header.Scope  = 1;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;
  UOpcode.Question.VarStoreId  = ThunkContext->FormSet->DefaultVarStoreId;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;
  
  //
  // Go over the Framework IFR binary to get the QuestionId for generated UEFI One Of Option opcode
  //
  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != EFI_IFR_END_ONE_OF_OP) {
    ASSERT (FwOpHeader->OpCode == FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP);
    
    FwOneOfOp = (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader;
    if ((FwOneOfOp->Flags & EFI_IFR_FLAG_INTERACTIVE) != 0) {
      UOpcode.Question.Flags |= EFI_IFR_FLAG_CALLBACK;
      
      if (UOpcode.Question.QuestionId == 0) {
        Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
        if (EFI_ERROR (Status)) {
          UOpcode.Question.QuestionId = AssignQuestionId (FwOneOfOp->Key, ThunkContext->FormSet);
        }
      }

    }

    if ((FwOneOfOp->Flags & EFI_IFR_FLAG_RESET_REQUIRED) == EFI_IFR_FLAG_RESET_REQUIRED) {
      UOpcode.Question.Flags |= EFI_IFR_FLAG_RESET_REQUIRED;
    }

    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
  }


  if (UOpcode.Question.QuestionId == 0) {
    //
    // Assign QuestionId if still not assigned.
    //
    Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
    if (EFI_ERROR (Status)) {
      UOpcode.Question.QuestionId = AssignQuestionId (FwOpcode->QuestionId, ThunkContext->FormSet);
    }
  }
  
  OneOfOpCodeBuffer = HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcode, sizeof (UOpcode));
  if (OneOfOpCodeBuffer == NULL) {
    return NULL;
  }
  *OpcodeCount += 1;

  //
  // Go over again the Framework IFR binary to build the UEFI One Of Option opcodes.
  //
  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != EFI_IFR_END_ONE_OF_OP) {

    FwOneOfOp = (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader;
      
    OpCodeBuffer = F2UCreateOneOfOptionOpCode (UefiUpdateDataHandle, (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader, FwOpcode->Width);
    if (OpCodeBuffer == NULL) {
      return NULL;
    }

    OpCodeBuffer = CreateGuidOptionKeyOpCode (UefiUpdateDataHandle, UOpcode.Question.QuestionId, FwOneOfOp->Value, FwOneOfOp->Key);
    if (OpCodeBuffer == NULL) {
      return NULL;
    }

    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  OpCodeBuffer = HiiCreateEndOpCode (UefiUpdateDataHandle);
  if (OpCodeBuffer != NULL) {
    *NextFwOpcode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  return OneOfOpCodeBuffer;
}

/**
  Create UEFI HII "Ordered List" Opcode from a Framework HII "Ordered List" Opcode.

  @param UefiUpdateDataHandle The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param ThunkContext         The HII Thunk Context.
  @param FwOpcode             The input Framework Opcode.
  @param NextFwOpcode         Returns the position of the next Framework Opcode after EFI_IFR_END_ONE_OF_OP of
                              the "Ordered List".
  @param OpcodeCount          The number of Opcode for the complete Framework "Ordered List" Opcode.
                      
  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateOrderedListOpCode (
  IN OUT    VOID                          *UefiUpdateDataHandle,
  IN       HII_THUNK_CONTEXT              *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_ORDERED_LIST *FwOpcode,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER    **NextFwOpcode,
  OUT      UINTN                          *OpcodeCount
  )
{
  EFI_IFR_ORDERED_LIST              UOpcode;
  EFI_STATUS                        Status;
  FRAMEWORK_EFI_IFR_OP_HEADER       *FwOpHeader;
  FRAMEWORK_EFI_IFR_ONE_OF_OPTION   *FwOneOfOp;
  UINT8                             *OpcodeBuffer;   
  UINT8                             *OrderListOpCode;

  ZeroMem (&UOpcode, sizeof(UOpcode));
  *OpcodeCount = 0;

  UOpcode.Header.Length = (UINT8) sizeof (UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_ORDERED_LIST_OP;
  UOpcode.Header.Scope  = 1;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;
  UOpcode.Question.VarStoreId  = ThunkContext->FormSet->DefaultVarStoreId;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  UOpcode.MaxContainers = FwOpcode->MaxEntries;

  //
  // Go over the Framework IFR binary to get the QuestionId for generated UEFI One Of Option opcode
  //
  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != EFI_IFR_END_ONE_OF_OP) {
    ASSERT (FwOpHeader->OpCode == FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP);
    
    FwOneOfOp = (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader;
    if ((FwOneOfOp->Flags & EFI_IFR_FLAG_INTERACTIVE) != 0) {
      UOpcode.Question.Flags |= EFI_IFR_FLAG_CALLBACK;
      
      if (UOpcode.Question.QuestionId == 0) {
        Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
        if (EFI_ERROR (Status)) {
          UOpcode.Question.QuestionId = AssignQuestionId (FwOneOfOp->Key, ThunkContext->FormSet);
        }

      }
    }

    if ((FwOneOfOp->Flags & EFI_IFR_FLAG_RESET_REQUIRED) ==  EFI_IFR_FLAG_RESET_REQUIRED) {
      UOpcode.Question.Flags |= EFI_IFR_FLAG_RESET_REQUIRED;
    }

    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
  }

  if (UOpcode.Question.QuestionId == 0) {
    Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
    if (EFI_ERROR (Status)) {
      UOpcode.Question.QuestionId = AssignQuestionId (FwOpcode->QuestionId, ThunkContext->FormSet);
    }
  }
 
  OrderListOpCode = HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcode, sizeof(UOpcode));
  if (OrderListOpCode == NULL) {
    return NULL;
  }
  *OpcodeCount += 1;

  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != EFI_IFR_END_ONE_OF_OP) {
    //
    // Each entry of Order List in Framework HII is always 1 byte in size
    //
    OpcodeBuffer = F2UCreateOneOfOptionOpCode (UefiUpdateDataHandle, (CONST FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader, 1);
    if (OpcodeBuffer == NULL) {
      return NULL;
    }
    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  OpcodeBuffer = HiiCreateEndOpCode (UefiUpdateDataHandle);
  if (OpcodeBuffer != NULL) {
    *NextFwOpcode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  return OrderListOpCode;
}

/**
  Create UEFI HII CheckBox Opcode from a Framework HII Checkbox Opcode.

  @param UefiUpdateDataHandle  The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param ThunkContext          The HII Thunk Context.
  @param FwOpcode              The input Framework Opcode.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateCheckBoxOpCode (
  IN OUT   VOID                        *UefiUpdateDataHandle,
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_CHECKBOX  *FwOpcode
  )
{
  EFI_STATUS       Status;
  EFI_IFR_CHECKBOX UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = (UINT8) sizeof (UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_CHECKBOX_OP;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  if (FwOpcode->Key == 0) {
    Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
    if (EFI_ERROR (Status)) {
      //
      // Add a new opcode and it will not trigger call back. So we just reuse the FW QuestionId.
      //
      UOpcode.Question.QuestionId = AssignQuestionId (FwOpcode->QuestionId, ThunkContext->FormSet);
    }
  } else {
    UOpcode.Question.QuestionId    = FwOpcode->Key;
  }

  //
  // We map 2 flags:
  //      EFI_IFR_FLAG_INTERACTIVE, 
  //      EFI_IFR_FLAG_RESET_REQUIRED,
  // to UEFI IFR Opcode Question flags. The rest flags are obsolete.
  //
  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_RESET_REQUIRED));


  UOpcode.Question.VarStoreId    = ThunkContext->FormSet->DefaultVarStoreId;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  //
  // We also map these 2 flags:
  //      EFI_IFR_FLAG_DEFAULT, 
  //      EFI_IFR_FLAG_MANUFACTURING,
  // to UEFI IFR CheckBox Opcode default flags.
  //
  UOpcode.Flags           = (UINT8) (FwOpcode->Flags & (EFI_IFR_FLAG_DEFAULT | EFI_IFR_FLAG_MANUFACTURING));

  return HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcode, sizeof(UOpcode));
}


/**
  Create UEFI HII Numeric Opcode from a Framework HII Numeric Opcode.

  @param UefiUpdateDataHandle    The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param ThunkContext            The HII Thunk Context.
  @param FwOpcode                The input Framework Opcode.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateNumericOpCode (
  IN OUT   VOID                        *UefiUpdateDataHandle,
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_NUMERIC   *FwOpcode
  )
{
  EFI_STATUS      Status;
  EFI_IFR_NUMERIC UOpcode;
  EFI_IFR_DEFAULT UOpcodeDefault;
  UINT8           *NumbericOpCode;
  UINT8           *OpcodeBuffer;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  if (FwOpcode->Key == 0) {
    Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
    if (EFI_ERROR (Status)) {
      //
      // Add a new opcode and it will not trigger call back. So we just reuse the FW QuestionId.
      //
      UOpcode.Question.QuestionId = AssignQuestionId (FwOpcode->QuestionId, ThunkContext->FormSet);
    }
  } else {
    UOpcode.Question.QuestionId    = FwOpcode->Key;
  }

  UOpcode.Header.Length = (UINT8) sizeof (UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_NUMERIC_OP;
  //
  // We need to create a nested default value for the UEFI Numeric Opcode.
  // So turn on the scope.
  //
  UOpcode.Header.Scope = 1;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  UOpcode.Question.VarStoreId    = ThunkContext->FormSet->DefaultVarStoreId;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_RESET_REQUIRED));

  //
  // Framework Numeric values are all in UINT16 and displayed as decimal.
  //
  UOpcode.data.u16.MinValue = FwOpcode->Minimum;
  UOpcode.data.u16.MaxValue = FwOpcode->Maximum;
  UOpcode.data.u16.Step = FwOpcode->Step;

  switch (FwOpcode->Width) {
    case 1: 
    {
      UOpcode.Flags           =  EFI_IFR_NUMERIC_SIZE_1 | EFI_IFR_DISPLAY_UINT_DEC; 
      break;
    } 
    case 2: 
    {
      UOpcode.Flags           =  EFI_IFR_NUMERIC_SIZE_2 | EFI_IFR_DISPLAY_UINT_DEC; 
      break;
    }
    default: 
    {
      ASSERT (FALSE);
      return NULL;
    }
  }
  
  NumbericOpCode = HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcode, sizeof(UOpcode));
  if (NumbericOpCode == NULL) {
    return NULL;
  }

  //
  // We need to create a default value.
  //
  ZeroMem (&UOpcodeDefault, sizeof (UOpcodeDefault));
  UOpcodeDefault.Header.Length = (UINT8) sizeof (UOpcodeDefault);
  UOpcodeDefault.Header.OpCode = EFI_IFR_DEFAULT_OP;

  UOpcodeDefault.DefaultId = 0;

  switch (FwOpcode->Width) {
    case 1: 
    {
      UOpcodeDefault.Type = EFI_IFR_TYPE_NUM_SIZE_8;
      break;
    } 
    case 2: 
    {
      UOpcodeDefault.Type = EFI_IFR_TYPE_NUM_SIZE_16;
      break;
    }
  }

  CopyMem (&UOpcodeDefault.Value.u8, &FwOpcode->Default, FwOpcode->Width);

  OpcodeBuffer = HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcodeDefault, sizeof(UOpcodeDefault));
  if (OpcodeBuffer == NULL) {
    return NULL;
  }

  OpcodeBuffer = HiiCreateEndOpCode (UefiUpdateDataHandle);
  if (OpcodeBuffer == NULL) {
    return NULL;
  }

  return NumbericOpCode;
}


/**
  Create UEFI HII String Opcode from a Framework HII String Opcode.

  @param UefiUpdateDataHandle     The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param ThunkContext             The HII Thunk Context.
  @param FwOpcode                 The input Framework Opcode.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateStringOpCode (
  IN OUT   VOID                        *UefiUpdateDataHandle,
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_STRING    *FwOpcode
  )
{
  EFI_IFR_STRING UOpcode;
  EFI_STATUS     Status;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  if (FwOpcode->Key == 0) {
    Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
    if (EFI_ERROR (Status)) {
      //
      // Add a new opcode and it will not trigger call back. So we just reuse the FW QuestionId.
      //
      UOpcode.Question.QuestionId = AssignQuestionId (FwOpcode->QuestionId, ThunkContext->FormSet);
    }
  } else {
    UOpcode.Question.QuestionId    = FwOpcode->Key;
  }

  UOpcode.Header.Length = (UINT8) sizeof (UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_STRING_OP;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_RESET_REQUIRED));

  UOpcode.Question.VarStoreId    = ThunkContext->FormSet->DefaultVarStoreId;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  UOpcode.MinSize = FwOpcode->MinSize;
  UOpcode.MaxSize = FwOpcode->MaxSize;
  UOpcode.Flags   = EFI_IFR_STRING_MULTI_LINE;

  return HiiCreateRawOpCodes (UefiUpdateDataHandle, (UINT8 *) &UOpcode, sizeof(UOpcode));
}

/**
  Create UEFI HII Banner Opcode from a Framework HII Banner Opcode.

  @param UefiUpdateDataHandle     The newly created UEFI HII opcode is appended to UefiUpdateDataHandle.
  @param FwOpcode                 The input Framework Opcode.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.
  
**/
UINT8 *
F2UCreateBannerOpCode (
  IN OUT   VOID              *UefiUpdateDataHandle,
  IN CONST EFI_IFR_BANNER    *FwOpcode
  )
{
  EFI_IFR_GUID_BANNER *UOpcode;

  UOpcode = (EFI_IFR_GUID_BANNER *) HiiCreateGuidOpCode (
                                      UefiUpdateDataHandle, 
                                      &gEfiIfrTianoGuid, 
                                      NULL,
                                      sizeof (EFI_IFR_GUID_BANNER)
                                      );  

  UOpcode->ExtendOpCode = EFI_IFR_EXTEND_OP_BANNER;
  UOpcode->Title          = FwOpcode->Title;
  UOpcode->LineNumber     = FwOpcode->LineNumber;
  UOpcode->Alignment      = FwOpcode->Alignment;

  return (UINT8 *) UOpcode;
}

/**
  Create a Hii Update data Handle used to call IfrLibUpdateForm.

  @param ThunkContext         The HII Thunk Context.
  @param FwUpdateData         The Framework Update Data.
  @param UefiOpCodeHandle     The UEFI opcode handle.

  @retval EFI_SUCCESS       The UEFI Update Data is created successfully.
  @retval EFI_UNSUPPORTED   There is unsupported opcode in FwUpdateData.
  @retval EFI_OUT_OF_RESOURCES There is not enough resource.
**/
EFI_STATUS 
FwUpdateDataToUefiUpdateData (
  IN       HII_THUNK_CONTEXT    *ThunkContext,
  IN CONST EFI_HII_UPDATE_DATA  *FwUpdateData,
  IN       VOID                 *UefiOpCodeHandle
  )
{
  FRAMEWORK_EFI_IFR_OP_HEADER          *FwOpCode;
  FRAMEWORK_EFI_IFR_OP_HEADER          *NextFwOpCode;
  UINTN                                Index;
  UINTN                                DataCount;
  UINT8                                *OpCodeBuffer;
  LIST_ENTRY                           *StorageList;
  FORMSET_STORAGE                      *Storage;
  FORM_BROWSER_FORMSET                 *FormSet;
  CHAR16                               *DefaultVarStoreName;
  UINT16                               DefaultVarStoreId;
  EFI_IFR_VARSTORE_SELECT              *SelectVarOp;

  FwOpCode = (FRAMEWORK_EFI_IFR_OP_HEADER *) &FwUpdateData->Data;

  FormSet = ThunkContext->FormSet;
  DefaultVarStoreId   = FormSet->DefaultVarStoreId;
  DefaultVarStoreName = FormSet->OriginalDefaultVarStoreName;

  for (Index = 0; Index < FwUpdateData->DataCount; Index += DataCount) {
    switch (FwOpCode->OpCode) {
      case FRAMEWORK_EFI_IFR_SUBTITLE_OP:
        OpCodeBuffer = HiiCreateSubTitleOpCode (UefiOpCodeHandle, ((FRAMEWORK_EFI_IFR_SUBTITLE  *) FwOpCode)->SubTitle, 0, 0, 0);
        DataCount = 1;
        break;
        
      case FRAMEWORK_EFI_IFR_TEXT_OP:
        OpCodeBuffer = F2UCreateTextOpCode (UefiOpCodeHandle, (FRAMEWORK_EFI_IFR_TEXT  *) FwOpCode);  
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_REF_OP:
        OpCodeBuffer = F2UCreateReferenceOpCode (UefiOpCodeHandle, (FRAMEWORK_EFI_IFR_REF *) FwOpCode);  
        DataCount = 1;
        break;
        
      case FRAMEWORK_EFI_IFR_ONE_OF_OP:
        OpCodeBuffer = F2UCreateOneOfOpCode (UefiOpCodeHandle, ThunkContext, (FRAMEWORK_EFI_IFR_ONE_OF *) FwOpCode, &NextFwOpCode, &DataCount);
        if (OpCodeBuffer != NULL) {
          FwOpCode = NextFwOpCode;
          //
          // FwOpCode is already updated to point to the next opcode.
          //
          continue;
        }
        break;

      case FRAMEWORK_EFI_IFR_ORDERED_LIST_OP:
        OpCodeBuffer = F2UCreateOrderedListOpCode (UefiOpCodeHandle, ThunkContext, (FRAMEWORK_EFI_IFR_ORDERED_LIST *) FwOpCode, &NextFwOpCode, &DataCount);
        if (OpCodeBuffer != NULL) {
          FwOpCode = NextFwOpCode;
          //
          // FwOpCode is already updated to point to the next opcode.
          //
          continue;
        }
        break;
        
      case FRAMEWORK_EFI_IFR_CHECKBOX_OP:
        OpCodeBuffer = F2UCreateCheckBoxOpCode (UefiOpCodeHandle, ThunkContext, (FRAMEWORK_EFI_IFR_CHECKBOX *) FwOpCode);  
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_STRING_OP:
        OpCodeBuffer = F2UCreateStringOpCode (UefiOpCodeHandle, ThunkContext, (FRAMEWORK_EFI_IFR_STRING *) FwOpCode);  
        DataCount = 1;
        break;

      case EFI_IFR_BANNER_OP:
        OpCodeBuffer = F2UCreateBannerOpCode (UefiOpCodeHandle, (EFI_IFR_BANNER *) FwOpCode);  
        DataCount = 1;
        break;

      case EFI_IFR_END_ONE_OF_OP:
        OpCodeBuffer = HiiCreateEndOpCode (UefiOpCodeHandle);
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_NUMERIC_OP:
        OpCodeBuffer = F2UCreateNumericOpCode (UefiOpCodeHandle, ThunkContext, (FRAMEWORK_EFI_IFR_NUMERIC *) FwOpCode);
        DataCount = 1;
        break;
      
      case EFI_IFR_VARSTORE_SELECT_OP:
        OpCodeBuffer = (UINT8 *) FwOpCode;
        SelectVarOp  = (EFI_IFR_VARSTORE_SELECT *) FwOpCode;
        //
        // Check whether the selected VarId is in StorageList.
        //
        StorageList = GetFirstNode (&FormSet->StorageListHead);
        while (!IsNull (&FormSet->StorageListHead, StorageList)) {
          Storage = FORMSET_STORAGE_FROM_LINK (StorageList);
          if (Storage->VarStoreId == SelectVarOp->VarId) {
            break;
          }
          StorageList = GetNextNode (&FormSet->StorageListHead, StorageList);
        }
        ASSERT (!IsNull (&FormSet->StorageListHead, StorageList));
        //
        // Change VarStoreId to the selected VarId.
        //
        FormSet->DefaultVarStoreId = SelectVarOp->VarId;
        if (SelectVarOp->VarId == DefaultVarStoreId)  {
          FormSet->OriginalDefaultVarStoreName = DefaultVarStoreName;
        }
        DataCount = 1;
        break;

      default:
        ASSERT (FALSE);
        return EFI_UNSUPPORTED;
    }

    if (OpCodeBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    FwOpCode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpCode + FwOpCode->Length);
  }

  //
  // Revert FromSet default varstore ID.
  //
  FormSet->DefaultVarStoreId           = DefaultVarStoreId;
  FormSet->OriginalDefaultVarStoreName = DefaultVarStoreName;
  return EFI_SUCCESS;
}

