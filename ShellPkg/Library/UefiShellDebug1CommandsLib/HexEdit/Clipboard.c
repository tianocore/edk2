/** @file
    Functions to deal with Clip Board

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HexEditor.h"

typedef struct {
  UINT8    *Buffer;
  UINTN    Size;
} HEFI_EDITOR_CLIPBOARD;

HEFI_EDITOR_CLIPBOARD  HClipBoard;

//
// for basic initialization of HClipBoard
//
HEFI_EDITOR_CLIPBOARD  HClipBoardConst = {
  NULL,
  0
};

/**
  Initialization function for HDiskImage.

  @param[in] EFI_SUCCESS      The operation was successful.
  @param[in] EFI_LOAD_ERROR   A load error occurred.
**/
EFI_STATUS
HClipBoardInit (
  VOID
  )
{
  //
  // basiclly initialize the HDiskImage
  //
  CopyMem (&HClipBoard, &HClipBoardConst, sizeof (HClipBoard));

  return EFI_SUCCESS;
}

/**
  Initialization function for HDiskImage.

  @param[in] EFI_SUCCESS      The operation was successful.
  @param[in] EFI_LOAD_ERROR   A load error occurred.
**/
EFI_STATUS
HClipBoardCleanup (
  VOID
  )
{
  SHELL_FREE_NON_NULL (HClipBoard.Buffer);

  return EFI_SUCCESS;
}

/**
  Set a buffer into the clipboard.

  @param[in] Buffer   The buffer to add to the clipboard.
  @param[in] Size     The size of Buffer in bytes.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
HClipBoardSet (
  IN UINT8  *Buffer,
  IN UINTN  Size
  )
{
  //
  // free the old clipboard buffer
  // and set new clipboard buffer
  //
  SHELL_FREE_NON_NULL (HClipBoard.Buffer);
  HClipBoard.Buffer = Buffer;

  HClipBoard.Size = Size;

  return EFI_SUCCESS;
}

/**
  Get a buffer from the clipboard.

  @param[out] Buffer   The pointer to the buffer to add to the clipboard.

  @return the size of the buffer.
**/
UINTN
HClipBoardGet (
  OUT UINT8  **Buffer
  )
{
  //
  // return the clipboard buffer
  //
  *Buffer = HClipBoard.Buffer;

  return HClipBoard.Size;
}
