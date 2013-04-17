/** @file
  FormDiplay protocol to show Form

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DISPLAY_PROTOCOL_H__
#define __DISPLAY_PROTOCOL_H__

#define FORM_DISPLAY_ENGINE_PROTOCOL_GUID  \
  { 0x9bbe29e9, 0xfda1, 0x41ec, { 0xad, 0x52, 0x45, 0x22, 0x13, 0x74, 0x2d, 0x2e } }

#define FORM_DISPLAY_ENGINE_STATEMENT_VERSION  0x10000
#define FORM_DISPLAY_ENGINE_VERSION       0x10000

typedef struct _FORM_DISPLAY_ENGINE_PROTOCOL   FORM_DISPLAY_ENGINE_PROTOCOL;

typedef struct {
  UINT8               Type;       // HII Data Type
  UINT8               *Buffer;    // Buffer Data and Length if Type is EFI_IFR_TYPE_BUFFER or EFI_IFR_TYPE_STRING
  UINT16              BufferLen;
  EFI_IFR_TYPE_VALUE  Value;
} EFI_HII_VALUE;

#define DISPLAY_QUESTION_OPTION_SIGNATURE  SIGNATURE_32 ('Q', 'O', 'P', 'T')

typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;
  EFI_IFR_ONE_OF_OPTION  OptionOpCode;   // OneOfOption Data
  EFI_IMAGE_ID           ImageId;        // Option ImageId and AnimationId
  EFI_ANIMATION_ID       AnimationId;
} DISPLAY_QUESTION_OPTION;

#define DISPLAY_QUESTION_OPTION_FROM_LINK(a)  CR (a, DISPLAY_QUESTION_OPTION, Link, DISPLAY_QUESTION_OPTION_SIGNATURE)


typedef struct _FORM_DISPLAY_ENGINE_STATEMENT FORM_DISPLAY_ENGINE_STATEMENT;

//
// Attribute for Statement and Form
//
#define HII_DISPLAY_GRAYOUT          BIT0
#define HII_DISPLAY_LOCK             BIT1
#define HII_DISPLAY_READONLY         BIT2
#define HII_DISPLAY_MODAL            BIT3

#define FORM_DISPLAY_ENGINE_FORM_SIGNATURE  SIGNATURE_32 ('F', 'F', 'R', 'M')

typedef struct {
  UINTN                         Signature;
  UINTN                         Version;                    // Version for future structure extension
  LIST_ENTRY                    StatementListHead;          // Statement List inside of Form
  EFI_GUID                      FormSetGuid;                // FormSet information
  EFI_HII_HANDLE                HiiHandle;                  // HiiHandle can be used to get String, Image or Animation
  UINT16                        FormId;                     // Form ID and Title.
  EFI_STRING_ID                 FormTitle;
  UINT32                        Attribute;                  // Form Attributes: Lock, Modal.
  BOOLEAN                       SettingChangedFlag;         // Flag to describe whether setting is changed or not.
  FORM_DISPLAY_ENGINE_STATEMENT *HighLightedStatement;      // Statement to be HighLighted
  EFI_GUID                      *FormRefreshEventGuid;      // EventGuid to notify Displayer that FormData is updated to be refreshed.
  LIST_ENTRY                    HotKeyListHead;             // Additional Hotkey registered by BrowserEx protocol.
  EFI_IMAGE_ID                  ImageId;                    // Form ImageId and AnimationId
  EFI_ANIMATION_ID              AnimationId;
} FORM_DISPLAY_ENGINE_FORM;

#define FORM_DISPLAY_ENGINE_FORM_FROM_LINK(a)  CR (a, FORM_DISPLAY_ENGINE_FORM, Link, FORM_DISPLAY_ENGINE_FORM_SIGNATURE)

/**
  Perform value check for a question.
  
  @param  Form       Form where Statement is in.
  @param  Statement  Value will check for it.
  @param  Value      New value will be checked.
  
  @retval TRUE   Input Value is valid.
  @retval FALSE  Input Value is invalid.
**/
typedef
BOOLEAN
(EFIAPI *VALIDATE_QUESTION) (
  IN FORM_DISPLAY_ENGINE_FORM      *Form,
  IN FORM_DISPLAY_ENGINE_STATEMENT *Statement,
  IN EFI_HII_VALUE                 *Value
  );

/**
  Perform Password check. 
  Passwork may be encrypted by driver that requires the specific check.
  
  @param  Form             Form where Password Statement is in.
  @param  Statement        Password statement
  @param  PasswordString   Password string to be checked. It may be NULL.
  
  @return Status     Status of Password check.
**/
typedef
EFI_STATUS
(EFIAPI *PASSWORD_CHECK) (
  IN FORM_DISPLAY_ENGINE_FORM      *Form,
  IN FORM_DISPLAY_ENGINE_STATEMENT *Statement, 
  IN EFI_STRING                    *PasswordString  OPTIONAL
  );

#define FORM_DISPLAY_ENGINE_STATEMENT_SIGNATURE  SIGNATURE_32 ('F', 'S', 'T', 'A')

struct _FORM_DISPLAY_ENGINE_STATEMENT{
  UINTN                 Signature;
  UINTN                 Version;              // Version for future structure extension
  LIST_ENTRY            DisplayLink;          // link to all the statement which will show in the display form.
  EFI_IFR_OP_HEADER     *OpCode;              // Pointer to statement opcode.
                                              // for Guided Opcode. All buffers will be here if GUIDED opcode scope is set.
  EFI_HII_VALUE         CurrentValue;         // Question CurrentValue
  BOOLEAN               SettingChangedFlag;   // Flag to describe whether setting is changed or not.
                                              // Displayer may depend on it to show it with the different color.
  LIST_ENTRY            NestStatementList;    // nested Statement list inside of EFI_IFR_SUBTITLE
  LIST_ENTRY            OptionListHead;       // nested EFI_IFR_ONE_OF_OPTION list (QUESTION_OPTION)
  UINT32                Attribute;            // Statement attributes: GRAYOUT, LOCK and READONLY
  VALIDATE_QUESTION     ValidateQuestion;     // ValidateQuestion to do InconsistIf check
  EFI_STRING_ID         InConsistentStringId; // InConsistentString popup will be used when ValidateQuestion returns FASLE.
                                              // If this ID is zero, then Display can customize error message for the invalid value.
  PASSWORD_CHECK        PasswordCheck;        // Password additional check. It may be NULL when the additional check is not required.
  EFI_IMAGE_ID          ImageId;              // Statement ImageId and AnimationId
  EFI_ANIMATION_ID      AnimationId;
};

#define FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK(a)  CR (a, FORM_DISPLAY_ENGINE_STATEMENT, DisplayLink, FORM_DISPLAY_ENGINE_STATEMENT_SIGNATURE)

typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;
  EFI_INPUT_KEY          KeyData;
  UINT32                 Action;        // Action is Discard, Default, Submit, Reset and Exit.
  UINT16                 DefaultId;
  EFI_STRING             HelpString;    // HotKey Help String
} HOTKEY_INFO;

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

struct _FORM_DISPLAY_ENGINE_PROTOCOL {
  FORM_DISPLAY        FormDisplay;
  EXIT_DISPLAY        ExitDisplay;
  CONFIRM_DATA_CHANGE ConfirmDataChange;
};

extern EFI_GUID gEfiFormDisplayEngineProtocolGuid;
#endif
