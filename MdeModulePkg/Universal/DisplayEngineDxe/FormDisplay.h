/** @file
  FormDiplay protocol to show Form

Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FORM_DISPLAY_H__
#define __FORM_DISPLAY_H__


#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/CustomizedDisplayLib.h>

#include <Protocol/FormBrowserEx2.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/DisplayProtocol.h>

#include <Guid/MdeModuleHii.h>

//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//
extern UINT8  DisplayEngineStrings[];
extern EFI_SCREEN_DESCRIPTOR         gStatementDimensions;
extern USER_INPUT                    *gUserInput;
extern FORM_DISPLAY_ENGINE_FORM      *gFormData;
extern EFI_HII_HANDLE                gHiiHandle;
extern UINT16                        gDirection;
extern LIST_ENTRY                    gMenuOption;

//
// Browser Global Strings
//
extern CHAR16            *gSaveFailed;
extern CHAR16            *gPromptForData;
extern CHAR16            *gPromptForPassword;
extern CHAR16            *gPromptForNewPassword;
extern CHAR16            *gConfirmPassword;
extern CHAR16            *gConfirmError;
extern CHAR16            *gPassowordInvalid;
extern CHAR16            *gPressEnter;
extern CHAR16            *gEmptyString;
extern CHAR16            *gMiniString;
extern CHAR16            *gOptionMismatch;
extern CHAR16            *gFormSuppress;
extern CHAR16            *gProtocolNotFound;

extern CHAR16            gPromptBlockWidth;
extern CHAR16            gOptionBlockWidth;
extern CHAR16            gHelpBlockWidth;
extern CHAR16            *mUnknownString;
extern BOOLEAN           gMisMatch;

//
// Screen definitions
//

#define LEFT_SKIPPED_COLUMNS          3
#define SCROLL_ARROW_HEIGHT           1
#define POPUP_PAD_SPACE_COUNT         5
#define POPUP_FRAME_WIDTH             2

#define UPPER_LOWER_CASE_OFFSET       0x20

//
// Display definitions
//
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

#define SUBTITLE_INDENT  2

//
// This is the Input Error Message
//
#define INPUT_ERROR 1

//
// This is the NV RAM update required Message
//
#define NV_UPDATE_REQUIRED  2
//
// Time definitions
//
#define ONE_SECOND  10000000

//
// It take 23 characters including the NULL to print a 64 bits number with "[" and "]".
// pow(2, 64) = [18446744073709551616]
// with extra '-' flat, set the width to 24.
//
#define MAX_NUMERIC_INPUT_WIDTH 24

#define EFI_HII_EXPRESSION_INCONSISTENT_IF   0
#define EFI_HII_EXPRESSION_NO_SUBMIT_IF      1
#define EFI_HII_EXPRESSION_GRAY_OUT_IF       2
#define EFI_HII_EXPRESSION_SUPPRESS_IF       3
#define EFI_HII_EXPRESSION_DISABLE_IF        4

//
// Character definitions
//
#define CHAR_SPACE              0x0020

#define FORM_DISPLAY_DRIVER_SIGNATURE SIGNATURE_32 ('F', 'D', 'D', 'V')
typedef struct {
  UINT32                             Signature;

  EFI_HANDLE                         Handle;

  //
  // Produced protocol
  //
  EDKII_FORM_DISPLAY_ENGINE_PROTOCOL FromDisplayProt;
} FORM_DISPLAY_DRIVER_PRIVATE_DATA;


typedef enum {
  UiNoOperation,
  UiSelect,
  UiUp,
  UiDown,
  UiLeft,
  UiRight,
  UiReset,
  UiPrevious,
  UiPageUp,
  UiPageDown,
  UiHotKey,
  UiMaxOperation
} UI_SCREEN_OPERATION;

typedef enum {
  CfInitialization,
  CfCheckSelection,
  CfRepaint,
  CfRefreshHighLight,
  CfUpdateHelpString,
  CfPrepareToReadKey,
  CfReadKey,
  CfScreenOperation,
  CfUiSelect,
  CfUiReset,
  CfUiLeft,
  CfUiRight,
  CfUiUp,
  CfUiPageUp,
  CfUiPageDown,
  CfUiDown,
  CfUiNoOperation,
  CfExit,
  CfUiHotKey,
  CfMaxControlFlag
} UI_CONTROL_FLAG;

typedef enum {
  UIEventNone,
  UIEventKey,
  UIEventTimeOut,
  UIEventDriver
} UI_EVENT_TYPE;

typedef struct {
  UINT16              ScanCode;
  UI_SCREEN_OPERATION ScreenOperation;
} SCAN_CODE_TO_SCREEN_OPERATION;

typedef struct {
  UI_SCREEN_OPERATION ScreenOperation;
  UI_CONTROL_FLAG     ControlFlag;
} SCREEN_OPERATION_T0_CONTROL_FLAG;

typedef struct {
  EFI_HII_HANDLE     HiiHandle;
  UINT16             FormId;
  
  //
  // Info for the highlight question.
  // HLT means highlight
  //
  // If one statement has questionid, save questionid info to find the question.
  // If one statement not has questionid info, save the opcode info to find the 
  // statement. If more than one statement has same opcode in one form(just like
  // empty subtitle info may has more than one info one form), also use Index 
  // info to find the statement.
  //
  EFI_QUESTION_ID    HLTQuestionId;
  EFI_IFR_OP_HEADER  *HLTOpCode;
  UINTN              HLTIndex;
  UINTN              HLTSequence;
  
  //
  // Info for the top of screen question.
  // TOS means Top Of Screen
  //
  EFI_QUESTION_ID    TOSQuestionId;
  EFI_IFR_OP_HEADER  *TOSOpCode;
  UINTN              TOSIndex;

  UINT16             SkipValue;
} DISPLAY_HIGHLIGHT_MENU_INFO;

typedef struct {
  EFI_EVENT   SyncEvent;
  UINT8       *TimeOut;
  CHAR16      *ErrorInfo;
} WARNING_IF_CONTEXT;

#define UI_MENU_OPTION_SIGNATURE  SIGNATURE_32 ('u', 'i', 'm', 'm')

typedef struct {
  UINTN                   Signature;
  LIST_ENTRY              Link;

  EFI_HII_HANDLE          Handle;
  FORM_DISPLAY_ENGINE_STATEMENT  *ThisTag;
  UINT16                  EntryNumber;

  UINTN                   Row;
  UINTN                   Col;
  UINTN                   OptCol;
  CHAR16                  *Description;
  UINTN                   Skip;           // Number of lines

  //
  // Display item sequence for date/time
  //  Date:      Month/Day/Year
  //  Sequence:  0     1   2
  //
  //  Time:      Hour : Minute : Second
  //  Sequence:  0      1        2
  //
  //
  UINTN                   Sequence;

  BOOLEAN                 GrayOut;
  BOOLEAN                 ReadOnly;

  //
  // Whether user could change value of this item
  //
  BOOLEAN                 IsQuestion;
  BOOLEAN                 NestInStatement;
} UI_MENU_OPTION;

#define MENU_OPTION_FROM_LINK(a)  CR (a, UI_MENU_OPTION, Link, UI_MENU_OPTION_SIGNATURE)

/**
  Print Question Value according to it's storage width and display attributes.

  @param  Question               The Question to be printed.
  @param  FormattedNumber        Buffer for output string.
  @param  BufferSize             The FormattedNumber buffer size in bytes.

  @retval EFI_SUCCESS            Print success.
  @retval EFI_BUFFER_TOO_SMALL   Buffer size is not enough for formatted number.

**/
EFI_STATUS
PrintFormattedNumber (
  IN FORM_DISPLAY_ENGINE_STATEMENT   *Question,
  IN OUT CHAR16               *FormattedNumber,
  IN UINTN                    BufferSize
  );

/**
  Set value of a data element in an Array by its Index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.
  @param  Value                  The value to be set.

**/
VOID
SetArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINTN                    Index,
  IN UINT64                   Value
  );

/**
  Return data element in an Array by its Index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.

  @retval Value                  The data to be returned

**/
UINT64
GetArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINTN                    Index
  );
  
/**
  Search an Option of a Question by its value.

  @param  Question               The Question
  @param  OptionValue            Value for Option to be searched.

  @retval Pointer                Pointer to the found Option.
  @retval NULL                   Option not found.

**/
DISPLAY_QUESTION_OPTION *
ValueToOption (
  IN FORM_DISPLAY_ENGINE_STATEMENT   *Question,
  IN EFI_HII_VALUE                   *OptionValue
  );

/**
  Compare two Hii value.

  @param  Value1                 Expression value to compare on left-hand.
  @param  Value2                 Expression value to compare on right-hand.
  @param  Result                 Return value after compare.
                                 retval 0                      Two operators equal.
                                 return Positive value if Value1 is greater than Value2.
                                 retval Negative value if Value1 is less than Value2.
  @param  HiiHandle              Only required for string compare.

  @retval other                  Could not perform compare on two values.
  @retval EFI_SUCCESS            Compare the value success.

**/
EFI_STATUS
CompareHiiValue (
  IN  EFI_HII_VALUE   *Value1,
  IN  EFI_HII_VALUE   *Value2,
  OUT INTN            *Result,
  IN  EFI_HII_HANDLE  HiiHandle OPTIONAL
  );

/**
  Draw a pop up windows based on the dimension, number of lines and
  strings specified.

  @param RequestedWidth  The width of the pop-up.
  @param NumberOfLines   The number of lines.
  @param ...             A series of text strings that displayed in the pop-up.

**/
VOID
EFIAPI
CreateMultiStringPopUp (
  IN  UINTN                       RequestedWidth,
  IN  UINTN                       NumberOfLines,
  ...
  );

/**
  Will copy LineWidth amount of a string in the OutputString buffer and return the
  number of CHAR16 characters that were copied into the OutputString buffer.
  The output string format is:
    Glyph Info + String info + '\0'.

  In the code, it deals \r,\n,\r\n same as \n\r, also it not process the \r or \g.

  @param  InputString            String description for this option.
  @param  LineWidth              Width of the desired string to extract in CHAR16
                                 characters
  @param  GlyphWidth             The glyph width of the begin of the char in the string.
  @param  Index                  Where in InputString to start the copy process
  @param  OutputString           Buffer to copy the string into

  @return Returns the number of CHAR16 characters that were copied into the OutputString 
  buffer, include extra glyph info and '\0' info.

**/
UINT16
GetLineByWidth (
  IN      CHAR16                      *InputString,
  IN      UINT16                      LineWidth,
  IN OUT  UINT16                      *GlyphWidth,
  IN OUT  UINTN                       *Index,
  OUT     CHAR16                      **OutputString
  );


/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The Hii handle for this string package.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  );
  
/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  If String is NULL, then ASSERT ().

  @param String          The input string to be counted.

  @return Storage space for the input string.

**/
UINTN
GetStringWidth (
  IN CHAR16               *String
  );

/**
  This routine reads a numeric value from the user input.

  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If numerical input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetNumericInput (
  IN  UI_MENU_OPTION              *MenuOption
  );

/**
  Get string or password input from user.

  @param  MenuOption        Pointer to the current input menu.
  @param  Prompt            The prompt string shown on popup window.
  @param  StringPtr         Old user input and destination for use input string.

  @retval EFI_SUCCESS       If string input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
ReadString (
  IN     UI_MENU_OPTION              *MenuOption,
  IN     CHAR16                      *Prompt,
  IN OUT CHAR16                      *StringPtr
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
  Wait for a key to be pressed by user.

  @param Key         The key which is pressed by user.

  @retval EFI_SUCCESS The function always completed successfully.

**/
EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY           *Key
  );

/**
  Get selection for OneOf and OrderedList (Left/Right will be ignored).

  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If Option input is processed successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetSelectionInputPopUp (
  IN  UI_MENU_OPTION              *MenuOption
  );

/**
  Process the help string: Split StringPtr to several lines of strings stored in
  FormattedString and the glyph width of each line cannot exceed gHelpBlockWidth.

  @param  StringPtr              The entire help string.
  @param  FormattedString        The oupput formatted string.
  @param  EachLineWidth          The max string length of each line in the formatted string.
  @param  RowCount               TRUE: if Question is selected.

**/
UINTN
ProcessHelpString (
  IN  CHAR16  *StringPtr,
  OUT CHAR16  **FormattedString,
  OUT UINT16  *EachLineWidth,
  IN  UINTN   RowCount
  );

/**
  Process a Question's Option (whether selected or un-selected).

  @param  MenuOption             The MenuOption for this Question.
  @param  Selected               TRUE: if Question is selected.
  @param  OptionString           Pointer of the Option String to be displayed.
  @param  SkipErrorValue         Whether need to return when value without option for it.

  @retval EFI_SUCCESS            Question Option process success.
  @retval Other                  Question Option process fail.

**/
EFI_STATUS
ProcessOptions (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected,
  OUT CHAR16                      **OptionString,
  IN  BOOLEAN                     SkipErrorValue
  );

/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

**/
VOID
SetUnicodeMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN CHAR16 Value
  );

/**
  Display one form, and return user input.
  
  @param FormData                Form Data to be shown.
  @param UserInputData           User input data.
  
  @retval EFI_SUCCESS            Form Data is shown, and user input is got.
**/
EFI_STATUS
EFIAPI 
FormDisplay (
  IN  FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT                *UserInputData
  );

/**
  Clear Screen to the initial state.
**/
VOID
EFIAPI 
DriverClearDisplayPage (
  VOID
  );

/**
  Exit Display and Clear Screen to the original state.

**/
VOID
EFIAPI 
ExitDisplay (
  VOID
  );

/**
  Process nothing.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
EmptyEventProcess (
  IN  EFI_EVENT    Event,
  IN  VOID         *Context
  );

/**
  Process for the refresh interval statement.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
RefreshTimeOutProcess (
  IN  EFI_EVENT    Event,
  IN  VOID         *Context
  );

/**
  Record the highlight menu and top of screen menu info.

  @param  Highlight               The menu opton which is highlight.
  @param  TopOfScreen             The menu opton which is at the top of the form.
  @param  SkipValue               The skip line info for the top of screen menu.

**/
VOID
UpdateHighlightMenuInfo (
  IN  LIST_ENTRY                      *Highlight,
  IN  LIST_ENTRY                      *TopOfScreen,
  IN  UINTN                           SkipValue
  );

#endif
