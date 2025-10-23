/** @file
Provides services to access SMRAM Save State Map

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MmSaveState.h"
#include <Register/Amd/SmramSaveStateMap.h>
#include <Library/BaseLib.h>
#include <Library/AmdSysCallLib.h>

#define AMD_MM_SAVE_STATE_REGISTER_SMMREVID_INDEX  1
#define AMD_MM_SAVE_STATE_REGISTER_MAX_INDEX       2

// Macro used to simplify the lookup table entries of type CPU_MM_SAVE_STATE_LOOKUP_ENTRY
#define MM_CPU_OFFSET(Field)  OFFSET_OF (AMD_SMRAM_SAVE_STATE_MAP, Field)

//
// Lookup table used to retrieve the widths and offsets associated with each
// supported EFI_MM_SAVE_STATE_REGISTER value
//
//  Per AMD64 Architecture Programmer's Manual Volume 2: System
//  Programming - 10.2.3 SMRAM State-Save Area (Rev 24593), the AMD64
//  architecture does not use the legacy SMM state-save area format
//  (Table 10-2) for 32-bit SMRAM Save State Map.
//
CONST CPU_MM_SAVE_STATE_LOOKUP_ENTRY  mCpuWidthOffset[] = {
  { 0, 0, 0, 0,                                    0,                                    FALSE },  //  Reserved

  //
  // Internally defined CPU Save State Registers. Not defined in PI SMM CPU Protocol.
  //
  { 0, 4, 0, MM_CPU_OFFSET (x64.SMMRevId),         0,                                    FALSE },  // AMD_MM_SAVE_STATE_REGISTER_SMMREVID_INDEX  = 1

  //
  // CPU Save State registers defined in PI SMM CPU Protocol.
  //
  { 0, 8, 0, MM_CPU_OFFSET (x64._GDTRBaseLoDword), MM_CPU_OFFSET (x64._GDTRBaseHiDword), FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_GDTBASE  = 4
  { 0, 8, 0, MM_CPU_OFFSET (x64._IDTRBaseLoDword), MM_CPU_OFFSET (x64._IDTRBaseLoDword), FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_IDTBASE  = 5
  { 0, 8, 0, MM_CPU_OFFSET (x64._LDTRBaseLoDword), MM_CPU_OFFSET (x64._LDTRBaseLoDword), FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_LDTBASE  = 6
  { 0, 2, 0, MM_CPU_OFFSET (x64._GDTRLimit),       0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_GDTLIMIT = 7
  { 0, 2, 0, MM_CPU_OFFSET (x64._IDTRLimit),       0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_IDTLIMIT = 8
  { 0, 4, 0, MM_CPU_OFFSET (x64._LDTRLimit),       0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_LDTLIMIT = 9
  { 0, 0, 0, 0,                                    0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_LDTINFO  = 10
  { 0, 2, 0, MM_CPU_OFFSET (x64._ES),              0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_ES       = 20
  { 0, 2, 0, MM_CPU_OFFSET (x64._CS),              0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_CS       = 21
  { 0, 2, 0, MM_CPU_OFFSET (x64._SS),              0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_SS       = 22
  { 0, 2, 0, MM_CPU_OFFSET (x64._DS),              0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_DS       = 23
  { 0, 2, 0, MM_CPU_OFFSET (x64._FS),              0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_FS       = 24
  { 0, 2, 0, MM_CPU_OFFSET (x64._GS),              0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_GS       = 25
  { 0, 2, 0, MM_CPU_OFFSET (x64._LDTR),            0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_LDTR_SEL = 26
  { 0, 2, 0, MM_CPU_OFFSET (x64._TR),              0,                                    FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_TR_SEL   = 27
  { 0, 8, 0, MM_CPU_OFFSET (x64._DR7),             MM_CPU_OFFSET (x64._DR7)         + 4, FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_DR7      = 28
  { 0, 8, 0, MM_CPU_OFFSET (x64._DR6),             MM_CPU_OFFSET (x64._DR6)         + 4, FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_DR6      = 29
  { 0, 8, 0, MM_CPU_OFFSET (x64._R8),              MM_CPU_OFFSET (x64._R8)          + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R8       = 30
  { 0, 8, 0, MM_CPU_OFFSET (x64._R9),              MM_CPU_OFFSET (x64._R9)          + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R9       = 31
  { 0, 8, 0, MM_CPU_OFFSET (x64._R10),             MM_CPU_OFFSET (x64._R10)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R10      = 32
  { 0, 8, 0, MM_CPU_OFFSET (x64._R11),             MM_CPU_OFFSET (x64._R11)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R11      = 33
  { 0, 8, 0, MM_CPU_OFFSET (x64._R12),             MM_CPU_OFFSET (x64._R12)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R12      = 34
  { 0, 8, 0, MM_CPU_OFFSET (x64._R13),             MM_CPU_OFFSET (x64._R13)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R13      = 35
  { 0, 8, 0, MM_CPU_OFFSET (x64._R14),             MM_CPU_OFFSET (x64._R14)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R14      = 36
  { 0, 8, 0, MM_CPU_OFFSET (x64._R15),             MM_CPU_OFFSET (x64._R15)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_R15      = 37
  { 0, 8, 0, MM_CPU_OFFSET (x64._RAX),             MM_CPU_OFFSET (x64._RAX)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RAX      = 38
  { 0, 8, 0, MM_CPU_OFFSET (x64._RBX),             MM_CPU_OFFSET (x64._RBX)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RBX      = 39
  { 0, 8, 0, MM_CPU_OFFSET (x64._RCX),             MM_CPU_OFFSET (x64._RCX)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RBX      = 39
  { 0, 8, 0, MM_CPU_OFFSET (x64._RDX),             MM_CPU_OFFSET (x64._RDX)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RDX      = 41
  { 0, 8, 0, MM_CPU_OFFSET (x64._RSP),             MM_CPU_OFFSET (x64._RSP)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RSP      = 42
  { 0, 8, 0, MM_CPU_OFFSET (x64._RBP),             MM_CPU_OFFSET (x64._RBP)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RBP      = 43
  { 0, 8, 0, MM_CPU_OFFSET (x64._RSI),             MM_CPU_OFFSET (x64._RSI)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RSI      = 44
  { 0, 8, 0, MM_CPU_OFFSET (x64._RDI),             MM_CPU_OFFSET (x64._RDI)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RDI      = 45
  { 0, 8, 0, MM_CPU_OFFSET (x64._RIP),             MM_CPU_OFFSET (x64._RIP)         + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RIP      = 46

  { 0, 8, 0, MM_CPU_OFFSET (x64._RFLAGS),          MM_CPU_OFFSET (x64._RFLAGS)      + 4, TRUE  },  //  EFI_MM_SAVE_STATE_REGISTER_RFLAGS   = 51
  { 0, 8, 0, MM_CPU_OFFSET (x64._CR0),             MM_CPU_OFFSET (x64._CR0)         + 4, FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_CR0      = 52
  { 0, 8, 0, MM_CPU_OFFSET (x64._CR3),             MM_CPU_OFFSET (x64._CR3)         + 4, FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_CR3      = 53
  { 0, 8, 0, MM_CPU_OFFSET (x64._CR4),             MM_CPU_OFFSET (x64._CR4)         + 4, FALSE },  //  EFI_MM_SAVE_STATE_REGISTER_CR4      = 54
  { 0, 0, 0, 0,                                    0,                                    FALSE }
};

// Table used by MmSaveStateGetRegisterIndex() to convert an EFI_MM_SAVE_STATE_REGISTER
// value to an index into a table of type CPU_MM_SAVE_STATE_LOOKUP_ENTRY
CONST CPU_MM_SAVE_STATE_REGISTER_RANGE  mCpuRegisterRanges[] = {
  MM_REGISTER_RANGE (EFI_MM_SAVE_STATE_REGISTER_GDTBASE, EFI_MM_SAVE_STATE_REGISTER_LDTINFO),
  MM_REGISTER_RANGE (EFI_MM_SAVE_STATE_REGISTER_ES,      EFI_MM_SAVE_STATE_REGISTER_RIP),
  MM_REGISTER_RANGE (EFI_MM_SAVE_STATE_REGISTER_RFLAGS,  EFI_MM_SAVE_STATE_REGISTER_CR4),
  { (EFI_MM_SAVE_STATE_REGISTER)0,                       (EFI_MM_SAVE_STATE_REGISTER)0,      0}
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
  EFI_MM_SAVE_STATE_IO_INFO  *IoInfo;
  AMD_SMRAM_SAVE_STATE_MAP   *CpuSaveState;
  UINT8                      DataWidth;
  UINT32                     SaveStateIoDword;

  // Read CPU State
  CpuSaveState = (AMD_SMRAM_SAVE_STATE_MAP *)gMmst->CpuSaveState[CpuIndex];

  // Check for special EFI_MM_SAVE_STATE_REGISTER_LMA
  if (Register == EFI_MM_SAVE_STATE_REGISTER_LMA) {
    // Only byte access is supported for this register
    if (Width != 1) {
      return EFI_INVALID_PARAMETER;
    }

    *(UINT8 *)Buffer = MmSaveStateGetRegisterLma ();

    return EFI_SUCCESS;
  }

  // Check for special EFI_MM_SAVE_STATE_REGISTER_IO
  if (Register == EFI_MM_SAVE_STATE_REGISTER_IO) {
    if (NeedSysCall () == TRUE) {
      SaveStateIoDword = (UINT32)SysCall (
                                   SMM_SC_SVST_READ,
                                   CpuIndex,
                                   (UINTN)&(CpuSaveState->x64.IO_DWord),
                                   sizeof (CpuSaveState->x64.IO_DWord)
                                   );
    } else {
      SaveStateIoDword = CpuSaveState->x64.IO_DWord;
    }

    // Check if IO Restart Dword [IO Trap] is valid or not using bit 1.
    if (!(SaveStateIoDword & 0x02u)) {
      return EFI_NOT_FOUND;
    }

    // Zero the IoInfo structure that will be returned in Buffer
    IoInfo = (EFI_MM_SAVE_STATE_IO_INFO *)Buffer;
    ZeroMem (IoInfo, sizeof (EFI_MM_SAVE_STATE_IO_INFO));

    IoInfo->IoPort = (UINT16)(SaveStateIoDword >> 16u);

    if (SaveStateIoDword & 0x10u) {
      IoInfo->IoWidth = EFI_MM_SAVE_STATE_IO_WIDTH_UINT8;
      DataWidth       = 0x01u;
    } else if (SaveStateIoDword & 0x20u) {
      IoInfo->IoWidth = EFI_MM_SAVE_STATE_IO_WIDTH_UINT16;
      DataWidth       = 0x02u;
    } else {
      IoInfo->IoWidth = EFI_MM_SAVE_STATE_IO_WIDTH_UINT32;
      DataWidth       = 0x04u;
    }

    if (SaveStateIoDword & 0x01u) {
      IoInfo->IoType = EFI_MM_SAVE_STATE_IO_TYPE_INPUT;
    } else {
      IoInfo->IoType = EFI_MM_SAVE_STATE_IO_TYPE_OUTPUT;
    }

    if ((IoInfo->IoType == EFI_MM_SAVE_STATE_IO_TYPE_INPUT) || (IoInfo->IoType == EFI_MM_SAVE_STATE_IO_TYPE_OUTPUT)) {
      MmSaveStateReadRegister (CpuIndex, EFI_MM_SAVE_STATE_REGISTER_RAX, DataWidth, &IoInfo->IoData);
    }

    return EFI_SUCCESS;
  }

  // Convert Register to a register lookup table index
  return MmSaveStateReadRegisterByIndex (
           CpuIndex,
           MmSaveStateGetRegisterIndex (Register, AMD_MM_SAVE_STATE_REGISTER_MAX_INDEX),
           Width,
           Buffer
           );
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
  UINTN                     RegisterIndex;
  AMD_SMRAM_SAVE_STATE_MAP  *CpuSaveState;

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
  // Convert Register to a register lookup table index
  //
  RegisterIndex = MmSaveStateGetRegisterIndex (Register, AMD_MM_SAVE_STATE_REGISTER_MAX_INDEX);
  if (RegisterIndex == 0) {
    return EFI_NOT_FOUND;
  }

  CpuSaveState = gMmst->CpuSaveState[CpuIndex];

  //
  // Do not write non-writable SaveState, because it will cause exception.
  //
  if (!mCpuWidthOffset[RegisterIndex].Writeable) {
    return EFI_UNSUPPORTED;
  }

  //
  // If 64-bit mode width is zero, then the specified register can not be accessed
  //
  if (mCpuWidthOffset[RegisterIndex].Width64 == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // If Width is bigger than the 64-bit mode width, then the specified register can not be accessed
  //
  if (Width > mCpuWidthOffset[RegisterIndex].Width64) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Write lower 32-bits of SMM State register
  //
  CopyMem ((UINT8 *)CpuSaveState + mCpuWidthOffset[RegisterIndex].Offset64Lo, Buffer, MIN (4, Width));
  if (Width >= 4) {
    //
    // Write upper 32-bits of SMM State register
    //
    CopyMem ((UINT8 *)CpuSaveState + mCpuWidthOffset[RegisterIndex].Offset64Hi, (UINT8 *)Buffer + 4, Width - 4);
  }

  return EFI_SUCCESS;
}

/**
  Returns LMA value of the Processor.

  @retval     UINT8     returns LMA bit value.
**/
UINT8
MmSaveStateGetRegisterLma (
  VOID
  )
{
  //
  // AMD64 processors support EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT only
  //
  return EFI_MM_SAVE_STATE_REGISTER_LMA_64BIT;
}

/**
  Read information from the CPU save state.

  @param  Register  Specifies the CPU register to read form the save state.
  @param  RegOffset Offset for the next register index.

  @retval 0   Register is not valid
  @retval >0  Index into mCpuWidthOffset[] associated with Register

**/
UINTN
MmSaveStateGetRegisterIndex (
  IN EFI_MM_SAVE_STATE_REGISTER  Register,
  IN UINTN                       RegOffset
  )
{
  UINTN  Index;
  UINTN  Offset;

  for (Index = 0, Offset = RegOffset; mCpuRegisterRanges[Index].Length != 0; Index++) {
    if ((Register >= mCpuRegisterRanges[Index].Start) && (Register <= mCpuRegisterRanges[Index].End)) {
      return Register - mCpuRegisterRanges[Index].Start + Offset;
    }

    Offset += mCpuRegisterRanges[Index].Length;
  }

  return 0;
}

/**
  Read a CPU Save State register on the target processor.

  This function abstracts the differences that whether the CPU Save State register is in the
  IA32 CPU Save State Map or X64 CPU Save State Map.

  This function supports reading a CPU Save State register in SMBase relocation handler.

  @param[in]  CpuIndex       Specifies the zero-based index of the CPU save state.
  @param[in]  RegisterIndex  Index into mCpuWidthOffset[] look up table.
  @param[in]  Width          The number of bytes to read from the CPU save state.
  @param[out] Buffer         Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
  @retval EFI_INVALID_PARAMTER  This or Buffer is NULL.

**/
EFI_STATUS
MmSaveStateReadRegisterByIndex (
  IN UINTN  CpuIndex,
  IN UINTN  RegisterIndex,
  IN UINTN  Width,
  OUT VOID  *Buffer
  )
{
  if (RegisterIndex == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // If 64-bit mode width is zero, then the specified register can not be accessed
  //
  if (mCpuWidthOffset[RegisterIndex].Width64 == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // If Width is bigger than the 64-bit mode width, then the specified register can not be accessed
  //
  if (Width > mCpuWidthOffset[RegisterIndex].Width64) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Write lower 32-bits of return buffer
  //
  if (NeedSysCall () == TRUE) {
    *(UINT32 *)Buffer = (UINT32)SysCall (
                                  SMM_SC_SVST_READ,
                                  CpuIndex,
                                  (UINTN)((UINT8 *)gMmst->CpuSaveState[CpuIndex]
                                          + mCpuWidthOffset[RegisterIndex].Offset64Lo),
                                  MIN (4, Width)
                                  );
  } else {
    CopyMem (
      Buffer,
      (UINT8 *)gMmst->CpuSaveState[CpuIndex] + mCpuWidthOffset[RegisterIndex].Offset64Lo,
      MIN (4, Width)
      );
  }

  if (Width > 4) {
    //
    // Write upper 32-bits of return buffer
    //
    if (NeedSysCall () == TRUE) {
      *((UINT32 *)Buffer + 1) = (UINT32)SysCall (
                                          SMM_SC_SVST_READ,
                                          CpuIndex,
                                          (UINTN)((UINT8 *)gMmst->CpuSaveState[CpuIndex] + mCpuWidthOffset[RegisterIndex].Offset64Hi),
                                          Width - 4
                                          );
    } else {
      CopyMem (
        (UINT8 *)Buffer + 4,
        (UINT8 *)gMmst->CpuSaveState[CpuIndex]
        + mCpuWidthOffset[RegisterIndex].Offset64Hi,
        Width - 4
        );
    }
  }

  return EFI_SUCCESS;
}
