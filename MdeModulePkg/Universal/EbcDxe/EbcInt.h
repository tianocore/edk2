/** @file
  Main routines for the EBC interpreter.  Includes the initialization and
  main interpreter routines.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EBC_INT_H_
#define _EBC_INT_H_


#include <Uefi.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/Ebc.h>
#include <Protocol/EbcVmTest.h>
#include <Protocol/EbcSimpleDebugger.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

extern VM_CONTEXT                    *mVmPtr;

//
// Bits of exception flags field of VM context
//
#define EXCEPTION_FLAG_FATAL    0x80000000  // can't continue
#define EXCEPTION_FLAG_ERROR    0x40000000  // bad, but try to continue
#define EXCEPTION_FLAG_WARNING  0x20000000  // harmless problem
#define EXCEPTION_FLAG_NONE     0x00000000  // for normal return
//
// Flags passed to the internal create-thunks function.
//
#define FLAG_THUNK_ENTRY_POINT  0x01  // thunk for an image entry point
#define FLAG_THUNK_PROTOCOL     0x00  // thunk for an EBC protocol service
//
// Put this value at the bottom of the VM's stack gap so we can check it on
// occasion to make sure the stack has not been corrupted.
//
#define VM_STACK_KEY_VALUE  0xDEADBEEF

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
  );

/**
  Add a thunk to our list of thunks for a given image handle.
  Also flush the instruction cache since we've written thunk code
  to memory that will be executed eventually.

  @param  ImageHandle            The image handle to which the thunk is tied.
  @param  ThunkBuffer            The buffer that has been created/allocated.
  @param  ThunkSize              The size of the thunk memory allocated.

  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
EbcAddImageThunk (
  IN EFI_HANDLE      ImageHandle,
  IN VOID            *ThunkBuffer,
  IN UINT32          ThunkSize
  );

//
// The interpreter calls these when an exception is detected,
// or as a periodic callback.
//
/**
  The VM interpreter calls this function when an exception is detected.

  @param  ExceptionType          Specifies the processor exception detected.
  @param  ExceptionFlags         Specifies the exception context.
  @param  VmPtr                  Pointer to a VM context for passing info to the
                                 EFI debugger.

  @retval EFI_SUCCESS            This function completed successfully.

**/
EFI_STATUS
EbcDebugSignalException (
  IN EFI_EXCEPTION_TYPE                   ExceptionType,
  IN EXCEPTION_FLAGS                      ExceptionFlags,
  IN VM_CONTEXT                           *VmPtr
  );

//
// Define a constant of how often to call the debugger periodic callback
// function.
//
#define EFI_TIMER_UNIT_1MS            (1000 * 10)
#define EBC_VM_PERIODIC_CALLBACK_RATE (1000 * EFI_TIMER_UNIT_1MS)
#define STACK_POOL_SIZE               (1024 * 1020)
#define MAX_STACK_NUM                 4

//
// External low level functions that are native-processor dependent
//
/**
  The VM thunk code stuffs an EBC entry point into a processor
  register. Since we can't use inline assembly to get it from
  the interpreter C code, stuff it into the return value
  register and return.

  @return  The contents of the register in which the entry point is passed.

**/
UINTN
EFIAPI
EbcLLGetEbcEntryPoint (
  VOID
  );

/**
  This function is called to execute an EBC CALLEX instruction.
  This instruction requires that we thunk out to external native
  code. For x64, we switch stacks, copy the arguments to the stack
  and jump to the specified function.
  On return, we restore the stack pointer to its original location.
  Destroys no working registers.

  @param  CallAddr     The function address.
  @param  EbcSp        The new EBC stack pointer.
  @param  FramePtr     The frame pointer.

  @return The unmodified value returned by the native code.

**/
INT64
EFIAPI
EbcLLCALLEXNative (
  IN UINTN        CallAddr,
  IN UINTN        EbcSp,
  IN VOID         *FramePtr
  );

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
  );

/**
  Returns the stack index and buffer assosicated with the Handle parameter.

  @param  Handle                The EFI handle as the index to the EBC stack.
  @param  StackBuffer           A pointer to hold the returned stack buffer.
  @param  BufferIndex           A pointer to hold the returned stack index.

  @retval EFI_OUT_OF_RESOURCES  The Handle parameter does not correspond to any
                                existing EBC stack.
  @retval EFI_SUCCESS           The stack index and buffer were found and
                                returned to the caller.

**/
EFI_STATUS
GetEBCStack(
  IN  EFI_HANDLE Handle,
  OUT VOID       **StackBuffer,
  OUT UINTN      *BufferIndex
  );

/**
  Returns from the EBC stack by stack Index.

  @param  Index        Specifies which EBC stack to return from.

  @retval EFI_SUCCESS  The function completed successfully.

**/
EFI_STATUS
ReturnEBCStack(
  IN UINTN Index
  );

/**
  Allocates memory to hold all the EBC stacks.

  @retval EFI_SUCCESS          The EBC stacks were allocated successfully.
  @retval EFI_OUT_OF_RESOURCES Not enough memory available for EBC stacks.

**/
EFI_STATUS
InitEBCStack (
  VOID
  );

/**
  Free all EBC stacks allocated before.

  @retval EFI_SUCCESS   All the EBC stacks were freed.

**/
EFI_STATUS
FreeEBCStack(
  VOID
  );

/**
  Returns from the EBC stack associated with the Handle parameter.

  @param  Handle      Specifies the EFI handle to find the EBC stack with.

  @retval EFI_SUCCESS The function completed successfully.

**/
EFI_STATUS
ReturnEBCStackByHandle(
  IN EFI_HANDLE Handle
  );

typedef struct {
  EFI_EBC_PROTOCOL  *This;
  VOID              *EntryPoint;
  EFI_HANDLE        ImageHandle;
  VM_CONTEXT        VmContext;
} EFI_EBC_THUNK_DATA;

#define EBC_PROTOCOL_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('e', 'b', 'c', 'p')


#define EBC_PROTOCOL_PRIVATE_DATA_FROM_THIS(a) \
      CR(a, EBC_PROTOCOL_PRIVATE_DATA, EbcProtocol, EBC_PROTOCOL_PRIVATE_DATA_SIGNATURE)


#endif // #ifndef _EBC_INT_H_
