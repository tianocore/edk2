/** @file
  This module contains EBC support routines that are customized based on
  the target AArch64 processor.

Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>
Copyright (c) 2015, The Linux Foundation. All rights reserved.<BR>
Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EbcInt.h"
#include "EbcExecute.h"
#include "EbcDebuggerHook.h"

//
// Amount of space that is not used in the stack
//
#define STACK_REMAIN_SIZE (1024 * 4)

#pragma pack(1)
typedef struct {
  UINT32    Instr[3];
  UINT32    Magic;
  UINT64    EbcEntryPoint;
  UINT64    EbcLlEntryPoint;
} EBC_INSTRUCTION_BUFFER;
#pragma pack()

extern CONST EBC_INSTRUCTION_BUFFER       mEbcInstructionBufferTemplate;

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

  This is a thunk function.

  @param  Arg1                  The 1st argument.
  @param  Arg2                  The 2nd argument.
  @param  Arg3                  The 3rd argument.
  @param  Arg4                  The 4th argument.
  @param  Arg5                  The 5th argument.
  @param  Arg6                  The 6th argument.
  @param  Arg7                  The 7th argument.
  @param  Arg8                  The 8th argument.
  @param  EntryPoint            The entrypoint of EBC code.
  @param  Args9_16[]            Array containing arguments #9 to #16.

  @return The value returned by the EBC application we're going to run.

**/
UINT64
EFIAPI
EbcInterpret (
  IN UINTN        Arg1,
  IN UINTN        Arg2,
  IN UINTN        Arg3,
  IN UINTN        Arg4,
  IN UINTN        Arg5,
  IN UINTN        Arg6,
  IN UINTN        Arg7,
  IN UINTN        Arg8,
  IN UINTN        EntryPoint,
  IN CONST UINTN  Args9_16[]
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
  PushU64 (&VmContext, (UINT64) Args9_16[7]);
  PushU64 (&VmContext, (UINT64) Args9_16[6]);
  PushU64 (&VmContext, (UINT64) Args9_16[5]);
  PushU64 (&VmContext, (UINT64) Args9_16[4]);
  PushU64 (&VmContext, (UINT64) Args9_16[3]);
  PushU64 (&VmContext, (UINT64) Args9_16[2]);
  PushU64 (&VmContext, (UINT64) Args9_16[1]);
  PushU64 (&VmContext, (UINT64) Args9_16[0]);
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
  // AArch64 does not do this so pad the stack accordingly.
  //
  PushU64 (&VmContext, (UINT64) 0);
  PushU64 (&VmContext, (UINT64) 0x1234567887654321ULL);

  //
  // For AArch64, this is where we say our return address is
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
  EbcDebuggerHookEbcInterpret (&VmContext);
  EbcExecute (&VmContext);

  //
  // Return the value in R[7] unless there was an error
  //
  ReturnEBCStack(StackIndex);
  return (UINT64) VmContext.Gpr[7];
}


/**
  Begin executing an EBC image.

  @param  ImageHandle      image handle for the EBC application we're executing
  @param  SystemTable      standard system table passed into an driver's entry
                           point
  @param  EntryPoint       The entrypoint of EBC code.

  @return The value returned by the EBC application we're going to run.

**/
UINT64
EFIAPI
ExecuteEbcImageEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable,
  IN UINTN                EntryPoint
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
  // For AArch64, this is where we say our return address is
  //
  VmContext.StackRetAddr  = (UINT64) VmContext.Gpr[0];

  //
  // Entry function needn't access high stack context, simply
  // put the stack pointer here.
  //

  //
  // Begin executing the EBC code
  //
  EbcDebuggerHookExecuteEbcImageEntryPoint (&VmContext);
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
  EBC_INSTRUCTION_BUFFER       *InstructionBuffer;

  //
  // Check alignment of pointer to EBC code
  //
  if ((UINT32) (UINTN) EbcEntryPoint & 0x01) {
    return EFI_INVALID_PARAMETER;
  }

  InstructionBuffer = EbcAllocatePoolForThunk (sizeof (EBC_INSTRUCTION_BUFFER));
  if (InstructionBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Give them the address of our buffer we're going to fix up
  //
  *Thunk = InstructionBuffer;

  //
  // Copy whole thunk instruction buffer template
  //
  CopyMem (InstructionBuffer, &mEbcInstructionBufferTemplate,
    sizeof (EBC_INSTRUCTION_BUFFER));

  //
  // Patch EbcEntryPoint and EbcLLEbcInterpret
  //
  InstructionBuffer->EbcEntryPoint = (UINT64)EbcEntryPoint;
  if ((Flags & FLAG_THUNK_ENTRY_POINT) != 0) {
    InstructionBuffer->EbcLlEntryPoint = (UINT64)EbcLLExecuteEbcImageEntryPoint;
  } else {
    InstructionBuffer->EbcLlEntryPoint = (UINT64)EbcLLEbcInterpret;
  }

  //
  // Add the thunk to the list for this image. Do this last since the add
  // function flushes the cache for us.
  //
  EbcAddImageThunk (ImageHandle, InstructionBuffer,
    sizeof (EBC_INSTRUCTION_BUFFER));

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
  CONST EBC_INSTRUCTION_BUFFER *InstructionBuffer;

  //
  // Processor specific code to check whether the callee is a thunk to EBC.
  //
  InstructionBuffer = (EBC_INSTRUCTION_BUFFER *)FuncAddr;

  if (CompareMem (InstructionBuffer, &mEbcInstructionBufferTemplate,
        sizeof(EBC_INSTRUCTION_BUFFER) - 2 * sizeof (UINT64)) == 0) {
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

    VmPtr->Ip = (VMIP) InstructionBuffer->EbcEntryPoint;
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

