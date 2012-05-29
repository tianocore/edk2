/** @file
Private structure, MACRO and function definitions for User Interface related functionalities.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UI_H_
#define _UI_H_

//
// Globals
//
#define REGULAR_NUMERIC 0
#define TIME_NUMERIC    1
#define DATE_NUMERIC    2

#define SUBTITLE_INDENT  2


//
// It take 23 characters including the NULL to print a 64 bits number with "[" and "]".
// pow(2, 64) = [18446744073709551616]
//
#define MAX_NUMERIC_INPUT_WIDTH 23

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
  CfUiDefault,
  CfUiNoOperation,
  CfExit,
  CfUiHotKey,
  CfMaxControlFlag
} UI_CONTROL_FLAG;

#define UI_ACTION_NONE               0
#define UI_ACTION_REFRESH_FORM       1
#define UI_ACTION_REFRESH_FORMSET    2
#define UI_ACTION_EXIT               3

typedef struct _UI_MENU_LIST UI_MENU_LIST;

typedef struct {
  EFI_HII_HANDLE  Handle;

  //
  // Target formset/form/Question information
  //
  EFI_GUID        FormSetGuid;
  UINT16          FormId;
  UINT16          QuestionId;
  UINTN           Sequence;  // used for time/date only.

  UINTN           TopRow;
  UINTN           BottomRow;
  UINTN           PromptCol;
  UINTN           OptionCol;
  UINTN           CurrentRow;

  //
  // Ation for Browser to taken:
  //   UI_ACTION_NONE            - navigation inside a form
  //   UI_ACTION_REFRESH_FORM    - re-evaluate expressions and repaint form
  //   UI_ACTION_REFRESH_FORMSET - re-parse formset IFR binary
  //
  UINTN           Action;

  //
  // Current selected fomset/form/Question
  //
  FORM_BROWSER_FORMSET    *FormSet;
  FORM_BROWSER_FORM       *Form;
  FORM_BROWSER_STATEMENT  *Statement;

  //
  // Whether the Form is editable
  //
  BOOLEAN                 FormEditable;

  UI_MENU_LIST            *CurrentMenu;
} UI_MENU_SELECTION;

#define UI_MENU_OPTION_SIGNATURE  SIGNATURE_32 ('u', 'i', 'm', 'm')
#define UI_MENU_LIST_SIGNATURE    SIGNATURE_32 ('u', 'i', 'm', 'l')

typedef struct {
  UINTN                   Signature;
  LIST_ENTRY              Link;

  EFI_HII_HANDLE          Handle;
  FORM_BROWSER_STATEMENT  *ThisTag;
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
} UI_MENU_OPTION;

#define MENU_OPTION_FROM_LINK(a)  CR (a, UI_MENU_OPTION, Link, UI_MENU_OPTION_SIGNATURE)

struct _UI_MENU_LIST {
  UINTN           Signature;
  LIST_ENTRY      Link;

  EFI_HII_HANDLE  HiiHandle;
  EFI_GUID        FormSetGuid;
  UINT16          FormId;
  UINT16          QuestionId;
  UINTN           Sequence;    // used for time/date only.

  UI_MENU_LIST    *Parent;
  LIST_ENTRY      ChildListHead;
};

#define UI_MENU_LIST_FROM_LINK(a)  CR (a, UI_MENU_LIST, Link, UI_MENU_LIST_SIGNATURE)

typedef struct _MENU_REFRESH_ENTRY MENU_REFRESH_ENTRY;
struct _MENU_REFRESH_ENTRY {
  MENU_REFRESH_ENTRY          *Next;
  UI_MENU_OPTION              *MenuOption;  // Describes the entry needing an update
  UI_MENU_SELECTION           *Selection;
  UINTN                       CurrentColumn;
  UINTN                       CurrentRow;
  UINTN                       CurrentAttribute;
  EFI_EVENT                   Event;
};

typedef struct {
  UINT16              ScanCode;
  UI_SCREEN_OPERATION ScreenOperation;
} SCAN_CODE_TO_SCREEN_OPERATION;

typedef struct {
  UI_SCREEN_OPERATION ScreenOperation;
  UI_CONTROL_FLAG     ControlFlag;
} SCREEN_OPERATION_T0_CONTROL_FLAG;


extern LIST_ENTRY          gMenuOption;
extern MENU_REFRESH_ENTRY  *gMenuRefreshHead;
extern UI_MENU_SELECTION   *gCurrentSelection;
extern BOOLEAN             mHiiPackageListUpdated;

//
// Global Functions
//
/**
  Initialize Menu option list.

**/
VOID
UiInitMenu (
  VOID
  );

/**
  Initialize Menu option list.

**/
VOID
UiInitMenuList (
  VOID
  );

/**
  Free Menu option linked list.

**/
VOID
UiFreeMenu (
  VOID
  );

/**
  Create a menu with specified formset GUID and form ID, and add it as a child
  of the given parent menu.

  @param  Parent                 The parent of menu to be added.
  @param  HiiHandle              Hii handle related to this formset.
  @param  FormSetGuid            The Formset Guid of menu to be added.
  @param  FormId                 The Form ID of menu to be added.

  @return A pointer to the newly added menu or NULL if memory is insufficient.

**/
UI_MENU_LIST *
UiAddMenuList (
  IN OUT UI_MENU_LIST     *Parent,
  IN EFI_HII_HANDLE       HiiHandle,
  IN EFI_GUID             *FormSetGuid,
  IN UINT16               FormId
  );

/**
  Search Menu with given FormId in the parent menu and all its child menus.

  @param  Parent                 The parent of menu to search.
  @param  FormSetGuid            The Formset GUID of the menu to search.  
  @param  FormId                 The Form ID of menu to search.

  @return A pointer to menu found or NULL if not found.

**/
UI_MENU_LIST *
UiFindChildMenuList (
  IN UI_MENU_LIST         *Parent,
  IN EFI_GUID             *FormSetGuid, 
  IN UINT16               FormId
  );

/**
  Search Menu with given FormSetGuid and FormId in all cached menu list.

  @param  FormSetGuid            The Formset GUID of the menu to search.
  @param  FormId                 The Form ID of menu to search.

  @return A pointer to menu found or NULL if not found.

**/
UI_MENU_LIST *
UiFindMenuList (
  IN EFI_GUID             *FormSetGuid,
  IN UINT16               FormId
  );

/**
  Free Menu option linked list.

**/
VOID
UiFreeRefreshList (
  VOID
  );

/**
  Add one menu option by specified description and context.

  @param  String                 String description for this option.
  @param  Handle                 Hii handle for the package list.
  @param  Form                   The form this statement belong to.
  @param  Statement              Statement of this Menu Option.
  @param  NumberOfLines          Display lines for this Menu Option.
  @param  MenuItemCount          The index for this Option in the Menu.

  @retval Pointer                Pointer to the added Menu Option.

**/
UI_MENU_OPTION *
UiAddMenuOption (
  IN CHAR16                  *String,
  IN EFI_HII_HANDLE          Handle,
  IN FORM_BROWSER_FORM       *Form,
  IN FORM_BROWSER_STATEMENT  *Statement,
  IN UINT16                  NumberOfLines,
  IN UINT16                  MenuItemCount
  );

/**
  Display menu and wait for user to select one menu option, then return it.
  If AutoBoot is enabled, then if user doesn't select any option,
  after period of time, it will automatically return the first menu option.

  @param  Selection              Menu selection.

  @return Return the pointer of the menu which selected,
  @return otherwise return NULL.

**/
EFI_STATUS
UiDisplayMenu (
  IN OUT UI_MENU_SELECTION           *Selection
  );

/**
  Free up the resource allocated for all strings required
  by Setup Browser.

**/
VOID
FreeBrowserStrings (
  VOID
  );

/**
  Process the goto op code, update the info in the selection structure.

  @param Statement    The statement belong to goto op code.
  @param Selection    The selection info.
  @param Repaint      Whether need to repaint the menu.
  @param NewLine      Whether need to create new line.

  @retval EFI_SUCCESS    The menu process successfully.
  @return Other value if the process failed.
**/
EFI_STATUS
ProcessGotoOpCode (
  IN OUT   FORM_BROWSER_STATEMENT      *Statement,
  IN OUT   UI_MENU_SELECTION           *Selection,
  OUT      BOOLEAN                     *Repaint,
  OUT      BOOLEAN                     *NewLine
  );


/**
  The worker function that send the displays to the screen. On output,
  the selection made by user is returned.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.

  @retval EFI_SUCCESS    The page is displayed successfully.
  @return Other value if the page failed to be diplayed.

**/
EFI_STATUS
SetupBrowser (
  IN OUT UI_MENU_SELECTION    *Selection
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
  Wait for a given event to fire, or for an optional timeout to expire.

  @param  Event                  The event to wait for
  @param  Timeout                An optional timeout value in 100 ns units.
  @param  RefreshInterval        Menu refresh interval (in seconds).

  @retval EFI_SUCCESS            Event fired before Timeout expired.
  @retval EFI_TIME_OUT           Timout expired before Event fired.

**/
EFI_STATUS
UiWaitForSingleEvent (
  IN EFI_EVENT                Event,
  IN UINT64                   Timeout, OPTIONAL
  IN UINT8                    RefreshInterval OPTIONAL
  );

/**
  Draw a pop up windows based on the dimension, number of lines and
  strings specified.

  @param ScreenWidth     The width of the pop-up.
  @param NumberOfLines   The number of lines.
  @param ...             A series of text strings that displayed in the pop-up.

**/
VOID
EFIAPI
CreateMultiStringPopUp (
  IN  UINTN                       ScreenWidth,
  IN  UINTN                       NumberOfLines,
  ...
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
  Get selection for OneOf and OrderedList (Left/Right will be ignored).

  @param  Selection         Pointer to current selection.
  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If Option input is processed successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetSelectionInputPopUp (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption
  );

/**
  This routine reads a numeric value from the user input.

  @param  Selection         Pointer to current selection.
  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If numerical input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetNumericInput (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption
  );

/**
  Update status bar on the bottom of menu.

  @param  Selection              Current selection info.
  @param  MessageType            The type of message to be shown.
  @param  Flags                  The flags in Question header.
  @param  State                  Set or clear.

**/
VOID
UpdateStatusBar (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UINTN                       MessageType,
  IN  UINT8                       Flags,
  IN  BOOLEAN                     State
  );

/**
  Process Question Config.

  @param  Selection              The UI menu selection.
  @param  Question               The Question to be peocessed.

  @retval EFI_SUCCESS            Question Config process success.
  @retval Other                  Question Config process fail.

**/
EFI_STATUS
ProcessQuestionConfig (
  IN  UI_MENU_SELECTION       *Selection,
  IN  FORM_BROWSER_STATEMENT  *Question
  );

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
  IN FORM_BROWSER_STATEMENT   *Question,
  IN OUT CHAR16               *FormattedNumber,
  IN UINTN                    BufferSize
  );

/**
  Search an Option of a Question by its value.

  @param  Question               The Question
  @param  OptionValue            Value for Option to be searched.

  @retval Pointer                Pointer to the found Option.
  @retval NULL                   Option not found.

**/
QUESTION_OPTION *
ValueToOption (
  IN FORM_BROWSER_STATEMENT   *Question,
  IN EFI_HII_VALUE            *OptionValue
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
  Process a Question's Option (whether selected or un-selected).

  @param  Selection              Pointer to UI_MENU_SELECTION.
  @param  MenuOption             The MenuOption for this Question.
  @param  Selected               TRUE: if Question is selected.
  @param  OptionString           Pointer of the Option String to be displayed.

  @retval EFI_SUCCESS            Question Option process success.
  @retval Other                  Question Option process fail.

**/
EFI_STATUS
ProcessOptions (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected,
  OUT CHAR16                      **OptionString
  );

/**
  Process the help string: Split StringPtr to several lines of strings stored in
  FormattedString and the glyph width of each line cannot exceed gHelpBlockWidth.

  @param  StringPtr              The entire help string.
  @param  FormattedString        The oupput formatted string.
  @param  EachLineWidth          The string length of each line in the formatted string.
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
  Update key's help imformation.

  @param Selection       Tell setup browser the information about the Selection
  @param  MenuOption     The Menu option
  @param  Selected       Whether or not a tag be selected

**/
VOID
UpdateKeyHelp (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected
  );

/**
  Clear retangle with specified text attribute.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
ClearLines (
  IN UINTN               LeftColumn,
  IN UINTN               RightColumn,
  IN UINTN               TopRow,
  IN UINTN               BottomRow,
  IN UINTN               TextAttribute
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
  Will copy LineWidth amount of a string in the OutputString buffer and return the
  number of CHAR16 characters that were copied into the OutputString buffer.
  In the code, it deals \r,\n,\r\n same as \n\r, also it not process the \r or \g.

  @param  InputString            String description for this option.
  @param  LineWidth              Width of the desired string to extract in CHAR16
                                 characters
  @param  GlyphWidth             The glyph width of the beging char in the string.
  @param  Index                  Where in InputString to start the copy process
  @param  OutputString           Buffer to copy the string into

  @return Returns the number of CHAR16 characters that were copied into the OutputString buffer.

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
  Get the supported width for a particular op-code

  @param  Statement              The FORM_BROWSER_STATEMENT structure passed in.
  @param  Handle                 The handle in the HII database being used

  @return Returns the number of CHAR16 characters that is support.

**/
UINT16
GetWidth (
  IN FORM_BROWSER_STATEMENT        *Statement,
  IN EFI_HII_HANDLE                 Handle
  );

/**
  Concatenate a narrow string to another string.

  @param Destination The destination string.
  @param Source      The source string. The string to be concatenated.
                     to the end of Destination.

**/
VOID
NewStrCat (
  IN OUT CHAR16               *Destination,
  IN     CHAR16               *Source
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
  Reset stack pointer to begin of the stack.

**/
VOID
ResetScopeStack (
  VOID
  );

/**
  Push the expression options onto the Stack.

  @param  Pointer                Pointer to the current expression.
  @param  Level                  Which type this expression belong to. Form, 
                                 statement or option?

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushConditionalExpression (
  IN FORM_EXPRESSION     *Pointer,
  IN EXPRESS_LEVEL               Level
  );

/**
  Pop the expression options from the Stack

  @param  Level                  Which type this expression belong to. Form, 
                                 statement or option?

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopConditionalExpression (
  IN  EXPRESS_LEVEL      Level
  );
  
/**
  Get the expression Buffer pointer.
  
  @param  Level                  Which type this expression belong to. Form, 
                                 statement or option?

  @retval  The start pointer of the expression buffer or NULL.

**/
FORM_EXPRESSION **
GetConditionalExpressionList (
  IN EXPRESS_LEVEL       Level
  );

/**
  Get the expression list count.
  
  @param  Level                  Which type this expression belong to. Form, 
                                 statement or option?

  @retval >=0                    The expression count
  @retval -1                     Input parameter error.

**/
INTN 
GetConditionalExpressionCount (
  IN EXPRESS_LEVEL       Level
  );

/**
  Push an Operand onto the Stack

  @param  Operand                Operand to push.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PushScope (
  IN UINT8   Operand
  );

/**
  Pop an Operand from the Stack

  @param  Operand                Operand to pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PopScope (
  OUT UINT8     *Operand
  );

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetCurrentExpressionStack (
  VOID
  );

/**
  Push current expression onto the Stack

  @param  Pointer                Pointer to current expression.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushCurrentExpression (
  IN VOID  *Pointer
  );

/**
  Pop current expression from the Stack

  @param  Pointer                Pointer to current expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopCurrentExpression (
  OUT VOID    **Pointer
  );

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetMapExpressionListStack (
  VOID
  );

/**
  Push the list of map expression onto the Stack

  @param  Pointer                Pointer to the list of map expression to be pushed.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushMapExpressionList (
  IN VOID  *Pointer
  );

/**
  Pop the list of map expression from the Stack

  @param  Pointer                Pointer to the list of map expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopMapExpressionList (
  OUT VOID    **Pointer
  );

/**
  Get Form given its FormId.

  @param  FormSet                The formset which contains this form.
  @param  FormId                 Id of this form.

  @retval Pointer                The form.
  @retval NULL                   Specified Form is not found in the formset.

**/
FORM_BROWSER_FORM *
IdToForm (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN UINT16                FormId
  );

/**
  Search a Question in Formset scope using its QuestionId.

  @param  FormSet                The formset which contains this form.
  @param  Form                   The form which contains this Question.
  @param  QuestionId             Id of this Question.

  @retval Pointer                The Question.
  @retval NULL                   Specified Question not found in the form.

**/
FORM_BROWSER_STATEMENT *
IdToQuestion (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN FORM_BROWSER_FORM     *Form,
  IN UINT16                QuestionId
  );

/**
  Zero extend integer/boolean/date/time to UINT64 for comparing.

  @param  Value                  HII Value to be converted.

**/
VOID
ExtendValueToU64 (
  IN  EFI_HII_VALUE   *Value
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
  Evaluate the result of a HII expression

  If Expression is NULL, then ASSERT.

  @param  FormSet                FormSet associated with this expression.
  @param  Form                   Form associated with this expression.
  @param  Expression             Expression to be evaluated.

  @retval EFI_SUCCESS            The expression evaluated successfuly
  @retval EFI_NOT_FOUND          The Question which referenced by a QuestionId
                                 could not be found.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack
  @retval EFI_INVALID_PARAMETER  Syntax error with the Expression

**/
EFI_STATUS
EvaluateExpression (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN FORM_BROWSER_FORM     *Form,
  IN OUT FORM_EXPRESSION   *Expression
  );

/**
  Return the result of the expression list. Check the expression list and 
  return the highest priority express result.  
  Priority: DisableIf > SuppressIf > GrayOutIf > FALSE

  @param  ExpList             The input expression list.
  @param  Evaluate            Whether need to evaluate the expression first.
  @param  FormSet             FormSet associated with this expression. Only 
                              needed when Evaluate is TRUE
  @param  Form                Form associated with this expression. Only 
                              needed when Evaluate is TRUE 

  @retval EXPRESS_RESULT      Return the higher priority express result. 
                              DisableIf > SuppressIf > GrayOutIf > FALSE

**/
EXPRESS_RESULT 
EvaluateExpressionList (
  IN FORM_EXPRESSION_LIST *ExpList,
  IN BOOLEAN              Evaluate,
  IN FORM_BROWSER_FORMSET *FormSet, OPTIONAL
  IN FORM_BROWSER_FORM    *Form OPTIONAL
  );

#endif // _UI_H
