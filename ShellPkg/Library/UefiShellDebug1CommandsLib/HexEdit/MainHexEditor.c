/** @file
    Defines the Main Editor data type -
     - Global variables
     - Instances of the other objects of the editor
     - Main Interfaces

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HexEditor.h"
#include "EditStatusBar.h"
#include "EditInputBar.h"

HEFI_EDITOR_COLOR_ATTRIBUTES    HOriginalColors;
INTN                            HOriginalMode;

//
// the first time editor launch
//
BOOLEAN                         HEditorFirst;

//
// it's time editor should exit
//
BOOLEAN                         HEditorExit;

BOOLEAN                         HEditorMouseAction;

extern HEFI_EDITOR_BUFFER_IMAGE HBufferImage;
extern HEFI_EDITOR_BUFFER_IMAGE HBufferImageBackupVar;

extern BOOLEAN                  HBufferImageMouseNeedRefresh;
extern BOOLEAN                  HBufferImageNeedRefresh;
extern BOOLEAN                  HBufferImageOnlyLineNeedRefresh;

HEFI_EDITOR_GLOBAL_EDITOR       HMainEditor;
HEFI_EDITOR_GLOBAL_EDITOR       HMainEditorBackupVar;

//
// basic initialization for MainEditor
//
HEFI_EDITOR_GLOBAL_EDITOR       HMainEditorConst = {
  &HBufferImage,
  {
    {0, 0}
  },
  {
    0,
    0
  },
  NULL,
  FALSE,
  NULL,
  0,
  0,
  1,
  1
};

/**
  Help info that will be displayed.
**/
EFI_STRING_ID  HexMainMenuHelpInfo[] = {
  STRING_TOKEN(STR_HEXEDIT_HELP_TITLE),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_LIST_TITLE),
  STRING_TOKEN(STR_HEXEDIT_HELP_DIV),
  STRING_TOKEN(STR_HEXEDIT_HELP_GO_TO_OFFSET),
  STRING_TOKEN(STR_HEXEDIT_HELP_SAVE_BUFFER),
  STRING_TOKEN(STR_HEXEDIT_HELP_EXIT),
  STRING_TOKEN(STR_HEXEDIT_HELP_SELECT_START),
  STRING_TOKEN(STR_HEXEDIT_HELP_SELECT_END),
  STRING_TOKEN(STR_HEXEDIT_HELP_CUT),
  STRING_TOKEN(STR_HEXEDIT_HELP_PASTE),
  STRING_TOKEN(STR_HEXEDIT_HELP_OPEN_FILE),
  STRING_TOKEN(STR_HEXEDIT_HELP_OPEN_DISK),
  STRING_TOKEN(STR_HEXEDIT_HELP_OPEN_MEMORY),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_EXIT_HELP),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_BLANK),
  STRING_TOKEN(STR_HEXEDIT_HELP_DIV),
  0
};


/**
  show help menu.

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
HMainCommandDisplayHelp (
  VOID
  )
{
  INT32           CurrentLine;
  CHAR16          *InfoString;
  EFI_KEY_DATA    KeyData;
  EFI_STATUS      Status;
  UINTN           EventIndex;

  //
  // print helpInfo
  //
  for (CurrentLine = 0; 0 != HexMainMenuHelpInfo[CurrentLine]; CurrentLine++) {
    InfoString = HiiGetString(gShellDebug1HiiHandle, HexMainMenuHelpInfo[CurrentLine]
, NULL);
    ShellPrintEx (0,CurrentLine+1,L"%E%s%N",InfoString);
  }

  //
  // scan for ctrl+w
  //
  while (TRUE) {
    Status = gBS->WaitForEvent (1, &HMainEditor.TextInputEx->WaitForKeyEx, &EventIndex);
    if (EFI_ERROR (Status) || (EventIndex != 0)) {
      continue;
    }
    Status = HMainEditor.TextInputEx->ReadKeyStrokeEx (HMainEditor.TextInputEx, &KeyData);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (((KeyData.KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) == 0) ||
        (KeyData.KeyState.KeyShiftState == EFI_SHIFT_STATE_VALID)) {
      //
      // For consoles that don't support/report shift state,
      // CTRL+W is translated to L'W' - L'A' + 1.
      //
      if (KeyData.Key.UnicodeChar == L'W' - L'A' + 1) {
        break;
      }
    } else if (((KeyData.KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) != 0) &&
               ((KeyData.KeyState.KeyShiftState & (EFI_LEFT_CONTROL_PRESSED | EFI_RIGHT_CONTROL_PRESSED)) != 0) &&
               ((KeyData.KeyState.KeyShiftState & ~(EFI_SHIFT_STATE_VALID | EFI_LEFT_CONTROL_PRESSED | EFI_RIGHT_CONTROL_PRESSED)) == 0)) {
      //
      // For consoles that supports/reports shift state,
      // make sure that only CONTROL shift key is pressed.
      //
      if ((KeyData.Key.UnicodeChar == 'w') || (KeyData.Key.UnicodeChar == 'W')) {
        break;
      }
    }
  }

  // update screen with buffer's info
  HBufferImageNeedRefresh = TRUE;
  HBufferImageOnlyLineNeedRefresh = FALSE;
  HBufferImageRefresh ();

  return EFI_SUCCESS;
}

/**
  Move cursor to specified lines.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
HMainCommandGoToOffset (
  VOID
  )
{
  UINTN       Size;
  UINT64      Offset;
  EFI_STATUS  Status;
  UINTN       FRow;
  UINTN       FCol;

  //
  // variable initialization
  //
  Size    = 0;
  Offset  = 0;
  FRow    = 0;
  FCol    = 0;

  //
  // get offset
  //
  Status = InputBarSetPrompt (L"Go To Offset: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (8);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {

      return EFI_SUCCESS;
    }
    //
    // THE input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      Status = ShellConvertStringToUint64 (InputBarGetString(), &Offset, TRUE, FALSE);
      if (EFI_ERROR (Status)) {
        StatusBarSetStatusString (L"Invalid Offset");
        return EFI_SUCCESS;
      }

      break;
    }
  }

  Size = HBufferImageGetTotalSize ();
  if (Offset >= Size) {
    StatusBarSetStatusString (L"Invalid Offset");
    return EFI_SUCCESS;
  }

  FRow  = (UINTN)DivU64x32(Offset , 0x10) + 1;
  FCol  = (UINTN)ModU64x32(Offset , 0x10) + 1;

  HBufferImageMovePosition (FRow, FCol, TRUE);

  HBufferImageNeedRefresh         = TRUE;
  HBufferImageOnlyLineNeedRefresh = FALSE;
  HBufferImageMouseNeedRefresh    = TRUE;

  return EFI_SUCCESS;
}

/**
  Save current opened buffer.
  If is file buffer, you can save to current file name or
  save to another file name.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainCommandSaveBuffer (
  VOID
  )
{
  EFI_STATUS          Status;
  BOOLEAN             Done;
  CHAR16              *FileName;
  BOOLEAN             OldFile;
  CHAR16              *Str;
  EFI_FILE_INFO       *Info;
  SHELL_FILE_HANDLE   ShellFileHandle;

  if (HMainEditor.BufferImage->BufferType != FileTypeFileBuffer) {
    if (!HMainEditor.BufferImage->Modified) {
      return EFI_SUCCESS;
    }

    Status = InputBarSetPrompt (L"Dangerous to save disk/mem buffer. Save (Yes/No/Cancel) ? ");
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // the answer is just one character
    //
    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // loop for user's answer
    // valid answer is just 'y' 'Y', 'n' 'N', 'c' 'C'
    //
    while (1) {
      Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        //
        // want to save this buffer first
        //
        Status = HBufferImageSave (
                  NULL,
                  HMainEditor.BufferImage->DiskImage->Name,
                  HMainEditor.BufferImage->DiskImage->Offset,
                  HMainEditor.BufferImage->DiskImage->Size,
                  HMainEditor.BufferImage->MemImage->Offset,
                  HMainEditor.BufferImage->MemImage->Size,
                  HMainEditor.BufferImage->BufferType
                  );

        if (EFI_ERROR (Status)) {
          StatusBarSetStatusString (L"BufferSave: Problems Writing");
          return Status;
        }

        return EFI_SUCCESS;

      case L'n':
      case L'N':
        //
        // the file won't be saved
        //
        return EFI_SUCCESS;

      case L'c':
      case L'C':
        return EFI_SUCCESS;
      }
      //
      // end of switch
      //
    }
    //
    // ENDOF WHILE
    //
  }
  //
  // ENDOF != FILEBUFFER
  //
  // This command will save currently opened file to disk.
  // You can choose save to another file name or just save to
  // current file name.
  // Below is the scenario of Save File command: (
  //    Suppose the old file name is A )
  // 1. An Input Bar will be prompted:    "File To Save: [ old file name]"
  //    IF user press ESC, Save File command ends .
  //    IF user press Enter, input file name will be A.
  //    IF user inputs a new file name B,  input file name will be B.
  //
  // 2. IF input file name is A, go to do Step 3.
  //    IF input file name is B, go to do Step 4.
  //
  // 3. IF A is read only, Status Bar will show "Access Denied"
  //       and Save File commands ends.
  //    IF A is not read only, save file buffer to disk
  //       and remove Modified flag in Title Bar , then Save File command ends.
  //
  // 4. IF B does not exist, create this file and save file buffer to it.
  //       Go to do Step 7.
  //    IF B exits, do Step 5.
  //
  // 5. An Input Bar will be prompted:
  //       "File Exists. Overwrite ( Yes/No/Cancel ) ?"
  //      IF user press 'y' or 'Y', do Step 6.
  //      IF user press 'n' or 'N', Save File commands ends.
  //      IF user press 'c' or 'C' or ESC, Save File commands ends.
  //
  // 6. IF B is a read-only file, Status Bar will show "Access Denied"
  //       and Save File commands ends.
  //    IF B can be read and write, save file buffer to B.
  //
  // 7. Update File Name field in Title Bar to B
  //       and remove the Modified flag in Title Bar.
  //
  Str = CatSPrint(NULL,
          L"File to Save: [%s]",
          HMainEditor.BufferImage->FileImage->FileName
          );
  if (Str == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (StrLen (Str) >= 50) {
    //
    // replace the long file name with "..."
    //
    Str[46] = L'.';
    Str[47] = L'.';
    Str[48] = L'.';
    Str[49] = L']';
    Str[50] = L'\0';
  }

  Status = InputBarSetPrompt (Str);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (100);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // get new file name
  //
  Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

  //
  // if user pressed ESC
  //
  if (Status == EFI_NOT_READY) {
    SHELL_FREE_NON_NULL (Str);
    return EFI_SUCCESS;
  }

  SHELL_FREE_NON_NULL (Str);

  //
  // if just enter pressed, so think save to current file name
  //
  if (StrLen (InputBarGetString()) == 0) {
    FileName = CatSPrint(NULL,
                L"%s",
                HMainEditor.BufferImage->FileImage->FileName
                );
  } else {
    FileName = CatSPrint(NULL, L"%s", InputBarGetString());
  }

  if (FileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (!IsValidFileName (FileName)) {
    StatusBarSetStatusString (L"Invalid File Name");
    SHELL_FREE_NON_NULL (FileName);
    return EFI_SUCCESS;
  }

  OldFile = FALSE;

  //
  // save to the old file
  //
  if (StringNoCaseCompare (
        &FileName,
        &HMainEditor.BufferImage->FileImage->FileName
        ) == 0) {
    OldFile = TRUE;
  }

  if (OldFile) {
    //
    // if the file is read only, so can not write back to it.
    //
    if (HMainEditor.BufferImage->FileImage->ReadOnly) {
      StatusBarSetStatusString (L"Access Denied");
      SHELL_FREE_NON_NULL (FileName);
      return EFI_SUCCESS;
    }
  } else {
    Status = ShellOpenFileByName (FileName, &ShellFileHandle, EFI_FILE_MODE_READ, 0);

    if (!EFI_ERROR (Status)) {

      Info = ShellGetFileInfo(ShellFileHandle);

      ShellCloseFile(&ShellFileHandle);
      //
      // check if read only
      //
      if (Info->Attribute & EFI_FILE_READ_ONLY) {
        StatusBarSetStatusString (L"Access Denied");
        SHELL_FREE_NON_NULL (FileName);
        return EFI_SUCCESS;
      }

      SHELL_FREE_NON_NULL(Info);
      //
      // ask user whether to overwrite this file
      //
      Status = InputBarSetPrompt (L"File exists. Overwrite (Yes/No/Cancel) ? ");
      if (EFI_ERROR (Status)) {
        SHELL_FREE_NON_NULL (FileName);
        return Status;
      }

      Status = InputBarSetStringSize (1);
      if (EFI_ERROR (Status)) {
        SHELL_FREE_NON_NULL (FileName);
        return Status;
      }

      Done = FALSE;
      while (!Done) {
        Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

        if (Status == EFI_NOT_READY) {
          SHELL_FREE_NON_NULL (FileName);
          return EFI_SUCCESS;
        }

        switch (InputBarGetString()[0]) {
        case L'y':
        case L'Y':
          Done = TRUE;
          break;
        case L'n':
        case L'N':
          SHELL_FREE_NON_NULL (FileName);
          return EFI_SUCCESS;
        case L'c':
        case L'C':
          SHELL_FREE_NON_NULL (FileName);
          return EFI_SUCCESS;
        } // switch
      } // while
    } // if opened existing file
  } // if OldFile

  //
  // save file back to disk
  //
  Status = HBufferImageSave (
            FileName,
            HMainEditor.BufferImage->DiskImage->Name,
            HMainEditor.BufferImage->DiskImage->Offset,
            HMainEditor.BufferImage->DiskImage->Size,
            HMainEditor.BufferImage->MemImage->Offset,
            HMainEditor.BufferImage->MemImage->Size,
            FileTypeFileBuffer
            );
  SHELL_FREE_NON_NULL (FileName);

  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Load a disk buffer editor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainCommandSelectStart (
  VOID
  )
{
  UINTN Start;

  Start = (HMainEditor.BufferImage->BufferPosition.Row - 1) * 0x10 + HMainEditor.BufferImage->BufferPosition.Column;

  //
  // last line
  //
  if (HMainEditor.BufferImage->CurrentLine->Link.ForwardLink == HMainEditor.BufferImage->ListHead) {
    if (HMainEditor.BufferImage->BufferPosition.Column > HMainEditor.BufferImage->CurrentLine->Size) {
      StatusBarSetStatusString (L"Invalid Block Start");
      return EFI_LOAD_ERROR;
    }
  }

  if (HMainEditor.SelectEnd != 0 && Start > HMainEditor.SelectEnd) {
    StatusBarSetStatusString (L"Invalid Block Start");
    return EFI_LOAD_ERROR;
  }

  HMainEditor.SelectStart = Start;

  HBufferImageNeedRefresh = TRUE;

  return EFI_SUCCESS;
}

/**
  Load a disk buffer editor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainCommandSelectEnd (
  VOID
  )
{
  UINTN End;

  End = (HMainEditor.BufferImage->BufferPosition.Row - 1) * 0x10 + HMainEditor.BufferImage->BufferPosition.Column;

  //
  // last line
  //
  if (HMainEditor.BufferImage->CurrentLine->Link.ForwardLink == HMainEditor.BufferImage->ListHead) {
    if (HMainEditor.BufferImage->BufferPosition.Column > HMainEditor.BufferImage->CurrentLine->Size) {
      StatusBarSetStatusString (L"Invalid Block End");
      return EFI_LOAD_ERROR;
    }
  }

  if (HMainEditor.SelectStart != 0 && End < HMainEditor.SelectStart) {
    StatusBarSetStatusString (L"Invalid Block End");
    return EFI_SUCCESS;
  }

  HMainEditor.SelectEnd   = End;

  HBufferImageNeedRefresh = TRUE;

  return EFI_SUCCESS;
}

/**
  Cut current line to clipboard.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainCommandCut (
  VOID
  )
{
  UINTN             Index;
  LIST_ENTRY    *Link;
  UINT8             *Buffer;
  UINTN             Count;

  //
  // not select, so not allowed to cut
  //
  if (HMainEditor.SelectStart == 0) {
    StatusBarSetStatusString (L"No Block is Selected");
    return EFI_SUCCESS;
  }
  //
  // not select, so not allowed to cut
  //
  if (HMainEditor.SelectEnd == 0) {
    StatusBarSetStatusString (L"No Block is Selected");
    return EFI_SUCCESS;
  }

  Link = HMainEditor.BufferImage->ListHead->ForwardLink;
  for (Index = 0; Index < (HMainEditor.SelectEnd - 1) / 0x10; Index++) {
    Link = Link->ForwardLink;
  }

  Count   = HMainEditor.SelectEnd - HMainEditor.SelectStart + 1;
  Buffer  = AllocateZeroPool (Count);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // cut the selected area
  //
  HBufferImageDeleteCharacterFromBuffer (
    HMainEditor.SelectStart - 1,
    Count,
    Buffer
    );

  //
  // put to clipboard
  //
  HClipBoardSet (Buffer, Count);

  HBufferImageNeedRefresh         = TRUE;
  HBufferImageOnlyLineNeedRefresh = FALSE;

  if (!HMainEditor.BufferImage->Modified) {
    HMainEditor.BufferImage->Modified = TRUE;
  }

  //
  // now no select area
  //
  HMainEditor.SelectStart = 0;
  HMainEditor.SelectEnd   = 0;

  return EFI_SUCCESS;
}

/**
  Paste line to file buffer.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainCommandPaste (
  VOID
  )
{

  BOOLEAN           OnlyLineRefresh;
  HEFI_EDITOR_LINE  *Line;
  UINT8             *Buffer;
  UINTN             Count;
  UINTN             FPos;

  Count = HClipBoardGet (&Buffer);
  if (Count == 0 || Buffer == NULL) {
    StatusBarSetStatusString (L"Nothing to Paste");
    return EFI_SUCCESS;
  }

  Line            = HMainEditor.BufferImage->CurrentLine;

  OnlyLineRefresh = FALSE;
  if (Line->Link.ForwardLink == HMainEditor.BufferImage->ListHead && Line->Size + Count < 0x10) {
    //
    // is at last line, and after paste will not exceed
    // so only this line need to be refreshed
    //
    // if after add, the line is 0x10, then will append a new line
    // so the whole page will need be refreshed
    //
    OnlyLineRefresh = TRUE;

  }

  FPos = 0x10 * (HMainEditor.BufferImage->BufferPosition.Row - 1) + HMainEditor.BufferImage->BufferPosition.Column - 1;

  HBufferImageAddCharacterToBuffer (FPos, Count, Buffer);

  if (OnlyLineRefresh) {
    HBufferImageNeedRefresh         = FALSE;
    HBufferImageOnlyLineNeedRefresh = TRUE;
  } else {
    HBufferImageNeedRefresh         = TRUE;
    HBufferImageOnlyLineNeedRefresh = FALSE;
  }

  if (!HMainEditor.BufferImage->Modified) {
    HMainEditor.BufferImage->Modified = TRUE;
  }

  return EFI_SUCCESS;

}

/**
  Exit editor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainCommandExit (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Below is the scenario of Exit command:
  // 1. IF currently opened file is not modified, exit the editor and
  //       Exit command ends.
  //    IF currently opened file is modified, do Step 2
  //
  // 2. An Input Bar will be prompted:
  //       "File modified. Save ( Yes/No/Cancel )?"
  //      IF user press 'y' or 'Y', currently opened file will be saved and
  //         Editor exits
  //      IF user press 'n' or 'N', currently opened file will not be saved
  //         and Editor exits.
  //      IF user press 'c' or 'C' or ESC, Exit command ends.
  //
  //
  // if file has been modified, so will prompt user
  //       whether to save the changes
  //
  if (HMainEditor.BufferImage->Modified) {

    Status = InputBarSetPrompt (L"Buffer modified. Save (Yes/No/Cancel) ? ");
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    while (1) {
      Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        //
        // write file back to disk
        //
        Status = HBufferImageSave (
                  HMainEditor.BufferImage->FileImage->FileName,
                  HMainEditor.BufferImage->DiskImage->Name,
                  HMainEditor.BufferImage->DiskImage->Offset,
                  HMainEditor.BufferImage->DiskImage->Size,
                  HMainEditor.BufferImage->MemImage->Offset,
                  HMainEditor.BufferImage->MemImage->Size,
                  HMainEditor.BufferImage->BufferType
                  );
        if (!EFI_ERROR (Status)) {
          HEditorExit = TRUE;
        }

        return Status;

      case L'n':
      case L'N':
        HEditorExit = TRUE;
        return EFI_SUCCESS;

      case L'c':
      case L'C':
        return EFI_SUCCESS;

      }
    }
  }

  HEditorExit = TRUE;
  return EFI_SUCCESS;

}

/**
  Load a file from disk to editor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainCommandOpenFile (
  VOID
  )
{
  BOOLEAN                         Done;
  EFI_STATUS                      Status;
  EDIT_FILE_TYPE                  BufferType;

  BufferType = HMainEditor.BufferImage->BufferType;

  //
  //  This command will open a file from current working directory.
  //  Read-only file can also be opened. But it can not be modified.
  // Below is the scenario of Open File command:
  // 1. IF currently opened file has not been modified, directly go to step .
  //  IF currently opened file has been modified, an Input Bar will be
  //     prompted as :
  //      "File Modified. Save ( Yes/No/Cancel) ?"
  //          IF user press 'y' or 'Y', currently opened file will be saved.
  //          IF user press 'n' or 'N', currently opened file will
  //             not be saved.
  //          IF user press 'c' or 'C' or ESC, Open File command ends and
  //             currently opened file is still opened.
  //
  // 2. An Input Bar will be prompted as :  "File Name to Open: "
  //      IF user press ESC, Open File command ends and
  //         currently opened file is still opened.
  //      Any other inputs with a Return will cause
  //          currently opened file close.
  //
  // 3. IF user input file name is an existing file ,
  //       this file will be read and opened.
  //    IF user input file name is a new file, this file will be created
  //       and opened. This file's type ( UNICODE or ASCII ) is the same with
  //       the old file.
  //
  //
  // if current file is modified, so you need to choose whether to
  //    save it first.
  //
  if (HMainEditor.BufferImage->Modified) {

    Status = InputBarSetPrompt (L"Buffer modified. Save (Yes/No/Cancel) ? ");
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // the answer is just one character
    //
    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // loop for user's answer
    // valid answer is just 'y' 'Y', 'n' 'N', 'c' 'C'
    //
    Done = FALSE;
    while (!Done) {
      Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        //
        // want to save this buffer first
        //
        Status = HBufferImageSave (
                  HMainEditor.BufferImage->FileImage->FileName,
                  HMainEditor.BufferImage->DiskImage->Name,
                  HMainEditor.BufferImage->DiskImage->Offset,
                  HMainEditor.BufferImage->DiskImage->Size,
                  HMainEditor.BufferImage->MemImage->Offset,
                  HMainEditor.BufferImage->MemImage->Size,
                  HMainEditor.BufferImage->BufferType
                  );
        if (EFI_ERROR (Status)) {
          return Status;
        }

        MainTitleBarRefresh (
          HMainEditor.BufferImage->BufferType == FileTypeFileBuffer?HMainEditor.BufferImage->FileImage->FileName:HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Name:NULL,
          HMainEditor.BufferImage->BufferType,
          HMainEditor.BufferImage->FileImage->ReadOnly,
          FALSE,
          HMainEditor.ScreenSize.Column,
          HMainEditor.ScreenSize.Row,
          HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Offset:HMainEditor.BufferImage->BufferType == FileTypeMemBuffer?HMainEditor.BufferImage->MemImage->Offset:0,
          HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Size  :HMainEditor.BufferImage->BufferType == FileTypeMemBuffer?HMainEditor.BufferImage->MemImage->Size  :0
          );
        Done = TRUE;
        break;

      case L'n':
      case L'N':
        //
        // the file won't be saved
        //
        Done = TRUE;
        break;

      case L'c':
      case L'C':
        return EFI_SUCCESS;
      }
    }
  }
  //
  // TO get the open file name
  //
  Status = InputBarSetPrompt (L"File Name to Open: ");
  if (EFI_ERROR (Status)) {
    HBufferImageRead (
      HMainEditor.BufferImage->FileImage->FileName,
      HMainEditor.BufferImage->DiskImage->Name,
      HMainEditor.BufferImage->DiskImage->Offset,
      HMainEditor.BufferImage->DiskImage->Size,
      HMainEditor.BufferImage->MemImage->Offset,
      HMainEditor.BufferImage->MemImage->Size,
      BufferType,
      TRUE
      );
    return Status;
  }

  Status = InputBarSetStringSize (100);
  if (EFI_ERROR (Status)) {
    Status = HBufferImageRead (
              HMainEditor.BufferImage->FileImage->FileName,
              HMainEditor.BufferImage->DiskImage->Name,
              HMainEditor.BufferImage->DiskImage->Offset,
              HMainEditor.BufferImage->DiskImage->Size,
              HMainEditor.BufferImage->MemImage->Offset,
              HMainEditor.BufferImage->MemImage->Size,
              BufferType,
              TRUE
              );
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {
      Status = HBufferImageRead (
                HMainEditor.BufferImage->FileImage->FileName,
                HMainEditor.BufferImage->DiskImage->Name,
                HMainEditor.BufferImage->DiskImage->Offset,
                HMainEditor.BufferImage->DiskImage->Size,
                HMainEditor.BufferImage->MemImage->Offset,
                HMainEditor.BufferImage->MemImage->Size,
                BufferType,
                TRUE
                );

      return Status;
    }
    //
    // THE input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      //
      // CHECK if filename's valid
      //
      if (!IsValidFileName (InputBarGetString())) {
        HBufferImageRead (
          HMainEditor.BufferImage->FileImage->FileName,
          HMainEditor.BufferImage->DiskImage->Name,
          HMainEditor.BufferImage->DiskImage->Offset,
          HMainEditor.BufferImage->DiskImage->Size,
          HMainEditor.BufferImage->MemImage->Offset,
          HMainEditor.BufferImage->MemImage->Size,
          BufferType,
          TRUE
          );

        StatusBarSetStatusString (L"Invalid File Name");
        return EFI_SUCCESS;
      }

      break;
    }
  }
  //
  // read from disk
  //
  Status = HBufferImageRead (
            InputBarGetString(),
            HMainEditor.BufferImage->DiskImage->Name,
            HMainEditor.BufferImage->DiskImage->Offset,
            HMainEditor.BufferImage->DiskImage->Size,
            HMainEditor.BufferImage->MemImage->Offset,
            HMainEditor.BufferImage->MemImage->Size,
            FileTypeFileBuffer,
            FALSE
            );

  if (EFI_ERROR (Status)) {
    HBufferImageRead (
      HMainEditor.BufferImage->FileImage->FileName,
      HMainEditor.BufferImage->DiskImage->Name,
      HMainEditor.BufferImage->DiskImage->Offset,
      HMainEditor.BufferImage->DiskImage->Size,
      HMainEditor.BufferImage->MemImage->Offset,
      HMainEditor.BufferImage->MemImage->Size,
      BufferType,
      TRUE
      );

    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Load a disk buffer editor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
  @retval EFI_NOT_FOUND           The disk was not found.
**/
EFI_STATUS
HMainCommandOpenDisk (
  VOID
  )
{
  UINT64                          Size;
  UINT64                          Offset;
  CHAR16                          *DeviceName;
  EFI_STATUS                      Status;
  BOOLEAN                         Done;

  EDIT_FILE_TYPE                  BufferType;

  //
  // variable initialization
  //
  Size        = 0;
  Offset      = 0;
  BufferType  = HMainEditor.BufferImage->BufferType;

  //
  // if current file is modified, so you need to choose
  // whether to save it first.
  //
  if (HMainEditor.BufferImage->Modified) {

    Status = InputBarSetPrompt (L"Buffer modified. Save (Yes/No/Cancel) ? ");
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // the answer is just one character
    //
    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // loop for user's answer
    // valid answer is just 'y' 'Y', 'n' 'N', 'c' 'C'
    //
    Done = FALSE;
    while (!Done) {
      Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        //
        // want to save this buffer first
        //
        Status = HBufferImageSave (
                  HMainEditor.BufferImage->FileImage->FileName,
                  HMainEditor.BufferImage->DiskImage->Name,
                  HMainEditor.BufferImage->DiskImage->Offset,
                  HMainEditor.BufferImage->DiskImage->Size,
                  HMainEditor.BufferImage->MemImage->Offset,
                  HMainEditor.BufferImage->MemImage->Size,
                  BufferType
                  );
        if (EFI_ERROR (Status)) {
          return Status;
        }

        MainTitleBarRefresh (
          HMainEditor.BufferImage->BufferType == FileTypeFileBuffer?HMainEditor.BufferImage->FileImage->FileName:HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Name:NULL,
          HMainEditor.BufferImage->BufferType,
          HMainEditor.BufferImage->FileImage->ReadOnly,
          FALSE,
          HMainEditor.ScreenSize.Column,
          HMainEditor.ScreenSize.Row,
          HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Offset:HMainEditor.BufferImage->BufferType == FileTypeMemBuffer?HMainEditor.BufferImage->MemImage->Offset:0,
          HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Size  :HMainEditor.BufferImage->BufferType == FileTypeMemBuffer?HMainEditor.BufferImage->MemImage->Size  :0
          );
        Done = TRUE;
        break;

      case L'n':
      case L'N':
        //
        // the file won't be saved
        //
        Done = TRUE;
        break;

      case L'c':
      case L'C':
        return EFI_SUCCESS;
      }
    }
  }
  //
  // get disk block device name
  //
  Status = InputBarSetPrompt (L"Block Device to Open: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (100);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {

      return EFI_SUCCESS;
    }
    //
    // THE input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      break;
    }
  }

  DeviceName = CatSPrint(NULL, L"%s", InputBarGetString());
  if (DeviceName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // get starting offset
  //
  Status = InputBarSetPrompt (L"First Block No.: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (16);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {

      return EFI_SUCCESS;
    }
    //
    // THE input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      Status = ShellConvertStringToUint64 (InputBarGetString(), &Offset, TRUE, FALSE);
      if (EFI_ERROR (Status)) {
        continue;
      }

      break;
    }
  }
  //
  // get Number of Blocks:
  //
  Status = InputBarSetPrompt (L"Number of Blocks: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (8);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {

      return EFI_SUCCESS;
    }
    //
    // THE input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      Status = ShellConvertStringToUint64 (InputBarGetString(), &Size, TRUE, FALSE);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (Size == 0) {
        continue;
      }

      break;
    }
  }

  Status = HBufferImageRead (
            NULL,
            DeviceName,
            (UINTN)Offset,
            (UINTN)Size,
            0,
            0,
            FileTypeDiskBuffer,
            FALSE
            );

  if (EFI_ERROR (Status)) {

    HBufferImageRead (
      HMainEditor.BufferImage->FileImage->FileName,
      HMainEditor.BufferImage->DiskImage->Name,
      HMainEditor.BufferImage->DiskImage->Offset,
      HMainEditor.BufferImage->DiskImage->Size,
      HMainEditor.BufferImage->MemImage->Offset,
      HMainEditor.BufferImage->MemImage->Size,
      BufferType,
      TRUE
      );
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Load memory content to editor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
  @retval EFI_NOT_FOUND           The disk was not found.
**/
EFI_STATUS
HMainCommandOpenMemory (
  VOID
  )
{
  UINT64                          Size;
  UINT64                          Offset;
  EFI_STATUS                      Status;
  BOOLEAN                         Done;
  EDIT_FILE_TYPE                  BufferType;

  //
  // variable initialization
  //
  Size        = 0;
  Offset      = 0;
  BufferType  = HMainEditor.BufferImage->BufferType;

  //
  // if current buffer is modified, so you need to choose
  // whether to save it first.
  //
  if (HMainEditor.BufferImage->Modified) {

    Status = InputBarSetPrompt (L"Buffer modified. Save (Yes/No/Cancel) ? ");
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // the answer is just one character
    //
    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // loop for user's answer
    // valid answer is just 'y' 'Y', 'n' 'N', 'c' 'C'
    //
    Done = FALSE;
    while (!Done) {
      Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        //
        // want to save this buffer first
        //
        Status = HBufferImageSave (
                  HMainEditor.BufferImage->FileImage->FileName,
                  HMainEditor.BufferImage->DiskImage->Name,
                  HMainEditor.BufferImage->DiskImage->Offset,
                  HMainEditor.BufferImage->DiskImage->Size,
                  HMainEditor.BufferImage->MemImage->Offset,
                  HMainEditor.BufferImage->MemImage->Size,
                  BufferType
                  );
        if (EFI_ERROR (Status)) {
          return Status;
        }

        MainTitleBarRefresh (
          HMainEditor.BufferImage->BufferType == FileTypeFileBuffer?HMainEditor.BufferImage->FileImage->FileName:HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Name:NULL,
          HMainEditor.BufferImage->BufferType,
          HMainEditor.BufferImage->FileImage->ReadOnly,
          FALSE,
          HMainEditor.ScreenSize.Column,
          HMainEditor.ScreenSize.Row,
          HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Offset:HMainEditor.BufferImage->BufferType == FileTypeMemBuffer?HMainEditor.BufferImage->MemImage->Offset:0,
          HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer?HMainEditor.BufferImage->DiskImage->Size  :HMainEditor.BufferImage->BufferType == FileTypeMemBuffer?HMainEditor.BufferImage->MemImage->Size  :0
          );
        Done = TRUE;
        break;

      case L'n':
      case L'N':
        //
        // the file won't be saved
        //
        Done = TRUE;
        break;

      case L'c':
      case L'C':
        return EFI_SUCCESS;
      }
    }
  }
  //
  // get starting offset
  //
  Status = InputBarSetPrompt (L"Starting Offset: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (8);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {

      return EFI_SUCCESS;
    }
    //
    // THE input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      Status = ShellConvertStringToUint64 (InputBarGetString(), &Offset, TRUE, FALSE);
      if (EFI_ERROR (Status)) {
        continue;
      }

      break;
    }
  }
  //
  // get Number of Blocks:
  //
  Status = InputBarSetPrompt (L"Buffer Size: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (8);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (HMainEditor.ScreenSize.Row, HMainEditor.ScreenSize.Column);

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {

      return EFI_SUCCESS;
    }
    //
    // THE input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      Status = ShellConvertStringToUint64 (InputBarGetString(), &Size, TRUE, FALSE);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (Size == 0) {
        continue;
      }

      break;
    }
  }

  if ((Offset + Size - 1)> 0xffffffff) {
    StatusBarSetStatusString (L"Invalid parameter");
    return EFI_LOAD_ERROR;
  }

  Status = HBufferImageRead (
            NULL,
            NULL,
            0,
            0,
            (UINTN)Offset,
            (UINTN)Size,
            FileTypeMemBuffer,
            FALSE
            );

  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Read Device Error!");
    HBufferImageRead (
      HMainEditor.BufferImage->FileImage->FileName,
      HMainEditor.BufferImage->DiskImage->Name,
      HMainEditor.BufferImage->DiskImage->Offset,
      HMainEditor.BufferImage->DiskImage->Size,
      HMainEditor.BufferImage->MemImage->Offset,
      HMainEditor.BufferImage->MemImage->Size,
      BufferType,
      TRUE
      );
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;

}

MENU_ITEM_FUNCTION HexMainControlBasedMenuFunctions[] = {
  NULL,
  NULL,                      /* Ctrl - A */
  NULL,                      /* Ctrl - B */
  NULL,                      /* Ctrl - C */
  HMainCommandSelectEnd,     /* Ctrl - D */
  HMainCommandDisplayHelp,   /* Ctrl - E */
  NULL,                      /* Ctrl - F */
  HMainCommandGoToOffset,    /* Ctrl - G */
  NULL,                      /* Ctrl - H */
  HMainCommandOpenDisk,      /* Ctrl - I */
  NULL,                      /* Ctrl - J */
  NULL,                      /* Ctrl - K */
  NULL,                      /* Ctrl - L */
  HMainCommandOpenMemory,    /* Ctrl - M */
  NULL,                      /* Ctrl - N */
  HMainCommandOpenFile,      /* Ctrl - O */
  NULL,                      /* Ctrl - P */
  HMainCommandExit,          /* Ctrl - Q */
  NULL,                      /* Ctrl - R */
  HMainCommandSaveBuffer,    /* Ctrl - S */
  HMainCommandSelectStart,   /* Ctrl - T */
  NULL,                      /* Ctrl - U */
  HMainCommandPaste,         /* Ctrl - V */
  NULL,                      /* Ctrl - W */
  HMainCommandCut,           /* Ctrl - X */
  NULL,                      /* Ctrl - Y */
  NULL,                      /* Ctrl - Z */
};

CONST EDITOR_MENU_ITEM HexEditorMenuItems[] = {
  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_GO_TO_OFFSET),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F1),
    HMainCommandGoToOffset
  },
  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_SAVE_BUFFER),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F2),
    HMainCommandSaveBuffer
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_EXIT),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F3),
    HMainCommandExit
  },

  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_SELECT_START),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F4),
    HMainCommandSelectStart
  },
  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_SELECT_END),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F5),
    HMainCommandSelectEnd
  },
  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_CUT),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F6),
    HMainCommandCut
  },
  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_PASTE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F7),
    HMainCommandPaste
  },

  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_OPEN_FILE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F8),
    HMainCommandOpenFile
  },
  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_OPEN_DISK),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F9),
    HMainCommandOpenDisk
  },
  {
    STRING_TOKEN(STR_HEXEDIT_LIBMENUBAR_OPEN_MEMORY),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F10),
    HMainCommandOpenMemory
  },

  {
    0,
    0,
    NULL
  }
};

/**
  Init function for MainEditor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainEditorInit (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleCount;
  UINTN       Index;

  //
  // basic initialization
  //
  CopyMem (&HMainEditor, &HMainEditorConst, sizeof (HMainEditor));

  //
  // set screen attributes
  //
  HMainEditor.ColorAttributes.Colors.Foreground = gST->ConOut->Mode->Attribute & 0x000000ff;

  HMainEditor.ColorAttributes.Colors.Background = (UINT8) (gST->ConOut->Mode->Attribute >> 4);

  HOriginalColors = HMainEditor.ColorAttributes.Colors;

  HOriginalMode = gST->ConOut->Mode->Mode;

  //
  // query screen size
  //
  gST->ConOut->QueryMode (
        gST->ConOut,
        gST->ConOut->Mode->Mode,
        &(HMainEditor.ScreenSize.Column),
        &(HMainEditor.ScreenSize.Row)
        );

  //
  // Find TextInEx in System Table ConsoleInHandle
  // Per UEFI Spec, TextInEx is required for a console capable platform.
  //
  Status = gBS->HandleProtocol (
              gST->ConsoleInHandle,
              &gEfiSimpleTextInputExProtocolGuid,
              (VOID**)&HMainEditor.TextInputEx
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find mouse in System Table ConsoleInHandle
  //
  Status = gBS->HandleProtocol (
                gST->ConsoleInHandle,
                &gEfiSimplePointerProtocolGuid,
                (VOID**)&HMainEditor.MouseInterface
                );
  if (EFI_ERROR (Status)) {
    //
    // If there is no Simple Pointer Protocol on System Table
    //
    HandleBuffer = NULL;
    HMainEditor.MouseInterface = NULL;
    Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimplePointerProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
    if (!EFI_ERROR (Status) && HandleCount > 0) {
      //
      // Try to find the first available mouse device
      //
      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiSimplePointerProtocolGuid,
                      (VOID**)&HMainEditor.MouseInterface
                      );
        if (!EFI_ERROR (Status)) {
          break;
        }
      }
    }
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  if (!EFI_ERROR (Status) && HMainEditor.MouseInterface != NULL) {
    HMainEditor.MouseAccumulatorX  = 0;
    HMainEditor.MouseAccumulatorY  = 0;
    HMainEditor.MouseSupported     = TRUE;
  }

  //
  // below will call the five components' init function
  //
  Status = MainTitleBarInit (L"UEFI HEXEDIT");
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_MAINEDITOR_TITLE), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }

  Status = ControlHotKeyInit (HexMainControlBasedMenuFunctions);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_MAINEDITOR_MAINMENU), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }
  Status = MenuBarInit (HexEditorMenuItems);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_MAINEDITOR_MAINMENU), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }

  Status = StatusBarInit ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_MAINEDITOR_STATUS), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }

  InputBarInit (HMainEditor.TextInputEx);

  Status = HBufferImageInit ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_MAINEDITOR_BUFFERIMAGE), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }

  Status = HClipBoardInit ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_MAINEDITOR_CLIPBOARD), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }
  //
  // clear whole screen and enable cursor
  //
  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  //
  // initialize EditorFirst and EditorExit
  //
  HEditorFirst        = TRUE;
  HEditorExit         = FALSE;
  HEditorMouseAction  = FALSE;

  return EFI_SUCCESS;
}

/**
  Cleanup function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainEditorCleanup (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // call the five components' cleanup function
  //
  MainTitleBarCleanup ();

  MenuBarCleanup ();

  StatusBarCleanup ();

  InputBarCleanup ();

  Status = HBufferImageCleanup ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_BUFFERIMAGE_CLEAN), gShellDebug1HiiHandle);
  }

  Status = HClipBoardCleanup ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_LIBEDITOR_CLIPBOARD_CLEAN), gShellDebug1HiiHandle);
  }
  //
  // restore old mode
  //
  if (HOriginalMode != gST->ConOut->Mode->Mode) {
    gST->ConOut->SetMode (gST->ConOut, HOriginalMode);
  }

  gST->ConOut->SetAttribute (
        gST->ConOut,
        EFI_TEXT_ATTR (HOriginalColors.Foreground, HOriginalColors.Background)
        );
  gST->ConOut->ClearScreen (gST->ConOut);

  return EFI_SUCCESS;
}

/**
  Refresh function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
HMainEditorRefresh (
  VOID
  )
{
  BOOLEAN NameChange;
  BOOLEAN ReadChange;

  NameChange = FALSE;
  ReadChange = FALSE;

  if (HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer) {
    if (HMainEditor.BufferImage->DiskImage != NULL &&
        HBufferImageBackupVar.DiskImage != NULL &&
        (HMainEditor.BufferImage->DiskImage->Offset != HBufferImageBackupVar.DiskImage->Offset ||
           HMainEditor.BufferImage->DiskImage->Size != HBufferImageBackupVar.DiskImage->Size) ){
      NameChange = TRUE;
    }
  } else if (HMainEditor.BufferImage->BufferType == FileTypeMemBuffer) {
    if (HMainEditor.BufferImage->MemImage != NULL &&
        HBufferImageBackupVar.MemImage != NULL &&
        (HMainEditor.BufferImage->MemImage->Offset != HBufferImageBackupVar.MemImage->Offset ||
           HMainEditor.BufferImage->MemImage->Size != HBufferImageBackupVar.MemImage->Size) ){
      NameChange = TRUE;
    }
  } else if (HMainEditor.BufferImage->BufferType == FileTypeFileBuffer) {
    if ( HMainEditor.BufferImage->FileImage != NULL &&
         HMainEditor.BufferImage->FileImage->FileName != NULL &&
         HBufferImageBackupVar.FileImage != NULL &&
         HBufferImageBackupVar.FileImage->FileName != NULL &&
         StrCmp (HMainEditor.BufferImage->FileImage->FileName, HBufferImageBackupVar.FileImage->FileName) != 0 ) {
      NameChange = TRUE;
    }
  }
  if ( HMainEditor.BufferImage->FileImage != NULL &&
       HBufferImageBackupVar.FileImage != NULL &&
       HMainEditor.BufferImage->FileImage->ReadOnly != HBufferImageBackupVar.FileImage->ReadOnly ) {
    ReadChange = TRUE;
  }

  //
  // to aVOID screen flicker
  // the stall value is from experience
  //
  gBS->Stall (50);

  //
  // call the components refresh function
  //
  if (HEditorFirst
    || NameChange
    || HMainEditor.BufferImage->BufferType != HBufferImageBackupVar.BufferType
    || HBufferImageBackupVar.Modified != HMainEditor.BufferImage->Modified
    || ReadChange ) {

    MainTitleBarRefresh (
      HMainEditor.BufferImage->BufferType == FileTypeFileBuffer&&HMainEditor.BufferImage->FileImage!=NULL?HMainEditor.BufferImage->FileImage->FileName:HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer&&HMainEditor.BufferImage->DiskImage!=NULL?HMainEditor.BufferImage->DiskImage->Name:NULL,
      HMainEditor.BufferImage->BufferType,
      (BOOLEAN)(HMainEditor.BufferImage->FileImage!=NULL?HMainEditor.BufferImage->FileImage->ReadOnly:FALSE),
      HMainEditor.BufferImage->Modified,
      HMainEditor.ScreenSize.Column,
      HMainEditor.ScreenSize.Row,
      HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer&&HMainEditor.BufferImage->DiskImage!=NULL?HMainEditor.BufferImage->DiskImage->Offset:HMainEditor.BufferImage->BufferType == FileTypeMemBuffer&&HMainEditor.BufferImage->MemImage!=NULL?HMainEditor.BufferImage->MemImage->Offset:0,
      HMainEditor.BufferImage->BufferType == FileTypeDiskBuffer&&HMainEditor.BufferImage->DiskImage!=NULL?HMainEditor.BufferImage->DiskImage->Size  :HMainEditor.BufferImage->BufferType == FileTypeMemBuffer&&HMainEditor.BufferImage->MemImage!=NULL?HMainEditor.BufferImage->MemImage->Size  :0
      );
    HBufferImageRefresh ();
  }
  if (HEditorFirst
    || HBufferImageBackupVar.DisplayPosition.Row != HMainEditor.BufferImage->DisplayPosition.Row
    || HBufferImageBackupVar.DisplayPosition.Column != HMainEditor.BufferImage->DisplayPosition.Column
    || StatusBarGetRefresh()) {

    StatusBarRefresh (
      HEditorFirst,
      HMainEditor.ScreenSize.Row,
      HMainEditor.ScreenSize.Column,
      (UINTN)(-1),
      (UINTN)(-1),
      FALSE
      );
    HBufferImageRefresh ();
  }

  if (HEditorFirst) {
    HBufferImageRefresh ();
  }

  //
  // EditorFirst is now set to FALSE
  //
  HEditorFirst = FALSE;

  return EFI_SUCCESS;
}

/**
  Handle the mouse input.

  @param[in] MouseState             The current mouse state.
  @param[out] BeforeLeftButtonDown  helps with selections.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
  @retval EFI_NOT_FOUND           The disk was not found.
**/
EFI_STATUS
HMainEditorHandleMouseInput (
  IN  EFI_SIMPLE_POINTER_STATE       MouseState,
  OUT BOOLEAN                        *BeforeLeftButtonDown
  )
{

  INT32             TextX;
  INT32             TextY;
  UINTN             FRow;
  UINTN             FCol;
  BOOLEAN           HighBits;
  LIST_ENTRY    *Link;
  HEFI_EDITOR_LINE  *Line;
  UINTN             Index;
  BOOLEAN           Action;

  Action = FALSE;

  //
  // have mouse movement
  //
  if (MouseState.RelativeMovementX || MouseState.RelativeMovementY) {
    //
    // handle
    //
    TextX = HGetTextX (MouseState.RelativeMovementX);
    TextY = HGetTextY (MouseState.RelativeMovementY);

    HBufferImageAdjustMousePosition (TextX, TextY);

    Action = TRUE;

  }

  if (MouseState.LeftButton) {
    HighBits = HBufferImageIsAtHighBits (
                HMainEditor.BufferImage->MousePosition.Column,
                &FCol
                );

    //
    // not at an movable place
    //
    if (FCol == 0) {
      //
      // now just move mouse pointer to legal position
      //
      //
      // move mouse position to legal position
      //
      HMainEditor.BufferImage->MousePosition.Column -= 10;
      if (HMainEditor.BufferImage->MousePosition.Column > 24) {
        HMainEditor.BufferImage->MousePosition.Column--;
        FCol = HMainEditor.BufferImage->MousePosition.Column / 3 + 1 + 1;
      } else {
        if (HMainEditor.BufferImage->MousePosition.Column < 24) {
          FCol = HMainEditor.BufferImage->MousePosition.Column / 3 + 1 + 1;
        } else {
          //
          // == 24
          //
          FCol = 9;
        }
      }

      HighBits = TRUE;

    }

    FRow = HMainEditor.BufferImage->BufferPosition.Row +
      HMainEditor.BufferImage->MousePosition.Row -
      HMainEditor.BufferImage->DisplayPosition.Row;

    if (HMainEditor.BufferImage->NumLines < FRow) {
      //
      // dragging
      //
      //
      // now just move mouse pointer to legal position
      //
      FRow      = HMainEditor.BufferImage->NumLines;
      HighBits  = TRUE;
    }

    Link = HMainEditor.BufferImage->ListHead->ForwardLink;
    for (Index = 0; Index < FRow - 1; Index++) {
      Link = Link->ForwardLink;
    }

    Line = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);

    //
    // dragging
    //
    //
    // now just move mouse pointer to legal position
    //
    if (FCol > Line->Size) {
      if (*BeforeLeftButtonDown) {
        HighBits = FALSE;

        if (Line->Size == 0) {
          if (FRow > 1) {
            FRow--;
            FCol = 16;
          } else {
            FRow  = 1;
            FCol  = 1;
          }

        } else {
          FCol = Line->Size;
        }
      } else {
        FCol      = Line->Size + 1;
        HighBits  = TRUE;
      }
    }

    HBufferImageMovePosition (FRow, FCol, HighBits);

    HMainEditor.BufferImage->MousePosition.Row    = HMainEditor.BufferImage->DisplayPosition.Row;

    HMainEditor.BufferImage->MousePosition.Column = HMainEditor.BufferImage->DisplayPosition.Column;

    *BeforeLeftButtonDown                         = TRUE;

    Action = TRUE;
  } else {
    //
    // else of if LButton
    //
    // release LButton
    //
    if (*BeforeLeftButtonDown) {
      Action = TRUE;
    }
    //
    // mouse up
    //
    *BeforeLeftButtonDown = FALSE;
  }

  if (Action) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Handle user key input. will route it to other components handle function.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainEditorKeyInput (
  VOID
  )
{
  EFI_KEY_DATA              KeyData;
  EFI_STATUS                Status;
  EFI_SIMPLE_POINTER_STATE  MouseState;
  BOOLEAN                   NoShiftState;
  BOOLEAN                   LengthChange;
  UINTN                     Size;
  UINTN                     OldSize;
  BOOLEAN                   BeforeMouseIsDown;
  BOOLEAN                   MouseIsDown;
  BOOLEAN                   FirstDown;
  BOOLEAN                   MouseDrag;
  UINTN                     FRow;
  UINTN                     FCol;
  UINTN                     SelectStartBackup;
  UINTN                     SelectEndBackup;

  //
  // variable initialization
  //
  OldSize       = 0;
  FRow          = 0;
  FCol          = 0;
  LengthChange  = FALSE;

  MouseIsDown   = FALSE;
  FirstDown     = FALSE;
  MouseDrag     = FALSE;

  do {

    Status              = EFI_SUCCESS;

    HEditorMouseAction  = FALSE;

    //
    // backup some key elements, so that can aVOID some refresh work
    //
    HMainEditorBackup ();

    //
    // wait for user key input
    //
    //
    // change priority of checking mouse/keyboard activity dynamically
    // so prevent starvation of keyboard.
    // if last time, mouse moves then this time check keyboard
    //
    if (HMainEditor.MouseSupported) {
      Status = HMainEditor.MouseInterface->GetState (
                                            HMainEditor.MouseInterface,
                                            &MouseState
                                            );
      if (!EFI_ERROR (Status)) {

        BeforeMouseIsDown = MouseIsDown;

        Status            = HMainEditorHandleMouseInput (MouseState, &MouseIsDown);

        if (!EFI_ERROR (Status)) {
          if (!BeforeMouseIsDown) {
            //
            // mouse down
            //
            if (MouseIsDown) {
              FRow              = HBufferImage.BufferPosition.Row;
              FCol              = HBufferImage.BufferPosition.Column;
              SelectStartBackup = HMainEditor.SelectStart;
              SelectEndBackup   = HMainEditor.SelectEnd;

              FirstDown         = TRUE;
            }
          } else {

            SelectStartBackup = HMainEditor.SelectStart;
            SelectEndBackup   = HMainEditor.SelectEnd;

            //
            // begin to drag
            //
            if (MouseIsDown) {
              if (FirstDown) {
                if (MouseState.RelativeMovementX || MouseState.RelativeMovementY) {
                  HMainEditor.SelectStart = 0;
                  HMainEditor.SelectEnd   = 0;
                  HMainEditor.SelectStart = (FRow - 1) * 0x10 + FCol;

                  MouseDrag               = TRUE;
                  FirstDown               = FALSE;
                }
              } else {
                if ((
                      (HBufferImage.BufferPosition.Row - 1) *
                    0x10 +
                    HBufferImage.BufferPosition.Column
                      ) >= HMainEditor.SelectStart
                        ) {
                  HMainEditor.SelectEnd = (HBufferImage.BufferPosition.Row - 1) *
                    0x10 +
                    HBufferImage.BufferPosition.Column;
                } else {
                  HMainEditor.SelectEnd = 0;
                }
              }
              //
              // end of if RelativeX/Y
              //
            } else {
              //
              // mouse is up
              //
              if (MouseDrag) {
                if (HBufferImageGetTotalSize () == 0) {
                  HMainEditor.SelectStart = 0;
                  HMainEditor.SelectEnd   = 0;
                  FirstDown               = FALSE;
                  MouseDrag               = FALSE;
                }

                if ((
                      (HBufferImage.BufferPosition.Row - 1) *
                    0x10 +
                    HBufferImage.BufferPosition.Column
                      ) >= HMainEditor.SelectStart
                        ) {
                  HMainEditor.SelectEnd = (HBufferImage.BufferPosition.Row - 1) *
                    0x10 +
                    HBufferImage.BufferPosition.Column;
                } else {
                  HMainEditor.SelectEnd = 0;
                }

                if (HMainEditor.SelectEnd == 0) {
                  HMainEditor.SelectStart = 0;
                }
              }

              FirstDown = FALSE;
              MouseDrag = FALSE;
            }

            if (SelectStartBackup != HMainEditor.SelectStart || SelectEndBackup != HMainEditor.SelectEnd) {
              if ((SelectStartBackup - 1) / 0x10 != (HMainEditor.SelectStart - 1) / 0x10) {
                HBufferImageNeedRefresh = TRUE;
              } else {
                if ((SelectEndBackup - 1) / 0x10 != (HMainEditor.SelectEnd - 1) / 0x10) {
                  HBufferImageNeedRefresh = TRUE;
                } else {
                  HBufferImageOnlyLineNeedRefresh = TRUE;
                }
              }
            }
          }

          HEditorMouseAction            = TRUE;
          HBufferImageMouseNeedRefresh  = TRUE;

        } else if (Status == EFI_LOAD_ERROR) {
          StatusBarSetStatusString (L"Invalid Mouse Movement ");
        }
      }
    }

    //
    // CheckEvent() returns Success when non-partial key is pressed.
    //
    Status = gBS->CheckEvent (HMainEditor.TextInputEx->WaitForKeyEx);
    if (!EFI_ERROR (Status)) {
      Status = HMainEditor.TextInputEx->ReadKeyStrokeEx (HMainEditor.TextInputEx, &KeyData);
      if (!EFI_ERROR (Status)) {
        //
        // dispatch to different components' key handling function
        // so not everywhere has to set this variable
        //
        HBufferImageMouseNeedRefresh = TRUE;

        //
        // clear previous status string
        //
        StatusBarSetRefresh();
        //
        // NoShiftState: TRUE when no shift key is pressed.
        //
        NoShiftState = ((KeyData.KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) == 0) || (KeyData.KeyState.KeyShiftState == EFI_SHIFT_STATE_VALID);
        //
        // dispatch to different components' key handling function
        //
        if (EFI_SUCCESS == MenuBarDispatchControlHotKey(&KeyData)) {
          Status = EFI_SUCCESS;
        } else if (NoShiftState && KeyData.Key.ScanCode == SCAN_NULL) {
          Status = HBufferImageHandleInput (&KeyData.Key);
        } else if (NoShiftState && ((KeyData.Key.ScanCode >= SCAN_UP) && (KeyData.Key.ScanCode <= SCAN_PAGE_DOWN))) {
          Status = HBufferImageHandleInput (&KeyData.Key);
        } else if (NoShiftState && ((KeyData.Key.ScanCode >= SCAN_F1) && KeyData.Key.ScanCode <= SCAN_F12)) {
          Status = MenuBarDispatchFunctionKey (&KeyData.Key);
        } else {
          StatusBarSetStatusString (L"Unknown Command");

          HBufferImageMouseNeedRefresh = FALSE;
        }

        if (Status != EFI_SUCCESS && Status != EFI_OUT_OF_RESOURCES) {
          //
          // not already has some error status
          //
          if (StrCmp (L"", StatusBarGetString()) == 0) {
            StatusBarSetStatusString (L"Disk Error. Try Again");
          }
        }
      }
      //
      // decide if has to set length warning
      //
      if (HBufferImage.BufferType != HBufferImageBackupVar.BufferType) {
        LengthChange = FALSE;
      } else {
        //
        // still the old buffer
        //
        if (HBufferImage.BufferType != FileTypeFileBuffer) {
          Size = HBufferImageGetTotalSize ();

          switch (HBufferImage.BufferType) {
          case FileTypeDiskBuffer:
            OldSize = HBufferImage.DiskImage->Size * HBufferImage.DiskImage->BlockSize;
            break;

          case FileTypeMemBuffer:
            OldSize = HBufferImage.MemImage->Size;
            break;

          default:
            OldSize = 0;
            break;
          }

          if (!LengthChange) {
            if (OldSize != Size) {
              StatusBarSetStatusString (L"Disk/Mem Buffer Length should not be changed");
            }
          }

          if (OldSize != Size) {
            LengthChange = TRUE;
          } else {
            LengthChange = FALSE;
          }
        }
      }
    }
    //
    // after handling, refresh editor
    //
    HMainEditorRefresh ();

  } while (Status != EFI_OUT_OF_RESOURCES && !HEditorExit);

  return Status;
}

/**
  Backup function for MainEditor.
**/
VOID
HMainEditorBackup (
  VOID
  )
{
  HMainEditorBackupVar.SelectStart  = HMainEditor.SelectStart;
  HMainEditorBackupVar.SelectEnd    = HMainEditor.SelectEnd;
  HBufferImageBackup ();
}
