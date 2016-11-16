/** @file
  Prototypes for the EBC Debugger hooks.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_EBC_DEBUGGER_HOOK_H_
#define _EFI_EBC_DEBUGGER_HOOK_H_

#include <Uefi.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/EbcVmTest.h>

/**
  The VM interpreter calls this function when an exception is detected.

  @param  ExceptionType          Specifies the processor exception detected.
  @param  ExceptionFlags         Specifies the exception context.
  @param  VmPtr                  Pointer to a VM context for passing info to the
                                 EFI debugger.

  @retval EFI_SUCCESS            This function completed successfully.

**/
EFI_STATUS
EbcDebugSignalException (
  IN EFI_EXCEPTION_TYPE                   ExceptionType,
  IN EXCEPTION_FLAGS                      ExceptionFlags,
  IN VM_CONTEXT                           *VmPtr
  );

//
// Hooks in EbcInt.c
//
VOID
EbcDebuggerHookInit (
  IN EFI_HANDLE                  Handle,
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *EbcDebugProtocol
  );

VOID
EbcDebuggerHookUnload (
  VOID
  );

VOID
EbcDebuggerHookEbcUnloadImage (
  IN EFI_HANDLE                  Handle
  );

//
// Hooks in EbcSupport.c
//
VOID
EbcDebuggerHookExecuteEbcImageEntryPoint (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookEbcInterpret (
  IN VM_CONTEXT *VmPtr
  );

//
// Hooks in EbcExecute.c
//
VOID
EbcDebuggerHookExecuteStart (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookExecuteEnd (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookCALLStart (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookCALLEnd (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookCALLEXStart (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookCALLEXEnd (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookRETStart (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookRETEnd (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookJMPStart (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookJMPEnd (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookJMP8Start (
  IN VM_CONTEXT *VmPtr
  );

VOID
EbcDebuggerHookJMP8End (
  IN VM_CONTEXT *VmPtr
  );

#endif
