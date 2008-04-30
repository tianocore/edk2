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
#include "OpcodeCreation.h"
#include "UefiIfrDefault.h"

EFI_GUID  mTianoExtendedOpcodeGuid = EFI_IFR_TIANO_GUID;

#define LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL   0x1000
EFI_STATUS
AppendToUpdateBuffer (
  IN CONST  UINT8                *OpCodeBuf,
  IN        UINTN                BufSize,
  OUT       EFI_HII_UPDATE_DATA  *UefiData
  )
{
  UINT8 * NewBuff;
  
  if (UefiData->Offset + BufSize > UefiData->BufferSize) {
    NewBuff = AllocateCopyPool (UefiData->BufferSize + LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL, UefiData->Data);
    if (NewBuff == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    UefiData->BufferSize += LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL;
    FreePool (UefiData->Data);
    UefiData->Data = NewBuff;
  }
  
  CopyMem (UefiData->Data + UefiData->Offset, OpCodeBuf, BufSize);
  UefiData->Offset += (UINT32) BufSize;

  return EFI_SUCCESS;
}

EFI_STATUS
UCreateEndOfOpcode (
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  )
{
  EFI_IFR_END UOpcode;

  ZeroMem (&UOpcode, sizeof (UOpcode));

  UOpcode.Header.OpCode = EFI_IFR_END_OP;
  UOpcode.Header.Length = sizeof (UOpcode);

  return AppendToUpdateBuffer ((UINT8 *)&UOpcode, sizeof(UOpcode), UefiData);
}

EFI_STATUS
F2UCreateSubtitleOpCode (
  IN CONST FRAMEWORK_EFI_IFR_SUBTITLE  *FwSubTitle,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  )
{
  EFI_IFR_SUBTITLE UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.OpCode = EFI_IFR_SUBTITLE_OP;
  UOpcode.Header.Length = sizeof (EFI_IFR_SUBTITLE);

  UOpcode.Statement.Prompt = FwSubTitle->SubTitle;

  return AppendToUpdateBuffer ((UINT8 *)&UOpcode, sizeof(UOpcode), UefiData);
}

EFI_STATUS
F2UCreateTextOpCode (
  IN CONST FRAMEWORK_EFI_IFR_TEXT      *FwText,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  )
{
  EFI_IFR_TEXT UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));
  
  UOpcode.Header.OpCode = EFI_IFR_TEXT_OP;
  UOpcode.Header.Length = sizeof (EFI_IFR_TEXT);

  UOpcode.Statement.Help   = FwText->Help;

  UOpcode.Statement.Prompt = FwText->Text;
  UOpcode.TextTwo          = FwText->TextTwo;
  
  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
}

/*
typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            FormId;
  STRING_REF        Prompt;
  STRING_REF        Help;   // The string Token for the context-help
  UINT8             Flags;  // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;    // Value to be passed to caller to identify this particular op-code
} FRAMEWORK_EFI_IFR_REF;

*/
EFI_STATUS
F2UCreateGotoOpCode (
  IN CONST FRAMEWORK_EFI_IFR_REF       *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
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
  UOpcode.Question.Flags  = (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));
  

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
}


/*
typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        Option;     // The string token describing the option
  UINT16            Value;      // The value associated with this option that is stored in the NVRAM if chosen
  UINT8             Flags;      // For now, if non-zero, means that it is the default option, - further definition likely above
  UINT16            Key;        // Value to be passed to caller to identify this particular op-code
} FRAMEWORK_EFI_IFR_ONE_OF_OPTION;

typedef union {
  UINT8           u8;
  UINT16          u16;
  UINT32          u32;
  UINT64          u64;
  BOOLEAN         b;
  EFI_HII_TIME    time;
  EFI_HII_DATE    date;
  EFI_STRING_ID   string;
} EFI_IFR_TYPE_VALUE;

typedef struct _EFI_IFR_ONE_OF_OPTION {
  EFI_IFR_OP_HEADER        Header;
  EFI_STRING_ID            Option;
  UINT8                    Flags;
  UINT8                    Type;
  EFI_IFR_TYPE_VALUE       Value;
} EFI_IFR_ONE_OF_OPTION;

*/
EFI_STATUS
F2UCreateOneOfOptionOpCode (
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF_OPTION    *FwOpcode,
  IN       UINTN                              Width,
  OUT      EFI_HII_UPDATE_DATA                *UefiData
  )
{
  EFI_IFR_ONE_OF_OPTION UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;

  UOpcode.Option        = FwOpcode->Option;
  CopyMem (&UOpcode.Value.u8, &FwOpcode->Value, Width);

  //
  
  // #define FRAMEWORK_EFI_IFR_FLAG_DEFAULT                    0x01
  // #define FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING        0x02
  // #define EFI_IFR_OPTION_DEFAULT 0x10
  // #define EFI_IFR_OPTION_DEFAULT_MFG 0x20
  //
  UOpcode.Flags |= (UINT8) ((FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_DEFAULT | FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING)) << 4);

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

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
}


/*
typedef struct _EFI_IFR_QUESTION_HEADER {
  EFI_IFR_STATEMENT_HEADER Header;
  EFI_QUESTION_ID          QuestionId;
  EFI_VARSTORE_ID          VarStoreId;
  union {
    EFI_STRING_ID          VarName;
    UINT16                 VarOffset;
  }                        VarStoreInfo;
  UINT8                    Flags;
} EFI_IFR_QUESTION_HEADER;

typedef union {
  struct {
    UINT8 MinValue;
    UINT8 MaxValue;
    UINT8 Step;
  } u8;
  struct {
    UINT16 MinValue;
    UINT16 MaxValue;
    UINT16 Step;
  } u16;
  struct {
    UINT32 MinValue;
    UINT32 MaxValue;
    UINT32 Step;
  } u32;
  struct {
    UINT64 MinValue;
    UINT64 MaxValue;
    UINT64 Step;
  } u64;
} MINMAXSTEP_DATA;

typedef struct _EFI_IFR_ONE_OF {
  EFI_IFR_OP_HEADER        Header;
  EFI_IFR_QUESTION_HEADER  Question;
  UINT8                    Flags;
  MINMAXSTEP_DATA          data;
} EFI_IFR_ONE_OF;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;      // The Size of the Data being saved
  STRING_REF        Prompt;     // The String Token for the Prompt
  STRING_REF        Help;       // The string Token for the context-help
} FRAMEWORK_EFI_IFR_ONE_OF;


*/

EFI_STATUS
F2UCreateOneOfOpCode (
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF    *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER **NextFwOpcode,
  OUT      UINTN                       *DataCount
  )
{
  EFI_STATUS     Status;
  EFI_IFR_ONE_OF UOpcode;
  FRAMEWORK_EFI_IFR_OP_HEADER *FwOpHeader;
  FRAMEWORK_EFI_IFR_ONE_OF_OPTION *FwOneOfOp;
  BOOLEAN        HasQuestionId;

  ASSERT (NextFwOpcode != NULL);
  ASSERT (DataCount != NULL);

  ZeroMem (&UOpcode, sizeof(UOpcode));
  *DataCount = 0;

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_ONE_OF_OP;
  UOpcode.Header.Scope  = 1;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;
 
  Status = AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  *DataCount += 1;

  //
  // Go over the Framework IFR binary to get the QuestionId for generated UEFI One Of Option opcode
  //
  HasQuestionId = FALSE;
  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != FRAMEWORK_EFI_IFR_END_ONE_OF_OP) {
    ASSERT (FwOpHeader->OpCode == FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP);
    
    FwOneOfOp = (FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader;
    if (((FwOneOfOp->Key & FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE) == 0) & !HasQuestionId) {
      HasQuestionId = TRUE;
      UOpcode.Question.QuestionId = FwOneOfOp->Key;
    }

    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
  }

  //
  // Go over again the Framework IFR binary to build the UEFI One Of Option opcodes.
  //
  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != FRAMEWORK_EFI_IFR_END_ONE_OF_OP) {
    Status = F2UCreateOneOfOptionOpCode ((FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader, FwOpcode->Width, UefiData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *DataCount += 1;
  }

  Status = UCreateEndOfOpcode (UefiData);
  if (!EFI_ERROR (Status)) {
    *NextFwOpcode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *DataCount += 1;
  }

  return Status;
}

/*
typedef struct _EFI_IFR_QUESTION_HEADER {
  EFI_IFR_STATEMENT_HEADER Header;
  EFI_QUESTION_ID          QuestionId;
  EFI_VARSTORE_ID          VarStoreId;
  union {
    EFI_STRING_ID          VarName;
    UINT16                 VarOffset;
  }                        VarStoreInfo;
  UINT8                    Flags;
} EFI_IFR_QUESTION_HEADER;

typedef struct _EFI_IFR_ORDERED_LIST {
  EFI_IFR_OP_HEADER        Header;
  EFI_IFR_QUESTION_HEADER  Question;
  UINT8                    MaxContainers;
  UINT8                    Flags;
} EFI_IFR_ORDERED_LIST;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The offset in NV for storage of the data
  UINT8             MaxEntries; // The maximum number of options in the ordered list (=size of NVStore)
  STRING_REF        Prompt;     // The string token for the prompt
  STRING_REF        Help;       // The string token for the context-help
} FRAMEWORK_EFI_IFR_ORDERED_LIST;

*/
EFI_STATUS
F2UCreateOrderedListOpCode (
  IN CONST FRAMEWORK_EFI_IFR_ORDERED_LIST *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER **NextFwOpcode,
  OUT      UINTN                       *DataCount
  )
{
  EFI_IFR_ORDERED_LIST              UOpcode;
  EFI_STATUS                        Status;
  FRAMEWORK_EFI_IFR_OP_HEADER       *FwOpHeader;

  ZeroMem (&UOpcode, sizeof(UOpcode));
  *DataCount = 0;

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_ORDERED_LIST_OP;
  UOpcode.Header.Scope  = 1;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  UOpcode.MaxContainers = FwOpcode->MaxEntries;
 
  Status = AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  *DataCount += 1;

  FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpcode + FwOpcode->Header.Length);
  while (FwOpHeader->OpCode != FRAMEWORK_EFI_IFR_END_ONE_OF_OP) {
    Status = F2UCreateOneOfOptionOpCode ((CONST FRAMEWORK_EFI_IFR_ONE_OF_OPTION *) FwOpHeader, FwOpcode->MaxEntries, UefiData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    FwOpHeader = (FRAMEWORK_EFI_IFR_OP_HEADER *) ((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *DataCount += 1;
  }

  Status = UCreateEndOfOpcode (UefiData);
  if (!EFI_ERROR (Status)) {
    *NextFwOpcode = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FwOpHeader + FwOpHeader->Length);
    *DataCount += 1;
  }

  return Status;
}

/*
typedef struct _EFI_IFR_QUESTION_HEADER {
  EFI_IFR_STATEMENT_HEADER Header;
  EFI_QUESTION_ID          QuestionId;
  EFI_VARSTORE_ID          VarStoreId;
  union {
    EFI_STRING_ID          VarName;
    UINT16                 VarOffset;
  }                        VarStoreInfo;
  UINT8                    Flags;
} EFI_IFR_QUESTION_HEADER;
*/

/*
typedef struct _EFI_IFR_CHECKBOX {
  EFI_IFR_OP_HEADER        Header;
  EFI_IFR_QUESTION_HEADER  Question;
  UINT8                    Flags;
} EFI_IFR_CHECKBOX;
*/

/*
typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;      // The Size of the Data being saved
  STRING_REF        Prompt;     // The String Token for the Prompt
  STRING_REF        Help;       // The string Token for the context-help
  UINT8             Flags;      // For now, if non-zero, means that it is the default option, - further definition likely
  UINT16            Key;        // Value to be passed to caller to identify this particular op-code
} FRAMEWORK_EFI_IFR_CHECKBOX, FRAMEWORK_EFI_IFR_CHECK_BOX;
*/


EFI_STATUS
F2UCreateCheckBoxOpCode (
  IN CONST FRAMEWORK_EFI_IFR_CHECKBOX  *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  )
{
  EFI_IFR_CHECKBOX UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_CHECKBOX_OP;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  UOpcode.Question.QuestionId    = FwOpcode->Key;
  UOpcode.Question.VarStoreId    = RESERVED_VARSTORE_ID;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  //
  // We only  map 2 flags:
  //      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE, 
  //      FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED,
  // to UEFI IFR Opcode Question flags. The rest flags are obsolete.
  //
  UOpcode.Question.Flags  = (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));

  //
  // We also map 2 flags:
  //      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE, 
  //      FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED,
  // to UEFI IFR CheckBox Opcode default flags.
  //
  UOpcode.Flags           = (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_DEFAULT | FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING));

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
}


/*
typedef struct _EFI_IFR_QUESTION_HEADER {
  EFI_IFR_STATEMENT_HEADER Header;
  EFI_QUESTION_ID          QuestionId;
  EFI_VARSTORE_ID          VarStoreId;
  union {
    EFI_STRING_ID          VarName;
    UINT16                 VarOffset;
  }                        VarStoreInfo;
  UINT8                    Flags;
} EFI_IFR_QUESTION_HEADER;

typedef union {
  struct {
    UINT8 MinValue;
    UINT8 MaxValue;
    UINT8 Step;
  } u8;
  struct {
    UINT16 MinValue;
    UINT16 MaxValue;
    UINT16 Step;
  } u16;
  struct {
    UINT32 MinValue;
    UINT32 MaxValue;
    UINT32 Step;
  } u32;
  struct {
    UINT64 MinValue;
    UINT64 MaxValue;
    UINT64 Step;
  } u64;
} MINMAXSTEP_DATA;

typedef struct _EFI_IFR_NUMERIC {
  EFI_IFR_OP_HEADER        Header;
  EFI_IFR_QUESTION_HEADER  Question;
  UINT8                    Flags;
  MINMAXSTEP_DATA          data;
} EFI_IFR_NUMERIC;


typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;      // The Size of the Data being saved
  STRING_REF        Prompt;     // The String Token for the Prompt
  STRING_REF        Help;       // The string Token for the context-help
  UINT8             Flags;      // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;        // Value to be passed to caller to identify this particular op-code
  UINT16            Minimum;
  UINT16            Maximum;
  UINT16            Step;       // If step is 0, then manual input is specified, otherwise, left/right arrow selection is called for
  UINT16            Default;
} FRAMEWORK_EFI_IFR_NUMERIC;

*/


EFI_STATUS
F2UCreateNumericOpCode (
  IN CONST FRAMEWORK_EFI_IFR_NUMERIC   *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  )
{
  EFI_STATUS      Status;
  EFI_IFR_NUMERIC UOpcode;
  EFI_IFR_DEFAULT UOpcodeDefault;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_NUMERIC_OP;
  //
  // We need to create a nested default value for the UEFI Numeric Opcode.
  // So turn on the scope.
  //
  if (FwOpcode->Default != 0) {
    UOpcode.Header.Scope = 1;
  }

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  UOpcode.Question.QuestionId    = FwOpcode->Key;
  UOpcode.Question.VarStoreId    = RESERVED_VARSTORE_ID;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  UOpcode.Question.Flags  = (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));

  //
  // Framework Numeric values are all in UINT16 and displayed as decimal.
  //
  UOpcode.data.u16.MinValue = FwOpcode->Minimum;
  UOpcode.data.u16.MaxValue = FwOpcode->Maximum;
  UOpcode.data.u16.Step = FwOpcode->Step;

  switch (FwOpcode->Width) {
    case 1: 
    {
      UOpcode.Flags           =  EFI_IFR_NUMERIC_SIZE_2 | EFI_IFR_DISPLAY_UINT_DEC; 
      break;
    } 
    case 2: 
    {
      UOpcode.Flags           =  EFI_IFR_NUMERIC_SIZE_1 | EFI_IFR_DISPLAY_UINT_DEC; 
      break;
    }
    default: 
    {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }
  }
  
  Status = AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // We need to create a default value.
  //
  if (FwOpcode->Default != 0) {
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

    Status = AppendToUpdateBuffer ((UINT8 *) &UOpcodeDefault, sizeof(UOpcodeDefault), UefiData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = UCreateEndOfOpcode (UefiData);
  }

  return Status;
}


/*

typedef struct _EFI_IFR_QUESTION_HEADER {
  EFI_IFR_STATEMENT_HEADER Header;
  EFI_QUESTION_ID          QuestionId;
  EFI_VARSTORE_ID          VarStoreId;
  union {
    EFI_STRING_ID          VarName;
    UINT16                 VarOffset;
  }                        VarStoreInfo;
  UINT8                    Flags;
} EFI_IFR_QUESTION_HEADER;

typedef struct _EFI_IFR_STRING {
  EFI_IFR_OP_HEADER        Header;
  EFI_IFR_QUESTION_HEADER  Question;
  UINT8                    MinSize;
  UINT8                    MaxSize;
  UINT8                    Flags;
} EFI_IFR_STRING;


typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId;   // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;        // The Size of the Data being saved -- BUGBUG -- remove someday
  STRING_REF        Prompt;       // The String Token for the Prompt
  STRING_REF        Help;         // The string Token for the context-help
  UINT8             Flags;        // This is included solely for purposes of interactive/dynamic support.
  UINT16            Key;          // Value to be passed to caller to identify this particular op-code
  UINT8             MinSize;      // Minimum allowable sized password
  UINT8             MaxSize;      // Maximum allowable sized password
} FRAMEWORK_EFI_IFR_STRING;


*/

EFI_STATUS
F2UCreateStringOpCode (
  IN CONST FRAMEWORK_EFI_IFR_STRING    *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  )
{
  EFI_IFR_STRING UOpcode;

  ZeroMem (&UOpcode, sizeof(UOpcode));

  UOpcode.Header.Length = sizeof(UOpcode);
  UOpcode.Header.OpCode = EFI_IFR_STRING_OP;

  UOpcode.Question.Header.Prompt = FwOpcode->Prompt;
  UOpcode.Question.Header.Help = FwOpcode->Help;

  UOpcode.Question.QuestionId    = FwOpcode->Key;
  UOpcode.Question.VarStoreId    = RESERVED_VARSTORE_ID;
  UOpcode.Question.VarStoreInfo.VarOffset = FwOpcode->QuestionId;

  UOpcode.Question.Flags  = (FwOpcode->Flags & (FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_RESET_REQUIRED));

  UOpcode.MinSize = FwOpcode->MinSize;
  UOpcode.MaxSize = FwOpcode->MaxSize;
  UOpcode.Flags   = EFI_IFR_STRING_MULTI_LINE;

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
}

/*
typedef struct _EFI_IFR_GUID_BANNER {
  EFI_IFR_OP_HEADER   Header;
  EFI_GUID            Guid;
  UINT8               ExtendOpCode; // Extended opcode is EFI_IFR_EXTEND_OP_BANNER
  EFI_STRING_ID       Title;        // The string token for the banner title
  UINT16              LineNumber;   // 1-based line number
  UINT8               Alignment;    // left, center, or right-aligned
} EFI_IFR_GUID_BANNER;

typedef struct {
  FRAMEWORK_EFI_IFR_OP_HEADER Header;
  STRING_REF        Title;        // The string token for the banner title
  UINT16            LineNumber;   // 1-based line number
  UINT8             Alignment;    // left, center, or right-aligned
} FRAMEWORK_EFI_IFR_BANNER;

*/

EFI_STATUS
F2UCreateBannerOpCode (
  IN CONST FRAMEWORK_EFI_IFR_BANNER    *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
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

  return AppendToUpdateBuffer ((UINT8 *) &UOpcode, sizeof(UOpcode), UefiData);
}



EFI_STATUS
ThunkFrameworkUpdateDataToUefiUpdateData (
  IN CONST FRAMEWORK_EFI_HII_UPDATE_DATA    *Data,
  IN       BOOLEAN                          AddData,
  OUT      EFI_HII_UPDATE_DATA              **UefiData
  )
{
  FRAMEWORK_EFI_IFR_OP_HEADER          *FrameworkOpcodeBuffer;
  FRAMEWORK_EFI_IFR_OP_HEADER          *NextFrameworkOpcodeBuffer;
  EFI_HII_UPDATE_DATA                  *UefiUpdateDataBuffer;
  UINTN                                Index;
  EFI_STATUS                           Status;
  UINTN                                DataCount;
  static UINTN                         mOneOfOptionWidth;

  mOneOfOptionWidth = 0;
  

  UefiUpdateDataBuffer = AllocateZeroPool (sizeof (EFI_HII_UPDATE_DATA));
  if (UefiUpdateDataBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  UefiUpdateDataBuffer->Data = AllocateZeroPool (LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL);
  if (UefiUpdateDataBuffer->Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UefiUpdateDataBuffer->BufferSize = LOCAL_UPDATE_DATA_BUFFER_INCREMENTAL;
  UefiUpdateDataBuffer->Offset = 0;

  FrameworkOpcodeBuffer = (FRAMEWORK_EFI_IFR_OP_HEADER *) &Data->Data;

  for (Index = 0; Index < Data->DataCount; Index += DataCount) {
    //
    // By default Datacount is 1. For FRAMEWORK_EFI_IFR_ONE_OF_OP and FRAMEWORK_EFI_IFR_ORDERED_LIST_OP,
    // DataCount maybe more than 1.
    //
    DataCount = 1;
    switch (FrameworkOpcodeBuffer->OpCode) {
      case FRAMEWORK_EFI_IFR_SUBTITLE_OP:
        Status = F2UCreateSubtitleOpCode ((FRAMEWORK_EFI_IFR_SUBTITLE  *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer);
        break;
        
      case FRAMEWORK_EFI_IFR_TEXT_OP:
        Status = F2UCreateTextOpCode ((FRAMEWORK_EFI_IFR_TEXT  *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer);  
        break;

      case FRAMEWORK_EFI_IFR_REF_OP:
        Status = F2UCreateGotoOpCode ((FRAMEWORK_EFI_IFR_REF *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer);  
        break;
        
      case FRAMEWORK_EFI_IFR_ONE_OF_OP:
        Status = F2UCreateOneOfOpCode ((FRAMEWORK_EFI_IFR_ONE_OF *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer, &NextFrameworkOpcodeBuffer, &DataCount);
        if (!EFI_ERROR (Status)) {
          FrameworkOpcodeBuffer = NextFrameworkOpcodeBuffer;
          //
          // F2UCreateOneOfOpCode has updated FrameworkOpcodeBuffer to point to the next opcode.
          //
          continue;
        }
        break;

      case FRAMEWORK_EFI_IFR_ORDERED_LIST_OP:
        Status = F2UCreateOrderedListOpCode ((FRAMEWORK_EFI_IFR_ORDERED_LIST *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer, &NextFrameworkOpcodeBuffer, &DataCount);
        if (!EFI_ERROR (Status)) {
          FrameworkOpcodeBuffer = NextFrameworkOpcodeBuffer;
          //
          // F2UCreateOrderedListOpCode has updated FrameworkOpcodeBuffer to point to the next opcode.
          //
          continue;
        }
        break;
        
      case FRAMEWORK_EFI_IFR_CHECKBOX_OP:
        Status = F2UCreateCheckBoxOpCode ((FRAMEWORK_EFI_IFR_CHECKBOX *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer);  
        break;

      case FRAMEWORK_EFI_IFR_STRING_OP:
        Status = F2UCreateStringOpCode ((FRAMEWORK_EFI_IFR_STRING *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer);  
        break;

      case FRAMEWORK_EFI_IFR_BANNER_OP:
        Status = F2UCreateBannerOpCode ((FRAMEWORK_EFI_IFR_BANNER *) FrameworkOpcodeBuffer, UefiUpdateDataBuffer);  
        break;

      case FRAMEWORK_EFI_IFR_END_ONE_OF_OP:
        Status = UCreateEndOfOpcode (UefiUpdateDataBuffer);
        mOneOfOptionWidth = 0;
        break;
        
      default:
        ASSERT (FALSE);
        return EFI_UNSUPPORTED;
    }

    if (EFI_ERROR (Status)) {
      FreePool (UefiUpdateDataBuffer->Data);
      FreePool (UefiUpdateDataBuffer);
      return Status;
    }

    FrameworkOpcodeBuffer = (FRAMEWORK_EFI_IFR_OP_HEADER *)((UINT8 *) FrameworkOpcodeBuffer + FrameworkOpcodeBuffer->Length);
  }

  *UefiData = UefiUpdateDataBuffer;
  
  return EFI_SUCCESS;
}

