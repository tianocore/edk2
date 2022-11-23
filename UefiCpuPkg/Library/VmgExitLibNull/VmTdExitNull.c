/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Uefi.h>
#include <Library/VmgExitLib.h>

/**
  Handle a #VE exception.

  Performs the necessary processing to handle a #VE exception.

  @param[in, out]  ExceptionType  Pointer to an EFI_EXCEPTION_TYPE to be set
                                  as value to use on error.
  @param[in, out]  SystemContext  Pointer to EFI_SYSTEM_CONTEXT

  @retval  EFI_SUCCESS            Exception handled
  @retval  EFI_UNSUPPORTED        #VE not supported, (new) exception value to
                                  propagate provided
  @retval  EFI_PROTOCOL_ERROR     #VE handling failed, (new) exception value to
                                  propagate provided

**/
EFI_STATUS
EFIAPI
VmTdExitHandleVe (
  IN OUT EFI_EXCEPTION_TYPE  *ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  *ExceptionType = VE_EXCEPTION;

  return EFI_UNSUPPORTED;
}
