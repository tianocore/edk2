#/* @file
#
#  Copyright (c) 2016, Linaro Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#*/

#include <Base.h>

#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>

EFI_STATUS
EFIAPI
ArmMmuPeiLibConstructor (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  extern UINT32             ArmReplaceLiveTranslationEntrySize;

  EFI_FV_FILE_INFO          FileInfo;
  EFI_STATUS                Status;

  ASSERT (FileHandle != NULL);

  Status = (*PeiServices)->FfsGetFileInfo (FileHandle, &FileInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // Some platforms do not cope very well with cache maintenance being
  // performed on regions backed by NOR flash. Since the firmware image
  // can be assumed to be clean to the PoC when running XIP, even when PEI
  // is executing from DRAM, we only need to perform the cache maintenance
  // when not executing in place.
  //
  if ((UINTN)FileInfo.Buffer <= (UINTN)ArmReplaceLiveTranslationEntry &&
      ((UINTN)FileInfo.Buffer + FileInfo.BufferSize >=
       (UINTN)ArmReplaceLiveTranslationEntry + ArmReplaceLiveTranslationEntrySize)) {
    DEBUG ((EFI_D_INFO, "ArmMmuLib: skipping cache maintenance on XIP PEIM\n"));
  } else {
    DEBUG ((EFI_D_INFO, "ArmMmuLib: performing cache maintenance on shadowed PEIM\n"));
    //
    // The ArmReplaceLiveTranslationEntry () helper function may be invoked
    // with the MMU off so we have to ensure that it gets cleaned to the PoC
    //
    WriteBackDataCacheRange (ArmReplaceLiveTranslationEntry,
      ArmReplaceLiveTranslationEntrySize);
  }

  return RETURN_SUCCESS;
}
