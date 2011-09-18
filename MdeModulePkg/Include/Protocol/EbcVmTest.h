/** @file
  EBC VM Test protocol for test purposes.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

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
  @param[out] BufferLen         Size of buffer that is requried to store data.

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
  @param[out] BufferLen         Size of buffer that is requried to store data.

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
