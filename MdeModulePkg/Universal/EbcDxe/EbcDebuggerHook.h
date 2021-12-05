/** @file
  Prototypes for the EBC Debugger hooks.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EXCEPTION_FLAGS     ExceptionFlags,
  IN VM_CONTEXT          *VmPtr
  );

/**

  The hook in InitializeEbcDriver.

  @param Handle           - The EbcDebugProtocol handle.
  @param EbcDebugProtocol - The EbcDebugProtocol interface.

**/
VOID
EbcDebuggerHookInit (
  IN EFI_HANDLE                  Handle,
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *EbcDebugProtocol
  );

/**

The hook in UnloadImage for EBC Interpreter.

**/
VOID
EbcDebuggerHookUnload (
  VOID
  );

/**

  The hook in EbcUnloadImage.
  Currently do nothing here.

  @param  Handle          The EbcImage handle.

**/
VOID
EbcDebuggerHookEbcUnloadImage (
  IN EFI_HANDLE  Handle
  );

/**

  Hooks in EbcSupport.c

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteEbcImageEntryPoint (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteEbcImageEntryPoint.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookEbcInterpret (
  IN VM_CONTEXT  *VmPtr
  );

/**
  The hook in EbcExecute, before ExecuteFunction.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteStart (
  IN VM_CONTEXT  *VmPtr
  );

/**
  The hook in EbcExecute, after ExecuteFunction.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteEnd (
  IN VM_CONTEXT  *VmPtr
  );

/**
  The hook in ExecuteCALL, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLStart (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteCALL, after move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEnd (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteCALL, before call EbcLLCALLEX.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEXStart (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteCALL, after call EbcLLCALLEX.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEXEnd (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteRET, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookRETStart (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteRET, after move IP.
  It will record trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookRETEnd (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteJMP, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMPStart (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteJMP, after move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMPEnd (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteJMP8, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMP8Start (
  IN VM_CONTEXT  *VmPtr
  );

/**

  The hook in ExecuteJMP8, after move IP..

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMP8End (
  IN VM_CONTEXT  *VmPtr
  );

#endif
