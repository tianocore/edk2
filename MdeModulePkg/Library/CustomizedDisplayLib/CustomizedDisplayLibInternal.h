/** @file

  This library class defines a set of interfaces to customize Display module

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CUSTOMIZED_DISPLAY_LIB_INTERNAL_H__
#define __CUSTOMIZED_DISPLAY_LIB_INTERNAL_H__

#include <PiDxe.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/FormBrowserEx2.h>
#include <Protocol/DisplayProtocol.h>
#include <Protocol/DevicePath.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/UserManager.h>
#include <Protocol/DevicePathFromText.h>

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
#include <Library/CustomizedDisplayLib.h>

#include "Colors.h"

#define FORMSET_CLASS_PLATFORM_SETUP  0x0001
#define FORMSET_CLASS_FRONT_PAGE      0x0002

#define FRONT_PAGE_HEADER_HEIGHT       6
#define NONE_FRONT_PAGE_HEADER_HEIGHT  3
#define FOOTER_HEIGHT                  4
#define STATUS_BAR_HEIGHT              1

//
// Screen definitions
//
#define BANNER_HEIGHT              6
#define BANNER_COLUMNS             3
#define BANNER_LEFT_COLUMN_INDENT  1

//
// Character definitions
//
#define UPPER_LOWER_CASE_OFFSET  0x20

//
// This is the Input Error Message
//
#define INPUT_ERROR  1

//
// This is the NV RAM update required Message
//
#define NV_UPDATE_REQUIRED  2

typedef struct {
  EFI_STRING_ID    Banner[BANNER_HEIGHT][BANNER_COLUMNS];
} BANNER_DATA;

extern  UINT16                 gClassOfVfr;                        // Formset class information
extern  BANNER_DATA            *gBannerData;
extern  EFI_SCREEN_DESCRIPTOR  gScreenDimensions;
extern  UINTN                  gFooterHeight;

//
// Browser Global Strings
//
extern CHAR16  *gEnterString;
extern CHAR16  *gEnterCommitString;
extern CHAR16  *gEnterEscapeString;
extern CHAR16  *gEscapeString;
extern CHAR16  *gMoveHighlight;
extern CHAR16  *gDecNumericInput;
extern CHAR16  *gHexNumericInput;
extern CHAR16  *gToggleCheckBox;
extern CHAR16  *gLibEmptyString;
extern CHAR16  *gAreYouSure;
extern CHAR16  *gYesResponse;
extern CHAR16  *gNoResponse;
extern CHAR16  *gPlusString;
extern CHAR16  *gMinusString;
extern CHAR16  *gAdjustNumber;
extern CHAR16  *gSaveChanges;
extern CHAR16  *gNvUpdateMessage;
extern CHAR16  *gInputErrorMessage;

/**

  Print banner info for front page.

  @param[in]  FormData             Form Data to be shown in Page

**/
VOID
PrintBannerInfo (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData
  );

/**
  Print framework and form title for a page.

  @param[in]  FormData             Form Data to be shown in Page
**/
VOID
PrintFramework (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData
  );

/**
  Validate the input screen diemenstion info.

  @param  FormData               The input form data info.

  @return EFI_SUCCESS            The input screen info is acceptable.
  @return EFI_INVALID_PARAMETER  The input screen info is not acceptable.

**/
EFI_STATUS
ScreenDiemensionInfoValidate (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData
  );

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
LibGetToken (
  IN  EFI_STRING_ID   Token,
  IN  EFI_HII_HANDLE  HiiHandle
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
LibGetStringWidth (
  IN CHAR16  *String
  );

/**
  Show all registered HotKey help strings on bottom Rows.

  @param FormData          The curent input form data info.
  @param SetState          Set HotKey or Clear HotKey

**/
VOID
PrintHotKeyHelpString (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData,
  IN BOOLEAN                   SetState
  );

/**
  Get step info from numeric opcode.

  @param[in] OpCode     The input numeric op code.

  @return step info for this opcode.
**/
UINT64
LibGetFieldFromNum (
  IN  EFI_IFR_OP_HEADER  *OpCode
  );

/**
  Initialize the HII String Token to the correct values.

**/
VOID
InitializeLibStrings (
  VOID
  );

/**
  Free the HII String.

**/
VOID
FreeLibStrings (
  VOID
  );

/**
  Wait for a key to be pressed by user.

  @param Key         The key which is pressed by user.

  @retval EFI_SUCCESS The function always completed successfully.

**/
EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY  *Key
  );

/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

**/
VOID
LibSetUnicodeMem (
  IN VOID    *Buffer,
  IN UINTN   Size,
  IN CHAR16  Value
  );

/**
  Prints a formatted unicode string to the default console, at
  the supplied cursor position.

  @param  Width      Width of String to be printed.
  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Fmt        Format string.
  @param  ...        Variable argument list for format string.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintAt (
  IN UINTN   Width,
  IN UINTN   Column,
  IN UINTN   Row,
  IN CHAR16  *Fmt,
  ...
  );

/**
  Process some op codes which is out side of current form.

  @param FormData                Pointer to the form data.

**/
VOID
ProcessExternedOpcode (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData
  );

#endif
