/** @file
  Contains the empty version of the EBC Debugger hooks, to be used when
  compiling the regular EBC VM module.
  As debugging is not needed for the standard EBC VM, all calls are left empty.

  The EBC Debugger defines its own version for these calls in EbdHooks.c.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EbcDebuggerHook.h"

VOID
EbcDebuggerHookInit (
  IN EFI_HANDLE                  Handle,
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *EbcDebugProtocol
  )
{
  return;
}

VOID
EbcDebuggerHookUnload (
  VOID
  )
{
  return;
}

VOID
EbcDebuggerHookEbcUnloadImage (
  IN EFI_HANDLE                  Handle
  )
{
  return;
}

VOID
EbcDebuggerHookExecuteEbcImageEntryPoint (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookEbcInterpret (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookExecuteStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookExecuteEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookCALLStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookCALLEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookCALLEXStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookCALLEXEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookRETStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookRETEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookJMPStart (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookJMPEnd (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookJMP8Start (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}

VOID
EbcDebuggerHookJMP8End (
  IN VM_CONTEXT *VmPtr
  )
{
  return;
}
