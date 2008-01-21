/**@file
  Copyright (c) 2007, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/



/**
  Get current boot mode.

  @param  HobStart               Start pointer of hob list
  @param  BootMode               Current boot mode recorded in PHIT hob

  @retval EFI_NOT_FOUND          Invalid hob header
  @retval EFI_SUCCESS            Boot mode found

**/
EFI_STATUS
R8_GetHobBootMode (
  IN  VOID           *HobStart,
  OUT EFI_BOOT_MODE  *BootMode
  )
;




/**
  Get the next guid hob.

  @param  HobStart               A pointer to the start hob.
  @param  Guid                   A pointer to a guid.
  @param  Buffer                 A pointer to the buffer.
  @param  BufferSize             Buffer size.

  @retval EFI_NOT_FOUND          Next Guid hob not found
  @retval EFI_SUCCESS            Next Guid hob found and data for this Guid got
  @retval EFI_INVALID_PARAMETER  invalid parameter

**/
EFI_STATUS
R8_GetNextGuidHob (
  IN OUT VOID      **HobStart,
  IN     EFI_GUID  * Guid,
  OUT    VOID      **Buffer,
  OUT    UINTN     *BufferSize OPTIONAL
  )
;


