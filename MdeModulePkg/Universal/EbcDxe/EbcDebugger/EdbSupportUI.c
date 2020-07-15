/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Edb.h"

/**
  Set the current coordinates of the cursor position.

  @param  ConOut        Point to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
  @param  Column        The position to set the cursor to.
  @param  Row           The position to set the cursor to.
  @param  LineLength    Length of a line.
  @param  TotalRow      Total row of a screen.
  @param  Str           Point to the string.
  @param  StrPos        The position of the string.
  @param  Len           The length of the string.

**/
VOID
EFIAPI
SetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut,
  IN  UINTN                           Column,
  IN  INTN                            Row,
  IN  UINTN                           LineLength,
  IN  UINTN                           TotalRow,
  IN  CHAR16                          *Str,
  IN  UINTN                           StrPos,
  IN  UINTN                           Len
  );

/**

  Function waits for a given event to fire, or for an optional timeout to expire.

  @param  Event            - The event to wait for
  @param  Timeout          - An optional timeout value in 100 ns units.

  @retval EFI_SUCCESS       - Event fired before Timeout expired.
  @retval EFI_TIME_OUT     - Timout expired before Event fired..

**/
EFI_STATUS
EFIAPI
WaitForSingleEvent (
  IN EFI_EVENT                  Event,
  IN UINT64                     Timeout OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
            TimerEvent,
            TimerRelative,
            Timeout
            );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (!EFI_ERROR (Status) && Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**

  Move the cursor position one character backward.

  @param  LineLength       Length of a line. Get it by calling QueryMode
  @param  Column           Current column of the cursor position
  @param  Row              Current row of the cursor position

**/
VOID
EFIAPI
ConMoveCursorBackward (
  IN     UINTN                   LineLength,
  IN OUT UINTN                   *Column,
  IN OUT UINTN                   *Row
  )
{
  ASSERT (Column != NULL);
  ASSERT (Row != NULL);
  //
  // If current column is 0, move to the last column of the previous line,
  // otherwise, just decrement column.
  //
  if (*Column == 0) {
    (*Column) = LineLength - 1;
    //
    //   if (*Row > 0) {
    //
    (*Row)--;
    //
    // }
    //
  } else {
    (*Column)--;
  }
}

/**

  Move the cursor position one character backward.

  @param  LineLength       Length of a line. Get it by calling QueryMode
  @param  TotalRow         Total row of a screen, get by calling QueryMode
  @param  Column           Current column of the cursor position
  @param  Row              Current row of the cursor position

**/
VOID
EFIAPI
ConMoveCursorForward (
  IN     UINTN                   LineLength,
  IN     UINTN                   TotalRow,
  IN OUT UINTN                   *Column,
  IN OUT UINTN                   *Row
  )
{
  ASSERT (Column != NULL);
  ASSERT (Row != NULL);
  //
  // If current column is at line end, move to the first column of the nest
  // line, otherwise, just increment column.
  //
  (*Column)++;
  if (*Column >= LineLength) {
    (*Column) = 0;
    if ((*Row) < TotalRow - 1) {
      (*Row)++;
    }
  }
}

CHAR16 mBackupSpace[EFI_DEBUG_INPUS_BUFFER_SIZE];
CHAR16 mInputBufferHistory[EFI_DEBUG_INPUS_BUFFER_SIZE];

/**

  Get user input.

  @param  Prompt       The prompt string.
  @param  InStr        Point to the input string.
  @param  StrLength    The max length of string user can input.

**/
VOID
EFIAPI
Input (
  IN CHAR16    *Prompt OPTIONAL,
  OUT CHAR16   *InStr,
  IN UINTN     StrLength
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     *ConOut;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL      *ConIn;
  BOOLEAN       Done;
  UINTN         Column;
  UINTN         Row;
  UINTN         StartColumn;
  UINTN         Update;
  UINTN         Delete;
  UINTN         Len;
  UINTN         StrPos;
  UINTN         Index;
  UINTN         LineLength;
  UINTN         TotalRow;
  UINTN         SkipLength;
  UINTN         OutputLength;
  UINTN         TailRow;
  UINTN         TailColumn;
  EFI_INPUT_KEY Key;
  BOOLEAN       InsertMode;
  BOOLEAN       NeedAdjust;
  UINTN         SubIndex;
  CHAR16        *CommandStr;

  ConOut = gST->ConOut;
  ConIn = gST->ConIn;

  ASSERT (ConOut != NULL);
  ASSERT (ConIn != NULL);
  ASSERT (InStr != NULL);

  if (Prompt != NULL) {
    ConOut->OutputString (ConOut, Prompt);
  }
  //
  // Read a line from the console
  //
  Len           = 0;
  StrPos        = 0;
  OutputLength  = 0;
  Update        = 0;
  Delete        = 0;
  InsertMode    = TRUE;
  NeedAdjust    = FALSE;

  //
  // If buffer is not large enough to hold a CHAR16, do nothing.
  //
  if (StrLength < 1) {
    return ;
  }
  //
  // Get the screen setting and the current cursor location
  //
  StartColumn = ConOut->Mode->CursorColumn;
  Column      = StartColumn;
  Row         = ConOut->Mode->CursorRow;
  ConOut->QueryMode (ConOut, ConOut->Mode->Mode, &LineLength, &TotalRow);
  if (LineLength == 0) {
    return ;
  }

  SetMem (InStr, StrLength * sizeof (CHAR16), 0);
  Done = FALSE;
  do {
    //
    // Read a key
    //
    WaitForSingleEvent (ConIn->WaitForKey, 0);
    ConIn->ReadKeyStroke (ConIn, &Key);

    switch (Key.UnicodeChar) {
    case CHAR_CARRIAGE_RETURN:
      //
      // All done, print a newline at the end of the string
      //
      TailRow     = Row + (Len - StrPos + Column) / LineLength;
      TailColumn  = (Len - StrPos + Column) % LineLength;
      Done        = TRUE;
      break;

    case CHAR_BACKSPACE:
      if (StrPos != 0) {
        //
        // If not move back beyond string beginning, move all characters behind
        // the current position one character forward
        //
        StrPos -= 1;
        Update  = StrPos;
        Delete  = 1;
        CopyMem (InStr + StrPos, InStr + StrPos + 1, sizeof (CHAR16) * (Len - StrPos));

        //
        // Adjust the current column and row
        //
        ConMoveCursorBackward (LineLength, &Column, &Row);

        NeedAdjust = TRUE;
      }
      break;

    default:
      if (Key.UnicodeChar >= ' ') {
        //
        // If we are at the buffer's end, drop the key
        //
        if (Len == StrLength - 1 && (InsertMode || StrPos == Len)) {
          break;
        }
        //
        // If in insert mode, move all characters behind the current position
        // one character backward to make space for this character. Then store
        // the character.
        //
        if (InsertMode) {
          for (Index = Len; Index > StrPos; Index -= 1) {
            InStr[Index] = InStr[Index - 1];
          }
        }

        InStr[StrPos] = Key.UnicodeChar;
        Update        = StrPos;

        StrPos += 1;
        OutputLength = 1;
      }
      break;

    case 0:
      switch (Key.ScanCode) {
      case SCAN_DELETE:
        //
        // Move characters behind current position one character forward
        //
        if (Len != 0) {
          Update  = StrPos;
          Delete  = 1;
          CopyMem (InStr + StrPos, InStr + StrPos + 1, sizeof (CHAR16) * (Len - StrPos));

          NeedAdjust = TRUE;
        }
        break;

      case SCAN_LEFT:
        //
        // Adjust current cursor position
        //
        if (StrPos != 0) {
          StrPos -= 1;
          ConMoveCursorBackward (LineLength, &Column, &Row);
        }
        break;

      case SCAN_RIGHT:
        //
        // Adjust current cursor position
        //
        if (StrPos < Len) {
          StrPos += 1;
          ConMoveCursorForward (LineLength, TotalRow, &Column, &Row);
        }
        break;

      case SCAN_HOME:
        //
        // Move current cursor position to the beginning of the command line
        //
        Row -= (StrPos + StartColumn) / LineLength;
        Column  = StartColumn;
        StrPos  = 0;
        break;

      case SCAN_END:
        //
        // Move current cursor position to the end of the command line
        //
        TailRow     = Row + (Len - StrPos + Column) / LineLength;
        TailColumn  = (Len - StrPos + Column) % LineLength;
        Row         = TailRow;
        Column      = TailColumn;
        StrPos      = Len;
        break;

      case SCAN_ESC:
        //
        // Prepare to clear the current command line
        //
        InStr[0]  = 0;
        Update    = 0;
        Delete    = Len;
        Row -= (StrPos + StartColumn) / LineLength;
        Column        = StartColumn;
        OutputLength  = 0;

        NeedAdjust = TRUE;
        break;

      case SCAN_INSERT:
        //
        // Toggle the SEnvInsertMode flag
        //
        InsertMode = (BOOLEAN)!InsertMode;
        break;

      case SCAN_UP:
      case SCAN_DOWN:
        //
        // show history
        //
        CopyMem (InStr, mInputBufferHistory, StrLength * sizeof(CHAR16));
        StrPos       = StrLen (mInputBufferHistory);
        Update       = 0;
        Delete       = 0;
        OutputLength = 0;

        TailRow      = Row + (StrPos + StartColumn) / LineLength;
        TailColumn   = (StrPos + StartColumn) % LineLength;
        Row          = TailRow;
        Column       = TailColumn;
        NeedAdjust   = FALSE;

        ConOut->SetCursorPosition (ConOut, StartColumn, Row);
        for (SubIndex = 0; SubIndex < EFI_DEBUG_INPUS_BUFFER_SIZE - (StartColumn - EFI_DEBUG_PROMPT_COLUMN); SubIndex++) {
          mBackupSpace[SubIndex] = L' ';
        }
        EDBPrint (mBackupSpace);
        SetMem (mBackupSpace, (EFI_DEBUG_INPUS_BUFFER_SIZE - (StartColumn - EFI_DEBUG_PROMPT_COLUMN)) * sizeof(CHAR16), 0);

        ConOut->SetCursorPosition (ConOut, StartColumn, Row);
        Len = StrPos;

        break;

      case SCAN_F1:
      case SCAN_F2:
      case SCAN_F3:
      case SCAN_F4:
      case SCAN_F5:
      case SCAN_F6:
      case SCAN_F7:
      case SCAN_F8:
      case SCAN_F9:
      case SCAN_F10:
      case SCAN_F11:
      case SCAN_F12:
        CommandStr = GetCommandNameByKey (Key);
        if (CommandStr != NULL) {
          StrnCpyS (InStr, StrLength, CommandStr, StrLength - 1);
          return ;
        }
        break;
      }
    }

    if (Done) {
      break;
    }
    //
    // If we need to update the output do so now
    //
    if (Update != -1) {
      if (NeedAdjust) {
        ConOut->SetCursorPosition (ConOut, Column, Row);
        for (SubIndex = 0; SubIndex < EFI_DEBUG_INPUS_BUFFER_SIZE - (Column - EFI_DEBUG_PROMPT_COLUMN); SubIndex++) {
          mBackupSpace[SubIndex] = L' ';
        }
        EDBPrint (mBackupSpace);
        SetMem (mBackupSpace, (EFI_DEBUG_INPUS_BUFFER_SIZE - (Column - EFI_DEBUG_PROMPT_COLUMN)) * sizeof(CHAR16), 0);
        ConOut->SetCursorPosition (ConOut, Column, Row);
        NeedAdjust = FALSE;
      }
      EDBPrint (InStr + Update);
      Len = StrLen (InStr);

      if (Delete != 0) {
        SetMem (InStr + Len, Delete * sizeof (CHAR16), 0x00);
      }

      if (StrPos > Len) {
        StrPos = Len;
      }

      Update = (UINTN) -1;

      //
      // After using print to reflect newly updates, if we're not using
      // BACKSPACE and DELETE, we need to move the cursor position forward,
      // so adjust row and column here.
      //
      if (Key.UnicodeChar != CHAR_BACKSPACE && !(Key.UnicodeChar == 0 && Key.ScanCode == SCAN_DELETE)) {
        //
        // Calulate row and column of the tail of current string
        //
        TailRow     = Row + (Len - StrPos + Column + OutputLength) / LineLength;
        TailColumn  = (Len - StrPos + Column + OutputLength) % LineLength;

        //
        // If the tail of string reaches screen end, screen rolls up, so if
        // Row does not equal TailRow, Row should be decremented
        //
        // (if we are recalling commands using UPPER and DOWN key, and if the
        // old command is too long to fit the screen, TailColumn must be 79.
        //
        if (TailColumn == 0 && TailRow >= TotalRow && (UINTN) Row != TailRow) {
          Row--;
        }
        //
        // Calculate the cursor position after current operation. If cursor
        // reaches line end, update both row and column, otherwise, only
        // column will be changed.
        //
        if (Column + OutputLength >= LineLength) {
          SkipLength = OutputLength - (LineLength - Column);

          Row += SkipLength / LineLength + 1;
          if ((UINTN) Row > TotalRow - 1) {
            Row = TotalRow - 1;
          }

          Column = SkipLength % LineLength;
        } else {
          Column += OutputLength;
        }
      }

      Delete = 0;
    }
    //
    // Set the cursor position for this key
    //
    SetCursorPosition (ConOut, Column, Row, LineLength, TotalRow, InStr, StrPos, Len);
  } while (!Done);

  CopyMem (mInputBufferHistory, InStr, StrLength * sizeof(CHAR16));

  //
  // Return the data to the caller
  //
  return ;
}

/**
  Set the current coordinates of the cursor position.

  @param  ConOut        Point to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
  @param  Column        The position to set the cursor to.
  @param  Row           The position to set the cursor to.
  @param  LineLength    Length of a line.
  @param  TotalRow      Total row of a screen.
  @param  Str           Point to the string.
  @param  StrPos        The position of the string.
  @param  Len           The length of the string.

**/
VOID
EFIAPI
SetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut,
  IN  UINTN                           Column,
  IN  INTN                            Row,
  IN  UINTN                           LineLength,
  IN  UINTN                           TotalRow,
  IN  CHAR16                          *Str,
  IN  UINTN                           StrPos,
  IN  UINTN                           Len
  )
{
  CHAR16  Backup;

  ASSERT (ConOut != NULL);
  ASSERT (Str != NULL);

  Backup = 0;
  if (Row >= 0) {
    ConOut->SetCursorPosition (ConOut, Column, Row);
    return ;
  }

  if (Len - StrPos > Column * Row) {
    Backup                          = *(Str + StrPos + Column * Row);
    *(Str + StrPos + Column * Row)  = 0;
  }

  EDBPrint (L"%s", Str + StrPos);
  if (Len - StrPos > Column * Row) {
    *(Str + StrPos + Column * Row) = Backup;
  }

  ConOut->SetCursorPosition (ConOut, 0, 0);
}

/**

  SetPageBreak.

**/
BOOLEAN
EFIAPI
SetPageBreak (
  VOID
  )
{
  EFI_INPUT_KEY Key;
  CHAR16        Str[3];
  BOOLEAN       OmitPrint;

  //
  // Check
  //
  if (!mDebuggerPrivate.EnablePageBreak) {
    return FALSE;
  }

  gST->ConOut->OutputString (gST->ConOut, L"Press ENTER to continue, 'q' to exit:");

  OmitPrint = FALSE;
  //
  // Wait for user input
  //
  Str[0]  = ' ';
  Str[1]  = 0;
  Str[2]  = 0;
  for (;;) {
    WaitForSingleEvent (gST->ConIn->WaitForKey, 0);
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    //
    // handle control keys
    //
    if (Key.UnicodeChar == CHAR_NULL) {
      if (Key.ScanCode == SCAN_ESC) {
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        OmitPrint = TRUE;
        break;
      }

      continue;
    }

    if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      gST->ConOut->OutputString (gST->ConOut, L"\r\n");
      break;
    }
    //
    // Echo input
    //
    Str[1] = Key.UnicodeChar;
    if (Str[1] == CHAR_BACKSPACE) {
      continue;
    }

    gST->ConOut->OutputString (gST->ConOut, Str);

    if ((Str[1] == L'q') || (Str[1] == L'Q')) {
      OmitPrint = TRUE;
    } else {
      OmitPrint = FALSE;
    }

    Str[0] = CHAR_BACKSPACE;
  }

  return OmitPrint;
}

/**
  Print a Unicode string to the output device.

  @param  Format    A Null-terminated Unicode format string.
  @param  ...       The variable argument list that contains pointers to Null-
                    terminated Unicode strings to be printed

**/
UINTN
EFIAPI
EDBPrint (
  IN CONST CHAR16  *Format,
  ...
  )
{
  UINTN   Return;
  VA_LIST Marker;
  CHAR16  Buffer[EFI_DEBUG_MAX_PRINT_BUFFER];

  VA_START (Marker, Format);
  Return = UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  if (gST->ConOut != NULL) {
    //
    // To be extra safe make sure ConOut has been initialized
    //
    gST->ConOut->OutputString (gST->ConOut, Buffer);
  }

  return Return;
}

/**
  Print a Unicode string to the output buffer.

  @param  Buffer          A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  Format          A Null-terminated Unicode format string.
  @param  ...             The variable argument list that contains pointers to Null-
                          terminated Unicode strings to be printed

**/
UINTN
EFIAPI
EDBSPrint (
  OUT CHAR16        *Buffer,
  IN  INTN          BufferSize,
  IN  CONST CHAR16  *Format,
  ...
  )
{
  UINTN   Return;
  VA_LIST Marker;

  ASSERT (BufferSize > 0);

  VA_START (Marker, Format);
  Return = UnicodeVSPrint (Buffer, (UINTN)BufferSize, Format, Marker);
  VA_END (Marker);

  return Return;
}

/**
  Print a Unicode string to the output buffer with specified offset..

  @param  Buffer          A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  Offset          The offset of the buffer.
  @param  Format          A Null-terminated Unicode format string.
  @param  ...             The variable argument list that contains pointers to Null-
                          terminated Unicode strings to be printed

**/
UINTN
EFIAPI
EDBSPrintWithOffset (
  OUT CHAR16        *Buffer,
  IN  INTN          BufferSize,
  IN  UINTN         Offset,
  IN  CONST CHAR16  *Format,
  ...
  )
{
  UINTN   Return;
  VA_LIST Marker;

  ASSERT (BufferSize - (Offset * sizeof(CHAR16)) > 0);

  VA_START (Marker, Format);
  Return = UnicodeVSPrint (Buffer + Offset, (UINTN)(BufferSize - (Offset * sizeof(CHAR16))), Format, Marker);
  VA_END (Marker);

  return Return;
}
