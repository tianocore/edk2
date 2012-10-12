/** @file
Private MACRO, structure and function definitions for Setup Browser module.

Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#ifndef _SETUP_H_
#define _SETUP_H_


#include <PiDxe.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/FormBrowserEx.h>
#include <Protocol/DevicePath.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/UserManager.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>

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
#define BANNER_LEFT_COLUMN_INDENT     1

#define FRONT_PAGE_HEADER_HEIGHT      6
#define NONE_FRONT_PAGE_HEADER_HEIGHT 3
#define LEFT_SKIPPED_COLUMNS          3
#define FOOTER_HEIGHT                 4
#define STATUS_BAR_HEIGHT             1
#define SCROLL_ARROW_HEIGHT           1
#define POPUP_PAD_SPACE_COUNT         5
#define POPUP_FRAME_WIDTH             2

//
// Definition for function key setting
//
#define NONE_FUNCTION_KEY_SETTING     0
#define ENABLE_FUNCTION_KEY_SETTING   1

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

#define EFI_IFR_SPECIFICATION_VERSION  (UINT16) (((EFI_SYSTEM_TABLE_REVISION >> 16) << 8) | (((EFI_SYSTEM_TABLE_REVISION & 0xFFFF) / 10) << 4) | ((EFI_SYSTEM_TABLE_REVISION & 0xFFFF) % 10))


#define SETUP_DRIVER_SIGNATURE SIGNATURE_32 ('F', 'B', 'D', 'V')
typedef struct {
  UINT32                             Signature;

  EFI_HANDLE                         Handle;

  //
  // Produced protocol
  //
  EFI_FORM_BROWSER2_PROTOCOL          FormBrowser2;
  
  EFI_FORM_BROWSER_EXTENSION_PROTOCOL FormBrowserEx;

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
#define EFI_HII_EXPRESSION_READ              7
#define EFI_HII_EXPRESSION_WRITE             8

#define EFI_HII_VARSTORE_BUFFER              0
#define EFI_HII_VARSTORE_NAME_VALUE          1
#define EFI_HII_VARSTORE_EFI_VARIABLE        2
#define EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER 3

#define FORM_INCONSISTENT_VALIDATION         0
#define FORM_NO_SUBMIT_VALIDATION            1

#define FORMSET_CLASS_PLATFORM_SETUP         0x0001
#define FORMSET_CLASS_FRONT_PAGE             0x0002

typedef struct {
  UINT8               Type;
  UINT8               *Buffer;
  UINT16              BufferLen;
  EFI_IFR_TYPE_VALUE  Value;
} EFI_HII_VALUE;

#define NAME_VALUE_NODE_SIGNATURE  SIGNATURE_32 ('N', 'V', 'S', 'T')

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;
  CHAR16           *Name;
  CHAR16           *Value;
  CHAR16           *EditValue;
} NAME_VALUE_NODE;

#define NAME_VALUE_NODE_FROM_LINK(a)  CR (a, NAME_VALUE_NODE, Link, NAME_VALUE_NODE_SIGNATURE)

#define FORMSET_STORAGE_SIGNATURE  SIGNATURE_32 ('F', 'S', 'T', 'G')

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

typedef union {
  EFI_STRING_ID         VarName;
  UINT16                VarOffset;
} VAR_STORE_INFO;

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

  FORMSET_STORAGE   *VarStorage; // For EFI_IFR_SET, EFI_IFR_GET
  VAR_STORE_INFO    VarStoreInfo;// For EFI_IFR_SET, EFI_IFR_GET
  UINT8             ValueType;   // For EFI_IFR_SET, EFI_IFR_GET
  UINT8             ValueWidth;  // For EFI_IFR_SET, EFI_IFR_GET
  CHAR16            *ValueName;  // For EFI_IFR_SET, EFI_IFR_GET
  LIST_ENTRY        MapExpressionList;   // nested expressions inside of Map opcode.
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

#define FORM_EXPRESSION_LIST_SIGNATURE  SIGNATURE_32 ('F', 'E', 'X', 'R')

typedef struct {
    UINTN               Signature;
    UINTN               Count;
    FORM_EXPRESSION    *Expression[1];   // Array[Count] of expressions
} FORM_EXPRESSION_LIST;

#define QUESTION_DEFAULT_SIGNATURE  SIGNATURE_32 ('Q', 'D', 'F', 'T')

typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Link;

  UINT16              DefaultId;
  EFI_HII_VALUE       Value;              // Default value

  FORM_EXPRESSION     *ValueExpression;   // Not-NULL indicates default value is provided by EFI_IFR_VALUE
} QUESTION_DEFAULT;

#define QUESTION_DEFAULT_FROM_LINK(a)  CR (a, QUESTION_DEFAULT, Link, QUESTION_DEFAULT_SIGNATURE)

#define QUESTION_OPTION_SIGNATURE  SIGNATURE_32 ('Q', 'O', 'P', 'T')

typedef struct {
  UINTN                Signature;
  LIST_ENTRY           Link;

  EFI_STRING_ID        Text;
  UINT8                Flags;
  EFI_HII_VALUE        Value;
  EFI_IMAGE_ID         ImageId;

  FORM_EXPRESSION_LIST *SuppressExpression; // Non-NULL indicates nested inside of SuppressIf
} QUESTION_OPTION;

#define QUESTION_OPTION_FROM_LINK(a)  CR (a, QUESTION_OPTION, Link, QUESTION_OPTION_SIGNATURE)

typedef enum {
  ExpressFalse = 0,
  ExpressGrayOut,  
  ExpressSuppress,
  ExpressDisable
} EXPRESS_RESULT;

typedef enum {
  ExpressNone = 0,
  ExpressForm,  
  ExpressStatement,
  ExpressOption
} EXPRESS_LEVEL;

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
  CHAR16                *VariableName;    // Name/Value or EFI Variable name
  CHAR16                *BlockName;       // Buffer storage block name: "OFFSET=...WIDTH=..."

  EFI_HII_VALUE         HiiValue;         // Edit copy for checkbox, numberic, oneof
  UINT8                 *BufferValue;     // Edit copy for string, password, orderedlist
  UINT8                 ValueType;        // Data type for orderedlist value array

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
  EFI_GUID              RefreshGuid;      // for EFI_IFR_REFRESH_ID
  BOOLEAN               Locked;           // Whether this statement is locked.
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
  FORM_EXPRESSION_LIST  *Expression;         // nesting inside of GrayOutIf/DisableIf/SuppressIf

  FORM_EXPRESSION       *ReadExpression;     // nested EFI_IFR_READ, provide this question value by read expression.
  FORM_EXPRESSION       *WriteExpression;    // nested EFI_IFR_WRITE, evaluate write expression after this question value is set.
} FORM_BROWSER_STATEMENT;

#define FORM_BROWSER_STATEMENT_FROM_LINK(a)  CR (a, FORM_BROWSER_STATEMENT, Link, FORM_BROWSER_STATEMENT_SIGNATURE)

#define FORM_BROWSER_CONFIG_REQUEST_SIGNATURE  SIGNATURE_32 ('F', 'C', 'R', 'S')
typedef struct {
  UINTN                 Signature;
  LIST_ENTRY            Link;

  CHAR16                *ConfigRequest; // <ConfigRequest> = <ConfigHdr> + <RequestElement>
  UINTN                 ElementCount;   // Number of <RequestElement> in the <ConfigRequest>  
  UINTN                 SpareStrLen;

  FORMSET_STORAGE       *Storage;
} FORM_BROWSER_CONFIG_REQUEST;
#define FORM_BROWSER_CONFIG_REQUEST_FROM_LINK(a)  CR (a, FORM_BROWSER_CONFIG_REQUEST, Link, FORM_BROWSER_CONFIG_REQUEST_SIGNATURE)

#define FORM_BROWSER_FORM_SIGNATURE  SIGNATURE_32 ('F', 'F', 'R', 'M')
#define STANDARD_MAP_FORM_TYPE 0x01

typedef struct {
  UINTN                Signature;
  LIST_ENTRY           Link;

  UINT16               FormId;               // FormId of normal form or formmap form.
  EFI_STRING_ID        FormTitle;            // FormTile of normal form, or FormMapMethod title of formmap form.
  UINT16               FormType;             // Specific form type for the different form.

  EFI_IMAGE_ID         ImageId;

  BOOLEAN              ModalForm;            // Whether this is a modal form.
  BOOLEAN              Locked;               // Whether this form is locked.

  BOOLEAN              NvUpdateRequired;     // Whether this form has NV update request.

  LIST_ENTRY           ExpressionListHead;   // List of Expressions (FORM_EXPRESSION)
  LIST_ENTRY           StatementListHead;    // List of Statements and Questions (FORM_BROWSER_STATEMENT)
  LIST_ENTRY           ConfigRequestHead;    // List of configreques for all storage.
  FORM_EXPRESSION_LIST *SuppressExpression;  // nesting inside of SuppressIf
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

#define FORM_BROWSER_FORMSET_SIGNATURE  SIGNATURE_32 ('F', 'B', 'F', 'S')

typedef struct {
  UINTN                           Signature;
  LIST_ENTRY                      Link;
  EFI_HII_HANDLE                  HiiHandle;      // unique id for formset.
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;

  UINTN                           IfrBinaryLength;
  UINT8                           *IfrBinaryData;

  EFI_GUID                        Guid;
  EFI_STRING_ID                   FormSetTitle;
  EFI_STRING_ID                   Help;
  UINT8                           NumberOfClassGuid;
  EFI_GUID                        ClassGuid[3];         // Up to three ClassGuid
  UINT16                          Class;                // Tiano extended Class code
  UINT16                          SubClass;             // Tiano extended Subclass code
  EFI_IMAGE_ID                    ImageId;

  FORM_BROWSER_STATEMENT          *StatementBuffer;     // Buffer for all Statements and Questions
  EXPRESSION_OPCODE               *ExpressionBuffer;    // Buffer for all Expression OpCode

  LIST_ENTRY                      StorageListHead;      // Storage list (FORMSET_STORAGE)
  LIST_ENTRY                      DefaultStoreListHead; // DefaultStore list (FORMSET_DEFAULTSTORE)
  LIST_ENTRY                      FormListHead;         // Form list (FORM_BROWSER_FORM)
  LIST_ENTRY                      ExpressionListHead;   // List of Expressions (FORM_EXPRESSION)
} FORM_BROWSER_FORMSET;

#define FORM_BROWSER_FORMSET_FROM_LINK(a)  CR (a, FORM_BROWSER_FORMSET, Link, FORM_BROWSER_FORMSET_SIGNATURE)

#define BROWSER_CONTEXT_SIGNATURE  SIGNATURE_32 ('B', 'C', 'T', 'X')

typedef struct {
  UINTN                 Signature;
  LIST_ENTRY            Link;

  //
  // Globals defined in Setup.c
  //
  BANNER_DATA           *BannerData;
  UINTN                 ClassOfVfr;
  UINTN                 FunctionKeySetting;
  BOOLEAN               ResetRequired;
  UINT16                Direction;
  EFI_SCREEN_DESCRIPTOR ScreenDimensions;
  CHAR16                *EnterString;
  CHAR16                *EnterCommitString;
  CHAR16                *EnterEscapeString;
  CHAR16                *EscapeString;
  CHAR16                *MoveHighlight;
  CHAR16                *MakeSelection;
  CHAR16                *DecNumericInput;
  CHAR16                *HexNumericInput;
  CHAR16                *ToggleCheckBox;
  CHAR16                *PromptForData;
  CHAR16                *PromptForPassword;
  CHAR16                *PromptForNewPassword;
  CHAR16                *ConfirmPassword;
  CHAR16                *ConfirmError;
  CHAR16                *PassowordInvalid;
  CHAR16                *PressEnter;
  CHAR16                *EmptyString;
  CHAR16                *AreYouSure;
  CHAR16                *YesResponse;
  CHAR16                *NoResponse;
  CHAR16                *MiniString;
  CHAR16                *PlusString;
  CHAR16                *MinusString;
  CHAR16                *AdjustNumber;
  CHAR16                *SaveChanges;
  CHAR16                *OptionMismatch;
  CHAR16                *FormSuppress;
  CHAR16                PromptBlockWidth;
  CHAR16                OptionBlockWidth;
  CHAR16                HelpBlockWidth;
  FORM_BROWSER_FORMSET  *OldFormSet;

  //
  // Globals defined in Ui.c
  //
  LIST_ENTRY           MenuOption;
  VOID                 *MenuRefreshHead;
} BROWSER_CONTEXT;

#define BROWSER_CONTEXT_FROM_LINK(a)  CR (a, BROWSER_CONTEXT, Link, BROWSER_CONTEXT_SIGNATURE)

#define BROWSER_HOT_KEY_SIGNATURE  SIGNATURE_32 ('B', 'H', 'K', 'S')

typedef struct {
  UINTN                 Signature;
  LIST_ENTRY            Link;
  
  EFI_INPUT_KEY         *KeyData;
  IN UINT32             Action;
  IN UINT16             DefaultId;
  IN EFI_STRING         HelpString;
} BROWSER_HOT_KEY;

#define BROWSER_HOT_KEY_FROM_LINK(a)  CR (a, BROWSER_HOT_KEY, Link, BROWSER_HOT_KEY_SIGNATURE)

//
// Scope for get defaut value. It may be GetDefaultForNoStorage, GetDefaultForStorage or GetDefaultForAll.
//
typedef enum {
  GetDefaultForNoStorage,       // Get default value for question which not has storage.
  GetDefaultForStorage,         // Get default value for question which has storage.
  GetDefaultForAll,             // Get default value for all questions.
  GetDefaultForMax              // Invalid value.
} BROWSER_GET_DEFAULT_VALUE;

//
// Get/set question value from/to.
//
typedef enum {
  GetSetValueWithEditBuffer,       // Get/Set question value from/to editbuffer in the storage.
  GetSetValueWithBuffer,           // Get/Set question value from/to buffer in the storage.
  GetSetValueWithHiiDriver,        // Get/Set question value from/to hii driver.
  GetSetValueWithMax               // Invalid value.
} GET_SET_QUESTION_VALUE_WITH;

extern EFI_HII_DATABASE_PROTOCOL         *mHiiDatabase;
extern EFI_HII_STRING_PROTOCOL           *mHiiString;
extern EFI_HII_CONFIG_ROUTING_PROTOCOL   *mHiiConfigRouting;

extern BANNER_DATA           *gBannerData;
extern EFI_HII_HANDLE        gFrontPageHandle;
extern UINTN                 gClassOfVfr;
extern UINTN                 gFunctionKeySetting;
extern BOOLEAN               gResetRequired;
extern EFI_HII_HANDLE        gHiiHandle;
extern UINT16                gDirection;
extern EFI_SCREEN_DESCRIPTOR gScreenDimensions;

extern FORM_BROWSER_FORMSET  *gOldFormSet;
extern LIST_ENTRY            gBrowserFormSetList;
extern LIST_ENTRY            gBrowserHotKeyList;
extern BROWSER_SETTING_SCOPE gBrowserSettingScope;
extern EXIT_HANDLER          ExitHandlerFunction;
extern UINTN                 gFooterHeight;

//
// Browser Global Strings
//
extern CHAR16            *gDiscardFailed;
extern CHAR16            *gDefaultFailed;
extern CHAR16            *gEnterString;
extern CHAR16            *gEnterCommitString;
extern CHAR16            *gEnterEscapeString;
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
extern CHAR16            *gSaveChanges;
extern CHAR16            *gOptionMismatch;
extern CHAR16            *gFormSuppress;

extern CHAR16            gPromptBlockWidth;
extern CHAR16            gOptionBlockWidth;
extern CHAR16            gHelpBlockWidth;

extern EFI_GUID          gZeroGuid;
extern EFI_GUID          gTianoHiiIfrGuid;

#include "Ui.h"
//
// Global Procedure Defines
//

/**
  Initialize the HII String Token to the correct values.

**/
VOID
InitializeBrowserStrings (
  VOID
  );

/**
  Prints a unicode string to the default console,
  using L"%s" format.

  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
PrintString (
  IN CHAR16       *String
  );

/**
  Prints a chracter to the default console,
  using L"%c" format.

  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
PrintChar (
  CHAR16       Character
  );

/**
  Prints a formatted unicode string to the default console, at
  the supplied cursor position.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at
  @param  Fmt        Format string
  @param  ...        Variable argument list for formating string.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintAt (
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *Fmt,
  ...
  );

/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at
  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
PrintStringAt (
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *String
  );

/**
  Prints a chracter to the default console, at
  the supplied cursor position, using L"%c" format.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
PrintCharAt (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       Character
  );

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
  Free resources allocated for a FormSet.

  @param  FormSet                Pointer of the FormSet

**/
VOID
DestroyFormSet (
  IN OUT FORM_BROWSER_FORMSET  *FormSet
  );

/**
  This function displays the page frame.

  @param  Selection              Selection contains the information about 
                                 the Selection, form and formset to be displayed.
                                 Selection action may be updated in retrieve callback.
**/
VOID
DisplayPageFrame (
  IN UI_MENU_SELECTION    *Selection
  );

/**
  Create a new string in HII Package List.

  @param  String                 The String to be added
  @param  HiiHandle              The package list in the HII database to insert the
                                 specified string.

  @return The output string.

**/
EFI_STRING_ID
NewString (
  IN  CHAR16                   *String,
  IN  EFI_HII_HANDLE           HiiHandle
  );

/**
  Delete a string from HII Package List.

  @param  StringId               Id of the string in HII database.
  @param  HiiHandle              The HII package list handle.

  @retval EFI_SUCCESS            The string was deleted successfully.

**/
EFI_STATUS
DeleteString (
  IN  EFI_STRING_ID            StringId,
  IN  EFI_HII_HANDLE           HiiHandle
  );

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  );

/**
  Draw a pop up windows based on the dimension, number of lines and
  strings specified.

  @param RequestedWidth  The width of the pop-up.
  @param NumberOfLines   The number of lines.
  @param Marker          The variable argument list for the list of string to be printed.

**/
VOID
CreateSharedPopUp (
  IN  UINTN                       RequestedWidth,
  IN  UINTN                       NumberOfLines,
  IN  VA_LIST                     Marker
  );

/**
  Routine used to abstract a generic dialog interface and return the selected key or string

  @param  NumberOfLines          The number of lines for the dialog box
  @param  HotKey                 Defines whether a single character is parsed
                                 (TRUE) and returned in KeyValue or a string is
                                 returned in StringBuffer.  Two special characters
                                 are considered when entering a string, a SCAN_ESC
                                 and an CHAR_CARRIAGE_RETURN.  SCAN_ESC terminates
                                 string input and returns
  @param  MaximumStringSize      The maximum size in bytes of a typed in string
                                 (each character is a CHAR16) and the minimum
                                 string returned is two bytes
  @param  StringBuffer           The passed in pointer to the buffer which will
                                 hold the typed in string if HotKey is FALSE
  @param  KeyValue               The EFI_KEY value returned if HotKey is TRUE..
  @param  ...                    A series of (quantity == NumberOfLines) text
                                 strings which will be used to construct the dialog
                                 box

  @retval EFI_SUCCESS            Displayed dialog and received user interaction
  @retval EFI_INVALID_PARAMETER  One of the parameters was invalid (e.g.
                                 (StringBuffer == NULL) && (HotKey == FALSE))
  @retval EFI_DEVICE_ERROR       User typed in an ESC character to exit the routine

**/
EFI_STATUS
EFIAPI
CreateDialog (
  IN  UINTN                       NumberOfLines,
  IN  BOOLEAN                     HotKey,
  IN  UINTN                       MaximumStringSize,
  OUT CHAR16                      *StringBuffer,
  OUT EFI_INPUT_KEY               *KeyValue,
  ...
  );

/**
  Get Value for given Name from a NameValue Storage.

  @param  Storage                The NameValue Storage.
  @param  Name                   The Name.
  @param  Value                  The retured Value.
  @param  GetValueFrom           Where to get source value, from EditValue or Value.

  @retval EFI_SUCCESS            Value found for given Name.
  @retval EFI_NOT_FOUND          No such Name found in NameValue storage.

**/
EFI_STATUS
GetValueByName (
  IN FORMSET_STORAGE             *Storage,
  IN CHAR16                      *Name,
  IN OUT CHAR16                  **Value,
  IN GET_SET_QUESTION_VALUE_WITH GetValueFrom
  );

/**
  Set Value of given Name in a NameValue Storage.

  @param  Storage                The NameValue Storage.
  @param  Name                   The Name.
  @param  Value                  The Value to set.
  @param  SetValueTo             Whether update editValue or Value.

  @retval EFI_SUCCESS            Value found for given Name.
  @retval EFI_NOT_FOUND          No such Name found in NameValue storage.

**/
EFI_STATUS
SetValueByName (
  IN FORMSET_STORAGE             *Storage,
  IN CHAR16                      *Name,
  IN CHAR16                      *Value,
  IN GET_SET_QUESTION_VALUE_WITH SetValueTo
  );

/**
  Get Question's current Value.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  Question               Question to be initialized.
  @param  GetValueFrom           Where to get value, may from editbuffer, buffer or hii driver.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
GetQuestionValue (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN OUT FORM_BROWSER_STATEMENT       *Question,
  IN GET_SET_QUESTION_VALUE_WITH      GetValueFrom
  );

/**
  Save Question Value to edit copy(cached) or Storage(uncached).

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  Question               Pointer to the Question.
  @param  SetValueTo             Update the question value to editbuffer , buffer or hii driver.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
SetQuestionValue (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN OUT FORM_BROWSER_STATEMENT       *Question,
  IN GET_SET_QUESTION_VALUE_WITH      SetValueTo
  );

/**
  Perform inconsistent check for a Form.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  Question               The Question to be validated.
  @param  Type                   Validation type: InConsistent or NoSubmit

  @retval EFI_SUCCESS            Form validation pass.
  @retval other                  Form validation failed.

**/
EFI_STATUS
ValidateQuestion (
  IN  FORM_BROWSER_FORMSET            *FormSet,
  IN  FORM_BROWSER_FORM               *Form,
  IN  FORM_BROWSER_STATEMENT          *Question,
  IN  UINTN                           Type
  );


/**
  Discard data based on the input setting scope (Form, FormSet or System).

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  SettingScope           Setting Scope for Discard action.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_UNSUPPORTED        Unsupport SettingScope.

**/
EFI_STATUS
DiscardForm (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN BROWSER_SETTING_SCOPE            SettingScope
  );

/**
  Submit data based on the input Setting level (Form, FormSet or System).

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  SettingScope           Setting Scope for Submit action.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_UNSUPPORTED        Unsupport SettingScope.

**/
EFI_STATUS
SubmitForm (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN BROWSER_SETTING_SCOPE            SettingScope
  );

/**
  Reset Question to its default value.

  @param  FormSet                The form set.
  @param  Form                   The form.
  @param  Question               The question.
  @param  DefaultId              The Class of the default.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetQuestionDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN FORM_BROWSER_STATEMENT           *Question,
  IN UINT16                           DefaultId
  );

/**
  Get current setting of Questions.

  @param  FormSet                FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
InitializeCurrentSetting (
  IN OUT FORM_BROWSER_FORMSET             *FormSet
  );

/**
  Initialize the internal data structure of a FormSet.

  @param  Handle                 PackageList Handle
  @param  FormSetGuid            GUID of a formset. If not specified (NULL or zero
                                 GUID), take the first FormSet found in package
                                 list.
  @param  FormSet                FormSet data structure.
  @param  UpdateGlobalVar        Whether need to update the global variable.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The specified FormSet could not be found.

**/
EFI_STATUS
InitializeFormSet (
  IN  EFI_HII_HANDLE                   Handle,
  IN OUT EFI_GUID                      *FormSetGuid,
  OUT FORM_BROWSER_FORMSET             *FormSet,
  IN  BOOLEAN                          UpdateGlobalVar                   
  );

/**
  Reset Questions to their initial value or default value in a Form, Formset or System.

  GetDefaultValueScope parameter decides which questions will reset 
  to its default value.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  DefaultId              The Class of the default.
  @param  SettingScope           Setting Scope for Default action.
  @param  GetDefaultValueScope   Get default value scope.
  @param  Storage                Get default value only for this storage.
  @param  RetrieveValueFirst     Whether call the retrieve call back to
                                 get the initial value before get default
                                 value.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_UNSUPPORTED        Unsupport SettingScope.

**/
EFI_STATUS
ExtractDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN UINT16                           DefaultId,
  IN BROWSER_SETTING_SCOPE            SettingScope,
  IN BROWSER_GET_DEFAULT_VALUE        GetDefaultValueScope,
  IN FORMSET_STORAGE                  *Storage,
  IN BOOLEAN                          RetrieveValueFirst
  );

/**
  Initialize Question's Edit copy from Storage.

  @param  Selection              Selection contains the information about 
                                 the Selection, form and formset to be displayed.
                                 Selection action may be updated in retrieve callback.
                                 If Selection is NULL, only initialize Question value.
  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
LoadFormConfig (
  IN OUT UI_MENU_SELECTION    *Selection,
  IN FORM_BROWSER_FORMSET     *FormSet,
  IN FORM_BROWSER_FORM        *Form
  );

/**
  Initialize Question's Edit copy from Storage for the whole Formset.

  @param  Selection              Selection contains the information about 
                                 the Selection, form and formset to be displayed.
                                 Selection action may be updated in retrieve callback.
                                 If Selection is NULL, only initialize Question value.
  @param  FormSet                FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
LoadFormSetConfig (
  IN OUT UI_MENU_SELECTION    *Selection,
  IN     FORM_BROWSER_FORMSET *FormSet
  );

/**
  Convert setting of Buffer Storage or NameValue Storage to <ConfigResp>.

  @param  Buffer                 The Storage to be conveted.
  @param  ConfigResp             The returned <ConfigResp>.
  @param  SingleForm             Whether update data for single form or formset level.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
StorageToConfigResp (
  IN VOID                    *Buffer,
  IN CHAR16                  **ConfigResp,
  IN BOOLEAN                 SingleForm
  );

/**
  Convert <ConfigResp> to settings in Buffer Storage or NameValue Storage.

  @param  Storage                The Storage to receive the settings.
  @param  ConfigResp             The <ConfigResp> to be converted.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
ConfigRespToStorage (
  IN FORMSET_STORAGE         *Storage,
  IN CHAR16                  *ConfigResp
  );

/**
  Fill storage's edit copy with settings requested from Configuration Driver.

  @param  FormSet                FormSet data structure.
  @param  Storage                Buffer Storage.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
LoadStorage (
  IN FORM_BROWSER_FORMSET    *FormSet,
  IN FORMSET_STORAGE         *Storage
  );

/**
  Fetch the Ifr binary data of a FormSet.

  @param  Handle                 PackageList Handle
  @param  FormSetGuid            GUID of a formset. If not specified (NULL or zero
                                 GUID), take the first FormSet found in package
                                 list.
  @param  BinaryLength           The length of the FormSet IFR binary.
  @param  BinaryData             The buffer designed to receive the FormSet.

  @retval EFI_SUCCESS            Buffer filled with the requested FormSet.
                                 BufferLength was updated.
  @retval EFI_INVALID_PARAMETER  The handle is unknown.
  @retval EFI_NOT_FOUND          A form or FormSet on the requested handle cannot
                                 be found with the requested FormId.

**/
EFI_STATUS
GetIfrBinaryData (
  IN  EFI_HII_HANDLE   Handle,
  IN OUT EFI_GUID      *FormSetGuid,
  OUT UINTN            *BinaryLength,
  OUT UINT8            **BinaryData
  );

/**
  Save globals used by previous call to SendForm(). SendForm() may be called from 
  HiiConfigAccess.Callback(), this will cause SendForm() be reentried.
  So, save globals of previous call to SendForm() and restore them upon exit.

**/
VOID
SaveBrowserContext (
  VOID
  );

/**
  Restore globals used by previous call to SendForm().

**/
VOID
RestoreBrowserContext (
  VOID
  );

/**
  This is the routine which an external caller uses to direct the browser
  where to obtain it's information.


  @param This            The Form Browser protocol instanse.
  @param Handles         A pointer to an array of Handles.  If HandleCount > 1 we
                         display a list of the formsets for the handles specified.
  @param HandleCount     The number of Handles specified in Handle.
  @param FormSetGuid     This field points to the EFI_GUID which must match the Guid
                         field in the EFI_IFR_FORM_SET op-code for the specified
                         forms-based package. If FormSetGuid is NULL, then this
                         function will display the first found forms package.
  @param FormId          This field specifies which EFI_IFR_FORM to render as the first
                         displayable page. If this field has a value of 0x0000, then
                         the forms browser will render the specified forms in their encoded order.
                         ScreenDimenions - This allows the browser to be called so that it occupies a
                         portion of the physical screen instead of dynamically determining the screen dimensions.
                         ActionRequest   - Points to the action recommended by the form.
  @param ScreenDimensions Points to recommended form dimensions, including any non-content area, in
                          characters.
  @param ActionRequest       Points to the action recommended by the form.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval  EFI_NOT_FOUND          No valid forms could be found to display.

**/
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
  );

/**
  This function is called by a callback handler to retrieve uncommitted state
  data from the browser.

  @param  This                   A pointer to the EFI_FORM_BROWSER2_PROTOCOL
                                 instance.
  @param  ResultsDataSize        A pointer to the size of the buffer associated
                                 with ResultsData.
  @param  ResultsData            A string returned from an IFR browser or
                                 equivalent. The results string will have no
                                 routing information in them.
  @param  RetrieveData           A BOOLEAN field which allows an agent to retrieve
                                 (if RetrieveData = TRUE) data from the uncommitted
                                 browser state information or set (if RetrieveData
                                 = FALSE) data in the uncommitted browser state
                                 information.
  @param  VariableGuid           An optional field to indicate the target variable
                                 GUID name to use.
  @param  VariableName           An optional field to indicate the target
                                 human-readable variable name.

  @retval EFI_SUCCESS            The results have been distributed or are awaiting
                                 distribution.
  @retval EFI_BUFFER_TOO_SMALL   The ResultsDataSize specified was too small to
                                 contain the results data.

**/
EFI_STATUS
EFIAPI
BrowserCallback (
  IN CONST EFI_FORM_BROWSER2_PROTOCOL  *This,
  IN OUT UINTN                         *ResultsDataSize,
  IN OUT EFI_STRING                    ResultsData,
  IN BOOLEAN                           RetrieveData,
  IN CONST EFI_GUID                    *VariableGuid, OPTIONAL
  IN CONST CHAR16                      *VariableName  OPTIONAL
  );

/**
  Find menu which will show next time.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.
  @param Repaint         Whether need to repaint the menu.
  @param NewLine         Whether need to show at new line.
  
  @retval TRUE           Need return.
  @retval FALSE          No need to return.
**/
BOOLEAN
FindNextMenu (
  IN OUT UI_MENU_SELECTION    *Selection,
  IN     BOOLEAN              *Repaint, 
  IN     BOOLEAN              *NewLine  
  );

/**
  check whether the formset need to update the NV.

  @param  FormSet                FormSet data structure.
  @param  SetValue               Whether set new value or clear old value.

**/
VOID
UpdateNvInfoInForm (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN BOOLEAN               SetValue
  );

/**
  check whether the formset need to update the NV.

  @param  FormSet                FormSet data structure.

  @retval TRUE                   Need to update the NV.
  @retval FALSE                  No need to update the NV.
**/
BOOLEAN 
IsNvUpdateRequired (
  IN FORM_BROWSER_FORMSET  *FormSet
  );

/**
  Call the call back function for the question and process the return action.

  @param Selection             On input, Selection tell setup browser the information
                               about the Selection, form and formset to be displayed.
                               On output, Selection return the screen item that is selected
                               by user.
  @param Statement             The Question which need to call.
  @param Action                The action request.
  @param SkipSaveOrDiscard     Whether skip save or discard action.

  @retval EFI_SUCCESS          The call back function excutes successfully.
  @return Other value if the call back function failed to excute.  
**/
EFI_STATUS 
ProcessCallBackFunction (
  IN OUT UI_MENU_SELECTION               *Selection,
  IN     FORM_BROWSER_STATEMENT          *Question,
  IN     EFI_BROWSER_ACTION              Action,
  IN     BOOLEAN                         SkipSaveOrDiscard
  );
  
/**
  Call the retrieve type call back function for one question to get the initialize data.
  
  This function only used when in the initialize stage, because in this stage, the 
  Selection->Form is not ready. For other case, use the ProcessCallBackFunction instead.

  @param ConfigAccess          The config access protocol produced by the hii driver.
  @param Statement             The Question which need to call.

  @retval EFI_SUCCESS          The call back function excutes successfully.
  @return Other value if the call back function failed to excute.  
**/
EFI_STATUS 
ProcessRetrieveForQuestion (
  IN     EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess,
  IN     FORM_BROWSER_STATEMENT          *Statement
  );

/**
  Find the matched FormSet context in the backup maintain list based on HiiHandle.
  
  @param Handle  The Hii Handle.
  
  @return the found FormSet context. If no found, NULL will return.

**/
FORM_BROWSER_FORMSET * 
GetFormSetFromHiiHandle (
  EFI_HII_HANDLE Handle
  );

/**
  Check whether the input HII handle is the FormSet that is being used.
  
  @param Handle  The Hii Handle.
  
  @retval TRUE   HII handle is being used.
  @retval FALSE  HII handle is not being used.

**/
BOOLEAN
IsHiiHandleInBrowserContext (
  EFI_HII_HANDLE Handle
  );

/**
  Configure what scope the hot key will impact.
  All hot keys have the same scope. The mixed hot keys with the different level are not supported.
  If no scope is set, the default scope will be FormSet level.
  After all registered hot keys are removed, previous Scope can reset to another level.
  
  @param[in] Scope               Scope level to be set. 
  
  @retval EFI_SUCCESS            Scope is set correctly.
  @retval EFI_INVALID_PARAMETER  Scope is not the valid value specified in BROWSER_SETTING_SCOPE. 
  @retval EFI_UNSPPORTED         Scope level is different from current one that the registered hot keys have.

**/
EFI_STATUS
EFIAPI
SetScope (
  IN BROWSER_SETTING_SCOPE Scope
  );

/**
  Register the hot key with its browser action, or unregistered the hot key.
  Only support hot key that is not printable character (control key, function key, etc.).
  If the action value is zero, the hot key will be unregistered if it has been registered.
  If the same hot key has been registered, the new action and help string will override the previous ones.
  
  @param[in] KeyData     A pointer to a buffer that describes the keystroke
                         information for the hot key. Its type is EFI_INPUT_KEY to 
                         be supported by all ConsoleIn devices.
  @param[in] Action      Action value that describes what action will be trigged when the hot key is pressed. 
  @param[in] DefaultId   Specifies the type of defaults to retrieve, which is only for DEFAULT action.
  @param[in] HelpString  Help string that describes the hot key information.
                         Its value may be NULL for the unregistered hot key.
  
  @retval EFI_SUCCESS            Hot key is registered or unregistered.
  @retval EFI_INVALID_PARAMETER  KeyData is NULL.
  @retval EFI_NOT_FOUND          KeyData is not found to be unregistered.
  @retval EFI_UNSUPPORTED        Key represents a printable character. It is conflicted with Browser.
**/
EFI_STATUS
EFIAPI
RegisterHotKey (
  IN EFI_INPUT_KEY *KeyData,
  IN UINT32        Action,
  IN UINT16        DefaultId,
  IN EFI_STRING    HelpString OPTIONAL
  );

/**
  Register Exit handler function. 
  When more than one handler function is registered, the latter one will override the previous one. 
  When NULL handler is specified, the previous Exit handler will be unregistered. 
  
  @param[in] Handler      Pointer to handler function. 

**/
VOID
EFIAPI
RegiserExitHandler (
  IN EXIT_HANDLER Handler
  );

/**
  Create reminder to let user to choose save or discard the changed browser data.
  Caller can use it to actively check the changed browser data.

  @retval BROWSER_NO_CHANGES       No browser data is changed.
  @retval BROWSER_SAVE_CHANGES     The changed browser data is saved.
  @retval BROWSER_DISCARD_CHANGES  The changed browser data is discard.

**/
UINT32
EFIAPI
SaveReminder (
  VOID
  );

/**
  Find the registered HotKey based on KeyData.
  
  @param[in] KeyData     A pointer to a buffer that describes the keystroke
                         information for the hot key.

  @return The registered HotKey context. If no found, NULL will return.
**/
BROWSER_HOT_KEY *
GetHotKeyFromRegisterList (
  IN EFI_INPUT_KEY *KeyData
  );

#endif
