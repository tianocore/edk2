/** @file
  Simple Console that sits on a SerialLib.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  Symbols used in table below
===========================
  ESC = 0x1B
  CSI = 0x9B
  DEL = 0x7f
  ^   = CTRL

+=========+======+===========+==========+==========+
|         | EFI  | UEFI 2.0  |          |          |
|         | Scan |           |  VT100+  |          |
|   KEY   | Code |  PC ANSI  |  VTUTF8  |   VT100  |
+=========+======+===========+==========+==========+
| NULL    | 0x00 |           |          |          |
| UP      | 0x01 | ESC [ A   | ESC [ A  | ESC [ A  |
| DOWN    | 0x02 | ESC [ B   | ESC [ B  | ESC [ B  |
| RIGHT   | 0x03 | ESC [ C   | ESC [ C  | ESC [ C  |
| LEFT    | 0x04 | ESC [ D   | ESC [ D  | ESC [ D  |
| HOME    | 0x05 | ESC [ H   | ESC h    | ESC [ H  |
| END     | 0x06 | ESC [ F   | ESC k    | ESC [ K  |
| INSERT  | 0x07 | ESC [ @   | ESC +    | ESC [ @  |
|         |      | ESC [ L   |          | ESC [ L  |
| DELETE  | 0x08 | ESC [ X   | ESC -    | ESC [ P  |
| PG UP   | 0x09 | ESC [ I   | ESC ?    | ESC [ V  |
|         |      |           |          | ESC [ ?  |
| PG DOWN | 0x0A | ESC [ G   | ESC /    | ESC [ U  |
|         |      |           |          | ESC [ /  |
| F1      | 0x0B | ESC [ M   | ESC 1    | ESC O P  |
| F2      | 0x0C | ESC [ N   | ESC 2    | ESC O Q  |
| F3      | 0x0D | ESC [ O   | ESC 3    | ESC O w  |
| F4      | 0x0E | ESC [ P   | ESC 4    | ESC O x  |
| F5      | 0x0F | ESC [ Q   | ESC 5    | ESC O t  |
| F6      | 0x10 | ESC [ R   | ESC 6    | ESC O u  |
| F7      | 0x11 | ESC [ S   | ESC 7    | ESC O q  |
| F8      | 0x12 | ESC [ T   | ESC 8    | ESC O r  |
| F9      | 0x13 | ESC [ U   | ESC 9    | ESC O p  |
| F10     | 0x14 | ESC [ V   | ESC 0    | ESC O M  |
| Escape  | 0x17 | ESC       | ESC      | ESC      |
| F11     | 0x15 |           | ESC !    |          |
| F12     | 0x16 |           | ESC @    |          |
+=========+======+===========+==========+==========+

*/

#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>

#include <Protocol/SerialIo.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/DevicePath.h>


#define MODE0_COLUMN_COUNT        80
#define MODE0_ROW_COUNT           25


EFI_STATUS
EFIAPI
TextInReset(
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
  IN BOOLEAN                        ExtendedVerification
  );


EFI_STATUS
EFIAPI
ReadKeyStroke(
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
  OUT EFI_INPUT_KEY                 *Key
  );


EFI_STATUS
EFIAPI
TextOutReset(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN BOOLEAN                          ExtendedVerification
  );

CHAR8 *
EFIAPI
SafeUnicodeStrToAsciiStr (
  IN      CONST CHAR16                *Source,
  OUT     CHAR8                       *Destination
  );

EFI_STATUS
EFIAPI
OutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  );


EFI_STATUS
EFIAPI
TestString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  );


EFI_STATUS
EFIAPI
QueryMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            ModeNumber,
  OUT UINTN                           *Columns,
  OUT UINTN                           *Rows
  );


EFI_STATUS
EFIAPI
SetMode(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            ModeNumber
  );


EFI_STATUS
EFIAPI
SetAttribute(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            Attribute
  );


EFI_STATUS
EFIAPI
ClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  );


EFI_STATUS
EFIAPI
SetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            Column,
  IN UINTN                            Row
  );


EFI_STATUS
EFIAPI
EnableCursor (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN BOOLEAN                          Enable
  );


 EFI_SIMPLE_TEXT_INPUT_PROTOCOL mSimpleTextIn = {
  TextInReset,
  ReadKeyStroke,
  NULL
};

 EFI_SIMPLE_TEXT_OUTPUT_MODE mSimpleTextOutMode = {
  1,
  0,
  EFI_TEXT_ATTR( EFI_LIGHTGRAY, EFI_BLACK ),
  0,
  0,
  TRUE
};

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL mSimpleTextOut = {
  TextOutReset,
  OutputString,
  TestString,
  QueryMode,
  SetMode,
  SetAttribute,
  ClearScreen,
  SetCursorPosition,
  EnableCursor,
  &mSimpleTextOutMode
};

EFI_HANDLE           mInstallHandle = NULL;

typedef struct {
  VENDOR_DEVICE_PATH        Guid;
  UART_DEVICE_PATH          Uart;
  EFI_DEVICE_PATH_PROTOCOL  End;
} SIMPLE_TEXT_OUT_DEVICE_PATH;

SIMPLE_TEXT_OUT_DEVICE_PATH mDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH), 0} },
    EFI_CALLER_ID_GUID
  },
  {
    { MESSAGING_DEVICE_PATH, MSG_UART_DP, { sizeof (UART_DEVICE_PATH), 0} },
    0,        // Reserved
    FixedPcdGet64 (PcdUartDefaultBaudRate),   // BaudRate
    FixedPcdGet8 (PcdUartDefaultDataBits),    // DataBits
    FixedPcdGet8 (PcdUartDefaultParity),      // Parity (N)
    FixedPcdGet8 (PcdUartDefaultStopBits)     // StopBits
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0} }
};




BOOLEAN
TextOutIsValidAscii (
  IN CHAR16       Ascii
  )
{
  //
  // valid ASCII code lies in the extent of 0x20 - 0x7F
  //
  if ((Ascii >= 0x20) && (Ascii <= 0x7F)) {
    return TRUE;
  }

  return FALSE;
}


BOOLEAN
TextOutIsValidEfiCntlChar (
  IN CHAR16       Char
  )
{
  //
  // only support four control characters.
  //
  if (Char == CHAR_NULL ||
      Char == CHAR_BACKSPACE ||
      Char == CHAR_LINEFEED ||
      Char == CHAR_CARRIAGE_RETURN ||
      Char == CHAR_TAB ) {
    return TRUE;
  }

  return FALSE;
}


VOID
EFIAPI
WaitForKeyEvent (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  )
{
  if (SerialPortPoll ())  {
    gBS->SignalEvent (Event);
  }
}


EFI_STATUS
EFIAPI
TextInReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
ReadKeyStroke (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
  OUT EFI_INPUT_KEY                 *Key
  )
{
  CHAR8             Char;

  if (!SerialPortPoll ()) {
    return EFI_NOT_READY;
  }

  SerialPortRead ((UINT8 *)&Char, 1);

  //
  // Check for ESC sequence. This code is not techincally correct VT100 code.
  // An illegal ESC sequence represents an ESC and the characters that follow.
  // This code will eat one or two chars after an escape. This is done to
  // prevent some complex FIFOing of the data. It is good enough to get
  // the arrow and delete keys working
  //
  Key->UnicodeChar = 0;
  Key->ScanCode    = SCAN_NULL;
  if (Char == 0x1b) {
    SerialPortRead ((UINT8 *)&Char, 1);
    if (Char == '[') {
      SerialPortRead ((UINT8 *)&Char, 1);
      switch (Char) {
      case 'A':
        Key->ScanCode = SCAN_UP;
        break;
      case 'B':
        Key->ScanCode = SCAN_DOWN;
        break;
      case 'C':
        Key->ScanCode = SCAN_RIGHT;
        break;
      case 'D':
        Key->ScanCode = SCAN_LEFT;
        break;
      case 'H':
        Key->ScanCode = SCAN_HOME;
        break;
      case 'K':
      case 'F': // PC ANSI
        Key->ScanCode = SCAN_END;
        break;
      case '@':
      case 'L':
        Key->ScanCode = SCAN_INSERT;
        break;
      case 'P':
      case 'X': // PC ANSI
        Key->ScanCode = SCAN_DELETE;
        break;
      case 'U':
      case '/':
      case 'G': // PC ANSI
        Key->ScanCode = SCAN_PAGE_DOWN;
        break;
      case 'V':
      case '?':
      case 'I': // PC ANSI
        Key->ScanCode = SCAN_PAGE_UP;
        break;

      // PCANSI that does not conflict with VT100
      case 'M':
        Key->ScanCode = SCAN_F1;
        break;
      case 'N':
        Key->ScanCode = SCAN_F2;
        break;
      case 'O':
        Key->ScanCode = SCAN_F3;
        break;
      case 'Q':
        Key->ScanCode = SCAN_F5;
        break;
      case 'R':
        Key->ScanCode = SCAN_F6;
        break;
      case 'S':
        Key->ScanCode = SCAN_F7;
        break;
      case 'T':
        Key->ScanCode = SCAN_F8;
        break;

      default:
        Key->UnicodeChar = Char;
        break;
      }
    } else if (Char == '0') {
      SerialPortRead ((UINT8 *)&Char, 1);
      switch (Char) {
      case 'P':
        Key->ScanCode = SCAN_F1;
        break;
      case 'Q':
        Key->ScanCode = SCAN_F2;
        break;
      case 'w':
        Key->ScanCode = SCAN_F3;
        break;
      case 'x':
        Key->ScanCode = SCAN_F4;
        break;
      case 't':
        Key->ScanCode = SCAN_F5;
        break;
      case 'u':
        Key->ScanCode = SCAN_F6;
        break;
      case 'q':
        Key->ScanCode = SCAN_F7;
        break;
      case 'r':
        Key->ScanCode = SCAN_F8;
        break;
      case 'p':
        Key->ScanCode = SCAN_F9;
        break;
      case 'm':
        Key->ScanCode = SCAN_F10;
        break;
      default :
        break;
      }
    }
  } else if (Char < ' ') {
    if ((Char == CHAR_BACKSPACE) ||
        (Char == CHAR_TAB)       ||
        (Char == CHAR_LINEFEED)  ||
        (Char == CHAR_CARRIAGE_RETURN)) {
      // Only let through EFI required control characters
      Key->UnicodeChar = (CHAR16)Char;
    }
  } else if (Char == 0x7f) {
    Key->ScanCode = SCAN_DELETE;
  } else {
    Key->UnicodeChar = (CHAR16)Char;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
TextOutReset (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS            Status;

  This->SetAttribute(
        This,
        EFI_TEXT_ATTR(This->Mode->Attribute & 0x0F, EFI_BACKGROUND_BLACK)
        );

  Status = This->SetMode (This, 0);

  return Status;
}

CHAR8 *
EFIAPI
SafeUnicodeStrToAsciiStr (
  IN      CONST CHAR16                *Source,
  OUT     CHAR8                       *Destination
  )
{
  CHAR8                               *ReturnValue;

  ASSERT (Destination != NULL);

  //
  // ASSERT if Source is long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (Source) != 0);

  //
  // Source and Destination should not overlap
  //
  ASSERT ((UINTN) ((CHAR16 *) Destination -  Source) > StrLen (Source));
  ASSERT ((UINTN) ((CHAR8 *) Source - Destination) > StrLen (Source));


  ReturnValue = Destination;
  while (*Source != '\0') {
    //
    // If any non-ascii characters in Source then replace it with '?'.
    //
    if (*Source < 0x80) {
      *Destination = (CHAR8) *Source;
    } else {
      *Destination = '?';

      //Surrogate pair check.
      if ((*Source >= 0xD800) && (*Source <= 0xDFFF)) {
        Source++;
      }
    }

    Destination++;
    Source++;
  }

  *Destination = '\0';

  //
  // ASSERT Original Destination is less long than PcdMaximumAsciiStringLength.
  // Length tests are performed inside AsciiStrLen().
  //
  ASSERT (AsciiStrSize (ReturnValue) != 0);

  return ReturnValue;
}

EFI_STATUS
EFIAPI
OutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  )
{
  UINTN                       Size;
  CHAR8*                      OutputString;
  EFI_STATUS                  Status;
  EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
  UINTN                       MaxColumn;
  UINTN                       MaxRow;

  Size = StrLen(String) + 1;
  OutputString = AllocatePool(Size);

  //If there is any non-ascii characters in String buffer then replace it with '?'
  //Eventually, UnicodeStrToAsciiStr API should be fixed.
  SafeUnicodeStrToAsciiStr(String, OutputString);
  SerialPortWrite ((UINT8 *)OutputString, Size - 1);

  //
  // Parse each character of the string to output
  // to update the cursor position information
  //
  Mode = This->Mode;

  Status = This->QueryMode (
                   This,
                   Mode->Mode,
                   &MaxColumn,
                   &MaxRow
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (; *String != CHAR_NULL; String++) {

    switch (*String) {
    case CHAR_BACKSPACE:
      if (Mode->CursorColumn > 0) {
        Mode->CursorColumn--;
      }
      break;

    case CHAR_LINEFEED:
      if (Mode->CursorRow < (INT32) (MaxRow - 1)) {
        Mode->CursorRow++;
      }
      break;

    case CHAR_CARRIAGE_RETURN:
      Mode->CursorColumn = 0;
      break;

    default:
      if (Mode->CursorColumn >= (INT32) (MaxColumn - 1)) {
        // Move the cursor as if we print CHAR_CARRIAGE_RETURN & CHAR_LINE_FEED
        // CHAR_LINEFEED
        if (Mode->CursorRow < (INT32) (MaxRow - 1)) {
          Mode->CursorRow++;
        }
        // CHAR_CARIAGE_RETURN
        Mode->CursorColumn = 0;
      } else {
        Mode->CursorColumn++;
      }
      break;
    }
  }

  FreePool(OutputString);

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
TestString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN CHAR16                           *String
  )
{
  CHAR8           Character;

  for ( ; *String != CHAR_NULL; String++) {
    Character = (CHAR8)*String;
    if (!(TextOutIsValidAscii (Character) || TextOutIsValidEfiCntlChar (Character))) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
QueryMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN UINTN                            ModeNumber,
  OUT UINTN                          *Columns,
  OUT UINTN                          *Rows
  )
{
  if (This->Mode->MaxMode > 1) {
    return EFI_DEVICE_ERROR;
  }

  if (ModeNumber == 0) {
    *Columns  = MODE0_COLUMN_COUNT;
    *Rows     = MODE0_ROW_COUNT;
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
SetMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *This,
  IN UINTN                              ModeNumber
  )
{
  if (ModeNumber != 0) {
    return EFI_UNSUPPORTED;
  }

  This->Mode->Mode = 0;
  This->ClearScreen (This);
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
SetAttribute(
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *This,
  IN UINTN                              Attribute
  )
{
  This->Mode->Attribute = (INT32)Attribute;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
ClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *This
  )
{
  EFI_STATUS    Status;

  Status = This->SetCursorPosition (This, 0, 0);
  return Status;
}


EFI_STATUS
EFIAPI
SetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *This,
  IN UINTN                              Column,
  IN UINTN                              Row
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_MODE       *Mode;
  EFI_STATUS                        Status;
  UINTN                             MaxColumn;
  UINTN                             MaxRow;

  Mode = This->Mode;

  Status = This->QueryMode(
                  This,
                  Mode->Mode,
                  &MaxColumn,
                  &MaxRow
                  );
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }

  if ((Column >= MaxColumn) || (Row >= MaxRow)) {
    return EFI_UNSUPPORTED;
  }

  Mode->CursorColumn = (INT32)Column;
  Mode->CursorRow = (INT32)Row;

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
EnableCursor (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN BOOLEAN                          Enable
  )
{
  if (!Enable) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
SimpleTextInOutEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  WaitForKeyEvent,
                  NULL,
                  &mSimpleTextIn.WaitForKey
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces(
                  &mInstallHandle,
                  &gEfiSimpleTextInProtocolGuid,   &mSimpleTextIn,
                  &gEfiSimpleTextOutProtocolGuid,  &mSimpleTextOut,
                  &gEfiDevicePathProtocolGuid,     &mDevicePath,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    gST->ConOut = &mSimpleTextOut;
    gST->ConIn = &mSimpleTextIn;
  }

  return Status;
}
