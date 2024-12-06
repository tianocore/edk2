/** @file
  Entry point to the Standalone MM Foundation when initialized during the SEC
  phase on ARM platforms that does no update the stack cookie dynamically.

Copyright (c) 2017 - 2021, Arm Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/Arm/StandaloneMmCoreEntryPoint.h>

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

extern
VOID
EFIAPI
_CModuleEntryPoint (
  IN VOID    *SharedBufAddress,
  IN UINT64  SharedBufSize,
  IN UINT64  cookie1,
  IN UINT64  cookie2
  );

/**
  The entry point of Standalone MM Foundation.

  @param  [in]  SharedBufAddress  Pointer to the Buffer between SPM and SP.
  @param  [in]  SharedBufSize     Size of the shared buffer.
  @param  [in]  cookie1           Cookie 1
  @param  [in]  cookie2           Cookie 2

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID    *SharedBufAddress,
  IN UINT64  SharedBufSize,
  IN UINT64  cookie1,
  IN UINT64  cookie2
  )
{
  //
  // Call the MM Core entry point
  //
  _CModuleEntryPoint (SharedBufAddress, SharedBufSize, cookie1, cookie2);
}
