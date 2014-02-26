/** @file
    Defines DiskImage - the view of the file that is visible at any point, 
    as well as the event handlers for editing the file
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_CLIP_BOARD_H_
#define _LIB_CLIP_BOARD_H_

#include "HexEditor.h"

/**
  Initialization function for HDiskImage

  @param[in] EFI_SUCCESS      The operation was successful.
  @param[in] EFI_LOAD_ERROR   A load error occured.
**/
EFI_STATUS
HClipBoardInit (
  VOID
  );

/**
  Initialization function for HDiskImage.

  @param[in] EFI_SUCCESS      The operation was successful.
  @param[in] EFI_LOAD_ERROR   A load error occured.
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
