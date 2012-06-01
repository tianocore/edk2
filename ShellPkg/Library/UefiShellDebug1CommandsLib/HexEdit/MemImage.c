/** @file
  Functions to deal with Mem buffer
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HexEditor.h"

extern EFI_HANDLE                 HImageHandleBackup;

extern HEFI_EDITOR_BUFFER_IMAGE   HBufferImage;

extern BOOLEAN                    HBufferImageNeedRefresh;
extern BOOLEAN                    HBufferImageOnlyLineNeedRefresh;
extern BOOLEAN                    HBufferImageMouseNeedRefresh;

extern HEFI_EDITOR_GLOBAL_EDITOR  HMainEditor;

HEFI_EDITOR_MEM_IMAGE             HMemImage;
HEFI_EDITOR_MEM_IMAGE             HMemImageBackupVar;

EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   DummyPciRootBridgeIo;

//
// for basic initialization of HDiskImage
//
HEFI_EDITOR_MEM_IMAGE             HMemImageConst = {
  NULL,
  0,
  0
};

/**
  Empty function.  always returns the same.

  @param[in] This        Ignored.
  @param[in] Width       Ignored.
  @param[in] Address     Ignored.
  @param[in] Count       Ignored.
  @param[in, out] Buffer Ignored.

  @retval EFI_UNSUPPORTED.
**/
EFI_STATUS
EFIAPI
DummyMemRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              * This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Empty function.  always returns the same.

  @param[in] This        Ignored.
  @param[in] Width       Ignored.
  @param[in] Address     Ignored.
  @param[in] Count       Ignored.
  @param[in, out] Buffer Ignored.

  @retval EFI_UNSUPPORTED.
**/
EFI_STATUS
EFIAPI
DummyMemWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              * This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Initialization function for HDiskImage.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_LOAD_ERROR    A load error occured.
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
                &gEfiPciRootBridgeIoProtocolGuid,
                NULL,
                (VOID**)&HMemImage.IoFncs
                );
  if (Status == EFI_NOT_FOUND) {
    //
    // For NT32, no EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL is available
    // Use Dummy PciRootBridgeIo for memory access
    //
    ZeroMem (&DummyPciRootBridgeIo, sizeof (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL));
    DummyPciRootBridgeIo.Mem.Read  = DummyMemRead;
    DummyPciRootBridgeIo.Mem.Write = DummyMemWrite;
    HMemImage.IoFncs = &DummyPciRootBridgeIo;
    Status = EFI_SUCCESS;
  }
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
  IN UINTN Offset,
  IN UINTN Size
  )
{

  HMemImage.Offset  = Offset;
  HMemImage.Size    = Size;

  return EFI_SUCCESS;
}

/**
  Read a disk from disk into HBufferImage.

  @param[in] Offset   The offset.
  @param[in] Size     The size.
  @param[in] Recover  if is for recover, no information print.

  @retval EFI_LOAD_ERROR        A load error occured.
  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageRead (
  IN UINTN     Offset,
  IN UINTN     Size,
  IN BOOLEAN   Recover
  )
{

  EFI_STATUS                      Status;
  void                            *Buffer;
  CHAR16                          *Str;
  HEFI_EDITOR_LINE                *Line;

  EDIT_FILE_TYPE                  BufferTypeBackup;

  BufferTypeBackup        = HBufferImage.BufferType;
  HBufferImage.BufferType = FileTypeMemBuffer;

  Buffer                  = AllocateZeroPool (Size);
  if (Buffer == NULL) {
    StatusBarSetStatusString (L"Read Memory Failed");
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HMemImage.IoFncs->Mem.Read (
                                  HMemImage.IoFncs,
                                  EfiPciWidthUint8,
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

  Status  = HMemImageSetMemOffsetSize (Offset, Size);

  HBufferImage.DisplayPosition.Row    = 2;
  HBufferImage.DisplayPosition.Column = 10;

  HBufferImage.MousePosition.Row      = 2;
  HBufferImage.MousePosition.Column   = 10;

  HBufferImage.LowVisibleRow          = 1;
  HBufferImage.HighBits               = TRUE;

  HBufferImage.BufferPosition.Row     = 1;
  HBufferImage.BufferPosition.Column  = 1;

  if (!Recover) {
    Str = CatSPrint(NULL, L"%d Lines Read", HBufferImage.NumLines);
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

  @retval EFI_LOAD_ERROR        A load error occured.
  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageSave (
  IN UINTN Offset,
  IN UINTN Size
  )
{

  EFI_STATUS                      Status;
  VOID                            *Buffer;

  EDIT_FILE_TYPE                  BufferTypeBackup;

  //
  // not modified, so directly return
  //
  if (HBufferImage.Modified == FALSE) {
    return EFI_SUCCESS;
  }

  BufferTypeBackup        = HBufferImage.BufferType;
  HBufferImage.BufferType = FileTypeMemBuffer;

  Buffer                  = AllocateZeroPool (Size);

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
                                  EfiPciWidthUint8,
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


