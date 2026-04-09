/** @file
  Execute 32-bit code in Protected Mode.

  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <FspEas.h>

/**
  FSP API functions.

  @param[in] Param1       The first parameter to pass to 32bit code.
  @param[in] Param2       The second parameter to pass to 32bit code.

  @return EFI_STATUS.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_FUNCTION)(
  IN VOID *Param1,
  IN VOID *Param2
  );

/**
  Wrapper for a thunk to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.

  @param[in] Function     The 32bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 32bit code.
  @param[in] Param2       The second parameter to pass to 32bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute32BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  )
{
  FSP_FUNCTION  EntryFunc;
  EFI_STATUS    Status;

  EntryFunc = (FSP_FUNCTION)(UINTN)(Function);
  Status    = EntryFunc ((VOID *)(UINTN)Param1, (VOID *)(UINTN)Param2);

  return Status;
}

/**
  Wrapper for a thunk to transition from compatibility mode to long mode to execute 64-bit code and then transit back to
  compatibility mode.

  @param[in] Function     The 64bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 64bit code.
  @param[in] Param2       The second parameter to pass to 64bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute64BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  )
{
  return EFI_UNSUPPORTED;
}
