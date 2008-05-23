/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Setup.h

Abstract:


Revision History


**/

#ifndef _SETUP_H
#define _SETUP_H


#include <PiDxe.h>

#include <Protocol/Print.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>

#include <MdeModuleHii.h>

#include <Library/GraphicsLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IfrSupportLib.h>
#include <Library/ExtendedIfrSupportLib.h>
#include <Library/HiiLib.h>
#include <Library/ExtendedHiiLib.h>

#include "Colors.h"

//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//

extern UINT8  SetupBrowserStrings[];

//
// Screen definitions
//
#define BANNER_HEIGHT                 6
#define BANNER_COLUMNS                3

#define FRONT_PAGE_HEADER_HEIGHT      6
#define NONE_FRONT_PAGE_HEADER_HEIGHT 3
#define LEFT_SKIPPED_COLUMNS          4
#define FOOTER_HEIGHT                 4
#define STATUS_BAR_HEIGHT             1
#define SCROLL_ARROW_HEIGHT           1
#define POPUP_PAD_SPACE_COUNT         5
#define POPUP_FRAME_WIDTH             2

//
// Definition for function key setting
//
#define NONE_FUNCTION_KEY_SETTING     0
#define DEFAULT_FUNCTION_KEY_SETTING  (FUNCTION_ONE | FUNCTION_TWO | FUNCTION_NINE | FUNCTION_TEN)

#define FUNCTION_ONE                  (1 << 0)
#define FUNCTION_TWO                  (1 << 1)
#define FUNCTION_NINE                 (1 << 2)
#define FUNCTION_TEN                  (1 << 3)

typedef struct {
  EFI_GUID  FormSetGuid;
  UINTN     KeySetting;
} FUNCTIION_KEY_SETTING;

//
// Character definitions
//
#define CHAR_SPACE              0x0020
#define UPPER_LOWER_CASE_OFFSET 0x20

//
// Time definitions
//
#define ONE_SECOND  10000000

//
// Display definitions
//
#define LEFT_HYPER_DELIMITER      L'<'
#define RIGHT_HYPER_DELIMITER     L'>'

#define LEFT_ONEOF_DELIMITER      L'<'
#define RIGHT_ONEOF_DELIMITER     L'>'

#define LEFT_NUMERIC_DELIMITER    L'['
#define RIGHT_NUMERIC_DELIMITER   L']'

#define LEFT_CHECKBOX_DELIMITER   L'['
#define RIGHT_CHECKBOX_DELIMITER  L']'

#define CHECK_ON                  L'X'
#define CHECK_OFF                 L' '

#define TIME_SEPARATOR            L':'
#define DATE_SEPARATOR            L'/'

#define YES_ANSWER                L'Y'
#define NO_ANSWER                 L'N'

//
// This is the Input Error Message
//
#define INPUT_ERROR 1

//
// This is the NV RAM update required Message
//
#define NV_UPDATE_REQUIRED  2

//
// Refresh the Status Bar with flags
//
#define REFRESH_STATUS_BAR  0xff

//
// Incremental string lenght of ConfigRequest
//
#define CONFIG_REQUEST_STRING_INCREMENTAL  1024

//
// HII value compare result
//
#define HII_VALUE_UNDEFINED     0
#define HII_VALUE_EQUAL         1
#define HII_VALUE_LESS_THAN     2
#define HII_VALUE_GREATER_THAN  3

//
// Incremental size of stack for expression
//
#define EXPRESSION_STACK_SIZE_INCREMENT    0x100


#define EFI_SPECIFICATION_ERRATA_VERSION   0

#define EFI_IFR_SPECIFICATION_VERSION  \
        ((((EFI_SPECIFICATION_VERSION) >> 8) & 0xff00) | \
         (((EFI_SPECIFICATION_VERSION) & 0xf) << 4) | \
         ((EFI_SPECIFICATION_ERRATA_VERSION) & 0xf))

#define SETUP_DRIVER_SIGNATURE EFI_SIGNATURE_32 ('F', 'B', 'D', 'V')
typedef struct {
  UINT32                             Signature;

  EFI_HANDLE                         Handle;

  //
  // Produced protocol
  //
  EFI_FORM_BROWSER2_PROTOCOL         FormBrowser2;
  EFI_PRINT_PROTOCOL                 Print;

} SETUP_DRIVER_PRIVATE_DATA;

typedef struct {
  EFI_STRING_ID  Banner[BANNER_HEIGHT][BANNER_COLUMNS];
} BANNER_DATA;

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

typedef struct {
  UINT8               Type;
  EFI_IFR_TYPE_VALUE  Value;
} EFI_HII_VALUE;

#define NAME_VALUE_NODE_SIGNATURE  EFI_SIGNATURE_32 ('N', 'V', 'S', 'T')

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;
  CHAR16           *Name;
  CHAR16           *Value;
  CHAR16           *EditValue;
} NAME_VALUE_NODE;

#define NAME_VALUE_NODE_FROM_LINK(a)  CR (a, NAME_VALUE_NODE, Link, NAME_VALUE_NODE_SIGNATURE)

#define FORMSET_STORAGE_SIGNATURE  EFI_SIGNATURE_32 ('F', 'S', 'T', 'G')

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;

  UINT8            Type;           // Storage type

  UINT16           VarStoreId;
  EFI_GUID         Guid;

  CHAR16           *Name;          // For EFI_IFR_VARSTORE
  UINT16           Size;
  UINT8            *Buffer;
  UINT8            *EditBuffer;    // Edit copy for Buffer Storage

  LIST_ENTRY       NameValueListHead; // List of NAME_VALUE_NODE

  UINT32           Attributes;     // For EFI_IFR_VARSTORE_EFI: EFI Variable attribute

  CHAR16           *ConfigHdr;     // <ConfigHdr>
  CHAR16           *ConfigRequest; // <ConfigRequest> = <ConfigHdr> + <RequestElement>
  UINTN            ElementCount;   // Number of <RequestElement> in the <ConfigRequest>
  UINTN            SpareStrLen;    // Spare length of ConfigRequest string buffer
} FORMSET_STORAGE;

#define FORMSET_STORAGE_FROM_LINK(a)  CR (a, FORMSET_STORAGE, Link, FORMSET_STORAGE_SIGNATURE)

#define EXPRESSION_OPCODE_SIGNATURE  EFI_SIGNATURE_32 ('E', 'X', 'O', 'P')

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;

  UINT8             Operand;

  UINT8             Format;      // For EFI_IFR_TO_STRING, EFI_IFR_FIND
  UINT8             Flags;       // For EFI_IFR_SPAN
  UINT8             RuleId;      // For EFI_IFR_RULE_REF

  EFI_HII_VALUE     Value;       // For EFI_IFR_EQ_ID_VAL, EFI_IFR_UINT64, EFI_IFR_UINT32, EFI_IFR_UINT16, EFI_IFR_UINT8, EFI_IFR_STRING_REF1

  EFI_QUESTION_ID   QuestionId;  // For EFI_IFR_EQ_ID_ID, EFI_IFR_EQ_ID_LIST, EFI_IFR_QUESTION_REF1
  EFI_QUESTION_ID   QuestionId2;

  UINT16            ListLength;  // For EFI_IFR_EQ_ID_LIST
  UINT16            *ValueList;

  EFI_STRING_ID     DevicePath;  // For EFI_IFR_QUESTION_REF3_2, EFI_IFR_QUESTION_REF3_3
  EFI_GUID          Guid;
} EXPRESSION_OPCODE;

#define EXPRESSION_OPCODE_FROM_LINK(a)  CR (a, EXPRESSION_OPCODE, Link, EXPRESSION_OPCODE_SIGNATURE)

#define FORM_EXPRESSION_SIGNATURE  EFI_SIGNATURE_32 ('F', 'E', 'X', 'P')

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

#define QUESTION_DEFAULT_SIGNATURE  EFI_SIGNATURE_32 ('Q', 'D', 'F', 'T')

typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;

  UINT16              DefaultId;
  EFI_HII_VALUE       Value;              // Default value

  FORM_EXPRESSION     *ValueExpression;   // Not-NULL indicates default value is provided by EFI_IFR_VALUE
} QUESTION_DEFAULT;

#define QUESTION_DEFAULT_FROM_LINK(a)  CR (a, QUESTION_DEFAULT, Link, QUESTION_DEFAULT_SIGNATURE)

#define QUESTION_OPTION_SIGNATURE  EFI_SIGNATURE_32 ('Q', 'O', 'P', 'T')

typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;

  EFI_STRING_ID       Text;
  UINT8               Flags;
  EFI_HII_VALUE       Value;
  EFI_IMAGE_ID        ImageId;

  FORM_EXPRESSION     *SuppressExpression; // Non-NULL indicates nested inside of SuppressIf
} QUESTION_OPTION;

#define QUESTION_OPTION_FROM_LINK(a)  CR (a, QUESTION_OPTION, Link, QUESTION_OPTION_SIGNATURE)

#define FORM_BROWSER_STATEMENT_SIGNATURE  EFI_SIGNATURE_32 ('F', 'S', 'T', 'A')
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
  union {
    EFI_STRING_ID       VarName;
    UINT16              VarOffset;
  }  VarStoreInfo;
  UINT16                StorageWidth;
  UINT8                 QuestionFlags;
  CHAR16                *VariableName;    // Name/Value or EFI Variable name
  CHAR16                *BlockName;       // Buffer storage block name: "OFFSET=...WIDTH=..."

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
  FORM_EXPRESSION       *ValueExpression;    // nested EFI_IFR_VALUE, provide Question value and indicate Question is ReadOnly
  LIST_ENTRY            DefaultListHead;     // nested EFI_IFR_DEFAULT list (QUESTION_DEFAULT), provide default values
  LIST_ENTRY            OptionListHead;      // nested EFI_IFR_ONE_OF_OPTION list (QUESTION_OPTION)

  EFI_IMAGE_ID          ImageId;             // nested EFI_IFR_IMAGE
  UINT8                 RefreshInterval;     // nested EFI_IFR_REFRESH, refresh interval(in seconds) for Question value, 0 means no refresh
  BOOLEAN               InSubtitle;          // nesting inside of EFI_IFR_SUBTITLE

  LIST_ENTRY            InconsistentListHead;// nested inconsistent expression list (FORM_EXPRESSION)
  LIST_ENTRY            NoSubmitListHead;    // nested nosubmit expression list (FORM_EXPRESSION)
  FORM_EXPRESSION       *GrayOutExpression;  // nesting inside of GrayOutIf
  FORM_EXPRESSION       *SuppressExpression; // nesting inside of SuppressIf

} FORM_BROWSER_STATEMENT;

#define FORM_BROWSER_STATEMENT_FROM_LINK(a)  CR (a, FORM_BROWSER_STATEMENT, Link, FORM_BROWSER_STATEMENT_SIGNATURE)

#define FORM_BROWSER_FORM_SIGNATURE  EFI_SIGNATURE_32 ('F', 'F', 'R', 'M')

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;

  UINT16            FormId;
  EFI_STRING_ID     FormTitle;

  EFI_IMAGE_ID      ImageId;

  LIST_ENTRY        ExpressionListHead;   // List of Expressions (FORM_EXPRESSION)
  LIST_ENTRY        StatementListHead;    // List of Statements and Questions (FORM_BROWSER_STATEMENT)
} FORM_BROWSER_FORM;

#define FORM_BROWSER_FORM_FROM_LINK(a)  CR (a, FORM_BROWSER_FORM, Link, FORM_BROWSER_FORM_SIGNATURE)

#define FORMSET_DEFAULTSTORE_SIGNATURE  EFI_SIGNATURE_32 ('F', 'D', 'F', 'S')

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;

  UINT16           DefaultId;
  EFI_STRING_ID    DefaultName;
} FORMSET_DEFAULTSTORE;

#define FORMSET_DEFAULTSTORE_FROM_LINK(a)  CR (a, FORMSET_DEFAULTSTORE, Link, FORMSET_DEFAULTSTORE_SIGNATURE)

typedef struct {
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;

  UINTN                           IfrBinaryLength;
  UINT8                           *IfrBinaryData;

  EFI_GUID                        Guid;
  EFI_STRING_ID                   FormSetTitle;
  EFI_STRING_ID                   Help;
  UINT16                          Class;
  UINT16                          SubClass;
  EFI_IMAGE_ID                    ImageId;

  FORM_BROWSER_STATEMENT          *StatementBuffer;     // Buffer for all Statements and Questions
  EXPRESSION_OPCODE               *ExpressionBuffer;    // Buffer for all Expression OpCode

  LIST_ENTRY                      StorageListHead;      // Storage list (FORMSET_STORAGE)
  LIST_ENTRY                      DefaultStoreListHead; // DefaultStore list (FORMSET_DEFAULTSTORE)
  LIST_ENTRY                      FormListHead;         // Form list (FORM_BROWSER_FORM)
} FORM_BROWSER_FORMSET;


extern EFI_HII_DATABASE_PROTOCOL         *mHiiDatabase;
extern EFI_HII_STRING_PROTOCOL           *mHiiString;
extern EFI_HII_CONFIG_ROUTING_PROTOCOL   *mHiiConfigRouting;

extern BANNER_DATA           *BannerData;
extern EFI_HII_HANDLE        FrontPageHandle;
extern UINTN                 gClassOfVfr;
extern UINTN                 gFunctionKeySetting;
extern BOOLEAN               gResetRequired;
extern BOOLEAN               gNvUpdateRequired;
extern EFI_HII_HANDLE        gHiiHandle;
extern BOOLEAN               gFirstIn;
extern UINT16                gDirection;
extern EFI_SCREEN_DESCRIPTOR gScreenDimensions;
extern BOOLEAN               gUpArrow;
extern BOOLEAN               gDownArrow;

//
// Browser Global Strings
//
extern CHAR16            *gFunctionOneString;
extern CHAR16            *gFunctionTwoString;
extern CHAR16            *gFunctionNineString;
extern CHAR16            *gFunctionTenString;
extern CHAR16            *gEnterString;
extern CHAR16            *gEnterCommitString;
extern CHAR16            *gEscapeString;
extern CHAR16            *gSaveFailed;
extern CHAR16            *gMoveHighlight;
extern CHAR16            *gMakeSelection;
extern CHAR16            *gDecNumericInput;
extern CHAR16            *gHexNumericInput;
extern CHAR16            *gToggleCheckBox;
extern CHAR16            *gPromptForData;
extern CHAR16            *gPromptForPassword;
extern CHAR16            *gPromptForNewPassword;
extern CHAR16            *gConfirmPassword;
extern CHAR16            *gConfirmError;
extern CHAR16            *gPassowordInvalid;
extern CHAR16            *gPressEnter;
extern CHAR16            *gEmptyString;
extern CHAR16            *gAreYouSure;
extern CHAR16            *gYesResponse;
extern CHAR16            *gNoResponse;
extern CHAR16            *gMiniString;
extern CHAR16            *gPlusString;
extern CHAR16            *gMinusString;
extern CHAR16            *gAdjustNumber;

extern CHAR16            gPromptBlockWidth;
extern CHAR16            gOptionBlockWidth;
extern CHAR16            gHelpBlockWidth;

extern EFI_GUID          gZeroGuid;
extern EFI_GUID          gTianoHiiIfrGuid;

//
// Global Procedure Defines
//
VOID
InitializeBrowserStrings (
  VOID
  )
;

UINTN
_Print (
  IN CHAR16                         *fmt,
  ...
  )
;

UINTN
PrintString (
  CHAR16       *String
  )
;

UINTN
PrintChar (
  CHAR16       Character
  )
;

UINTN
PrintAt (
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *fmt,
  ...
  )
;

UINTN
PrintStringAt (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       *String
  )
;

UINTN
PrintCharAt (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       Character
  )
;

EFI_STATUS
ParseOpCodes (
  IN FORM_BROWSER_FORMSET              *FormSet
  )
;

VOID
DestroyFormSet (
  IN OUT FORM_BROWSER_FORMSET  *FormSet
  )
;

VOID
DisplayPageFrame (
  VOID
  )
;

EFI_STRING_ID
NewString (
  IN  CHAR16                   *String,
  IN  EFI_HII_HANDLE           HiiHandle
  )
;

EFI_STATUS
DeleteString (
  IN  EFI_STRING_ID            StringId,
  IN  EFI_HII_HANDLE           HiiHandle
  )
;
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
;

VOID
CreateSharedPopUp (
  IN  UINTN                       RequestedWidth,
  IN  UINTN                       NumberOfLines,
  IN  CHAR16                      **ArrayOfStrings
  )
;

EFI_STATUS
CreateDialog (
  IN  UINTN                       NumberOfLines,
  IN  BOOLEAN                     HotKey,
  IN  UINTN                       MaximumStringSize,
  OUT CHAR16                      *StringBuffer,
  OUT EFI_INPUT_KEY               *KeyValue,
  IN  CHAR16                      *String,
  ...
  )
;

EFI_STATUS
GetQuestionValue (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN OUT FORM_BROWSER_STATEMENT       *Question,
  IN BOOLEAN                          Cached
  )
;

EFI_STATUS
SetQuestionValue (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN OUT FORM_BROWSER_STATEMENT       *Question,
  IN BOOLEAN                          Cached
  )
;

EFI_STATUS
ValidateQuestion (
  IN  FORM_BROWSER_FORMSET            *FormSet,
  IN  FORM_BROWSER_FORM               *Form,
  IN  FORM_BROWSER_STATEMENT          *Question,
  IN  UINTN                           Type
  )
;

EFI_STATUS
SubmitForm (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form
  )
;

EFI_STATUS
GetQuestionDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN FORM_BROWSER_STATEMENT           *Question,
  IN UINT16                           DefaultId
  )
;

EFI_STATUS
InitializeCurrentSetting (
  IN OUT FORM_BROWSER_FORMSET             *FormSet
  )
;

EFI_STATUS
InitializeFormSet (
  IN  EFI_HII_HANDLE                   Handle,
  IN OUT EFI_GUID                      *FormSetGuid,
  OUT FORM_BROWSER_FORMSET             *FormSet
  )
;

EFI_STATUS
ExtractFormDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN  FORM_BROWSER_FORM               *Form,
  IN UINT16                           DefaultId
  )
;

EFI_STATUS
LoadFormConfig (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form
  )
;

EFI_STATUS
StorageToConfigResp (
  IN FORMSET_STORAGE         *Storage,
  IN CHAR16                  **ConfigResp
  )
;

EFI_STATUS
ConfigRespToStorage (
  IN FORMSET_STORAGE         *Storage,
  IN CHAR16                  *ConfigResp
  )
;

EFI_STATUS
LoadStorage (
  IN FORM_BROWSER_FORMSET    *FormSet,
  IN FORMSET_STORAGE         *Storage
  )
;

EFI_STATUS
GetIfrBinaryData (
  IN  EFI_HII_HANDLE   Handle,
  IN OUT EFI_GUID      *FormSetGuid,
  OUT UINTN            *BinaryLength,
  OUT UINT8            **BinaryData
  )
;

EFI_STATUS
EFIAPI
SendForm (
  IN  CONST EFI_FORM_BROWSER2_PROTOCOL *This,
  IN  EFI_HII_HANDLE                   *Handles,
  IN  UINTN                            HandleCount,
  IN  EFI_GUID                         *FormSetGuid, OPTIONAL
  IN  UINT16                           FormId, OPTIONAL
  IN  CONST EFI_SCREEN_DESCRIPTOR      *ScreenDimensions, OPTIONAL
  OUT EFI_BROWSER_ACTION_REQUEST       *ActionRequest  OPTIONAL
  )
;

EFI_STATUS
EFIAPI
BrowserCallback (
  IN CONST EFI_FORM_BROWSER2_PROTOCOL  *This,
  IN OUT UINTN                         *ResultsDataSize,
  IN OUT EFI_STRING                    ResultsData,
  IN BOOLEAN                           RetrieveData,
  IN CONST EFI_GUID                    *VariableGuid, OPTIONAL
  IN CONST CHAR16                      *VariableName  OPTIONAL
  )
;

#endif
