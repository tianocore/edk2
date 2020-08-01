/** @file
    Defines DiskImage - the view of the file that is visible at any point,
    as well as the event handlers for editing the file

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_CLIP_BOARD_H_
#define _LIB_CLIP_BOARD_H_

#include "HexEditor.h"

/**
  Initialization function for HDiskImage

  @param[in] EFI_SUCCESS      The operation was successful.
  @param[in] EFI_LOAD_ERROR   A load error occurred.
**/
EFI_STATUS
HClipBoardInit (
  VOID
  );

/**
  Initialization function for HDiskImage.

  @param[in] EFI_SUCCESS      The operation was successful.
  @param[in] EFI_LOAD_ERROR   A load error occurred.
**/
EFI_STATUS
HClipBoardCleanup (
  VOID
  );

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
  );

/**
  Get a buffer from the clipboard.

  @param[out] Buffer   The pointer to the buffer to add to the clipboard.

  @return the size of the buffer.
**/
UINTN
HClipBoardGet (
  OUT UINT8  **Buffer
  );

#endif
