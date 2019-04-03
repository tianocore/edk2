/** @file
  Execute 32-bit code in Protected Mode.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <FspApi.h>

typedef
EFI_STATUS
(EFIAPI *FSP_FUNCTION) (
  IN VOID *Param1
  );

/**
  Wrapper for a thunk  to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.

  @param[in] Function     The 32bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 32bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute32BitCode (
  IN UINT64      Function,
  IN UINT64      Param1
  )
{
  FSP_FUNCTION               EntryFunc;
  EFI_STATUS                 Status;

  EntryFunc = (FSP_FUNCTION) (UINTN) (Function);
  Status    = EntryFunc ((VOID *)(UINTN)Param1);

  return Status;
}

