/** @file
  Functions to deal with Mem buffer

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HexEditor.h"

extern EFI_HANDLE  HImageHandleBackup;

extern HEFI_EDITOR_BUFFER_IMAGE  HBufferImage;

extern BOOLEAN  HBufferImageNeedRefresh;
extern BOOLEAN  HBufferImageOnlyLineNeedRefresh;
extern BOOLEAN  HBufferImageMouseNeedRefresh;

extern HEFI_EDITOR_GLOBAL_EDITOR  HMainEditor;

HEFI_EDITOR_MEM_IMAGE  HMemImage;
HEFI_EDITOR_MEM_IMAGE  HMemImageBackupVar;

//
// for basic initialization of HDiskImage
//
HEFI_EDITOR_MEM_IMAGE  HMemImageConst = {
  NULL,
  0,
  0
};

/**
  Initialization function for HDiskImage.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_LOAD_ERROR    A load error occurred.
**/
EFI_STATUS
HMemImageInit (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // basically initialize the HMemImage
  //
  CopyMem (&HMemImage, &HMemImageConst, sizeof (HMemImage));

  Status = gBS->LocateProtocol (
                  &gEfiCpuIo2ProtocolGuid,
                  NULL,
                  (VOID **)&HMemImage.IoFncs
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  } else {
    return EFI_LOAD_ERROR;
  }
}

/**
  Backup function for HDiskImage. Only a few fields need to be backup.
  This is for making the Disk buffer refresh as few as possible.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
HMemImageBackup (
  VOID
  )
{
  HMemImageBackupVar.Offset = HMemImage.Offset;
  HMemImageBackupVar.Size   = HMemImage.Size;

  return EFI_SUCCESS;
}

/**
  Set FileName field in HFileImage.

  @param[in] Offset   The offset.
  @param[in] Size     The size.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageSetMemOffsetSize (
  IN UINTN  Offset,
  IN UINTN  Size
  )
{
  HMemImage.Offset = Offset;
  HMemImage.Size   = Size;

  return EFI_SUCCESS;
}

/**
  Read a disk from disk into HBufferImage.

  @param[in] Offset   The offset.
  @param[in] Size     The size.
  @param[in] Recover  if is for recover, no information print.

  @retval EFI_LOAD_ERROR        A load error occurred.
  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageRead (
  IN UINTN    Offset,
  IN UINTN    Size,
  IN BOOLEAN  Recover
  )
{
  EFI_STATUS        Status;
  void              *Buffer;
  CHAR16            *Str;
  HEFI_EDITOR_LINE  *Line;

  HBufferImage.BufferType = FileTypeMemBuffer;

  Buffer = AllocateZeroPool (Size);
  if (Buffer == NULL) {
    StatusBarSetStatusString (L"Read Memory Failed");
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HMemImage.IoFncs->Mem.Read (
                                   HMemImage.IoFncs,
                                   EfiCpuIoWidthUint8,
                                   Offset,
                                   Size,
                                   Buffer
                                   );

  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    StatusBarSetStatusString (L"Memory Specified Not Accessible");
    return EFI_LOAD_ERROR;
  }

  HBufferImageFree ();

  Status = HBufferImageBufferToList (Buffer, Size);
  FreePool (Buffer);

  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Read Memory Failed");
    return Status;
  }

  Status = HMemImageSetMemOffsetSize (Offset, Size);

  HBufferImage.DisplayPosition.Row    = 2;
  HBufferImage.DisplayPosition.Column = 10;

  HBufferImage.MousePosition.Row    = 2;
  HBufferImage.MousePosition.Column = 10;

  HBufferImage.LowVisibleRow = 1;
  HBufferImage.HighBits      = TRUE;

  HBufferImage.BufferPosition.Row    = 1;
  HBufferImage.BufferPosition.Column = 1;

  if (!Recover) {
    Str = CatSPrint (NULL, L"%d Lines Read", HBufferImage.NumLines);
    if (Str == NULL) {
      StatusBarSetStatusString (L"Read Memory Failed");
      return EFI_OUT_OF_RESOURCES;
    }

    StatusBarSetStatusString (Str);
    SHELL_FREE_NON_NULL (Str);

    HMainEditor.SelectStart = 0;
    HMainEditor.SelectEnd   = 0;
  }

  //
  // has line
  //
  if (HBufferImage.Lines != NULL) {
    HBufferImage.CurrentLine = CR (HBufferImage.ListHead->ForwardLink, HEFI_EDITOR_LINE, Link, EFI_EDITOR_LINE_LIST);
  } else {
    //
    // create a dummy line
    //
    Line = HBufferImageCreateLine ();
    if (Line == NULL) {
      StatusBarSetStatusString (L"Read Memory Failed");
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

  @param[in] Offset   The offset.
  @param[in] Size     The size.

  @retval EFI_LOAD_ERROR        A load error occurred.
  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageSave (
  IN UINTN  Offset,
  IN UINTN  Size
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer;

  //
  // not modified, so directly return
  //
  if (HBufferImage.Modified == FALSE) {
    return EFI_SUCCESS;
  }

  HBufferImage.BufferType = FileTypeMemBuffer;

  Buffer = AllocateZeroPool (Size);

  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HBufferImageListToBuffer (Buffer, Size);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  //
  // write back to memory
  //
  Status = HMemImage.IoFncs->Mem.Write (
                                   HMemImage.IoFncs,
                                   EfiCpuIoWidthUint8,
                                   Offset,
                                   Size,
                                   Buffer
                                   );

  FreePool (Buffer);

  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }

  //
  // now not modified
  //
  HBufferImage.Modified = FALSE;

  return EFI_SUCCESS;
}
