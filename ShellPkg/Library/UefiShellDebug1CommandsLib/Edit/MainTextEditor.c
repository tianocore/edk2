/** @file
  Implements editor interface functions.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TextEditor.h"
#include "EditStatusBar.h"
#include "EditInputBar.h"
#include "EditMenuBar.h"

//
// the first time editor launch
//
BOOLEAN                       EditorFirst;

//
// it's time editor should exit
//
BOOLEAN                       EditorExit;

BOOLEAN                       EditorMouseAction;

extern EFI_EDITOR_FILE_BUFFER FileBuffer;

extern BOOLEAN                FileBufferNeedRefresh;

extern BOOLEAN                FileBufferOnlyLineNeedRefresh;

extern BOOLEAN                FileBufferMouseNeedRefresh;

extern EFI_EDITOR_FILE_BUFFER FileBufferBackupVar;

EFI_EDITOR_GLOBAL_EDITOR      MainEditor;


/**
  Load a file from disk to editor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
MainCommandOpenFile (
  VOID
  );

/**
  Switch a file from ASCII to UNICODE or vise-versa.

  @retval EFI_SUCCESS           The switch was ok or a warning was presented.
**/
EFI_STATUS
MainCommandSwitchFileType (
  VOID
  );

/**
  move cursor to specified lines

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
MainCommandGotoLine (
  VOID
  );

/**
  Save current file to disk, you can save to current file name or
  save to another file name.

  @retval EFI_SUCCESS           The file was saved correctly.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR          A file access error occured.
**/
EFI_STATUS
MainCommandSaveFile (
  VOID
  );

/**
  Show help information for the editor.

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
MainCommandDisplayHelp (
  VOID
  );

/**
  exit editor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandExit (
  VOID
  );

/**
  search string in file buffer

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandSearch (
  VOID
  );

/**
  search string in file buffer, and replace it with another str

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandSearchReplace (
  VOID
  );

/**
  cut current line to clipboard

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandCutLine (
  VOID
  );

/**
  paste line to file buffer.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandPasteLine (
  VOID
  );

/**
  Help info that will be displayed.
**/
EFI_STRING_ID  MainMenuHelpInfo[] = {
  STRING_TOKEN(STR_EDIT_HELP_TITLE),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_LIST_TITLE),
  STRING_TOKEN(STR_EDIT_HELP_DIV),
  STRING_TOKEN(STR_EDIT_HELP_GO_TO_LINE),
  STRING_TOKEN(STR_EDIT_HELP_SAVE_FILE),
  STRING_TOKEN(STR_EDIT_HELP_EXIT),
  STRING_TOKEN(STR_EDIT_HELP_SEARCH),
  STRING_TOKEN(STR_EDIT_HELP_SEARCH_REPLACE),
  STRING_TOKEN(STR_EDIT_HELP_CUT_LINE),
  STRING_TOKEN(STR_EDIT_HELP_PASTE_LINE),
  STRING_TOKEN(STR_EDIT_HELP_OPEN_FILE),
  STRING_TOKEN(STR_EDIT_HELP_FILE_TYPE),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_EXIT_HELP),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_BLANK),
  STRING_TOKEN(STR_EDIT_HELP_DIV),
0
};

MENU_ITEM_FUNCTION MainControlBasedMenuFunctions[] = {
  NULL,
  NULL,                      /* Ctrl - A */
  NULL,                      /* Ctrl - B */
  NULL,                      /* Ctrl - C */
  NULL,                      /* Ctrl - D */
  MainCommandDisplayHelp,    /* Ctrl - E */
  MainCommandSearch,         /* Ctrl - F */
  MainCommandGotoLine,       /* Ctrl - G */
  NULL,                      /* Ctrl - H */
  NULL,                      /* Ctrl - I */
  NULL,                      /* Ctrl - J */
  MainCommandCutLine,        /* Ctrl - K */
  NULL,                      /* Ctrl - L */
  NULL,                      /* Ctrl - M */
  NULL,                      /* Ctrl - N */
  MainCommandOpenFile,       /* Ctrl - O */
  NULL,                      /* Ctrl - P */
  MainCommandExit,           /* Ctrl - Q */
  MainCommandSearchReplace,  /* Ctrl - R */
  MainCommandSaveFile,       /* Ctrl - S */
  MainCommandSwitchFileType, /* Ctrl - T */
  MainCommandPasteLine,      /* Ctrl - U */
  NULL,                      /* Ctrl - V */
  NULL,                      /* Ctrl - W */
  NULL,                      /* Ctrl - X */
  NULL,                      /* Ctrl - Y */
  NULL,                      /* Ctrl - Z */
};

EDITOR_MENU_ITEM  MainMenuItems[] = {
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_GO_TO_LINE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F1),
    MainCommandGotoLine
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_SAVE_FILE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F2),
    MainCommandSaveFile
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_EXIT),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F3),
    MainCommandExit
  },

  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_SEARCH),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F4),
    MainCommandSearch
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_SEARCH_REPLACE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F5),
    MainCommandSearchReplace
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_CUT_LINE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F6),
    MainCommandCutLine
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_PASTE_LINE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F7),
    MainCommandPasteLine
  },

  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_OPEN_FILE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F8),
    MainCommandOpenFile
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_FILE_TYPE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F9),
    MainCommandSwitchFileType
  },
  {
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_FILE_TYPE),
    STRING_TOKEN(STR_EDIT_LIBMENUBAR_F11),
    MainCommandSwitchFileType
  },

  {
    0,
    0,
    NULL
  }
};


/**
  Load a file from disk to editor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
MainCommandOpenFile (
  VOID
  )
{
  BOOLEAN     Done;
  EFI_STATUS  Status;

  //
  //  This command will open a file from current working directory.
  //   Read-only file can also be opened. But it can not be modified.
  // Below is the scenario of Open File command:
  // 1.IF currently opened file has not been modIFied, directly go to step .
  //   IF currently opened file has been modified,
  //     an Input Bar will be prompted as :
  //       "File Modified. Save ( Yes/No/Cancel) ?"
  //           IF user press 'y' or 'Y', currently opened file will be saved.
  //           IF user press 'n' or 'N', currently opened file will
  //              not be saved.
  //           IF user press 'c' or 'C' or ESC, Open File command ends and
  //              currently opened file is still opened.
  //
  // 2.  An Input Bar will be prompted as :  "File Name to Open: "
  //       IF user press ESC, Open File command ends and
  //          currently opened file is still opened.
  //       Any other inputs with a Return will
  //          cause currently opened file close.
  //
  // 3.  IF user input file name is an existing file , this file will be read
  //        and opened.
  //    IF user input file name is a new file, this file will be created
  //        and opened. This file's type ( UNICODE or ASCII ) is the same
  //        with the old file.
  // if current file is modified, so you need to choose
  // whether to save it first.
  //
  if (MainEditor.FileBuffer->FileModified) {

    Status = InputBarSetPrompt (L"File modified. Save (Yes/No/Cancel) ? ");
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
      Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
      StatusBarSetRefresh();

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
        // want to save this file first
        //
        Status = FileBufferSave (MainEditor.FileBuffer->FileName);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        MainTitleBarRefresh (MainEditor.FileBuffer->FileName, MainEditor.FileBuffer->FileType, MainEditor.FileBuffer->ReadOnly, MainEditor.FileBuffer->FileModified, MainEditor.ScreenSize.Column, MainEditor.ScreenSize.Row, 0, 0);
        FileBufferRestorePosition ();
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
    FileBufferRead (MainEditor.FileBuffer->FileName, TRUE);
    return Status;
  }

  Status = InputBarSetStringSize (100);
  if (EFI_ERROR (Status)) {
    FileBufferRead (MainEditor.FileBuffer->FileName, TRUE);
    return Status;
  }

  while (1) {
    Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
    StatusBarSetRefresh();

    //
    // ESC pressed
    //
    if (Status == EFI_NOT_READY) {
      return EFI_SUCCESS;
    }
    //
    // The input string length should > 0
    //
    if (StrLen (InputBarGetString()) > 0) {
      //
      // CHECK if filename is valid
      //
      if (!IsValidFileName (InputBarGetString())) {
        FileBufferRead (MainEditor.FileBuffer->FileName, TRUE);
        StatusBarSetStatusString (L"Invalid File Name");
        return EFI_SUCCESS;
      }

      break;
    }
  }
  //
  // read from disk
  //
  Status = FileBufferRead (InputBarGetString(), FALSE);

  if (EFI_ERROR (Status)) {
    FileBufferRead (MainEditor.FileBuffer->FileName, TRUE);
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Switch a file from ASCII to UNICODE or vise-versa.

  @retval EFI_SUCCESS           The switch was ok or a warning was presented.
**/
EFI_STATUS
MainCommandSwitchFileType (
  VOID
  )
{
  //
  // Below is the scenario of File Type command:
  // After File Type is executed, file type will be changed to another type
  // if file is read-only, can not be modified
  //
  if (MainEditor.FileBuffer->ReadOnly) {
    StatusBarSetStatusString (L"Read Only File Can Not Be Modified");
    return EFI_SUCCESS;
  }

  if (MainEditor.FileBuffer->FileType == FileTypeUnicode) {
    MainEditor.FileBuffer->FileType = FileTypeAscii;
  } else {
    MainEditor.FileBuffer->FileType = FileTypeUnicode;
  }

  MainEditor.FileBuffer->FileModified = TRUE;

  return EFI_SUCCESS;
}

/**
  cut current line to clipboard

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandCutLine (
  VOID
  )
{
  EFI_STATUS      Status;
  EFI_EDITOR_LINE *Line;

  //
  // This command will cut current line ( where cursor is on ) to clip board.
  //      And cursor will move to the beginning of next line.
  // Below is the scenario of Cut Line command:
  // 1.  IF cursor is on valid line, current line will be cut to clip board.
  //     IF cursor is not on valid line, an Status String will be prompted :
  //        "Nothing to Cut".
  //
  Line = NULL;
  Status = FileBufferCutLine (&Line);
  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  MainEditor.CutLine = Line;

  return EFI_SUCCESS;
}

/**
  paste line to file buffer.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandPasteLine (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Below is the scenario of Paste Line command:
  // 1.  IF nothing is on clipboard, a Status String will be prompted :
  //        "No Line to Paste" and Paste Line command ends.
  //     IF something is on clipboard, insert it above current line.
  // nothing on clipboard
  //
  if (MainEditor.CutLine == NULL) {
    StatusBarSetStatusString (L"No Line to Paste");
    return EFI_SUCCESS;
  }

  Status = FileBufferPasteLine ();

  return Status;
}


/**
  search string in file buffer

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandSearch (
  VOID
  )
{
  EFI_STATUS  Status;
  CHAR16      *Buffer;
  BOOLEAN     Done;
  UINTN       Offset;

  //
  // Below is the scenario of Search command:
  // 1.  An Input Bar will be prompted : "Enter Search String:".
  //       IF user press ESC, Search command ends.
  //       IF user just press Enter, Search command ends.
  //       IF user inputs the search string,  do Step 2.
  //
  // 2.  IF input search string is found, cursor will move to the first
  //        occurrence and do Step 3.
  //     IF input search string is not found, a Status String
  //        "Search String Not Found" will be prompted and Search command ends.
  //
  // 3.  An Input Bar will be prompted: "Find Next (Yes/No/Cancel ) ?".
  //      IF user press ESC, Search command ends.
  //       IF user press 'y' or 'Y', do Step 2.
  //       IF user press 'n' or 'N', Search command ends.
  //       IF user press 'c' or 'C', Search command ends.
  //
  Status = InputBarSetPrompt (L"Enter Search String: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (40);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
  StatusBarSetRefresh();

  //
  // ESC
  //
  if (Status == EFI_NOT_READY) {
    return EFI_SUCCESS;
  }
  //
  // just enter pressed
  //
  if (StrLen (InputBarGetString()) == 0) {
    return EFI_SUCCESS;
  }

  Buffer = CatSPrint (NULL, L"%s", InputBarGetString());
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // the first time , search from current position
  //
  Offset = 0;
  do {
    //
    // since search may be continued to search multiple times
    // so we need to backup editor each time
    //
    MainEditorBackup ();

    Status = FileBufferSearch (Buffer, Offset);

    if (Status == EFI_NOT_FOUND) {
      break;
    }
    //
    // Find next
    //
    Status = InputBarSetPrompt (L"Find Next (Yes/No) ?");
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }

    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }

    Done = FALSE;
    while (!Done) {
      Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
      StatusBarSetRefresh();

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        FreePool (Buffer);
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        Done = TRUE;
        break;

      case L'n':
      case L'N':
        FreePool (Buffer);
        return EFI_SUCCESS;

      }
      //
      // end of which
      //
    }
    //
    // end of while !Done
    // for search second, third time, search from current position + strlen
    //
    Offset = StrLen (Buffer);

  } while (1);
  //
  // end of do
  //
  FreePool (Buffer);
  StatusBarSetStatusString (L"Search String Not Found");

  return EFI_SUCCESS;
}

/**
  Search string in file buffer, and replace it with another str.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandSearchReplace (
  VOID
  )
{
  EFI_STATUS  Status;
  CHAR16      *Search;
  CHAR16      *Replace;
  BOOLEAN     Done;
  BOOLEAN     First;
  BOOLEAN     ReplaceOption;
  UINTN       SearchLen;
  UINTN       ReplaceLen;
  BOOLEAN     ReplaceAll;

  ReplaceOption = FALSE;

  //
  // Below is the scenario of Search/Replace command:
  // 1.  An Input Bar is prompted : "Enter Search String:".
  //       IF user press ESC, Search/Replace command ends.
  //       IF user just press Enter, Search/Replace command ends.
  //       IF user inputs the search string S, do Step 2.
  //
  // 2.  An Input Bar is prompted: "Replace With:".
  //       IF user press ESC, Search/Replace command ends.
  //      IF user inputs the replace string R, do Step 3.
  //
  // 3.  IF input search string is not found, an Status String
  //        "Search String Not Found" will be prompted
  //        and Search/Replace command ends
  //     IF input search string is found, do Step 4.
  //
  // 4.  An Input Bar will be prompted: "Replace ( Yes/No/All/Cancel )?"
  //       IF user press 'y' or 'Y', S will be replaced with R and do Step 5
  //       IF user press 'n' or 'N', S will not be replaced and do Step 5.
  //       IF user press 'a' or 'A', all the S from file current position on
  //          will be replaced with R and Search/Replace command ends.
  //       IF user press 'c' or 'C' or ESC, Search/Replace command ends.
  //
  // 5.  An Input Bar will be prompted: "Find Next (Yes/No/Cancel) ?".
  //       IF user press ESC, Search/Replace command ends.
  //       IF user press 'y' or 'Y', do Step 3.
  //       IF user press 'n' or 'N', Search/Replace command ends.
  //       IF user press 'c' or 'C', Search/Replace command ends.
  // input search string
  //
  Status = InputBarSetPrompt (L"Enter Search String: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (40);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
  StatusBarSetRefresh();

  //
  // ESC
  //
  if (Status == EFI_NOT_READY) {
    return EFI_SUCCESS;
  }
  //
  // if just pressed enter
  //
  if (StrLen (InputBarGetString()) == 0) {
    return EFI_SUCCESS;
  }

  Search = CatSPrint (NULL, L"%s", InputBarGetString());
  if (Search == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SearchLen = StrLen (Search);

  //
  // input replace string
  //
  Status = InputBarSetPrompt (L"Replace With: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarSetStringSize (40);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
  StatusBarSetRefresh();

  //
  // ESC
  //
  if (Status == EFI_NOT_READY) {
    return EFI_SUCCESS;
  }

  Replace = CatSPrint (NULL, L"%s", InputBarGetString());
  if (Replace == NULL) {
    FreePool (Search);
    return EFI_OUT_OF_RESOURCES;
  }

  ReplaceLen  = StrLen (Replace);

  First       = TRUE;
  ReplaceAll  = FALSE;
  do {
    //
    // since search may be continued to search multiple times
    // so we need to backup editor each time
    //
    MainEditorBackup ();

    if (First) {
      Status = FileBufferSearch (Search, 0);
    } else {
      //
      // if just replace, so skip this replace string
      // if replace string is an empty string, so skip to next character
      //
      if (ReplaceOption) {
        Status = FileBufferSearch (Search, (ReplaceLen == 0) ? 1 : ReplaceLen);
      } else {
        Status = FileBufferSearch (Search, SearchLen);
      }
    }

    if (Status == EFI_NOT_FOUND) {
      break;
    }
    //
    // replace or not?
    //
    Status = InputBarSetPrompt (L"Replace (Yes/No/All/Cancel) ?");

    if (EFI_ERROR (Status)) {
      FreePool (Search);
      FreePool (Replace);
      return Status;
    }

    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      FreePool (Search);
      FreePool (Replace);
      return Status;
    }

    Done = FALSE;
    while (!Done) {
      Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
      StatusBarSetRefresh();

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        FreePool (Search);
        FreePool (Replace);
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        Done          = TRUE;
        ReplaceOption = TRUE;
        break;

      case L'n':
      case L'N':
        Done          = TRUE;
        ReplaceOption = FALSE;
        break;

      case L'a':
      case L'A':
        Done          = TRUE;
        ReplaceOption = TRUE;
        ReplaceAll    = TRUE;
        break;

      case L'c':
      case L'C':
        FreePool (Search);
        FreePool (Replace);
        return EFI_SUCCESS;

      }
      //
      // end of which
      //
    }
    //
    // end of while !Done
    // Decide to Replace
    //
    if (ReplaceOption) {
      //
      // file is read-only
      //
      if (MainEditor.FileBuffer->ReadOnly) {
        StatusBarSetStatusString (L"Read Only File Can Not Be Modified");
        return EFI_SUCCESS;
      }
      //
      // replace all
      //
      if (ReplaceAll) {
        Status = FileBufferReplaceAll (Search, Replace, 0);
        FreePool (Search);
        FreePool (Replace);
        return Status;
      }
      //
      // replace
      //
      Status = FileBufferReplace (Replace, SearchLen);
      if (EFI_ERROR (Status)) {
        FreePool (Search);
        FreePool (Replace);
        return Status;
      }
    }
    //
    // Find next
    //
    Status = InputBarSetPrompt (L"Find Next (Yes/No) ?");
    if (EFI_ERROR (Status)) {
      FreePool (Search);
      FreePool (Replace);
      return Status;
    }

    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      FreePool (Search);
      FreePool (Replace);
      return Status;
    }

    Done = FALSE;
    while (!Done) {
      Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
      StatusBarSetRefresh();

      //
      // ESC pressed
      //
      if (Status == EFI_NOT_READY) {
        FreePool (Search);
        FreePool (Replace);
        return EFI_SUCCESS;
      }

      switch (InputBarGetString()[0]) {
      case L'y':
      case L'Y':
        Done = TRUE;
        break;

      case L'n':
      case L'N':
        FreePool (Search);
        FreePool (Replace);
        return EFI_SUCCESS;

      }
      //
      // end of which
      //
    }
    //
    // end of while !Done
    //
    First = FALSE;

  } while (1);
  //
  // end of do
  //
  FreePool (Search);
  FreePool (Replace);

  StatusBarSetStatusString (L"Search String Not Found");

  return EFI_SUCCESS;
}

/**
  exit editor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainCommandExit (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Below is the scenario of Exit command:
  // 1.  IF currently opened file is not modified, exit the editor and
  //        Exit command ends.
  //     IF currently opened file is modified, do Step 2
  //
  // 2.  An Input Bar will be prompted:
  //        "File modified. Save ( Yes/No/Cancel )?"
  //       IF user press 'y' or 'Y', currently opened file will be saved
  //          and Editor exits
  //       IF user press 'n' or 'N', currently opened file will not be saved
  //          and Editor exits.
  //       IF user press 'c' or 'C' or ESC, Exit command ends.
  // if file has been modified, so will prompt user whether to save the changes
  //
  if (MainEditor.FileBuffer->FileModified) {

    Status = InputBarSetPrompt (L"File modified. Save (Yes/No/Cancel) ? ");
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = InputBarSetStringSize (1);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    while (1) {
      Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
      StatusBarSetRefresh();

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
        Status = FileBufferSave (MainEditor.FileBuffer->FileName);
        if (!EFI_ERROR (Status)) {
          EditorExit = TRUE;
        }

        return Status;

      case L'n':
      case L'N':
        EditorExit = TRUE;
        return EFI_SUCCESS;

      case L'c':
      case L'C':
        return EFI_SUCCESS;

      }
    }
  }

  EditorExit = TRUE;
  return EFI_SUCCESS;

}

/**
  move cursor to specified lines

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
MainCommandGotoLine (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Row;

  //
  // Below is the scenario of Go To Line command:
  // 1.  An Input Bar will be prompted : "Go To Line:".
  //       IF user press ESC, Go To Line command ends.
  //       IF user just press Enter, cursor remains unchanged.
  //       IF user inputs line number, do Step 2.
  //
  // 2.  IF input line number is valid, move cursor to the beginning
  //        of specified line and Go To Line command ends.
  //    IF input line number is invalid, a Status String will be prompted:
  //        "No Such Line" and Go To Line command ends.
  //
  Status = InputBarSetPrompt (L"Go To Line: ");
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // line number's digit <= 6
  //
  Status = InputBarSetStringSize (6);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
  StatusBarSetRefresh();

  //
  // press ESC
  //
  if (Status == EFI_NOT_READY) {
    return EFI_SUCCESS;
  }
  //
  // if JUST press enter
  //
  if (StrLen (InputBarGetString()) == 0) {
    return EFI_SUCCESS;
  }

  Row = ShellStrToUintn (InputBarGetString());

  //
  // invalid line number
  //
  if (Row > MainEditor.FileBuffer->NumLines || Row <= 0) {
    StatusBarSetStatusString (L"No Such Line");
    return EFI_SUCCESS;
  }
  //
  // move cursor to that line's start
  //
  FileBufferMovePosition (Row, 1);

  return EFI_SUCCESS;
}

/**
  Save current file to disk, you can save to current file name or
  save to another file name.

  @retval EFI_SUCCESS           The file was saved correctly.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR          A file access error occured.
**/
EFI_STATUS
MainCommandSaveFile (
  VOID
  )
{
  EFI_STATUS        Status;
  CHAR16            *FileName;
  BOOLEAN           OldFile;
  CHAR16            *Str;
  SHELL_FILE_HANDLE FileHandle;
  EFI_FILE_INFO     *Info;

  //
  // This command will save currently opened file to disk.
  // You can choose save to another file name or just save to
  //    current file name.
  // Below is the scenario of Save File command:
  //    ( Suppose the old file name is A )
  // 1.  An Input Bar will be prompted:    "File To Save: [ old file name]"
  //     IF user press ESC, Save File command ends .
  //     IF user press Enter, input file name will be A.
  //     IF user inputs a new file name B,  input file name will be B.
  //
  // 2.  IF input file name is A, go to do Step 3.
  //     IF input file name is B, go to do Step 4.
  //
  // 3.  IF A is read only, Status Bar will show "Access Denied" and
  //       Save File commands ends.
  //     IF A is not read only, save file buffer to disk and remove modified
  //       flag in Title Bar , then Save File command ends.
  //
  // 4.  IF B does not exist, create this file and save file buffer to it.
  //       Go to do Step 7.
  //     IF B exits, do Step 5.
  //
  // 5.An Input Bar will be prompted:
  //      "File Exists. Overwrite ( Yes/No/Cancel )?"
  //       IF user press 'y' or 'Y', do Step 6.
  //       IF user press 'n' or 'N', Save File commands ends.
  //       IF user press 'c' or 'C' or ESC, Save File commands ends.
  //
  // 6. IF B is a read-only file, Status Bar will show "Access Denied" and
  //       Save File commands ends.
  //    IF B can be read and write, save file buffer to B.
  //
  // 7.  Update File Name field in Title Bar to B and remove the modified
  //       flag in Title Bar.
  //
  Str = CatSPrint (NULL, L"File to Save: [%s]", MainEditor.FileBuffer->FileName);
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
    Str[50] = CHAR_NULL;
  }

  Status = InputBarSetPrompt (Str);
  FreePool(Str);

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
  Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
  StatusBarSetRefresh();

  //
  // if user pressed ESC
  //
  if (Status == EFI_NOT_READY) {
    return EFI_SUCCESS;
  }

  //
  // if just enter pressed, so think save to current file name
  //
  if (StrLen (InputBarGetString()) == 0) {
    FileName = CatSPrint (NULL, L"%s", MainEditor.FileBuffer->FileName);
  } else {
    FileName = CatSPrint (NULL, L"%s", InputBarGetString());
  }

  if (FileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (!IsValidFileName (FileName)) {
    StatusBarSetStatusString (L"Invalid File Name");
    FreePool (FileName);
    return EFI_SUCCESS;
  }

  OldFile = FALSE;

  //
  // save to the old file
  //
  if (StringNoCaseCompare (&FileName, &MainEditor.FileBuffer->FileName) == 0) {
    OldFile = TRUE;
  }

  if (OldFile) {
    //
    // if the file is read only, so can not write back to it.
    //
    if (MainEditor.FileBuffer->ReadOnly == TRUE) {
      StatusBarSetStatusString (L"Access Denied");
      FreePool (FileName);
      return EFI_SUCCESS;
    }
  } else {
    //
    // if the file exists
    //
    if (ShellFileExists(FileName) != EFI_NOT_FOUND) {
      //
      // check for read only
      //
      Status = ShellOpenFileByName(FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR(Status)) {
        StatusBarSetStatusString (L"Open Failed");
        FreePool (FileName);
        return EFI_SUCCESS;
      }

      Info = ShellGetFileInfo(FileHandle);
      if (Info == NULL) {
        StatusBarSetStatusString (L"Access Denied");
        FreePool (FileName);
        return (EFI_SUCCESS);
      }

      if (Info->Attribute & EFI_FILE_READ_ONLY) {
        StatusBarSetStatusString (L"Access Denied - Read Only");
        FreePool (Info);
        FreePool (FileName);
        return (EFI_SUCCESS);
      }
      FreePool (Info);

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

      while (TRUE) {
        Status = InputBarRefresh (MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column);
        StatusBarSetRefresh();

        //
        // ESC pressed
        //
        if (Status == EFI_NOT_READY) {
          SHELL_FREE_NON_NULL (FileName);
          return EFI_SUCCESS;
        }

        switch (InputBarGetString()[0]) {
        case L'y':
        case L'Y':
          break;

        case L'n':
        case L'N':
        case L'c':
        case L'C':
          SHELL_FREE_NON_NULL (FileName);
          return EFI_SUCCESS;
        } // end switch
      } // while (!done)
    } // file does exist
  } // if old file name same

  //
  // save file to disk with specified name
  //
  FileBufferSetModified();
  Status = FileBufferSave (FileName);
  SHELL_FREE_NON_NULL (FileName);

  return Status;
}

/**
  Show help information for the editor.

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
MainCommandDisplayHelp (
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
  for (CurrentLine = 0; 0 != MainMenuHelpInfo[CurrentLine]; CurrentLine++) {
    InfoString = HiiGetString(gShellDebug1HiiHandle, MainMenuHelpInfo[CurrentLine], NULL);
    ShellPrintEx (0, CurrentLine+1, L"%E%s%N", InfoString);
  }

  //
  // scan for ctrl+w
  //
  while (TRUE) {
    Status = gBS->WaitForEvent (1, &MainEditor.TextInputEx->WaitForKeyEx, &EventIndex);
    if (EFI_ERROR (Status) || (EventIndex != 0)) {
      continue;
    }
    Status = MainEditor.TextInputEx->ReadKeyStrokeEx (MainEditor.TextInputEx, &KeyData);
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
  //
  // update screen with file buffer's info
  //
  FileBufferRestorePosition ();
  FileBufferNeedRefresh = TRUE;
  FileBufferOnlyLineNeedRefresh = FALSE;
  FileBufferRefresh ();

  return EFI_SUCCESS;
}

EFI_EDITOR_COLOR_ATTRIBUTES   OriginalColors;
INTN                          OriginalMode;


//
// basic initialization for MainEditor
//
EFI_EDITOR_GLOBAL_EDITOR      MainEditorConst = {
  &FileBuffer,
  {
    {0, 0}
  },
  {
    0,
    0
  },
  NULL,
  NULL,
  FALSE,
  NULL
};

/**
  The initialization function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainEditorInit (
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
  CopyMem (&MainEditor, &MainEditorConst, sizeof (MainEditor));

  //
  // set screen attributes
  //
  MainEditor.ColorAttributes.Colors.Foreground  = gST->ConOut->Mode->Attribute & 0x000000ff;

  MainEditor.ColorAttributes.Colors.Background  = (UINT8) (gST->ConOut->Mode->Attribute >> 4);
  OriginalColors = MainEditor.ColorAttributes.Colors;

  OriginalMode = gST->ConOut->Mode->Mode;

  //
  // query screen size
  //
  gST->ConOut->QueryMode (
        gST->ConOut,
        gST->ConOut->Mode->Mode,
        &(MainEditor.ScreenSize.Column),
        &(MainEditor.ScreenSize.Row)
        );

  //
  // Find TextInEx in System Table ConsoleInHandle
  // Per UEFI Spec, TextInEx is required for a console capable platform.
  //
  Status = gBS->HandleProtocol (
              gST->ConsoleInHandle,
              &gEfiSimpleTextInputExProtocolGuid,
              (VOID**)&MainEditor.TextInputEx
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
                (VOID**)&MainEditor.MouseInterface
                );
  if (EFI_ERROR (Status)) {
    //
    // If there is no Simple Pointer Protocol on System Table
    //
    HandleBuffer = NULL;
    MainEditor.MouseInterface = NULL;
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
                      (VOID**)&MainEditor.MouseInterface
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

  if (!EFI_ERROR (Status) && MainEditor.MouseInterface != NULL) {
    MainEditor.MouseAccumulatorX  = 0;
    MainEditor.MouseAccumulatorY  = 0;
    MainEditor.MouseSupported     = TRUE;
  }

  //
  // below will call the five components' init function
  //
  Status = MainTitleBarInit (L"UEFI EDIT");
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_LIBEDITOR_TITLEBAR), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }

  Status = ControlHotKeyInit (MainControlBasedMenuFunctions);
  Status = MenuBarInit (MainMenuItems);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_LIBEDITOR_MAINMENU), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }

  Status = StatusBarInit ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_LIBEDITOR_STATUSBAR), gShellDebug1HiiHandle);
    return EFI_LOAD_ERROR;
  }

  InputBarInit (MainEditor.TextInputEx);

  Status = FileBufferInit ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_LIBEDITOR_FILEBUFFER), gShellDebug1HiiHandle);
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
  EditorFirst       = TRUE;
  EditorExit        = FALSE;
  EditorMouseAction = FALSE;

  return EFI_SUCCESS;
}

/**
  The cleanup function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
MainEditorCleanup (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // call the five components' cleanup function
  // if error, do not exit
  // just print some warning
  //
  MainTitleBarCleanup();
  StatusBarCleanup();
  InputBarCleanup();
  MenuBarCleanup ();

  Status = FileBufferCleanup ();
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_LIBEDITOR_FILEBUFFER_CLEANUP), gShellDebug1HiiHandle);
  }
  //
  // restore old mode
  //
  if (OriginalMode != gST->ConOut->Mode->Mode) {
    gST->ConOut->SetMode (gST->ConOut, OriginalMode);
  }
  //
  // restore old screen color
  //
  gST->ConOut->SetAttribute (
        gST->ConOut,
        EFI_TEXT_ATTR (OriginalColors.Foreground, OriginalColors.Background)
        );

  gST->ConOut->ClearScreen (gST->ConOut);

  return EFI_SUCCESS;
}

/**
  Refresh the main editor component.
**/
VOID
MainEditorRefresh (
  VOID
  )
{
  //
  // The Stall value is from experience. NOT from spec.  avoids 'flicker'
  //
  gBS->Stall (50);

  //
  // call the components refresh function
  //
  if (EditorFirst
    || StrCmp (FileBufferBackupVar.FileName, FileBuffer.FileName) != 0
    || FileBufferBackupVar.FileType != FileBuffer.FileType
    || FileBufferBackupVar.FileModified != FileBuffer.FileModified
    || FileBufferBackupVar.ReadOnly != FileBuffer.ReadOnly) {

    MainTitleBarRefresh (MainEditor.FileBuffer->FileName, MainEditor.FileBuffer->FileType, MainEditor.FileBuffer->ReadOnly, MainEditor.FileBuffer->FileModified, MainEditor.ScreenSize.Column, MainEditor.ScreenSize.Row, 0, 0);
    FileBufferRestorePosition ();
  }

  if (EditorFirst
    || FileBufferBackupVar.FilePosition.Row != FileBuffer.FilePosition.Row
    || FileBufferBackupVar.FilePosition.Column != FileBuffer.FilePosition.Column
    || FileBufferBackupVar.ModeInsert != FileBuffer.ModeInsert
    || StatusBarGetRefresh()) {

    StatusBarRefresh (EditorFirst, MainEditor.ScreenSize.Row, MainEditor.ScreenSize.Column, MainEditor.FileBuffer->FilePosition.Row, MainEditor.FileBuffer->FilePosition.Column, MainEditor.FileBuffer->ModeInsert);
    FileBufferRestorePosition ();
  }

  if (EditorFirst) {
    FileBufferRestorePosition ();
  }

  FileBufferRefresh ();

  //
  // EditorFirst is now set to FALSE
  //
  EditorFirst = FALSE;
}

/**
  Get's the resultant location of the cursor based on the relative movement of the Mouse.

  @param[in] GuidX    The relative mouse movement.

  @return The X location of the mouse.
**/
INT32
GetTextX (
  IN INT32 GuidX
  )
{
  INT32 Gap;

  MainEditor.MouseAccumulatorX += GuidX;
  Gap = (MainEditor.MouseAccumulatorX * (INT32) MainEditor.ScreenSize.Column) / (INT32) (50 * (INT32) MainEditor.MouseInterface->Mode->ResolutionX);
  MainEditor.MouseAccumulatorX = (MainEditor.MouseAccumulatorX * (INT32) MainEditor.ScreenSize.Column) % (INT32) (50 * (INT32) MainEditor.MouseInterface->Mode->ResolutionX);
  MainEditor.MouseAccumulatorX = MainEditor.MouseAccumulatorX / (INT32) MainEditor.ScreenSize.Column;
  return Gap;
}

/**
  Get's the resultant location of the cursor based on the relative movement of the Mouse.

  @param[in] GuidY    The relative mouse movement.

  @return The Y location of the mouse.
**/
INT32
GetTextY (
  IN INT32 GuidY
  )
{
  INT32 Gap;

  MainEditor.MouseAccumulatorY += GuidY;
  Gap = (MainEditor.MouseAccumulatorY * (INT32) MainEditor.ScreenSize.Row) / (INT32) (50 * (INT32) MainEditor.MouseInterface->Mode->ResolutionY);
  MainEditor.MouseAccumulatorY = (MainEditor.MouseAccumulatorY * (INT32) MainEditor.ScreenSize.Row) % (INT32) (50 * (INT32) MainEditor.MouseInterface->Mode->ResolutionY);
  MainEditor.MouseAccumulatorY = MainEditor.MouseAccumulatorY / (INT32) MainEditor.ScreenSize.Row;

  return Gap;
}

/**
  Support mouse movement.  Move the cursor.

  @param[in] MouseState     The current mouse state.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_NOT_FOUND     There was no mouse support found.
**/
EFI_STATUS
MainEditorHandleMouseInput (
  IN EFI_SIMPLE_POINTER_STATE       MouseState
  )
{

  INT32           TextX;
  INT32           TextY;
  UINTN           FRow;
  UINTN           FCol;

  LIST_ENTRY  *Link;
  EFI_EDITOR_LINE *Line;

  UINTN           Index;
  BOOLEAN         Action;

  //
  // mouse action means:
  //    mouse movement
  //    mouse left button
  //
  Action = FALSE;

  //
  // have mouse movement
  //
  if (MouseState.RelativeMovementX || MouseState.RelativeMovementY) {
    //
    // handle
    //
    TextX = GetTextX (MouseState.RelativeMovementX);
    TextY = GetTextY (MouseState.RelativeMovementY);

    FileBufferAdjustMousePosition (TextX, TextY);

    Action = TRUE;

  }

  //
  // if left button pushed down
  //
  if (MouseState.LeftButton) {

    FCol = MainEditor.FileBuffer->MousePosition.Column - 1 + 1;

    FRow = MainEditor.FileBuffer->FilePosition.Row +
      MainEditor.FileBuffer->MousePosition.Row -
      MainEditor.FileBuffer->DisplayPosition.Row;

    //
    // beyond the file line length
    //
    if (MainEditor.FileBuffer->NumLines < FRow) {
      FRow = MainEditor.FileBuffer->NumLines;
    }

    Link = MainEditor.FileBuffer->ListHead->ForwardLink;
    for (Index = 0; Index < FRow - 1; Index++) {
      Link = Link->ForwardLink;
    }

    Line = CR (Link, EFI_EDITOR_LINE, Link, LINE_LIST_SIGNATURE);

    //
    // beyond the line's column length
    //
    if (FCol > Line->Size + 1) {
      FCol = Line->Size + 1;
    }

    FileBufferMovePosition (FRow, FCol);

    MainEditor.FileBuffer->MousePosition.Row    = MainEditor.FileBuffer->DisplayPosition.Row;

    MainEditor.FileBuffer->MousePosition.Column = MainEditor.FileBuffer->DisplayPosition.Column;

    Action = TRUE;
  }
  //
  // mouse has action
  //
  if (Action) {
    return EFI_SUCCESS;
  }

  //
  // no mouse action
  //
  return EFI_NOT_FOUND;
}

/**
  Handle user key input. This routes to other functions for the actions.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
MainEditorKeyInput (
  VOID
  )
{
  EFI_KEY_DATA              KeyData;
  EFI_STATUS                Status;
  EFI_SIMPLE_POINTER_STATE  MouseState;
  BOOLEAN                   NoShiftState;

  do {

    Status            = EFI_SUCCESS;
    EditorMouseAction = FALSE;

    //
    // backup some key elements, so that can aVOID some refresh work
    //
    MainEditorBackup ();

    //
    // change priority of checking mouse/keyboard activity dynamically
    // so prevent starvation of keyboard.
    // if last time, mouse moves then this time check keyboard
    //
    if (MainEditor.MouseSupported) {
      Status = MainEditor.MouseInterface->GetState (
                                            MainEditor.MouseInterface,
                                            &MouseState
                                            );
      if (!EFI_ERROR (Status)) {

        Status = MainEditorHandleMouseInput (MouseState);

        if (!EFI_ERROR (Status)) {
          EditorMouseAction           = TRUE;
          FileBufferMouseNeedRefresh  = TRUE;
        } else if (Status == EFI_LOAD_ERROR) {
          StatusBarSetStatusString (L"Invalid Mouse Movement ");
        }
      }
    }

    //
    // CheckEvent() returns Success when non-partial key is pressed.
    //
    Status = gBS->CheckEvent (MainEditor.TextInputEx->WaitForKeyEx);
    if (!EFI_ERROR (Status)) {
      Status = MainEditor.TextInputEx->ReadKeyStrokeEx (MainEditor.TextInputEx, &KeyData);
      if (!EFI_ERROR (Status)) {
        //
        // dispatch to different components' key handling function
        // so not everywhere has to set this variable
        //
        FileBufferMouseNeedRefresh = TRUE;
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
        if (EFI_NOT_FOUND != MenuBarDispatchControlHotKey(&KeyData)) {
          Status = EFI_SUCCESS;
        } else if (NoShiftState && ((KeyData.Key.ScanCode == SCAN_NULL) || ((KeyData.Key.ScanCode >= SCAN_UP) && (KeyData.Key.ScanCode <= SCAN_PAGE_DOWN)))) {
          Status = FileBufferHandleInput (&KeyData.Key);
        } else if (NoShiftState && (KeyData.Key.ScanCode >= SCAN_F1) && (KeyData.Key.ScanCode <= SCAN_F12)) {
          Status = MenuBarDispatchFunctionKey (&KeyData.Key);
        } else {
          StatusBarSetStatusString (L"Unknown Command");
          FileBufferMouseNeedRefresh = FALSE;
        }

        if (Status != EFI_SUCCESS && Status != EFI_OUT_OF_RESOURCES) {
          //
          // not already has some error status
          //
          if (StatusBarGetString() != NULL && StrCmp (L"", StatusBarGetString()) == 0) {
            StatusBarSetStatusString (L"Disk Error. Try Again");
          }
        }

      }
    }
    //
    // after handling, refresh editor
    //
    MainEditorRefresh ();

  } while (Status != EFI_OUT_OF_RESOURCES && !EditorExit);

  return Status;
}

/**
  Set clipboard

  @param[in] Line   A pointer to the line to be set to clipboard

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES A memory allocation failed.
**/
EFI_STATUS
MainEditorSetCutLine (
  EFI_EDITOR_LINE *Line
  )
{
  if (Line == NULL) {
    return EFI_SUCCESS;
  }

  if (MainEditor.CutLine != NULL) {
    //
    // free the old clipboard
    //
    LineFree (MainEditor.CutLine);
  }
  //
  // duplicate the line to clipboard
  //
  MainEditor.CutLine = LineDup (Line);
  if (MainEditor.CutLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Backup function for MainEditor

  @retval EFI_SUCCESS The operation was successful.
**/
EFI_STATUS
MainEditorBackup (
  VOID
  )
{
  FileBufferBackup ();

  return EFI_SUCCESS;
}
