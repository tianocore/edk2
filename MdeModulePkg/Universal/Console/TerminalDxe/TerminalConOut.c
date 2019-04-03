/** @file
  Implementation for EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL protocol.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2016 Silicon Graphics, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Terminal.h"

//
// This list is used to define the valid extend chars.
// It also provides a mapping from Unicode to PCANSI or
// ASCII. The ASCII mapping we just made up.
//
//
UNICODE_TO_CHAR  UnicodeToPcAnsiOrAscii[] = {
  { BOXDRAW_HORIZONTAL,                 0xc4, L'-' },
  { BOXDRAW_VERTICAL,                   0xb3, L'|' },
  { BOXDRAW_DOWN_RIGHT,                 0xda, L'/' },
  { BOXDRAW_DOWN_LEFT,                  0xbf, L'\\' },
  { BOXDRAW_UP_RIGHT,                   0xc0, L'\\' },
  { BOXDRAW_UP_LEFT,                    0xd9, L'/' },
  { BOXDRAW_VERTICAL_RIGHT,             0xc3, L'|' },
  { BOXDRAW_VERTICAL_LEFT,              0xb4, L'|' },
  { BOXDRAW_DOWN_HORIZONTAL,            0xc2, L'+' },
  { BOXDRAW_UP_HORIZONTAL,              0xc1, L'+' },
  { BOXDRAW_VERTICAL_HORIZONTAL,        0xc5, L'+' },
  { BOXDRAW_DOUBLE_HORIZONTAL,          0xcd, L'-' },
  { BOXDRAW_DOUBLE_VERTICAL,            0xba, L'|' },
  { BOXDRAW_DOWN_RIGHT_DOUBLE,          0xd5, L'/' },
  { BOXDRAW_DOWN_DOUBLE_RIGHT,          0xd6, L'/' },
  { BOXDRAW_DOUBLE_DOWN_RIGHT,          0xc9, L'/' },
  { BOXDRAW_DOWN_LEFT_DOUBLE,           0xb8, L'\\' },
  { BOXDRAW_DOWN_DOUBLE_LEFT,           0xb7, L'\\' },
  { BOXDRAW_DOUBLE_DOWN_LEFT,           0xbb, L'\\' },
  { BOXDRAW_UP_RIGHT_DOUBLE,            0xd4, L'\\' },
  { BOXDRAW_UP_DOUBLE_RIGHT,            0xd3, L'\\' },
  { BOXDRAW_DOUBLE_UP_RIGHT,            0xc8, L'\\' },
  { BOXDRAW_UP_LEFT_DOUBLE,             0xbe, L'/' },
  { BOXDRAW_UP_DOUBLE_LEFT,             0xbd, L'/' },
  { BOXDRAW_DOUBLE_UP_LEFT,             0xbc, L'/' },
  { BOXDRAW_VERTICAL_RIGHT_DOUBLE,      0xc6, L'|' },
  { BOXDRAW_VERTICAL_DOUBLE_RIGHT,      0xc7, L'|' },
  { BOXDRAW_DOUBLE_VERTICAL_RIGHT,      0xcc, L'|' },
  { BOXDRAW_VERTICAL_LEFT_DOUBLE,       0xb5, L'|' },
  { BOXDRAW_VERTICAL_DOUBLE_LEFT,       0xb6, L'|' },
  { BOXDRAW_DOUBLE_VERTICAL_LEFT,       0xb9, L'|' },
  { BOXDRAW_DOWN_HORIZONTAL_DOUBLE,     0xd1, L'+' },
  { BOXDRAW_DOWN_DOUBLE_HORIZONTAL,     0xd2, L'+' },
  { BOXDRAW_DOUBLE_DOWN_HORIZONTAL,     0xcb, L'+' },
  { BOXDRAW_UP_HORIZONTAL_DOUBLE,       0xcf, L'+' },
  { BOXDRAW_UP_DOUBLE_HORIZONTAL,       0xd0, L'+' },
  { BOXDRAW_DOUBLE_UP_HORIZONTAL,       0xca, L'+' },
  { BOXDRAW_VERTICAL_HORIZONTAL_DOUBLE, 0xd8, L'+' },
  { BOXDRAW_VERTICAL_DOUBLE_HORIZONTAL, 0xd7, L'+' },
  { BOXDRAW_DOUBLE_VERTICAL_HORIZONTAL, 0xce, L'+' },

  { BLOCKELEMENT_FULL_BLOCK,            0xdb, L'*' },
  { BLOCKELEMENT_LIGHT_SHADE,           0xb0, L'+' },

  { GEOMETRICSHAPE_UP_TRIANGLE,         '^', L'^' },
  { GEOMETRICSHAPE_RIGHT_TRIANGLE,      '>', L'>' },
  { GEOMETRICSHAPE_DOWN_TRIANGLE,       'v', L'v' },
  { GEOMETRICSHAPE_LEFT_TRIANGLE,       '<', L'<' },

  { ARROW_LEFT,                         '<', L'<' },
  { ARROW_UP,                           '^', L'^' },
  { ARROW_RIGHT,                        '>', L'>' },
  { ARROW_DOWN,                         'v', L'v' },

  { 0x0000,                             0x00, L'\0' }
};

CHAR16 mSetModeString[]            = { ESC, '[', '=', '3', 'h', 0 };
CHAR16 mSetAttributeString[]       = { ESC, '[', '0', 'm', ESC, '[', '4', '0', 'm', ESC, '[', '4', '0', 'm', 0 };
CHAR16 mClearScreenString[]        = { ESC, '[', '2', 'J', 0 };
CHAR16 mSetCursorPositionString[]  = { ESC, '[', '0', '0', ';', '0', '0', 'H', 0 };
CHAR16 mCursorForwardString[]      = { ESC, '[', '0', '0', 'C', 0 };
CHAR16 mCursorBackwardString[]     = { ESC, '[', '0', '0', 'D', 0 };

//
// Body of the ConOut functions
//

/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.Reset().

  If ExtendeVerification is TRUE, then perform dependent serial device reset,
  and set display mode to mode 0.
  If ExtendedVerification is FALSE, only set display mode to mode 0.

  @param  This                  Indicates the calling context.
  @param  ExtendedVerification  Indicates that the driver may perform a more
                                exhaustive verification operation of the device
                                during reset.

  @retval EFI_SUCCESS           The reset operation succeeds.
  @retval EFI_DEVICE_ERROR      The terminal is not functioning correctly or the serial port reset fails.

**/
EFI_STATUS
EFIAPI
TerminalConOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS    Status;
  TERMINAL_DEV  *TerminalDevice;

  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);

  //
  // Perform a more exhaustive reset by resetting the serial port.
  //
  if (ExtendedVerification) {
    //
    // Report progress code here
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_PC_RESET),
      TerminalDevice->DevicePath
      );

    Status = TerminalDevice->SerialIo->Reset (TerminalDevice->SerialIo);
    if (EFI_ERROR (Status)) {
      //
      // Report error code here
      //
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR),
        TerminalDevice->DevicePath
        );

      return Status;
    }
  }

  This->SetAttribute (This, EFI_TEXT_ATTR (This->Mode->Attribute & 0x0F, EFI_BLACK));

  Status = This->SetMode (This, 0);

  return Status;
}


/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString().

  The Unicode string will be converted to terminal expressible data stream
  and send to terminal via serial port.

  @param  This                    Indicates the calling context.
  @param  WString                 The Null-terminated Unicode string to be displayed
                                  on the terminal screen.

  @retval EFI_SUCCESS             The string is output successfully.
  @retval EFI_DEVICE_ERROR        The serial port fails to send the string out.
  @retval EFI_WARN_UNKNOWN_GLYPH  Indicates that some of the characters in the Unicode string could not
                                  be rendered and are skipped.
  @retval EFI_UNSUPPORTED         If current display mode is out of range.

**/
EFI_STATUS
EFIAPI
TerminalConOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  TERMINAL_DEV                *TerminalDevice;
  EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
  UINTN                       MaxColumn;
  UINTN                       MaxRow;
  UINTN                       Length;
  UTF8_CHAR                   Utf8Char;
  CHAR8                       GraphicChar;
  CHAR8                       AsciiChar;
  EFI_STATUS                  Status;
  UINT8                       ValidBytes;
  CHAR8                       CrLfStr[2];
  //
  //  flag used to indicate whether condition happens which will cause
  //  return EFI_WARN_UNKNOWN_GLYPH
  //
  BOOLEAN                     Warning;

  ValidBytes  = 0;
  Warning     = FALSE;
  AsciiChar   = 0;

  //
  //  get Terminal device data structure pointer.
  //
  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);

  //
  //  Get current display mode
  //
  Mode = This->Mode;

  if (Mode->Mode >= Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  This->QueryMode (
          This,
          Mode->Mode,
          &MaxColumn,
          &MaxRow
          );

  for (; *WString != CHAR_NULL; WString++) {

    switch (TerminalDevice->TerminalType) {

    case TerminalTypePcAnsi:
    case TerminalTypeVt100:
    case TerminalTypeVt100Plus:
    case TerminalTypeTtyTerm:

      if (!TerminalIsValidTextGraphics (*WString, &GraphicChar, &AsciiChar)) {
        //
        // If it's not a graphic character convert Unicode to ASCII.
        //
        GraphicChar = (CHAR8) *WString;

        if (!(TerminalIsValidAscii (GraphicChar) || TerminalIsValidEfiCntlChar (GraphicChar))) {
          //
          // when this driver use the OutputString to output control string,
          // TerminalDevice->OutputEscChar is set to let the Esc char
          // to be output to the terminal emulation software.
          //
          if ((GraphicChar == 27) && TerminalDevice->OutputEscChar) {
            GraphicChar = 27;
          } else {
            GraphicChar = '?';
            Warning     = TRUE;
          }
        }

        AsciiChar = GraphicChar;

      }

      if (TerminalDevice->TerminalType != TerminalTypePcAnsi) {
        GraphicChar = AsciiChar;
      }

      Length = 1;

      Status = TerminalDevice->SerialIo->Write (
                                          TerminalDevice->SerialIo,
                                          &Length,
                                          &GraphicChar
                                          );

      if (EFI_ERROR (Status)) {
        goto OutputError;
      }

      break;

    case TerminalTypeVtUtf8:
      UnicodeToUtf8 (*WString, &Utf8Char, &ValidBytes);
      Length = ValidBytes;
      Status = TerminalDevice->SerialIo->Write (
                                          TerminalDevice->SerialIo,
                                          &Length,
                                          (UINT8 *) &Utf8Char
                                          );
      if (EFI_ERROR (Status)) {
        goto OutputError;
      }
      break;
    }
    //
    //  Update cursor position.
    //
    switch (*WString) {

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
      if (Mode->CursorColumn < (INT32) (MaxColumn - 1)) {

        Mode->CursorColumn++;

      } else {

        Mode->CursorColumn = 0;
        if (Mode->CursorRow < (INT32) (MaxRow - 1)) {
          Mode->CursorRow++;
        }

        if (TerminalDevice->TerminalType == TerminalTypeTtyTerm &&
            !TerminalDevice->OutputEscChar) {
          //
          // We've written the last character on the line.  The
          // terminal doesn't actually wrap its cursor until we print
          // the next character, but the driver thinks it has wrapped
          // already.  Print CR LF to synchronize the terminal with
          // the driver, but only if we're not in the middle of
          // printing an escape sequence.
          //
          CrLfStr[0] = '\r';
          CrLfStr[1] = '\n';

          Length = sizeof(CrLfStr);

          Status = TerminalDevice->SerialIo->Write (
                                                TerminalDevice->SerialIo,
                                                &Length,
                                                CrLfStr
                                                );

          if (EFI_ERROR (Status)) {
            goto OutputError;
          }
        }
      }
      break;

    };

  }

  if (Warning) {
    return EFI_WARN_UNKNOWN_GLYPH;
  }

  return EFI_SUCCESS;

OutputError:
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    (EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_OUTPUT_ERROR),
    TerminalDevice->DevicePath
    );

  return EFI_DEVICE_ERROR;
}


/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.TestString().

  If one of the characters in the *Wstring is
  neither valid Unicode drawing characters,
  not ASCII code, then this function will return
  EFI_UNSUPPORTED.

  @param  This              Indicates the calling context.
  @param  WString           The Null-terminated Unicode string to be tested.

  @retval EFI_SUCCESS       The terminal is capable of rendering the output string.
  @retval EFI_UNSUPPORTED   Some of the characters in the Unicode string cannot be rendered.

**/
EFI_STATUS
EFIAPI
TerminalConOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  TERMINAL_DEV  *TerminalDevice;
  EFI_STATUS    Status;

  //
  //  get Terminal device data structure pointer.
  //
  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);

  switch (TerminalDevice->TerminalType) {

  case TerminalTypePcAnsi:
  case TerminalTypeVt100:
  case TerminalTypeVt100Plus:
  case TerminalTypeTtyTerm:
    Status = AnsiTestString (TerminalDevice, WString);
    break;

  case TerminalTypeVtUtf8:
    Status = VTUTF8TestString (TerminalDevice, WString);
    break;

  default:
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status;
}


/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.QueryMode().

  It returns information for an available text mode
  that the terminal supports.

  @param This        Indicates the calling context.
  @param ModeNumber  The mode number to return information on.
  @param Columns     The returned columns of the requested mode.
  @param Rows        The returned rows of the requested mode.

  @retval EFI_SUCCESS       The requested mode information is returned.
  @retval EFI_UNSUPPORTED   The mode number is not valid.

**/
EFI_STATUS
EFIAPI
TerminalConOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
{
  TERMINAL_DEV  *TerminalDevice;

  if (ModeNumber >= (UINTN) This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get Terminal device data structure pointer.
  //
  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);
  *Columns = TerminalDevice->TerminalConsoleModeData[ModeNumber].Columns;
  *Rows    = TerminalDevice->TerminalConsoleModeData[ModeNumber].Rows;

  return EFI_SUCCESS;
}


/**
  Implements EFI_SIMPLE_TEXT_OUT.SetMode().

  Set the terminal to a specified display mode.
  In this driver, we only support mode 0.

  @param This          Indicates the calling context.
  @param ModeNumber    The text mode to set.

  @retval EFI_SUCCESS       The requested text mode is set.
  @retval EFI_DEVICE_ERROR  The requested text mode cannot be set
                            because of serial device error.
  @retval EFI_UNSUPPORTED   The text mode number is not valid.

**/
EFI_STATUS
EFIAPI
TerminalConOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  )
{
  EFI_STATUS    Status;
  TERMINAL_DEV  *TerminalDevice;

  //
  //  get Terminal device data structure pointer.
  //
  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);

  if (ModeNumber >= (UINTN) This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  //
  // Set the current mode
  //
  This->Mode->Mode = (INT32) ModeNumber;

  This->ClearScreen (This);

  TerminalDevice->OutputEscChar = TRUE;
  Status                        = This->OutputString (This, mSetModeString);
  TerminalDevice->OutputEscChar = FALSE;

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  This->Mode->Mode  = (INT32) ModeNumber;

  Status            = This->ClearScreen (This);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;

}


/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.SetAttribute().

  @param This        Indicates the calling context.
  @param Attribute   The attribute to set. Only bit0..6 are valid, all other bits
                     are undefined and must be zero.

  @retval EFI_SUCCESS        The requested attribute is set.
  @retval EFI_DEVICE_ERROR   The requested attribute cannot be set due to serial port error.
  @retval EFI_UNSUPPORTED    The attribute requested is not defined by EFI spec.

**/
EFI_STATUS
EFIAPI
TerminalConOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  )
{
  UINT8         ForegroundControl;
  UINT8         BackgroundControl;
  UINT8         BrightControl;
  INT32         SavedColumn;
  INT32         SavedRow;
  EFI_STATUS    Status;
  TERMINAL_DEV  *TerminalDevice;

  SavedColumn = 0;
  SavedRow    = 0;

  //
  //  get Terminal device data structure pointer.
  //
  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);

  //
  //  only the bit0..6 of the Attribute is valid
  //
  if ((Attribute | 0x7f) != 0x7f) {
    return EFI_UNSUPPORTED;
  }

  //
  // Skip outputting the command string for the same attribute
  // It improves the terminal performance significantly
  //
  if (This->Mode->Attribute == (INT32) Attribute) {
    return EFI_SUCCESS;
  }

  //
  //  convert Attribute value to terminal emulator
  //  understandable foreground color
  //
  switch (Attribute & 0x07) {

  case EFI_BLACK:
    ForegroundControl = 30;
    break;

  case EFI_BLUE:
    ForegroundControl = 34;
    break;

  case EFI_GREEN:
    ForegroundControl = 32;
    break;

  case EFI_CYAN:
    ForegroundControl = 36;
    break;

  case EFI_RED:
    ForegroundControl = 31;
    break;

  case EFI_MAGENTA:
    ForegroundControl = 35;
    break;

  case EFI_BROWN:
    ForegroundControl = 33;
    break;

  default:

  case EFI_LIGHTGRAY:
    ForegroundControl = 37;
    break;

  }
  //
  //  bit4 of the Attribute indicates bright control
  //  of terminal emulator.
  //
  BrightControl = (UINT8) ((Attribute >> 3) & 1);

  //
  //  convert Attribute value to terminal emulator
  //  understandable background color.
  //
  switch ((Attribute >> 4) & 0x07) {

  case EFI_BLACK:
    BackgroundControl = 40;
    break;

  case EFI_BLUE:
    BackgroundControl = 44;
    break;

  case EFI_GREEN:
    BackgroundControl = 42;
    break;

  case EFI_CYAN:
    BackgroundControl = 46;
    break;

  case EFI_RED:
    BackgroundControl = 41;
    break;

  case EFI_MAGENTA:
    BackgroundControl = 45;
    break;

  case EFI_BROWN:
    BackgroundControl = 43;
    break;

  default:

  case EFI_LIGHTGRAY:
    BackgroundControl = 47;
    break;
  }
  //
  // terminal emulator's control sequence to set attributes
  //
  mSetAttributeString[BRIGHT_CONTROL_OFFSET]          = (CHAR16) ('0' + BrightControl);
  mSetAttributeString[FOREGROUND_CONTROL_OFFSET + 0]  = (CHAR16) ('0' + (ForegroundControl / 10));
  mSetAttributeString[FOREGROUND_CONTROL_OFFSET + 1]  = (CHAR16) ('0' + (ForegroundControl % 10));
  mSetAttributeString[BACKGROUND_CONTROL_OFFSET + 0]  = (CHAR16) ('0' + (BackgroundControl / 10));
  mSetAttributeString[BACKGROUND_CONTROL_OFFSET + 1]  = (CHAR16) ('0' + (BackgroundControl % 10));

  //
  // save current column and row
  // for future scrolling back use.
  //
  SavedColumn                   = This->Mode->CursorColumn;
  SavedRow                      = This->Mode->CursorRow;

  TerminalDevice->OutputEscChar = TRUE;
  Status                        = This->OutputString (This, mSetAttributeString);
  TerminalDevice->OutputEscChar = FALSE;

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  //  scroll back to saved cursor position.
  //
  This->Mode->CursorColumn  = SavedColumn;
  This->Mode->CursorRow     = SavedRow;

  This->Mode->Attribute     = (INT32) Attribute;

  return EFI_SUCCESS;

}


/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.ClearScreen().
  It clears the ANSI terminal's display to the
  currently selected background color.

  @param This     Indicates the calling context.

  @retval EFI_SUCCESS       The operation completed successfully.
  @retval EFI_DEVICE_ERROR  The terminal screen cannot be cleared due to serial port error.
  @retval EFI_UNSUPPORTED   The terminal is not in a valid display mode.

**/
EFI_STATUS
EFIAPI
TerminalConOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  EFI_STATUS    Status;
  TERMINAL_DEV  *TerminalDevice;

  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);

  //
  //  control sequence for clear screen request
  //
  TerminalDevice->OutputEscChar = TRUE;
  Status                        = This->OutputString (This, mClearScreenString);
  TerminalDevice->OutputEscChar = FALSE;

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = This->SetCursorPosition (This, 0, 0);

  return Status;
}


/**
  Implements EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.SetCursorPosition().

  @param This      Indicates the calling context.
  @param Column    The row to set cursor to.
  @param Row       The column to set cursor to.

  @retval EFI_SUCCESS       The operation completed successfully.
  @retval EFI_DEVICE_ERROR  The request fails due to serial port error.
  @retval EFI_UNSUPPORTED   The terminal is not in a valid text mode, or the cursor position
                            is invalid for current mode.

**/
EFI_STATUS
EFIAPI
TerminalConOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
  UINTN                       MaxColumn;
  UINTN                       MaxRow;
  EFI_STATUS                  Status;
  TERMINAL_DEV                *TerminalDevice;
  CHAR16                      *String;

  TerminalDevice = TERMINAL_CON_OUT_DEV_FROM_THIS (This);

  //
  //  get current mode
  //
  Mode = This->Mode;

  //
  //  get geometry of current mode
  //
  Status = This->QueryMode (
                  This,
                  Mode->Mode,
                  &MaxColumn,
                  &MaxRow
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (Column >= MaxColumn || Row >= MaxRow) {
    return EFI_UNSUPPORTED;
  }
  //
  // control sequence to move the cursor
  //
  // Optimize cursor motion control sequences for TtyTerm.  Move
  // within the current line if possible, and don't output anyting if
  // it isn't necessary.
  //
  if (TerminalDevice->TerminalType == TerminalTypeTtyTerm &&
      (UINTN)Mode->CursorRow == Row) {
    if ((UINTN)Mode->CursorColumn > Column) {
      mCursorBackwardString[FW_BACK_OFFSET + 0] = (CHAR16) ('0' + ((Mode->CursorColumn - Column) / 10));
      mCursorBackwardString[FW_BACK_OFFSET + 1] = (CHAR16) ('0' + ((Mode->CursorColumn - Column) % 10));
      String = mCursorBackwardString;
    }
    else if (Column > (UINTN)Mode->CursorColumn) {
      mCursorForwardString[FW_BACK_OFFSET + 0] = (CHAR16) ('0' + ((Column - Mode->CursorColumn) / 10));
      mCursorForwardString[FW_BACK_OFFSET + 1] = (CHAR16) ('0' + ((Column - Mode->CursorColumn) % 10));
      String = mCursorForwardString;
    }
    else {
      String = L"";  // No cursor motion necessary
    }
  }
  else {
    mSetCursorPositionString[ROW_OFFSET + 0]    = (CHAR16) ('0' + ((Row + 1) / 10));
    mSetCursorPositionString[ROW_OFFSET + 1]    = (CHAR16) ('0' + ((Row + 1) % 10));
    mSetCursorPositionString[COLUMN_OFFSET + 0] = (CHAR16) ('0' + ((Column + 1) / 10));
    mSetCursorPositionString[COLUMN_OFFSET + 1] = (CHAR16) ('0' + ((Column + 1) % 10));
    String = mSetCursorPositionString;
  }

  TerminalDevice->OutputEscChar               = TRUE;
  Status = This->OutputString (This, String);
  TerminalDevice->OutputEscChar = FALSE;

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  //  update current cursor position
  //  in the Mode data structure.
  //
  Mode->CursorColumn  = (INT32) Column;
  Mode->CursorRow     = (INT32) Row;

  return EFI_SUCCESS;
}


/**
  Implements SIMPLE_TEXT_OUTPUT.EnableCursor().

  In this driver, the cursor cannot be hidden.

  @param This      Indicates the calling context.
  @param Visible   If TRUE, the cursor is set to be visible,
                   If FALSE, the cursor is set to be invisible.

  @retval EFI_SUCCESS      The request is valid.
  @retval EFI_UNSUPPORTED  The terminal does not support cursor hidden.

**/
EFI_STATUS
EFIAPI
TerminalConOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  )
{
  if (!Visible) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


/**
  Detects if a Unicode char is for Box Drawing text graphics.

  @param  Graphic      Unicode char to test.
  @param  PcAnsi       Optional pointer to return PCANSI equivalent of
                       Graphic.
  @param  Ascii        Optional pointer to return ASCII equivalent of
                       Graphic.

  @retval TRUE         If Graphic is a supported Unicode Box Drawing character.

**/
BOOLEAN
TerminalIsValidTextGraphics (
  IN  CHAR16  Graphic,
  OUT CHAR8   *PcAnsi, OPTIONAL
  OUT CHAR8   *Ascii OPTIONAL
  )
{
  UNICODE_TO_CHAR *Table;

  if ((((Graphic & 0xff00) != 0x2500) && ((Graphic & 0xff00) != 0x2100))) {
    //
    // Unicode drawing code charts are all in the 0x25xx range,
    //  arrows are 0x21xx
    //
    return FALSE;
  }

  for (Table = UnicodeToPcAnsiOrAscii; Table->Unicode != 0x0000; Table++) {
    if (Graphic == Table->Unicode) {
      if (PcAnsi != NULL) {
        *PcAnsi = Table->PcAnsi;
      }

      if (Ascii != NULL) {
        *Ascii = Table->Ascii;
      }

      return TRUE;
    }
  }

  return FALSE;
}

/**
  Detects if a valid ASCII char.

  @param  Ascii        An ASCII character.

  @retval TRUE         If it is a valid ASCII character.
  @retval FALSE        If it is not a valid ASCII character.

**/
BOOLEAN
TerminalIsValidAscii (
  IN  CHAR16  Ascii
  )
{
  //
  // valid ascii code lies in the extent of 0x20 ~ 0x7f
  //
  if ((Ascii >= 0x20) && (Ascii <= 0x7f)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Detects if a valid EFI control character.

  @param  CharC        An input EFI Control character.

  @retval TRUE         If it is a valid EFI control character.
  @retval FALSE        If it is not a valid EFI control character.

**/
BOOLEAN
TerminalIsValidEfiCntlChar (
  IN  CHAR16  CharC
  )
{
  //
  // only support four control characters.
  //
  if (CharC == CHAR_NULL ||
      CharC == CHAR_BACKSPACE ||
      CharC == CHAR_LINEFEED ||
      CharC == CHAR_CARRIAGE_RETURN ||
      CharC == CHAR_TAB
      ) {
    return TRUE;
  }

  return FALSE;
}
