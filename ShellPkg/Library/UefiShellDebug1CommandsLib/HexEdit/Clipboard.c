/** @file
    Functions to deal with Clip Board
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HexEditor.h"

typedef struct {
  UINT8 *Buffer;
  UINTN Size;
} HEFI_EDITOR_CLIPBOARD;

HEFI_EDITOR_CLIPBOARD HClipBoard;

//
// for basic initialization of HClipBoard
//
HEFI_EDITOR_CLIPBOARD HClipBoardConst = {
  NULL,
  0
};

/**
  Initialization function for HDiskImage.

  @param[in] EFI_SUCCESS      The operation was successful.
  @param[in] EFI_LOAD_ERROR   A load error occured.
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
  @param[in] EFI_LOAD_ERROR   A load error occured.
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
  IN UINT8 *Buffer,
  IN UINTN Size
  )
{
  //
  // free the old clipboard buffer
  // and set new clipboard buffer
  //
  SHELL_FREE_NON_NULL (HClipBoard.Buffer);
  HClipBoard.Buffer = Buffer;

  HClipBoard.Size   = Size;

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
