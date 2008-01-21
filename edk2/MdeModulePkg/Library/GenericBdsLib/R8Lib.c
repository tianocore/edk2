/**@file
  Copyright (c) 2007, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "InternalBdsLib.h"

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
{
  //
  // Porting Guide:
  // This library interface is simply obsolete.
  // Include the source code to user code.
  // In fact, since EFI_HANDOFF_HOB must be the first Hob,
  // the following code can retrieve boot mode.
  //
  // EFI_HOB_HANDOFF_INFO_TABLE *HandOffHob;
  //
  // HandOffHob = GetHobList ();
  // ASSERT (HandOffHob->Header.HobType == EFI_HOB_TYPE_HANDOFF);
  //
  // BootMode = HandOffHob->BootMode;
  //
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  if (Hob.Header->HobType != EFI_HOB_TYPE_HANDOFF) {
    return EFI_NOT_FOUND;
  }

  *BootMode = Hob.HandoffInformationTable->BootMode;
  return EFI_SUCCESS;
}




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
{
  //
  // Porting Guide:
  // This library interface is changed substantially with R9 counerpart GetNextGuidHob ().
  // 1. R9 GetNextGuidHob has two parameters and returns the matched GUID HOB from the StartHob.
  // 2. R9 GetNextGuidHob does not strip the HOB header, so caller is required to apply
  //    GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE () to extract the data section and its
  //    size info respectively.
  // 3. this function does not skip the starting HOB pointer unconditionally:
  //    it returns HobStart back if HobStart itself meets the requirement;
  //    caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  //
  EFI_PEI_HOB_POINTERS  GuidHob;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  GuidHob.Raw = GetNextGuidHob (Guid, *HobStart);
  if (GuidHob.Raw == NULL) {
    return EFI_NOT_FOUND;
  }

  *Buffer = GET_GUID_HOB_DATA (GuidHob.Guid);
  if (BufferSize != NULL) {
    *BufferSize = GET_GUID_HOB_DATA_SIZE (GuidHob.Guid);
  }

  *HobStart = GET_NEXT_HOB (GuidHob);

  return EFI_SUCCESS;
}


