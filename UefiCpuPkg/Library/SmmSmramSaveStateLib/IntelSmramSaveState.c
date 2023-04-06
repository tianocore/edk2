/** @file
Provides services to access SMRAM Save State Map

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmramSaveState.h"
#include <Register/Intel/SmramSaveStateMap.h>
#include <Register/Intel/Cpuid.h>
#include <Library/BaseLib.h>

#define SMM_SAVE_STATE_REGISTER_SMMREVID_INDEX   1
#define SMM_SAVE_STATE_REGISTER_IOMISC_INDEX     2
#define SMM_SAVE_STATE_REGISTER_IOMEMADDR_INDEX  3
#define INTEL_SMM_SAVE_STATE_REGISTER_MAX_INDEX  4

///
/// Macro used to simplify the lookup table entries of type CPU_SMM_SAVE_STATE_LOOKUP_ENTRY
///
#define SMM_CPU_OFFSET(Field)  OFFSET_OF (SMRAM_SAVE_STATE_MAP, Field)

///
/// Lookup table used to retrieve the widths and offsets associated with each
/// supported EFI_SMM_SAVE_STATE_REGISTER value
///
CONST CPU_SMM_SAVE_STATE_LOOKUP_ENTRY  mSmmSmramCpuWidthOffset[] = {
  { 0, 0, 0,                              0,                                   0,                                   FALSE },  //  Reserved

  //
  // Internally defined CPU Save State Registers. Not defined in PI SMM CPU Protocol.
  //
  { 4, 4, SMM_CPU_OFFSET (x86.SMMRevId),  SMM_CPU_OFFSET (x64.SMMRevId),       0,                                   FALSE }, // SMM_SAVE_STATE_REGISTER_SMMREVID_INDEX  = 1
  { 4, 4, SMM_CPU_OFFSET (x86.IOMisc),    SMM_CPU_OFFSET (x64.IOMisc),         0,                                   FALSE }, // SMM_SAVE_STATE_REGISTER_IOMISC_INDEX    = 2
  { 4, 8, SMM_CPU_OFFSET (x86.IOMemAddr), SMM_CPU_OFFSET (x64.IOMemAddr),      SMM_CPU_OFFSET (x64.IOMemAddr) + 4,  FALSE }, // SMM_SAVE_STATE_REGISTER_IOMEMADDR_INDEX = 3

  //
  // CPU Save State registers defined in PI SMM CPU Protocol.
  //
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64.GdtBaseLoDword), SMM_CPU_OFFSET (x64.GdtBaseHiDword), FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_GDTBASE  = 4
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64.IdtBaseLoDword), SMM_CPU_OFFSET (x64.IdtBaseHiDword), FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_IDTBASE  = 5
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64.LdtBaseLoDword), SMM_CPU_OFFSET (x64.LdtBaseHiDword), FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_LDTBASE  = 6
  { 0, 0, 0,                              0,                                   0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_GDTLIMIT = 7
  { 0, 0, 0,                              0,                                   0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_IDTLIMIT = 8
  { 0, 0, 0,                              0,                                   0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_LDTLIMIT = 9
  { 0, 0, 0,                              0,                                   0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_LDTINFO  = 10

  { 4, 4, SMM_CPU_OFFSET (x86._ES),       SMM_CPU_OFFSET (x64._ES),            0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_ES       = 20
  { 4, 4, SMM_CPU_OFFSET (x86._CS),       SMM_CPU_OFFSET (x64._CS),            0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_CS       = 21
  { 4, 4, SMM_CPU_OFFSET (x86._SS),       SMM_CPU_OFFSET (x64._SS),            0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_SS       = 22
  { 4, 4, SMM_CPU_OFFSET (x86._DS),       SMM_CPU_OFFSET (x64._DS),            0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_DS       = 23
  { 4, 4, SMM_CPU_OFFSET (x86._FS),       SMM_CPU_OFFSET (x64._FS),            0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_FS       = 24
  { 4, 4, SMM_CPU_OFFSET (x86._GS),       SMM_CPU_OFFSET (x64._GS),            0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_GS       = 25
  { 0, 4, 0,                              SMM_CPU_OFFSET (x64._LDTR),          0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_LDTR_SEL = 26
  { 4, 4, SMM_CPU_OFFSET (x86._TR),       SMM_CPU_OFFSET (x64._TR),            0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_TR_SEL   = 27
  { 4, 8, SMM_CPU_OFFSET (x86._DR7),      SMM_CPU_OFFSET (x64._DR7),           SMM_CPU_OFFSET (x64._DR7)    + 4,    FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_DR7      = 28
  { 4, 8, SMM_CPU_OFFSET (x86._DR6),      SMM_CPU_OFFSET (x64._DR6),           SMM_CPU_OFFSET (x64._DR6)    + 4,    FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_DR6      = 29
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R8),            SMM_CPU_OFFSET (x64._R8)     + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R8       = 30
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R9),            SMM_CPU_OFFSET (x64._R9)     + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R9       = 31
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R10),           SMM_CPU_OFFSET (x64._R10)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R10      = 32
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R11),           SMM_CPU_OFFSET (x64._R11)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R11      = 33
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R12),           SMM_CPU_OFFSET (x64._R12)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R12      = 34
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R13),           SMM_CPU_OFFSET (x64._R13)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R13      = 35
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R14),           SMM_CPU_OFFSET (x64._R14)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R14      = 36
  { 0, 8, 0,                              SMM_CPU_OFFSET (x64._R15),           SMM_CPU_OFFSET (x64._R15)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_R15      = 37
  { 4, 8, SMM_CPU_OFFSET (x86._EAX),      SMM_CPU_OFFSET (x64._RAX),           SMM_CPU_OFFSET (x64._RAX)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RAX      = 38
  { 4, 8, SMM_CPU_OFFSET (x86._EBX),      SMM_CPU_OFFSET (x64._RBX),           SMM_CPU_OFFSET (x64._RBX)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RBX      = 39
  { 4, 8, SMM_CPU_OFFSET (x86._ECX),      SMM_CPU_OFFSET (x64._RCX),           SMM_CPU_OFFSET (x64._RCX)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RCX      = 40
  { 4, 8, SMM_CPU_OFFSET (x86._EDX),      SMM_CPU_OFFSET (x64._RDX),           SMM_CPU_OFFSET (x64._RDX)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RDX      = 41
  { 4, 8, SMM_CPU_OFFSET (x86._ESP),      SMM_CPU_OFFSET (x64._RSP),           SMM_CPU_OFFSET (x64._RSP)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RSP      = 42
  { 4, 8, SMM_CPU_OFFSET (x86._EBP),      SMM_CPU_OFFSET (x64._RBP),           SMM_CPU_OFFSET (x64._RBP)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RBP      = 43
  { 4, 8, SMM_CPU_OFFSET (x86._ESI),      SMM_CPU_OFFSET (x64._RSI),           SMM_CPU_OFFSET (x64._RSI)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RSI      = 44
  { 4, 8, SMM_CPU_OFFSET (x86._EDI),      SMM_CPU_OFFSET (x64._RDI),           SMM_CPU_OFFSET (x64._RDI)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RDI      = 45
  { 4, 8, SMM_CPU_OFFSET (x86._EIP),      SMM_CPU_OFFSET (x64._RIP),           SMM_CPU_OFFSET (x64._RIP)    + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RIP      = 46

  { 4, 8, SMM_CPU_OFFSET (x86._EFLAGS),   SMM_CPU_OFFSET (x64._RFLAGS),        SMM_CPU_OFFSET (x64._RFLAGS) + 4,    TRUE  }, //  EFI_SMM_SAVE_STATE_REGISTER_RFLAGS   = 51
  { 4, 8, SMM_CPU_OFFSET (x86._CR0),      SMM_CPU_OFFSET (x64._CR0),           SMM_CPU_OFFSET (x64._CR0)    + 4,    FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_CR0      = 52
  { 4, 8, SMM_CPU_OFFSET (x86._CR3),      SMM_CPU_OFFSET (x64._CR3),           SMM_CPU_OFFSET (x64._CR3)    + 4,    FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_CR3      = 53
  { 0, 4, 0,                              SMM_CPU_OFFSET (x64._CR4),           0,                                   FALSE }, //  EFI_SMM_SAVE_STATE_REGISTER_CR4      = 54
};

///
/// Structure used to build a lookup table for the IOMisc width information
///
typedef struct {
  UINT8                          Width;
  EFI_SMM_SAVE_STATE_IO_WIDTH    IoWidth;
} CPU_SMM_SAVE_STATE_IO_WIDTH;

///
/// Lookup table for the IOMisc width information
///
STATIC CONST CPU_SMM_SAVE_STATE_IO_WIDTH  mSmmCpuIoWidth[] = {
  { 0, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT8  },  // Undefined           = 0
  { 1, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT8  },  // SMM_IO_LENGTH_BYTE  = 1
  { 2, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT16 },  // SMM_IO_LENGTH_WORD  = 2
  { 0, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT8  },  // Undefined           = 3
  { 4, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT32 },  // SMM_IO_LENGTH_DWORD = 4
  { 0, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT8  },  // Undefined           = 5
  { 0, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT8  },  // Undefined           = 6
  { 0, EFI_SMM_SAVE_STATE_IO_WIDTH_UINT8  }   // Undefined           = 7
};

///
/// Lookup table for the IOMisc type information
///
STATIC CONST EFI_SMM_SAVE_STATE_IO_TYPE  mSmmCpuIoType[] = {
  EFI_SMM_SAVE_STATE_IO_TYPE_OUTPUT,     // SMM_IO_TYPE_OUT_DX        = 0
  EFI_SMM_SAVE_STATE_IO_TYPE_INPUT,      // SMM_IO_TYPE_IN_DX         = 1
  EFI_SMM_SAVE_STATE_IO_TYPE_STRING,     // SMM_IO_TYPE_OUTS          = 2
  EFI_SMM_SAVE_STATE_IO_TYPE_STRING,     // SMM_IO_TYPE_INS           = 3
  (EFI_SMM_SAVE_STATE_IO_TYPE)0,         // Undefined                 = 4
  (EFI_SMM_SAVE_STATE_IO_TYPE)0,         // Undefined                 = 5
  EFI_SMM_SAVE_STATE_IO_TYPE_REP_PREFIX, // SMM_IO_TYPE_REP_OUTS      = 6
  EFI_SMM_SAVE_STATE_IO_TYPE_REP_PREFIX, // SMM_IO_TYPE_REP_INS       = 7
  EFI_SMM_SAVE_STATE_IO_TYPE_OUTPUT,     // SMM_IO_TYPE_OUT_IMMEDIATE = 8
  EFI_SMM_SAVE_STATE_IO_TYPE_INPUT,      // SMM_IO_TYPE_OUT_IMMEDIATE = 9
  (EFI_SMM_SAVE_STATE_IO_TYPE)0,         // Undefined                 = 10
  (EFI_SMM_SAVE_STATE_IO_TYPE)0,         // Undefined                 = 11
  (EFI_SMM_SAVE_STATE_IO_TYPE)0,         // Undefined                 = 12
  (EFI_SMM_SAVE_STATE_IO_TYPE)0,         // Undefined                 = 13
  (EFI_SMM_SAVE_STATE_IO_TYPE)0,         // Undefined                 = 14
  (EFI_SMM_SAVE_STATE_IO_TYPE)0          // Undefined                 = 15
};

/**
  Read an SMM Save State register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for reading the
  SMM Save Sate register.

  @param[in]  CpuIndex  The index of the CPU to read the SMM Save State.  The
                        value must be between 0 and the NumberOfCpus field in
                        the System Management System Table (SMST).
  @param[in]  Register  The SMM Save State register to read.
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
SmramSaveStateReadRegister (
  IN  UINTN                        CpuIndex,
  IN  EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN  UINTN                        Width,
  OUT VOID                         *Buffer
  )
{
  UINT32                      SmmRevId;
  SMRAM_SAVE_STATE_IOMISC     IoMisc;
  EFI_SMM_SAVE_STATE_IO_INFO  *IoInfo;

  //
  // Check for special EFI_SMM_SAVE_STATE_REGISTER_LMA
  //
  if (Register == EFI_SMM_SAVE_STATE_REGISTER_LMA) {
    //
    // Only byte access is supported for this register
    //
    if (Width != 1) {
      return EFI_INVALID_PARAMETER;
    }

    *(UINT8 *)Buffer = SmramSaveStateGetRegisterLma ();

    return EFI_SUCCESS;
  }

  //
  // Check for special EFI_SMM_SAVE_STATE_REGISTER_IO
  //
  if (Register == EFI_SMM_SAVE_STATE_REGISTER_IO) {
    //
    // Get SMM Revision ID
    //
    SmramSaveStateReadRegisterByIndex (CpuIndex, SMM_SAVE_STATE_REGISTER_SMMREVID_INDEX, sizeof (SmmRevId), &SmmRevId);

    //
    // See if the CPU supports the IOMisc register in the save state
    //
    if (SmmRevId < SMRAM_SAVE_STATE_MIN_REV_ID_IOMISC) {
      return EFI_NOT_FOUND;
    }

    //
    // Get the IOMisc register value
    //
    SmramSaveStateReadRegisterByIndex (CpuIndex, SMM_SAVE_STATE_REGISTER_IOMISC_INDEX, sizeof (IoMisc.Uint32), &IoMisc.Uint32);

    //
    // Check for the SMI_FLAG in IOMisc
    //
    if (IoMisc.Bits.SmiFlag == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // Only support IN/OUT, but not INS/OUTS/REP INS/REP OUTS.
    //
    if ((mSmmCpuIoType[IoMisc.Bits.Type] != EFI_SMM_SAVE_STATE_IO_TYPE_INPUT) &&
        (mSmmCpuIoType[IoMisc.Bits.Type] != EFI_SMM_SAVE_STATE_IO_TYPE_OUTPUT))
    {
      return EFI_NOT_FOUND;
    }

    //
    // Compute index for the I/O Length and I/O Type lookup tables
    //
    if ((mSmmCpuIoWidth[IoMisc.Bits.Length].Width == 0) || (mSmmCpuIoType[IoMisc.Bits.Type] == 0)) {
      return EFI_NOT_FOUND;
    }

    //
    // Make sure the incoming buffer is large enough to hold IoInfo before accessing
    //
    if (Width < sizeof (EFI_SMM_SAVE_STATE_IO_INFO)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Zero the IoInfo structure that will be returned in Buffer
    //
    IoInfo = (EFI_SMM_SAVE_STATE_IO_INFO *)Buffer;
    ZeroMem (IoInfo, sizeof (EFI_SMM_SAVE_STATE_IO_INFO));

    //
    // Use lookup tables to help fill in all the fields of the IoInfo structure
    //
    IoInfo->IoPort  = (UINT16)IoMisc.Bits.Port;
    IoInfo->IoWidth = mSmmCpuIoWidth[IoMisc.Bits.Length].IoWidth;
    IoInfo->IoType  = mSmmCpuIoType[IoMisc.Bits.Type];
    SmramSaveStateReadRegister (CpuIndex, EFI_SMM_SAVE_STATE_REGISTER_RAX, mSmmCpuIoWidth[IoMisc.Bits.Length].Width, &IoInfo->IoData);
    return EFI_SUCCESS;
  }

  //
  // Convert Register to a register lookup table index
  //
  return SmramSaveStateReadRegisterByIndex (CpuIndex, SmramSaveStateGetRegisterIndex (Register), Width, Buffer);
}

/**
  Writes an SMM Save State register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for writing the
  SMM Save Sate register.

  @param[in] CpuIndex  The index of the CPU to write the SMM Save State.  The
                       value must be between 0 and the NumberOfCpus field in
                       the System Management System Table (SMST).
  @param[in] Register  The SMM Save State register to write.
  @param[in] Width     The number of bytes to write to the CPU save state.
  @param[in] Buffer    Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS           The register was written to Save State.
  @retval EFI_INVALID_PARAMTER  Buffer is NULL.
  @retval EFI_UNSUPPORTED       This function does not support writing Register.
  @retval EFI_NOT_FOUND         If desired Register not found.
**/
EFI_STATUS
EFIAPI
SmramSaveStateWriteRegister (
  IN UINTN                        CpuIndex,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        Width,
  IN CONST VOID                   *Buffer
  )
{
  UINTN                 RegisterIndex;
  SMRAM_SAVE_STATE_MAP  *CpuSaveState;

  //
  // Writes to EFI_SMM_SAVE_STATE_REGISTER_LMA are ignored
  //
  if (Register == EFI_SMM_SAVE_STATE_REGISTER_LMA) {
    return EFI_SUCCESS;
  }

  //
  // Writes to EFI_SMM_SAVE_STATE_REGISTER_IO are not supported
  //
  if (Register == EFI_SMM_SAVE_STATE_REGISTER_IO) {
    return EFI_NOT_FOUND;
  }

  //
  // Convert Register to a register lookup table index
  //
  RegisterIndex = SmramSaveStateGetRegisterIndex (Register);
  if (RegisterIndex == 0) {
    return EFI_NOT_FOUND;
  }

  CpuSaveState = gSmst->CpuSaveState[CpuIndex];

  //
  // Do not write non-writable SaveState, because it will cause exception.
  //
  if (!mSmmSmramCpuWidthOffset[RegisterIndex].Writeable) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check CPU mode
  //
  if (SmramSaveStateGetRegisterLma ()  == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
    //
    // If 32-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmSmramCpuWidthOffset[RegisterIndex].Width32 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 32-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmSmramCpuWidthOffset[RegisterIndex].Width32) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write SMM State register
    //
    ASSERT (CpuSaveState != NULL);
    CopyMem ((UINT8 *)CpuSaveState + mSmmSmramCpuWidthOffset[RegisterIndex].Offset32, Buffer, Width);
  } else {
    //
    // If 64-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmSmramCpuWidthOffset[RegisterIndex].Width64 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 64-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmSmramCpuWidthOffset[RegisterIndex].Width64) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write at most 4 of the lower bytes of SMM State register
    //
    CopyMem ((UINT8 *)CpuSaveState + mSmmSmramCpuWidthOffset[RegisterIndex].Offset64Lo, Buffer, MIN (4, Width));
    if (Width > 4) {
      //
      // Write at most 4 of the upper bytes of SMM State register
      //
      CopyMem ((UINT8 *)CpuSaveState + mSmmSmramCpuWidthOffset[RegisterIndex].Offset64Hi, (UINT8 *)Buffer + 4, Width - 4);
    }
  }

  return EFI_SUCCESS;
}
