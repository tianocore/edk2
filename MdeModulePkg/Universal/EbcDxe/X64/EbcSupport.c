/** @file
  This module contains EBC support routines that are customized based on
  the target x64 processor.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EbcInt.h"
#include "EbcExecute.h"

//
// NOTE: This is the stack size allocated for the interpreter
//       when it executes an EBC image. The requirements can change
//       based on whether or not a debugger is present, and other
//       platform-specific configurations.
//
#define VM_STACK_SIZE   (1024 * 8)

#define STACK_REMAIN_SIZE (1024 * 4)

//
// This is instruction buffer used to create EBC thunk
//
#define EBC_ENTRYPOINT_SIGNATURE           0xAFAFAFAFAFAFAFAFull
#define EBC_LL_EBC_ENTRYPOINT_SIGNATURE    0xFAFAFAFAFAFAFAFAull
UINT8  mInstructionBufferTemplate[] = {
  //
  // Add a magic code here to help the VM recognize the thunk..
  // mov rax, 0xca112ebcca112ebc  => 48 B8 BC 2E 11 CA BC 2E 11 CA
  //
  0x48, 0xB8, 0xBC, 0x2E, 0x11, 0xCA, 0xBC, 0x2E, 0x11, 0xCA,
  //
  // Add code bytes to load up a processor register with the EBC entry point.
  //  mov r10, EbcEntryPoint  => 49 BA XX XX XX XX XX XX XX XX (To be fixed at runtime)
  // These 8 bytes of the thunk entry is the address of the EBC
  // entry point.
  //
  0x49, 0xBA, 
    (UINT8)(EBC_ENTRYPOINT_SIGNATURE & 0xFF),
    (UINT8)((EBC_ENTRYPOINT_SIGNATURE >> 8) & 0xFF),
    (UINT8)((EBC_ENTRYPOINT_SIGNATURE >> 16) & 0xFF),
    (UINT8)((EBC_ENTRYPOINT_SIGNATURE >> 24) & 0xFF),
    (UINT8)((EBC_ENTRYPOINT_SIGNATURE >> 32) & 0xFF),
    (UINT8)((EBC_ENTRYPOINT_SIGNATURE >> 40) & 0xFF),
    (UINT8)((EBC_ENTRYPOINT_SIGNATURE >> 48) & 0xFF),
    (UINT8)((EBC_ENTRYPOINT_SIGNATURE >> 56) & 0xFF),
  //
  // Stick in a load of r11 with the address of appropriate VM function.
  //  mov r11, EbcLLEbcInterpret  => 49 BB XX XX XX XX XX XX XX XX (To be fixed at runtime)
  //
  0x49, 0xBB,
    (UINT8)(EBC_LL_EBC_ENTRYPOINT_SIGNATURE & 0xFF),
    (UINT8)((EBC_LL_EBC_ENTRYPOINT_SIGNATURE >> 8) & 0xFF),
    (UINT8)((EBC_LL_EBC_ENTRYPOINT_SIGNATURE >> 16) & 0xFF),
    (UINT8)((EBC_LL_EBC_ENTRYPOINT_SIGNATURE >> 24) & 0xFF),
    (UINT8)((EBC_LL_EBC_ENTRYPOINT_SIGNATURE >> 32) & 0xFF),
    (UINT8)((EBC_LL_EBC_ENTRYPOINT_SIGNATURE >> 40) & 0xFF),
    (UINT8)((EBC_LL_EBC_ENTRYPOINT_SIGNATURE >> 48) & 0xFF),
    (UINT8)((EBC_LL_EBC_ENTRYPOINT_SIGNATURE >> 56) & 0xFF),
  //
  // Stick in jump opcode bytes
  //  jmp r11 => 41 FF E3
  //
  0x41, 0xFF, 0xE3,
};

/**
  Begin executing an EBC image.
  This is used for Ebc Thunk call.

  @return The value returned by the EBC application we're going to run.

**/
UINT64
EFIAPI
EbcLLEbcInterpret (
  VOID
  );

/**
  Begin executing an EBC image.
  This is used for Ebc image entrypoint.

  @return The value returned by the EBC application we're going to run.

**/
UINT64
EFIAPI
EbcLLExecuteEbcImageEntryPoint (
  VOID
  );

/**
  Pushes a 64 bit unsigned value to the VM stack.

  @param VmPtr  The pointer to current VM context.
  @param Arg    The value to be pushed.

**/
VOID
PushU64 (
  IN VM_CONTEXT *VmPtr,
  IN UINT64     Arg
  )
{
  //
  // Advance the VM stack down, and then copy the argument to the stack.
  // Hope it's aligned.
  //
  VmPtr->Gpr[0] -= sizeof (UINT64);
  *(UINT64 *) VmPtr->Gpr[0] = Arg;
  return;
}


/**
  Begin executing an EBC image.

  This is a thunk function. Microsoft x64 compiler only provide fast_call
  calling convention, so the first four arguments are passed by rcx, rdx,
  r8, and r9, while other arguments are passed in stack.

  @param  EntryPoint            The entrypoint of EBC code.
  @param  Arg1                  The 1st argument.
  @param  Arg2                  The 2nd argument.
  @param  Arg3                  The 3rd argument.
  @param  Arg4                  The 4th argument.
  @param  Arg5                  The 5th argument.
  @param  Arg6                  The 6th argument.
  @param  Arg7                  The 7th argument.
  @param  Arg8                  The 8th argument.
  @param  Arg9                  The 9th argument.
  @param  Arg10                 The 10th argument.
  @param  Arg11                 The 11th argument.
  @param  Arg12                 The 12th argument.
  @param  Arg13                 The 13th argument.
  @param  Arg14                 The 14th argument.
  @param  Arg15                 The 15th argument.
  @param  Arg16                 The 16th argument.

  @return The value returned by the EBC application we're going to run.

**/
UINT64
EFIAPI
EbcInterpret (
  IN UINTN      EntryPoint,
  IN UINTN      Arg1,
  IN UINTN      Arg2,
  IN UINTN      Arg3,
  IN UINTN      Arg4,
  IN UINTN      Arg5,
  IN UINTN      Arg6,
  IN UINTN      Arg7,
  IN UINTN      Arg8,
  IN UINTN      Arg9,
  IN UINTN      Arg10,
  IN UINTN      Arg11,
  IN UINTN      Arg12,
  IN UINTN      Arg13,
  IN UINTN      Arg14,
  IN UINTN      Arg15,
  IN UINTN      Arg16
  )
{
  //
  // Create a new VM context on the stack
  //
  VM_CONTEXT  VmContext;
  UINTN       Addr;
  EFI_STATUS  Status;
  UINTN       StackIndex;

  //
  // Get the EBC entry point
  //
  Addr = EntryPoint;

  //
  // Now clear out our context
  //
  ZeroMem ((VOID *) &VmContext, sizeof (VM_CONTEXT));

  //
  // Set the VM instruction pointer to the correct location in memory.
  //
  VmContext.Ip = (VMIP) Addr;

  //
  // Initialize the stack pointer for the EBC. Get the current system stack
  // pointer and adjust it down by the max needed for the interpreter.
  //

  //
  // Adjust the VM's stack pointer down.
  //

  Status = GetEBCStack((EFI_HANDLE)(UINTN)-1, &VmContext.StackPool, &StackIndex);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  VmContext.StackTop = (UINT8*)VmContext.StackPool + (STACK_REMAIN_SIZE);
  VmContext.Gpr[0] = (UINT64) ((UINT8*)VmContext.StackPool + STACK_POOL_SIZE);
  VmContext.HighStackBottom = (UINTN) VmContext.Gpr[0];
  VmContext.Gpr[0] -= sizeof (UINTN);

  //
  // Align the stack on a natural boundary.
  //
  VmContext.Gpr[0] &= ~(VM_REGISTER)(sizeof (UINTN) - 1);

  //
  // Put a magic value in the stack gap, then adjust down again.
  //
  *(UINTN *) (UINTN) (VmContext.Gpr[0]) = (UINTN) VM_STACK_KEY_VALUE;
  VmContext.StackMagicPtr             = (UINTN *) (UINTN) VmContext.Gpr[0];

  //
  // The stack upper to LowStackTop is belong to the VM.
  //
  VmContext.LowStackTop   = (UINTN) VmContext.Gpr[0];

  //
  // For the worst case, assume there are 4 arguments passed in registers, store
  // them to VM's stack.
  //
  PushU64 (&VmContext, (UINT64) Arg16);
  PushU64 (&VmContext, (UINT64) Arg15);
  PushU64 (&VmContext, (UINT64) Arg14);
  PushU64 (&VmContext, (UINT64) Arg13);
  PushU64 (&VmContext, (UINT64) Arg12);
  PushU64 (&VmContext, (UINT64) Arg11);
  PushU64 (&VmContext, (UINT64) Arg10);
  PushU64 (&VmContext, (UINT64) Arg9);
  PushU64 (&VmContext, (UINT64) Arg8);
  PushU64 (&VmContext, (UINT64) Arg7);
  PushU64 (&VmContext, (UINT64) Arg6);
  PushU64 (&VmContext, (UINT64) Arg5);
  PushU64 (&VmContext, (UINT64) Arg4);
  PushU64 (&VmContext, (UINT64) Arg3);
  PushU64 (&VmContext, (UINT64) Arg2);
  PushU64 (&VmContext, (UINT64) Arg1);

  //
  // Interpreter assumes 64-bit return address is pushed on the stack.
  // The x64 does not do this so pad the stack accordingly.
  //
  PushU64 (&VmContext, (UINT64) 0);
  PushU64 (&VmContext, (UINT64) 0x1234567887654321ULL);

  //
  // For x64, this is where we say our return address is
  //
  VmContext.StackRetAddr  = (UINT64) VmContext.Gpr[0];

  //
  // We need to keep track of where the EBC stack starts. This way, if the EBC
  // accesses any stack variables above its initial stack setting, then we know
  // it's accessing variables passed into it, which means the data is on the
  // VM's stack.
  // When we're called, on the stack (high to low) we have the parameters, the
  // return address, then the saved ebp. Save the pointer to the return address.
  // EBC code knows that's there, so should look above it for function parameters.
  // The offset is the size of locals (VMContext + Addr + saved ebp).
  // Note that the interpreter assumes there is a 16 bytes of return address on
  // the stack too, so adjust accordingly.
  //  VmContext.HighStackBottom = (UINTN)(Addr + sizeof (VmContext) + sizeof (Addr));
  //

  //
  // Begin executing the EBC code
  //
  EbcExecute (&VmContext);

  //
  // Return the value in R[7] unless there was an error
  //
  ReturnEBCStack(StackIndex);
  return (UINT64) VmContext.Gpr[7];
}


/**
  Begin executing an EBC image.

  @param  EntryPoint       The entrypoint of EBC code.
  @param  ImageHandle      image handle for the EBC application we're executing
  @param  SystemTable      standard system table passed into an driver's entry
                           point

  @return The value returned by the EBC application we're going to run.

**/
UINT64
EFIAPI
ExecuteEbcImageEntryPoint (
  IN UINTN                EntryPoint,
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  //
  // Create a new VM context on the stack
  //
  VM_CONTEXT  VmContext;
  UINTN       Addr;
  EFI_STATUS  Status;
  UINTN       StackIndex;

  //
  // Get the EBC entry point
  //
  Addr = EntryPoint;

  //
  // Now clear out our context
  //
  ZeroMem ((VOID *) &VmContext, sizeof (VM_CONTEXT));

  //
  // Save the image handle so we can track the thunks created for this image
  //
  VmContext.ImageHandle = ImageHandle;
  VmContext.SystemTable = SystemTable;

  //
  // Set the VM instruction pointer to the correct location in memory.
  //
  VmContext.Ip = (VMIP) Addr;

  //
  // Initialize the stack pointer for the EBC. Get the current system stack
  // pointer and adjust it down by the max needed for the interpreter.
  //

  Status = GetEBCStack(ImageHandle, &VmContext.StackPool, &StackIndex);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  VmContext.StackTop = (UINT8*)VmContext.StackPool + (STACK_REMAIN_SIZE);
  VmContext.Gpr[0] = (UINT64) ((UINT8*)VmContext.StackPool + STACK_POOL_SIZE);
  VmContext.HighStackBottom = (UINTN) VmContext.Gpr[0];
  VmContext.Gpr[0] -= sizeof (UINTN);


  //
  // Put a magic value in the stack gap, then adjust down again
  //
  *(UINTN *) (UINTN) (VmContext.Gpr[0]) = (UINTN) VM_STACK_KEY_VALUE;
  VmContext.StackMagicPtr             = (UINTN *) (UINTN) VmContext.Gpr[0];

  //
  // Align the stack on a natural boundary
  VmContext.Gpr[0] &= ~(VM_REGISTER)(sizeof(UINTN) - 1);
  //
  VmContext.LowStackTop   = (UINTN) VmContext.Gpr[0];

  //
  // Simply copy the image handle and system table onto the EBC stack.
  // Greatly simplifies things by not having to spill the args.
  //
  PushU64 (&VmContext, (UINT64) SystemTable);
  PushU64 (&VmContext, (UINT64) ImageHandle);

  //
  // VM pushes 16-bytes for return address. Simulate that here.
  //
  PushU64 (&VmContext, (UINT64) 0);
  PushU64 (&VmContext, (UINT64) 0x1234567887654321ULL);

  //
  // For x64, this is where we say our return address is
  //
  VmContext.StackRetAddr  = (UINT64) VmContext.Gpr[0];

  //
  // Entry function needn't access high stack context, simply
  // put the stack pointer here.
  //

  //
  // Begin executing the EBC code
  //
  EbcExecute (&VmContext);

  //
  // Return the value in R[7] unless there was an error
  //
  ReturnEBCStack(StackIndex);
  return (UINT64) VmContext.Gpr[7];
}


/**
  Create thunks for an EBC image entry point, or an EBC protocol service.

  @param  ImageHandle           Image handle for the EBC image. If not null, then
                                we're creating a thunk for an image entry point.
  @param  EbcEntryPoint         Address of the EBC code that the thunk is to call
  @param  Thunk                 Returned thunk we create here
  @param  Flags                 Flags indicating options for creating the thunk

  @retval EFI_SUCCESS           The thunk was created successfully.
  @retval EFI_INVALID_PARAMETER The parameter of EbcEntryPoint is not 16-bit
                                aligned.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory to created the EBC
                                Thunk.
  @retval EFI_BUFFER_TOO_SMALL  EBC_THUNK_SIZE is not larger enough.

**/
EFI_STATUS
EbcCreateThunks (
  IN EFI_HANDLE           ImageHandle,
  IN VOID                 *EbcEntryPoint,
  OUT VOID                **Thunk,
  IN  UINT32              Flags
  )
{
  UINT8       *Ptr;
  UINT8       *ThunkBase;
  UINT32      Index;
  INT32       ThunkSize;

  //
  // Check alignment of pointer to EBC code
  //
  if ((UINT32) (UINTN) EbcEntryPoint & 0x01) {
    return EFI_INVALID_PARAMETER;
  }

  ThunkSize = sizeof(mInstructionBufferTemplate);

  Ptr = AllocatePool (sizeof(mInstructionBufferTemplate));

  if (Ptr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  //  Print(L"Allocate TH: 0x%X\n", (UINT32)Ptr);
  //
  // Save the start address so we can add a pointer to it to a list later.
  //
  ThunkBase = Ptr;

  //
  // Give them the address of our buffer we're going to fix up
  //
  *Thunk = (VOID *) Ptr;

  //
  // Copy whole thunk instruction buffer template
  //
  CopyMem (Ptr, mInstructionBufferTemplate, sizeof(mInstructionBufferTemplate));

  //
  // Patch EbcEntryPoint and EbcLLEbcInterpret
  //
  for (Index = 0; Index < sizeof(mInstructionBufferTemplate) - sizeof(UINTN); Index++) {
    if (*(UINTN *)&Ptr[Index] == EBC_ENTRYPOINT_SIGNATURE) {
      *(UINTN *)&Ptr[Index] = (UINTN)EbcEntryPoint;
    }
    if (*(UINTN *)&Ptr[Index] == EBC_LL_EBC_ENTRYPOINT_SIGNATURE) {
      if ((Flags & FLAG_THUNK_ENTRY_POINT) != 0) {
        *(UINTN *)&Ptr[Index] = (UINTN)EbcLLExecuteEbcImageEntryPoint;
      } else {
        *(UINTN *)&Ptr[Index] = (UINTN)EbcLLEbcInterpret;
      }
    }
  }

  //
  // Add the thunk to the list for this image. Do this last since the add
  // function flushes the cache for us.
  //
  EbcAddImageThunk (ImageHandle, (VOID *) ThunkBase, ThunkSize);

  return EFI_SUCCESS;
}


/**
  This function is called to execute an EBC CALLEX instruction.
  The function check the callee's content to see whether it is common native
  code or a thunk to another piece of EBC code.
  If the callee is common native code, use EbcLLCAllEXASM to manipulate,
  otherwise, set the VM->IP to target EBC code directly to avoid another VM
  be startup which cost time and stack space.

  @param  VmPtr            Pointer to a VM context.
  @param  FuncAddr         Callee's address
  @param  NewStackPointer  New stack pointer after the call
  @param  FramePtr         New frame pointer after the call
  @param  Size             The size of call instruction

**/
VOID
EbcLLCALLEX (
  IN VM_CONTEXT   *VmPtr,
  IN UINTN        FuncAddr,
  IN UINTN        NewStackPointer,
  IN VOID         *FramePtr,
  IN UINT8        Size
  )
{
  UINTN    IsThunk;
  UINTN    TargetEbcAddr;
  UINT8    InstructionBuffer[sizeof(mInstructionBufferTemplate)];
  UINTN    Index;
  UINTN    IndexOfEbcEntrypoint;

  IsThunk       = 1;
  TargetEbcAddr = 0;
  IndexOfEbcEntrypoint = 0;

  //
  // Processor specific code to check whether the callee is a thunk to EBC.
  //
  CopyMem (InstructionBuffer, (VOID *)FuncAddr, sizeof(InstructionBuffer));
  //
  // Fill the signature according to mInstructionBufferTemplate
  //
  for (Index = 0; Index < sizeof(mInstructionBufferTemplate) - sizeof(UINTN); Index++) {
    if (*(UINTN *)&mInstructionBufferTemplate[Index] == EBC_ENTRYPOINT_SIGNATURE) {
      *(UINTN *)&InstructionBuffer[Index] = EBC_ENTRYPOINT_SIGNATURE;
      IndexOfEbcEntrypoint = Index;
    }
    if (*(UINTN *)&mInstructionBufferTemplate[Index] == EBC_LL_EBC_ENTRYPOINT_SIGNATURE) {
      *(UINTN *)&InstructionBuffer[Index] = EBC_LL_EBC_ENTRYPOINT_SIGNATURE;
    }
  }
  //
  // Check if we need thunk to native
  //
  if (CompareMem (InstructionBuffer, mInstructionBufferTemplate, sizeof(mInstructionBufferTemplate)) != 0) {
    IsThunk = 0;
  }

  if (IsThunk == 1){
    //
    // The callee is a thunk to EBC, adjust the stack pointer down 16 bytes and
    // put our return address and frame pointer on the VM stack.
    // Then set the VM's IP to new EBC code.
    //
    VmPtr->Gpr[0] -= 8;
    VmWriteMemN (VmPtr, (UINTN) VmPtr->Gpr[0], (UINTN) FramePtr);
    VmPtr->FramePtr = (VOID *) (UINTN) VmPtr->Gpr[0];
    VmPtr->Gpr[0] -= 8;
    VmWriteMem64 (VmPtr, (UINTN) VmPtr->Gpr[0], (UINT64) (UINTN) (VmPtr->Ip + Size));

    CopyMem (&TargetEbcAddr, (UINT8 *)FuncAddr + IndexOfEbcEntrypoint, sizeof(UINTN));
    VmPtr->Ip = (VMIP) (UINTN) TargetEbcAddr;
  } else {
    //
    // The callee is not a thunk to EBC, call native code,
    // and get return value.
    //
    VmPtr->Gpr[7] = EbcLLCALLEXNative (FuncAddr, NewStackPointer, FramePtr);

    //
    // Advance the IP.
    //
    VmPtr->Ip += Size;
  }
}

