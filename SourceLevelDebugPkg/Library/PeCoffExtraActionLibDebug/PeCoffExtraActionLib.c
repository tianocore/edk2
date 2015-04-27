/** @file
  PE/Coff Extra Action library instances.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PeCoffExtraActionLib.h>

/**
  Check if the hardware breakpoint in Drx is enabled by checking the Lx and Gx bit in Dr7.
  
  It assumes that DebugAgent will set both Lx and Gx bit when setting up the hardware breakpoint.


  @param  RegisterIndex  Index of Dr register. The value range is from 0 to 3.
  @param  Dr7            Value of Dr7 register.

  @return TRUE   The hardware breakpoint specified in the Drx is enabled.
  @return FALSE  The hardware breakpoint specified in the Drx is disabled.

**/
BOOLEAN
IsDrxEnabled (
  IN  UINT8  RegisterIndex,
  IN  UINTN  Dr7
  )
{
  return (BOOLEAN) (((Dr7 >> (RegisterIndex * 2)) & (BIT0 | BIT1)) == (BIT0 | BIT1));
}

/**
  Common routine to report the PE/COFF image loading/relocating or unloading event.

  If ImageContext is NULL, then ASSERT().
  
  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image.
  @param  Signature     IMAGE_LOAD_SIGNATURE or IMAGE_UNLOAD_SIGNATURE.

**/
VOID
PeCoffLoaderExtraActionCommon (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN     UINTN                         Signature
  )
{
  BOOLEAN                    InterruptState;
  UINTN                      Dr0;
  UINTN                      Dr1;
  UINTN                      Dr2;
  UINTN                      Dr3;
  UINTN                      Dr7;
  UINTN                      Cr4;
  UINTN                      NewDr7;
  UINT8                      LoadImageMethod;
  UINT8                      DebugAgentStatus;
  IA32_DESCRIPTOR            IdtDescriptor;
  IA32_IDT_GATE_DESCRIPTOR   OriginalIdtEntry;
  BOOLEAN                    IdtEntryHooked;
  UINT32                     RegEdx;

  ASSERT (ImageContext != NULL);

  if (ImageContext->PdbPointer != NULL) {
    DEBUG((EFI_D_ERROR, "    PDB = %a\n", ImageContext->PdbPointer));
  }

  //
  // Disable interrupts and save the current interrupt state
  //
  InterruptState = SaveAndDisableInterrupts ();

  IdtEntryHooked  = FALSE;
  LoadImageMethod = PcdGet8 (PcdDebugLoadImageMethod);
  if (LoadImageMethod == DEBUG_LOAD_IMAGE_METHOD_IO_HW_BREAKPOINT) {
    //
    // If the CPU does not support Debug Extensions(CPUID:01 EDX:BIT2)
    // then force use of DEBUG_LOAD_IMAGE_METHOD_SOFT_INT3 
    //
    AsmCpuid (1, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & BIT2) == 0) {
      LoadImageMethod = DEBUG_LOAD_IMAGE_METHOD_SOFT_INT3;
    }
  }
  AsmReadIdtr (&IdtDescriptor);
  if (LoadImageMethod == DEBUG_LOAD_IMAGE_METHOD_SOFT_INT3) {
    if (!CheckDebugAgentHandler (&IdtDescriptor, SOFT_INT_VECTOR_NUM)) {
      //
      // Do not trigger INT3 if Debug Agent did not setup IDT entries.
      //
      return;
    }
  } else {
    if (!CheckDebugAgentHandler (&IdtDescriptor, IO_HW_BREAKPOINT_VECTOR_NUM)) {
      //
      // Save and update IDT entry for INT1
      //
      SaveAndUpdateIdtEntry1 (&IdtDescriptor, &OriginalIdtEntry);
      IdtEntryHooked = TRUE;
    }
  }
  
  //
  // Save Debug Register State
  //
  Dr0 = AsmReadDr0 ();
  Dr1 = AsmReadDr1 ();
  Dr2 = AsmReadDr2 ();
  Dr3 = AsmReadDr3 ();
  Dr7 = AsmReadDr7 () | BIT10; // H/w sets bit 10, some simulators don't
  Cr4 = AsmReadCr4 ();

  //
  // DR0 = Signature
  // DR1 = The address of the Null-terminated ASCII string for the PE/COFF image's PDB file name
  // DR2 = The pointer to the ImageContext structure
  // DR3 = IO_PORT_BREAKPOINT_ADDRESS
  // DR7 = Disables all HW breakpoints except for DR3 I/O port access of length 1 byte
  // CR4 = Make sure DE(BIT3) is set
  //
  AsmWriteDr7 (BIT10);
  AsmWriteDr0 (Signature);
  AsmWriteDr1 ((UINTN) ImageContext->PdbPointer);
  AsmWriteDr2 ((UINTN) ImageContext);
  AsmWriteDr3 (IO_PORT_BREAKPOINT_ADDRESS);

  if (LoadImageMethod == DEBUG_LOAD_IMAGE_METHOD_IO_HW_BREAKPOINT) {
    AsmWriteDr7 (0x20000480);
    AsmWriteCr4 (Cr4 | BIT3);
    //
    // Do an IN from IO_PORT_BREAKPOINT_ADDRESS to generate a HW breakpoint until the port
    // returns a read value other than DEBUG_AGENT_IMAGE_WAIT
    //
    do {
      DebugAgentStatus = IoRead8 (IO_PORT_BREAKPOINT_ADDRESS);
    } while (DebugAgentStatus == DEBUG_AGENT_IMAGE_WAIT);

  } else if (LoadImageMethod == DEBUG_LOAD_IMAGE_METHOD_SOFT_INT3) {
    //
    // Generate a software break point.
    //
    CpuBreakpoint ();
  }

  //
  // Restore Debug Register State only when Host didn't change it inside exception handler.
  // E.g.: User halts the target and sets the HW breakpoint while target is 
  //       in the above exception handler
  //
  NewDr7 = AsmReadDr7 () | BIT10; // H/w sets bit 10, some simulators don't
  if (!IsDrxEnabled (0, NewDr7) && (AsmReadDr0 () == 0 || AsmReadDr0 () == Signature)) {
    //
    // If user changed Dr3 (by setting HW bp in the above exception handler,
    // we will not set Dr0 to 0 in GO/STEP handler because the break cause is not IMAGE_LOAD/_UNLOAD.
    //
    AsmWriteDr0 (Dr0);
  }
  if (!IsDrxEnabled (1, NewDr7) && (AsmReadDr1 () == (UINTN) ImageContext->PdbPointer)) {
    AsmWriteDr1 (Dr1);
  }
  if (!IsDrxEnabled (2, NewDr7) && (AsmReadDr2 () == (UINTN) ImageContext)) {
    AsmWriteDr2 (Dr2);
  }
  if (!IsDrxEnabled (3, NewDr7) && (AsmReadDr3 () == IO_PORT_BREAKPOINT_ADDRESS)) {
    AsmWriteDr3 (Dr3);
  }
  if (LoadImageMethod == DEBUG_LOAD_IMAGE_METHOD_IO_HW_BREAKPOINT) {
    if (AsmReadCr4 () == (Cr4 | BIT3)) {
      AsmWriteCr4 (Cr4);
    }
    if (NewDr7 == 0x20000480) {
      AsmWriteDr7 (Dr7);
    }
  } else if (LoadImageMethod == DEBUG_LOAD_IMAGE_METHOD_SOFT_INT3) {
    if (NewDr7 == BIT10) {
      AsmWriteDr7 (Dr7);
    }
  }
  //
  // Restore original IDT entry for INT1 if it was hooked.
  //
  if (IdtEntryHooked) {
    RestoreIdtEntry1 (&IdtDescriptor, &OriginalIdtEntry);
  }
  //
  // Restore the interrupt state
  //
  SetInterruptState (InterruptState);
}

/**
  Performs additional actions after a PE/COFF image has been loaded and relocated.

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that has already been loaded and relocated.

**/
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  PeCoffLoaderExtraActionCommon (ImageContext, IMAGE_LOAD_SIGNATURE);
}

/**
  Performs additional actions just before a PE/COFF image is unloaded.  Any resources
  that were allocated by PeCoffLoaderRelocateImageExtraAction() must be freed.

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that is being unloaded.

**/
VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  PeCoffLoaderExtraActionCommon (ImageContext, IMAGE_UNLOAD_SIGNATURE);
}
