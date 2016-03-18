/** @file
    Defines BufferImage - the view of the file that is visible at any point, 
    as well as the event handlers for editing the file
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_BUFFER_IMAGE_H_
#define _LIB_BUFFER_IMAGE_H_

#include "HexEditor.h"

/**
  Initialization function for HBufferImage

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_LOAD_ERROR    A load error occured.
**/
EFI_STATUS
HBufferImageInit (
  VOID
  );

/**
  Cleanup function for HBufferImage

  @retval EFI_SUCCESS  The operation was successful.
**/
EFI_STATUS
HBufferImageCleanup (
  VOID
  );

/**
  Refresh function for HBufferImage.

  @retval EFI_SUCCESS     The operation was successful.
  @retval EFI_LOAD_ERROR  A Load error occured.

**/
EFI_STATUS
HBufferImageRefresh (
  VOID
  );

/**
  Dispatch input to different handler

  @param[in] Key    The input key:
                     the keys can be:
                       ASCII KEY
                        Backspace/Delete
                        Direction key: up/down/left/right/pgup/pgdn
                        Home/End
                        INS

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_LOAD_ERROR        A load error occured.
  @retval EFI_OUT_OF_RESOURCES  A Memory allocation failed.
**/
EFI_STATUS
HBufferImageHandleInput (
  IN  EFI_INPUT_KEY *Key
  );

/**
  Backup function for HBufferImage. Only a few fields need to be backup. 
  This is for making the file buffer refresh as few as possible.

  @retval EFI_SUCCESS  The operation was successful.
**/
EFI_STATUS
HBufferImageBackup (
  VOID
  );

/**
  Read an image into a buffer friom a source.

  @param[in] FileName     Pointer to the file name.  OPTIONAL and ignored if not FileTypeFileBuffer.
  @param[in] DiskName     Pointer to the disk name.  OPTIONAL and ignored if not FileTypeDiskBuffer.
  @param[in] DiskOffset   Offset into the disk.  OPTIONAL and ignored if not FileTypeDiskBuffer.
  @param[in] DiskSize     Size of the disk buffer.  OPTIONAL and ignored if not FileTypeDiskBuffer.
  @param[in] MemOffset    Offset into the Memory.  OPTIONAL and ignored if not FileTypeMemBuffer.
  @param[in] MemSize      Size of the Memory buffer.  OPTIONAL and ignored if not FileTypeMemBuffer.
  @param[in] BufferType   The type of buffer to save.  IGNORED.
  @param[in] Recover      TRUE for recovermode, FALSE otherwise.

  @return EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
EFIAPI
HBufferImageRead (
  IN CONST CHAR16                   *FileName,
  IN CONST CHAR16                   *DiskName,
  IN UINTN                          DiskOffset,
  IN UINTN                          DiskSize,
  IN UINTN                          MemOffset,
  IN UINTN                          MemSize,
  IN EDIT_FILE_TYPE                 BufferType,
  IN BOOLEAN                        Recover
  );

/**
  Save the current image.

  @param[in] FileName     Pointer to the file name.  OPTIONAL and ignored if not FileTypeFileBuffer.
  @param[in] DiskName     Pointer to the disk name.  OPTIONAL and ignored if not FileTypeDiskBuffer.
  @param[in] DiskOffset   Offset into the disk.  OPTIONAL and ignored if not FileTypeDiskBuffer.
  @param[in] DiskSize     Size of the disk buffer.  OPTIONAL and ignored if not FileTypeDiskBuffer.
  @param[in] MemOffset    Offset into the Memory.  OPTIONAL and ignored if not FileTypeMemBuffer.
  @param[in] MemSize      Size of the Memory buffer.  OPTIONAL and ignored if not FileTypeMemBuffer.
  @param[in] BufferType   The type of buffer to save.  IGNORED.

  @return EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
HBufferImageSave (
  IN CHAR16                         *FileName,
  IN CHAR16                         *DiskName,
  IN UINTN                          DiskOffset,
  IN UINTN                          DiskSize,
  IN UINTN                          MemOffset,
  IN UINTN                          MemSize,
  IN EDIT_FILE_TYPE                 BufferType
  );

/**
  According to cursor's file position, adjust screen display.

  @param[in] NewFilePosRow    Row of file position ( start from 1 ).
  @param[in] NewFilePosCol    Column of file position ( start from 1 ).
  @param[in] HighBits         Cursor will on high4 bits or low4 bits.
**/
VOID
HBufferImageMovePosition (
  IN UINTN    NewFilePosRow,
  IN UINTN    NewFilePosCol,
  IN BOOLEAN  HighBits
  );


/**
  Create a new line and append it to the line list.
    Fields affected:
    NumLines
    Lines 

  @retval NULL    create line failed.
  @return         the line created.

**/
HEFI_EDITOR_LINE  *
HBufferImageCreateLine (
  VOID
  );

/**
  Free the current image.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
HBufferImageFree (
  VOID
  );

/**
  Delete character from buffer.
  
  @param[in] Pos      Position, Pos starting from 0.
  @param[in] Count    The Count of characters to delete.
  @param[out] DeleteBuffer    The DeleteBuffer.

  @retval EFI_SUCCESS Success 
**/
EFI_STATUS
HBufferImageDeleteCharacterFromBuffer (
  IN  UINTN         Pos,
  IN  UINTN         Count,
  OUT UINT8         *DeleteBuffer
  );

/**
  Add character to buffer, add before pos.

  @param[in] Pos        Position, Pos starting from 0.
  @param[in] Count      Count of characters to add.
  @param[in] AddBuffer  Add buffer.

  @retval EFI_SUCCESS   Success.  
**/
EFI_STATUS
HBufferImageAddCharacterToBuffer (
  IN  UINTN          Pos,
  IN  UINTN          Count,
  IN  UINT8          *AddBuffer
  );

/**
  Change the raw buffer to a list of lines for the UI.
  
  @param[in] Buffer   The pointer to the buffer to fill.
  @param[in] Bytes    The size of the buffer in bytes.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
HBufferImageBufferToList (
  IN VOID   *Buffer,
  IN UINTN  Bytes
  );

/**
  Change the list of lines from the UI to a raw buffer.
  
  @param[in] Buffer   The pointer to the buffer to fill.
  @param[in] Bytes    The size of the buffer in bytes.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
HBufferImageListToBuffer (
  IN VOID   *Buffer,
  IN UINTN  Bytes
  );

/**
  Move the mouse in the image buffer.

  @param[in] TextX    The x-coordinate.
  @param[in] TextY    The y-coordinate.
**/
VOID
EFIAPI
HBufferImageAdjustMousePosition (
  IN INT32 TextX,
  IN INT32 TextY
  );

/**
  Function to decide if a column number is stored in the high bits.

  @param[in] Column     The column to examine.
  @param[out] FCol      The actual column number.

  @retval TRUE      The actual column was in high bits and is now in FCol.
  @retval FALSE     There was not a column number in the high bits.
**/
BOOLEAN
HBufferImageIsAtHighBits (
  IN  UINTN Column,
  OUT UINTN *FCol
  );

/**
  Get the size of the open buffer.

  @retval The size in bytes.
**/
UINTN
HBufferImageGetTotalSize (
  VOID
  );

#endif
