/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DxeLoadFunc.c

Abstract:

  Ia32-specifc functionality for DxeLoad.

--*/

#include "DxeIpl.h"

VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS   DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS   HobList
  )
{
  EFI_STATUS                Status;
  EFI_PHYSICAL_ADDRESS      BaseOfStack;
  EFI_PHYSICAL_ADDRESS      TopOfStack;
  EFI_PHYSICAL_ADDRESS      PageTables;

  Status = PeiServicesAllocatePages (EfiBootServicesData, EFI_SIZE_TO_PAGES (STACK_SIZE), &BaseOfStack);
  ASSERT_EFI_ERROR (Status);
  
  if (FeaturePcdGet(PcdDxeIplSwitchToLongMode)) {
    //
    // Compute the top of the stack we were allocated, which is used to load X64 dxe core. 
    // Pre-allocate a 32 bytes which confroms to x64 calling convention.
    //
    // The first four parameters to a function are passed in rcx, rdx, r8 and r9. 
    // Any further parameters are pushed on the stack. Furthermore, space (4 * 8bytes) for the 
    // register parameters is reserved on the stack, in case the called function 
    // wants to spill them; this is important if the function is variadic. 
    //
    TopOfStack = BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - 32;

    //
    //  X64 Calling Conventions requires that the stack must be aligned to 16 bytes
    //
    TopOfStack = (EFI_PHYSICAL_ADDRESS) (UINTN) ALIGN_POINTER (TopOfStack, 16);
    //
    // Load the GDT of Go64. Since the GDT of 32-bit Tiano locates in the BS_DATA
    // memory, it may be corrupted when copying FV to high-end memory 
    //
    LoadGo64Gdt();
    //
    // Limit to 36 bits of addressing for debug. Should get it from CPU
    //
    PageTables = CreateIdentityMappingPageTables (36);
    //
    // Go to Long Mode. Interrupts will not get turned on until the CPU AP is loaded.
    // Call x64 drivers passing in single argument, a pointer to the HOBs.
    //
    ActivateLongMode (
      PageTables, 
      (EFI_PHYSICAL_ADDRESS)(UINTN)(HobList.Raw), 
      TopOfStack,
      0x00000000,
      DxeCoreEntryPoint
      );
  } else {
    //
    // Compute the top of the stack we were allocated. Pre-allocate a UINTN
    // for safety.
    //
    TopOfStack = BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT;
    TopOfStack = (EFI_PHYSICAL_ADDRESS) (UINTN) ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

    SwitchStack (
      (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeCoreEntryPoint,
      HobList.Raw,
      NULL,
      (VOID *) (UINTN) TopOfStack
      );
  } 
}
