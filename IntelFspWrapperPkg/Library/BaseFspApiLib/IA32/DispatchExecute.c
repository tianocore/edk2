/** @file
  Execute 32-bit code in Protected Mode.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

