/** @file
  This module contains EBC support routines that are customized based on
  the target processor.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
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
#define VM_STACK_SIZE   (1024 * 4)
#define EBC_THUNK_SIZE  32

#define STACK_REMAIN_SIZE (1024 * 4)


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

  IsThunk       = 1;
  TargetEbcAddr = 0;

  //
  // Processor specific code to check whether the callee is a thunk to EBC.
  //
  if (*((UINT8 *)FuncAddr) != 0xB8) {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 1) != 0xBC)  {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 2) != 0x2E)  {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 3) != 0x11)  {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 4) != 0xCA)  {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 5) != 0xB8)  {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 10) != 0xB9)  {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 15) != 0xFF)  {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT8 *)FuncAddr + 16) != 0xE1)  {
    IsThunk = 0;
    goto Action;
  }

  TargetEbcAddr = ((UINTN)(*((UINT8 *)FuncAddr + 9)) << 24) + ((UINTN)(*((UINT8 *)FuncAddr + 8)) << 16) +
                    ((UINTN)(*((UINT8 *)FuncAddr + 7)) << 8) + ((UINTN)(*((UINT8 *)FuncAddr + 6)));

Action:
  if (IsThunk == 1){
    //
    // The callee is a thunk to EBC, adjust the stack pointer down 16 bytes and
    // put our return address and frame pointer on the VM stack.
    // Then set the VM's IP to new EBC code.
    //
    VmPtr->R[0] -= 8;
    VmWriteMemN (VmPtr, (UINTN) VmPtr->R[0], (UINTN) FramePtr);
    VmPtr->FramePtr = (VOID *) (UINTN) VmPtr->R[0];
    VmPtr->R[0] -= 8;
    VmWriteMem64 (VmPtr, (UINTN) VmPtr->R[0], (UINT64) (UINTN) (VmPtr->Ip + Size));

    VmPtr->Ip = (VMIP) (UINTN) TargetEbcAddr;
  } else {
    //
    // The callee is not a thunk to EBC, call native code.
    //
    EbcLLCALLEXNative (FuncAddr, NewStackPointer, FramePtr);

    //
    // Get return value and advance the IP.
    //
    VmPtr->R[7] = EbcLLGetReturnValue ();
    VmPtr->Ip += Size;
  }
}


/**
  Begin executing an EBC image. The address of the entry point is passed
  in via a processor register, so we'll need to make a call to get the
  value.

  This is a thunk function. Microsoft x64 compiler only provide fast_call
  calling convention, so the first four arguments are passed by rcx, rdx,
  r8, and r9, while other arguments are passed in stack.

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
EbcInterpret (
  IN OUT UINTN      Arg1,
  IN OUT UINTN      Arg2,
  IN OUT UINTN      Arg3,
  IN OUT UINTN      Arg4,
  IN OUT UINTN      Arg5,
  IN OUT UINTN      Arg6,
  IN OUT UINTN      Arg7,
  IN OUT UINTN      Arg8,
  IN OUT UINTN      Arg9,
  IN OUT UINTN      Arg10,
  IN OUT UINTN      Arg11,
  IN OUT UINTN      Arg12,
  IN OUT UINTN      Arg13,
  IN OUT UINTN      Arg14,
  IN OUT UINTN      Arg15,
  IN OUT UINTN      Arg16
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
  // Get the EBC entry point from the processor register.
  //
  Addr = EbcLLGetEbcEntryPoint ();

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
  // Align the stack on a natural boundary
  //

  //
  // Allocate stack pool
  //
  Status = GetEBCStack((EFI_HANDLE)-1, &VmContext.StackPool, &StackIndex);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  VmContext.StackTop = (UINT8*)VmContext.StackPool + (STACK_REMAIN_SIZE);
  VmContext.R[0] = (UINT64)(UINTN) ((UINT8*)VmContext.StackPool + STACK_POOL_SIZE);
  VmContext.HighStackBottom = (UINTN)VmContext.R[0];
  VmContext.R[0] &= ~(sizeof (UINTN) - 1);
  VmContext.R[0] -= sizeof (UINTN);

  //
  // Put a magic value in the stack gap, then adjust down again
  //
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) VM_STACK_KEY_VALUE;
  VmContext.StackMagicPtr             = (UINTN *) (UINTN) VmContext.R[0];
  VmContext.LowStackTop   = (UINTN) VmContext.R[0];

  //
  // For IA32, this is where we say our return address is
  //
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg16;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg15;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg14;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg13;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg12;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg11;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg10;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg9;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg8;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg7;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg6;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg5;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg4;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg3;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg2;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) Arg1;
  VmContext.R[0] -= 16;
  VmContext.StackRetAddr  = (UINT64) VmContext.R[0];

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
  return (UINT64) VmContext.R[7];
}


/**
  Begin executing an EBC image. The address of the entry point is passed
  in via a processor register, so we'll need to make a call to get the
  value.

  @param  ImageHandle      image handle for the EBC application we're executing
  @param  SystemTable      standard system table passed into an driver's entry
                           point

  @return The value returned by the EBC application we're going to run.

**/
UINT64
ExecuteEbcImageEntryPoint (
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
  // Get the EBC entry point from the processor register. Make sure you don't
  // call any functions before this or you could mess up the register the
  // entry point is passed in.
  //
  Addr = EbcLLGetEbcEntryPoint ();

  //
  //  Print(L"*** Thunked into EBC entry point - ImageHandle = 0x%X\n", (UINTN)ImageHandle);
  //  Print(L"EBC entry point is 0x%X\n", (UINT32)(UINTN)Addr);
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

  //
  // Allocate stack pool
  //
  Status = GetEBCStack(ImageHandle, &VmContext.StackPool, &StackIndex);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  VmContext.StackTop = (UINT8*)VmContext.StackPool + (STACK_REMAIN_SIZE);
  VmContext.R[0] = (UINT64)(UINTN) ((UINT8*)VmContext.StackPool + STACK_POOL_SIZE);
  VmContext.HighStackBottom = (UINTN)VmContext.R[0];
  VmContext.R[0] -= sizeof (UINTN);

  //
  // Put a magic value in the stack gap, then adjust down again
  //
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) VM_STACK_KEY_VALUE;
  VmContext.StackMagicPtr             = (UINTN *) (UINTN) VmContext.R[0];

  //
  // Align the stack on a natural boundary
  //  VmContext.R[0] &= ~(sizeof(UINTN) - 1);
  //
  VmContext.LowStackTop   = (UINTN) VmContext.R[0];
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) SystemTable;
  VmContext.R[0] -= sizeof (UINTN);
  *(UINTN *) (UINTN) (VmContext.R[0]) = (UINTN) ImageHandle;

  VmContext.R[0] -= 16;
  VmContext.StackRetAddr  = (UINT64) VmContext.R[0];
  //
  // VM pushes 16-bytes for return address. Simulate that here.
  //

  //
  // Begin executing the EBC code
  //
  EbcExecute (&VmContext);

  //
  // Return the value in R[7] unless there was an error
  //
  return (UINT64) VmContext.R[7];
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
  UINT32      Addr;
  INT32       Size;
  INT32       ThunkSize;

  //
  // Check alignment of pointer to EBC code
  //
  if ((UINT32) (UINTN) EbcEntryPoint & 0x01) {
    return EFI_INVALID_PARAMETER;
  }

  Size      = EBC_THUNK_SIZE;
  ThunkSize = Size;

  Ptr = AllocatePool (Size);

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
  // Add a magic code here to help the VM recognize the thunk..
  // mov eax, 0xca112ebc  => B8 BC 2E 11 CA
  //
  *Ptr = 0xB8;
  Ptr++;
  Size--;
  Addr = (UINT32) 0xCA112EBC;
  for (Index = 0; Index < sizeof (Addr); Index++) {
    *Ptr = (UINT8) (UINTN) Addr;
    Addr >>= 8;
    Ptr++;
    Size--;
  }

  //
  // Add code bytes to load up a processor register with the EBC entry point.
  // mov eax, 0xaa55aa55  => B8 55 AA 55 AA
  // The first 8 bytes of the thunk entry is the address of the EBC
  // entry point.
  //
  *Ptr = 0xB8;
  Ptr++;
  Size--;
  Addr = (UINT32) EbcEntryPoint;
  for (Index = 0; Index < sizeof (Addr); Index++) {
    *Ptr = (UINT8) (UINTN) Addr;
    Addr >>= 8;
    Ptr++;
    Size--;
  }
  //
  // Stick in a load of ecx with the address of appropriate VM function.
  //  mov ecx 12345678h  => 0xB9 0x78 0x56 0x34 0x12
  //
  if ((Flags & FLAG_THUNK_ENTRY_POINT) != 0) {
    Addr = (UINT32) (UINTN) ExecuteEbcImageEntryPoint;
  } else {
    Addr = (UINT32) (UINTN) EbcInterpret;
  }

  //
  // MOV ecx
  //
  *Ptr = 0xB9;
  Ptr++;
  Size--;
  for (Index = 0; Index < sizeof (Addr); Index++) {
    *Ptr = (UINT8) Addr;
    Addr >>= 8;
    Ptr++;
    Size--;
  }
  //
  // Stick in jump opcode bytes for jmp ecx => 0xFF 0xE1
  //
  *Ptr = 0xFF;
  Ptr++;
  Size--;
  *Ptr = 0xE1;
  Size--;

  //
  // Double check that our defined size is ok (application error)
  //
  if (Size < 0) {
    ASSERT (FALSE);
    return EFI_BUFFER_TOO_SMALL;
  }
  //
  // Add the thunk to the list for this image. Do this last since the add
  // function flushes the cache for us.
  //
  EbcAddImageThunk (ImageHandle, (VOID *) ThunkBase, ThunkSize);

  return EFI_SUCCESS;
}
