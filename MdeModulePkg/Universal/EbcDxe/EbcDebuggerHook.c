/** @file
  Contains the empty version of the EBC Debugger hooks, to be used when
  compiling the regular EBC VM module.
  As debugging is not needed for the standard EBC VM, all calls are left empty.

  The EBC Debugger defines its own version for these calls in EbdHooks.c.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EbcDebuggerHook.h"

/**

  The hook in InitializeEbcDriver.

  @param Handle           - The EbcDebugProtocol handle.
  @param EbcDebugProtocol - The EbcDebugProtocol interface.

**/
VOID
EbcDebuggerHookInit (
  IN EFI_HANDLE                  Handle,
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *EbcDebugProtocol
  )
{
  return;
}

/**

The hook in UnloadImage for EBC Interpreter.

**/
VOID
EbcDebuggerHookUnload (
  VOID
  )
{
  return;
}

/**

  The hook in EbcUnloadImage.
  Currently do nothing here.

  @param  Handle          The EbcImage handle.

**/
VOID
EbcDebuggerHookEbcUnloadImage (
  IN EFI_HANDLE                  Handle
  )
{
  return;
}

/**

  The hook in ExecuteEbcImageEntryPoint.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteEbcImageEntryPoint (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteEbcImageEntryPoint.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookEbcInterpret (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**
  The hook in EbcExecute, before ExecuteFunction.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**
  The hook in EbcExecute, after ExecuteFunction.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteCALL, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteCALL, after move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteCALL, before call EbcLLCALLEX.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEXStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteCALL, after call EbcLLCALLEX.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEXEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteRET, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookRETStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteRET, after move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookRETEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteJMP, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMPStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteJMP, after move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMPEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteJMP8, before move IP.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMP8Start (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

/**

  The hook in ExecuteJMP8, after move IP..

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMP8End (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}
