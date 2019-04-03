/** @file
  Functions to deal with file buffer.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HexEditor.h"

extern EFI_HANDLE                 HImageHandleBackup;
extern HEFI_EDITOR_BUFFER_IMAGE   HBufferImage;

extern BOOLEAN                    HBufferImageNeedRefresh;
extern BOOLEAN                    HBufferImageOnlyLineNeedRefresh;
extern BOOLEAN                    HBufferImageMouseNeedRefresh;

extern HEFI_EDITOR_GLOBAL_EDITOR  HMainEditor;

HEFI_EDITOR_FILE_IMAGE            HFileImage;
HEFI_EDITOR_FILE_IMAGE            HFileImageBackupVar;

//
// for basic initialization of HFileImage
//
HEFI_EDITOR_BUFFER_IMAGE          HFileImageConst = {
  NULL,
  0,
  FALSE
};

/**
  Initialization function for HFileImage

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
HFileImageInit (
  VOID
  )
{
  //
  // basically initialize the HFileImage
  //
  CopyMem (&HFileImage, &HFileImageConst, sizeof (HFileImage));

  CopyMem (
    &HFileImageBackupVar,
    &HFileImageConst,
    sizeof (HFileImageBackupVar)
    );

  return EFI_SUCCESS;
}

/**
  Backup function for HFileImage. Only a few fields need to be backup.
  This is for making the file buffer refresh as few as possible.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HFileImageBackup (
  VOID
  )
{
  SHELL_FREE_NON_NULL (HFileImageBackupVar.FileName);
  HFileImageBackupVar.FileName = CatSPrint(NULL, L"%s", HFileImage.FileName);
  if (HFileImageBackupVar.FileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Cleanup function for HFileImage.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
HFileImageCleanup (
  VOID
  )
{

  SHELL_FREE_NON_NULL (HFileImage.FileName);
  SHELL_FREE_NON_NULL (HFileImageBackupVar.FileName);

  return EFI_SUCCESS;
}

/**
  Set FileName field in HFileImage

  @param[in] Str  File name to set.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HFileImageSetFileName (
  IN CONST CHAR16 *Str
  )
{
  if (Str == HFileImage.FileName) {
    //
    // This function might be called using HFileImage.FileName as Str.
    // Directly return without updating HFileImage.FileName.
    //
    return EFI_SUCCESS;
  }
  //
  // free the old file name
  //
  SHELL_FREE_NON_NULL (HFileImage.FileName);
  HFileImage.FileName = AllocateCopyPool (StrSize (Str), Str);
  if (HFileImage.FileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Read a file from disk into HBufferImage.

  @param[in] FileName     filename to read.
  @param[in] Recover      if is for recover, no information print.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occured.
**/
EFI_STATUS
HFileImageRead (
  IN CONST CHAR16  *FileName,
  IN BOOLEAN Recover
  )
{
  HEFI_EDITOR_LINE                *Line;
  UINT8                           *Buffer;
  CHAR16                          *UnicodeBuffer;
  EFI_STATUS                      Status;

  //
  // variable initialization
  //
  Line                    = NULL;

  //
  // in this function, when you return error ( except EFI_OUT_OF_RESOURCES )
  // you should set status string
  // since this function maybe called before the editorhandleinput loop
  // so any error will cause editor return
  // so if you want to print the error status
  // you should set the status string
  //
  Status = ReadFileIntoBuffer (FileName, (VOID**)&Buffer, &HFileImage.Size, &HFileImage.ReadOnly);
  //
  // NULL pointer is only also a failure for a non-zero file size.
  //
  if ((EFI_ERROR(Status)) || (Buffer == NULL && HFileImage.Size != 0)) {
    UnicodeBuffer = CatSPrint(NULL, L"Read error on file %s: %r", FileName, Status);
    if (UnicodeBuffer == NULL) {
      SHELL_FREE_NON_NULL(Buffer);
      return EFI_OUT_OF_RESOURCES;
    }

    StatusBarSetStatusString (UnicodeBuffer);
    FreePool (UnicodeBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  HFileImageSetFileName (FileName);

  //
  // free the old lines
  //
  HBufferImageFree ();

  Status = HBufferImageBufferToList (Buffer, HFileImage.Size);
  SHELL_FREE_NON_NULL (Buffer);
  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Error parsing file.");
    return Status;
  }

  HBufferImage.DisplayPosition.Row    = 2;
  HBufferImage.DisplayPosition.Column = 10;
  HBufferImage.MousePosition.Row      = 2;
  HBufferImage.MousePosition.Column   = 10;
  HBufferImage.LowVisibleRow          = 1;
  HBufferImage.HighBits               = TRUE;
  HBufferImage.BufferPosition.Row     = 1;
  HBufferImage.BufferPosition.Column  = 1;
  HBufferImage.BufferType = FileTypeFileBuffer;

  if (!Recover) {
    UnicodeBuffer = CatSPrint(NULL, L"%d Lines Read", HBufferImage.NumLines);
    if (UnicodeBuffer == NULL) {
      SHELL_FREE_NON_NULL(Buffer);
      return EFI_OUT_OF_RESOURCES;
    }

    StatusBarSetStatusString (UnicodeBuffer);
    FreePool (UnicodeBuffer);

    HMainEditor.SelectStart = 0;
    HMainEditor.SelectEnd   = 0;
  }

  //
  // has line
  //
  if (HBufferImage.Lines != 0) {
    HBufferImage.CurrentLine = CR (HBufferImage.ListHead->ForwardLink, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
  } else {
    //
    // create a dummy line
    //
    Line = HBufferImageCreateLine ();
    if (Line == NULL) {
      SHELL_FREE_NON_NULL(Buffer);
      return EFI_OUT_OF_RESOURCES;
    }

    HBufferImage.CurrentLine = Line;
  }

  HBufferImage.Modified           = FALSE;
  HBufferImageNeedRefresh         = TRUE;
  HBufferImageOnlyLineNeedRefresh = FALSE;
  HBufferImageMouseNeedRefresh    = TRUE;

  return EFI_SUCCESS;
}

/**
  Save lines in HBufferImage to disk.

  @param[in] FileName     The file name.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occured.
**/
EFI_STATUS
HFileImageSave (
  IN CHAR16 *FileName
  )
{

  LIST_ENTRY                      *Link;
  HEFI_EDITOR_LINE                *Line;
  CHAR16                          *Str;
  EFI_STATUS                      Status;
  UINTN                           NumLines;
  SHELL_FILE_HANDLE                 FileHandle;
  UINTN                           TotalSize;
  UINT8                           *Buffer;
  UINT8                           *Ptr;
  EDIT_FILE_TYPE                  BufferTypeBackup;

  BufferTypeBackup        = HBufferImage.BufferType;
  HBufferImage.BufferType = FileTypeFileBuffer;

  //
  // if is the old file
  //
  if (HFileImage.FileName != NULL && FileName != NULL && StrCmp (FileName, HFileImage.FileName) == 0) {
    //
    // check whether file exists on disk
    //
    if (ShellIsFile(FileName) == EFI_SUCCESS) {
      //
      // current file exists on disk
      // so if not modified, then not save
      //
      if (HBufferImage.Modified == FALSE) {
        return EFI_SUCCESS;
      }
      //
      // if file is read-only, set error
      //
      if (HFileImage.ReadOnly == TRUE) {
        StatusBarSetStatusString (L"Read Only File Can Not Be Saved");
        return EFI_SUCCESS;
      }
    }
  }

   if (ShellIsDirectory(FileName) == EFI_SUCCESS) {
    StatusBarSetStatusString (L"Directory Can Not Be Saved");
    return EFI_LOAD_ERROR;
  }

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);

  if (!EFI_ERROR (Status)) {
    //
    // the file exits, delete it
    //
    Status = ShellDeleteFile (&FileHandle);
    if (EFI_ERROR (Status) || Status == EFI_WARN_DELETE_FAILURE) {
      StatusBarSetStatusString (L"Write File Failed");
      return EFI_LOAD_ERROR;
    }
 }

  //
  // write all the lines back to disk
  //
  NumLines  = 0;
  TotalSize = 0;
  for (Link = HBufferImage.ListHead->ForwardLink; Link != HBufferImage.ListHead; Link = Link->ForwardLink) {
    Line = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);

    if (Line->Size != 0) {
      TotalSize += Line->Size;
    }
    //
    // end of if Line -> Size != 0
    //
    NumLines++;
  }
  //
  // end of for Link
  //
  Buffer = AllocateZeroPool (TotalSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = Buffer;
  for (Link = HBufferImage.ListHead->ForwardLink; Link != HBufferImage.ListHead; Link = Link->ForwardLink) {
    Line = CR (Link, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);

    if (Line->Size != 0) {
      CopyMem (Ptr, Line->Buffer, Line->Size);
      Ptr += Line->Size;
    }
    //
    // end of if Line -> Size != 0
    //
  }


  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);

  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Create File Failed");
    return EFI_LOAD_ERROR;
  }

  Status = ShellWriteFile (FileHandle, &TotalSize, Buffer);
  FreePool (Buffer);
  if (EFI_ERROR (Status)) {
    ShellDeleteFile (&FileHandle);
    return EFI_LOAD_ERROR;
  }

  ShellCloseFile(&FileHandle);

  HBufferImage.Modified = FALSE;

  //
  // set status string
  //
  Str = CatSPrint(NULL, L"%d Lines Wrote", NumLines);
  StatusBarSetStatusString (Str);
  FreePool (Str);

  //
  // now everything is ready , you can set the new file name to filebuffer
  //
  if ((BufferTypeBackup != FileTypeFileBuffer && FileName != NULL) ||
     (FileName != NULL && HFileImage.FileName != NULL && StringNoCaseCompare (&FileName, &HFileImage.FileName) != 0)){
    //
    // not the same
    //
    HFileImageSetFileName (FileName);
    if (HFileImage.FileName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  HFileImage.ReadOnly = FALSE;

  return EFI_SUCCESS;
}
