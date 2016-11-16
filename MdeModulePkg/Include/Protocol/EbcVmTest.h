/** @file
  EBC VM Test protocol for test purposes.

Copyright (c) 2011 - 2016, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EBC_VM_TEST_PROTOCOL_H_
#define _EBC_VM_TEST_PROTOCOL_H_

//
// Define a protocol for an EBC VM test interface.
//
#define EFI_EBC_VM_TEST_PROTOCOL_GUID \
  { \
    0xAAEACCFD, 0xF27B, 0x4C17, { 0xB6, 0x10, 0x75, 0xCA, 0x1F, 0x2D, 0xFB, 0x52 } \
  }

//
// Define for forward reference.
//
typedef struct _EFI_EBC_VM_TEST_PROTOCOL EFI_EBC_VM_TEST_PROTOCOL;

//
// VM major/minor version
//
#define VM_MAJOR_VERSION  1
#define VM_MINOR_VERSION  0

//
// Bits in the VM->StopFlags field
//
#define STOPFLAG_APP_DONE         0x0001
#define STOPFLAG_BREAKPOINT       0x0002
#define STOPFLAG_INVALID_BREAK    0x0004
#define STOPFLAG_BREAK_ON_CALLEX  0x0008

//
// Masks for working with the VM flags register
//
#define VMFLAGS_CC        0x0001  // condition flag
#define VMFLAGS_STEP      0x0002  // step instruction mode
#define VMFLAGS_ALL_VALID (VMFLAGS_CC | VMFLAGS_STEP)

//
// Macros for operating on the VM flags register
//
#define VMFLAG_SET(pVM, Flag)   (pVM->Flags |= (Flag))
#define VMFLAG_ISSET(pVM, Flag) ((pVM->Flags & (Flag)) ? 1 : 0)
#define VMFLAG_CLEAR(pVM, Flag) (pVM->Flags &= ~(Flag))

//
// Define a macro to get the operand. Then we can change it to be either a
// direct read or have it call a function to read memory.
//
#define GETOPERANDS(pVM)  (UINT8) (*(UINT8 *) (pVM->Ip + 1))
#define GETOPCODE(pVM)    (UINT8) (*(UINT8 *) pVM->Ip)

//
// Macros for operating on the VM GP registers
//
#define OPERAND1_REGDATA(pVM, Op) pVM->Gpr[OPERAND1_REGNUM (Op)]
#define OPERAND2_REGDATA(pVM, Op) pVM->Gpr[OPERAND2_REGNUM (Op)]

//
// Bits of exception flags field of VM context
//
#define EXCEPTION_FLAG_FATAL    0x80000000  // can't continue
#define EXCEPTION_FLAG_ERROR    0x40000000  // bad, but try to continue
#define EXCEPTION_FLAG_WARNING  0x20000000  // harmless problem
#define EXCEPTION_FLAG_NONE     0x00000000  // for normal return

///
/// instruction pointer for the VM
///
typedef UINT8   *VMIP;

typedef INT64   VM_REGISTER;
typedef UINT32  EXCEPTION_FLAGS;

typedef struct {
  VM_REGISTER       Gpr[8];                 ///< General purpose registers.
                                            ///< Flags register:
                                            ///<   0  Set to 1 if the result of the last compare was true
                                            ///<   1  Set to 1 if stepping
  UINT64            Flags;                  ///<   2..63 Reserved.
  VMIP              Ip;                     ///< Instruction pointer.
  UINTN             LastException;
  EXCEPTION_FLAGS   ExceptionFlags;         ///< to keep track of exceptions
  UINT32            StopFlags;
  UINT32            CompilerVersion;        ///< via break(6)
  UINTN             HighStackBottom;        ///< bottom of the upper stack
  UINTN             LowStackTop;            ///< top of the lower stack
  UINT64            StackRetAddr;           ///< location of final return address on stack
  UINTN             *StackMagicPtr;         ///< pointer to magic value on stack to detect corruption
  EFI_HANDLE        ImageHandle;            ///< for this EBC driver
  EFI_SYSTEM_TABLE  *SystemTable;           ///< for debugging only
  UINTN             LastAddrConverted;      ///< for debug
  UINTN             LastAddrConvertedValue; ///< for debug
  VOID              *FramePtr;
  VOID              *EntryPoint;            ///< entry point of EBC image
  UINTN             ImageBase;
  VOID              *StackPool;
  VOID              *StackTop;
} VM_CONTEXT;

/**
  Given a pointer to a new VM context, execute one or more instructions. This
  function is only used for test purposes.

  @param[in]      This              A pointer to the EFI_EBC_VM_TEST_PROTOCOL structure.
  @param[in]      VmPtr             A pointer to a VM context.
  @param[in, out] InstructionCount  A pointer to a UINTN value holding the number of
                                    instructions to execute. If it holds value of 0,
                                    then the instruction to be executed is 1.

  @retval EFI_UNSUPPORTED       At least one of the opcodes is not supported.
  @retval EFI_SUCCESS           All of the instructions are executed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EBC_VM_TEST_EXECUTE) (
  IN EFI_EBC_VM_TEST_PROTOCOL         *This,
  IN VM_CONTEXT                       *VmPtr,
  IN OUT UINTN                        *InstructionCount
  );

/**
  Convert AsmText to the instruction. This function is only used for test purposes.

  @param[in]  This              A pointer to the EFI_EBC_VM_TEST_PROTOCOL structure.
  @param[in]  AsmText           A pointer to EBC ASM text code.
  @param[out] Buffer            Buffer to store the instruction.
  @param[out] BufferLen         Size of buffer that is required to store data.

  @retval EFI_UNSUPPORTED       This functionality is unsupported.
  @retval EFI_SUCCESS           Successfully convert AsmText to the instruction. 

**/
typedef
EFI_STATUS
(EFIAPI *EBC_VM_TEST_ASM) (
  IN EFI_EBC_VM_TEST_PROTOCOL         *This,
  IN CHAR16                           *AsmText,
  IN OUT INT8                         *Buffer,
  IN OUT UINTN                        *BufferLen
  );

/**
  Dump the executed instruction. This function is only used for test purposes.

  @param[in]  This              A pointer to the EFI_EBC_VM_TEST_PROTOCOL structure.
  @param[out] AsmText           Contain the disasm text.
  @param[out] Buffer            Buffer to store the instruction.
  @param[out] BufferLen         Size of buffer that is required to store data.

  @retval EFI_UNSUPPORTED       This functionality is unsupported.
  @retval EFI_SUCCESS           Successfully dump the executed instruction.

**/
typedef
EFI_STATUS
(EFIAPI *EBC_VM_TEST_DASM) (
  IN EFI_EBC_VM_TEST_PROTOCOL         *This,
  IN OUT CHAR16                       *AsmText,
  IN OUT INT8                         *Buffer,
  IN OUT UINTN                        *Len
  );

//
// Prototype for the actual EBC test protocol interface
//
struct _EFI_EBC_VM_TEST_PROTOCOL {
  EBC_VM_TEST_EXECUTE Execute;
  EBC_VM_TEST_ASM     Assemble;
  EBC_VM_TEST_DASM    Disassemble;
};

extern EFI_GUID gEfiEbcVmTestProtocolGuid;

#endif
