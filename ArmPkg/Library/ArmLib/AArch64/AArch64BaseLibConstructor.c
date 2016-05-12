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

RETURN_STATUS
EFIAPI
AArch64LibConstructor (
  VOID
  )
{
  extern UINT32 ArmReplaceLiveTranslationEntrySize;

  //
  // The ArmReplaceLiveTranslationEntry () helper function may be invoked
  // with the MMU off so we have to ensure that it gets cleaned to the PoC
  //
  WriteBackDataCacheRange (ArmReplaceLiveTranslationEntry,
    ArmReplaceLiveTranslationEntrySize);

  return RETURN_SUCCESS;
}
