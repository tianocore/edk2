/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

  x86Thunk.c

Abstract:

  Real Mode Thunk Functions

--*/

#include "Thunk16Lib.h"
#include "EfiCommonLib.h"

extern CONST UINTN                  mCode16Size;

extern
IA32_REGISTER_SET *
EFIAPI
_Thunk16 (
  IN OUT  IA32_REGISTER_SET         *RegisterSet,
  IN      UINT32                    ThunkFlags,
  IN      UINT32                    RealModeCs
  );

extern
VOID
EFIAPI
_Code16Addr (
  VOID
  );

VOID
EFIAPI
AsmFxRestore (
  IN CONST IA32_FX_BUFFER *Buffer
  );

VOID
EFIAPI
AsmFxSave (
  OUT IA32_FX_BUFFER *Buffer
  );

UINTN
EFIAPI
AsmGetEflags (
  VOID
  );

VOID
EFIAPI
AsmSetEflags (
  IN UINTN   Eflags
  );

//
// Implementation
//
STATIC
IA32_REGISTER_SET *
AsmThunk16 (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN OUT  IA32_REGISTER_SET         *RegisterSet,
  IN      UINT32                    ThunkFlags
  )
/*++

Routine Description:

  Do the 16-bit thunk code.

  NOTE: This function must be called on TPL_HIGH_LEVEL or with interrupts
        disabled because of GDTR and IDTR manipulations.
        This function must be placed in identity mapped pages.

Arguments:

  ThunkContext  - Thunk context to use.
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
{
  IA32_FX_BUFFER                    *FpSavedState;
  UINT8                             FpBuffer[sizeof (*FpSavedState) + 0x10];
  UINTN                             Eflags;

  FpSavedState = (IA32_FX_BUFFER*)(((UINTN)FpBuffer + 0xf) & ~0xf);

  if (!(ThunkFlags & THUNK_USER_STACK)) {
    RegisterSet->E.ESP = (UINT16)ThunkContext->DefaultStack;
    RegisterSet->E.SS = (UINT16)((ThunkContext->DefaultStack >> 4) & 0xf000);
  }

  if (ThunkFlags & THUNK_SAVE_FP_STATE) {
    AsmFxSave (FpSavedState);
  }

  Eflags = AsmGetEflags ();

  EfiCommonLibCopyMem (
    RegisterSet,
    _Thunk16 (
      RegisterSet,
      (UINT16)(ThunkFlags >> 16),
      ThunkContext->RealModeBuffer >> 4
      ),
    sizeof (*RegisterSet)
    );

   AsmSetEflags (Eflags);

  if (ThunkFlags & THUNK_SAVE_FP_STATE) {
    AsmFxRestore (FpSavedState);
  }

  return RegisterSet;
}

UINTN
EFIAPI
AsmThunk16GetProperties (
  OUT     UINTN                     *MinimumStackSize
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
{
  //
  // This size should be large enough to hold the register set as well as saved
  // CPU contexts including GDTR, CR0 and CR4
  //
  if (MinimumStackSize) {
    *MinimumStackSize = sizeof (IA32_REGISTER_SET) + 0x200;
  }

  return mCode16Size;
}

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
{
  BufferSize &= ~3;

  ThunkContext->RealModeBuffer = (UINT32)(UINTN)RealModeBuffer;
  ThunkContext->DefaultStack   = (UINT32)(ThunkContext->RealModeBuffer + BufferSize);
  EfiCommonLibCopyMem (RealModeBuffer, (VOID*)(UINTN)_Code16Addr, mCode16Size);

  return ThunkContext;
}

#pragma pack (1)

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
  UINTN                             EFLAGS;
  UINT32                            EIP;
  UINT16                            CS;
  UINT16                            SS;
} IA32_REGS;

typedef struct {
  UINT16  Limit;
  UINT32  Base;
} IA32_DESC;

typedef struct {
  UINT32    RetEip;
  UINT16    RetCs;
  UINT16    ThunkFlags;
#ifdef EFI32
  UINT32    SavedEsp;
  UINT16    SavedSs;
#endif
  IA32_DESC SavedGdtr;
#ifdef EFIX64
  UINT16    Resvd1;
#endif
  UINT32    SavedCr0;
  UINT32    SavedCr4;
} _STK16;
#pragma pack ()

#define STACK_PARAM_SIZE  16

BOOLEAN
AsmThunk16SetUserStack (
  IN THUNK_CONTEXT             *ThunkContext,
  IN VOID                      *Stack,
  IN UINTN                     StackSize
  )
{
  if (StackSize > STACK_PARAM_SIZE) {
    return FALSE;
  }

  EfiCommonLibCopyMem ((VOID *)(UINTN)(ThunkContext->DefaultStack - sizeof(_STK16) - sizeof(IA32_REGS) - STACK_PARAM_SIZE), Stack, StackSize);

  return TRUE;
}

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
{
  ThunkContext->RealModeBuffer = 0;
}

IA32_REGISTER_SET *
EFIAPI
AsmThunk16FarCall86 (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN OUT  IA32_REGISTER_SET         *RegisterSet,
  IN      UINT32                    Flags
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
  ThunkFlags    - THUNK_USER_STACK: The stack specified by SS:ESP would be
                  used instead of the default stack.

Returns:

  RegisterSet is returned.

--*/
{
  return AsmThunk16 (ThunkContext, RegisterSet, Flags);
}

IA32_REGISTER_SET *
EFIAPI
AsmThunk16Int86 (
  IN      THUNK_CONTEXT             *ThunkContext,
  IN      UINT8                     IntNumber,
  IN OUT  IA32_REGISTER_SET         *RegisterSet,
  IN      UINT32                    Flags
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
  ThunkFlags    - THUNK_USER_STACK: The stack specified by SS:ESP would be
                  used instead of the default stack.

Returns:

  RegisterSet is returned.

--*/
{
  UINT32  *VectorBase;
  
  //
  // The base address of legacy interrupt vector table is 0.
  // We use this base address to get the legacy interrupt handler.
  //
  VectorBase = 0;
  RegisterSet->E.EIP = (UINT16)(VectorBase)[IntNumber];
  RegisterSet->E.CS  = (UINT16)((VectorBase)[IntNumber] >> 16);

  return AsmThunk16 (ThunkContext, RegisterSet, Flags | THUNK_INTERRUPT);
}
