/** @file
  Definition of EBC Support function.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IPF_EBC_SUPPORT_H_
#define _IPF_EBC_SUPPORT_H_

#define VM_STACK_SIZE   (1024 * 32)

#define EBC_THUNK_SIZE  128
#define STACK_REMAIN_SIZE (1024 * 4)

//
// For code execution, thunks must be aligned on 16-byte boundary
//
#define EBC_THUNK_ALIGNMENT 16

//
// Opcodes for IPF instructions. We'll need to hand-create thunk code (stuffing
// bits) to insert a jump to the interpreter.
//
#define OPCODE_NOP              (UINT64) 0x00008000000
#define OPCODE_BR_COND_SPTK_FEW (UINT64) 0x00100000000
#define OPCODE_MOV_BX_RX        (UINT64) 0x00E00100000

//
// Opcode for MOVL instruction
//
#define MOVL_OPCODE 0x06

#endif
