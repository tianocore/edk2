/** @file
  FormDiplay protocol to show Form

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DISPLAY_PROTOCOL_H__
#define __DISPLAY_PROTOCOL_H__

#include <Protocol/FormBrowser2.h>

#define EDKII_FORM_DISPLAY_ENGINE_PROTOCOL_GUID  \
  { 0x9bbe29e9, 0xfda1, 0x41ec, { 0xad, 0x52, 0x45, 0x22, 0x13, 0x74, 0x2d, 0x2e } }

//
// Do nothing.
//
#define BROWSER_ACTION_NONE         BIT16
//
// ESC Exit
//
#define BROWSER_ACTION_FORM_EXIT    BIT17

#define BROWSER_SUCCESS                   0x0
#define BROWSER_ERROR                     BIT31
#define BROWSER_SUBMIT_FAIL               BROWSER_ERROR | 0x01
#define BROWSER_NO_SUBMIT_IF              BROWSER_ERROR | 0x02
#define BROWSER_FORM_NOT_FOUND            BROWSER_ERROR | 0x03
#define BROWSER_FORM_SUPPRESS             BROWSER_ERROR | 0x04
#define BROWSER_PROTOCOL_NOT_FOUND        BROWSER_ERROR | 0x05
#define BROWSER_INCONSISTENT_IF           BROWSER_ERROR | 0x06
#define BROWSER_WARNING_IF                BROWSER_ERROR | 0x07
#define BROWSER_SUBMIT_FAIL_NO_SUBMIT_IF  BROWSER_ERROR | 0x08
#define BROWSER_RECONNECT_REQUIRED        BROWSER_ERROR | 0x09
#define BROWSER_RECONNECT_FAIL            BROWSER_ERROR | 0x0A
#define BROWSER_RECONNECT_SAVE_CHANGES    BROWSER_ERROR | 0x0B

#define FORM_DISPLAY_ENGINE_STATEMENT_VERSION_1  0x10000
#define FORM_DISPLAY_ENGINE_VERSION_1            0x10000

typedef struct {
  //
  // HII Data Type
  //
  UINT8               Type;
  //
  // Buffer Data and Length if Type is EFI_IFR_TYPE_BUFFER or EFI_IFR_TYPE_STRING
  //
  UINT8               *Buffer;
  UINT16              BufferLen;
  EFI_IFR_TYPE_VALUE  Value;
} EFI_HII_VALUE;

#define DISPLAY_QUESTION_OPTION_SIGNATURE  SIGNATURE_32 ('Q', 'O', 'P', 'T')

typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;
  //
  // OneOfOption Data
  //
  EFI_IFR_ONE_OF_OPTION  *OptionOpCode;
  //
  // Option ImageId and AnimationId
  //
  EFI_IMAGE_ID           ImageId;
  EFI_ANIMATION_ID       AnimationId;
} DISPLAY_QUESTION_OPTION;

#define DISPLAY_QUESTION_OPTION_FROM_LINK(a)  CR (a, DISPLAY_QUESTION_OPTION, Link, DISPLAY_QUESTION_OPTION_SIGNATURE)

typedef struct _FORM_DISPLAY_ENGINE_STATEMENT FORM_DISPLAY_ENGINE_STATEMENT;
typedef struct _FORM_DISPLAY_ENGINE_FORM      FORM_DISPLAY_ENGINE_FORM;

#define STATEMENT_VALID             0x0
#define STATEMENT_INVALID           BIT31

#define INCOSISTENT_IF_TRUE         STATEMENT_INVALID | 0x01
#define WARNING_IF_TRUE             STATEMENT_INVALID | 0x02
#define STRING_TOO_LONG             STATEMENT_INVALID | 0x03
// ... to be extended.

typedef struct {
  //
  // StringId for INCONSITENT_IF or WARNING_IF
  //
  EFI_STRING_ID  StringId;
  //
  // TimeOut for WARNING_IF
  //
  UINT8          TimeOut;
} STATEMENT_ERROR_INFO;

/**
  Perform value check for a question.

  @param  Form       Form where Statement is in.
  @param  Statement  Value will check for it.
  @param  Value      New value will be checked.

  @retval Status     Value Status

**/
typedef
UINT32
(EFIAPI *VALIDATE_QUESTION) (
  IN FORM_DISPLAY_ENGINE_FORM      *Form,
  IN FORM_DISPLAY_ENGINE_STATEMENT *Statement,
  IN EFI_HII_VALUE                 *Value,
  OUT STATEMENT_ERROR_INFO         *ErrorInfo
  );

/**
  Perform Password check.
  Passwork may be encrypted by driver that requires the specific check.

  @param  Form             Form where Password Statement is in.
  @param  Statement        Password statement
  @param  PasswordString   Password string to be checked. It may be NULL.
                           NULL means to restore password.
                           "" string can be used to checked whether old password does exist.

  @return Status     Status of Password check.
**/
typedef
EFI_STATUS
(EFIAPI *PASSWORD_CHECK) (
  IN FORM_DISPLAY_ENGINE_FORM      *Form,
  IN FORM_DISPLAY_ENGINE_STATEMENT *Statement,
  IN EFI_STRING                    PasswordString  OPTIONAL
  );

#define FORM_DISPLAY_ENGINE_STATEMENT_SIGNATURE  SIGNATURE_32 ('F', 'S', 'T', 'A')

//
// Attribute for Statement and Form
//
#define HII_DISPLAY_NONE             0
#define HII_DISPLAY_GRAYOUT          BIT0
#define HII_DISPLAY_LOCK             BIT1
#define HII_DISPLAY_READONLY         BIT2
#define HII_DISPLAY_MODAL            BIT3
#define HII_DISPLAY_SUPPRESS         BIT4

struct _FORM_DISPLAY_ENGINE_STATEMENT{
  UINTN                 Signature;
  //
  // Version for future structure extension
  //
  UINTN                 Version;
  //
  // link to all the statement which will show in the display form.
  //
  LIST_ENTRY            DisplayLink;
  //
  // Pointer to statement opcode.
  // for Guided Opcode. All buffers will be here if GUIDED opcode scope is set.
  //
  EFI_IFR_OP_HEADER     *OpCode;
  //
  // Question CurrentValue
  //
  EFI_HII_VALUE         CurrentValue;
  //
  // Flag to describe whether setting is changed or not.
  // Displayer may depend on it to show it with the different color.
  //
  BOOLEAN               SettingChangedFlag;
  //
  // nested Statement list inside of EFI_IFR_SUBTITLE
  //
  LIST_ENTRY            NestStatementList;
  //
  // nested EFI_IFR_ONE_OF_OPTION list (QUESTION_OPTION)
  //
  LIST_ENTRY            OptionListHead;
  //
  // Statement attributes: GRAYOUT, LOCK and READONLY
  //
  UINT32                Attribute;

  //
  // ValidateQuestion to do InconsistIf check
  // It may be NULL if any value is valid.
  //
  VALIDATE_QUESTION     ValidateQuestion;

  //
  // Password additional check. It may be NULL when the additional check is not required.
  //
  PASSWORD_CHECK        PasswordCheck;

  //
  // Statement ImageId and AnimationId
  //
  EFI_IMAGE_ID          ImageId;
  EFI_ANIMATION_ID      AnimationId;
};

#define FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK(a)  CR (a, FORM_DISPLAY_ENGINE_STATEMENT, DisplayLink, FORM_DISPLAY_ENGINE_STATEMENT_SIGNATURE)

#define BROWSER_HOT_KEY_SIGNATURE  SIGNATURE_32 ('B', 'H', 'K', 'S')

typedef struct {
  UINTN                 Signature;
  LIST_ENTRY            Link;

  EFI_INPUT_KEY         *KeyData;
  //
  // Action is Discard, Default, Submit, Reset and Exit.
  //
  UINT32                 Action;
  UINT16                 DefaultId;
  //
  // HotKey Help String
  //
  EFI_STRING             HelpString;
} BROWSER_HOT_KEY;

#define BROWSER_HOT_KEY_FROM_LINK(a)  CR (a, BROWSER_HOT_KEY, Link, BROWSER_HOT_KEY_SIGNATURE)

#define FORM_DISPLAY_ENGINE_FORM_SIGNATURE  SIGNATURE_32 ('F', 'F', 'R', 'M')

struct _FORM_DISPLAY_ENGINE_FORM {
  UINTN                Signature;
  //
  // Version for future structure extension
  //
  UINTN                Version;
  //
  // Statement List inside of Form
  //
  LIST_ENTRY            StatementListHead;
  //
  // Statement List outside of Form
  //
  LIST_ENTRY            StatementListOSF;
  //
  // The input screen dimenstions info.
  //
  EFI_SCREEN_DESCRIPTOR *ScreenDimensions;
  //
  // FormSet information
  //
  EFI_GUID             FormSetGuid;
  //
  // HiiHandle can be used to get String, Image or Animation
  //
  EFI_HII_HANDLE       HiiHandle;

  //
  // Form ID and Title.
  //
  UINT16               FormId;
  EFI_STRING_ID        FormTitle;
  //
  // Form Attributes: Lock, Modal.
  //
  UINT32               Attribute;
  //
  // Flag to describe whether setting is changed or not.
  // Displayer depends on it to show ChangedFlag.
  //
  BOOLEAN              SettingChangedFlag;

  //
  // Statement to be HighLighted
  //
  FORM_DISPLAY_ENGINE_STATEMENT *HighLightedStatement;
  //
  // Event to notify Displayer that FormData is updated to be refreshed.
  //
  EFI_EVENT              FormRefreshEvent;
  //
  // Additional Hotkey registered by BrowserEx protocol.
  //
  LIST_ENTRY             HotKeyListHead;

  //
  // Form ImageId and AnimationId
  //
  EFI_IMAGE_ID         ImageId;
  EFI_ANIMATION_ID     AnimationId;

  //
  // If Status is error, display needs to handle it.
  //
  UINT32               BrowserStatus;
  //
  // String for error status. It may be NULL.
  //
  EFI_STRING           ErrorString;
};

#define FORM_DISPLAY_ENGINE_FORM_FROM_LINK(a)  CR (a, FORM_DISPLAY_ENGINE_FORM, Link, FORM_DISPLAY_ENGINE_FORM_SIGNATURE)

typedef struct {
  FORM_DISPLAY_ENGINE_STATEMENT  *SelectedStatement; // Selected Statement and InputValue

  EFI_HII_VALUE                  InputValue;

  UINT32                         Action;             // If SelectedStatement is NULL, Action will be used.
                                                     // Trig Action (Discard, Default, Submit, Reset and Exit)
  UINT16                         DefaultId;
} USER_INPUT;

/**
  Display one form, and return user input.

  @param FormData                Form Data to be shown.
  @param UserInputData           User input data.

  @retval EFI_SUCCESS            Form Data is shown, and user input is got.
**/
typedef
EFI_STATUS
(EFIAPI *FORM_DISPLAY) (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT               *UserInputData
);

/**
  Exit Display and Clear Screen to the original state.

**/
typedef
VOID
(EFIAPI *EXIT_DISPLAY) (
  VOID
);

/**
  Confirm how to handle the changed data.

  @return Action of Submit, Discard and None
**/
typedef
UINTN
(EFIAPI *CONFIRM_DATA_CHANGE) (
  VOID
);

typedef struct {
  FORM_DISPLAY        FormDisplay;
  EXIT_DISPLAY        ExitDisplay;
  CONFIRM_DATA_CHANGE ConfirmDataChange;
} EDKII_FORM_DISPLAY_ENGINE_PROTOCOL;

extern EFI_GUID gEdkiiFormDisplayEngineProtocolGuid;
#endif
