#/* @file
#
#  Copyright (c) 2016, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#*/

#include <Base.h>

#include <Library/ArmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>

//
// This is a hack. We define a weak symbol with external linkage, which may or
// may not be overridden by a non-weak alternative that is defined with a non
// zero value in the object that contains the MMU routines. Since static
// libraries are pulled in on a per-object basis, and since the MMU object will
// only be pulled in if any of its other symbols are referenced by the client
// module, we can use the value below to figure out whether the MMU routines are
// in use by this module, and decide whether cache maintenance of the function
// ArmReplaceLiveTranslationEntry () is required.
//
INT32 __attribute__((weak)) HaveMmuRoutines;

EFI_STATUS
EFIAPI
AArch64LibConstructor (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  extern UINT32             ArmReplaceLiveTranslationEntrySize;

  EFI_FV_FILE_INFO          FileInfo;
  EFI_STATUS                Status;

  if (HaveMmuRoutines == 0) {
    return RETURN_SUCCESS;
  }

  ASSERT (FileHandle != NULL);

  Status = (*PeiServices)->FfsGetFileInfo (FileHandle, &FileInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // Some platforms do not cope very well with cache maintenance being
  // performed on regions backed by NOR flash. Since the cache maintenance
  // is unnecessary to begin with in that case, perform it only when not
  // executing in place.
  //
  if ((UINTN)FileInfo.Buffer <= (UINTN)ArmReplaceLiveTranslationEntry &&
      ((UINTN)FileInfo.Buffer + FileInfo.BufferSize >=
       (UINTN)ArmReplaceLiveTranslationEntry + ArmReplaceLiveTranslationEntrySize)) {
    DEBUG ((EFI_D_INFO, "ArmLib: skipping cache maintence on XIP PEIM\n"));
  } else {
    DEBUG ((EFI_D_INFO, "ArmLib: performing cache maintence on shadowed PEIM\n"));
    //
    // The ArmReplaceLiveTranslationEntry () helper function may be invoked
    // with the MMU off so we have to ensure that it gets cleaned to the PoC
    //
    WriteBackDataCacheRange (ArmReplaceLiveTranslationEntry,
      ArmReplaceLiveTranslationEntrySize);
  }

  return RETURN_SUCCESS;
}
