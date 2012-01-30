/** @file
  Implements filebuffer interface functions.

  Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TextEditor.h"
#include <Guid/FileSystemInfo.h>
#include <Library/FileHandleLib.h>

EFI_EDITOR_FILE_BUFFER  FileBuffer;
EFI_EDITOR_FILE_BUFFER  FileBufferBackupVar;

//
// for basic initialization of FileBuffer
//
EFI_EDITOR_FILE_BUFFER  FileBufferConst = {
  NULL,
  FileTypeUnicode,
  NULL,
  NULL,
  0,
  {
    0,
    0
  },
  {
    0,
    0
  },
  {
    0,
    0
  },
  {
    0,
    0
  },
  FALSE,
  TRUE,
  FALSE,
  NULL
};

//
// the whole edit area needs to be refreshed
//
BOOLEAN          FileBufferNeedRefresh;	

//
// only the current line in edit area needs to be refresh
//
BOOLEAN                 FileBufferOnlyLineNeedRefresh;

BOOLEAN                 FileBufferMouseNeedRefresh;

extern BOOLEAN          EditorMouseAction;

/**
  Initialization function for FileBuffer.

  @param EFI_SUCCESS            The initialization was successful.
  @param EFI_LOAD_ERROR         A default name could not be created.
  @param EFI_OUT_OF_RESOURCES   A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferInit (
  VOID
  )
{
  //
  // basically initialize the FileBuffer
  //
  CopyMem (&FileBuffer         , &FileBufferConst, sizeof (EFI_EDITOR_FILE_BUFFER));
  CopyMem (&FileBufferBackupVar, &FileBufferConst, sizeof (EFI_EDITOR_FILE_BUFFER));

  //
  // set default FileName
  //
  FileBuffer.FileName = EditGetDefaultFileName (L"txt");
  if (FileBuffer.FileName == NULL) {
    return EFI_LOAD_ERROR;
  }

  FileBuffer.ListHead = AllocateZeroPool (sizeof (LIST_ENTRY));
  if (FileBuffer.ListHead == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (FileBuffer.ListHead);

  FileBuffer.DisplayPosition.Row    = 2;
  FileBuffer.DisplayPosition.Column = 1;
  FileBuffer.LowVisibleRange.Row    = 2;
  FileBuffer.LowVisibleRange.Column = 1;

  FileBufferNeedRefresh             = FALSE;
  FileBufferMouseNeedRefresh        = FALSE;
  FileBufferOnlyLineNeedRefresh     = FALSE;

  return EFI_SUCCESS;
}

/**
  Backup function for FileBuffer.  Only backup the following items:
      Mouse/Cursor position
      File Name, Type, ReadOnly, Modified
      Insert Mode

  This is for making the file buffer refresh as few as possible.

  @retval EFI_SUCCESS           The backup operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferBackup (
  VOID
  )
{
  FileBufferBackupVar.MousePosition = FileBuffer.MousePosition;

  SHELL_FREE_NON_NULL (FileBufferBackupVar.FileName);
  FileBufferBackupVar.FileName        = NULL;
  FileBufferBackupVar.FileName        = StrnCatGrow (&FileBufferBackupVar.FileName, NULL, FileBuffer.FileName, 0);

  FileBufferBackupVar.ModeInsert      = FileBuffer.ModeInsert;
  FileBufferBackupVar.FileType        = FileBuffer.FileType;

  FileBufferBackupVar.FilePosition    = FileBuffer.FilePosition;
  FileBufferBackupVar.LowVisibleRange = FileBuffer.LowVisibleRange;

  FileBufferBackupVar.FileModified    = FileBuffer.FileModified;
  FileBufferBackupVar.ReadOnly        = FileBuffer.ReadOnly;

  return EFI_SUCCESS;
}

/**
  Advance to the next Count lines
  
  @param[in] Count              The line number to advance by.
  @param[in] CurrentLine        The pointer to the current line structure.
  @param[in] LineList           The pointer to the linked list of lines.

  @retval NULL                  There was an error.
  @return  The line structure after the advance.
**/
EFI_EDITOR_LINE *
EFIAPI
InternalEditorMiscLineAdvance (
  IN CONST UINTN            Count,
  IN CONST EFI_EDITOR_LINE  *CurrentLine,
  IN CONST LIST_ENTRY       *LineList
  )

{
  UINTN                 Index;
  CONST EFI_EDITOR_LINE *Line;

  if (CurrentLine == NULL || LineList == NULL) {
    return NULL;
  }

  for (Line = CurrentLine, Index = 0; Index < Count; Index++) {
    //
    // if already last line
    //
    if (Line->Link.ForwardLink == LineList) {
      return NULL;
    }

    Line = CR (Line->Link.ForwardLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
  }

  return ((EFI_EDITOR_LINE *)Line);
}

/**
  Retreat to the previous Count lines.
  
  @param[in] Count              The line number to retreat by.
  @param[in] CurrentLine        The pointer to the current line structure.
  @param[in] LineList           The pointer to the linked list of lines.

  @retval NULL                  There was an error.
  @return  The line structure after the retreat.
**/
EFI_EDITOR_LINE *
EFIAPI
InternalEditorMiscLineRetreat (
  IN CONST UINTN            Count,
  IN CONST EFI_EDITOR_LINE  *CurrentLine,
  IN CONST LIST_ENTRY       *LineList
  )

{
  UINTN                 Index;
  CONST EFI_EDITOR_LINE *Line;

  if (CurrentLine == NULL || LineList == NULL) {
    return NULL;
  }

  for (Line = CurrentLine, Index = 0; Index < Count; Index++) {
    //
    // already the first line
    //
    if (Line->Link.BackLink == LineList) {
      return NULL;
    }

    Line = CR (Line->Link.BackLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
  }

  return ((EFI_EDITOR_LINE *)Line);
}

/**
  Advance/Retreat lines
  
  @param[in] Count  line number to advance/retreat
                       >0 : advance
                       <0 : retreat

  @retval NULL An error occured.
  @return The line after advance/retreat.
**/
EFI_EDITOR_LINE *
MoveLine (
  IN CONST INTN Count
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           AbsCount;

  //
  // if < 0, then retreat
  // if > 0, the advance
  //
  if (Count <= 0) {
    AbsCount  = (UINTN)ABS(Count);
    Line      = InternalEditorMiscLineRetreat (AbsCount,MainEditor.FileBuffer->CurrentLine,MainEditor.FileBuffer->ListHead);
  } else {
    Line = InternalEditorMiscLineAdvance ((UINTN)Count,MainEditor.FileBuffer->CurrentLine,MainEditor.FileBuffer->ListHead);
  }

  return Line;
}

/**
  Function to update the 'screen' to display the mouse position.

  @retval EFI_SUCCESS           The backup operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferRestoreMousePosition (
  VOID
  )
{
  EFI_EDITOR_COLOR_UNION  Orig;
  EFI_EDITOR_COLOR_UNION  New;
  UINTN                   FRow;
  UINTN                   FColumn;
  BOOLEAN                 HasCharacter;
  EFI_EDITOR_LINE         *CurrentLine;
  EFI_EDITOR_LINE         *Line;
  CHAR16                  Value;

  //
  // variable initialization
  //
  Line = NULL;

  if (MainEditor.MouseSupported) {

    if (FileBufferMouseNeedRefresh) {

      FileBufferMouseNeedRefresh = FALSE;

      //
      // if mouse position not moved and only mouse action
      // so do not need to refresh mouse position
      //
      if ((FileBuffer.MousePosition.Row == FileBufferBackupVar.MousePosition.Row &&
          FileBuffer.MousePosition.Column == FileBufferBackupVar.MousePosition.Column)
          && EditorMouseAction) {
        return EFI_SUCCESS;
      }
      //
      // backup the old screen attributes
      //
      Orig                  = MainEditor.ColorAttributes;
      New.Colors.Foreground = Orig.Colors.Background;
      New.Colors.Background = Orig.Colors.Foreground;

      //
      // clear the old mouse position
      //
      FRow          = FileBuffer.LowVisibleRange.Row + FileBufferBackupVar.MousePosition.Row - 2;

      FColumn       = FileBuffer.LowVisibleRange.Column + FileBufferBackupVar.MousePosition.Column - 1;

      HasCharacter  = TRUE;
      if (FRow > FileBuffer.NumLines) {
        HasCharacter = FALSE;
      } else {
        CurrentLine = FileBuffer.CurrentLine;
        Line        = MoveLine (FRow - FileBuffer.FilePosition.Row);

        if (Line == NULL || FColumn > Line->Size) {
          HasCharacter = FALSE;
        }

        FileBuffer.CurrentLine = CurrentLine;
      }

      ShellPrintEx (
        (INT32)FileBufferBackupVar.MousePosition.Column - 1,
        (INT32)FileBufferBackupVar.MousePosition.Row - 1,
        L" "
        );

      if (HasCharacter) {
        Value = (Line->Buffer[FColumn - 1]);
        ShellPrintEx (
          (INT32)FileBufferBackupVar.MousePosition.Column - 1,
          (INT32)FileBufferBackupVar.MousePosition.Row - 1,
          L"%c",
          Value
          );
      }
      //
      // set the new mouse position
      //
      gST->ConOut->SetAttribute (gST->ConOut, New.Data);

      //
      // clear the old mouse position
      //
      FRow          = FileBuffer.LowVisibleRange.Row + FileBuffer.MousePosition.Row - 2;
      FColumn       = FileBuffer.LowVisibleRange.Column + FileBuffer.MousePosition.Column - 1;

      HasCharacter  = TRUE;
      if (FRow > FileBuffer.NumLines) {
        HasCharacter = FALSE;
      } else {
        CurrentLine = FileBuffer.CurrentLine;
        Line        = MoveLine (FRow - FileBuffer.FilePosition.Row);

        if (Line == NULL || FColumn > Line->Size) {
          HasCharacter = FALSE;
        }

        FileBuffer.CurrentLine = CurrentLine;
      }

      ShellPrintEx (
        (INT32)FileBuffer.MousePosition.Column - 1,
        (INT32)FileBuffer.MousePosition.Row - 1,
        L" "
        );

      if (HasCharacter) {
        Value = Line->Buffer[FColumn - 1];
        ShellPrintEx (
          (INT32)FileBuffer.MousePosition.Column - 1,
          (INT32)FileBuffer.MousePosition.Row - 1,
          L"%c",
          Value
          );
      }
      //
      // end of HasCharacter
      //
      gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
    }
    //
    // end of MouseNeedRefresh
    //
  }
  //
  // end of MouseSupported
  //
  return EFI_SUCCESS;
}

/**
  Free all the lines in FileBuffer
   Fields affected:
     Lines
     CurrentLine
     NumLines
     ListHead

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferFreeLines (
  VOID
  )
{
  LIST_ENTRY  *Link;
  EFI_EDITOR_LINE *Line;

  //
  // free all the lines
  //
  if (FileBuffer.Lines != NULL) {

    Line  = FileBuffer.Lines;
    Link  = &(Line->Link);
    do {
      Line  = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
      Link  = Link->ForwardLink;

      //
      // free line's buffer and line itself
      //
      LineFree (Line);
    } while (Link != FileBuffer.ListHead);
  }
  //
  // clean the line list related structure
  //
  FileBuffer.Lines            = NULL;
  FileBuffer.CurrentLine      = NULL;
  FileBuffer.NumLines         = 0;

  FileBuffer.ListHead->ForwardLink  = FileBuffer.ListHead;
  FileBuffer.ListHead->BackLink  = FileBuffer.ListHead;

  return EFI_SUCCESS;
}

/**
  Cleanup function for FileBuffer.

  @retval EFI_SUCCESS   The cleanup was successful.
**/
EFI_STATUS
EFIAPI
FileBufferCleanup (
  VOID
  )
{
  EFI_STATUS  Status;

  SHELL_FREE_NON_NULL (FileBuffer.FileName);

  //
  // free all the lines
  //
  Status = FileBufferFreeLines ();

  SHELL_FREE_NON_NULL (FileBuffer.ListHead);
  FileBuffer.ListHead = NULL;

  SHELL_FREE_NON_NULL (FileBufferBackupVar.FileName);
  return Status;

}

/**
  Print a line specified by Line on a row specified by Row of the screen.

  @param[in] Line               The line to print.
  @param[in] Row                The row on the screen to print onto (begin from 1).

  @retval EFI_SUCCESS           The printing was successful.
**/
EFI_STATUS
FileBufferPrintLine (
  IN CONST EFI_EDITOR_LINE  *Line,
  IN CONST UINTN            Row
  )
{

  CHAR16  *Buffer;
  UINTN   Limit;
  CHAR16  PrintLine[200];
  CHAR16  PrintLine2[250];

  //
  // print start from correct character
  //
  Buffer  = Line->Buffer + FileBuffer.LowVisibleRange.Column - 1;

  Limit   = Line->Size - FileBuffer.LowVisibleRange.Column + 1;
  if (Limit > Line->Size) {
    Limit = 0;
  }

  StrnCpy (PrintLine, Buffer, MIN(MIN(Limit,MainEditor.ScreenSize.Column), 200));
  for (; Limit < MainEditor.ScreenSize.Column; Limit++) {
    PrintLine[Limit] = L' ';
  }

  PrintLine[MainEditor.ScreenSize.Column] = CHAR_NULL;
  ShellCopySearchAndReplace(PrintLine, PrintLine2,  250, L"%", L"^%", FALSE, FALSE);

  ShellPrintEx (
    0,
    (INT32)Row - 1,
    L"%s",
    PrintLine2
    );

  return EFI_SUCCESS;
}

/**
  Set the cursor position according to FileBuffer.DisplayPosition.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferRestorePosition (
  VOID
  )
{
  //
  // set cursor position
  //
  return (gST->ConOut->SetCursorPosition (
        gST->ConOut,
        FileBuffer.DisplayPosition.Column - 1,
        FileBuffer.DisplayPosition.Row - 1
        ));
}

/**
  Refresh the screen with whats in the buffer.

  @retval EFI_SUCCESS     The refresh was successful.
  @retval EFI_LOAD_ERROR  There was an error finding what to write.
**/
EFI_STATUS
EFIAPI
FileBufferRefresh (
  VOID
  )
{
  LIST_ENTRY  *Link;
  EFI_EDITOR_LINE *Line;
  UINTN           Row;

  //
  // if it's the first time after editor launch, so should refresh
  //
  if (!EditorFirst) {
    //
    // no definite required refresh
    // and file position displayed on screen has not been changed
    //
    if (!FileBufferNeedRefresh &&
        !FileBufferOnlyLineNeedRefresh &&
        FileBufferBackupVar.LowVisibleRange.Row == FileBuffer.LowVisibleRange.Row &&
        FileBufferBackupVar.LowVisibleRange.Column == FileBuffer.LowVisibleRange.Column
        ) {

      FileBufferRestoreMousePosition ();
      FileBufferRestorePosition ();

      return EFI_SUCCESS;
    }
  }

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  //
  // only need to refresh current line
  //
  if (FileBufferOnlyLineNeedRefresh &&
      FileBufferBackupVar.LowVisibleRange.Row == FileBuffer.LowVisibleRange.Row &&
      FileBufferBackupVar.LowVisibleRange.Column == FileBuffer.LowVisibleRange.Column
      ) {

    EditorClearLine (FileBuffer.DisplayPosition.Row, MainEditor.ScreenSize.Column, MainEditor.ScreenSize.Row);
    FileBufferPrintLine (
      FileBuffer.CurrentLine,
      FileBuffer.DisplayPosition.Row
      );
  } else {
    //
    // the whole edit area need refresh
    //

    //
    // no line
    //
    if (FileBuffer.Lines == NULL) {
      FileBufferRestoreMousePosition ();
      FileBufferRestorePosition ();
      gST->ConOut->EnableCursor (gST->ConOut, TRUE);

      return EFI_SUCCESS;
    }
    //
    // get the first line that will be displayed
    //
    Line = MoveLine (FileBuffer.LowVisibleRange.Row - FileBuffer.FilePosition.Row);
    if (Line == NULL) {
      gST->ConOut->EnableCursor (gST->ConOut, TRUE);

      return EFI_LOAD_ERROR;
    }

    Link  = &(Line->Link);
    Row   = 2;
    do {
      Line = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

      //
      // print line at row
      //
      FileBufferPrintLine (Line, Row);

      Link = Link->ForwardLink;
      Row++;
    } while (Link != FileBuffer.ListHead && Row <= (MainEditor.ScreenSize.Row - 1));
    //
    // while not file end and not screen full
    //
    while (Row <= (MainEditor.ScreenSize.Row - 1)) {
      EditorClearLine (Row, MainEditor.ScreenSize.Column, MainEditor.ScreenSize.Row);
      Row++;
    }
  }

  FileBufferRestoreMousePosition ();
  FileBufferRestorePosition ();

  FileBufferNeedRefresh         = FALSE;
  FileBufferOnlyLineNeedRefresh = FALSE;

  gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  return EFI_SUCCESS;
}

/**
  Create a new line and append it to the line list.
    Fields affected:
      NumLines
      Lines

  @retval NULL    The create line failed.
  @return         The line created.
**/
EFI_EDITOR_LINE *
EFIAPI
FileBufferCreateLine (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;

  //
  // allocate a line structure
  //
  Line = AllocateZeroPool (sizeof (EFI_EDITOR_LINE));
  if (Line == NULL) {
    return NULL;
  }
  //
  // initialize the structure
  //
  Line->Signature = LINE_LIST_SIGNATURE;
  Line->Size      = 0;
  Line->TotalSize = 0;
  Line->Type      = NewLineTypeDefault;

  //
  // initial buffer of the line is "\0"
  //
  ASSERT(CHAR_NULL == CHAR_NULL);
  Line->Buffer = CatSPrint (NULL, L"\0");
  if (Line->Buffer == NULL) {
    return NULL;
  }

  FileBuffer.NumLines++;

  //
  // insert the line into line list
  //
  InsertTailList (FileBuffer.ListHead, &Line->Link);

  if (FileBuffer.Lines == NULL) {
    FileBuffer.Lines = CR (FileBuffer.ListHead->ForwardLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
  }

  return Line;
}

/**
  Set FileName field in FileBuffer.

  @param Str                    The file name to set.
  
  @retval EFI_SUCCESS           The filename was successfully set.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_INVALID_PARAMETER Str is not a valid filename.
**/
EFI_STATUS
EFIAPI
FileBufferSetFileName (
  IN CONST CHAR16 *Str
  )
{
  //
  // Verify the parameters
  //
  if (!IsValidFileName(Str)) {
    return (EFI_INVALID_PARAMETER);
  }
  //
  // free the old file name
  //
  SHELL_FREE_NON_NULL (FileBuffer.FileName);

  //
  // Allocate and set the new name
  //
  FileBuffer.FileName = CatSPrint (NULL, L"%s", Str);
  if (FileBuffer.FileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
/**
  Free the existing file lines and reset the modified flag.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferFree (
  VOID
  )
{
  //
  // free all the lines
  //
  FileBufferFreeLines ();
  FileBuffer.FileModified = FALSE;

  return EFI_SUCCESS;
}


/**
  Read a file from disk into the FileBuffer.
  
  @param[in] FileName           The filename to read.
  @param[in] Recover            TRUE if is for recover mode, no information printouts.
  
  @retval EFI_SUCCESS            The load was successful.
  @retval EFI_LOAD_ERROR         The load failed.
  @retval EFI_OUT_OF_RESOURCES   A memory allocation failed.
  @retval EFI_INVALID_PARAMETER  FileName is a directory.
**/
EFI_STATUS
EFIAPI
FileBufferRead (
  IN CONST CHAR16  *FileName,
  IN CONST BOOLEAN Recover
  )
{
  EFI_EDITOR_LINE                 *Line;
  EE_NEWLINE_TYPE                 Type;
  UINTN                           LoopVar1;
  UINTN                           LoopVar2;
  UINTN                           LineSize;
  VOID                            *Buffer;
  CHAR16                          *UnicodeBuffer;
  UINT8                           *AsciiBuffer;
  UINTN                           FileSize;
  SHELL_FILE_HANDLE               FileHandle;
  BOOLEAN                         CreateFile;
  EFI_STATUS                      Status;
  UINTN                           LineSizeBackup;
  EFI_FILE_INFO                   *Info;

  Line          = NULL;
  LoopVar1      = 0;
  FileSize      = 0;
  UnicodeBuffer = NULL;
  Type          = NewLineTypeDefault;
  FileHandle    = NULL;
  CreateFile    = FALSE;

  //
  // in this function, when you return error ( except EFI_OUT_OF_RESOURCES )
  // you should set status string via StatusBarSetStatusString(L"blah")
  // since this function maybe called before the editorhandleinput loop
  // so any error will cause editor return
  // so if you want to print the error status
  // you should set the status string
  //

  //
  // try to open the file
  //
  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ, 0);

  if (!EFI_ERROR(Status)) {
    CreateFile = FALSE;
    if (FileHandle == NULL) {
      StatusBarSetStatusString (L"Disk Error");
      return EFI_LOAD_ERROR;
    }

    Info = ShellGetFileInfo(FileHandle);
    
    if (Info->Attribute & EFI_FILE_DIRECTORY) {
      StatusBarSetStatusString (L"Directory Can Not Be Edited");
      FreePool (Info);
      return EFI_INVALID_PARAMETER;
    }

    if (Info->Attribute & EFI_FILE_READ_ONLY) {
      FileBuffer.ReadOnly = TRUE;
    } else {
      FileBuffer.ReadOnly = FALSE;
    }
    //
    // get file size
    //
    FileSize = (UINTN) Info->FileSize;

    FreePool (Info);
  } else if (Status == EFI_NOT_FOUND) {
    //
    // file not exists.  add create and try again
    //
    Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_WRITE_PROTECTED ||
          Status == EFI_ACCESS_DENIED ||
          Status == EFI_NO_MEDIA ||
          Status == EFI_MEDIA_CHANGED
          ) {
        StatusBarSetStatusString (L"Access Denied");
      } else if (Status == EFI_DEVICE_ERROR || Status == EFI_VOLUME_CORRUPTED || Status == EFI_VOLUME_FULL) {
        StatusBarSetStatusString (L"Disk Error");
      } else {
        StatusBarSetStatusString (L"Invalid File Name or Current-working-directory");
      }

      return Status;
    } else {
      //
      // it worked.  now delete it and move on with the name (now validated)
      //
      Status = ShellDeleteFile (&FileHandle);
      if (Status == EFI_WARN_DELETE_FAILURE) {
        Status = EFI_ACCESS_DENIED;
      }
      FileHandle = NULL;
      if (EFI_ERROR (Status)) {
        StatusBarSetStatusString (L"Access Denied");
        return Status;
      }
    }
    //
    // file doesn't exist, so set CreateFile to TRUE
    //
    CreateFile          = TRUE;
    FileBuffer.ReadOnly = FALSE;

    //
    // all the check ends
    // so now begin to set file name, free lines
    //
    if (StrCmp (FileName, FileBuffer.FileName) != 0) {
      FileBufferSetFileName (FileName);
    }
    //
    // free the old lines
    //
    FileBufferFree ();

  }
  //
  // the file exists
  //
  if (!CreateFile) {
    //
    // allocate buffer to read file
    //
    Buffer = AllocateZeroPool (FileSize);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // read file into Buffer
    //
    Status = ShellReadFile (FileHandle, &FileSize, Buffer);
    ShellCloseFile(&FileHandle);
    FileHandle = NULL;
    if (EFI_ERROR (Status)) {
      StatusBarSetStatusString (L"Read File Failed");
      SHELL_FREE_NON_NULL (Buffer);
      return EFI_LOAD_ERROR;
    }
    //
    // nothing in this file
    //
    if (FileSize == 0) {
      SHELL_FREE_NON_NULL (Buffer);
      //
      // since has no head, so only can be an ASCII file
      //
      FileBuffer.FileType = FileTypeAscii;

      goto Done;
    }

    AsciiBuffer = Buffer;

    if (FileSize < 2) {
      //
      // size < Unicode file header, so only can be ASCII file
      //
      FileBuffer.FileType = FileTypeAscii;
    } else {
      //
      // Unicode file
      //
      if (*(UINT16 *) Buffer == EFI_UNICODE_BYTE_ORDER_MARK) {
        //
        // Unicode file's size should be even
        //
        if ((FileSize % 2) != 0) {
          StatusBarSetStatusString (L"File Format Wrong");
          SHELL_FREE_NON_NULL (Buffer);
          return EFI_LOAD_ERROR;
        }

        FileSize /= 2;

        FileBuffer.FileType = FileTypeUnicode;
        UnicodeBuffer       = Buffer;

        //
        // pass this 0xff and 0xfe
        //
        UnicodeBuffer++;
        FileSize--;
      } else {
        FileBuffer.FileType = FileTypeAscii;
      }
      //
      // end of AsciiBuffer ==
      //
    }
    //
    // end of FileSize < 2
    // all the check ends
    // so now begin to set file name, free lines
    //
    if (StrCmp (FileName, FileBuffer.FileName) != 0) {
      FileBufferSetFileName (FileName);
    }

    //
    // free the old lines
    //
    FileBufferFree ();

    //
    // parse file content line by line
    //
    for (LoopVar1 = 0; LoopVar1 < FileSize; LoopVar1++) {
      Type = NewLineTypeUnknown;

      for (LineSize = LoopVar1; LineSize < FileSize; LineSize++) {
        if (FileBuffer.FileType == FileTypeAscii) {
          if (AsciiBuffer[LineSize] == CHAR_CARRIAGE_RETURN) {
            Type = NewLineTypeCarriageReturn;

            //
            // has LF following
            //
            if (LineSize < FileSize - 1) {
              if (AsciiBuffer[LineSize + 1] == CHAR_LINEFEED) {
                Type = NewLineTypeCarriageReturnLineFeed;
              }
            }

            break;
          } else if (AsciiBuffer[LineSize] == CHAR_LINEFEED) {
            Type = NewLineTypeLineFeed;

            //
            // has CR following
            //
            if (LineSize < FileSize - 1) {
              if (AsciiBuffer[LineSize + 1] == CHAR_CARRIAGE_RETURN) {
                Type = NewLineTypeLineFeedCarriageReturn;
              }
            }

            break;
          }
        } else {
          if (UnicodeBuffer[LineSize] == CHAR_CARRIAGE_RETURN) {
            Type = NewLineTypeCarriageReturn;

            //
            // has LF following
            //
            if (LineSize < FileSize - 1) {
              if (UnicodeBuffer[LineSize + 1] == CHAR_LINEFEED) {
                Type = NewLineTypeCarriageReturnLineFeed;
              }
            }

            break;
          } else if (UnicodeBuffer[LineSize] == CHAR_LINEFEED) {
            Type = NewLineTypeLineFeed;

            //
            // has CR following
            //
            if (LineSize < FileSize - 1) {
              if (UnicodeBuffer[LineSize + 1] == CHAR_CARRIAGE_RETURN) {
                Type = NewLineTypeLineFeedCarriageReturn;
              }
            }

            break;
          }
        }
        //
        // endif == ASCII
        //
      }
      //
      // end of for LineSize
      //
      // if the type is wrong, then exit
      //
      if (Type == NewLineTypeUnknown) {
        //
        // Now if Type is NewLineTypeUnknown, it should be file end
        //
        Type = NewLineTypeDefault;
      }

      LineSizeBackup = LineSize;

      //
      // create a new line
      //
      Line = FileBufferCreateLine ();
      if (Line == NULL) {
        SHELL_FREE_NON_NULL (Buffer);
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // calculate file length
      //
      LineSize -= LoopVar1;

      //
      // Unicode and one CHAR_NULL
      //
      SHELL_FREE_NON_NULL (Line->Buffer);
      Line->Buffer = AllocateZeroPool (LineSize * 2 + 2);

      if (Line->Buffer == NULL) {
        RemoveEntryList (&Line->Link);
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // copy this line to Line->Buffer
      //
      for (LoopVar2 = 0; LoopVar2 < LineSize; LoopVar2++) {
        if (FileBuffer.FileType == FileTypeAscii) {
          Line->Buffer[LoopVar2] = (CHAR16) AsciiBuffer[LoopVar1];
        } else {
          Line->Buffer[LoopVar2] = UnicodeBuffer[LoopVar1];
        }

        LoopVar1++;
      }
      //
      // LoopVar1 now points to where CHAR_CARRIAGE_RETURN or CHAR_LINEFEED;
      //
      Line->Buffer[LineSize]  = 0;

      Line->Size              = LineSize;
      Line->TotalSize         = LineSize;
      Line->Type              = Type;

      if (Type == NewLineTypeCarriageReturnLineFeed || Type == NewLineTypeLineFeedCarriageReturn) {
        LoopVar1++;
      }

      //
      // last character is a return, SO create a new line
      //
      if (((Type == NewLineTypeCarriageReturnLineFeed || Type == NewLineTypeLineFeedCarriageReturn) && LineSizeBackup == FileSize - 2) ||
          ((Type == NewLineTypeLineFeed || Type == NewLineTypeCarriageReturn) && LineSizeBackup == FileSize - 1)
          ) {
        Line = FileBufferCreateLine ();
        if (Line == NULL) {
          SHELL_FREE_NON_NULL (Buffer);
          return EFI_OUT_OF_RESOURCES;
        }
      }
      //
      // end of if
      //
    }
    //
    // end of LoopVar1
    //
    SHELL_FREE_NON_NULL (Buffer);

  }
  //
  // end of if CreateFile
  //
Done:

  FileBuffer.DisplayPosition.Row    = 2;
  FileBuffer.DisplayPosition.Column = 1;
  FileBuffer.LowVisibleRange.Row    = 1;
  FileBuffer.LowVisibleRange.Column = 1;
  FileBuffer.FilePosition.Row       = 1;
  FileBuffer.FilePosition.Column    = 1;
  FileBuffer.MousePosition.Row      = 2;
  FileBuffer.MousePosition.Column   = 1;

  if (!Recover) {
    UnicodeBuffer = CatSPrint (NULL, L"%d Lines Read", FileBuffer.NumLines);
    if (UnicodeBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    StatusBarSetStatusString (UnicodeBuffer);
    FreePool (UnicodeBuffer);
  }
/*
    //
    // check whether we have fs?: in filename
    //
    LoopVar1             = 0;
    FSMappingPtr  = NULL;
    while (FileName[LoopVar1] != 0) {
      if (FileName[LoopVar1] == L':') {
        FSMappingPtr = &FileName[LoopVar1];
        break;
      }

      LoopVar1++;
    }

    if (FSMappingPtr == NULL) {
      CurDir = ShellGetCurrentDir (NULL);
    } else {
      LoopVar1 = 0;
      LoopVar2 = 0;
      while (FileName[LoopVar1] != 0) {
        if (FileName[LoopVar1] == L':') {
          break;
        }

        FSMapping[LoopVar2++] = FileName[LoopVar1];

        LoopVar1++;
      }

      FSMapping[LoopVar2]  = 0;
      CurDir        = ShellGetCurrentDir (FSMapping);
    }

    if (CurDir != NULL) {
      for (LoopVar1 = 0; LoopVar1 < StrLen (CurDir) && CurDir[LoopVar1] != ':'; LoopVar1++);

      CurDir[LoopVar1]   = 0;
      DevicePath  = (EFI_DEVICE_PATH_PROTOCOL *) ShellGetMap (CurDir);
      FreePool (CurDir);
    } else {
      return EFI_LOAD_ERROR;
    }

    Status = LibDevicePathToInterface (
              &gEfiSimpleFileSystemProtocolGuid,
              DevicePath,
              (VOID **) &Vol
              );
    if (EFI_ERROR (Status)) {
      return EFI_LOAD_ERROR;
    }

    Status = Vol->OpenVolume (Vol, &RootFs);
    if (EFI_ERROR (Status)) {
      return EFI_LOAD_ERROR;
    }
    //
    // Get volume information of file system
    //
    Size        = SIZE_OF_EFI_FILE_SYSTEM_INFO + 100;
    VolumeInfo  = (EFI_FILE_SYSTEM_INFO *) AllocateZeroPool (Size);
    Status      = RootFs->GetInfo (RootFs, &gEfiFileSystemInfoGuid, &Size, VolumeInfo);
    if (EFI_ERROR (Status)) {
      RootFs->Close (RootFs);
      return EFI_LOAD_ERROR;
    }

    if (VolumeInfo->ReadOnly) {
      StatusBarSetStatusString (L"WARNING: Volume Read Only");
    }

    FreePool (VolumeInfo);
    RootFs->Close (RootFs);
  }
//
*/
  //
  // has line
  //
  if (FileBuffer.Lines != 0) {
    FileBuffer.CurrentLine = CR (FileBuffer.ListHead->ForwardLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
  } else {
    //
    // create a dummy line
    //
    Line = FileBufferCreateLine ();
    if (Line == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    FileBuffer.CurrentLine = Line;
  }

  FileBuffer.FileModified       = FALSE;
  FileBufferNeedRefresh         = TRUE;
  FileBufferOnlyLineNeedRefresh = FALSE;
  FileBufferMouseNeedRefresh    = TRUE;


  return EFI_SUCCESS;
}

/**
  According to FileBuffer.NewLineType & FileBuffer.FileType,
  get the return buffer and size.

  @param[in] Type               The type of line.
  @param[out] Buffer            The buffer to fill.
  @param[out] Size              The amount of the buffer used on return.
**/
VOID
EFIAPI
GetNewLine (
  IN CONST EE_NEWLINE_TYPE Type,
  OUT CHAR8           *Buffer,
  OUT UINT8           *Size
  )
{
  UINT8 NewLineSize;

  //
  // give new line buffer,
  // and will judge unicode or ascii
  //
  NewLineSize = 0;

  //
  // not legal new line type
  //
  if (Type != NewLineTypeLineFeed && Type != NewLineTypeCarriageReturn && Type != NewLineTypeCarriageReturnLineFeed && Type != NewLineTypeLineFeedCarriageReturn) {
    *Size = 0;
    return ;
  }
  //
  // use_cr: give 0x0d
  //
  if (Type == NewLineTypeCarriageReturn) {
    if (MainEditor.FileBuffer->FileType == FileTypeUnicode) {
      Buffer[0]   = 0x0d;
      Buffer[1]   = 0;
      NewLineSize = 2;
    } else {
      Buffer[0]   = 0x0d;
      NewLineSize = 1;
    }

    *Size = NewLineSize;
    return ;
  }
  //
  // use_lf: give 0x0a
  //
  if (Type == NewLineTypeLineFeed) {
    if (MainEditor.FileBuffer->FileType == FileTypeUnicode) {
      Buffer[0]   = 0x0a;
      Buffer[1]   = 0;
      NewLineSize = 2;
    } else {
      Buffer[0]   = 0x0a;
      NewLineSize = 1;
    }

    *Size = NewLineSize;
    return ;
  }
  //
  // use_crlf: give 0x0d 0x0a
  //
  if (Type == NewLineTypeCarriageReturnLineFeed) {
    if (MainEditor.FileBuffer->FileType == FileTypeUnicode) {
      Buffer[0]   = 0x0d;
      Buffer[1]   = 0;
      Buffer[2]   = 0x0a;
      Buffer[3]   = 0;

      NewLineSize = 4;
    } else {
      Buffer[0]   = 0x0d;
      Buffer[1]   = 0x0a;
      NewLineSize = 2;
    }

    *Size = NewLineSize;
    return ;
  }
  //
  // use_lfcr: give 0x0a 0x0d
  //
  if (Type == NewLineTypeLineFeedCarriageReturn) {
    if (MainEditor.FileBuffer->FileType == FileTypeUnicode) {
      Buffer[0]   = 0x0a;
      Buffer[1]   = 0;
      Buffer[2]   = 0x0d;
      Buffer[3]   = 0;

      NewLineSize = 4;
    } else {
      Buffer[0]   = 0x0a;
      Buffer[1]   = 0x0d;
      NewLineSize = 2;
    }

    *Size = NewLineSize;
    return ;
  }

}

/**
  Change a Unicode string to an ASCII string.

  @param[in] UStr     The Unicode string.
  @param[in] Length   The maximum size of AStr.
  @param[out] AStr    ASCII string to pass out.

  @return The actuall length.
**/
UINTN
EFIAPI
UnicodeToAscii (
  IN CONST CHAR16   *UStr,
  IN CONST UINTN    Length,
  OUT CHAR8         *AStr
  )
{
  UINTN Index;

  //
  // just buffer copy, not character copy
  //
  for (Index = 0; Index < Length; Index++) {
    *AStr++ = (CHAR8) *UStr++;
  }

  return Index;
}

/**
  Save lines in FileBuffer to disk

  @param[in] FileName           The file name for writing.

  @retval EFI_SUCCESS           Data was written.
  @retval EFI_LOAD_ERROR        
  @retval EFI_OUT_OF_RESOURCES  There were not enough resources to write the file.
**/
EFI_STATUS
EFIAPI
FileBufferSave (
  IN CONST CHAR16 *FileName
  )
{
  SHELL_FILE_HANDLE FileHandle;
  LIST_ENTRY        *Link;
  EFI_EDITOR_LINE   *Line;
  CHAR16            *Str;

  EFI_STATUS        Status;
  UINTN             Length;
  UINTN             NumLines;
  CHAR8             NewLineBuffer[4];
  UINT8             NewLineSize;

  EFI_FILE_INFO     *Info;

  UINT64            Attribute;

  EE_NEWLINE_TYPE   Type;

  UINTN             TotalSize;
  //
  // 2M
  //
  CHAR8             *Cache;
  UINTN             LeftSize;
  UINTN             Size;
  CHAR8             *Ptr;

  Length    = 0;
  //
  // 2M
  //
  TotalSize = 0x200000;

  Attribute = 0;



  //
  // if is the old file
  //
  if (FileBuffer.FileName != NULL && StrCmp (FileName, FileBuffer.FileName) == 0) {
    //
    // file has not been modified
    //
    if (!FileBuffer.FileModified) {
      return EFI_SUCCESS;
    }

    //
    // if file is read-only, set error
    //
    if (FileBuffer.ReadOnly) {
      StatusBarSetStatusString (L"Read Only File Can Not Be Saved");
      return EFI_SUCCESS;
    }
  }

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);

  if (!EFI_ERROR (Status)) {
    Info = ShellGetFileInfo(FileHandle);

    if (Info != NULL && Info->Attribute & EFI_FILE_DIRECTORY) {
      StatusBarSetStatusString (L"Directory Can Not Be Saved");
      ShellCloseFile(FileHandle);
      FreePool(Info);
      return EFI_LOAD_ERROR;
    }
    
    if (Info != NULL) {
      Attribute = Info->Attribute & ~EFI_FILE_READ_ONLY;
      FreePool(Info);
    }

    //
    // if file exits, so delete it
    //
    Status = ShellDeleteFile (&FileHandle);
    if (EFI_ERROR (Status) || Status == EFI_WARN_DELETE_FAILURE) {
      StatusBarSetStatusString (L"Write File Failed");
      return EFI_LOAD_ERROR;
    }
 }

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, Attribute);

  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Create File Failed");
    return EFI_LOAD_ERROR;
  }

  //
  // if file is Unicode file, write Unicode header to it.
  //
  if (FileBuffer.FileType == FileTypeUnicode) {
    Length  = 2;
    Status  = ShellWriteFile (FileHandle, &Length, (VOID*)&gUnicodeFileTag);
    if (EFI_ERROR (Status)) {
      ShellDeleteFile (&FileHandle);
      return EFI_LOAD_ERROR;
    }
  }

  Cache = AllocateZeroPool (TotalSize);
  if (Cache == NULL) {
    ShellDeleteFile (&FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // write all the lines back to disk
  //
  NumLines  = 0;
  Type      = NewLineTypeCarriageReturnLineFeed;

  Ptr       = Cache;
  LeftSize  = TotalSize;

  for (Link = FileBuffer.ListHead->ForwardLink; Link != FileBuffer.ListHead; Link = Link->ForwardLink) {
    Line = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

    if (Line->Type != NewLineTypeDefault) {
      Type = Line->Type;
    }
    //
    // newline character is at most 4 bytes ( two Unicode characters )
    //
    Length = 4;
    if (Line->Buffer != NULL && Line->Size != 0) {
      if (FileBuffer.FileType == FileTypeAscii) {
        Length += Line->Size;
      } else {
        Length += (Line->Size * 2);
      }
      //
      // end if FileTypeAscii
      //
    }

    //
    // no cache room left, so write cache to disk
    //
    if (LeftSize < Length) {
      Size    = TotalSize - LeftSize;
      Status  = ShellWriteFile (FileHandle, &Size, Cache);
      if (EFI_ERROR (Status)) {
        ShellDeleteFile (&FileHandle);        
        FreePool (Cache);
        return EFI_LOAD_ERROR;
      }
      Ptr       = Cache;
      LeftSize  = TotalSize;
    }

    if (Line->Buffer != NULL && Line->Size != 0) {
      if (FileBuffer.FileType == FileTypeAscii) {
        UnicodeToAscii (Line->Buffer, Line->Size, Ptr);
        Length = Line->Size;
      } else {
        Length = (Line->Size * 2);
        CopyMem (Ptr, (CHAR8 *) Line->Buffer, Length);
      }
      //
      // end if FileTypeAscii
      //
      Ptr += Length;
      LeftSize -= Length;

    }
    //
    // end of if Line -> Buffer != NULL && Line -> Size != 0
    //
    // if not the last line , write return buffer to disk
    //
    if (Link->ForwardLink != FileBuffer.ListHead) {
      GetNewLine (Type, NewLineBuffer, &NewLineSize);
      CopyMem (Ptr, (CHAR8 *) NewLineBuffer, NewLineSize);

      Ptr += NewLineSize;
      LeftSize -= NewLineSize;
    }

    NumLines++;
  }

  if (TotalSize != LeftSize) {
    Size    = TotalSize - LeftSize;
    Status  = ShellWriteFile (FileHandle, &Size, Cache);
    if (EFI_ERROR (Status)) {
      ShellDeleteFile (&FileHandle);
      FreePool (Cache);
      return EFI_LOAD_ERROR;
    }
  }

  FreePool (Cache);

  ShellCloseFile(&FileHandle);

  FileBuffer.FileModified = FALSE;

  //
  // set status string
  //
  Str = CatSPrint (NULL, L"%d Lines Wrote", NumLines);
  if (Str == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StatusBarSetStatusString (Str);
  SHELL_FREE_NON_NULL (Str);

  //
  // now everything is ready , you can set the new file name to filebuffer
  //
  if (FileName != NULL && FileBuffer.FileName != NULL && StrCmp (FileName, FileBuffer.FileName) != 0) {
    //
    // not the same
    //
    FileBufferSetFileName (FileName);
    if (FileBuffer.FileName == NULL) {
      ShellDeleteFile (&FileHandle);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  FileBuffer.ReadOnly = FALSE;
  return EFI_SUCCESS;
}

/**
  Scroll cursor to left 1 character position.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferScrollLeft (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FRow;
  UINTN           FCol;

  Line  = FileBuffer.CurrentLine;

  FRow  = FileBuffer.FilePosition.Row;
  FCol  = FileBuffer.FilePosition.Column;

  //
  // if already at start of this line, so move to the end of previous line
  //
  if (FCol <= 1) {
    //
    // has previous line
    //
    if (Line->Link.BackLink != FileBuffer.ListHead) {
      FRow--;
      Line  = CR (Line->Link.BackLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
      FCol  = Line->Size + 1;
    } else {
      return EFI_SUCCESS;
    }
  } else {
    //
    // if not at start of this line, just move to previous column
    //
    FCol--;
  }

  FileBufferMovePosition (FRow, FCol);

  return EFI_SUCCESS;
}

/**
  Delete a char in line

  @param[in, out] Line   The line to delete in.
  @param[in] Pos         Position to delete the char at ( start from 0 ).
**/
VOID
EFIAPI
LineDeleteAt (
  IN  OUT EFI_EDITOR_LINE       *Line,
  IN      UINTN                 Pos
  )
{
  UINTN Index;

  //
  // move the latter characters front
  //
  for (Index = Pos - 1; Index < Line->Size; Index++) {
    Line->Buffer[Index] = Line->Buffer[Index + 1];
  }

  Line->Size--;
}

/**
  Concatenate Src into Dest.

  @param[in, out] Dest   Destination string
  @param[in] Src         Src String.
**/
VOID
EFIAPI
LineCat (
  IN  OUT EFI_EDITOR_LINE *Dest,
  IN      EFI_EDITOR_LINE *Src
  )
{
  CHAR16  *Str;
  UINTN   Size;

  Size                = Dest->Size;

  Dest->Buffer[Size]  = 0;

  //
  // concatenate the two strings
  //
  Str = CatSPrint (NULL, L"%s%s", Dest->Buffer, Src->Buffer);
  if (Str == NULL) {
    Dest->Buffer = NULL;
    return ;
  }

  Dest->Size      = Size + Src->Size;
  Dest->TotalSize = Dest->Size;

  FreePool (Dest->Buffer);
  FreePool (Src->Buffer);

  //
  // put str to dest->buffer
  //
  Dest->Buffer = Str;
}

/**
  Delete the previous character.

  @retval EFI_SUCCESS           The delete was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferDoBackspace (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  EFI_EDITOR_LINE *End;
  LIST_ENTRY  *Link;
  UINTN           FileColumn;

  FileColumn  = FileBuffer.FilePosition.Column;

  Line        = FileBuffer.CurrentLine;

  //
  // the first column
  //
  if (FileColumn == 1) {
    //
    // the first row
    //
    if (FileBuffer.FilePosition.Row == 1) {
      return EFI_SUCCESS;
    }

    FileBufferScrollLeft ();

    Line  = FileBuffer.CurrentLine;
    Link  = Line->Link.ForwardLink;
    End   = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

    //
    // concatenate this line with previous line
    //
    LineCat (Line, End);
    if (Line->Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // remove End from line list
    //
    RemoveEntryList (&End->Link);
    FreePool (End);

    FileBuffer.NumLines--;

    FileBufferNeedRefresh         = TRUE;
    FileBufferOnlyLineNeedRefresh = FALSE;

  } else {
    //
    // just delete the previous character
    //
    LineDeleteAt (Line, FileColumn - 1);
    FileBufferScrollLeft ();
    FileBufferOnlyLineNeedRefresh = TRUE;
  }

  if (!FileBuffer.FileModified) {
    FileBuffer.FileModified = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Add a return into line at current position.

  @retval EFI_SUCCESS           The insetrion of the character was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferDoReturn (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  EFI_EDITOR_LINE *NewLine;
  UINTN           FileColumn;
  UINTN           Index;
  CHAR16          *Buffer;
  UINTN           Row;
  UINTN           Col;

  FileBufferNeedRefresh         = TRUE;
  FileBufferOnlyLineNeedRefresh = FALSE;

  Line                          = FileBuffer.CurrentLine;

  FileColumn                    = FileBuffer.FilePosition.Column;

  NewLine                       = AllocateZeroPool (sizeof (EFI_EDITOR_LINE));
  if (NewLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLine->Signature  = LINE_LIST_SIGNATURE;
  NewLine->Size       = Line->Size - FileColumn + 1;
  NewLine->TotalSize  = NewLine->Size;
  NewLine->Buffer     = CatSPrint (NULL, L"\0");
  if (NewLine->Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLine->Type = NewLineTypeDefault;

  if (NewLine->Size > 0) {
    //
    // UNICODE + CHAR_NULL
    //
    Buffer = AllocateZeroPool (2 * (NewLine->Size + 1));
    if (Buffer == NULL) {
      FreePool (NewLine->Buffer);
      FreePool (NewLine);
      return EFI_OUT_OF_RESOURCES;
    }

    FreePool (NewLine->Buffer);

    NewLine->Buffer = Buffer;

    for (Index = 0; Index < NewLine->Size; Index++) {
      NewLine->Buffer[Index] = Line->Buffer[Index + FileColumn - 1];
    }

    NewLine->Buffer[NewLine->Size]  = CHAR_NULL;

    Line->Buffer[FileColumn - 1]    = CHAR_NULL;
    Line->Size                      = FileColumn - 1;
  }
  //
  // increase NumLines
  //
  FileBuffer.NumLines++;

  //
  // insert it into the correct position of line list
  //
  NewLine->Link.BackLink     = &(Line->Link);
  NewLine->Link.ForwardLink     = Line->Link.ForwardLink;
  Line->Link.ForwardLink->BackLink = &(NewLine->Link);
  Line->Link.ForwardLink        = &(NewLine->Link);

  //
  // move cursor to the start of next line
  //
  Row = FileBuffer.FilePosition.Row + 1;
  Col = 1;

  FileBufferMovePosition (Row, Col);

  //
  // set file is modified
  //
  if (!FileBuffer.FileModified) {
    FileBuffer.FileModified = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Delete current character from current line.  This is the effect caused 
  by the 'del' key.

  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FileBufferDoDelete (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  EFI_EDITOR_LINE *Next;
  LIST_ENTRY  *Link;
  UINTN           FileColumn;

  Line        = FileBuffer.CurrentLine;
  FileColumn  = FileBuffer.FilePosition.Column;

  //
  // the last column
  //
  if (FileColumn >= Line->Size + 1) {
    //
    // the last line
    //
    if (Line->Link.ForwardLink == FileBuffer.ListHead) {
      return EFI_SUCCESS;
    }
    //
    // since last character,
    // so will add the next line to this line
    //
    Link  = Line->Link.ForwardLink;
    Next  = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
    LineCat (Line, Next);
    if (Line->Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    RemoveEntryList (&Next->Link);
    FreePool (Next);

    FileBuffer.NumLines--;

    FileBufferNeedRefresh         = TRUE;
    FileBufferOnlyLineNeedRefresh = FALSE;

  } else {
    //
    // just delete current character
    //
    LineDeleteAt (Line, FileColumn);
    FileBufferOnlyLineNeedRefresh = TRUE;
  }

  if (!FileBuffer.FileModified) {
    FileBuffer.FileModified = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Scroll cursor to right 1 character.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferScrollRight (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FRow;
  UINTN           FCol;

  Line = FileBuffer.CurrentLine;
  if (Line->Buffer == NULL) {
    return EFI_SUCCESS;
  }

  FRow  = FileBuffer.FilePosition.Row;
  FCol  = FileBuffer.FilePosition.Column;

  //
  // if already at end of this line, scroll it to the start of next line
  //
  if (FCol > Line->Size) {
    //
    // has next line
    //
    if (Line->Link.ForwardLink != FileBuffer.ListHead) {
      FRow++;
      FCol = 1;
    } else {
      return EFI_SUCCESS;
    }
  } else {
    //
    // if not at end of this line, just move to next column
    //
    FCol++;
  }

  FileBufferMovePosition (FRow, FCol);

  return EFI_SUCCESS;
}

/**
  Insert a char into line

  
  @param[in] Line     The line to insert into.
  @param[in] Char     The char to insert.
  @param[in] Pos      The position to insert the char at ( start from 0 ).
  @param[in] StrSize  The current string size ( include CHAR_NULL ),unit is Unicode character.

  @return The new string size ( include CHAR_NULL ) ( unit is Unicode character ).
**/
UINTN
EFIAPI
LineStrInsert (
  IN      EFI_EDITOR_LINE  *Line,
  IN      CHAR16           Char,
  IN      UINTN            Pos,
  IN      UINTN            StrSize
  )
{
  UINTN   Index;
  CHAR16  *TempStringPtr;
  CHAR16  *Str;

  Index = (StrSize) * 2;

  Str   = Line->Buffer;

  //
  // do not have free space
  //
  if (Line->TotalSize <= Line->Size) {
    Str = ReallocatePool (Index, Index + 16, Str);
    if (Str == NULL) {
      return 0;
    }

    Line->TotalSize += 8;
  }
  //
  // move the later part of the string one character right
  //
  TempStringPtr = Str;
  for (Index = StrSize; Index > Pos; Index--) {
    TempStringPtr[Index] = TempStringPtr[Index - 1];
  }
  //
  // insert char into it.
  //
  TempStringPtr[Index]      = Char;

  Line->Buffer  = Str;
  Line->Size++;

  return StrSize + 1;
}

/**
  Add a character to the current line.

  @param[in] Char               The Character to input.

  @retval EFI_SUCCESS           The input was succesful.
**/
EFI_STATUS
EFIAPI
FileBufferAddChar (
  IN  CHAR16  Char
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FilePos;

  Line = FileBuffer.CurrentLine;

  //
  // only needs to refresh current line
  //
  FileBufferOnlyLineNeedRefresh = TRUE;

  //
  // when is insert mode, or cursor is at end of this line,
  // so insert this character
  // or replace the character.
  //
  FilePos = FileBuffer.FilePosition.Column - 1;
  if (FileBuffer.ModeInsert || FilePos + 1 > Line->Size) {
    LineStrInsert (Line, Char, FilePos, Line->Size + 1);
  } else {
    Line->Buffer[FilePos] = Char;
  }
  //
  // move cursor to right
  //
  FileBufferScrollRight ();

  if (!FileBuffer.FileModified) {
    FileBuffer.FileModified = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Handles inputs from characters (ASCII key + Backspace + return)

  @param[in] Char               The input character.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_LOAD_ERROR        There was an error.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferDoCharInput (
  IN CONST CHAR16 Char
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  switch (Char) {
  case CHAR_NULL:
    break;

  case CHAR_BACKSPACE:
    Status = FileBufferDoBackspace ();
    break;

  case CHAR_TAB:
    //
    // Tabs are ignored
    //
    break;

  case CHAR_LINEFEED:
  case CHAR_CARRIAGE_RETURN:
    Status = FileBufferDoReturn ();
    break;

  default:
    //
    // DEAL WITH ASCII CHAR, filter out thing like ctrl+f
    //
    if (Char > 127 || Char < 32) {
      Status = StatusBarSetStatusString (L"Unknown Command");
    } else {
      Status = FileBufferAddChar (Char);
    }

    break;

  }

  return Status;
}

/**
  Scroll cursor to the next line.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferScrollDown (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FRow;
  UINTN           FCol;

  Line = FileBuffer.CurrentLine;
  if (Line->Buffer == NULL) {
    return EFI_SUCCESS;
  }

  FRow  = FileBuffer.FilePosition.Row;
  FCol  = FileBuffer.FilePosition.Column;

  //
  // has next line
  //
  if (Line->Link.ForwardLink != FileBuffer.ListHead) {
    FRow++;
    Line = CR (Line->Link.ForwardLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

    //
    // if the next line is not that long, so move to end of next line
    //
    if (FCol > Line->Size) {
      FCol = Line->Size + 1;
    }

  } else {
    return EFI_SUCCESS;
  }

  FileBufferMovePosition (FRow, FCol);

  return EFI_SUCCESS;
}

/**
  Scroll the cursor to previous line.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferScrollUp (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FRow;
  UINTN           FCol;

  Line  = FileBuffer.CurrentLine;

  FRow  = FileBuffer.FilePosition.Row;
  FCol  = FileBuffer.FilePosition.Column;

  //
  // has previous line
  //
  if (Line->Link.BackLink != FileBuffer.ListHead) {
    FRow--;
    Line = CR (Line->Link.BackLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

    //
    // if previous line is not that long, so move to the end of previous line
    //
    if (FCol > Line->Size) {
      FCol = Line->Size + 1;
    }

  } else {
    return EFI_SUCCESS;
  }

  FileBufferMovePosition (FRow, FCol);

  return EFI_SUCCESS;
}

/**
  Scroll cursor to next page.

  @retval EFI_SUCCESS     The operation wa successful.
**/
EFI_STATUS
EFIAPI
FileBufferPageDown (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FRow;
  UINTN           FCol;
  UINTN           Gap;

  Line  = FileBuffer.CurrentLine;

  FRow  = FileBuffer.FilePosition.Row;
  FCol  = FileBuffer.FilePosition.Column;

  //
  // has next page
  //
  if (FileBuffer.NumLines >= FRow + (MainEditor.ScreenSize.Row - 2)) {
    Gap = (MainEditor.ScreenSize.Row - 2);
  } else {
    //
    // MOVE CURSOR TO LAST LINE
    //
    Gap = FileBuffer.NumLines - FRow;
  }
  //
  // get correct line
  //
  Line = MoveLine (Gap);

  //
  // if that line, is not that long, so move to the end of that line
  //
  if (Line != NULL && FCol > Line->Size) {
    FCol = Line->Size + 1;
  }

  FRow += Gap;

  FileBufferMovePosition (FRow, FCol);

  return EFI_SUCCESS;
}

/**
  Scroll cursor to previous screen.

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferPageUp (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FRow;
  UINTN           FCol;
  UINTN           Gap;
  INTN            Retreat;

  Line  = FileBuffer.CurrentLine;

  FRow  = FileBuffer.FilePosition.Row;
  FCol  = FileBuffer.FilePosition.Column;

  //
  // has previous page
  //
  if (FRow > (MainEditor.ScreenSize.Row - 2)) {
    Gap = (MainEditor.ScreenSize.Row - 2);
  } else {
    //
    // the first line of file will displayed on the first line of screen
    //
    Gap = FRow - 1;
  }

  Retreat = Gap;
  Retreat = -Retreat;

  //
  // get correct line
  //
  Line = MoveLine (Retreat);

  //
  // if that line is not that long, so move to the end of that line
  //
  if (Line != NULL && FCol > Line->Size) {
    FCol = Line->Size + 1;
  }

  FRow -= Gap;

  FileBufferMovePosition (FRow, FCol);

  return EFI_SUCCESS;
}

/**
  Scroll cursor to end of the current line.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferEnd (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           FRow;
  UINTN           FCol;

  Line  = FileBuffer.CurrentLine;

  FRow  = FileBuffer.FilePosition.Row;

  //
  // goto the last column of the line
  //
  FCol = Line->Size + 1;

  FileBufferMovePosition (FRow, FCol);

  return EFI_SUCCESS;
}

/** 
  Dispatch input to different handler
  @param[in] Key                The input key.  One of:
                                    ASCII KEY
                                    Backspace/Delete
                                    Return
                                    Direction key: up/down/left/right/pgup/pgdn
                                    Home/End
                                    INS

  @retval EFI_SUCCESS           The dispatch was done successfully.
  @retval EFI_LOAD_ERROR        The dispatch was not successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferHandleInput (
  IN CONST EFI_INPUT_KEY *Key
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  switch (Key->ScanCode) {
  //
  // ordinary key input
  //
  case SCAN_NULL:
    if (!FileBuffer.ReadOnly) {
      Status = FileBufferDoCharInput (Key->UnicodeChar);
    } else {
      Status = StatusBarSetStatusString (L"Read Only File Can Not Be Modified");
    }

    break;

  //
  // up arrow
  //
  case SCAN_UP:
    Status = FileBufferScrollUp ();
    break;

  //
  // down arrow
  //
  case SCAN_DOWN:
    Status = FileBufferScrollDown ();
    break;

  //
  // right arrow
  //
  case SCAN_RIGHT:
    Status = FileBufferScrollRight ();
    break;

  //
  // left arrow
  //
  case SCAN_LEFT:
    Status = FileBufferScrollLeft ();
    break;

  //
  // page up
  //
  case SCAN_PAGE_UP:
    Status = FileBufferPageUp ();
    break;

  //
  // page down
  //
  case SCAN_PAGE_DOWN:
    Status = FileBufferPageDown ();
    break;

  //
  // delete
  //
  case SCAN_DELETE:
    if (!FileBuffer.ReadOnly) {
      Status = FileBufferDoDelete ();
    } else {
      Status = StatusBarSetStatusString (L"Read Only File Can Not Be Modified");
    }

    break;

  //
  // home
  //
  case SCAN_HOME:
    FileBufferMovePosition (FileBuffer.FilePosition.Row, 1);
    Status = EFI_SUCCESS;
    break;

  //
  // end
  //
  case SCAN_END:
    Status = FileBufferEnd ();
    break;

  //
  // insert
  //
  case SCAN_INSERT:
    FileBuffer.ModeInsert = (BOOLEAN)!FileBuffer.ModeInsert;
    Status = EFI_SUCCESS;
    break;

  default:
    Status = StatusBarSetStatusString (L"Unknown Command");
    break;
  }

  return Status;
}

/**
  Check user specified FileRow is above current screen.

  @param[in] FileRow    The row of file position ( start from 1 ).

  @retval TRUE    It is above the current screen.
  @retval FALSE   It is not above the current screen.
**/
BOOLEAN
EFIAPI
AboveCurrentScreen (
  IN UINTN FileRow
  )
{
  //
  // if is to the above of the screen
  //
  if (FileRow < FileBuffer.LowVisibleRange.Row) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check user specified FileRow is under current screen.

  @param[in] FileRow    The row of file position ( start from 1 ).

  @retval TRUE      It is under the current screen.
  @retval FALSE     It is not under the current screen.
**/
BOOLEAN
EFIAPI
UnderCurrentScreen (
  IN UINTN FileRow
  )
{
  //
  // if is to the under of the screen
  //
  if (FileRow > FileBuffer.LowVisibleRange.Row + (MainEditor.ScreenSize.Row - 2) - 1) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check user specified FileCol is left to current screen.

  @param[in] FileCol    The column of file position ( start from 1 ).

  @retval TRUE    It is to the left.
  @retval FALSE   It is not to the left.
**/
BOOLEAN
EFIAPI
LeftCurrentScreen (
  IN UINTN FileCol
  )
{
  //
  // if is to the left of the screen
  //
  if (FileCol < FileBuffer.LowVisibleRange.Column) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check user specified FileCol is right to current screen.

  @param[in] FileCol    The column of file position ( start from 1 ).

  @retval TRUE    It is to the right.
  @retval FALSE   It is not to the right.
**/
BOOLEAN
EFIAPI
RightCurrentScreen (
  IN UINTN FileCol
  )
{
  //
  // if is to the right of the screen
  //
  if (FileCol > FileBuffer.LowVisibleRange.Column + MainEditor.ScreenSize.Column - 1) {
    return TRUE;
  }

  return FALSE;
}

/**
  Advance/Retreat lines and set CurrentLine in FileBuffer to it
  
  @param[in] Count The line number to advance/retreat
                     >0 : advance
                     <0: retreat

  @retval NULL An error occured.
  @return The line after advance/retreat.
**/
EFI_EDITOR_LINE *
EFIAPI
MoveCurrentLine (
  IN  INTN Count
  )
{
  EFI_EDITOR_LINE *Line;
  UINTN           AbsCount;

  if (Count <= 0) {
    AbsCount  = (UINTN)ABS(Count);
    Line      = InternalEditorMiscLineRetreat (AbsCount,MainEditor.FileBuffer->CurrentLine,MainEditor.FileBuffer->ListHead);
  } else {
    Line = InternalEditorMiscLineAdvance ((UINTN)Count,MainEditor.FileBuffer->CurrentLine,MainEditor.FileBuffer->ListHead);
  }

  if (Line == NULL) {
    return NULL;
  }

  MainEditor.FileBuffer->CurrentLine = Line;

  return Line;
}

/**
  According to cursor's file position, adjust screen display

  @param[in] NewFilePosRow    The row of file position ( start from 1 ).
  @param[in] NewFilePosCol    The column of file position ( start from 1 ).
**/
VOID
EFIAPI
FileBufferMovePosition (
  IN CONST UINTN NewFilePosRow,
  IN CONST UINTN NewFilePosCol
  )
{
  INTN    RowGap;
  INTN    ColGap;
  UINTN   Abs;
  BOOLEAN Above;
  BOOLEAN Under;
  BOOLEAN Right;
  BOOLEAN Left;

  //
  // CALCULATE gap between current file position and new file position
  //
  RowGap  = NewFilePosRow - FileBuffer.FilePosition.Row;
  ColGap  = NewFilePosCol - FileBuffer.FilePosition.Column;

  Under   = UnderCurrentScreen (NewFilePosRow);
  Above   = AboveCurrentScreen (NewFilePosRow);
  //
  // if is below current screen
  //
  if (Under) {
    //
    // display row will be unchanged
    //
    FileBuffer.FilePosition.Row = NewFilePosRow;
  } else {
    if (Above) {
      //
      // has enough above line, so display row unchanged
      // not has enough above lines, so the first line is at the
      // first display line
      //
      if (NewFilePosRow < (FileBuffer.DisplayPosition.Row - 1)) {
        FileBuffer.DisplayPosition.Row = NewFilePosRow + 1;
      }

      FileBuffer.FilePosition.Row = NewFilePosRow;
    } else {
      //
      // in current screen
      //
      FileBuffer.FilePosition.Row = NewFilePosRow;
      if (RowGap < 0) {
        Abs = (UINTN)ABS(RowGap);
        FileBuffer.DisplayPosition.Row -= Abs;
      } else {
        FileBuffer.DisplayPosition.Row += RowGap;
      }
    }
  }

  FileBuffer.LowVisibleRange.Row  = FileBuffer.FilePosition.Row - (FileBuffer.DisplayPosition.Row - 2);

  Right = RightCurrentScreen (NewFilePosCol);
  Left = LeftCurrentScreen (NewFilePosCol);

  //
  // if right to current screen
  //
  if (Right) {
    //
    // display column will be changed to end
    //
    FileBuffer.DisplayPosition.Column = MainEditor.ScreenSize.Column;
    FileBuffer.FilePosition.Column    = NewFilePosCol;
  } else {
    if (Left) {
      //
      // has enough left characters , so display row unchanged
      // not has enough left characters,
      // so the first character is at the first display column
      //
      if (NewFilePosCol < (FileBuffer.DisplayPosition.Column)) {
        FileBuffer.DisplayPosition.Column = NewFilePosCol;
      }

      FileBuffer.FilePosition.Column = NewFilePosCol;
    } else {
      //
      // in current screen
      //
      FileBuffer.FilePosition.Column = NewFilePosCol;
      if (ColGap < 0) {
        Abs = (UINTN)(-ColGap);
        FileBuffer.DisplayPosition.Column -= Abs;
      } else {
        FileBuffer.DisplayPosition.Column += ColGap;
      }
    }
  }

  FileBuffer.LowVisibleRange.Column = FileBuffer.FilePosition.Column - (FileBuffer.DisplayPosition.Column - 1);

  //
  // let CurrentLine point to correct line;
  //
  FileBuffer.CurrentLine = MoveCurrentLine (RowGap);

}

/**
  Cut current line out and return a pointer to it.

  @param[out] CutLine    Upon a successful return pointer to the pointer to 
                        the allocated cut line.

  @retval EFI_SUCCESS             The cut was successful.
  @retval EFI_NOT_FOUND           There was no selection to cut.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferCutLine (
  OUT EFI_EDITOR_LINE **CutLine
  )
{
  EFI_EDITOR_LINE *Line;
  EFI_EDITOR_LINE *NewLine;
  UINTN           Row;
  UINTN           Col;

  if (FileBuffer.ReadOnly) {
    StatusBarSetStatusString (L"Read Only File Can Not Be Modified");
    return EFI_SUCCESS;
  }

  Line = FileBuffer.CurrentLine;

  //
  // if is the last dummy line, SO CAN not cut
  //
  if (StrCmp (Line->Buffer, L"\0") == 0 && Line->Link.ForwardLink == FileBuffer.ListHead
  //
  // last line
  //
  ) {
    //
    // LAST LINE AND NOTHING ON THIS LINE, SO CUT NOTHING
    //
    StatusBarSetStatusString (L"Nothing to Cut");
    return EFI_NOT_FOUND;
  }
  //
  // if is the last line, so create a dummy line
  //
  if (Line->Link.ForwardLink == FileBuffer.ListHead) {
    //
    // last line
    // create a new line
    //
    NewLine = FileBufferCreateLine ();
    if (NewLine == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  FileBuffer.NumLines--;
  Row = FileBuffer.FilePosition.Row;
  Col = 1;
  //
  // move home
  //
  FileBuffer.CurrentLine = CR (
                            FileBuffer.CurrentLine->Link.ForwardLink,
                            EFI_EDITOR_LINE,
                            Link,
                            LINE_LIST_SIGNATURE
                            );

  RemoveEntryList (&Line->Link);

  FileBuffer.Lines = CR (FileBuffer.ListHead->ForwardLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

  FileBufferMovePosition (Row, Col);

  FileBuffer.FileModified       = TRUE;
  FileBufferNeedRefresh         = TRUE;
  FileBufferOnlyLineNeedRefresh = FALSE;

  *CutLine                      = Line;

  return EFI_SUCCESS;
}

/**
  Paste a line into line list.

  @retval EFI_SUCCESS             The paste was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferPasteLine (
  VOID
  )
{
  EFI_EDITOR_LINE *Line;
  EFI_EDITOR_LINE *NewLine;
  UINTN           Row;
  UINTN           Col;

  //
  // if nothing is on clip board
  // then do nothing
  //
  if (MainEditor.CutLine == NULL) {
    return EFI_SUCCESS;
  }
  //
  // read only file can not be pasted on
  //
  if (FileBuffer.ReadOnly) {
    StatusBarSetStatusString (L"Read Only File Can Not Be Modified");
    return EFI_SUCCESS;
  }

  NewLine = LineDup (MainEditor.CutLine);
  if (NewLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // insert it above current line
  //
  Line                    = FileBuffer.CurrentLine;
  NewLine->Link.BackLink     = Line->Link.BackLink;
  NewLine->Link.ForwardLink     = &Line->Link;

  Line->Link.BackLink->ForwardLink = &NewLine->Link;
  Line->Link.BackLink        = &NewLine->Link;

  FileBuffer.NumLines++;
  FileBuffer.CurrentLine  = NewLine;

  FileBuffer.Lines        = CR (FileBuffer.ListHead->ForwardLink, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

  Col                     = 1;
  //
  // move home
  //
  Row = FileBuffer.FilePosition.Row;

  FileBufferMovePosition (Row, Col);

  //
  // after paste, set some value so that refresh knows to do something
  //
  FileBuffer.FileModified       = TRUE;
  FileBufferNeedRefresh         = TRUE;
  FileBufferOnlyLineNeedRefresh = FALSE;

  return EFI_SUCCESS;
}

/**
  Search string from current position on in file

  @param[in] Str    The search string.
  @param[in] Offset The offset from current position.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_NOT_FOUND     The string Str was not found.
**/
EFI_STATUS
EFIAPI
FileBufferSearch (
  IN CONST CHAR16  *Str,
  IN CONST UINTN Offset
  )
{
  CHAR16          *Current;
  UINTN           Position;
  UINTN           Row;
  UINTN           Column;
  EFI_EDITOR_LINE *Line;
  CHAR16          *CharPos;
  LIST_ENTRY      *Link;
  BOOLEAN         Found;

  Column = 0;
  Position = 0;
  
  //
  // search if in current line
  //
  Current = FileBuffer.CurrentLine->Buffer + FileBuffer.FilePosition.Column - 1 + Offset;

  if (Current >= (FileBuffer.CurrentLine->Buffer + FileBuffer.CurrentLine->Size)) {
    //
    // the end
    //
    Current = FileBuffer.CurrentLine->Buffer + FileBuffer.CurrentLine->Size;
  }

  Found = FALSE;

  CharPos  =  StrStr (Current, Str);
  if (CharPos != NULL) {
    Position = CharPos - Current + 1;
    Found   = TRUE;
  } 

  //
  // found
  //
  if (Found) {
    Column  = (Position - 1) + FileBuffer.FilePosition.Column + Offset;
    Row     = FileBuffer.FilePosition.Row;
  } else {
    //
    // not found so find through next lines
    //
    Link  = FileBuffer.CurrentLine->Link.ForwardLink;

    Row   = FileBuffer.FilePosition.Row + 1;
    while (Link != FileBuffer.ListHead) {
      Line      = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
//      Position  = StrStr (Line->Buffer, Str);
      CharPos  =  StrStr (Line->Buffer, Str);
      if (CharPos != NULL) {
        Position = CharPos - Line->Buffer + 1;
        Found   = TRUE;
      } 
      
      if (Found) {
        //
        // found
        //
        Column = Position;
        break;
      }

      Row++;
      Link = Link->ForwardLink;
    }

    if (Link == FileBuffer.ListHead) {
      Found = FALSE;
    } else {
      Found = TRUE;
    }
  }

  if (!Found) {
    return EFI_NOT_FOUND;
  }

  FileBufferMovePosition (Row, Column);

  //
  // call refresh to fresh edit area,
  // because the outer may loop to find multiply occurrence of this string
  //
  FileBufferRefresh ();

  return EFI_SUCCESS;
}

/**
  Replace SearchLen characters from current position on with Replace.

  This will modify the current buffer at the current position.

  @param[in] Replace    The string to replace.
  @param[in] SearchLen  Search string's length.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferReplace (
  IN CONST CHAR16   *Replace,
  IN CONST UINTN    SearchLen
  )
{
  UINTN   ReplaceLen;
  UINTN   Index;
  CHAR16  *Buffer;
  UINTN   NewSize;
  UINTN   OldSize;
  UINTN   Gap;

  ReplaceLen  = StrLen (Replace);

  OldSize     = FileBuffer.CurrentLine->Size + 1;
  //
  // include CHAR_NULL
  //
  NewSize = OldSize + (ReplaceLen - SearchLen);

  if (ReplaceLen > SearchLen) {
    //
    // do not have the enough space
    //
    if (FileBuffer.CurrentLine->TotalSize + 1 <= NewSize) {
      FileBuffer.CurrentLine->Buffer = ReallocatePool (
                                        2 * OldSize,
                                        2 * NewSize,
                                        FileBuffer.CurrentLine->Buffer
                                        );
      FileBuffer.CurrentLine->TotalSize = NewSize - 1;
    }

    if (FileBuffer.CurrentLine->Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // the end CHAR_NULL character;
    //
    Buffer  = FileBuffer.CurrentLine->Buffer + (NewSize - 1);
    Gap     = ReplaceLen - SearchLen;

    //
    // keep the latter part
    //
    for (Index = 0; Index < (FileBuffer.CurrentLine->Size - FileBuffer.FilePosition.Column - SearchLen + 2); Index++) {
      *Buffer = *(Buffer - Gap);
      Buffer--;
    }
    //
    // set replace into it
    //
    Buffer = FileBuffer.CurrentLine->Buffer + FileBuffer.FilePosition.Column - 1;
    for (Index = 0; Index < ReplaceLen; Index++) {
      Buffer[Index] = Replace[Index];
    }
  }

  if (ReplaceLen < SearchLen) {
    Buffer = FileBuffer.CurrentLine->Buffer + FileBuffer.FilePosition.Column - 1;

    for (Index = 0; Index < ReplaceLen; Index++) {
      Buffer[Index] = Replace[Index];
    }

    Buffer += ReplaceLen;
    Gap = SearchLen - ReplaceLen;

    //
    // set replace into it
    //
    for (Index = 0; Index < (FileBuffer.CurrentLine->Size - FileBuffer.FilePosition.Column - ReplaceLen + 2); Index++) {
      *Buffer = *(Buffer + Gap);
      Buffer++;
    }
  }

  if (ReplaceLen == SearchLen) {
    Buffer = FileBuffer.CurrentLine->Buffer + FileBuffer.FilePosition.Column - 1;
    for (Index = 0; Index < ReplaceLen; Index++) {
      Buffer[Index] = Replace[Index];
    }
  }

  FileBuffer.CurrentLine->Size += (ReplaceLen - SearchLen);

  FileBufferOnlyLineNeedRefresh = TRUE;

  FileBuffer.FileModified       = TRUE;

  MainTitleBarRefresh (MainEditor.FileBuffer->FileName, MainEditor.FileBuffer->FileType, MainEditor.FileBuffer->ReadOnly, MainEditor.FileBuffer->FileModified, MainEditor.ScreenSize.Column, MainEditor.ScreenSize.Row, 0, 0);
  FileBufferRestorePosition ();
  FileBufferRefresh ();

  return EFI_SUCCESS;
}

/**
  Move the mouse cursor position.

  @param[in] TextX      The new x-coordinate.
  @param[in] TextY      The new y-coordinate.
**/
VOID
EFIAPI
FileBufferAdjustMousePosition (
  IN CONST INT32 TextX,
  IN CONST INT32 TextY
  )
{
  UINTN CoordinateX;
  UINTN CoordinateY;
  UINTN AbsX;
  UINTN AbsY;

  //
  // TextX and TextY is mouse movement data returned by mouse driver
  // This function will change it to MousePosition
  //
  //
  // get absolute value
  //

  AbsX = ABS(TextX);
  AbsY = ABS(TextY);

  CoordinateX = FileBuffer.MousePosition.Column;
  CoordinateY = FileBuffer.MousePosition.Row;

  if (TextX >= 0) {
    CoordinateX += TextX;
  } else {
    if (CoordinateX >= AbsX) {
      CoordinateX -= AbsX;
    } else {
      CoordinateX = 0;
    }
  }

  if (TextY >= 0) {
    CoordinateY += TextY;
  } else {
    if (CoordinateY >= AbsY) {
      CoordinateY -= AbsY;
    } else {
      CoordinateY = 0;
    }
  }
  //
  // check whether new mouse column position is beyond screen
  // if not, adjust it
  //
  if (CoordinateX >= 1 && CoordinateX <= MainEditor.ScreenSize.Column) {
    FileBuffer.MousePosition.Column = CoordinateX;
  } else if (CoordinateX < 1) {
    FileBuffer.MousePosition.Column = 1;
  } else if (CoordinateX > MainEditor.ScreenSize.Column) {
    FileBuffer.MousePosition.Column = MainEditor.ScreenSize.Column;
  }
  //
  // check whether new mouse row position is beyond screen
  // if not, adjust it
  //
  if (CoordinateY >= 2 && CoordinateY <= (MainEditor.ScreenSize.Row - 1)) {
    FileBuffer.MousePosition.Row = CoordinateY;
  } else if (CoordinateY < 2) {
    FileBuffer.MousePosition.Row = 2;
  } else if (CoordinateY > (MainEditor.ScreenSize.Row - 1)) {
    FileBuffer.MousePosition.Row = (MainEditor.ScreenSize.Row - 1);
  }

}

/**
  Search and replace operation.

  @param[in] SearchStr    The string to search for.
  @param[in] ReplaceStr   The string to replace with.
  @param[in] Offset       The column to start at.
**/
EFI_STATUS
EFIAPI
FileBufferReplaceAll (
  IN CHAR16 *SearchStr,
  IN CHAR16 *ReplaceStr,
  IN UINTN  Offset
  )
{
  CHAR16          *Buffer;
  UINTN           Position;
  UINTN           Column;
  UINTN           ReplaceLen;
  UINTN           SearchLen;
  UINTN           Index;
  UINTN           NewSize;
  UINTN           OldSize;
  UINTN           Gap;
  EFI_EDITOR_LINE *Line;
  LIST_ENTRY      *Link;
  CHAR16          *CharPos;

  SearchLen   = StrLen (SearchStr);
  ReplaceLen  = StrLen (ReplaceStr);

  Column      = FileBuffer.FilePosition.Column + Offset - 1;

  if (Column > FileBuffer.CurrentLine->Size) {
    Column = FileBuffer.CurrentLine->Size;
  }

  Link = &(FileBuffer.CurrentLine->Link);

  while (Link != FileBuffer.ListHead) {
    Line      = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);
    CharPos  =  StrStr (Line->Buffer + Column, SearchStr);
    if (CharPos != NULL) {
      Position = CharPos - Line->Buffer;// + Column;
      //
      // found
      //
      if (ReplaceLen > SearchLen) {
        OldSize = Line->Size + 1;
        //
        // include CHAR_NULL
        //
        NewSize = OldSize + (ReplaceLen - SearchLen);

        //
        // do not have the enough space
        //
        if (Line->TotalSize + 1 <= NewSize) {
          Line->Buffer = ReallocatePool (
                          2 * OldSize,
                          2 * NewSize,
                          Line->Buffer
                          );
          Line->TotalSize = NewSize - 1;
        }

        if (Line->Buffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        //
        // the end CHAR_NULL character;
        //
        Buffer  = Line->Buffer + (NewSize - 1);
        Gap     = ReplaceLen - SearchLen;

        //
        // keep the latter part
        //
        for (Index = 0; Index < (Line->Size - Position - SearchLen + 1); Index++) {
          *Buffer = *(Buffer - Gap);
          Buffer--;
        }

      } else if (ReplaceLen < SearchLen){
        Buffer  = Line->Buffer + Position + ReplaceLen;
        Gap     = SearchLen - ReplaceLen;

        for (Index = 0; Index < (Line->Size - Position - ReplaceLen + 1); Index++) {
          *Buffer = *(Buffer + Gap);
          Buffer++;
        }
      } else {
        ASSERT(ReplaceLen == SearchLen);
      }
      //
      // set replace into it
      //
      Buffer = Line->Buffer + Position;
      for (Index = 0; Index < ReplaceLen; Index++) {
        Buffer[Index] = ReplaceStr[Index];
      }

      Line->Size += (ReplaceLen - SearchLen);
      Column += ReplaceLen;
    } else {
      //
      // not found
      //
      Column  = 0;
      Link    = Link->ForwardLink;
    }
  }
  //
  // call refresh to fresh edit area
  //
  FileBuffer.FileModified = TRUE;
  FileBufferNeedRefresh   = TRUE;
  FileBufferRefresh ();

  return EFI_SUCCESS;
}

/**
  Set the modified state to TRUE.
**/
VOID
EFIAPI
FileBufferSetModified (
  VOID
  )
{
  FileBuffer.FileModified = TRUE;
}

