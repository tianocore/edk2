/*++

Copyright (c) 2006, Intel Corporation                                                         
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

--*/

#ifndef _SETUP_H
#define _SETUP_H

//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//
#include "SetupBrowserStrDefs.h"
extern UINT8  SetupBrowserStrings[];

//
// Screen definitions
//
#define BANNER_HEIGHT                 4
#define BANNER_COLUMNS                3

#define FRONT_PAGE_HEADER_HEIGHT      4
#define NONE_FRONT_PAGE_HEADER_HEIGHT 3
#define LEFT_SKIPPED_COLUMNS          4
#define FOOTER_HEIGHT                 4
#define STATUS_BAR_HEIGHT             1
#define SCROLL_ARROW_HEIGHT           1
#define POPUP_PAD_SPACE_COUNT         5
#define POPUP_FRAME_WIDTH             2


#define EFI_SETUP_APPLICATION_SUBCLASS    0x00
#define EFI_GENERAL_APPLICATION_SUBCLASS  0x01
#define EFI_FRONT_PAGE_SUBCLASS           0x02
#define EFI_SINGLE_USE_SUBCLASS           0x03  // Used to display a single entity and then exit
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

#define LEFT_CHECKBOX_DELIMITER   L"["
#define RIGHT_CHECKBOX_DELIMITER  L"]"

#define CHECK_ON                  L"X"
#define CHECK_OFF                 L" "

#define TIME_SEPARATOR            L':'
#define DATE_SEPARATOR            L'/'

#define YES_ANSWER                L'Y'
#define NO_ANSWER                 L'N'

//
// Up to how many lines does the ordered list display
//
#define ORDERED_LIST_SIZE 4

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
// This width is basically the sum of the prompt and option widths
//
#define QUESTION_BLOCK_WIDTH  50

//
// Width of the Language Description (Using ISO-639-2 3 ASCII letter standard)
//
#define LANG_DESC_WIDTH 3

//
// Maximum Number of Binaries we can see
//
#define MAX_BINARIES  255

//
// Invalid Handle
//
#define EFI_HII_INVALID_HANDLE  0xFFFF

//
// Invalid Offset Value
//
#define INVALID_OFFSET_VALUE  0xFFFF

struct StringPart {
  struct StringPart *Next;
  CHAR8             String[QUESTION_BLOCK_WIDTH + 2];
};

//
// The tag definition defines the data associated with a tag (an operation
// in the IFR lingo).  The tag is thus a modified union of all the data
// required for tags.  The user should be careful to only rely upon information
// relevant to that tag as the contents of other fields is undefined.
//
// The intent here is for this to be all of the data associated with a particular tag.
// Some of this data is extracted from the IFR and left alone.  Other data will be derived
// when the page is selected (since that's the first time we really know what language the
// page is to be displayed in) and still other data will vary based on the selection.
// If you'd like to consider alternatives, let me know.  This structure has grown somewhat organically.
// It gets a new item stuffed in it when a new item is needed.  When I finally decided I needed the
// StringPart structure, items got added here, for example.
//
typedef struct {
  UINT8                 Operand;        // The operand (first byte) of the variable length tag.
  EFI_GUID              GuidValue;      // Primarily for FormSet data
  EFI_PHYSICAL_ADDRESS  CallbackHandle;
  UINT16                Class;
  UINT16                SubClass;
  UINT16                NumberOfLines;  // The number of lines the tag takes up on the page.  Adjusted when we display the page as it can change from language to language.
  UINT16                PageLine;
  UINT16                PageColumn;
  UINT16                OptionWidth;    // The option can be wider than the column usually associated with options.  This is the width on the last option line
  STRING_REF            Text;           // Used for title, subtitle, prompt, etc.  This is the string token associated with the string.  This token is language independent.
  STRING_REF            TextTwo;        // Used for title, subtitle, prompt, etc.  This is the string token associated with the string.  This token is language independent.
  STRING_REF            Help;           // Null means no help  Same as above but for languages.
  UINT16                Consistency;    // Do we need to check this opcode against consistency?  If > 0, yes.
  UINT16                Id;
  UINT16                Id2;            // The questions (mainly) have identifiers associated with them.  These are filled in from the IFR tags and used by e.g. the RPN calculations. (com1 is set to, versus com2 is set to)
  //
  // These are the three values that are created to determine where in the variable the data is stored.  This should, in general,
  // be allocated by the build tool.  The one major issue is, once storage is allocated for something, it can't be reallocated or we will get a mess.
  //
  UINT16                StorageStart;
  //
  // These are the three values that are created to determine where in the variable the data is stored.  This should, in general,
  // be allocated by the build tool.  The one major issue is, once storage is allocated for something, it can't be reallocated or we will get a mess.
  //
  UINT8                 StorageWidth; 
  //
  // These are the three values that are created to determine where in the variable the data is stored.  This should, in general,
  // be allocated by the build tool.  The one major issue is, once storage is allocated for something, it can't be reallocated or we will get a mess.
  //
  UINT16                Value;
  //
  // (Default or current)
  //
  UINT8                 Flags;        
  UINT16                Key;
  //
  // Used to preserve a value during late consistency checking
  //
  UINT16                OldValue;     
  UINT16                Minimum;
  UINT16                Maximum;
  UINT16                Step;
  UINT16                Default;
  UINT16                NvDataSize;
  UINT16                ConsistencyId;
  BOOLEAN               GrayOut;
  BOOLEAN               Suppress;
  UINT16                Encoding;     // Data from the tags.  The first three are used by the numeric input.  Encoding is used by the password stuff (a placeholder today - may go away).
  UINT16                *IntList;     // List of the values possible for a list question
  //
  // The string is obtained from the string list and formatted into lines and the lines are held in this linked list.
  // If we have more than a screen's worth of items, we will end up with cases where we have to display the last couple
  // lines of a tag's string above the currently selected one, or, display a few lines of a tag at the bottom of a screen.
  //
  struct StringPart     *StringList;  
  BOOLEAN               ResetRequired;    // Primarily used to determine if a reset is required by changing this op-code.
  UINT16                VariableNumber;   // Used to define which variable the StorageStart will be pertinent for (0-based)  For single variable VFR this will always be 0.
  //
  // Used to define which variable the StorageStart will be pertinent for (0-based)  This is used for boolean check of ID versus ID
  // so that a user can compare the value of one variable.field content versus another variable.field content.
  //
  UINT16                VariableNumber2;  
} EFI_TAG;

#define EFI_FORM_DATA_SIGNATURE EFI_SIGNATURE_32 ('F', 'o', 'r', 'm')

typedef struct {
  UINTN                     Signature;

  EFI_HII_PROTOCOL          *Hii;
  EFI_FORM_BROWSER_PROTOCOL FormConfig;
} EFI_FORM_CONFIGURATION_DATA;

#define EFI_FORM_DATA_FROM_THIS(a)  CR (a, EFI_FORM_CONFIGURATION_DATA, FormConfig, EFI_FORM_DATA_SIGNATURE)

typedef struct _EFI_VARIABLE_DEFINITION {
  CHAR8                           *NvRamMap;
  CHAR8                           *FakeNvRamMap;    // This is where the storage for NULL devices go (e.g. RTC)
  EFI_GUID                        Guid;
  UINT16                          VariableId;
  UINT16                          VariableSize;
  UINT16                          VariableFakeSize; // For dynamically created and NULL device options, this is the latest size
  CHAR16                          *VariableName;
  struct _EFI_VARIABLE_DEFINITION *Next;
  struct _EFI_VARIABLE_DEFINITION *Previous;
} EFI_VARIABLE_DEFINITION;

typedef struct {
  UINT32      Length;                               // Length in bytes between beginning of struc and end of Strings
  CHAR8       LanguageCode[4];                      // ISO-639-2 language code with a null-terminator
  RELOFST     PrintableLanguageName;                // Translated name of the Language, "English"/"Espanol" etc
  UINT32      Attributes;                           // If on, the language is intended to be printed right to left.  The default (off) is to print left to right.
  RELOFST     StringsPointers[1];                   // Pointing to string offset from beginning of String Binary
  EFI_STRING  Strings[1];                           // Array of String Entries.  Note the number of entries for Strings and StringsPointers will be the same
} EFI_LANGUAGE_SET;

//
// This encapsulates all the pointers associated with found IFR binaries
//
typedef struct _EFI_IFR_BINARY {
  struct _EFI_IFR_BINARY  *Next;
  VOID                    *IfrPackage;  // Handy for use in freeing the data later since this is the header of the buffer
  VOID                    *FormBinary;
  EFI_HII_HANDLE          Handle;
  STRING_REF              TitleToken;
  BOOLEAN                 UnRegisterOnExit;
} EFI_IFR_BINARY;

//
// This encapsulates all the questions (tags) for a particular Form Set
//
typedef struct _EFI_FORM_TAGS {
  struct _EFI_FORM_TAGS *Next;
  EFI_TAG               *Tags;
} EFI_FORM_TAGS;

//
// This is the database of all inconsistency data.  Each op-code associated
// with inconsistency will be tracked here.  This optimizes the search requirement
// since we will back mark the main tag structure with the op-codes that have reference
// to inconsistency data.  This way when parsing the main tag structure and encountering
// the inconsistency mark - we can search this database to know what the inconsistency
// parameters are for that entry.
//
typedef struct _EFI_INCONSISTENCY_DATA {
  struct _EFI_INCONSISTENCY_DATA  *Next;
  struct _EFI_INCONSISTENCY_DATA  *Previous;
  UINT8                           Operand;
  STRING_REF                      Popup;
  UINT16                          QuestionId1;
  UINT16                          QuestionId2;
  UINT16                          Value;
  UINT16                          ListLength;
  UINT16                          ConsistencyId;
  UINT16                          *ValueList;
  UINT16                          VariableNumber;
  UINT16                          VariableNumber2;
  UINT8                           Width;
} EFI_INCONSISTENCY_DATA;

//
// Encapsulating all found Tag information from all sources
// Each encapsulation also contains the NvRamMap buffer and the Size of the NV store
//
typedef struct _EFI_FILE_FORM_TAGS {
  struct _EFI_FILE_FORM_TAGS  *NextFile;
  EFI_INCONSISTENCY_DATA      *InconsistentTags;
  EFI_VARIABLE_DEFINITION     *VariableDefinitions;
  EFI_FORM_TAGS               FormTags;
} EFI_FILE_FORM_TAGS;

typedef struct {
  STRING_REF  Banner[BANNER_HEIGHT][BANNER_COLUMNS];
} BANNER_DATA;

//
// Head of the Binary structures
//
EFI_IFR_BINARY    *gBinaryDataHead;

//
// The IFR binary that the user chose to run
//
UINTN             gActiveIfr;

EFI_HII_PROTOCOL  *Hii;

VOID              *CachedNVEntry;
BANNER_DATA       *BannerData;
EFI_HII_HANDLE    FrontPageHandle;
STRING_REF        FrontPageTimeOutTitle;
INT16             FrontPageTimeOutValue;
UINTN             gClassOfVfr;
UINTN             gFunctionKeySetting;
BOOLEAN           gResetRequired;
BOOLEAN           gExitRequired;
BOOLEAN           gSaveRequired;
BOOLEAN           gNvUpdateRequired;
UINT16            gConsistencyId;
UINTN             gPriorMenuEntry;
EFI_HII_HANDLE    gHiiHandle;
BOOLEAN           gFirstIn;
VOID              *gPreviousValue;
UINT16            gDirection;
EFI_SCREEN_DESCRIPTOR gScreenDimensions;
BOOLEAN           gUpArrow;
BOOLEAN           gDownArrow;
BOOLEAN           gTimeOnScreen;
BOOLEAN           gDateOnScreen;

//
// Browser Global Strings
//
CHAR16            *gFunctionOneString;
CHAR16            *gFunctionTwoString;
CHAR16            *gFunctionNineString;
CHAR16            *gFunctionTenString;
CHAR16            *gEnterString;
CHAR16            *gEnterCommitString;
CHAR16            *gEscapeString;
CHAR16            *gMoveHighlight;
CHAR16            *gMakeSelection;
CHAR16            *gNumericInput;
CHAR16            *gToggleCheckBox;
CHAR16            *gPromptForPassword;
CHAR16            *gPromptForNewPassword;
CHAR16            *gConfirmPassword;
CHAR16            *gConfirmError;
CHAR16            *gPressEnter;
CHAR16            *gEmptyString;
CHAR16            *gAreYouSure;
CHAR16            *gYesResponse;
CHAR16            *gNoResponse;
CHAR16            *gMiniString;
CHAR16            *gPlusString;
CHAR16            *gMinusString;
CHAR16            *gAdjustNumber;

CHAR16            gPromptBlockWidth;
CHAR16            gOptionBlockWidth;
CHAR16            gHelpBlockWidth;

//
// Global Procedure Defines
//
VOID
InitializeBrowserStrings (
  VOID
  )
;

UINTN
Print (
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

VOID
DisplayPageFrame (
  VOID
  )
;

CHAR16            *
GetToken (
  IN  STRING_REF                              IfrBinaryTitle,
  IN  EFI_HII_HANDLE                          HiiHandle
  )
;

VOID
GetTagCount (
  IN      UINT8                                 *RawFormSet,
  IN OUT  UINT16                                *NumberOfTags
  )
;

VOID
GetNumericHeader (
  IN  EFI_TAG             *Tag,
  IN  UINT8               *RawFormSet,
  IN  UINT16              Index,
  IN  UINT16              NumberOfLines,
  IN  EFI_FILE_FORM_TAGS  *FileFormTags,
  IN  UINT16              CurrentVariable
  )
;

VOID
GetQuestionHeader (
  IN  EFI_TAG             *Tag,
  IN  UINT8               *RawFormSet,
  IN  UINT16              Index,
  IN  EFI_FILE_FORM_TAGS  *FileFormTags,
  IN  UINT16              CurrentVariable
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

#endif
