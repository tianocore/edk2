/** @file
  Provides interface to shell console logger.

  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett-Packard Development Company, L.P.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Shell.h"

/**
  Install our intermediate ConOut into the system table to
  keep a log of all the info that is displayed to the user.

  @param[in] ScreensToSave  Sets how many screen-worths of data to save.
  @param[out] ConsoleInfo   The object to pass into later functions.

  @retval EFI_SUCCESS       The operation was successful.
  @return other             The operation failed.

  @sa ConsoleLoggerResetBuffers
  @sa InstallProtocolInterface
**/
EFI_STATUS
ConsoleLoggerInstall (
  IN CONST UINTN                   ScreensToSave,
  OUT CONSOLE_LOGGER_PRIVATE_DATA  **ConsoleInfo
  )
{
  EFI_STATUS  Status;

  ASSERT (ConsoleInfo != NULL);

  (*ConsoleInfo) = AllocateZeroPool (sizeof (CONSOLE_LOGGER_PRIVATE_DATA));
  if ((*ConsoleInfo) == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  (*ConsoleInfo)->Signature                   = CONSOLE_LOGGER_PRIVATE_DATA_SIGNATURE;
  (*ConsoleInfo)->OldConOut                   = gST->ConOut;
  (*ConsoleInfo)->OldConHandle                = gST->ConsoleOutHandle;
  (*ConsoleInfo)->Buffer                      = NULL;
  (*ConsoleInfo)->BufferSize                  = 0;
  (*ConsoleInfo)->OriginalStartRow            = 0;
  (*ConsoleInfo)->CurrentStartRow             = 0;
  (*ConsoleInfo)->RowsPerScreen               = 0;
  (*ConsoleInfo)->ColsPerScreen               = 0;
  (*ConsoleInfo)->Attributes                  = NULL;
  (*ConsoleInfo)->AttribSize                  = 0;
  (*ConsoleInfo)->ScreenCount                 = ScreensToSave;
  (*ConsoleInfo)->HistoryMode.MaxMode         = 1;
  (*ConsoleInfo)->HistoryMode.Mode            = 0;
  (*ConsoleInfo)->HistoryMode.Attribute       = 0;
  (*ConsoleInfo)->HistoryMode.CursorColumn    = 0;
  (*ConsoleInfo)->HistoryMode.CursorRow       = 0;
  (*ConsoleInfo)->HistoryMode.CursorVisible   = FALSE;
  (*ConsoleInfo)->OurConOut.Reset             = ConsoleLoggerReset;
  (*ConsoleInfo)->OurConOut.OutputString      = ConsoleLoggerOutputString;
  (*ConsoleInfo)->OurConOut.TestString        = ConsoleLoggerTestString;
  (*ConsoleInfo)->OurConOut.QueryMode         = ConsoleLoggerQueryMode;
  (*ConsoleInfo)->OurConOut.SetMode           = ConsoleLoggerSetMode;
  (*ConsoleInfo)->OurConOut.SetAttribute      = ConsoleLoggerSetAttribute;
  (*ConsoleInfo)->OurConOut.ClearScreen       = ConsoleLoggerClearScreen;
  (*ConsoleInfo)->OurConOut.SetCursorPosition = ConsoleLoggerSetCursorPosition;
  (*ConsoleInfo)->OurConOut.EnableCursor      = ConsoleLoggerEnableCursor;
  (*ConsoleInfo)->OurConOut.Mode              = gST->ConOut->Mode;
  (*ConsoleInfo)->Enabled                     = TRUE;

  Status = ConsoleLoggerResetBuffers (*ConsoleInfo);
  if (EFI_ERROR (Status)) {
    SHELL_FREE_NON_NULL ((*ConsoleInfo));
    *ConsoleInfo = NULL;
    return (Status);
  }

  Status = gBS->InstallProtocolInterface (&gImageHandle, &gEfiSimpleTextOutProtocolGuid, EFI_NATIVE_INTERFACE, (VOID *)&((*ConsoleInfo)->OurConOut));
  if (EFI_ERROR (Status)) {
    SHELL_FREE_NON_NULL ((*ConsoleInfo)->Buffer);
    SHELL_FREE_NON_NULL ((*ConsoleInfo)->Attributes);
    SHELL_FREE_NON_NULL ((*ConsoleInfo));
    *ConsoleInfo = NULL;
    return (Status);
  }

  gST->ConsoleOutHandle = gImageHandle;
  gST->ConOut           = &(*ConsoleInfo)->OurConOut;

  //
  // Update the CRC32 in the EFI System Table header
  //
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (
         (UINT8 *)&gST->Hdr,
         gST->Hdr.HeaderSize,
         &gST->Hdr.CRC32
         );
  return (Status);
}

/**
  Return the system to the state it was before InstallConsoleLogger
  was installed.

  @param[in] ConsoleInfo  The object from the install function.

  @retval EFI_SUCCESS     The operation was successful
  @return other           The operation failed.  This was from UninstallProtocolInterface.
**/
EFI_STATUS
ConsoleLoggerUninstall (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  ASSERT (ConsoleInfo != NULL);
  ASSERT (ConsoleInfo->OldConOut != NULL);

  if (ConsoleInfo->Buffer != NULL) {
    FreePool (ConsoleInfo->Buffer);
    DEBUG_CODE (
      ConsoleInfo->Buffer = NULL;
      );
    DEBUG_CODE (
      ConsoleInfo->BufferSize = 0;
      );
  }

  if (ConsoleInfo->Attributes != NULL) {
    FreePool (ConsoleInfo->Attributes);
    DEBUG_CODE (
      ConsoleInfo->Attributes = NULL;
      );
    DEBUG_CODE (
      ConsoleInfo->AttribSize = 0;
      );
  }

  gST->ConsoleOutHandle = ConsoleInfo->OldConHandle;
  gST->ConOut           = ConsoleInfo->OldConOut;

  //
  // Update the CRC32 in the EFI System Table header
  //
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (
         (UINT8 *)&gST->Hdr,
         gST->Hdr.HeaderSize,
         &gST->Hdr.CRC32
         );

  return (gBS->UninstallProtocolInterface (gImageHandle, &gEfiSimpleTextOutProtocolGuid, (VOID *)&ConsoleInfo->OurConOut));
}

/**
  Displays previously logged output back to the screen.

  This will scroll the screen forwards and backwards through the log of previous
  output.  If Rows is 0 then the size of 1/2 the screen will be scrolled.  If Rows
  is (UINTN)(-1) then the size of the screen will be scrolled.

  @param[in] Forward      If TRUE then the log will be displayed forwards (scroll to newer).
                          If FALSE then the log will be displayed backwards (scroll to older).
  @param[in] Rows         Determines how many rows the log should scroll.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.
**/
EFI_STATUS
ConsoleLoggerDisplayHistory (
  IN CONST BOOLEAN                Forward,
  IN CONST UINTN                  Rows,
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  UINTN  RowChange;

  ASSERT (ConsoleInfo != NULL);

  //
  // Calculate the row number change
  //
  switch (Rows) {
    case ((UINTN)(-1)):
      RowChange = ConsoleInfo->RowsPerScreen;
      break;
    case (0):
      RowChange = ConsoleInfo->RowsPerScreen / 2;
      break;
    default:
      RowChange = Rows;
      break;
  }

  //
  // Do the math for direction
  //
  if (Forward) {
    if ((ConsoleInfo->OriginalStartRow - ConsoleInfo->CurrentStartRow) < RowChange) {
      RowChange = ConsoleInfo->OriginalStartRow - ConsoleInfo->CurrentStartRow;
    }
  } else {
    if (ConsoleInfo->CurrentStartRow < RowChange) {
      RowChange = ConsoleInfo->CurrentStartRow;
    }
  }

  //
  // If we are already at one end or the other
  //
  if (RowChange == 0) {
    return (EFI_SUCCESS);
  }

  //
  // Clear the screen
  //
  ConsoleInfo->OldConOut->ClearScreen (ConsoleInfo->OldConOut);

  //
  // Set the new start row
  //
  if (Forward) {
    ConsoleInfo->CurrentStartRow += RowChange;
  } else {
    ConsoleInfo->CurrentStartRow -= RowChange;
  }

  //
  // Change the screen
  //
  return (UpdateDisplayFromHistory (ConsoleInfo));
}

/**
  Function to return to normal output whent he scrolling is complete.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS   The operation was successful.
  @return other         The operation failed.  See UpdateDisplayFromHistory.

  @sa UpdateDisplayFromHistory
**/
EFI_STATUS
ConsoleLoggerStopHistory (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  ASSERT (ConsoleInfo != NULL);
  if (ConsoleInfo->CurrentStartRow == ConsoleInfo->OriginalStartRow) {
    return (EFI_SUCCESS);
  }

  //
  // Clear the screen
  //
  ConsoleInfo->OldConOut->ClearScreen (ConsoleInfo->OldConOut);

  ConsoleInfo->CurrentStartRow = ConsoleInfo->OriginalStartRow;
  return (UpdateDisplayFromHistory (ConsoleInfo));
}

/**
  Updates the hidden ConOut to be displaying the correct stuff.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS     The operation was successful.
  @return other           The operation failed.
**/
EFI_STATUS
UpdateDisplayFromHistory (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  RetVal;
  CHAR16      *Screen;
  INT32       *Attributes;
  UINTN       CurrentRow;
  CHAR16      TempCharHolder;
  UINTN       Column;
  INT32       CurrentAttrib;
  UINTN       CurrentColumn;
  CHAR16      *StringSegment;
  CHAR16      *StringSegmentEnd;
  CHAR16      StringSegmentEndChar;
  INT32       OrigAttribute;

  ASSERT (ConsoleInfo != NULL);
  TempCharHolder = CHAR_NULL;
  RetVal         = EFI_SUCCESS;
  OrigAttribute  = ConsoleInfo->OldConOut->Mode->Attribute;

  //
  // Disable cursor visibility and move it to the top left corner
  //
  ConsoleInfo->OldConOut->EnableCursor (ConsoleInfo->OldConOut, FALSE);
  ConsoleInfo->OldConOut->SetCursorPosition (ConsoleInfo->OldConOut, 0, 0);

  Screen     = &ConsoleInfo->Buffer[(ConsoleInfo->ColsPerScreen + 2) * ConsoleInfo->CurrentStartRow];
  Attributes = &ConsoleInfo->Attributes[ConsoleInfo->ColsPerScreen * ConsoleInfo->CurrentStartRow];
  for ( CurrentRow = 0
        ; CurrentRow < ConsoleInfo->RowsPerScreen
        ; CurrentRow++,
        Screen += (ConsoleInfo->ColsPerScreen + 2),
        Attributes += ConsoleInfo->ColsPerScreen
        )
  {
    //
    // dont use the last char - prevents screen scroll
    //
    if (CurrentRow == (ConsoleInfo->RowsPerScreen-1)) {
      TempCharHolder                         = Screen[ConsoleInfo->ColsPerScreen - 1];
      Screen[ConsoleInfo->ColsPerScreen - 1] = CHAR_NULL;
    }

    for ( Column = 0
          ; Column < ConsoleInfo->ColsPerScreen
          ; Column++
          )
    {
      if (Screen[Column] != CHAR_NULL) {
        CurrentAttrib = Attributes[Column];
        CurrentColumn = Column;
        StringSegment = &Screen[Column];

        //
        // Find the first char with a different attribute and make that temporarily NULL
        // so we can do fewer printout statements.  (later) restore that one and we will
        // start at that column on the next loop.
        //
        StringSegmentEndChar = CHAR_NULL;
        for ( StringSegmentEnd = StringSegment
              ; *StringSegmentEnd != CHAR_NULL
              ; StringSegmentEnd++,
              Column++
              )
        {
          if (Attributes[Column] != CurrentAttrib) {
            StringSegmentEndChar = *StringSegmentEnd;
            *StringSegmentEnd    = CHAR_NULL;
            break;
          }
        } // StringSegmentEnd loop

        //
        // Now write out as much as had the same Attributes
        //

        ConsoleInfo->OldConOut->SetAttribute (ConsoleInfo->OldConOut, CurrentAttrib);
        ConsoleInfo->OldConOut->SetCursorPosition (ConsoleInfo->OldConOut, CurrentColumn, CurrentRow);
        Status = ConsoleInfo->OldConOut->OutputString (ConsoleInfo->OldConOut, StringSegment);

        if (EFI_ERROR (Status)) {
          ASSERT (FALSE);
          RetVal = Status;
        }

        //
        // If we found a change in attribute put the character back and decrement the column
        // so when it increments it will point at that character and we will start printing
        // a segment with that new attribute
        //
        if (StringSegmentEndChar != CHAR_NULL) {
          *StringSegmentEnd    = StringSegmentEndChar;
          StringSegmentEndChar = CHAR_NULL;
          Column--;
        }
      }
    } // column for loop

    //
    // If we removed the last char and this was the last row put it back
    //
    if (TempCharHolder != CHAR_NULL) {
      Screen[ConsoleInfo->ColsPerScreen - 1] = TempCharHolder;
      TempCharHolder                         = CHAR_NULL;
    }
  } // row for loop

  //
  // If we are setting the screen back to original turn on the cursor and make it visible
  // and set the attributes back to what they were
  //
  if (ConsoleInfo->CurrentStartRow == ConsoleInfo->OriginalStartRow) {
    ConsoleInfo->OldConOut->SetAttribute (
                              ConsoleInfo->OldConOut,
                              ConsoleInfo->HistoryMode.Attribute
                              );
    ConsoleInfo->OldConOut->SetCursorPosition (
                              ConsoleInfo->OldConOut,
                              ConsoleInfo->HistoryMode.CursorColumn,
                              ConsoleInfo->HistoryMode.CursorRow - ConsoleInfo->OriginalStartRow
                              );

    Status = ConsoleInfo->OldConOut->EnableCursor (
                                       ConsoleInfo->OldConOut,
                                       ConsoleInfo->HistoryMode.CursorVisible
                                       );
    if (EFI_ERROR (Status)) {
      RetVal = Status;
    }
  } else {
    ConsoleInfo->OldConOut->SetAttribute (
                              ConsoleInfo->OldConOut,
                              OrigAttribute
                              );
  }

  return (RetVal);
}

/**
  Reset the text output device hardware and optionally run diagnostics

  @param  This                pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  @param ExtendedVerification Indicates that a more extensive test may be performed

  @retval EFI_SUCCESS         The text output device was reset.
  @retval EFI_DEVICE_ERROR    The text output device is not functioning correctly and
                              could not be reset.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS                   Status;
  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Forward the request to the original ConOut
  //
  Status = ConsoleInfo->OldConOut->Reset (ConsoleInfo->OldConOut, ExtendedVerification);

  //
  // Check that the buffers are still correct for logging
  //
  if (!EFI_ERROR (Status)) {
    ConsoleLoggerResetBuffers (ConsoleInfo);
    if (ExtendedVerification) {
      ConsoleInfo->OriginalStartRow = 0;
      ConsoleInfo->CurrentStartRow  = 0;
    }
  }

  return Status;
}

/**
  Appends a string to the history buffer.  If the buffer is full then the oldest
  information in the buffer will be dropped.  Information is added in a line by
  line manner such that an empty line takes up just as much space as a full line.

  @param[in] String       String pointer to add.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.
**/
EFI_STATUS
AppendStringToHistory (
  IN CONST CHAR16                 *String,
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  CONST CHAR16  *Walker;
  UINTN         CopySize;
  UINTN         PrintIndex;
  UINTN         Index;

  ASSERT (ConsoleInfo != NULL);

  for ( Walker = String
        ; Walker != NULL && *Walker != CHAR_NULL
        ; Walker++
        )
  {
    switch (*Walker) {
      case (CHAR_BACKSPACE):
        if (ConsoleInfo->HistoryMode.CursorColumn > 0) {
          ConsoleInfo->HistoryMode.CursorColumn--;
        }

        break;
      case (CHAR_LINEFEED):
        if (ConsoleInfo->HistoryMode.CursorRow >= (INT32)((ConsoleInfo->RowsPerScreen * ConsoleInfo->ScreenCount)-1)) {
          //
          // Should never be bigger
          //
          ASSERT (ConsoleInfo->HistoryMode.CursorRow == (INT32)((ConsoleInfo->RowsPerScreen * ConsoleInfo->ScreenCount)-1));

          //
          // scroll history attributes 'up' 1 row and set the last row to default attribute
          //
          CopySize = ConsoleInfo->ColsPerScreen
                     * ((ConsoleInfo->RowsPerScreen * ConsoleInfo->ScreenCount) - 1)
                     * sizeof (ConsoleInfo->Attributes[0]);
          ASSERT (CopySize < ConsoleInfo->AttribSize);
          CopyMem (
            ConsoleInfo->Attributes,
            ConsoleInfo->Attributes + ConsoleInfo->ColsPerScreen,
            CopySize
            );

          for ( Index = 0
                ; Index < ConsoleInfo->ColsPerScreen
                ; Index++
                )
          {
            *(ConsoleInfo->Attributes + (CopySize/sizeof (ConsoleInfo->Attributes[0])) + Index) = ConsoleInfo->HistoryMode.Attribute;
          }

          //
          // scroll history buffer 'up' 1 row and set the last row to spaces (L' ')
          //
          CopySize = (ConsoleInfo->ColsPerScreen + 2)
                     * ((ConsoleInfo->RowsPerScreen * ConsoleInfo->ScreenCount) - 1)
                     * sizeof (ConsoleInfo->Buffer[0]);
          ASSERT (CopySize < ConsoleInfo->BufferSize);
          CopyMem (
            ConsoleInfo->Buffer,
            ConsoleInfo->Buffer + (ConsoleInfo->ColsPerScreen + 2),
            CopySize
            );

          //
          // Set that last row of chars to spaces
          //
          SetMem16 (((UINT8 *)ConsoleInfo->Buffer)+CopySize, ConsoleInfo->ColsPerScreen*sizeof (CHAR16), L' ');
        } else {
          //
          // we are not on the last row
          //

          //
          // We should not be scrolling history
          //
          ASSERT (ConsoleInfo->OriginalStartRow == ConsoleInfo->CurrentStartRow);
          //
          // are we at the end of a row?
          //
          if (ConsoleInfo->HistoryMode.CursorRow == (INT32)(ConsoleInfo->OriginalStartRow + ConsoleInfo->RowsPerScreen - 1)) {
            ConsoleInfo->OriginalStartRow++;
            ConsoleInfo->CurrentStartRow++;
          }

          ConsoleInfo->HistoryMode.CursorRow++;
        }

        break;
      case (CHAR_CARRIAGE_RETURN):
        //
        // Move the cursor to the beginning of the current row.
        //
        ConsoleInfo->HistoryMode.CursorColumn = 0;
        break;
      default:
        //
        // Acrtually print characters into the history buffer
        //

        PrintIndex = ConsoleInfo->HistoryMode.CursorRow * ConsoleInfo->ColsPerScreen + ConsoleInfo->HistoryMode.CursorColumn;

        for ( // no initializer needed
              ; ConsoleInfo->HistoryMode.CursorColumn < (INT32)ConsoleInfo->ColsPerScreen
              ; ConsoleInfo->HistoryMode.CursorColumn++,
              PrintIndex++,
              Walker++
              )
        {
          if (  (*Walker == CHAR_NULL)
             || (*Walker == CHAR_BACKSPACE)
             || (*Walker == CHAR_LINEFEED)
             || (*Walker == CHAR_CARRIAGE_RETURN)
                )
          {
            Walker--;
            break;
          }

          //
          // The buffer is 2*CursorRow more since it has that many \r\n characters at the end of each row.
          //

          ASSERT (PrintIndex + ConsoleInfo->HistoryMode.CursorRow < ConsoleInfo->BufferSize);
          ConsoleInfo->Buffer[PrintIndex + (2*ConsoleInfo->HistoryMode.CursorRow)] = *Walker;
          ASSERT (PrintIndex < ConsoleInfo->AttribSize);
          ConsoleInfo->Attributes[PrintIndex] = ConsoleInfo->HistoryMode.Attribute;
        } // for loop

        //
        // Add the carriage return and line feed at the end of the lines
        //
        if (ConsoleInfo->HistoryMode.CursorColumn >= (INT32)ConsoleInfo->ColsPerScreen) {
          AppendStringToHistory (L"\r\n", ConsoleInfo);
          Walker--;
        }

        break;
    } // switch for character
  } // for loop

  return (EFI_SUCCESS);
}

/**
  Worker function to handle printing the output to the screen
  and the history buffer

  @param[in] String               The string to output
  @param[in] ConsoleInfo          The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS             The string was printed
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to output
                                  the text.
  @retval EFI_UNSUPPORTED         The output device's mode is not currently in a
                                  defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH  This warning code indicates that some of the
                                  characters in the Unicode string could not be
                                  rendered and were skipped.
**/
EFI_STATUS
ConsoleLoggerOutputStringSplit (
  IN CONST CHAR16                 *String,
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  EFI_STATUS  Status;

  //
  // Forward the request to the original ConOut
  //
  Status = ConsoleInfo->OldConOut->OutputString (ConsoleInfo->OldConOut, (CHAR16 *)String);

  if (EFI_ERROR (Status)) {
    return (Status);
  }

  return (AppendStringToHistory (String, ConsoleInfo));
}

/**
  Function to handle page break mode.

  This function will prompt for continue or break.

  @retval EFI_SUCCESS   Continue was choosen
  @return other         Break was choosen
**/
EFI_STATUS
ConsoleLoggerDoPageBreak (
  VOID
  )
{
  SHELL_PROMPT_RESPONSE  *Resp;
  EFI_STATUS             Status;

  Resp = NULL;
  ASSERT (ShellInfoObject.PageBreakEnabled);
  ShellInfoObject.PageBreakEnabled = FALSE;
  Status                           = ShellPromptForResponseHii (ShellPromptResponseTypeQuitContinue, STRING_TOKEN (STR_SHELL_QUIT_CONT), ShellInfoObject.HiiHandle, (VOID **)&Resp);
  ShellInfoObject.PageBreakEnabled = TRUE;
  ASSERT (Resp != NULL);
  if (Resp == NULL) {
    return (EFI_NOT_FOUND);
  }

  if (EFI_ERROR (Status)) {
    if (Resp != NULL) {
      FreePool (Resp);
    }

    return (Status);
  }

  if (*Resp == ShellPromptResponseContinue) {
    FreePool (Resp);
    ShellInfoObject.ConsoleInfo->RowCounter = 0;
    //    ShellInfoObject.ConsoleInfo->OurConOut.Mode->CursorRow    = 0;
    //    ShellInfoObject.ConsoleInfo->OurConOut.Mode->CursorColumn = 0;

    return (EFI_SUCCESS);
  } else if (*Resp == ShellPromptResponseQuit) {
    FreePool (Resp);
    ShellInfoObject.ConsoleInfo->Enabled = FALSE;
    //
    // When user wants to quit, the shell should stop running the command.
    //
    gBS->SignalEvent (ShellInfoObject.NewEfiShellProtocol->ExecutionBreak);
    return (EFI_DEVICE_ERROR);
  } else {
    ASSERT (FALSE);
  }

  return (EFI_SUCCESS);
}

/**
  Worker function to handle printing the output with page breaks.

  @param[in] String               The string to output
  @param[in] ConsoleInfo          The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS             The string was printed
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to output
                                  the text.
  @retval EFI_UNSUPPORTED         The output device's mode is not currently in a
                                  defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH  This warning code indicates that some of the
                                  characters in the Unicode string could not be
                                  rendered and were skipped.
**/
EFI_STATUS
ConsoleLoggerPrintWithPageBreak (
  IN CONST CHAR16                 *String,
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  CONST CHAR16  *Walker;
  CONST CHAR16  *LineStart;
  CHAR16        *StringCopy;
  CHAR16        TempChar;

  StringCopy = NULL;
  StringCopy = StrnCatGrow (&StringCopy, NULL, String, 0);
  if (StringCopy == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  for ( Walker = StringCopy,
        LineStart = StringCopy
        ; Walker != NULL && *Walker != CHAR_NULL
        ; Walker++
        )
  {
    switch (*Walker) {
      case (CHAR_BACKSPACE):
        if (ConsoleInfo->OurConOut.Mode->CursorColumn > 0) {
          ConsoleInfo->OurConOut.Mode->CursorColumn--;
        }

        break;
      case (CHAR_LINEFEED):
        //
        // add a temp NULL terminator
        //
        TempChar                = *(Walker + 1);
        *((CHAR16 *)(Walker+1)) = CHAR_NULL;

        //
        // output the string
        //
        ConsoleLoggerOutputStringSplit (LineStart, ConsoleInfo);

        //
        // restore the temp NULL terminator to its original character
        //
        *((CHAR16 *)(Walker+1)) = TempChar;

        //
        // Update LineStart Variable
        //
        LineStart = Walker + 1;

        //
        // increment row count
        //
        ShellInfoObject.ConsoleInfo->RowCounter++;
        ConsoleInfo->OurConOut.Mode->CursorRow++;

        break;
      case (CHAR_CARRIAGE_RETURN):
        //
        // Move the cursor to the beginning of the current row.
        //
        ConsoleInfo->OurConOut.Mode->CursorColumn = 0;
        break;
      default:
        //
        // increment column count
        //
        ConsoleInfo->OurConOut.Mode->CursorColumn++;
        //
        // check if that is the last column
        //
        if ((INTN)ConsoleInfo->ColsPerScreen == ConsoleInfo->OurConOut.Mode->CursorColumn + 1) {
          //
          // output a line similar to the linefeed character.
          //

          //
          // add a temp NULL terminator
          //
          TempChar                = *(Walker + 1);
          *((CHAR16 *)(Walker+1)) = CHAR_NULL;

          //
          // output the string
          //
          ConsoleLoggerOutputStringSplit (LineStart, ConsoleInfo);

          //
          // restore the temp NULL terminator to its original character
          //
          *((CHAR16 *)(Walker+1)) = TempChar;

          //
          // Update LineStart Variable
          //
          LineStart = Walker + 1;

          //
          // increment row count and zero the column
          //
          ShellInfoObject.ConsoleInfo->RowCounter++;
          ConsoleInfo->OurConOut.Mode->CursorRow++;
          ConsoleInfo->OurConOut.Mode->CursorColumn = 0;
        } // last column on line

        break;
    } // switch for character

    //
    // check if that was the last printable row.  If yes handle PageBreak mode
    //
    if ((ConsoleInfo->RowsPerScreen) -1 == ShellInfoObject.ConsoleInfo->RowCounter) {
      if (EFI_ERROR (ConsoleLoggerDoPageBreak ())) {
        //
        // We got an error which means 'break' and halt the printing
        //
        SHELL_FREE_NON_NULL (StringCopy);
        return (EFI_DEVICE_ERROR);
      }
    }
  } // for loop

  if ((LineStart != NULL) && (*LineStart != CHAR_NULL)) {
    ConsoleLoggerOutputStringSplit (LineStart, ConsoleInfo);
  }

  SHELL_FREE_NON_NULL (StringCopy);
  return (EFI_SUCCESS);
}

/**
  Write a Unicode string to the output device.

  @param[in] This                 Protocol instance pointer.
  @param[in] WString              The NULL-terminated Unicode string to be displayed on the output
                                  device(s). All output devices must also support the Unicode
                                  drawing defined in this file.
  @retval EFI_SUCCESS             The string was output to the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to output
                                  the text.
  @retval EFI_UNSUPPORTED         The output device's mode is not currently in a
                                  defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH  This warning code indicates that some of the
                                  characters in the Unicode string could not be
                                  rendered and were skipped.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TxtInEx;
  EFI_KEY_DATA                       KeyData;
  UINTN                              EventIndex;
  CONSOLE_LOGGER_PRIVATE_DATA        *ConsoleInfo;

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);
  if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleOut) {
    return (EFI_UNSUPPORTED);
  }

  ASSERT (ShellInfoObject.ConsoleInfo == ConsoleInfo);

  Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiSimpleTextInputExProtocolGuid, (VOID **)&TxtInEx);
  if (!EFI_ERROR (Status)) {
    while (ShellInfoObject.HaltOutput) {
      ShellInfoObject.HaltOutput = FALSE;
      //
      // just get some key
      //
      Status = gBS->WaitForEvent (1, &TxtInEx->WaitForKeyEx, &EventIndex);
      ASSERT_EFI_ERROR (Status);
      Status = TxtInEx->ReadKeyStrokeEx (TxtInEx, &KeyData);
      if (EFI_ERROR (Status)) {
        break;
      }

      if ((KeyData.Key.UnicodeChar == L's') && (KeyData.Key.ScanCode == SCAN_NULL) &&
          ((KeyData.KeyState.KeyShiftState == (EFI_SHIFT_STATE_VALID | EFI_LEFT_CONTROL_PRESSED)) ||
           (KeyData.KeyState.KeyShiftState == (EFI_SHIFT_STATE_VALID | EFI_RIGHT_CONTROL_PRESSED))
          )
          )
      {
        ShellInfoObject.HaltOutput = TRUE;
      }
    }
  }

  if (!ShellInfoObject.ConsoleInfo->Enabled) {
    return (EFI_DEVICE_ERROR);
  } else if (ShellInfoObject.PageBreakEnabled) {
    return (ConsoleLoggerPrintWithPageBreak (WString, ConsoleInfo));
  } else {
    return (ConsoleLoggerOutputStringSplit (WString, ConsoleInfo));
  }
}

/**
  Verifies that all characters in a Unicode string can be output to the
  target device.

  @param[in] This     Protocol instance pointer.
  @param[in] WString  The NULL-terminated Unicode string to be examined for the output
                      device(s).

  @retval EFI_SUCCESS           The device(s) are capable of rendering the output string.
  @retval EFI_UNSUPPORTED       Some of the characters in the Unicode string cannot be
                                rendered by one or more of the output devices mapped
                                by the EFI handle.

**/
EFI_STATUS
EFIAPI
ConsoleLoggerTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);
  //
  // Forward the request to the original ConOut
  //
  return (ConsoleInfo->OldConOut->TestString (ConsoleInfo->OldConOut, WString));
}

/**
  Returns information for an available text mode that the output device(s)
  supports.

  @param[in] This               Protocol instance pointer.
  @param[in] ModeNumber         The mode number to return information on.
  @param[out] Columns           Upon return, the number of columns in the selected geometry
  @param[out] Rows              Upon return, the number of rows in the selected geometry

  @retval EFI_SUCCESS           The requested mode information was returned.
  @retval EFI_DEVICE_ERROR      The device had an error and could not
                                complete the request.
  @retval EFI_UNSUPPORTED       The mode number was not valid.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
{
  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);
  //
  // Forward the request to the original ConOut
  //
  return (ConsoleInfo->OldConOut->QueryMode (
                                    ConsoleInfo->OldConOut,
                                    ModeNumber,
                                    Columns,
                                    Rows
                                    ));
}

/**
  Sets the output device(s) to a specified mode.

  @param[in] This               Protocol instance pointer.
  @param[in] ModeNumber         The mode number to set.


  @retval EFI_SUCCESS           The requested text mode was set.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The mode number was not valid.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  )
{
  EFI_STATUS  Status;

  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Forward the request to the original ConOut
  //
  Status = ConsoleInfo->OldConOut->SetMode (ConsoleInfo->OldConOut, ModeNumber);

  //
  // Check that the buffers are still correct for logging
  //
  if (!EFI_ERROR (Status)) {
    ConsoleInfo->OurConOut.Mode = ConsoleInfo->OldConOut->Mode;
    ConsoleLoggerResetBuffers (ConsoleInfo);
    ConsoleInfo->OriginalStartRow = 0;
    ConsoleInfo->CurrentStartRow  = 0;
    ConsoleInfo->OurConOut.ClearScreen (&ConsoleInfo->OurConOut);
  }

  return Status;
}

/**
  Sets the background and foreground colors for the OutputString () and
  ClearScreen () functions.

  @param[in] This               Protocol instance pointer.
  @param[in] Attribute          The attribute to set. Bits 0..3 are the foreground color, and
                                bits 4..6 are the background color. All other bits are undefined
                                and must be zero. The valid Attributes are defined in this file.

  @retval EFI_SUCCESS           The attribute was set.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The attribute requested is not defined.

**/
EFI_STATUS
EFIAPI
ConsoleLoggerSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  )
{
  EFI_STATUS  Status;

  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Forward the request to the original ConOut
  //
  Status = ConsoleInfo->OldConOut->SetAttribute (ConsoleInfo->OldConOut, Attribute);

  //
  // Record console output history
  //
  if (!EFI_ERROR (Status)) {
    ConsoleInfo->HistoryMode.Attribute = (INT32)Attribute;
  }

  return Status;
}

/**
  Clears the output device(s) display to the currently selected background
  color.

  @param[in] This               Protocol instance pointer.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The output device is not in a valid text mode.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  EFI_STATUS                   Status;
  CHAR16                       *Screen;
  INT32                        *Attributes;
  UINTN                        Row;
  UINTN                        Column;
  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleOut) {
    return (EFI_UNSUPPORTED);
  }

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Forward the request to the original ConOut
  //
  Status = ConsoleInfo->OldConOut->ClearScreen (ConsoleInfo->OldConOut);

  //
  // Record console output history
  //
  if (!EFI_ERROR (Status)) {
    Screen     = &ConsoleInfo->Buffer[(ConsoleInfo->ColsPerScreen + 2) * ConsoleInfo->CurrentStartRow];
    Attributes = &ConsoleInfo->Attributes[ConsoleInfo->ColsPerScreen * ConsoleInfo->CurrentStartRow];
    for ( Row = ConsoleInfo->OriginalStartRow
          ; Row < (ConsoleInfo->RowsPerScreen * ConsoleInfo->ScreenCount)
          ; Row++
          )
    {
      for ( Column = 0
            ; Column < ConsoleInfo->ColsPerScreen
            ; Column++,
            Screen++,
            Attributes++
            )
      {
        *Screen     = L' ';
        *Attributes = ConsoleInfo->OldConOut->Mode->Attribute;
      }

      //
      // Skip the NULL on each column end in text buffer only
      //
      Screen += 2;
    }

    ConsoleInfo->HistoryMode.CursorColumn = 0;
    ConsoleInfo->HistoryMode.CursorRow    = 0;
  }

  return Status;
}

/**
  Sets the current coordinates of the cursor position

  @param[in] This               Protocol instance pointer.
  @param[in] Column             Column to put the cursor in.  Must be between zero and Column returned from QueryMode
  @param[in] Row                Row to put the cursor in.  Must be between zero and Row returned from QueryMode

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The output device is not in a valid text mode, or the
                                cursor position is invalid for the current mode.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  )
{
  EFI_STATUS                   Status;
  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleOut) {
    return (EFI_UNSUPPORTED);
  }

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);
  //
  // Forward the request to the original ConOut
  //
  Status = ConsoleInfo->OldConOut->SetCursorPosition (
                                     ConsoleInfo->OldConOut,
                                     Column,
                                     Row
                                     );

  //
  // Record console output history
  //
  if (!EFI_ERROR (Status)) {
    ConsoleInfo->HistoryMode.CursorColumn = (INT32)Column;
    ConsoleInfo->HistoryMode.CursorRow    = (INT32)(ConsoleInfo->OriginalStartRow + Row);
  }

  return Status;
}

/**
  Makes the cursor visible or invisible

  @param[in] This       Protocol instance pointer.
  @param[in] Visible    If TRUE, the cursor is set to be visible. If FALSE, the cursor is
                        set to be invisible.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device had an error and could not complete the
                                request, or the device does not support changing
                                the cursor mode.
  @retval EFI_UNSUPPORTED       The output device is not in a valid text mode.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  )
{
  EFI_STATUS  Status;

  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;

  ConsoleInfo = CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS (This);
  //
  // Forward the request to the original ConOut
  //
  Status = ConsoleInfo->OldConOut->EnableCursor (ConsoleInfo->OldConOut, Visible);

  //
  // Record console output history
  //
  if (!EFI_ERROR (Status)) {
    ConsoleInfo->HistoryMode.CursorVisible = Visible;
  }

  return Status;
}

/**
  Function to update and verify that the current buffers are correct.

  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  This will be used when a mode has changed or a reset occurred to verify all
  history buffers.
**/
EFI_STATUS
ConsoleLoggerResetBuffers (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  EFI_STATUS  Status;

  if (ConsoleInfo->Buffer != NULL) {
    FreePool (ConsoleInfo->Buffer);
    ConsoleInfo->Buffer     = NULL;
    ConsoleInfo->BufferSize = 0;
  }

  if (ConsoleInfo->Attributes != NULL) {
    FreePool (ConsoleInfo->Attributes);
    ConsoleInfo->Attributes = NULL;
    ConsoleInfo->AttribSize = 0;
  }

  Status = gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &ConsoleInfo->ColsPerScreen, &ConsoleInfo->RowsPerScreen);
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  ConsoleInfo->BufferSize = (ConsoleInfo->ColsPerScreen + 2) * ConsoleInfo->RowsPerScreen * ConsoleInfo->ScreenCount * sizeof (ConsoleInfo->Buffer[0]);
  ConsoleInfo->AttribSize = ConsoleInfo->ColsPerScreen * ConsoleInfo->RowsPerScreen * ConsoleInfo->ScreenCount * sizeof (ConsoleInfo->Attributes[0]);

  ConsoleInfo->Buffer = (CHAR16 *)AllocateZeroPool (ConsoleInfo->BufferSize);

  if (ConsoleInfo->Buffer == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  ConsoleInfo->Attributes = (INT32 *)AllocateZeroPool (ConsoleInfo->AttribSize);
  if (ConsoleInfo->Attributes == NULL) {
    FreePool (ConsoleInfo->Buffer);
    ConsoleInfo->Buffer = NULL;
    return (EFI_OUT_OF_RESOURCES);
  }

  CopyMem (&ConsoleInfo->HistoryMode, ConsoleInfo->OldConOut->Mode, sizeof (EFI_SIMPLE_TEXT_OUTPUT_MODE));

  return (EFI_SUCCESS);
}
