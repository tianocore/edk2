/** @file
Implement Functions to convert IFR Opcode in format defined in Framework HII specification to
format defined in UEFI HII Specification.

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"
#include "UefiIfrDefault.h"

EFI_GUID  mTianoExtendedOpcodeGuid = EFI_IFR_TIANO_GUID;


EFI_IFR_GUID_OPTIONKEY mOptionKeyTemplate = {
   {EFI_IFR_GUID_OP, sizeof (EFI_IFR_GUID_OPTIONKEY), 0},
   EFI_IFR_FRAMEWORK_GUID,
   EFI_IFR_EXTEND_OP_OPTIONKEY,
   0,
   {0},
   0
};

/**
  The dynamic creation of these opcodes is supported in Framework HII modules.
  Therefore, Framework HII Thunk module only map these opcode between Framework
  HII's definitions to UEFI HII's.
**/
typedef struct { 
  UINT8 FrameworkIfrOp;
  UINT8 UefiIfrOp;
} IFR_OPCODE_MAP;
  
IFR_OPCODE_MAP mQuestionOpcodeMap [] = {
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

  for (Index = 0; Index < sizeof (mQuestionOpcodeMap) / sizeof (mQuestionOpcodeMap[0]); Index++) {
    if (FwOp == mQuestionOpcodeMap[Index].FrameworkIfrOp) {
      *UefiOp = mQuestionOpcodeMap[Index].UefiIfrOp;
      return EFI_SUCCESS;
    }
  }

  *UefiOp = (UINT8) (FRAMEWORK_EFI_IFR_LAST_OPCODE + 1);
  return EFI_NOT_FOUND;
}

/**
  Translate a Framework Question Opcode to UEFI Question Opcode.

  @param FwOp     Framework Opcode.
  @param UefiOp   UEFI Opcode.

  @retval     EFI_SUCCESS     The UEFI opcode is found and returned.
  @retval     EFI_NOT_FOUND   The UEFI opcode is not found.
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



#define LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL   0x1000
/**
  Append the newly created OpCode buffer to EFI_HII_UPDATE_DATA buffer.
  Increase the Data buffer in EFI_HII_UPDATE_DATA if the EFI_HII_UPDATE_DATA 
  buffer's Data Buffer does not have space left for the newly created
  OpCode.

  @param OpCodeBuf      The newly created OpCode Buffer to be appended to 
                        EFI_HII_UPDATE_DATA buffer.
  @param OpCodeBufSize  The size of OpCodeBuf.
  @param UefiUpdateData The EFI_HII_UPDATE_DATA to be appended.

  @retval EFI_SUCCESS   The OpCode buffer is appended to EFI_HII_UPDATE_DATA successfull.
  @retval EFI_OUT_OF_RESOURCES There is not enough memory.
**/
EFI_STATUS
AppendToUpdateBuffer (
  IN CONST  UINT8                *OpCodeBuf,
  IN        UINTN                OpCodeBufSize,
  IN OUT    EFI_HII_UPDATE_DATA  *UefiUpdateData
  )
{
  UINT8 * NewBuff;
  
  if (UefiUpdateData->Offset + OpCodeBufSize > UefiUpdateData->BufferSize) {
    NewBuff = AllocateCopyPool (UefiUpdateData->BufferSize + LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL, UefiUpdateData->Data);
    if (NewBuff == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    UefiUpdateData->BufferSize += LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL;
    FreePool (UefiUpdateData->Data);
    UefiUpdateData->Data = NewBuff;
  }
  
  CopyMem (UefiUpdateData->Data + UefiUpdateData->Offset, OpCodeBuf, OpCodeBufSize);
  UefiUpdateData->Offset += (UINT32) OpCodeBufSize;

  return EFI_SUCCESS;
}

/**
  Assign a Question ID.

  If FwQuestionId is 0, then assign a new question ID. The new question ID
  is MaxQuestionId incremented by 1. The MaxQuestionId of FormSet is also
  incremented by 1.

  If FwQuestionId is not 0, then it is used as the Framework Question ID.

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
  Create UEFI HII "End Of" Opcode and append it to UefiUpdateData buffer.

  @param UefiUpdateData     The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
UCreateEndOfOpcode (
  IN OUT      EFI_HII_UPDATE_DATA         *UefiUpdateData
  )
{
  EFI_IFR_END UOpcode;

  ZeroMem (&UOpcode, sizeof (UOpcode));

  UOpcode.Header.OpCode = EFI_IFR_END_OP;
  UOpcode.Header.Length = sizeof (UOpcode);

  return AppendToUpdateBuffer ((UINT8 *)&UOpcode, sizeof(UOpcode), UefiUpdateData);
}

/**
  Create UEFI HII Subtitle Opcode from a Framework HII Subtitle Opcode.

  @param FwOpcode         The input Framework Opcode.
  @param UefiUpdateData   The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateSubtitleOpCode (
  IN CONST FRAMEWORK_EFI_IFR_SUBTITLE  *FwOpcode,
  IN OUT   EFI_HII_UPDATE_DATA         *UefiUpdateData
  )
{
  EFI_IFR_SUBTITLE UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.OpCode = EFI_IFR_SUBTITLE_OP;
  UOpcode.Header.Length = sizeof (EFI_IFR_SUBTITLE);

  UOpcode.Statement.Prompt = FwOpcode->SubTitle;

  return AppendToUpdateBuffer ((UINT8 *)&UOpcode, sizeof(UOpcode), UefiUpdateData);
}

/**
  Create UEFI HII Text Opcode from a Framework HII Text Opcode.

  @param FwOpcode        The input Framework Opcode.
  @param UefiUpdateData  The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateTextOpCode (
  IN CONST FRAMEWORK_EFI_IFR_TEXT      *FwOpcode,
  IN OUT   EFI_HII_UPDATE_DATA         *UefiUpdateData
  )
{
  EFI_IFR_TEXT      UTextOpCode;
  EFI_IFR_ACTION    UActionOpCode;

  if ((FwOpcode->Flags & FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE) == 0) {
    ZeroMem (&UTextOpCode, sizeof(UTextOpCode));
    
    UTextOpCode.Header.OpCode = EFI_IFR_TEXT_OP;
    UTextOpCode.Header.Length = sizeof (EFI_IFR_TEXT);

    UTextOpCode.Statement.Help   = FwOpcode->Help;

    UTextOpCode.Statement.Prompt = FwOpcode->Text;
    UTextOpCode.TextTwo          = FwOpcode->TextTwo;
    
    return AppendToUpdateBuffer ((UINT8 *) &UTextOpCode, sizeof(UTextOpCode), UefiUpdateData);
  } else {
    //
    // Iteractive Text Opcode is EFI_IFR_ACTION
    //

    ZeroMem (&UActionOpCode, sizeof (UActionOpCode));

    UActionOpCode.Header.OpCode = EFI_IFR_ACTION_OP;
    UActionOpCode.Header.Length = sizeof (EFI_IFR_ACTION);

    UActionOpCode.Question.Header.Prompt = FwOpcode->Text;
    UActionOpCode.Question.Header.Help  = FwOpcode->Help;
    UActionOpCode.Question.Flags      = EFI_IFR_FLAG_CALLBACK;
    UActionOpCode.Question.QuestionId = FwOpcode->Key;

    return AppendToUpdateBuffer ((UINT8 *) &UActionOpCode, sizeof(UActionOpCode), UefiUpdateData);
    
  }
}

/**
  Create UEFI HII Reference Opcode from a Framework HII Reference Opcode.

  @param FwOpcode           The input Framework Opcode.
  @param UefiUpdateData     The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateReferenceOpCode (
  IN CONST FRAMEWORK_EFI_IFR_REF       *FwOpcode,
  IN OUT    EFI_HII_UPDATE_DATA        *UefiUpdateData
  )
{
  EFI_IFR_REF UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_REF_OP;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;
  UOpcode.Question.QuestionId = FwOpcode->Key;

  UOpcode.FormId = FwOpcode->FormId;

  //
  // We only map FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE and FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED to 
  // UEFI IFR Opcode flags. The rest flags are obsolete.
  //
  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));
  

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
}


/**
  Create UEFI HII "One Of Option" Opcode from a Framework HII "One Of Option" Opcode.

  @param FwOpcode        The input Framework Opcode.
  @param Width           The size of the One Of Option. 1 bytes or 2 bytes.
  @param UefiUpdateData  The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateOneOfOptionOpCode (
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF_OPTION    *FwOpcode,
  IN       UINTN                              Width,
  IN OUT   EFI_HII_UPDATE_DATA                *UefiUpdateData
  )
{
  EFI_IFR_ONE_OF_OPTION UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;

  UOpcode.Option        = FwOpcode->Option;
  CopyMem (&UOpcode.Value.u8, &FwOpcode->Value, Width);

  //
  // #define FRAMEWORK_EFI_IFR_FLAG_DEFAULT           0x01
  // #define FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING     0x02
  // #define EFI_IFR_OPTION_DEFAULT                   0x10
  // #define EFI_IFR_OPTION_DEFAULT_MFG               0x20
  //
  UOpcode.Flags = (UINT8) (UOpcode.Flags  | (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_DEFAULT | FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING)) << 4);

  switch (Width) {
    case 1:
      UOpcode.Type = EFI_IFR_TYPE_NUM_SIZE_8;
      break;
      
    case 2:
      UOpcode.Type = EFI_IFR_TYPE_NUM_SIZE_16;
      break;
      
    default:
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
  }

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
}

/**
  Create a GUID Opcode EFI_IFR_GUID_OPTIONKEY to map the Framework One Of Option callback key
  to a UEFI Question ID. This information is used to invoke the Framework HII Browser Callback
  function. The opcode is appened to UefiUpdateData.

  @param    QuestionId      The UEFI Question ID.
  @param    OptionValue     The value of the "One Of Option".
  @param    KeyValue        The Framework "One Of Option" callback key.
  @param    UefiDat         The UEFI Update Data buffer.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
**/

EFI_STATUS
CreateGuidOptionKeyOpCode (
  IN EFI_QUESTION_ID                   QuestionId,
  IN UINT16                            OptionValue,
  IN EFI_QUESTION_ID                   KeyValue,
  IN OUT    EFI_HII_UPDATE_DATA        *UefiUpdateData
  )
{
  EFI_IFR_GUID_OPTIONKEY              UOpcode;

  CopyMem (&UOpcode, &mOptionKeyTemplate, sizeof (EFI_IFR_GUID_OPTIONKEY));

  UOpcode.QuestionId  = QuestionId;
  CopyMem (&UOpcode.OptionValue, &OptionValue, sizeof (OptionValue)); 
  UOpcode.KeyValue = KeyValue;
  
  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
}

/**
  Create UEFI HII "One Of" Opcode from a Framework HII "One Of" Opcode.

  @param ThunkContext The HII Thunk Context.
  @param FwOpcode     The input Framework Opcode.
  @param UefiUpdateData     The newly created UEFI HII opcode is appended to UefiUpdateData.
  @param NextFwOpcode Returns the position of the next Framework Opcode after FRAMEWORK_EFI_IFR_END_ONE_OF_OP of
                      the "One Of Option".
  @param OpcodeCount  The number of Opcode for the complete Framework "One Of" Opcode.
                      
  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateOneOfOpCode (
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF    *FwOpcode,
  IN OUT   EFI_HII_UPDATE_DATA         *UefiUpdateData,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER **NextFwOpcode,
  OUT      UINTN                       *OpcodeCount
  )
{
  EFI_STATUS                          Status;
  EFI_IFR_ONE_OF                      UOpcode;
  FRAMEWORK_EFI_IFR_OP_HEADER         *FwOpHeader;
  FRAMEWORK_EFI_IFR_ONE_OF_OPTION     *FwOneOfOp;

  ASSERT (NextFwOpcode != NULL);
  ASSERT (OpcodeCount != NULL);

  ZeroMem (&UOpcode, sizeof(UOpcode));
  *OpcodeCount = 0;

  UOpcode.Header.Length = sizeof(UOpcode);
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
  while (FwOpHeader->OpCode != FRAMEWORK_EFI_IFR_END_ONE_OF_OP) {
    ASSERT (FwOpHeader->OpCode == FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP);
    
    FwOneOfOp = (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader;
    if ((FwOneOfOp->Flags & FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE) != 0) {
      UOpcode.Question.Flags |= EFI_IFR_FLAG_CALLBACK;
      
      if (UOpcode.Question.QuestionId == 0) {
        Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
        if (EFI_ERROR (Status)) {
          UOpcode.Question.QuestionId = AssignQuestionId (FwOneOfOp->Key, ThunkContext->FormSet);
        }
      }

    }

    if (FwOneOfOp->Flags & FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED) {
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
  
  Status = AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof (UOpcode), UefiUpdateData);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  *OpcodeCount += 1;

  //
  // Go over again the Framework IFR binary to build the UEFI One Of Option opcodes.
  //
  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != FRAMEWORK_EFI_IFR_END_ONE_OF_OP) {

    FwOneOfOp = (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader;
      
    Status = F2UCreateOneOfOptionOpCode ((FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader, FwOpcode->Width, UefiUpdateData);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = CreateGuidOptionKeyOpCode (UOpcode.Question.QuestionId, FwOneOfOp->Value, FwOneOfOp->Key, UefiUpdateData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  Status = UCreateEndOfOpcode (UefiUpdateData);
  if (!EFI_ERROR (Status)) {
    *NextFwOpcode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  return Status;
}

/**
  Create UEFI HII "Ordered List" Opcode from a Framework HII "Ordered List" Opcode.

  @param ThunkContext The HII Thunk Context.
  @param FwOpcode     The input Framework Opcode.
  @param UefiUpdateData The newly created UEFI HII opcode is appended to UefiUpdateData.
  @param NextFwOpcode Returns the position of the next Framework Opcode after FRAMEWORK_EFI_IFR_END_ONE_OF_OP of
                      the "Ordered List".
  @param OpcodeCount  The number of Opcode for the complete Framework "Ordered List" Opcode.
                      
  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateOrderedListOpCode (
  IN       HII_THUNK_CONTEXT              *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_ORDERED_LIST *FwOpcode,
  IN OUT    EFI_HII_UPDATE_DATA           *UefiUpdateData,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER    **NextFwOpcode,
  OUT      UINTN                          *OpcodeCount
  )
{
  EFI_IFR_ORDERED_LIST              UOpcode;
  EFI_STATUS                        Status;
  FRAMEWORK_EFI_IFR_OP_HEADER       *FwOpHeader;
  FRAMEWORK_EFI_IFR_ONE_OF_OPTION     *FwOneOfOp;

  ZeroMem (&UOpcode, sizeof(UOpcode));
  *OpcodeCount = 0;

  UOpcode.Header.Length = sizeof(UOpcode);
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
  while (FwOpHeader->OpCode != FRAMEWORK_EFI_IFR_END_ONE_OF_OP) {
    ASSERT (FwOpHeader->OpCode == FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP);
    
    FwOneOfOp = (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader;
    if ((FwOneOfOp->Flags & FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE) != 0) {
      UOpcode.Question.Flags |= EFI_IFR_FLAG_CALLBACK;
      
      if (UOpcode.Question.QuestionId == 0) {
        Status = FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
        if (EFI_ERROR (Status)) {
          UOpcode.Question.QuestionId = AssignQuestionId (FwOneOfOp->Key, ThunkContext->FormSet);
        }

      }
    }

    if (FwOneOfOp->Flags & FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED) {
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
 
  Status = AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  *OpcodeCount += 1;

  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != FRAMEWORK_EFI_IFR_END_ONE_OF_OP) {
    //
    // Each entry of Order List in Framework HII is always 1 byte in size
    //
    Status = F2UCreateOneOfOptionOpCode ((CONST FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader, 1, UefiUpdateData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  Status = UCreateEndOfOpcode (UefiUpdateData);
  if (!EFI_ERROR (Status)) {
    *NextFwOpcode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *OpcodeCount += 1;
  }

  return Status;
}

/**
  Create UEFI HII CheckBox Opcode from a Framework HII Checkbox Opcode.

  @param ThunkContext    The HII Thunk Context.
  @param FwOpcode        The input Framework Opcode.
  @param UefiUpdateData  The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateCheckBoxOpCode (
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_CHECKBOX  *FwOpcode,
  IN OUT   EFI_HII_UPDATE_DATA         *UefiUpdateData
  )
{
  EFI_STATUS       Status;
  EFI_IFR_CHECKBOX UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
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
  //      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE, 
  //      FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED,
  // to UEFI IFR Opcode Question flags. The rest flags are obsolete.
  //
  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));


  UOpcode.Question.VarStoreId    = ThunkContext->FormSet->DefaultVarStoreId;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  //
  // We also map these 2 flags:
  //      FRAMEWORK_EFI_IFR_FLAG_DEFAULT, 
  //      FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING,
  // to UEFI IFR CheckBox Opcode default flags.
  //
  UOpcode.Flags           = (UINT8) (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_DEFAULT | FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING));

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
}


/**
  Create UEFI HII Numeric Opcode from a Framework HII Numeric Opcode.

  @param ThunkContext The HII Thunk Context.
  @param FwOpcode     The input Framework Opcode.
  @param UefiUpdateData     The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateNumericOpCode (
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_NUMERIC   *FwOpcode,
  IN OUT    EFI_HII_UPDATE_DATA        *UefiUpdateData
  )
{
  EFI_STATUS      Status;
  EFI_IFR_NUMERIC UOpcode;
  EFI_IFR_DEFAULT UOpcodeDefault;

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

  UOpcode.Header.Length = sizeof(UOpcode);
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

  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));

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
      return EFI_INVALID_PARAMETER;
    }
  }
  
  Status = AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // We need to create a default value.
  //
  ZeroMem (&UOpcodeDefault, sizeof (UOpcodeDefault));
  UOpcodeDefault.Header.Length = sizeof (UOpcodeDefault);
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

  Status = AppendToUpdateBuffer ((UINT8 *) &UOpcodeDefault, sizeof(UOpcodeDefault), UefiUpdateData);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = UCreateEndOfOpcode (UefiUpdateData);

  return Status;
}


/**
  Create UEFI HII String Opcode from a Framework HII String Opcode.

  @param ThunkContext The HII Thunk Context.
  @param FwOpcode     The input Framework Opcode.
  @param UefiUpdateData     The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateStringOpCode (
  IN       HII_THUNK_CONTEXT           *ThunkContext,
  IN CONST FRAMEWORK_EFI_IFR_STRING    *FwOpcode,
  IN OUT    EFI_HII_UPDATE_DATA        *UefiUpdateData
  )
{
  EFI_IFR_STRING UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  if (FwOpcode->Key == 0) {
    FwQIdToUefiQId (ThunkContext->FormSet, FwOpcode->Header.OpCode, FwOpcode->QuestionId, &UOpcode.Question.QuestionId);
  } else {
    UOpcode.Question.QuestionId    = FwOpcode->Key;
  }

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_STRING_OP;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  UOpcode.Question.QuestionId    = FwOpcode->Key;
  UOpcode.Question.Flags  = (UINT8) (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));

  UOpcode.Question.VarStoreId    = ThunkContext->FormSet->DefaultVarStoreId;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  UOpcode.MinSize = FwOpcode->MinSize;
  UOpcode.MaxSize = FwOpcode->MaxSize;
  UOpcode.Flags   = EFI_IFR_STRING_MULTI_LINE;

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
}

/**
  Create UEFI HII Banner Opcode from a Framework HII Banner Opcode.

  @param FwOpcode     The input Framework Opcode.
  @param UefiUpdateData     The newly created UEFI HII opcode is appended to UefiUpdateData.

  @retval EFI_SUCCESS           The UEFI HII opcode is created successfully and appended to UefiUpdateData.
  @retval EFI_OUT_OF_RESOURCE   There is not enough resource.
  
**/
EFI_STATUS
F2UCreateBannerOpCode (
  IN CONST FRAMEWORK_EFI_IFR_BANNER    *FwOpcode,
  IN OUT    EFI_HII_UPDATE_DATA        *UefiUpdateData
  )
{
  EFI_IFR_GUID_BANNER UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_GUID_OP;

  CopyMem (&UOpcode.Guid, &mTianoExtendedOpcodeGuid, sizeof (EFI_GUID));
  UOpcode.ExtendOpCode = EFI_IFR_EXTEND_OP_BANNER;
  UOpcode.Title          = FwOpcode->Title;
  UOpcode.LineNumber     = FwOpcode->LineNumber;
  UOpcode.Alignment      = FwOpcode->Alignment;

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiUpdateData);
}

/**
  Create a EFI_HII_UPDATE_DATA structure used to call IfrLibUpdateForm.

  @param ThunkContext   The HII Thunk Context.
  @param FwUpdateData   The Framework Update Data.
  @param UefiUpdateData The UEFI Update Data.

  @retval EFI_SUCCESS       The UEFI Update Data is created successfully.
  @retval EFI_UNSUPPORTED   There is unsupported opcode in FwUpdateData.
  @retval EFI_OUT_OF_RESOURCES There is not enough resource.
**/
EFI_STATUS
FwUpdateDataToUefiUpdateData (
  IN       HII_THUNK_CONTEXT                *ThunkContext,
  IN CONST FRAMEWORK_EFI_HII_UPDATE_DATA    *FwUpdateData,
  OUT      EFI_HII_UPDATE_DATA              **UefiUpdateData
  )
{
  FRAMEWORK_EFI_IFR_OP_HEADER          *FwOpCode;
  FRAMEWORK_EFI_IFR_OP_HEADER          *NextFwOpCode;
  EFI_HII_UPDATE_DATA                  *UefiOpCode;
  UINTN                                Index;
  EFI_STATUS                           Status;
  UINTN                                DataCount;

  UefiOpCode = AllocateZeroPool (sizeof (EFI_HII_UPDATE_DATA));
  if (UefiOpCode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  UefiOpCode->Data = AllocateZeroPool (LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL);
  if (UefiOpCode->Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UefiOpCode->BufferSize = LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL;
  UefiOpCode->Offset = 0;

  FwOpCode = (FRAMEWORK_EFI_IFR_OP_HEADER *) &FwUpdateData->Data;

  for (Index = 0; Index < FwUpdateData->DataCount; Index += DataCount) {
    switch (FwOpCode->OpCode) {
      case FRAMEWORK_EFI_IFR_SUBTITLE_OP:
        Status = F2UCreateSubtitleOpCode ((FRAMEWORK_EFI_IFR_SUBTITLE  *) FwOpCode, UefiOpCode);
        DataCount = 1;
        break;
        
      case FRAMEWORK_EFI_IFR_TEXT_OP:
        Status = F2UCreateTextOpCode ((FRAMEWORK_EFI_IFR_TEXT  *) FwOpCode, UefiOpCode);  
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_REF_OP:
        Status = F2UCreateReferenceOpCode ((FRAMEWORK_EFI_IFR_REF *) FwOpCode, UefiOpCode);  
        DataCount = 1;
        break;
        
      case FRAMEWORK_EFI_IFR_ONE_OF_OP:
        Status = F2UCreateOneOfOpCode (ThunkContext, (FRAMEWORK_EFI_IFR_ONE_OF *) FwOpCode, UefiOpCode, &NextFwOpCode, &DataCount);
        if (!EFI_ERROR (Status)) {
          FwOpCode = NextFwOpCode;
          //
          // FwOpCode is already updated to point to the next opcode.
          //
          continue;
        }
        break;

      case FRAMEWORK_EFI_IFR_ORDERED_LIST_OP:
        Status = F2UCreateOrderedListOpCode (ThunkContext, (FRAMEWORK_EFI_IFR_ORDERED_LIST *) FwOpCode, UefiOpCode, &NextFwOpCode, &DataCount);
        if (!EFI_ERROR (Status)) {
          FwOpCode = NextFwOpCode;
          //
          // FwOpCode is already updated to point to the next opcode.
          //
          continue;
        }
        break;
        
      case FRAMEWORK_EFI_IFR_CHECKBOX_OP:
        Status = F2UCreateCheckBoxOpCode (ThunkContext, (FRAMEWORK_EFI_IFR_CHECKBOX *) FwOpCode, UefiOpCode);  
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_STRING_OP:
        Status = F2UCreateStringOpCode (ThunkContext, (FRAMEWORK_EFI_IFR_STRING *) FwOpCode, UefiOpCode);  
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_BANNER_OP:
        Status = F2UCreateBannerOpCode ((FRAMEWORK_EFI_IFR_BANNER *) FwOpCode, UefiOpCode);  
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_END_ONE_OF_OP:
        Status = UCreateEndOfOpcode (UefiOpCode);
        DataCount = 1;
        break;

      case FRAMEWORK_EFI_IFR_NUMERIC_OP:
        Status = F2UCreateNumericOpCode (ThunkContext, (FRAMEWORK_EFI_IFR_NUMERIC *) FwOpCode, UefiOpCode);
        DataCount = 1;
        break;

      default:
        ASSERT (FALSE);
        return EFI_UNSUPPORTED;
    }

    if (EFI_ERROR (Status)) {
      FreePool (UefiOpCode->Data);
      FreePool (UefiOpCode);
      return Status;
    }

    FwOpCode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpCode + FwOpCode->Length);
  }

  *UefiUpdateData = UefiOpCode;
  
  return EFI_SUCCESS;
}

