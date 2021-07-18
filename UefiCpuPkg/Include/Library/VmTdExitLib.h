/** @file
  Public header file for the VMTDEXIT Support library class.

  This library class defines some routines used when invoking the VMEXIT
  instruction in support of VMX and TDX to handle #VE exceptions.

  Copyright (c) 2020, Intel Inc. All rights reserved.<BR>
  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VM_TD_EXIT_LIB_H__
#define __VM_TD_EXIT_LIB_H__

#include <Library/BaseLib.h>
#include <Uefi/UefiBaseType.h>
#include <Protocol/DebugSupport.h>

#define VE_EXCEPTION    20

/**
  Handle a #VE exception.

  Performs the necessary processing to handle a #VE exception.

  The base library function returns an error equal to VE_EXCEPTION,
  to be propagated to the standard exception handling stack.

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
  );

#endif
