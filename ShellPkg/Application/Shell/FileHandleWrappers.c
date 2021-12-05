/** @file
  EFI_FILE_PROTOCOL wrappers for other items (Like Environment Variables,
  StdIn, StdOut, StdErr, etc...).

  Copyright 2016 Dell Inc.
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"
#include "FileHandleInternal.h"

#define MEM_WRITE_REALLOC_OVERHEAD  1024

/**
  File style interface for console (Open).

  @param[in] This       Ignored.
  @param[out] NewHandle Ignored.
  @param[in] FileName   Ignored.
  @param[in] OpenMode   Ignored.
  @param[in] Attributes Ignored.

  @retval EFI_NOT_FOUND
**/
EFI_STATUS
EFIAPI
FileInterfaceOpenNotFound (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  )
{
  return (EFI_NOT_FOUND);
}

/**
  File style interface for console (Close, Delete, & Flush)

  @param[in] This       Ignored.

  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FileInterfaceNopGeneric (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  return (EFI_SUCCESS);
}

/**
  File style interface for console (GetPosition).

  @param[in] This       Ignored.
  @param[out] Position  Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopGetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  File style interface for console (SetPosition).

  @param[in] This       Ignored.
  @param[in] Position   Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  File style interface for console (GetInfo).

  @param[in] This              Ignored.
  @param[in] InformationType   Ignored.
  @param[in, out] BufferSize   Ignored.
  @param[out] Buffer           Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  File style interface for console (SetInfo).

  @param[in] This       Ignored.
  @param[in] InformationType   Ignored.
  @param[in] BufferSize Ignored.
  @param[in] Buffer     Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  File style interface for StdOut (Write).

  Writes data to the screen.

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to write.

  @retval EFI_UNSUPPORTED No output console is supported.
  @return A return value from gST->ConOut->OutputString.
**/
EFI_STATUS
EFIAPI
FileInterfaceStdOutWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  if (ShellInfoObject.ShellInitSettings.BitUnion.Bits.NoConsoleOut) {
    return (EFI_UNSUPPORTED);
  }

  if (*((CHAR16 *)Buffer) == gUnicodeFileTag) {
    return (gST->ConOut->OutputString (gST->ConOut, (CHAR16 *)Buffer + 1));
  }

  return (gST->ConOut->OutputString (gST->ConOut, Buffer));
}

/**
  File style interface for StdIn (Write).

  @param[in] This            Ignored.
  @param[in, out] BufferSize Ignored.
  @param[in] Buffer          Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceStdInWrite (
  IN      EFI_FILE_PROTOCOL  *This,
  IN OUT  UINTN              *BufferSize,
  IN      VOID               *Buffer
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  File style interface for console StdErr (Write).

  Writes error to the error output.

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to write.

  @return A return value from gST->StdErr->OutputString.
**/
EFI_STATUS
EFIAPI
FileInterfaceStdErrWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  return (gST->StdErr->OutputString (gST->StdErr, Buffer));
}

/**
  File style interface for console StdOut (Read).

  @param[in] This              Ignored.
  @param[in, out] BufferSize   Ignored.
  @param[out] Buffer           Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceStdOutRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  File style interface for console StdErr (Read).

  @param[in] This              Ignored.
  @param[in, out] BufferSize   Ignored.
  @param[out] Buffer           Ignored.

  @retval EFI_UNSUPPORTED Always.
**/
EFI_STATUS
EFIAPI
FileInterfaceStdErrRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  File style interface for NUL file (Read).

  @param[in] This              Ignored.
  @param[in, out] BufferSize   Poiner to 0 upon return.
  @param[out] Buffer           Ignored.

  @retval EFI_SUCCESS Always.
**/
EFI_STATUS
EFIAPI
FileInterfaceNulRead (
  IN      EFI_FILE_PROTOCOL  *This,
  IN OUT  UINTN              *BufferSize,
  OUT     VOID               *Buffer
  )
{
  *BufferSize = 0;
  return (EFI_SUCCESS);
}

/**
  File style interface for NUL file (Write).

  @param[in] This              Ignored.
  @param[in, out] BufferSize   Ignored.
  @param[in] Buffer            Ignored.

  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FileInterfaceNulWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  return (EFI_SUCCESS);
}

/**
  Create the TAB completion list.

  @param[in]  InputString       The command line to expand.
  @param[in]  StringLen         Length of the command line.
  @param[in]  BufferSize        Buffer size.
  @param[in, out] TabCompletionList Return the TAB completion list.
  @param[in, out] TabUpdatePos      Return the TAB update position.
**/
EFI_STATUS
CreateTabCompletionList (
  IN CONST CHAR16             *InputString,
  IN CONST UINTN              StringLen,
  IN CONST UINTN              BufferSize,
  IN OUT EFI_SHELL_FILE_INFO  **TabCompletionList,
  IN OUT   UINTN              *TabUpdatePos
  )
{
  BOOLEAN              InQuotation;
  UINTN                TabPos;
  UINTN                Index;
  CONST CHAR16         *Cwd;
  EFI_STATUS           Status;
  CHAR16               *TabStr;
  EFI_SHELL_FILE_INFO  *FileList;
  EFI_SHELL_FILE_INFO  *FileInfo;
  EFI_SHELL_FILE_INFO  *TempFileInfo;

  //
  // Allocate buffers
  //
  TabStr = AllocateZeroPool (BufferSize);
  if (TabStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // handle auto complete of file and directory names...
  // E.g.: cd fs0:\EFI\Bo<TAB>
  //          ^        ^
  //          TabPos   TabUpdatePos
  //
  TabPos        = 0;
  *TabUpdatePos = 0;
  FileList      = NULL;
  InQuotation   = FALSE;
  for (Index = 0; Index < StringLen; Index++) {
    switch (InputString[Index]) {
      case L'\"':
        InQuotation = (BOOLEAN)(!InQuotation);
        break;

      case L' ':
        if (!InQuotation) {
          TabPos        = Index + 1;
          *TabUpdatePos = TabPos;
        }

        break;

      case L':':
      //
      // handle the case "fs0:<TAB>"
      // Update the TabUpdatePos as well.
      //
      case L'\\':
        *TabUpdatePos = Index + 1;
        break;

      default:
        break;
    }
  }

  if (StrStr (InputString + TabPos, L":") == NULL) {
    //
    // If file path doesn't contain ":", ...
    //
    Cwd = ShellInfoObject.NewEfiShellProtocol->GetCurDir (NULL);
    if (Cwd != NULL) {
      if (InputString[TabPos] != L'\\') {
        //
        // and it doesn't begin with "\\", it's a path relative to current directory.
        // TabStr = "<cwd>\\"
        //
        StrnCpyS (TabStr, BufferSize / sizeof (CHAR16), Cwd, (BufferSize) / sizeof (CHAR16) - 1);
        StrCatS (TabStr, (BufferSize) / sizeof (CHAR16), L"\\");
      } else {
        //
        // and it begins with "\\", it's a path pointing to root directory of current map.
        // TabStr = "fsx:"
        //
        Index = StrStr (Cwd, L":") - Cwd + 1;
        StrnCpyS (TabStr, BufferSize / sizeof (CHAR16), Cwd, Index);
      }
    }
  }

  StrnCatS (TabStr, (BufferSize) / sizeof (CHAR16), InputString + TabPos, StringLen - TabPos);
  StrnCatS (TabStr, (BufferSize) / sizeof (CHAR16), L"*", (BufferSize) / sizeof (CHAR16) - 1 - StrLen (TabStr));
  Status = ShellInfoObject.NewEfiShellProtocol->FindFiles (TabStr, &FileList);

  //
  // Filter out the non-directory for "CD" command
  // Filter "." and ".." for all
  //
  if (!EFI_ERROR (Status) && (FileList != NULL)) {
    //
    // Skip the spaces in the beginning
    //
    while (*InputString == L' ') {
      InputString++;
    }

    for (FileInfo = (EFI_SHELL_FILE_INFO *)GetFirstNode (&FileList->Link); !IsNull (&FileList->Link, &FileInfo->Link); ) {
      if (((StrCmp (FileInfo->FileName, L".") == 0) || (StrCmp (FileInfo->FileName, L"..") == 0)) ||
          ((((InputString[0] == L'c') || (InputString[0] == L'C')) && ((InputString[1] == L'd') || (InputString[1] == L'D'))) &&
           (ShellIsDirectory (FileInfo->FullName) != EFI_SUCCESS)))
      {
        TempFileInfo = FileInfo;
        FileInfo     = (EFI_SHELL_FILE_INFO *)RemoveEntryList (&FileInfo->Link);
        InternalFreeShellFileInfoNode (TempFileInfo);
      } else {
        FileInfo = (EFI_SHELL_FILE_INFO *)GetNextNode (&FileList->Link, &FileInfo->Link);
      }
    }
  }

  if ((FileList != NULL) && !IsListEmpty (&FileList->Link)) {
    Status = EFI_SUCCESS;
  } else {
    ShellInfoObject.NewEfiShellProtocol->FreeFileList (&FileList);
    Status = EFI_NOT_FOUND;
  }

  FreePool (TabStr);

  *TabCompletionList = FileList;
  return Status;
}

/**
  File style interface for console (Read).

  This will return a single line of input from the console.

  @param This           A pointer to the EFI_FILE_PROTOCOL instance that is the
                        file handle to read data from. Not used.
  @param BufferSize     On input, the size of the Buffer. On output, the amount
                        of data returned in Buffer. In both cases, the size is
                        measured in bytes.
  @param Buffer         The buffer into which the data is read.


  @retval EFI_SUCCESS           The data was read.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_DEVICE_ERROR      An attempt was made to read from a deleted file.
  @retval EFI_DEVICE_ERROR      On entry, the current file position is beyond the end of the file.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to read the current directory
                                entry. BufferSize has been updated with the size
                                needed to complete the request.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileInterfaceStdInRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  CHAR16         *CurrentString;
  BOOLEAN        Done;
  UINTN          TabUpdatePos;        // Start index of the string updated by TAB stroke
  UINTN          Column;              // Column of current cursor
  UINTN          Row;                 // Row of current cursor
  UINTN          StartColumn;         // Column at the beginning of the line
  UINTN          Update;              // Line index for update
  UINTN          Delete;              // Num of chars to delete from console after update
  UINTN          StringLen;           // Total length of the line
  UINTN          StringCurPos;        // Line index corresponding to the cursor
  UINTN          MaxStr;              // Maximum possible line length
  UINTN          TotalColumn;         // Num of columns in the console
  UINTN          TotalRow;            // Num of rows in the console
  UINTN          SkipLength;
  UINTN          OutputLength;        // Length of the update string
  UINTN          TailRow;             // Row of end of line
  UINTN          TailColumn;          // Column of end of line
  EFI_INPUT_KEY  Key;

  BUFFER_LIST          *LinePos;
  BUFFER_LIST          *NewPos;
  BOOLEAN              InScrolling;
  EFI_STATUS           Status;
  BOOLEAN              InTabScrolling; // Whether in TAB-completion state
  EFI_SHELL_FILE_INFO  *TabCompleteList;
  EFI_SHELL_FILE_INFO  *TabCurrent;
  UINTN                EventIndex;
  CHAR16               *TabOutputStr;

  //
  // If buffer is not large enough to hold a CHAR16, return minimum buffer size
  //
  if (*BufferSize < sizeof (CHAR16) * 2) {
    *BufferSize = sizeof (CHAR16) * 2;
    return (EFI_BUFFER_TOO_SMALL);
  }

  Done            = FALSE;
  CurrentString   = Buffer;
  StringLen       = 0;
  StringCurPos    = 0;
  OutputLength    = 0;
  Update          = 0;
  Delete          = 0;
  LinePos         = NewPos = (BUFFER_LIST *)(&ShellInfoObject.ViewingSettings.CommandHistory);
  InScrolling     = FALSE;
  InTabScrolling  = FALSE;
  Status          = EFI_SUCCESS;
  TabOutputStr    = NULL;
  TabUpdatePos    = 0;
  TabCompleteList = NULL;
  TabCurrent      = NULL;

  //
  // Get the screen setting and the current cursor location
  //
  Column = StartColumn = gST->ConOut->Mode->CursorColumn;
  Row    = gST->ConOut->Mode->CursorRow;
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &TotalColumn, &TotalRow);

  //
  // Limit the line length to the buffer size or the minimum size of the
  // screen. (The smaller takes effect)
  //
  MaxStr = TotalColumn * (TotalRow - 1) - StartColumn;
  if (MaxStr > *BufferSize / sizeof (CHAR16)) {
    MaxStr = *BufferSize / sizeof (CHAR16);
  }

  ZeroMem (CurrentString, MaxStr * sizeof (CHAR16));
  do {
    //
    // Read a key
    //
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_READY) {
        continue;
      }

      ZeroMem (CurrentString, MaxStr * sizeof (CHAR16));
      StringLen = 0;
      break;
    }

    //
    // Press PageUp or PageDown to scroll the history screen up or down.
    // Press any other key to quit scrolling.
    //
    if ((Key.UnicodeChar == 0) && ((Key.ScanCode == SCAN_PAGE_UP) || (Key.ScanCode == SCAN_PAGE_DOWN))) {
      if (Key.ScanCode == SCAN_PAGE_UP) {
        ConsoleLoggerDisplayHistory (FALSE, 0, ShellInfoObject.ConsoleInfo);
      } else if (Key.ScanCode == SCAN_PAGE_DOWN) {
        ConsoleLoggerDisplayHistory (TRUE, 0, ShellInfoObject.ConsoleInfo);
      }

      InScrolling = TRUE;
    } else {
      if (InScrolling) {
        ConsoleLoggerStopHistory (ShellInfoObject.ConsoleInfo);
        InScrolling = FALSE;
      }
    }

    //
    // If we are quitting TAB scrolling...
    //
    if (InTabScrolling && (Key.UnicodeChar != CHAR_TAB)) {
      if (TabCompleteList != NULL) {
        ShellInfoObject.NewEfiShellProtocol->FreeFileList (&TabCompleteList);
        DEBUG_CODE (
          TabCompleteList = NULL;
          );
      }

      InTabScrolling = FALSE;
    }

    switch (Key.UnicodeChar) {
      case CHAR_CARRIAGE_RETURN:
        //
        // All done, print a newline at the end of the string
        //
        TailRow    = Row + (StringLen - StringCurPos + Column) / TotalColumn;
        TailColumn = (StringLen - StringCurPos + Column) % TotalColumn;
        ShellPrintEx ((INT32)TailColumn, (INT32)TailRow, L"%N\n");
        Done = TRUE;
        break;

      case CHAR_BACKSPACE:
        if (StringCurPos != 0) {
          //
          // If not move back beyond string beginning, move all characters behind
          // the current position one character forward
          //
          StringCurPos--;
          Update = StringCurPos;
          Delete = 1;
          CopyMem (CurrentString + StringCurPos, CurrentString + StringCurPos + 1, sizeof (CHAR16) * (StringLen - StringCurPos));

          //
          // Adjust the current column and row
          //
          MoveCursorBackward (TotalColumn, &Column, &Row);
        }

        break;

      case CHAR_TAB:
        if (!InTabScrolling) {
          TabCurrent = NULL;
          //
          // Initialize a tab complete operation.
          //
          Status = CreateTabCompletionList (CurrentString, StringLen, *BufferSize, &TabCompleteList, &TabUpdatePos);
          if (!EFI_ERROR (Status)) {
            InTabScrolling = TRUE;
          }

          //
          // We do not set up the replacement.
          // The next section will do that.
          //
        }

        if (InTabScrolling) {
          //
          // We are in a tab complete operation.
          // set up the next replacement.
          //
          ASSERT (TabCompleteList != NULL);
          if (TabCurrent == NULL) {
            TabCurrent = (EFI_SHELL_FILE_INFO *)GetFirstNode (&TabCompleteList->Link);
          } else {
            TabCurrent = (EFI_SHELL_FILE_INFO *)GetNextNode (&TabCompleteList->Link, &TabCurrent->Link);
          }

          //
          // Skip over the empty list beginning node
          //
          if (IsNull (&TabCompleteList->Link, &TabCurrent->Link)) {
            TabCurrent = (EFI_SHELL_FILE_INFO *)GetNextNode (&TabCompleteList->Link, &TabCurrent->Link);
          }
        }

        break;

      default:
        if (Key.UnicodeChar >= ' ') {
          //
          // If we are at the buffer's end, drop the key
          //
          if ((StringLen == MaxStr - 1) && (ShellInfoObject.ViewingSettings.InsertMode || (StringCurPos == StringLen))) {
            break;
          }

          //
          // If in insert mode, make space by moving each other character 1
          // space higher in the array
          //
          if (ShellInfoObject.ViewingSettings.InsertMode) {
            CopyMem (CurrentString + StringCurPos + 1, CurrentString + StringCurPos, (StringLen - StringCurPos)*sizeof (CurrentString[0]));
          }

          CurrentString[StringCurPos] = Key.UnicodeChar;
          Update                      = StringCurPos;

          StringCurPos += 1;
          OutputLength  = 1;
        }

        break;

      case 0:
        switch (Key.ScanCode) {
          case SCAN_DELETE:
            //
            // Move characters behind current position one character forward
            //
            if (StringLen != 0) {
              Update = StringCurPos;
              Delete = 1;
              CopyMem (CurrentString + StringCurPos, CurrentString + StringCurPos + 1, sizeof (CHAR16) * (StringLen - StringCurPos));
            }

            break;

          case SCAN_UP:
            //
            // Prepare to print the previous command
            //
            NewPos = (BUFFER_LIST *)GetPreviousNode (&ShellInfoObject.ViewingSettings.CommandHistory.Link, &LinePos->Link);
            if (IsNull (&ShellInfoObject.ViewingSettings.CommandHistory.Link, &LinePos->Link)) {
              NewPos = (BUFFER_LIST *)GetPreviousNode (&ShellInfoObject.ViewingSettings.CommandHistory.Link, &LinePos->Link);
            }

            break;

          case SCAN_DOWN:
            //
            // Prepare to print the next command
            //
            NewPos = (BUFFER_LIST *)GetNextNode (&ShellInfoObject.ViewingSettings.CommandHistory.Link, &LinePos->Link);
            if (NewPos == (BUFFER_LIST *)(&ShellInfoObject.ViewingSettings.CommandHistory)) {
              NewPos = (BUFFER_LIST *)GetNextNode (&ShellInfoObject.ViewingSettings.CommandHistory.Link, &LinePos->Link);
            }

            break;

          case SCAN_LEFT:
            //
            // Adjust current cursor position
            //
            if (StringCurPos != 0) {
              --StringCurPos;
              MoveCursorBackward (TotalColumn, &Column, &Row);
            }

            break;

          case SCAN_RIGHT:
            //
            // Adjust current cursor position
            //
            if (StringCurPos < StringLen) {
              ++StringCurPos;
              MoveCursorForward (TotalColumn, TotalRow, &Column, &Row);
            }

            break;

          case SCAN_HOME:
            //
            // Move current cursor position to the beginning of the command line
            //
            Row         -= (StringCurPos + StartColumn) / TotalColumn;
            Column       = StartColumn;
            StringCurPos = 0;
            break;

          case SCAN_END:
            //
            // Move current cursor position to the end of the command line
            //
            TailRow      = Row + (StringLen - StringCurPos + Column) / TotalColumn;
            TailColumn   = (StringLen - StringCurPos + Column) % TotalColumn;
            Row          = TailRow;
            Column       = TailColumn;
            StringCurPos = StringLen;
            break;

          case SCAN_ESC:
            //
            // Prepare to clear the current command line
            //
            CurrentString[0] = 0;
            Update           = 0;
            Delete           = StringLen;
            Row             -= (StringCurPos + StartColumn) / TotalColumn;
            Column           = StartColumn;
            OutputLength     = 0;
            break;

          case SCAN_INSERT:
            //
            // Toggle the SEnvInsertMode flag
            //
            ShellInfoObject.ViewingSettings.InsertMode = (BOOLEAN) !ShellInfoObject.ViewingSettings.InsertMode;
            break;

          case SCAN_F7:
            //
            // Print command history
            //
            PrintCommandHistory (TotalColumn, TotalRow, 4);
            *CurrentString = CHAR_NULL;
            Done           = TRUE;
            break;
        }
    }

    if (Done) {
      break;
    }

    //
    // If we are in auto-complete mode, we are preparing to print
    // the next file or directory name
    //
    if (InTabScrolling) {
      TabOutputStr = AllocateZeroPool (*BufferSize);
      if (TabOutputStr == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      }
    }

    if (InTabScrolling && (TabOutputStr != NULL)) {
      //
      // Adjust the column and row to the start of TAB-completion string.
      //
      Column       = (StartColumn + TabUpdatePos) % TotalColumn;
      Row         -= (StartColumn + StringCurPos) / TotalColumn - (StartColumn + TabUpdatePos) / TotalColumn;
      OutputLength = StrLen (TabCurrent->FileName);
      //
      // if the output string contains  blank space, quotation marks L'\"'
      // should be added to the output.
      //
      if (StrStr (TabCurrent->FileName, L" ") != NULL) {
        TabOutputStr[0] = L'\"';
        CopyMem (TabOutputStr + 1, TabCurrent->FileName, OutputLength * sizeof (CHAR16));
        TabOutputStr[OutputLength + 1] = L'\"';
        TabOutputStr[OutputLength + 2] = CHAR_NULL;
      } else {
        CopyMem (TabOutputStr, TabCurrent->FileName, OutputLength * sizeof (CHAR16));
        TabOutputStr[OutputLength] = CHAR_NULL;
      }

      OutputLength = StrLen (TabOutputStr) < MaxStr - 1 ? StrLen (TabOutputStr) : MaxStr - 1;
      CopyMem (CurrentString + TabUpdatePos, TabOutputStr, OutputLength * sizeof (CHAR16));
      CurrentString[TabUpdatePos + OutputLength] = CHAR_NULL;
      StringCurPos                               = TabUpdatePos + OutputLength;
      Update                                     = TabUpdatePos;
      if (StringLen > TabUpdatePos + OutputLength) {
        Delete = StringLen - TabUpdatePos - OutputLength;
      }

      FreePool (TabOutputStr);
    }

    //
    // If we have a new position, we are preparing to print a previous or
    // next command.
    //
    if (NewPos != (BUFFER_LIST *)(&ShellInfoObject.ViewingSettings.CommandHistory)) {
      Column = StartColumn;
      Row   -= (StringCurPos + StartColumn) / TotalColumn;

      LinePos = NewPos;
      NewPos  = (BUFFER_LIST *)(&ShellInfoObject.ViewingSettings.CommandHistory);

      OutputLength = StrLen (LinePos->Buffer) < MaxStr - 1 ? StrLen (LinePos->Buffer) : MaxStr - 1;
      CopyMem (CurrentString, LinePos->Buffer, OutputLength * sizeof (CHAR16));
      CurrentString[OutputLength] = CHAR_NULL;

      StringCurPos = OutputLength;

      //
      // Draw new input string
      //
      Update = 0;
      if (StringLen > OutputLength) {
        //
        // If old string was longer, blank its tail
        //
        Delete = StringLen - OutputLength;
      }
    }

    //
    // If we need to update the output do so now
    //
    if (Update != (UINTN)-1) {
      ShellPrintEx ((INT32)Column, (INT32)Row, L"%s%.*s", CurrentString + Update, Delete, L"");
      StringLen = StrLen (CurrentString);

      if (Delete != 0) {
        SetMem (CurrentString + StringLen, Delete * sizeof (CHAR16), CHAR_NULL);
      }

      if (StringCurPos > StringLen) {
        StringCurPos = StringLen;
      }

      Update = (UINTN)-1;

      //
      // After using print to reflect newly updates, if we're not using
      // BACKSPACE and DELETE, we need to move the cursor position forward,
      // so adjust row and column here.
      //
      if ((Key.UnicodeChar != CHAR_BACKSPACE) && !((Key.UnicodeChar == 0) && (Key.ScanCode == SCAN_DELETE))) {
        //
        // Calculate row and column of the tail of current string
        //
        TailRow    = Row + (StringLen - StringCurPos + Column + OutputLength) / TotalColumn;
        TailColumn = (StringLen - StringCurPos + Column + OutputLength) % TotalColumn;

        //
        // If the tail of string reaches screen end, screen rolls up, so if
        // Row does not equal TailRow, Row should be decremented
        //
        // (if we are recalling commands using UPPER and DOWN key, and if the
        // old command is too long to fit the screen, TailColumn must be 79.
        //
        if ((TailColumn == 0) && (TailRow >= TotalRow) && (Row != TailRow)) {
          Row--;
        }

        //
        // Calculate the cursor position after current operation. If cursor
        // reaches line end, update both row and column, otherwise, only
        // column will be changed.
        //
        if (Column + OutputLength >= TotalColumn) {
          SkipLength = OutputLength - (TotalColumn - Column);

          Row += SkipLength / TotalColumn + 1;
          if (Row > TotalRow - 1) {
            Row = TotalRow - 1;
          }

          Column = SkipLength % TotalColumn;
        } else {
          Column += OutputLength;
        }
      }

      Delete = 0;
    }

    //
    // Set the cursor position for this key
    //
    gST->ConOut->SetCursorPosition (gST->ConOut, Column, Row);
  } while (!Done);

  if ((CurrentString != NULL) && (StrLen (CurrentString) > 0)) {
    //
    // add the line to the history buffer
    //
    AddLineToCommandHistory (CurrentString);
  }

  //
  // Return the data to the caller
  //
  *BufferSize = StringLen * sizeof (CHAR16);

  //
  // if this was used it should be deallocated by now...
  // prevent memory leaks...
  //
  if (TabCompleteList != NULL) {
    ShellInfoObject.NewEfiShellProtocol->FreeFileList (&TabCompleteList);
  }

  ASSERT (TabCompleteList == NULL);

  return Status;
}

//
// FILE style interfaces for StdIn/StdOut/StdErr
//
EFI_FILE_PROTOCOL  FileInterfaceStdIn = {
  EFI_FILE_REVISION,
  FileInterfaceOpenNotFound,
  FileInterfaceNopGeneric,
  FileInterfaceNopGeneric,
  FileInterfaceStdInRead,
  FileInterfaceStdInWrite,
  FileInterfaceNopGetPosition,
  FileInterfaceNopSetPosition,
  FileInterfaceNopGetInfo,
  FileInterfaceNopSetInfo,
  FileInterfaceNopGeneric
};

EFI_FILE_PROTOCOL  FileInterfaceStdOut = {
  EFI_FILE_REVISION,
  FileInterfaceOpenNotFound,
  FileInterfaceNopGeneric,
  FileInterfaceNopGeneric,
  FileInterfaceStdOutRead,
  FileInterfaceStdOutWrite,
  FileInterfaceNopGetPosition,
  FileInterfaceNopSetPosition,
  FileInterfaceNopGetInfo,
  FileInterfaceNopSetInfo,
  FileInterfaceNopGeneric
};

EFI_FILE_PROTOCOL  FileInterfaceStdErr = {
  EFI_FILE_REVISION,
  FileInterfaceOpenNotFound,
  FileInterfaceNopGeneric,
  FileInterfaceNopGeneric,
  FileInterfaceStdErrRead,
  FileInterfaceStdErrWrite,
  FileInterfaceNopGetPosition,
  FileInterfaceNopSetPosition,
  FileInterfaceNopGetInfo,
  FileInterfaceNopSetInfo,
  FileInterfaceNopGeneric
};

EFI_FILE_PROTOCOL  FileInterfaceNulFile = {
  EFI_FILE_REVISION,
  FileInterfaceOpenNotFound,
  FileInterfaceNopGeneric,
  FileInterfaceNopGeneric,
  FileInterfaceNulRead,
  FileInterfaceNulWrite,
  FileInterfaceNopGetPosition,
  FileInterfaceNopSetPosition,
  FileInterfaceNopGetInfo,
  FileInterfaceNopSetInfo,
  FileInterfaceNopGeneric
};

//
// This is identical to EFI_FILE_PROTOCOL except for the additional member
// for the name.
//

typedef struct {
  UINT64                   Revision;
  EFI_FILE_OPEN            Open;
  EFI_FILE_CLOSE           Close;
  EFI_FILE_DELETE          Delete;
  EFI_FILE_READ            Read;
  EFI_FILE_WRITE           Write;
  EFI_FILE_GET_POSITION    GetPosition;
  EFI_FILE_SET_POSITION    SetPosition;
  EFI_FILE_GET_INFO        GetInfo;
  EFI_FILE_SET_INFO        SetInfo;
  EFI_FILE_FLUSH           Flush;
  CHAR16                   Name[1];
} EFI_FILE_PROTOCOL_ENVIRONMENT;
// ANSI compliance helper to get size of the struct.
#define SIZE_OF_EFI_FILE_PROTOCOL_ENVIRONMENT  EFI_FIELD_OFFSET (EFI_FILE_PROTOCOL_ENVIRONMENT, Name)

/**
  File style interface for Environment Variable (Close).

  Frees the memory for this object.

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.

  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FileInterfaceEnvClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  VOID        *NewBuffer;
  UINTN       NewSize;
  EFI_STATUS  Status;
  BOOLEAN     Volatile;
  UINTN       TotalSize;

  //
  // Most if not all UEFI commands will have an '\r\n' at the end of any output.
  // Since the output was redirected to a variable, it does not make sense to
  // keep this.  So, before closing, strip the trailing '\r\n' from the variable
  // if it exists.
  //
  NewBuffer = NULL;
  NewSize   = 0;
  TotalSize = 0;

  Status = IsVolatileEnv (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name, &Volatile);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SHELL_GET_ENVIRONMENT_VARIABLE (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name, &NewSize, NewBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    TotalSize = NewSize + sizeof (CHAR16);
    NewBuffer = AllocateZeroPool (TotalSize);
    if (NewBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SHELL_GET_ENVIRONMENT_VARIABLE (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name, &NewSize, NewBuffer);
  }

  if (!EFI_ERROR (Status) && (NewBuffer != NULL)) {
    if (TotalSize / sizeof (CHAR16) >= 3) {
      if ((((CHAR16 *)NewBuffer)[TotalSize / sizeof (CHAR16) - 2] == CHAR_LINEFEED) &&
          (((CHAR16 *)NewBuffer)[TotalSize / sizeof (CHAR16) - 3] == CHAR_CARRIAGE_RETURN)
          )
      {
        ((CHAR16 *)NewBuffer)[TotalSize / sizeof (CHAR16) - 3] = CHAR_NULL;
        //
        // If the NewBuffer end with \r\n\0, We will replace '\r' by '\0' and then update TotalSize.
        //
        TotalSize -= sizeof (CHAR16) * 2;
      }

      if (Volatile) {
        Status = SHELL_SET_ENVIRONMENT_VARIABLE_V (
                   ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
                   TotalSize - sizeof (CHAR16),
                   NewBuffer
                   );

        if (!EFI_ERROR (Status)) {
          Status = ShellAddEnvVarToList (
                     ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
                     NewBuffer,
                     TotalSize,
                     EFI_VARIABLE_BOOTSERVICE_ACCESS
                     );
        }
      } else {
        Status = SHELL_SET_ENVIRONMENT_VARIABLE_NV (
                   ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
                   TotalSize - sizeof (CHAR16),
                   NewBuffer
                   );

        if (!EFI_ERROR (Status)) {
          Status = ShellAddEnvVarToList (
                     ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
                     NewBuffer,
                     TotalSize,
                     EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS
                     );
        }
      }
    }
  }

  SHELL_FREE_NON_NULL (NewBuffer);
  FreePool ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This);
  return (Status);
}

/**
  File style interface for Environment Variable (Delete).

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.

  @retval The return value from FileInterfaceEnvClose().
**/
EFI_STATUS
EFIAPI
FileInterfaceEnvDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  SHELL_DELETE_ENVIRONMENT_VARIABLE (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name);
  return (FileInterfaceEnvClose (This));
}

/**
  File style interface for Environment Variable (Read).

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[out] Buffer           The pointer to the buffer to fill.

  @retval EFI_SUCCESS   The data was read.
**/
EFI_STATUS
EFIAPI
FileInterfaceEnvRead (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  OUT    VOID               *Buffer
  )
{
  EFI_STATUS  Status;

  *BufferSize = *BufferSize / sizeof (CHAR16) * sizeof (CHAR16);
  if (*BufferSize != 0) {
    //
    // Make sure the first unicode character is \xFEFF
    //
    *(CHAR16 *)Buffer = gUnicodeFileTag;
    Buffer            = (CHAR16 *)Buffer + 1;
    *BufferSize      -= sizeof (gUnicodeFileTag);
  }

  Status = SHELL_GET_ENVIRONMENT_VARIABLE (
             ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
             BufferSize,
             Buffer
             );
  if (!EFI_ERROR (Status) || (Status == EFI_BUFFER_TOO_SMALL)) {
    //
    // BufferSize is valid and needs update when Status is Success or BufferTooSmall.
    //
    *BufferSize += sizeof (gUnicodeFileTag);
  }

  return Status;
}

/**
  File style interface for Volatile Environment Variable (Write).
  This function also caches the environment variable into gShellEnvVarList.

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to write.

  @retval EFI_SUCCESS             The data was successfully write to variable.
  @retval SHELL_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileInterfaceEnvVolWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  VOID        *NewBuffer;
  UINTN       NewSize;
  EFI_STATUS  Status;
  UINTN       TotalSize;

  NewBuffer = NULL;
  NewSize   = 0;
  TotalSize = 0;

  Status = SHELL_GET_ENVIRONMENT_VARIABLE (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name, &NewSize, NewBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    TotalSize = NewSize + *BufferSize + sizeof (CHAR16);
  } else if (Status == EFI_NOT_FOUND) {
    TotalSize = *BufferSize + sizeof (CHAR16);
  } else {
    return Status;
  }

  NewBuffer = AllocateZeroPool (TotalSize);
  if (NewBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    Status = SHELL_GET_ENVIRONMENT_VARIABLE (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name, &NewSize, NewBuffer);
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    FreePool (NewBuffer);
    return Status;
  }

  CopyMem ((UINT8 *)NewBuffer + NewSize, Buffer, *BufferSize);
  Status = ShellAddEnvVarToList (
             ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
             NewBuffer,
             TotalSize,
             EFI_VARIABLE_BOOTSERVICE_ACCESS
             );
  if (EFI_ERROR (Status)) {
    FreePool (NewBuffer);
    return Status;
  }

  Status = SHELL_SET_ENVIRONMENT_VARIABLE_V (
             ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
             TotalSize - sizeof (CHAR16),
             NewBuffer
             );
  if (EFI_ERROR (Status)) {
    ShellRemvoeEnvVarFromList (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name);
  }

  FreePool (NewBuffer);
  return Status;
}

/**
  File style interface for Non Volatile Environment Variable (Write).
  This function also caches the environment variable into gShellEnvVarList.

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to write.

  @retval EFI_SUCCESS             The data was successfully write to variable.
  @retval SHELL_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileInterfaceEnvNonVolWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  VOID        *NewBuffer;
  UINTN       NewSize;
  EFI_STATUS  Status;
  UINTN       TotalSize;

  NewBuffer = NULL;
  NewSize   = 0;
  TotalSize = 0;

  Status = SHELL_GET_ENVIRONMENT_VARIABLE (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name, &NewSize, NewBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    TotalSize = NewSize + *BufferSize + sizeof (CHAR16);
  } else if (Status == EFI_NOT_FOUND) {
    TotalSize = *BufferSize + sizeof (CHAR16);
  } else {
    return Status;
  }

  NewBuffer = AllocateZeroPool (TotalSize);
  if (NewBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    Status = SHELL_GET_ENVIRONMENT_VARIABLE (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name, &NewSize, NewBuffer);
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    FreePool (NewBuffer);
    return Status;
  }

  CopyMem ((UINT8 *)NewBuffer + NewSize, Buffer, *BufferSize);
  Status = ShellAddEnvVarToList (
             ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
             NewBuffer,
             TotalSize,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS
             );
  if (EFI_ERROR (Status)) {
    FreePool (NewBuffer);
    return Status;
  }

  Status = SHELL_SET_ENVIRONMENT_VARIABLE_NV (
             ((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name,
             TotalSize - sizeof (CHAR16),
             NewBuffer
             );
  if (EFI_ERROR (Status)) {
    ShellRemvoeEnvVarFromList (((EFI_FILE_PROTOCOL_ENVIRONMENT *)This)->Name);
  }

  FreePool (NewBuffer);
  return Status;
}

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  environment variables through file operations.

  @param EnvName    The name of the Environment Variable to be operated on.

  @retval NULL      Memory could not be allocated.
  @return other     a pointer to an EFI_FILE_PROTOCOL structure
**/
EFI_FILE_PROTOCOL *
CreateFileInterfaceEnv (
  IN CONST CHAR16  *EnvName
  )
{
  EFI_STATUS                     Status;
  EFI_FILE_PROTOCOL_ENVIRONMENT  *EnvFileInterface;
  UINTN                          EnvNameSize;
  BOOLEAN                        Volatile;

  if (EnvName == NULL) {
    return (NULL);
  }

  Status = IsVolatileEnv (EnvName, &Volatile);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get some memory
  //
  EnvNameSize      = StrSize (EnvName);
  EnvFileInterface = AllocateZeroPool (sizeof (EFI_FILE_PROTOCOL_ENVIRONMENT)+EnvNameSize);
  if (EnvFileInterface == NULL) {
    return (NULL);
  }

  //
  // Assign the generic members
  //
  EnvFileInterface->Revision    = EFI_FILE_REVISION;
  EnvFileInterface->Open        = FileInterfaceOpenNotFound;
  EnvFileInterface->Close       = FileInterfaceEnvClose;
  EnvFileInterface->GetPosition = FileInterfaceNopGetPosition;
  EnvFileInterface->SetPosition = FileInterfaceNopSetPosition;
  EnvFileInterface->GetInfo     = FileInterfaceNopGetInfo;
  EnvFileInterface->SetInfo     = FileInterfaceNopSetInfo;
  EnvFileInterface->Flush       = FileInterfaceNopGeneric;
  EnvFileInterface->Delete      = FileInterfaceEnvDelete;
  EnvFileInterface->Read        = FileInterfaceEnvRead;

  CopyMem (EnvFileInterface->Name, EnvName, EnvNameSize);

  //
  // Assign the different members for Volatile and Non-Volatile variables
  //
  if (Volatile) {
    EnvFileInterface->Write = FileInterfaceEnvVolWrite;
  } else {
    EnvFileInterface->Write = FileInterfaceEnvNonVolWrite;
  }

  return ((EFI_FILE_PROTOCOL *)EnvFileInterface);
}

/**
  Move the cursor position one character backward.

  @param[in] LineLength       Length of a line. Get it by calling QueryMode
  @param[in, out] Column      Current column of the cursor position
  @param[in, out] Row         Current row of the cursor position
**/
VOID
MoveCursorBackward (
  IN     UINTN  LineLength,
  IN OUT UINTN  *Column,
  IN OUT UINTN  *Row
  )
{
  //
  // If current column is 0, move to the last column of the previous line,
  // otherwise, just decrement column.
  //
  if (*Column == 0) {
    *Column = LineLength - 1;
    if (*Row > 0) {
      (*Row)--;
    }

    return;
  }

  (*Column)--;
}

/**
  Move the cursor position one character forward.

  @param[in] LineLength       Length of a line.
  @param[in] TotalRow         Total row of a screen
  @param[in, out] Column      Current column of the cursor position
  @param[in, out] Row         Current row of the cursor position
**/
VOID
MoveCursorForward (
  IN     UINTN  LineLength,
  IN     UINTN  TotalRow,
  IN OUT UINTN  *Column,
  IN OUT UINTN  *Row
  )
{
  //
  // Increment Column.
  // If this puts column past the end of the line, move to first column
  // of the next row.
  //
  (*Column)++;
  if (*Column >= LineLength) {
    (*Column) = 0;
    if ((*Row) < TotalRow - 1) {
      (*Row)++;
    }
  }
}

/**
  Prints out each previously typed command in the command list history log.

  When each screen is full it will pause for a key before continuing.

  @param[in] TotalCols    How many columns are on the screen
  @param[in] TotalRows    How many rows are on the screen
  @param[in] StartColumn  which column to start at
**/
VOID
PrintCommandHistory (
  IN CONST UINTN  TotalCols,
  IN CONST UINTN  TotalRows,
  IN CONST UINTN  StartColumn
  )
{
  BUFFER_LIST  *Node;
  UINTN        Index;
  UINTN        LineNumber;
  UINTN        LineCount;

  ShellPrintEx (-1, -1, L"\n");
  Index      = 0;
  LineNumber = 0;
  //
  // go through history list...
  //
  for ( Node = (BUFFER_LIST *)GetFirstNode (&ShellInfoObject.ViewingSettings.CommandHistory.Link)
        ; !IsNull (&ShellInfoObject.ViewingSettings.CommandHistory.Link, &Node->Link)
        ; Node = (BUFFER_LIST *)GetNextNode (&ShellInfoObject.ViewingSettings.CommandHistory.Link, &Node->Link)
        )
  {
    Index++;
    LineCount = ((StrLen (Node->Buffer) + StartColumn + 1) / TotalCols) + 1;

    if (LineNumber + LineCount >= TotalRows) {
      ShellPromptForResponseHii (
        ShellPromptResponseTypeEnterContinue,
        STRING_TOKEN (STR_SHELL_ENTER_TO_CONT),
        ShellInfoObject.HiiHandle,
        NULL
        );
      LineNumber = 0;
    }

    ShellPrintEx (-1, -1, L"%2d. %s\n", Index, Node->Buffer);
    LineNumber += LineCount;
  }
}

//
// This is identical to EFI_FILE_PROTOCOL except for the additional members
// for the buffer, size, and position.
//

typedef struct {
  UINT64                   Revision;
  EFI_FILE_OPEN            Open;
  EFI_FILE_CLOSE           Close;
  EFI_FILE_DELETE          Delete;
  EFI_FILE_READ            Read;
  EFI_FILE_WRITE           Write;
  EFI_FILE_GET_POSITION    GetPosition;
  EFI_FILE_SET_POSITION    SetPosition;
  EFI_FILE_GET_INFO        GetInfo;
  EFI_FILE_SET_INFO        SetInfo;
  EFI_FILE_FLUSH           Flush;
  VOID                     *Buffer;
  UINT64                   Position;
  UINT64                   BufferSize;
  BOOLEAN                  Unicode;
  UINT64                   FileSize;
} EFI_FILE_PROTOCOL_MEM;

/**
  File style interface for Mem (SetPosition).

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.
  @param[out] Position  The position to set.

  @retval EFI_SUCCESS             The position was successfully changed.
  @retval EFI_INVALID_PARAMETER   The Position was invalid.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            Position
  )
{
  if (Position <= ((EFI_FILE_PROTOCOL_MEM *)This)->FileSize) {
    ((EFI_FILE_PROTOCOL_MEM *)This)->Position = Position;
    return (EFI_SUCCESS);
  } else {
    return (EFI_INVALID_PARAMETER);
  }
}

/**
  File style interface for Mem (GetPosition).

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.
  @param[out] Position  The pointer to the position.

  @retval EFI_SUCCESS   The position was retrieved.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemGetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  )
{
  *Position = ((EFI_FILE_PROTOCOL_MEM *)This)->Position;
  return (EFI_SUCCESS);
}

/**
  File style interface for Mem (GetInfo).

  @param  This            Protocol instance pointer.
  @param  InformationType Type of information to return in Buffer.
  @param  BufferSize      On input size of buffer, on output amount of data in buffer.
  @param  Buffer          The buffer to return data.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORT        InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_BUFFER_TOO_SMALL Buffer was too small; required size returned in BufferSize.

**/
EFI_STATUS
EFIAPI
FileInterfaceMemGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  EFI_FILE_INFO  *FileInfo;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    if (*BufferSize < sizeof (EFI_FILE_INFO)) {
      *BufferSize = sizeof (EFI_FILE_INFO);
      return EFI_BUFFER_TOO_SMALL;
    }

    if (Buffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    FileInfo       = (EFI_FILE_INFO *)Buffer;
    FileInfo->Size = sizeof (*FileInfo);
    ZeroMem (FileInfo, sizeof (*FileInfo));
    FileInfo->FileSize     = ((EFI_FILE_PROTOCOL_MEM *)This)->FileSize;
    FileInfo->PhysicalSize = FileInfo->FileSize;
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  File style interface for Mem (Write).

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to write.

  @retval EFI_OUT_OF_RESOURCES The operation failed due to lack of resources.
  @retval EFI_SUCCESS          The data was written.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  CHAR8                  *AsciiBuffer;
  EFI_FILE_PROTOCOL_MEM  *MemFile;

  MemFile = (EFI_FILE_PROTOCOL_MEM *)This;
  if (MemFile->Unicode) {
    //
    // Unicode
    //
    if ((UINTN)(MemFile->Position + (*BufferSize)) > (UINTN)(MemFile->BufferSize)) {
      MemFile->Buffer = ReallocatePool ((UINTN)(MemFile->BufferSize), (UINTN)(MemFile->BufferSize) + (*BufferSize) + MEM_WRITE_REALLOC_OVERHEAD, MemFile->Buffer);
      if (MemFile->Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      MemFile->BufferSize += (*BufferSize) + MEM_WRITE_REALLOC_OVERHEAD;
    }

    CopyMem (((UINT8 *)MemFile->Buffer) + MemFile->Position, Buffer, *BufferSize);
    MemFile->Position += (*BufferSize);
    MemFile->FileSize  = MemFile->Position;
    return (EFI_SUCCESS);
  } else {
    //
    // Ascii
    //
    AsciiBuffer = AllocateZeroPool (*BufferSize);
    if (AsciiBuffer == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }

    AsciiSPrint (AsciiBuffer, *BufferSize, "%S", Buffer);
    if ((UINTN)(MemFile->Position + AsciiStrSize (AsciiBuffer)) > (UINTN)(MemFile->BufferSize)) {
      MemFile->Buffer = ReallocatePool ((UINTN)(MemFile->BufferSize), (UINTN)(MemFile->BufferSize) + AsciiStrSize (AsciiBuffer) + MEM_WRITE_REALLOC_OVERHEAD, MemFile->Buffer);
      if (MemFile->Buffer == NULL) {
        FreePool (AsciiBuffer);
        return EFI_OUT_OF_RESOURCES;
      }

      MemFile->BufferSize += AsciiStrSize (AsciiBuffer) + MEM_WRITE_REALLOC_OVERHEAD;
    }

    CopyMem (((UINT8 *)MemFile->Buffer) + MemFile->Position, AsciiBuffer, AsciiStrSize (AsciiBuffer));
    MemFile->Position += (*BufferSize / sizeof (CHAR16));
    MemFile->FileSize  = MemFile->Position;
    FreePool (AsciiBuffer);
    return (EFI_SUCCESS);
  }
}

/**
  File style interface for Mem (Read).

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to fill.

  @retval EFI_SUCCESS   The data was read.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  EFI_FILE_PROTOCOL_MEM  *MemFile;

  MemFile = (EFI_FILE_PROTOCOL_MEM *)This;
  if (*BufferSize > (UINTN)((MemFile->FileSize) - (UINTN)(MemFile->Position))) {
    (*BufferSize) = (UINTN)((MemFile->FileSize) - (UINTN)(MemFile->Position));
  }

  CopyMem (Buffer, ((UINT8 *)MemFile->Buffer) + MemFile->Position, (*BufferSize));
  MemFile->Position = MemFile->Position + (*BufferSize);
  return (EFI_SUCCESS);
}

/**
  File style interface for Mem (Close).

  Frees all memory associated with this object.

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.

  @retval EFI_SUCCESS   The 'file' was closed.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  SHELL_FREE_NON_NULL (((EFI_FILE_PROTOCOL_MEM *)This)->Buffer);
  SHELL_FREE_NON_NULL (This);
  return (EFI_SUCCESS);
}

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  a file entirely in memory through file operations.

  @param[in] Unicode Boolean value with TRUE for Unicode and FALSE for Ascii.

  @retval NULL      Memory could not be allocated.
  @return other     A pointer to an EFI_FILE_PROTOCOL structure.
**/
EFI_FILE_PROTOCOL *
CreateFileInterfaceMem (
  IN CONST BOOLEAN  Unicode
  )
{
  EFI_FILE_PROTOCOL_MEM  *FileInterface;

  //
  // Get some memory
  //
  FileInterface = AllocateZeroPool (sizeof (EFI_FILE_PROTOCOL_MEM));
  if (FileInterface == NULL) {
    return (NULL);
  }

  //
  // Assign the generic members
  //
  FileInterface->Revision    = EFI_FILE_REVISION;
  FileInterface->Open        = FileInterfaceOpenNotFound;
  FileInterface->Close       = FileInterfaceMemClose;
  FileInterface->GetPosition = FileInterfaceMemGetPosition;
  FileInterface->SetPosition = FileInterfaceMemSetPosition;
  FileInterface->GetInfo     = FileInterfaceMemGetInfo;
  FileInterface->SetInfo     = FileInterfaceNopSetInfo;
  FileInterface->Flush       = FileInterfaceNopGeneric;
  FileInterface->Delete      = FileInterfaceNopGeneric;
  FileInterface->Read        = FileInterfaceMemRead;
  FileInterface->Write       = FileInterfaceMemWrite;
  FileInterface->Unicode     = Unicode;

  ASSERT (FileInterface->Buffer      == NULL);
  ASSERT (FileInterface->BufferSize  == 0);
  ASSERT (FileInterface->Position    == 0);

  if (Unicode) {
    FileInterface->Buffer = AllocateZeroPool (sizeof (gUnicodeFileTag));
    if (FileInterface->Buffer == NULL) {
      FreePool (FileInterface);
      return NULL;
    }

    *((CHAR16 *)(FileInterface->Buffer)) = EFI_UNICODE_BYTE_ORDER_MARK;
    FileInterface->BufferSize            = 2;
    FileInterface->Position              = 2;
  }

  return ((EFI_FILE_PROTOCOL *)FileInterface);
}

typedef struct {
  UINT64                   Revision;
  EFI_FILE_OPEN            Open;
  EFI_FILE_CLOSE           Close;
  EFI_FILE_DELETE          Delete;
  EFI_FILE_READ            Read;
  EFI_FILE_WRITE           Write;
  EFI_FILE_GET_POSITION    GetPosition;
  EFI_FILE_SET_POSITION    SetPosition;
  EFI_FILE_GET_INFO        GetInfo;
  EFI_FILE_SET_INFO        SetInfo;
  EFI_FILE_FLUSH           Flush;
  BOOLEAN                  Unicode;
  EFI_FILE_PROTOCOL        *Orig;
} EFI_FILE_PROTOCOL_FILE;

/**
  Set a files current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open.

**/
EFI_STATUS
EFIAPI
FileInterfaceFileSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  return ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->SetPosition (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, Position);
}

/**
  Get a file's current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Data was written.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open..

**/
EFI_STATUS
EFIAPI
FileInterfaceFileGetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  )
{
  return ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->GetPosition (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, Position);
}

/**
  Get information about a file.

  @param  This            Protocol instance pointer.
  @param  InformationType Type of information to return in Buffer.
  @param  BufferSize      On input size of buffer, on output amount of data in buffer.
  @param  Buffer          The buffer to return data.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORT        InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_BUFFER_TOO_SMALL Buffer was too small; required size returned in BufferSize.

**/
EFI_STATUS
EFIAPI
FileInterfaceFileGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  return ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->GetInfo (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, InformationType, BufferSize, Buffer);
}

/**
  Set information about a file

  @param  This            Protocol instance pointer.
  @param  InformationType Type of information in Buffer.
  @param  BufferSize      Size of buffer.
  @param  Buffer          The data to write.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORT        InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.

**/
EFI_STATUS
EFIAPI
FileInterfaceFileSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  return ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->SetInfo (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, InformationType, BufferSize, Buffer);
}

/**
  Flush data back for the file handle.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORT        Writes to Open directory are not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
FileInterfaceFileFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  return ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Flush (((EFI_FILE_PROTOCOL_FILE *)This)->Orig);
}

/**
  Read data from the file.

  @param  This       Protocol instance pointer.
  @param  BufferSize On input size of buffer, on output amount of data in buffer.
  @param  Buffer     The buffer in which data is read.

  @retval EFI_SUCCESS          Data was read.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL  BufferSize is too small. BufferSize contains required size.

**/
EFI_STATUS
EFIAPI
FileInterfaceFileRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  EFI_STATUS  Status;
  UINT64      Position;
  CHAR8       *AsciiStrBuffer;
  CHAR16      *UscStrBuffer;
  UINTN       Size;

  if (((EFI_FILE_PROTOCOL_FILE *)This)->Unicode) {
    //
    // Unicode
    // There might be different file tag for the Unicode file. We cannot unconditionally insert the \xFEFF.
    // So we choose to leave the file content as is.
    //
    return (((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Read (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, BufferSize, Buffer));
  } else {
    //
    // Ascii
    //
    *BufferSize = *BufferSize / sizeof (CHAR16) * sizeof (CHAR16);
    if (*BufferSize == 0) {
      return EFI_SUCCESS;
    }

    Status = ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->GetPosition (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, &Position);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Position == 0) {
      //
      // First two bytes in Buffer is for the Unicode file tag.
      //
      *(CHAR16 *)Buffer = gUnicodeFileTag;
      Buffer            = (CHAR16 *)Buffer + 1;
      Size              = *BufferSize / sizeof (CHAR16) - 1;
    } else {
      Size = *BufferSize / sizeof (CHAR16);
    }

    AsciiStrBuffer = AllocateZeroPool (Size + 1);
    if (AsciiStrBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    UscStrBuffer = AllocateZeroPool ((Size + 1) * sizeof (CHAR16));
    if (UscStrBuffer == NULL) {
      SHELL_FREE_NON_NULL (AsciiStrBuffer);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Read (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, &Size, AsciiStrBuffer);
    if (!EFI_ERROR (Status)) {
      AsciiStrToUnicodeStrS (AsciiStrBuffer, UscStrBuffer, Size + 1);
      *BufferSize = Size * sizeof (CHAR16);
      CopyMem (Buffer, UscStrBuffer, *BufferSize);
    }

    SHELL_FREE_NON_NULL (AsciiStrBuffer);
    SHELL_FREE_NON_NULL (UscStrBuffer);
    return Status;
  }
}

/**
  Opens a new file relative to the source file's location.

  @param[in]  This       The protocol instance pointer.
  @param[out]  NewHandle Returns File Handle for FileName.
  @param[in]  FileName   Null terminated string. "\", ".", and ".." are supported.
  @param[in]  OpenMode   Open mode for file.
  @param[in]  Attributes Only used for EFI_FILE_MODE_CREATE.

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_NOT_FOUND        The specified file could not be found on the device.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_MEDIA_CHANGED    The media has changed.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources.
  @retval EFI_VOLUME_FULL      The volume is full.
**/
EFI_STATUS
EFIAPI
FileInterfaceFileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  )
{
  return ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Open (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, NewHandle, FileName, OpenMode, Attributes);
}

/**
  Close and delete the file handle.

  @param  This                     Protocol instance pointer.

  @retval EFI_SUCCESS              The device was opened.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed but the file was not deleted.

**/
EFI_STATUS
EFIAPI
FileInterfaceFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  Status = ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Delete (((EFI_FILE_PROTOCOL_FILE *)This)->Orig);
  FreePool (This);
  return (Status);
}

/**
  File style interface for File (Close).

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.

  @retval EFI_SUCCESS   The file was closed.
**/
EFI_STATUS
EFIAPI
FileInterfaceFileClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;

  Status = ((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Close (((EFI_FILE_PROTOCOL_FILE *)This)->Orig);
  FreePool (This);
  return (Status);
}

/**
  File style interface for File (Write).

  If the file was opened with ASCII mode the data will be processed through
  AsciiSPrint before writing.

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to write.

  @retval EFI_SUCCESS   The data was written.
**/
EFI_STATUS
EFIAPI
FileInterfaceFileWrite (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  )
{
  CHAR8       *AsciiBuffer;
  UINTN       Size;
  EFI_STATUS  Status;

  if (((EFI_FILE_PROTOCOL_FILE *)This)->Unicode) {
    //
    // Unicode
    //
    return (((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Write (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, BufferSize, Buffer));
  } else {
    //
    // Ascii
    //
    AsciiBuffer = AllocateZeroPool (*BufferSize);
    AsciiSPrint (AsciiBuffer, *BufferSize, "%S", Buffer);
    Size   = AsciiStrSize (AsciiBuffer) - 1; // (we dont need the null terminator)
    Status = (((EFI_FILE_PROTOCOL_FILE *)This)->Orig->Write (((EFI_FILE_PROTOCOL_FILE *)This)->Orig, &Size, AsciiBuffer));
    FreePool (AsciiBuffer);
    return (Status);
  }
}

/**
  Create a file interface with unicode information.

  This will create a new EFI_FILE_PROTOCOL identical to the Templace
  except that the new one has Unicode and Ascii knowledge.

  @param[in] Template   A pointer to the EFI_FILE_PROTOCOL object.
  @param[in] Unicode    TRUE for UCS-2, FALSE for ASCII.

  @return a new EFI_FILE_PROTOCOL object to be used instead of the template.
**/
EFI_FILE_PROTOCOL *
CreateFileInterfaceFile (
  IN CONST EFI_FILE_PROTOCOL  *Template,
  IN CONST BOOLEAN            Unicode
  )
{
  EFI_FILE_PROTOCOL_FILE  *NewOne;

  NewOne = AllocateZeroPool (sizeof (EFI_FILE_PROTOCOL_FILE));
  if (NewOne == NULL) {
    return (NULL);
  }

  CopyMem (NewOne, Template, sizeof (EFI_FILE_PROTOCOL_FILE));
  NewOne->Orig        = (EFI_FILE_PROTOCOL *)Template;
  NewOne->Unicode     = Unicode;
  NewOne->Open        = FileInterfaceFileOpen;
  NewOne->Close       = FileInterfaceFileClose;
  NewOne->Delete      = FileInterfaceFileDelete;
  NewOne->Read        = FileInterfaceFileRead;
  NewOne->Write       = FileInterfaceFileWrite;
  NewOne->GetPosition = FileInterfaceFileGetPosition;
  NewOne->SetPosition = FileInterfaceFileSetPosition;
  NewOne->GetInfo     = FileInterfaceFileGetInfo;
  NewOne->SetInfo     = FileInterfaceFileSetInfo;
  NewOne->Flush       = FileInterfaceFileFlush;

  return ((EFI_FILE_PROTOCOL *)NewOne);
}
