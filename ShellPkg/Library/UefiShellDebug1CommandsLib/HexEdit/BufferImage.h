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

EFI_STATUS
HBufferImageInit (
  VOID
  );
EFI_STATUS
HBufferImageCleanup (
  VOID
  );
EFI_STATUS
HBufferImageRefresh (
  VOID
  );
EFI_STATUS
HBufferImageHide (
  VOID
  );
EFI_STATUS
HBufferImageHandleInput (
  EFI_INPUT_KEY *
  );
EFI_STATUS
HBufferImageBackup (
  VOID
  );

EFI_STATUS
HBufferImageRead (
  IN CONST CHAR16   *,
  IN CONST CHAR16   *,
  IN          UINTN,
  IN          UINTN,
  IN          UINTN,
  IN          UINTN,
  IN          EDIT_FILE_TYPE,
  IN          BOOLEAN
  );

EFI_STATUS
HBufferImageSave (
  IN CHAR16   *,
  IN CHAR16   *,
  IN          UINTN,
  IN          UINTN,
  IN          UINTN,
  IN          UINTN,
  IN          EDIT_FILE_TYPE
  );

INTN
HBufferImageCharToHex (
  IN CHAR16
  );

EFI_STATUS
HBufferImageRestoreMousePosition (
  VOID
  );
EFI_STATUS
HBufferImageRestorePosition (
  VOID
  );

VOID
HBufferImageMovePosition (
  IN UINTN,
  IN UINTN,
  IN BOOLEAN
  );

EFI_STATUS
HBufferImageHandleInput (
  EFI_INPUT_KEY *
  );

HEFI_EDITOR_LINE  *
HBufferImageCreateLine (
  VOID
  );

EFI_STATUS
HBufferImageDoCharInput (
  CHAR16
  );
EFI_STATUS
HBufferImageAddChar (
  CHAR16
  );

BOOLEAN
HInCurrentScreen (
  UINTN
  );
BOOLEAN
HAboveCurrentScreen (
  UINTN
  );
BOOLEAN
HUnderCurrentScreen (
  UINTN
  );

EFI_STATUS
HBufferImageScrollRight (
  VOID
  );
EFI_STATUS
HBufferImageScrollLeft (
  VOID
  );
EFI_STATUS
HBufferImageScrollDown (
  VOID
  );
EFI_STATUS
HBufferImageScrollUp (
  VOID
  );
EFI_STATUS
HBufferImagePageUp (
  VOID
  );
EFI_STATUS
HBufferImagePageDown (
  VOID
  );
EFI_STATUS
HBufferImageHome (
  VOID
  );
EFI_STATUS
HBufferImageEnd (
  VOID
  );

EFI_STATUS
HBufferImageDoBackspace (
  VOID
  );
EFI_STATUS
HBufferImageDoDelete (
  VOID
  );

EFI_STATUS
HBufferImageCutLine (
  HEFI_EDITOR_LINE **
  );
EFI_STATUS
HBufferImagePasteLine (
  VOID
  );

EFI_STATUS
HBufferImageGetFileInfo (
  EFI_FILE_HANDLE,
  CHAR16          *,
  EFI_FILE_INFO   **
  );

EFI_STATUS
HBufferImageSearch (
  CHAR16  *,
  UINTN
  );
EFI_STATUS
HBufferImageReplace (
  CHAR16  *,
  UINTN
  );

EFI_STATUS
HBufferImageFree (
  VOID
  ) ;

EFI_STATUS
HBufferImageDeleteCharacterFromBuffer (
  IN      UINTN,
  IN      UINTN,
  UINT8   *
  );

EFI_STATUS
HBufferImageAddCharacterToBuffer (
  IN      UINTN,
  IN      UINTN,
  UINT8   *
  );

EFI_STATUS
HBufferImageBufferToList (
  IN VOID   *,
  IN        UINTN
  );

EFI_STATUS
HBufferImageListToBuffer (
  IN VOID   *,
  IN        UINTN
  );

VOID
HBufferImageAdjustMousePosition (
  INT32,
  INT32
  );

BOOLEAN
HBufferImageIsAtHighBits (
  UINTN,
  UINTN *
  ) ;

EFI_STATUS
HBufferImageCutLine (
  HEFI_EDITOR_LINE **
  );

UINTN
HBufferImageGetTotalSize (
  VOID
  );

BOOLEAN
HBufferImageIsInSelectedArea (
  UINTN,
  UINTN
  );

#endif
