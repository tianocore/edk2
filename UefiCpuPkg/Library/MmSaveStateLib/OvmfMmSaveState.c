/** @file
Provides services to access SMRAM Save State Map

Copyright (c) 2010 - 2023, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MmSaveState.h"
#include <Register/QemuSmramSaveStateMap.h>
#include <Library/BaseLib.h>

///
/// Macro used to simplify the lookup table entries of type
/// CPU_MM_SAVE_STATE_LOOKUP_ENTRY
///
#define MM_CPU_OFFSET(Field)  OFFSET_OF (QEMU_SMRAM_SAVE_STATE_MAP, Field)

///
/// Lookup table used to retrieve the widths and offsets associated with each
/// supported EFI_MM_SAVE_STATE_REGISTER value
///
CONST CPU_MM_SAVE_STATE_LOOKUP_ENTRY  mCpuWidthOffset[] = {
  {
    0,                                    // Width32
    0,                                    // Width64
    0,                                    // Offset32
    0,                                    // Offset64Lo
    0,                                    // Offset64Hi
    FALSE                                 // Writeable
  }, // Reserved

  //
  // CPU Save State registers defined in PI SMM CPU Protocol.
  //
  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._GDTRBase),       // Offset64Lo
    MM_CPU_OFFSET (x64._GDTRBase) + 4,   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_GDTBASE = 4

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._IDTRBase),       // Offset64Lo
    MM_CPU_OFFSET (x64._IDTRBase) + 4,   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_IDTBASE = 5

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._LDTRBase),       // Offset64Lo
    MM_CPU_OFFSET (x64._LDTRBase) + 4,   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_LDTBASE = 6

  {
    0,                                   // Width32
    0,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._GDTRLimit),      // Offset64Lo
    MM_CPU_OFFSET (x64._GDTRLimit) + 4,  // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_GDTLIMIT = 7

  {
    0,                                   // Width32
    0,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._IDTRLimit),      // Offset64Lo
    MM_CPU_OFFSET (x64._IDTRLimit) + 4,  // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_IDTLIMIT = 8

  {
    0,                                   // Width32
    0,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._LDTRLimit),      // Offset64Lo
    MM_CPU_OFFSET (x64._LDTRLimit) + 4,  // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_LDTLIMIT = 9

  {
    0,                                    // Width32
    0,                                    // Width64
    0,                                    // Offset32
    0,                                    // Offset64Lo
    0 + 4,                                // Offset64Hi
    FALSE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_LDTINFO = 10

  {
    4,                                   // Width32
    4,                                   // Width64
    MM_CPU_OFFSET (x86._ES),             // Offset32
    MM_CPU_OFFSET (x64._ES),             // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_ES = 20

  {
    4,                                   // Width32
    4,                                   // Width64
    MM_CPU_OFFSET (x86._CS),             // Offset32
    MM_CPU_OFFSET (x64._CS),             // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_CS = 21

  {
    4,                                   // Width32
    4,                                   // Width64
    MM_CPU_OFFSET (x86._SS),             // Offset32
    MM_CPU_OFFSET (x64._SS),             // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_SS = 22

  {
    4,                                   // Width32
    4,                                   // Width64
    MM_CPU_OFFSET (x86._DS),             // Offset32
    MM_CPU_OFFSET (x64._DS),             // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_DS = 23

  {
    4,                                   // Width32
    4,                                   // Width64
    MM_CPU_OFFSET (x86._FS),             // Offset32
    MM_CPU_OFFSET (x64._FS),             // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_FS = 24

  {
    4,                                   // Width32
    4,                                   // Width64
    MM_CPU_OFFSET (x86._GS),             // Offset32
    MM_CPU_OFFSET (x64._GS),             // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_GS = 25

  {
    0,                                   // Width32
    4,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._LDTR),           // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_LDTR_SEL = 26

  {
    4,                                   // Width32
    4,                                   // Width64
    MM_CPU_OFFSET (x86._TR),             // Offset32
    MM_CPU_OFFSET (x64._TR),             // Offset64Lo
    0,                                   // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_TR_SEL = 27

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._DR7),            // Offset32
    MM_CPU_OFFSET (x64._DR7),            // Offset64Lo
    MM_CPU_OFFSET (x64._DR7) + 4,        // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_DR7 = 28

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._DR6),            // Offset32
    MM_CPU_OFFSET (x64._DR6),            // Offset64Lo
    MM_CPU_OFFSET (x64._DR6) + 4,        // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_DR6 = 29

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R8),             // Offset64Lo
    MM_CPU_OFFSET (x64._R8) + 4,         // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R8 = 30

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R9),             // Offset64Lo
    MM_CPU_OFFSET (x64._R9) + 4,         // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R9 = 31

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R10),            // Offset64Lo
    MM_CPU_OFFSET (x64._R10) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R10 = 32

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R11),            // Offset64Lo
    MM_CPU_OFFSET (x64._R11) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R11 = 33

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R12),            // Offset64Lo
    MM_CPU_OFFSET (x64._R12) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R12 = 34

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R13),            // Offset64Lo
    MM_CPU_OFFSET (x64._R13) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R13 = 35

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R14),            // Offset64Lo
    MM_CPU_OFFSET (x64._R14) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R14 = 36

  {
    0,                                   // Width32
    8,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._R15),            // Offset64Lo
    MM_CPU_OFFSET (x64._R15) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_R15 = 37

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._EAX),            // Offset32
    MM_CPU_OFFSET (x64._RAX),            // Offset64Lo
    MM_CPU_OFFSET (x64._RAX) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RAX = 38

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._EBX),            // Offset32
    MM_CPU_OFFSET (x64._RBX),            // Offset64Lo
    MM_CPU_OFFSET (x64._RBX) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RBX = 39

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._ECX),            // Offset32
    MM_CPU_OFFSET (x64._RCX),            // Offset64Lo
    MM_CPU_OFFSET (x64._RCX) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RCX = 40

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._EDX),            // Offset32
    MM_CPU_OFFSET (x64._RDX),            // Offset64Lo
    MM_CPU_OFFSET (x64._RDX) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RDX = 41

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._ESP),            // Offset32
    MM_CPU_OFFSET (x64._RSP),            // Offset64Lo
    MM_CPU_OFFSET (x64._RSP) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RSP = 42

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._EBP),            // Offset32
    MM_CPU_OFFSET (x64._RBP),            // Offset64Lo
    MM_CPU_OFFSET (x64._RBP) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RBP = 43

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._ESI),            // Offset32
    MM_CPU_OFFSET (x64._RSI),            // Offset64Lo
    MM_CPU_OFFSET (x64._RSI) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RSI = 44

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._EDI),            // Offset32
    MM_CPU_OFFSET (x64._RDI),            // Offset64Lo
    MM_CPU_OFFSET (x64._RDI) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RDI = 45

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._EIP),            // Offset32
    MM_CPU_OFFSET (x64._RIP),            // Offset64Lo
    MM_CPU_OFFSET (x64._RIP) + 4,        // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RIP = 46

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._EFLAGS),         // Offset32
    MM_CPU_OFFSET (x64._RFLAGS),         // Offset64Lo
    MM_CPU_OFFSET (x64._RFLAGS) + 4,     // Offset64Hi
    TRUE                                 // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_RFLAGS = 51

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._CR0),            // Offset32
    MM_CPU_OFFSET (x64._CR0),            // Offset64Lo
    MM_CPU_OFFSET (x64._CR0) + 4,        // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_CR0 = 52

  {
    4,                                   // Width32
    8,                                   // Width64
    MM_CPU_OFFSET (x86._CR3),            // Offset32
    MM_CPU_OFFSET (x64._CR3),            // Offset64Lo
    MM_CPU_OFFSET (x64._CR3) + 4,        // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_CR3 = 53

  {
    0,                                   // Width32
    4,                                   // Width64
    0,                                   // Offset32
    MM_CPU_OFFSET (x64._CR4),            // Offset64Lo
    MM_CPU_OFFSET (x64._CR4) + 4,        // Offset64Hi
    FALSE                                // Writeable
  }, // EFI_MM_SAVE_STATE_REGISTER_CR4 = 54
};

/**
  Read a save state register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for reading the
  MM Save State register.

  @param[in]  CpuIndex  The index of the CPU to read the Save State register.
                        The value must be between 0 and the NumberOfCpus field in
                        the System Management System Table (SMST).
  @param[in]  Register  The MM Save State register to read.
  @param[in]  Width     The number of bytes to read from the CPU save state.
  @param[out] Buffer    Upon return, this holds the CPU register value read
                        from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_INVALID_PARAMTER  Buffer is NULL.
  @retval EFI_UNSUPPORTED       This function does not support reading Register.
  @retval EFI_NOT_FOUND         If desired Register not found.
**/
EFI_STATUS
EFIAPI
MmSaveStateReadRegister (
  IN  UINTN                       CpuIndex,
  IN  EFI_MM_SAVE_STATE_REGISTER  Register,
  IN  UINTN                       Width,
  OUT VOID                        *Buffer
  )
{
  UINTN                      RegisterIndex;
  QEMU_SMRAM_SAVE_STATE_MAP  *CpuSaveState;

  //
  // Check for special EFI_MM_SAVE_STATE_REGISTER_LMA
  //
  if (Register == EFI_MM_SAVE_STATE_REGISTER_LMA) {
    //
    // Only byte access is supported for this register
    //
    if (Width != 1) {
      return EFI_INVALID_PARAMETER;
    }

    CpuSaveState = (QEMU_SMRAM_SAVE_STATE_MAP *)gSmst->CpuSaveState[CpuIndex];

    //
    // Check CPU mode
    //
    if ((CpuSaveState->x86.SMMRevId & 0xFFFF) == 0) {
      *(UINT8 *)Buffer = 32;
    } else {
      *(UINT8 *)Buffer = 64;
    }

    return EFI_SUCCESS;
  }

  //
  // Check for special EFI_MM_SAVE_STATE_REGISTER_IO
  //
  if (Register == EFI_MM_SAVE_STATE_REGISTER_IO) {
    return EFI_NOT_FOUND;
  }

  //
  // Convert Register to a register lookup table index.  Let
  // PiSmmCpuDxeSmm implement other special registers (currently
  // there is only EFI_MM_SAVE_STATE_REGISTER_PROCESSOR_ID).
  //
  RegisterIndex = MmSaveStateGetRegisterIndex (Register);
  if (RegisterIndex == 0) {
    return (Register < EFI_MM_SAVE_STATE_REGISTER_IO ?
            EFI_NOT_FOUND :
            EFI_UNSUPPORTED);
  }

  return MmSaveStateReadRegisterByIndex (CpuIndex, RegisterIndex, Width, Buffer);
}

/**
  Writes a save state register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for writing the
  MM save state register.

  @param[in] CpuIndex  The index of the CPU to write the MM Save State.  The
                       value must be between 0 and the NumberOfCpus field in
                       the System Management System Table (SMST).
  @param[in] Register  The MM Save State register to write.
  @param[in] Width     The number of bytes to write to the CPU save state.
  @param[in] Buffer    Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS           The register was written to Save State.
  @retval EFI_INVALID_PARAMTER  Buffer is NULL.
  @retval EFI_UNSUPPORTED       This function does not support writing Register.
  @retval EFI_NOT_FOUND         If desired Register not found.
**/
EFI_STATUS
EFIAPI
MmSaveStateWriteRegister (
  IN UINTN                       CpuIndex,
  IN EFI_MM_SAVE_STATE_REGISTER  Register,
  IN UINTN                       Width,
  IN CONST VOID                  *Buffer
  )
{
  UINTN                      RegisterIndex;
  QEMU_SMRAM_SAVE_STATE_MAP  *CpuSaveState;

  //
  // Writes to EFI_MM_SAVE_STATE_REGISTER_LMA are ignored
  //
  if (Register == EFI_MM_SAVE_STATE_REGISTER_LMA) {
    return EFI_SUCCESS;
  }

  //
  // Writes to EFI_MM_SAVE_STATE_REGISTER_IO are not supported
  //
  if (Register == EFI_MM_SAVE_STATE_REGISTER_IO) {
    return EFI_NOT_FOUND;
  }

  //
  // Convert Register to a register lookup table index.  Let
  // PiSmmCpuDxeSmm implement other special registers (currently
  // there is only EFI_MM_SAVE_STATE_REGISTER_PROCESSOR_ID).
  //
  RegisterIndex = MmSaveStateGetRegisterIndex (Register);
  if (RegisterIndex == 0) {
    return (Register < EFI_MM_SAVE_STATE_REGISTER_IO ?
            EFI_NOT_FOUND :
            EFI_UNSUPPORTED);
  }

  CpuSaveState = (QEMU_SMRAM_SAVE_STATE_MAP *)gSmst->CpuSaveState[CpuIndex];

  //
  // Do not write non-writable SaveState, because it will cause exception.
  //
  if (!mCpuWidthOffset[RegisterIndex].Writeable) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check CPU mode
  //
  if ((CpuSaveState->x86.SMMRevId & 0xFFFF) == 0) {
    //
    // If 32-bit mode width is zero, then the specified register can not be
    // accessed
    //
    if (mCpuWidthOffset[RegisterIndex].Width32 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 32-bit mode width, then the specified
    // register can not be accessed
    //
    if (Width > mCpuWidthOffset[RegisterIndex].Width32) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write SMM State register
    //
    ASSERT (CpuSaveState != NULL);
    CopyMem (
      (UINT8 *)CpuSaveState + mCpuWidthOffset[RegisterIndex].Offset32,
      Buffer,
      Width
      );
  } else {
    //
    // If 64-bit mode width is zero, then the specified register can not be
    // accessed
    //
    if (mCpuWidthOffset[RegisterIndex].Width64 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 64-bit mode width, then the specified
    // register can not be accessed
    //
    if (Width > mCpuWidthOffset[RegisterIndex].Width64) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write lower 32-bits of SMM State register
    //
    CopyMem (
      (UINT8 *)CpuSaveState + mCpuWidthOffset[RegisterIndex].Offset64Lo,
      Buffer,
      MIN (4, Width)
      );
    if (Width >= 4) {
      //
      // Write upper 32-bits of SMM State register
      //
      CopyMem (
        (UINT8 *)CpuSaveState + mCpuWidthOffset[RegisterIndex].Offset64Hi,
        (UINT8 *)Buffer + 4,
        Width - 4
        );
    }
  }

  return EFI_SUCCESS;
}

/**
  Returns LMA value of the Processor.

  @param[in]  CpuIndex  Specifies the zero-based index of the CPU save state.

  @retval     UINT8     returns LMA bit value.
**/
UINT8
EFIAPI
MmSaveStateGetRegisterLma (
  IN UINTN  CpuIndex
  )
{
  QEMU_SMRAM_SAVE_STATE_MAP  *CpuSaveState;

  CpuSaveState = (QEMU_SMRAM_SAVE_STATE_MAP *)gSmst->CpuSaveState[CpuIndex];

  if ((CpuSaveState->x86.SMMRevId & 0xFFFF) == 0) {
    return EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT;
  }

  return EFI_MM_SAVE_STATE_REGISTER_LMA_64BIT;
}
