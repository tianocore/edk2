/*++

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Thunk16Lib.h

Abstract:

  Real Mode Thunk Header file

--*/

#ifndef __THUNK_16_LIB_H__
#define __THUNK_16_LIB_H__
#include "Tiano.h"

//
// Thunk Flags
//
#define THUNK_SAVE_FP_STATE         0x1
#define THUNK_USER_STACK            0x2
#define THUNK_INTERRUPT             0x10000

//
// Byte packed structure for 16-bit Real Mode FLAGS
//
typedef union {
  struct {
    UINT32  CF:1;           // Carry Flag
    UINT32  Reserved_0:1;   // Reserved
    UINT32  PF:1;           // Parity Flag
    UINT32  Reserved_1:1;   // Reserved
    UINT32  AF:1;           // Auxiliary Carry Flag
    UINT32  Reserved_2:1;   // Reserved
    UINT32  ZF:1;           // Zero Flag
    UINT32  SF:1;           // Sign Flag
    UINT32  TF:1;           // Trap Flag
    UINT32  IF:1;           // Interrupt Enable Flag
    UINT32  DF:1;           // Direction Flag
    UINT32  OF:1;           // Overflow Flag
    UINT32  IOPL:2;         // I/O Privilege Level
    UINT32  NT:1;           // Nested Task
    UINT32  Reserved_3:1;   // Reserved
  } Bits;
  UINTN     UintN;
} IA32_FLAGS16;

//
// Byte packed structure for EFLAGS
// 32-bits on IA32
// 64-bits on X64
//

typedef union {
  struct {
    UINT32  CF:1;           // Carry Flag
    UINT32  Reserved_0:1;   // Reserved
    UINT32  PF:1;           // Parity Flag
    UINT32  Reserved_1:1;   // Reserved
    UINT32  AF:1;           // Auxiliary Carry Flag
    UINT32  Reserved_2:1;   // Reserved
    UINT32  ZF:1;           // Zero Flag
    UINT32  SF:1;           // Sign Flag
    UINT32  TF:1;           // Trap Flag
    UINT32  IF:1;           // Interrupt Enable Flag
    UINT32  DF:1;           // Direction Flag
    UINT32  OF:1;           // Overflow Flag
    UINT32  IOPL:2;         // I/O Privilege Level
    UINT32  NT:1;           // Nested Task
    UINT32  Reserved_3:1;   // Reserved
    UINT32  RF:1;           // Resume Flag
    UINT32  VM:1;           // Virtual 8086 Mode
    UINT32  AC:1;           // Alignment Check
    UINT32  VIF:1;          // Virtual Interrupt Flag
    UINT32  VIP:1;          // Virtual Interrupt Pending
    UINT32  ID:1;           // ID Flag
    UINT32  Reserved_4:10;  // Reserved
  } Bits;
  UINTN     UintN;
} IA32_EFLAGS32;

//
// Byte packed structure for an FP/SSE/SSE2 context
//
typedef struct {
  UINT8  Buffer[512];
} IA32_FX_BUFFER;

//
// Structures for the 16-bit real mode thunks
//
typedef struct {
  UINT32                            Reserved1;
  UINT32                            Reserved2;
  UINT32                            Reserved3;
  UINT32                            Reserved4;
  UINT8                             BL;
  UINT8                             BH;
  UINT16                            Reserved5;
  UINT8                             DL;
  UINT8                             DH;
  UINT16                            Reserved6;
  UINT8                             CL;
  UINT8                             CH;
  UINT16                            Reserved7;
  UINT8                             AL;
  UINT8                             AH;
  UINT16                            Reserved8;
} IA32_BYTE_REGS;

typedef struct {
  UINT16                            DI;
  UINT16                            Reserved1;
  UINT16                            SI;
  UINT16                            Reserved2;
  UINT16                            BP;
  UINT16                            Reserved3;
  UINT16                            SP;
  UINT16                            Reserved4;
  UINT16                            BX;
  UINT16                            Reserved5;
  UINT16                            DX;
  UINT16                            Reserved6;
  UINT16                            CX;
  UINT16                            Reserved7;
  UINT16                            AX;
  UINT16                            Reserved8;
  UINT16                            DS;
  UINT16                            ES;
  UINT16                            FS;
  UINT16                            GS;
  IA32_FLAGS16                      Flags;
  UINT16                            IP;
  UINT16                            Reserved10;
  UINT16                            CS;
  UINT16                            SS;
} IA32_WORD_REGS;

typedef struct {
  UINT32                            EDI;
  UINT32                            ESI;
  UINT32                            EBP;
  UINT32                            ESP;
  UINT32                            EBX;
  UINT32                            EDX;
  UINT32                            ECX;
  UINT32                            EAX;
  UINT16                            DS;
  UINT16                            ES;
  UINT16                            FS;
  UINT16                            GS;
  IA32_EFLAGS32                     EFLAGS;
  UINT32                            EIP;
  UINT16                            CS;
  UINT16                            SS;
} IA32_DWORD_REGS;

typedef union {
  IA32_DWORD_REGS                   E;
  IA32_WORD_REGS                    X;
  IA32_BYTE_REGS                    H;
} IA32_REGISTER_SET;

//
// Byte packed structure for an 16-bit real mode thunks
//
typedef struct {
  UINT32                            RealModeBuffer;
  UINT32                            DefaultStack;
} THUNK_CONTEXT;

//
// 16-bit thunking services
//

UINTN
EFIAPI
AsmThunk16GetProperties (
  OUT     UINTN                     *MinimumStackSize OPTIONAL
  )
/*++

Routine Description:

  Returns the properties of this real mode thunk implementation. Currently
  there are 2 properties has been defined, the minimum real mode buffer size
  and the minimum stack size.

Arguments:

  MinimumStackSize  - The minimum size required for a 16-bit stack.

Returns:

  The minimum size of the real mode buffer needed by this thunk implementation
  is returned.

--*/
;

THUNK_CONTEXT *
EFIAPI
AsmThunk16SetProperties (
  OUT     THUNK_CONTEXT             *ThunkContext,
  IN      VOID                      *RealModeBuffer,
  IN      UINTN                     BufferSize
  )
/*++

Routine Description:

  Tell this real mode thunk implementation the address and size of the real
  mode buffer needed.

Arguments:

  ThunkContext    - The thunk context whose properties to set.
  RealModeBuffer  - The address of the buffer allocated by caller. It should be
                    aligned on a 16-byte boundary.
                    This buffer must be in identity mapped pages.
  BufferSize      - The size of RealModeBuffer. Must be larger than the minimum
                    size required as returned by AsmThunk16GetProperties().

Returns:

  None

--*/
;

BOOLEAN
AsmThunk16SetUserStack (
  IN THUNK_CONTEXT             *ThunkContext,
  IN VOID                      *Stack,
  IN UINTN                     StackSize
  );

VOID
EFIAPI
AsmThunk16Destroy (
  IN OUT  THUNK_CONTEXT             *ThunkContext
  )
/*++

Routine Description:

  Reset all internal states to their initial values. The caller should not
  release the real mode buffer until after a call to this function.

Arguments:

  ThunkContext  - The thunk context to destroy.

Returns:

  None

--*/
;

IA32_REGISTER_SET *
EFIAPI
AsmThunk16FarCall86 (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN OUT  IA32_REGISTER_SET         *RegisterSet,
  IN      UINT32                    ThunkFlags
  )
/*++

Routine Description:

  Make a far call to 16-bit code.

  NOTE: This function must be called on TPL_HIGH_LEVEL or with interrupts
        disabled because of GDTR and IDTR manipulations.
        This function must be placed in identity mapped pages.

Arguments:

  ThunkContext  - Thunk context to use.
  RegisterSet   - CPU registers would be set to the values contained in this
                  structure before making the far call. Then CPU registers are
                  copied back to this structure.
                  CS:EIP points to the real mode code being called on input.
                  SS:ESP points to the real mode stack if THUNK_USER_STACK is
                  set on input, otherwise ignored.
                  EFlages is ignored on input.
                  On output, values of CS, EIP, SS and ESP should be ignored.
  ThunkFlags    - 2 flags have currently been defined, THUNK_SAVE_FP_STATE and
                  THUNK_USER_STACK.
                  THUNK_SAVE_FP_STATE - FPU state would be saved/restored
                                        before/after calling real mode code.
                  THUNK_USER_STACK    - The stack specified by SS:ESP would be
                                        used instead of the default stack.

Returns:

  RegisterSet is returned.

--*/
;

IA32_REGISTER_SET *
EFIAPI
AsmThunk16Int86 (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN      UINT8                     IntNumber,
  IN OUT  IA32_REGISTER_SET         *RegisterSet,
  IN      UINT32                    ThunkFlags
  )
/*++

Routine Description:

  Invoke a 16-bit interrupt handler.

  NOTE: This function must be called on TPL_HIGH_LEVEL or with interrupts
        disabled because of GDTR and IDTR manipulations.
        This function must be placed in identity mapped pages.

Arguments:

  ThunkContext  - Thunk context to use.
  IntNumber     - The ordinal of the interrupt handler ranging from 0 to 255.
  RegisterSet   - CPU registers would be set to the values contained in this
                  structure before making the far call. Then CPU registers are
                  copied back to this structure.
                  SS:ESP points to the real mode stack if THUNK_USER_STACK is
                  set on input, otherwise ignored.
                  EFlages is ignored on input.
                  On output, values of CS, EIP, SS and ESP should be ignored.
  ThunkFlags    - 2 flags have currently been defined, THUNK_SAVE_FP_STATE and
                  THUNK_USER_STACK.
                  THUNK_SAVE_FP_STATE - FPU state would be saved/restored
                                        before/after calling real mode code.
                  THUNK_USER_STACK    - The stack specified by SS:ESP would be
                                        used instead of the default stack.

Returns:

  RegisterSet is returned.

--*/
;

#endif
