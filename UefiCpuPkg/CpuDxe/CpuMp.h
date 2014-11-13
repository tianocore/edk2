/** @file
  CPU DXE MP support

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_MP_H_
#define _CPU_MP_H_

/**
  Initialize Multi-processor support.

**/
VOID
InitializeMpSupport (
  VOID
  );

typedef
VOID
(EFIAPI *STACKLESS_AP_ENTRY_POINT)(
  VOID
  );

/**
  Starts the Application Processors and directs them to jump to the
  specified routine.

  The processor jumps to this code in flat mode, but the processor's
  stack is not initialized.

  @param ApEntryPoint    Pointer to the Entry Point routine

  @retval EFI_SUCCESS           The APs were started
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate memory to start APs

**/
EFI_STATUS
StartApsStackless (
  IN STACKLESS_AP_ENTRY_POINT ApEntryPoint
  );

/**
  The AP entry point that the Startup-IPI target code will jump to.

  The processor jumps to this code in flat mode, but the processor's
  stack is not initialized.

**/
VOID
EFIAPI
AsmApEntryPoint (
  VOID
  );

/**
  Releases the lock preventing other APs from using the shared AP
  stack.

  Once the AP has transitioned to using a new stack, it can call this
  function to allow another AP to proceed with using the shared stack.

**/
VOID
EFIAPI
AsmApDoneWithCommonStack (
  VOID
  );

#endif // _CPU_MP_H_

