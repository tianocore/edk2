/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Ui.h 

Abstract:

  Head file UI

Revision History

--*/

#ifndef _UI_H
#define _UI_H

//
// Globals
//
#define REGULAR_NUMERIC 0
#define TIME_NUMERIC    1
#define DATE_NUMERIC    2

typedef enum {
  UiNoOperation,
  UiDefault,
  UiSelect,
  UiUp,
  UiDown,
  UiLeft,
  UiRight,
  UiReset,
  UiSave,
  UiPrevious,
  UiPageUp,
  UiPageDown,
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
  CfUiPrevious,
  CfUiSelect,
  CfUiReset,
  CfUiLeft,
  CfUiRight,
  CfUiUp,
  CfUiPageUp,
  CfUiPageDown,
  CfUiDown,
  CfUiSave,
  CfUiDefault,
  CfUiNoOperation,
  CfExit,
  CfMaxControlFlag
} UI_CONTROL_FLAG;

#define UI_MENU_OPTION_SIGNATURE  EFI_SIGNATURE_32 ('u', 'i', 'm', 'm')
#define UI_MENU_LIST_SIGNATURE    EFI_SIGNATURE_32 ('u', 'i', 'm', 'l')

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;

  UINTN           Row;
  UINTN           Col;
  UINTN           OptCol;
  CHAR16          *Description;
  UINTN           Skip;

  UINTN           IfrNumber;
  VOID            *FormBinary;
  EFI_HII_HANDLE  Handle;
  EFI_TAG         *Tags;
  UINTN           TagIndex;
  EFI_TAG         *ThisTag;
  UINT16          FormId;
  BOOLEAN         Previous;
  UINT16          EntryNumber;
  UINT16          Consistency;
  BOOLEAN         GrayOut;
} UI_MENU_OPTION;

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      MenuLink;

  UI_MENU_OPTION  Selection;
  UINTN           FormerEntryNumber;
} UI_MENU_LIST;

typedef struct _MENU_REFRESH_ENTRY {
  struct _MENU_REFRESH_ENTRY  *Next;
  EFI_FILE_FORM_TAGS          *FileFormTagsHead;
  UINTN                       CurrentColumn;
  UINTN                       CurrentRow;
  UINTN                       CurrentAttribute;
  UI_MENU_OPTION              *MenuOption;  // Describes the entry needing an update
} MENU_REFRESH_ENTRY;

typedef struct {
  UINT16              ScanCode;
  UI_SCREEN_OPERATION ScreenOperation;
} SCAN_CODE_TO_SCREEN_OPERATION;

typedef struct {
  UI_SCREEN_OPERATION ScreenOperation;
  UI_CONTROL_FLAG     ControlFlag;
} SCREEN_OPERATION_T0_CONTROL_FLAG;

LIST_ENTRY          Menu;
LIST_ENTRY          gMenuList;
MENU_REFRESH_ENTRY  *gMenuRefreshHead;

INTN                gEntryNumber;
BOOLEAN             gLastOpr;
//
// Global Functions
//
VOID
UiInitMenu (
  VOID
  )
;

VOID
UiInitMenuList (
  VOID
  )
;

VOID
UiRemoveMenuListEntry (
  IN  UI_MENU_OPTION    *Selection,
  OUT UI_MENU_OPTION    **PreviousSelection
  )
;

VOID
UiFreeMenuList (
  VOID
  )
;

VOID
UiAddMenuListEntry (
  IN UI_MENU_OPTION   *Selection
  )
;

VOID
UiFreeMenu (
  VOID
  )
;

VOID
UiAddMenuOption (
  IN CHAR16         *String,
  IN EFI_HII_HANDLE Handle,
  IN EFI_TAG        *Tag,
  IN VOID           *FormBinary,
  IN UINTN          IfrNumber
  )
;

VOID
UiAddSubMenuOption (
  IN CHAR16           *String,
  IN EFI_HII_HANDLE   Handle,
  IN EFI_TAG          *Tag,
  IN UINTN            TagIndex,
  IN UINT16           FormId,
  IN UINT16           MenuItemCount
  )
;

UI_MENU_OPTION      *
UiDisplayMenu (
  IN  BOOLEAN                      SubMenu,
  IN  EFI_FILE_FORM_TAGS           *FileFormTagsHead,
  OUT EFI_IFR_DATA_ARRAY           *PageData
  )
;

VOID
InitPage (
  VOID
  )
;

UI_MENU_OPTION      *
SetupBrowser (
  IN  UI_MENU_OPTION              *Selection,
  IN  BOOLEAN                     Callback,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  UINT8                       *CallbackData
  )
;


VOID
SetUnicodeMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN CHAR16 Value
  )
;

EFI_STATUS
UiWaitForSingleEvent (
  IN EFI_EVENT                Event,
  IN UINT64                   Timeout OPTIONAL
  )
;

VOID
CreatePopUp (
  IN  UINTN                       ScreenWidth,
  IN  UINTN                       NumberOfLines,
  IN  CHAR16                      *ArrayOfStrings,
  ...
  )
;

EFI_STATUS
ReadString (
  IN  UI_MENU_OPTION              *MenuOption,
  OUT CHAR16                      *StringPtr
  )
;

EFI_STATUS
ReadPassword (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     PromptForPassword,
  IN  EFI_TAG                     *Tag,
  IN  EFI_IFR_DATA_ARRAY          *PageData,
  IN  BOOLEAN                     SecondEntry,
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  OUT CHAR16                      *StringPtr
  )
;

VOID
EncodePassword (
  IN  CHAR16                      *Password,
  IN  UINT8                       MaxSize
  )
;

EFI_STATUS
GetSelectionInputPopUp (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  EFI_TAG                     *Tag,
  IN  UINTN                       ValueCount,
  OUT UINT16                      *Value,
  OUT UINT16                      *KeyValue
  )
;

EFI_STATUS
GetSelectionInputLeftRight (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  EFI_TAG                     *Tag,
  IN  UINTN                       ValueCount,
  OUT UINT16                      *Value
  )
;

EFI_STATUS
GetNumericInput (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  BOOLEAN                     ManualInput,
  IN  EFI_TAG                     *Tag,
  IN  UINTN                       NumericType,
  OUT UINT16                      *Value
  )
;

VOID
UpdateStatusBar (
  IN  UINTN                       MessageType,
  IN  UINT8                       Flags,
  IN  BOOLEAN                     State
  )
;

EFI_STATUS
ProcessOptions (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  EFI_IFR_DATA_ARRAY          *PageData,
  OUT CHAR16                      **OptionString
  )
;

VOID
ProcessHelpString (
  IN  CHAR16                      *StringPtr,
  OUT CHAR16                      **FormattedString,
  IN  UINTN                       RowCount
  )
;

VOID
UpdateKeyHelp (
  IN  UI_MENU_OPTION              *Selection,
  IN  BOOLEAN                     Selected
  )
;

BOOLEAN
ValueIsNotValid (
  IN  BOOLEAN                     Complex,
  IN  UINT16                      Value,
  IN  EFI_TAG                     *Tag,
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  IN  STRING_REF                  *PopUp
  )
;

VOID
FreeData (
  IN EFI_FILE_FORM_TAGS            *FileFormTagsHead,
  IN CHAR16                        *FormattedString,
  IN CHAR16                        *OptionString
  )
;

VOID
ClearLines (
  UINTN                                       LeftColumn,
  UINTN                                       RightColumn,
  UINTN                                       TopRow,
  UINTN                                       BottomRow,
  UINTN                                       TextAttribute
  )
;

UINTN
GetStringWidth (
  CHAR16                                      *String
  )
;

UINT16
GetLineByWidth (
  IN      CHAR16                      *InputString,
  IN      UINT16                      LineWidth,
  IN OUT  UINTN                       *Index,
  OUT     CHAR16                      **OutputString
  )
;

UINT16
GetWidth (
  IN EFI_TAG                          *Tag,
  IN EFI_HII_HANDLE                   Handle
  )
;

VOID
NewStrCat (
  CHAR16                                      *Destination,
  CHAR16                                      *Source
  )
;

VOID
IfrToFormTag (
  IN  UINT8               OpCode,
  IN  EFI_TAG             *TargetTag,
  IN  VOID                *FormData,
  EFI_VARIABLE_DEFINITION *VariableDefinitionsHead
  )
;

EFI_STATUS
ExtractNvValue (
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  IN  UINT16                      VariableId,
  IN  UINT16                      VariableSize,
  IN  UINT16                      OffsetValue,
  OUT VOID                        **Buffer
  )
;

EFI_STATUS
ExtractRequestedNvMap (
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  IN  UINT16                      VariableId,
  OUT EFI_VARIABLE_DEFINITION     **VariableDefinition
  )
;

BOOLEAN
ValueIsScroll (
  IN  BOOLEAN                 Direction,
  IN  LIST_ENTRY              *CurrentPos
  )
;

UINTN
AdjustDateAndTimePosition (
  IN  BOOLEAN                 DirectionUp,
  IN  LIST_ENTRY              **CurrentPosition
  )
;

EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY           *Key
  )
;
#endif // _UI_H
