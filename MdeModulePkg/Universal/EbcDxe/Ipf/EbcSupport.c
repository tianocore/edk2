/** @file
  This module contains EBC support routines that are customized based on
  the target processor.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EbcInt.h"
#include "EbcExecute.h"
#include "EbcSupport.h"
#include "EbcDebuggerHook.h"

/**
  Given raw bytes of Itanium based code, format them into a bundle and
  write them out.

  @param  MemPtr                 pointer to memory location to write the bundles
                                 to.
  @param  Template               5-bit template.
  @param  Slot0                  Instruction slot 0 data for the bundle.
  @param  Slot1                  Instruction slot 1 data for the bundle.
  @param  Slot2                  Instruction slot 2 data for the bundle.

  @retval EFI_INVALID_PARAMETER  Pointer is not aligned
  @retval EFI_INVALID_PARAMETER  No more than 5 bits in template
  @retval EFI_INVALID_PARAMETER  More than 41 bits used in code
  @retval EFI_SUCCESS            All data is written.

**/
EFI_STATUS
WriteBundle (
  IN    VOID    *MemPtr,
  IN    UINT8   Template,
  IN    UINT64  Slot0,
  IN    UINT64  Slot1,
  IN    UINT64  Slot2
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
}

/**
  Begin executing an EBC image. The address of the entry point is passed
  in via a processor register, so we'll need to make a call to get the
  value.

  This is a thunk function. Microsoft x64 compiler only provide fast_call
  calling convention, so the first four arguments are passed by rcx, rdx,
  r8, and r9, while other arguments are passed in stack.

  @param  Arg1                  The 1st argument.
  @param  ...                   The variable arguments list.

  @return The value returned by the EBC application we're going to run.

**/
UINT64
EFIAPI
EbcInterpret (
  UINT64      Arg1,
  ...
  )
{
  //
  // Create a new VM context on the stack
  //
  VM_CONTEXT  VmContext;
  UINTN       Addr;
  EFI_STATUS  Status;
  UINTN       StackIndex;
  VA_LIST     List;
  UINT64      Arg2;
  UINT64      Arg3;
  UINT64      Arg4;
  UINT64      Arg5;
  UINT64      Arg6;
  UINT64      Arg7;
  UINT64      Arg8;
  UINT64      Arg9;
  UINT64      Arg10;
  UINT64      Arg11;
  UINT64      Arg12;
  UINT64      Arg13;
  UINT64      Arg14;
  UINT64      Arg15;
  UINT64      Arg16;
  //
  // Get the EBC entry point from the processor register. Make sure you don't
  // call any functions before this or you could mess up the register the
  // entry point is passed in.
  //
  Addr = EbcLLGetEbcEntryPoint ();
  //
  // Need the args off the stack.
  //
  VA_START (List, Arg1);
  Arg2      = VA_ARG (List, UINT64);
  Arg3      = VA_ARG (List, UINT64);
  Arg4      = VA_ARG (List, UINT64);
  Arg5      = VA_ARG (List, UINT64);
  Arg6      = VA_ARG (List, UINT64);
  Arg7      = VA_ARG (List, UINT64);
  Arg8      = VA_ARG (List, UINT64);
  Arg9      = VA_ARG (List, UINT64);
  Arg10     = VA_ARG (List, UINT64);
  Arg11     = VA_ARG (List, UINT64);
  Arg12     = VA_ARG (List, UINT64);
  Arg13     = VA_ARG (List, UINT64);
  Arg14     = VA_ARG (List, UINT64);
  Arg15     = VA_ARG (List, UINT64);
  Arg16     = VA_ARG (List, UINT64);
  VA_END (List);
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
  // NOTE: Eventually we should have the interpreter allocate memory
  //       for stack space which it will use during its execution. This
  //       would likely improve performance because the interpreter would
  //       no longer be required to test each memory access and adjust
  //       those reading from the stack gap.
  //
  // For IPF, the stack looks like (assuming 10 args passed)
  //   arg10
  //   arg9       (Bottom of high stack)
  //   [ stack gap for interpreter execution ]
  //   [ magic value for detection of stack corruption ]
  //   arg8       (Top of low stack)
  //   arg7....
  //   arg1
  //   [ 64-bit return address ]
  //   [ ebc stack ]
  // If the EBC accesses memory in the stack gap, then we assume that it's
  // actually trying to access args9 and greater. Therefore we need to
  // adjust memory accesses in this region to point above the stack gap.
  //
  //
  // Now adjust the EBC stack pointer down to leave a gap for interpreter
  // execution. Then stuff a magic value there.
  //

  Status = GetEBCStack((EFI_HANDLE)(UINTN)-1, &VmContext.StackPool, &StackIndex);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  VmContext.StackTop = (UINT8*)VmContext.StackPool + (STACK_REMAIN_SIZE);
  VmContext.Gpr[0] = (UINT64) ((UINT8*)VmContext.StackPool + STACK_POOL_SIZE);
  VmContext.HighStackBottom = (UINTN) VmContext.Gpr[0];
  VmContext.Gpr[0] -= sizeof (UINTN);


  PushU64 (&VmContext, (UINT64) VM_STACK_KEY_VALUE);
  VmContext.StackMagicPtr = (UINTN *) VmContext.Gpr[0];
  VmContext.LowStackTop   = (UINTN) VmContext.Gpr[0];
  //
  // Push the EBC arguments on the stack. Does not matter that they may not
  // all be valid.
  //
  PushU64 (&VmContext, Arg16);
  PushU64 (&VmContext, Arg15);
  PushU64 (&VmContext, Arg14);
  PushU64 (&VmContext, Arg13);
  PushU64 (&VmContext, Arg12);
  PushU64 (&VmContext, Arg11);
  PushU64 (&VmContext, Arg10);
  PushU64 (&VmContext, Arg9);
  PushU64 (&VmContext, Arg8);
  PushU64 (&VmContext, Arg7);
  PushU64 (&VmContext, Arg6);
  PushU64 (&VmContext, Arg5);
  PushU64 (&VmContext, Arg4);
  PushU64 (&VmContext, Arg3);
  PushU64 (&VmContext, Arg2);
  PushU64 (&VmContext, Arg1);
  //
  // Push a bogus return address on the EBC stack because the
  // interpreter expects one there. For stack alignment purposes on IPF,
  // EBC return addresses are always 16 bytes. Push a bogus value as well.
  //
  PushU64 (&VmContext, 0);
  PushU64 (&VmContext, 0xDEADBEEFDEADBEEF);
  VmContext.StackRetAddr = (UINT64) VmContext.Gpr[0];

  //
  // Begin executing the EBC code
  //
  EbcDebuggerHookEbcInterpret (&VmContext);
  EbcExecute (&VmContext);

  //
  // Return the value in Gpr[7] unless there was an error
  //
  ReturnEBCStack(StackIndex);
  return (UINT64) VmContext.Gpr[7];
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
EFIAPI
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
  // Get the stack pointer. This is the bottom of the upper stack.
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
  // Allocate stack space for the interpreter. Then put a magic value
  // at the bottom so we can detect stack corruption.
  //
  PushU64 (&VmContext, (UINT64) VM_STACK_KEY_VALUE);
  VmContext.StackMagicPtr = (UINTN *) (UINTN) VmContext.Gpr[0];

  //
  // When we thunk to external native code, we copy the last 8 qwords from
  // the EBC stack into the processor registers, and adjust the stack pointer
  // up. If the caller is not passing 8 parameters, then we've moved the
  // stack pointer up into the stack gap. If this happens, then the caller
  // can mess up the stack gap contents (in particular our magic value).
  // Therefore, leave another gap below the magic value. Pick 10 qwords down,
  // just as a starting point.
  //
  VmContext.Gpr[0] -= 10 * sizeof (UINT64);

  //
  // Align the stack pointer such that after pushing the system table,
  // image handle, and return address on the stack, it's aligned on a 16-byte
  // boundary as required for IPF.
  //
  VmContext.Gpr[0] &= (INT64)~0x0f;
  VmContext.LowStackTop = (UINTN) VmContext.Gpr[0];
  //
  // Simply copy the image handle and system table onto the EBC stack.
  // Greatly simplifies things by not having to spill the args
  //
  PushU64 (&VmContext, (UINT64) SystemTable);
  PushU64 (&VmContext, (UINT64) ImageHandle);

  //
  // Interpreter assumes 64-bit return address is pushed on the stack.
  // IPF does not do this so pad the stack accordingly. Also, a
  // "return address" is 16 bytes as required for IPF stack alignments.
  //
  PushU64 (&VmContext, (UINT64) 0);
  PushU64 (&VmContext, (UINT64) 0x1234567887654321);
  VmContext.StackRetAddr = (UINT64) VmContext.Gpr[0];

  //
  // Begin executing the EBC code
  //
  EbcDebuggerHookExecuteEbcImageEntryPoint (&VmContext);
  EbcExecute (&VmContext);

  //
  // Return the value in Gpr[7] unless there was an error
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
  UINT64      Addr;
  UINT64      Code[3];    // Code in a bundle
  UINT64      RegNum;     // register number for MOVL
  UINT64      BitI;       // bits of MOVL immediate data
  UINT64      BitIc;         // bits of MOVL immediate data
  UINT64      BitImm5c;      // bits of MOVL immediate data
  UINT64      BitImm9d;      // bits of MOVL immediate data
  UINT64      BitImm7b;      // bits of MOVL immediate data
  UINT64      Br;         // branch register for loading and jumping
  UINT64      *Data64Ptr;
  UINT32      ThunkSize;
  UINT32      Size;

  //
  // Check alignment of pointer to EBC code, which must always be aligned
  // on a 2-byte boundary.
  //
  if ((UINT32) (UINTN) EbcEntryPoint & 0x01) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Allocate memory for the thunk. Make the (most likely incorrect) assumption
  // that the returned buffer is not aligned, so round up to the next
  // alignment size.
  //
  Size      = EBC_THUNK_SIZE + EBC_THUNK_ALIGNMENT - 1;
  ThunkSize = Size;
  Ptr = EbcAllocatePoolForThunk (Size);

  if (Ptr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Save the start address of the buffer.
  //
  ThunkBase = Ptr;

  //
  // Make sure it's aligned for code execution. If not, then
  // round up.
  //
  if ((UINT32) (UINTN) Ptr & (EBC_THUNK_ALIGNMENT - 1)) {
    Ptr = (UINT8 *) (((UINTN) Ptr + (EBC_THUNK_ALIGNMENT - 1)) &~ (UINT64) (EBC_THUNK_ALIGNMENT - 1));
  }
  //
  // Return the pointer to the thunk to the caller to user as the
  // image entry point.
  //
  *Thunk = (VOID *) Ptr;

  //
  // Clear out the thunk entry
  // ZeroMem(Ptr, Size);
  //
  // For IPF, when you do a call via a function pointer, the function pointer
  // actually points to a function descriptor which consists of a 64-bit
  // address of the function, followed by a 64-bit gp for the function being
  // called. See the the Software Conventions and Runtime Architecture Guide
  // for details.
  // So first off in our thunk, create a descriptor for our actual thunk code.
  // This means we need to create a pointer to the thunk code (which follows
  // the descriptor we're going to create), followed by the gp of the Vm
  // interpret function we're going to eventually execute.
  //
  Data64Ptr = (UINT64 *) Ptr;

  //
  // Write the function's entry point (which is our thunk code that follows
  // this descriptor we're creating).
  //
  *Data64Ptr = (UINT64) (Data64Ptr + 2);
  //
  // Get the gp from the descriptor for EbcInterpret and stuff it in our thunk
  // descriptor.
  //
  *(Data64Ptr + 1) = *(UINT64 *) ((UINT64 *) (UINTN) EbcInterpret + 1);
  //
  // Advance our thunk data pointer past the descriptor. Since the
  // descriptor consists of 16 bytes, the pointer is still aligned for
  // IPF code execution (on 16-byte boundary).
  //
  Ptr += sizeof (UINT64) * 2;

  //
  // *************************** MAGIC BUNDLE ********************************
  //
  // Write magic code bundle for: movl r8 = 0xca112ebcca112ebc to help the VM
  // to recognize it is a thunk.
  //
  Addr = (UINT64) 0xCA112EBCCA112EBC;

  //
  // Now generate the code bytes. First is nop.m 0x0
  //
  Code[0] = OPCODE_NOP;

  //
  // Next is simply Addr[62:22] (41 bits) of the address
  //
  Code[1] = RShiftU64 (Addr, 22) & 0x1ffffffffff;

  //
  // Extract bits from the address for insertion into the instruction
  // i = Addr[63:63]
  //
  BitI = RShiftU64 (Addr, 63) & 0x01;
  //
  // ic = Addr[21:21]
  //
  BitIc = RShiftU64 (Addr, 21) & 0x01;
  //
  // imm5c = Addr[20:16] for 5 bits
  //
  BitImm5c = RShiftU64 (Addr, 16) & 0x1F;
  //
  // imm9d = Addr[15:7] for 9 bits
  //
  BitImm9d = RShiftU64 (Addr, 7) & 0x1FF;
  //
  // imm7b = Addr[6:0] for 7 bits
  //
  BitImm7b = Addr & 0x7F;

  //
  // The EBC entry point will be put into r8, so r8 can be used here
  // temporary. R8 is general register and is auto-serialized.
  //
  RegNum = 8;

  //
  // Next is jumbled data, including opcode and rest of address
  //
  Code[2] = LShiftU64 (BitImm7b, 13);
  Code[2] = Code[2] | LShiftU64 (0x00, 20);   // vc
  Code[2] = Code[2] | LShiftU64 (BitIc, 21);
  Code[2] = Code[2] | LShiftU64 (BitImm5c, 22);
  Code[2] = Code[2] | LShiftU64 (BitImm9d, 27);
  Code[2] = Code[2] | LShiftU64 (BitI, 36);
  Code[2] = Code[2] | LShiftU64 ((UINT64)MOVL_OPCODE, 37);
  Code[2] = Code[2] | LShiftU64 ((RegNum & 0x7F), 6);

  WriteBundle ((VOID *) Ptr, 0x05, Code[0], Code[1], Code[2]);

  //
  // *************************** FIRST BUNDLE ********************************
  //
  // Write code bundle for: movl r8 = EBC_ENTRY_POINT so we pass
  // the ebc entry point in to the interpreter function via a processor
  // register.
  // Note -- we could easily change this to pass in a pointer to a structure
  // that contained, among other things, the EBC image's entry point. But
  // for now pass it directly.
  //
  Ptr += 16;
  Addr = (UINT64) EbcEntryPoint;

  //
  // Now generate the code bytes. First is nop.m 0x0
  //
  Code[0] = OPCODE_NOP;

  //
  // Next is simply Addr[62:22] (41 bits) of the address
  //
  Code[1] = RShiftU64 (Addr, 22) & 0x1ffffffffff;

  //
  // Extract bits from the address for insertion into the instruction
  // i = Addr[63:63]
  //
  BitI = RShiftU64 (Addr, 63) & 0x01;
  //
  // ic = Addr[21:21]
  //
  BitIc = RShiftU64 (Addr, 21) & 0x01;
  //
  // imm5c = Addr[20:16] for 5 bits
  //
  BitImm5c = RShiftU64 (Addr, 16) & 0x1F;
  //
  // imm9d = Addr[15:7] for 9 bits
  //
  BitImm9d = RShiftU64 (Addr, 7) & 0x1FF;
  //
  // imm7b = Addr[6:0] for 7 bits
  //
  BitImm7b = Addr & 0x7F;

  //
  // Put the EBC entry point in r8, which is the location of the return value
  // for functions.
  //
  RegNum = 8;

  //
  // Next is jumbled data, including opcode and rest of address
  //
  Code[2] = LShiftU64 (BitImm7b, 13);
  Code[2] = Code[2] | LShiftU64 (0x00, 20);   // vc
  Code[2] = Code[2] | LShiftU64 (BitIc, 21);
  Code[2] = Code[2] | LShiftU64 (BitImm5c, 22);
  Code[2] = Code[2] | LShiftU64 (BitImm9d, 27);
  Code[2] = Code[2] | LShiftU64 (BitI, 36);
  Code[2] = Code[2] | LShiftU64 ((UINT64)MOVL_OPCODE, 37);
  Code[2] = Code[2] | LShiftU64 ((RegNum & 0x7F), 6);

  WriteBundle ((VOID *) Ptr, 0x05, Code[0], Code[1], Code[2]);

  //
  // *************************** NEXT BUNDLE *********************************
  //
  // Write code bundle for:
  //   movl rx = offset_of(EbcInterpret|ExecuteEbcImageEntryPoint)
  //
  // Advance pointer to next bundle, then compute the offset from this bundle
  // to the address of the entry point of the interpreter.
  //
  Ptr += 16;
  if ((Flags & FLAG_THUNK_ENTRY_POINT) != 0) {
    Addr = (UINT64) ExecuteEbcImageEntryPoint;
  } else {
    Addr = (UINT64) EbcInterpret;
  }
  //
  // Indirection on Itanium-based systems
  //
  Addr = *(UINT64 *) Addr;

  //
  // Now write the code to load the offset into a register
  //
  Code[0] = OPCODE_NOP;

  //
  // Next is simply Addr[62:22] (41 bits) of the address
  //
  Code[1] = RShiftU64 (Addr, 22) & 0x1ffffffffff;

  //
  // Extract bits from the address for insertion into the instruction
  // i = Addr[63:63]
  //
  BitI = RShiftU64 (Addr, 63) & 0x01;
  //
  // ic = Addr[21:21]
  //
  BitIc = RShiftU64 (Addr, 21) & 0x01;
  //
  // imm5c = Addr[20:16] for 5 bits
  //
  BitImm5c = RShiftU64 (Addr, 16) & 0x1F;
  //
  // imm9d = Addr[15:7] for 9 bits
  //
  BitImm9d = RShiftU64 (Addr, 7) & 0x1FF;
  //
  // imm7b = Addr[6:0] for 7 bits
  //
  BitImm7b = Addr & 0x7F;

  //
  // Put it in r31, a scratch register
  //
  RegNum = 31;

  //
  // Next is jumbled data, including opcode and rest of address
  //
  Code[2] =   LShiftU64(BitImm7b, 13);
  Code[2] = Code[2] | LShiftU64 (0x00, 20);   // vc
  Code[2] = Code[2] | LShiftU64 (BitIc, 21);
  Code[2] = Code[2] | LShiftU64 (BitImm5c, 22);
  Code[2] = Code[2] | LShiftU64 (BitImm9d, 27);
  Code[2] = Code[2] | LShiftU64 (BitI, 36);
  Code[2] = Code[2] | LShiftU64 ((UINT64)MOVL_OPCODE, 37);
  Code[2] = Code[2] | LShiftU64 ((RegNum & 0x7F), 6);

  WriteBundle ((VOID *) Ptr, 0x05, Code[0], Code[1], Code[2]);

  //
  // *************************** NEXT BUNDLE *********************************
  //
  // Load branch register with EbcInterpret() function offset from the bundle
  // address: mov b6 = RegNum
  //
  // See volume 3 page 4-29 of the Arch. Software Developer's Manual.
  //
  // Advance pointer to next bundle
  //
  Ptr += 16;
  Code[0] = OPCODE_NOP;
  Code[1] = OPCODE_NOP;
  Code[2] = OPCODE_MOV_BX_RX;

  //
  // Pick a branch register to use. Then fill in the bits for the branch
  // register and user register (same user register as previous bundle).
  //
  Br = 6;
  Code[2] |= LShiftU64 (Br, 6);
  Code[2] |= LShiftU64 (RegNum, 13);
  WriteBundle ((VOID *) Ptr, 0x0d, Code[0], Code[1], Code[2]);

  //
  // *************************** NEXT BUNDLE *********************************
  //
  // Now do the branch:  (p0) br.cond.sptk.few b6
  //
  // Advance pointer to next bundle.
  // Fill in the bits for the branch register (same reg as previous bundle)
  //
  Ptr += 16;
  Code[0] = OPCODE_NOP;
  Code[1] = OPCODE_NOP;
  Code[2] = OPCODE_BR_COND_SPTK_FEW;
  Code[2] |= LShiftU64 (Br, 13);
  WriteBundle ((VOID *) Ptr, 0x1d, Code[0], Code[1], Code[2]);

  //
  // Add the thunk to our list of allocated thunks so we can do some cleanup
  // when the image is unloaded. Do this last since the Add function flushes
  // the instruction cache for us.
  //
  EbcAddImageThunk (ImageHandle, (VOID *) ThunkBase, ThunkSize);

  //
  // Done
  //
  return EFI_SUCCESS;
}


/**
  Given raw bytes of Itanium based code, format them into a bundle and
  write them out.

  @param  MemPtr                 pointer to memory location to write the bundles
                                 to.
  @param  Template               5-bit template.
  @param  Slot0                  Instruction slot 0 data for the bundle.
  @param  Slot1                  Instruction slot 1 data for the bundle.
  @param  Slot2                  Instruction slot 2 data for the bundle.

  @retval EFI_INVALID_PARAMETER  Pointer is not aligned
  @retval EFI_INVALID_PARAMETER  No more than 5 bits in template
  @retval EFI_INVALID_PARAMETER  More than 41 bits used in code
  @retval EFI_SUCCESS            All data is written.

**/
EFI_STATUS
WriteBundle (
  IN    VOID    *MemPtr,
  IN    UINT8   Template,
  IN    UINT64  Slot0,
  IN    UINT64  Slot1,
  IN    UINT64  Slot2
  )
{
  UINT8   *BPtr;
  UINT32  Index;
  UINT64  Low64;
  UINT64  High64;

  //
  // Verify pointer is aligned
  //
  if ((UINT64) MemPtr & 0xF) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Verify no more than 5 bits in template
  //
  if ((Template &~0x1F) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Verify max of 41 bits used in code
  //
  if (((Slot0 | Slot1 | Slot2) &~0x1ffffffffff) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Low64   = LShiftU64 (Slot1, 46);
  Low64   = Low64 | LShiftU64 (Slot0, 5) | Template;

  High64  = RShiftU64 (Slot1, 18);
  High64  = High64 | LShiftU64 (Slot2, 23);

  //
  // Now write it all out
  //
  BPtr = (UINT8 *) MemPtr;
  for (Index = 0; Index < 8; Index++) {
    *BPtr = (UINT8) Low64;
    Low64 = RShiftU64 (Low64, 8);
    BPtr++;
  }

  for (Index = 0; Index < 8; Index++) {
    *BPtr   = (UINT8) High64;
    High64  = RShiftU64 (High64, 8);
    BPtr++;
  }

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
  UINTN    CodeOne18;
  UINTN    CodeOne23;
  UINTN    CodeTwoI;
  UINTN    CodeTwoIc;
  UINTN    CodeTwo7b;
  UINTN    CodeTwo5c;
  UINTN    CodeTwo9d;
  UINTN    CalleeAddr;

  IsThunk       = 1;
  TargetEbcAddr = 0;

  //
  // FuncAddr points to the descriptor of the target instructions.
  //
  CalleeAddr = *((UINT64 *)FuncAddr);

  //
  // Processor specific code to check whether the callee is a thunk to EBC.
  //
  if (*((UINT64 *)CalleeAddr) != 0xBCCA000100000005) {
    IsThunk = 0;
    goto Action;
  }
  if (*((UINT64 *)CalleeAddr + 1) != 0x697623C1004A112E)  {
    IsThunk = 0;
    goto Action;
  }

  CodeOne18 = RShiftU64 (*((UINT64 *)CalleeAddr + 2), 46) & 0x3FFFF;
  CodeOne23 = (*((UINT64 *)CalleeAddr + 3)) & 0x7FFFFF;
  CodeTwoI  = RShiftU64 (*((UINT64 *)CalleeAddr + 3), 59) & 0x1;
  CodeTwoIc = RShiftU64 (*((UINT64 *)CalleeAddr + 3), 44) & 0x1;
  CodeTwo7b = RShiftU64 (*((UINT64 *)CalleeAddr + 3), 36) & 0x7F;
  CodeTwo5c = RShiftU64 (*((UINT64 *)CalleeAddr + 3), 45) & 0x1F;
  CodeTwo9d = RShiftU64 (*((UINT64 *)CalleeAddr + 3), 50) & 0x1FF;

  TargetEbcAddr = CodeTwo7b;
  TargetEbcAddr = TargetEbcAddr | LShiftU64 (CodeTwo9d, 7);
  TargetEbcAddr = TargetEbcAddr | LShiftU64 (CodeTwo5c, 16);
  TargetEbcAddr = TargetEbcAddr | LShiftU64 (CodeTwoIc, 21);
  TargetEbcAddr = TargetEbcAddr | LShiftU64 (CodeOne18, 22);
  TargetEbcAddr = TargetEbcAddr | LShiftU64 (CodeOne23, 40);
  TargetEbcAddr = TargetEbcAddr | LShiftU64 (CodeTwoI, 63);

Action:
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
    VmWriteMem64 (VmPtr, (UINTN) VmPtr->Gpr[0], (UINT64) (VmPtr->Ip + Size));

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
