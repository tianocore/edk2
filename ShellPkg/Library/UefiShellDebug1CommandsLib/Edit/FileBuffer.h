/** @file
  Declares filebuffer interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_FILE_BUFFER_H_
#define _LIB_FILE_BUFFER_H_

#include "TextEditorTypes.h"

/**
  Initialization function for FileBuffer.

  @param EFI_SUCCESS            The initialization was successful.
  @param EFI_LOAD_ERROR         A default name could not be created.
  @param EFI_OUT_OF_RESOURCES   A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferInit (
  VOID
  );

/**
  Cleanup function for FileBuffer.

  @retval EFI_SUCCESS   The cleanup was successful.
**/
EFI_STATUS
EFIAPI
FileBufferCleanup (
  VOID
  );

/**
  Refresh the screen with whats in the buffer.

  @retval EFI_SUCCESS     The refresh was successful.
  @retval EFI_LOAD_ERROR  There was an error finding what to write.
**/
EFI_STATUS
EFIAPI
FileBufferRefresh (
  VOID
  );

/** 
  Dispatch input to different handler
  @param[in] Key                The input key.  One of:
                                    ASCII KEY
                                    Backspace/Delete
                                    Return
                                    Direction key: up/down/left/right/pgup/pgdn
                                    Home/End
                                    INS

  @retval EFI_SUCCESS           The dispatch was done successfully.
  @retval EFI_LOAD_ERROR        The dispatch was not successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferHandleInput (
  IN CONST EFI_INPUT_KEY * Key
  );

/**
  Backup function for FileBuffer.  Only backup the following items:
      Mouse/Cursor position
      File Name, Type, ReadOnly, Modified
      Insert Mode

  This is for making the file buffer refresh as few as possible.

  @retval EFI_SUCCESS           The backup operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferBackup (
  VOID
  );

/**
  Set the cursor position according to FileBuffer.DisplayPosition.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
FileBufferRestorePosition (
  VOID
  );

/**
  Set FileName field in FileBuffer.

  @param Str                    The file name to set.
  
  @retval EFI_SUCCESS           The filename was successfully set.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_INVALID_PARAMETER Str is not a valid filename.
**/
EFI_STATUS
EFIAPI
FileBufferSetFileName (
  IN CONST CHAR16 *Str
  );

/**
  Read a file from disk into the FileBuffer.
  
  @param[in] FileName           The filename to read.
  @param[in] Recover            TRUE if is for recover mode, no information printouts.
  
  @retval EFI_SUCCESS            The load was successful.
  @retval EFI_LOAD_ERROR         The load failed.
  @retval EFI_OUT_OF_RESOURCES   A memory allocation failed.
  @retval EFI_INVALID_PARAMETER  FileName is a directory.
**/
EFI_STATUS
EFIAPI
FileBufferRead (
  IN CONST CHAR16  *FileName,
  IN CONST BOOLEAN Recover
  );

/**
  Save lines in FileBuffer to disk

  @param[in] FileName           The file name for writing.

  @retval EFI_SUCCESS           Data was written.
  @retval EFI_LOAD_ERROR        
  @retval EFI_OUT_OF_RESOURCES  There were not enough resources to write the file.
**/
EFI_STATUS
EFIAPI
FileBufferSave (
  CONST CHAR16 *FileName
  );

/**
  According to cursor's file position, adjust screen display

  @param[in] NewFilePosRow    The row of file position ( start from 1 ).
  @param[in] NewFilePosCol    The column of file position ( start from 1 ).
**/
VOID
EFIAPI
FileBufferMovePosition (
  IN CONST UINTN NewFilePosRow,
  IN CONST UINTN NewFilePosCol
  );

/**
  Cut current line out and return a pointer to it.

  @param[out] CutLine    Upon a successful return pointer to the pointer to 
                        the allocated cut line.

  @retval EFI_SUCCESS             The cut was successful.
  @retval EFI_NOT_FOUND           There was no selection to cut.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferCutLine (
  OUT EFI_EDITOR_LINE **CutLine
  );

/**
  Paste a line into line list.

  @retval EFI_SUCCESS             The paste was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferPasteLine (
  VOID
  );

/**
  Search string from current position on in file

  @param[in] Str    The search string.
  @param[in] Offset The offset from current position.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_NOT_FOUND     The string Str was not found.
**/
EFI_STATUS
EFIAPI
FileBufferSearch (
  IN CONST CHAR16  *Str,
  IN CONST UINTN Offset
  );

/**
  Replace SearchLen characters from current position on with Replace.

  This will modify the current buffer at the current position.

  @param[in] Replace    The string to replace.
  @param[in] SearchLen  Search string's length.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileBufferReplace (
  IN CONST CHAR16   *Replace,
  IN CONST UINTN    SearchLen
  );

/**
  Search and replace operation.

  @param[in] SearchStr    The string to search for.
  @param[in] ReplaceStr   The string to replace with.
  @param[in] Offset       The column to start at.
**/
EFI_STATUS
EFIAPI
FileBufferReplaceAll (
  IN CHAR16 *SearchStr,
  IN CHAR16 *ReplaceStr,
  IN UINTN  Offset
  );

/**
  Move the mouse cursor position.

  @param[in] TextX      The new x-coordinate.
  @param[in] TextY      The new y-coordinate.
**/
VOID
EFIAPI
FileBufferAdjustMousePosition (
  IN CONST INT32 TextX,
  IN CONST INT32 TextY
  );

/**
  Set the modified state to TRUE.
**/
VOID
EFIAPI
FileBufferSetModified (
  VOID
  );

#endif
