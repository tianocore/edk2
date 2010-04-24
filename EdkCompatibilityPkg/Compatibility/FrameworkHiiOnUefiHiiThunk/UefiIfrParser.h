/** @file
  Function and Macro defintions for IFR parsing. To get the default value from IFR package, the IFR
  opcode needs to be parsed. Most of code is taken from MdeModulePkg\Universal\SetupBrowserDxe\IfrParse.c.
  This parser is simplified from the origianl IfrParser.c in the following way:

  1) All data structure definition that have nothing to do with IFR Default value scanning (
     required to implement Framework HII's GetDefaultImage ()) is removed.
  2) Ignore the IFR opcode which is invalid for Form Package
     generated using Framework VFR file.

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UEFI_IFR_PARSER_
#define _HII_THUNK_UEFI_IFR_PARSER_


//
// IFR relative definition
//
#define EFI_HII_EXPRESSION_INCONSISTENT_IF   0
#define EFI_HII_EXPRESSION_NO_SUBMIT_IF      1
#define EFI_HII_EXPRESSION_GRAY_OUT_IF       2
#define EFI_HII_EXPRESSION_SUPPRESS_IF       3
#define EFI_HII_EXPRESSION_DISABLE_IF        4
#define EFI_HII_EXPRESSION_VALUE             5
#define EFI_HII_EXPRESSION_RULE              6

#define EFI_HII_VARSTORE_BUFFER              0
#define EFI_HII_VARSTORE_NAME_VALUE          1
#define EFI_HII_VARSTORE_EFI_VARIABLE        2

#define FORM_INCONSISTENT_VALIDATION         0
#define FORM_NO_SUBMIT_VALIDATION            1

extern EFI_GUID  gTianoHiiIfrGuid;

#define ONE_OF_OPTION_MAP_ENTRY_FROM_LINK(Record) CR(Record, ONE_OF_OPTION_MAP_ENTRY, Link, ONE_OF_OPTION_MAP_ENTRY_SIGNATURE)
#define ONE_OF_OPTION_MAP_ENTRY_SIGNATURE            SIGNATURE_32 ('O', 'O', 'M', 'E')
typedef struct {
  UINT32              Signature;
  LIST_ENTRY          Link;

  UINT16              FwKey;
  EFI_IFR_TYPE_VALUE  Value;
  
} ONE_OF_OPTION_MAP_ENTRY;



#define ONE_OF_OPTION_MAP_FROM_LINK(Record) CR(Record, ONE_OF_OPTION_MAP, Link, ONE_OF_OPTION_MAP_SIGNATURE)
#define ONE_OF_OPTION_MAP_SIGNATURE            SIGNATURE_32 ('O', 'O', 'O', 'M')
typedef struct {
  UINT32            Signature;
  LIST_ENTRY        Link;       

  UINT16            VarStoreId;

  UINT8             ValueType; //EFI_IFR_TYPE_NUM_* 

  EFI_QUESTION_ID   QuestionId;

  LIST_ENTRY        OneOfOptionMapEntryListHead; //ONE_OF_OPTION_MAP_ENTRY
} ONE_OF_OPTION_MAP;


typedef struct {
  UINT8               Type;
  EFI_IFR_TYPE_VALUE  Value;
} EFI_HII_VALUE;

#define NAME_VALUE_NODE_SIGNATURE  SIGNATURE_32 ('N', 'V', 'S', 'T')

#define FORMSET_STORAGE_SIGNATURE  SIGNATURE_32 ('F', 'S', 'T', 'G')

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;

  UINT8            Type;           // Storage type

  UINT16           VarStoreId;
  EFI_GUID         Guid;

  CHAR16           *Name;          // For EFI_IFR_VARSTORE
  UINT16           Size;

  UINT32           Attributes;     // For EFI_IFR_VARSTORE_EFI: EFI Variable attribute

} FORMSET_STORAGE;

#define FORMSET_STORAGE_FROM_LINK(a)  CR (a, FORMSET_STORAGE, Link, FORMSET_STORAGE_SIGNATURE)

#if 0

#define EXPRESSION_OPCODE_SIGNATURE  SIGNATURE_32 ('E', 'X', 'O', 'P')

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;

  UINT8             Operand;

  UINT8             Format;      // For EFI_IFR_TO_STRING, EFI_IFR_FIND
  UINT8             Flags;       // For EFI_IFR_SPAN
  UINT8             RuleId;      // For EFI_IFR_RULE_REF

  EFI_HII_VALUE     Value;       // For EFI_IFR_EQ_ID_VAL, EFI_IFR_UINT64, EFI_IFR_UINT32, EFI_IFR_UINT16, EFI_IFR_UINT8, EFI_IFR_STRING_REF1

  EFI_QUESTION_ID   QuestionId;  // For EFI_IFR_EQ_ID_ID, EFI_IFR_EQ_ID_VAL_LIST, EFI_IFR_QUESTION_REF1
  EFI_QUESTION_ID   QuestionId2;

  UINT16            ListLength;  // For EFI_IFR_EQ_ID_VAL_LIST
  UINT16            *ValueList;

  EFI_STRING_ID     DevicePath;  // For EFI_IFR_QUESTION_REF3_2, EFI_IFR_QUESTION_REF3_3
  EFI_GUID          Guid;
} EXPRESSION_OPCODE;

#define EXPRESSION_OPCODE_FROM_LINK(a)  CR (a, EXPRESSION_OPCODE, Link, EXPRESSION_OPCODE_SIGNATURE)

#define FORM_EXPRESSION_SIGNATURE  SIGNATURE_32 ('F', 'E', 'X', 'P')

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;

  UINT8             Type;            // Type for this expression

  UINT8             RuleId;          // For EFI_IFR_RULE only
  EFI_STRING_ID     Error;           // For EFI_IFR_NO_SUBMIT_IF, EFI_IFR_INCONSISTENT_IF only

  EFI_HII_VALUE     Result;          // Expression evaluation result

  LIST_ENTRY        OpCodeListHead;  // OpCodes consist of this expression (EXPRESSION_OPCODE)
} FORM_EXPRESSION;

#define FORM_EXPRESSION_FROM_LINK(a)  CR (a, FORM_EXPRESSION, Link, FORM_EXPRESSION_SIGNATURE)
#endif

#define QUESTION_DEFAULT_SIGNATURE  SIGNATURE_32 ('Q', 'D', 'F', 'T')

typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;

  UINT16              DefaultId;
  EFI_HII_VALUE       Value;              // Default value

} QUESTION_DEFAULT;

#define QUESTION_DEFAULT_FROM_LINK(a)  CR (a, QUESTION_DEFAULT, Link, QUESTION_DEFAULT_SIGNATURE)

#define QUESTION_OPTION_SIGNATURE  SIGNATURE_32 ('Q', 'O', 'P', 'T')

typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;

  EFI_STRING_ID       Text;
  UINT8               Flags;
  EFI_HII_VALUE       Value;
  EFI_IMAGE_ID        ImageId;

} QUESTION_OPTION;

#define QUESTION_OPTION_FROM_LINK(a)  CR (a, QUESTION_OPTION, Link, QUESTION_OPTION_SIGNATURE)

typedef union {
  EFI_STRING_ID       VarName;
  UINT16              VarOffset;
} VAR_STORE_INFO;

#define FORM_BROWSER_STATEMENT_SIGNATURE  SIGNATURE_32 ('F', 'S', 'T', 'A')
typedef struct {
  UINTN                 Signature;
  LIST_ENTRY            Link;

  UINT8                 Operand;          // The operand (first byte) of this Statement or Question

  //
  // Statement Header
  //
  EFI_STRING_ID         Prompt;
  EFI_STRING_ID         Help;
  EFI_STRING_ID         TextTwo;          // For EFI_IFR_TEXT

  //
  // Question Header
  //
  EFI_QUESTION_ID       QuestionId;       // The value of zero is reserved
  EFI_VARSTORE_ID       VarStoreId;       // A value of zero indicates no variable storage
  FORMSET_STORAGE       *Storage;
  VAR_STORE_INFO        VarStoreInfo;
  
  UINT16                StorageWidth;
  UINT8                 QuestionFlags;

  EFI_HII_VALUE         HiiValue;         // Edit copy for checkbox, numberic, oneof
  UINT8                 *BufferValue;     // Edit copy for string, password, orderedlist

  //
  // OpCode specific members
  //
  UINT8                 Flags;            // for EFI_IFR_CHECKBOX, EFI_IFR_DATE, EFI_IFR_NUMERIC, EFI_IFR_ONE_OF,
                                          // EFI_IFR_ORDERED_LIST, EFI_IFR_STRING,EFI_IFR_SUBTITLE,EFI_IFR_TIME, EFI_IFR_BANNER
  UINT8                 MaxContainers;    // for EFI_IFR_ORDERED_LIST

  UINT16                BannerLineNumber; // for EFI_IFR_BANNER, 1-based line number
  EFI_STRING_ID         QuestionConfig;   // for EFI_IFR_ACTION, if 0 then no configuration string will be processed

  UINT64                Minimum;          // for EFI_IFR_ONE_OF/EFI_IFR_NUMERIC, it's Min/Max value
  UINT64                Maximum;          // for EFI_IFR_STRING/EFI_IFR_PASSWORD, it's Min/Max length
  UINT64                Step;

  EFI_DEFAULT_ID        DefaultId;        // for EFI_IFR_RESET_BUTTON
  EFI_FORM_ID           RefFormId;        // for EFI_IFR_REF
  EFI_QUESTION_ID       RefQuestionId;    // for EFI_IFR_REF2
  EFI_GUID              RefFormSetId;     // for EFI_IFR_REF3
  EFI_STRING_ID         RefDevicePath;    // for EFI_IFR_REF4

  //
  // Get from IFR parsing
  //
  LIST_ENTRY            DefaultListHead;     // nested EFI_IFR_DEFAULT list (QUESTION_DEFAULT), provide default values
  LIST_ENTRY            OptionListHead;      // nested EFI_IFR_ONE_OF_OPTION list (QUESTION_OPTION)

  EFI_IMAGE_ID          ImageId;             // nested EFI_IFR_IMAGE
  UINT8                 RefreshInterval;     // nested EFI_IFR_REFRESH, refresh interval(in seconds) for Question value, 0 means no refresh
  BOOLEAN               InSubtitle;          // nesting inside of EFI_IFR_SUBTITLE

} FORM_BROWSER_STATEMENT;

#define FORM_BROWSER_STATEMENT_FROM_LINK(a)  CR (a, FORM_BROWSER_STATEMENT, Link, FORM_BROWSER_STATEMENT_SIGNATURE)

#define FORM_BROWSER_FORM_SIGNATURE  SIGNATURE_32 ('F', 'F', 'R', 'M')

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;

  UINT16            FormId;
  EFI_STRING_ID     FormTitle;

  EFI_IMAGE_ID      ImageId;

#if 0
  LIST_ENTRY        ExpressionListHead;   // List of Expressions (FORM_EXPRESSION)
#endif
  LIST_ENTRY        StatementListHead;    // List of Statements and Questions (FORM_BROWSER_STATEMENT)
} FORM_BROWSER_FORM;

#define FORM_BROWSER_FORM_FROM_LINK(a)  CR (a, FORM_BROWSER_FORM, Link, FORM_BROWSER_FORM_SIGNATURE)

#define FORMSET_DEFAULTSTORE_SIGNATURE  SIGNATURE_32 ('F', 'D', 'F', 'S')

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;

  UINT16           DefaultId;
  EFI_STRING_ID    DefaultName;
} FORMSET_DEFAULTSTORE;

#define FORMSET_DEFAULTSTORE_FROM_LINK(a)  CR (a, FORMSET_DEFAULTSTORE, Link, FORMSET_DEFAULTSTORE_SIGNATURE)

typedef struct {
  EFI_HII_HANDLE                  HiiHandle;

  UINTN                           IfrBinaryLength;
  UINT8                           *IfrBinaryData;

  EFI_GUID                        Guid;
  EFI_STRING_ID                   FormSetTitle;
  EFI_STRING_ID                   Help;
  UINT16                          Class;
  UINT16                          SubClass;
  EFI_IMAGE_ID                    ImageId;

  FORM_BROWSER_STATEMENT          *StatementBuffer;     // Buffer for all Statements and Questions
#if 0
  EXPRESSION_OPCODE               *ExpressionBuffer;    // Buffer for all Expression OpCode
#endif

  LIST_ENTRY                      StorageListHead;      // Storage list (FORMSET_STORAGE)
  LIST_ENTRY                      DefaultStoreListHead; // DefaultStore list (FORMSET_DEFAULTSTORE)
  LIST_ENTRY                      FormListHead;         // Form list (FORM_BROWSER_FORM)

  LIST_ENTRY                      OneOfOptionMapListHead; //ONE_OF_OPTION_MAP

  UINT16                          MaxQuestionId;

  //
  // Added for Framework HII Thunk. 
  // Default Variable Storage built from a Framework VFR file using UEFI VFR Compiler in Compatibility mode is determined 
  // by priority rules defined in GetFormsetDefaultVarstoreId (). See the function description for details.
  //
  EFI_VARSTORE_ID                 DefaultVarStoreId;
  CHAR16                          *OriginalDefaultVarStoreName;

  UINTN                           NumberOfStatement;
    
} FORM_BROWSER_FORMSET;


/**
  Parse opcodes in the formset IFR binary.

  @param  FormSet                Pointer of the FormSet data structure.

  @retval EFI_SUCCESS            Opcode parse success.
  @retval Other                  Opcode parse fail.

**/
EFI_STATUS
ParseOpCodes (
  IN FORM_BROWSER_FORMSET              *FormSet
  );

/**
  Free resources allocated for a FormSet

  @param  FormSet                Pointer of the FormSet

  @return None.

**/
VOID
DestroyFormSet (
  IN OUT FORM_BROWSER_FORMSET  *FormSet
  );

#endif

