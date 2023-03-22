/** @file
  Header file for Virtual Machine support. Contains EBC defines that can
  be of use to a disassembler for the most part. Also provides function
  prototypes for VM functions.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EBC_EXECUTE_H_
#define _EBC_EXECUTE_H_

//
// Macros to check and set alignment
//
#define ASSERT_ALIGNED(addr, size)  ASSERT (ADDRESS_IS_ALIGNED (addr, size))

//
// Debug macro
//
#define EBCMSG(s)  gST->ConOut->OutputString (gST->ConOut, s)

/**
  Execute an EBC image from an entry point or from a published protocol.

  @param  VmPtr             A pointer to a VM context.

  @retval EFI_UNSUPPORTED   At least one of the opcodes is not supported.
  @retval EFI_SUCCESS       All of the instructions are executed successfully.

**/
EFI_STATUS
EbcExecute (
  IN VM_CONTEXT  *VmPtr
  );

/**
  Returns the version of the EBC virtual machine.

  @return The 64-bit version of EBC virtual machine.

**/
UINT64
GetVmVersion (
  VOID
  );

/**
  Writes UINTN data to memory address.

  This routine is called by the EBC data
  movement instructions that write to memory. Since these writes
  may be to the stack, which looks like (high address on top) this,

  [EBC entry point arguments]
  [VM stack]
  [EBC stack]

  we need to detect all attempts to write to the EBC entry point argument
  stack area and adjust the address (which will initially point into the
  VM stack) to point into the EBC entry point arguments.

  @param  VmPtr             A pointer to a VM context.
  @param  Addr              Address to write to.
  @param  Data              Value to write to Addr.

  @retval EFI_SUCCESS       The instruction is executed successfully.
  @retval Other             Some error occurs when writing data to the address.

**/
EFI_STATUS
VmWriteMemN (
  IN VM_CONTEXT  *VmPtr,
  IN UINTN       Addr,
  IN UINTN       Data
  );

/**
  Writes 64-bit data to memory address.

  This routine is called by the EBC data
  movement instructions that write to memory. Since these writes
  may be to the stack, which looks like (high address on top) this,

  [EBC entry point arguments]
  [VM stack]
  [EBC stack]

  we need to detect all attempts to write to the EBC entry point argument
  stack area and adjust the address (which will initially point into the
  VM stack) to point into the EBC entry point arguments.

  @param  VmPtr             A pointer to a VM context.
  @param  Addr              Address to write to.
  @param  Data              Value to write to Addr.

  @retval EFI_SUCCESS       The instruction is executed successfully.
  @retval Other             Some error occurs when writing data to the address.

**/
EFI_STATUS
VmWriteMem64 (
  IN VM_CONTEXT  *VmPtr,
  IN UINTN       Addr,
  IN UINT64      Data
  );

/**
  Given a pointer to a new VM context, execute one or more instructions. This
  function is only used for test purposes via the EBC VM test protocol.

  @param  This              A pointer to the EFI_EBC_VM_TEST_PROTOCOL structure.
  @param  VmPtr             A pointer to a VM context.
  @param  InstructionCount  A pointer to a UINTN value holding the number of
                            instructions to execute. If it holds value of 0,
                            then the instruction to be executed is 1.

  @retval EFI_UNSUPPORTED   At least one of the opcodes is not supported.
  @retval EFI_SUCCESS       All of the instructions are executed successfully.

**/
EFI_STATUS
EFIAPI
EbcExecuteInstructions (
  IN EFI_EBC_VM_TEST_PROTOCOL  *This,
  IN VM_CONTEXT                *VmPtr,
  IN OUT UINTN                 *InstructionCount
  );

#endif // ifndef _EBC_EXECUTE_H_
