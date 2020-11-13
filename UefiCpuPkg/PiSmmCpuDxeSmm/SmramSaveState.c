/** @file
Provides services to access SMRAM Save State Map

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>

#include <Library/SmmCpuFeaturesLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>

#include "PiSmmCpuDxeSmm.h"

typedef struct {
  UINT64                            Signature;              // Offset 0x00
  UINT16                            Reserved1;              // Offset 0x08
  UINT16                            Reserved2;              // Offset 0x0A
  UINT16                            Reserved3;              // Offset 0x0C
  UINT16                            SmmCs;                  // Offset 0x0E
  UINT16                            SmmDs;                  // Offset 0x10
  UINT16                            SmmSs;                  // Offset 0x12
  UINT16                            SmmOtherSegment;        // Offset 0x14
  UINT16                            Reserved4;              // Offset 0x16
  UINT64                            Reserved5;              // Offset 0x18
  UINT64                            Reserved6;              // Offset 0x20
  UINT64                            Reserved7;              // Offset 0x28
  UINT64                            SmmGdtPtr;              // Offset 0x30
  UINT32                            SmmGdtSize;             // Offset 0x38
  UINT32                            Reserved8;              // Offset 0x3C
  UINT64                            Reserved9;              // Offset 0x40
  UINT64                            Reserved10;             // Offset 0x48
  UINT16                            Reserved11;             // Offset 0x50
  UINT16                            Reserved12;             // Offset 0x52
  UINT32                            Reserved13;             // Offset 0x54
  UINT64                            Reserved14;             // Offset 0x58
} PROCESSOR_SMM_DESCRIPTOR;

extern CONST PROCESSOR_SMM_DESCRIPTOR      gcPsd;

//
// EFER register LMA bit
//
#define LMA BIT10

///
/// Macro used to simplify the lookup table entries of type CPU_SMM_SAVE_STATE_LOOKUP_ENTRY
///
#define SMM_CPU_OFFSET(Field) OFFSET_OF (SMRAM_SAVE_STATE_MAP, Field)

///
/// Macro used to simplify the lookup table entries of type CPU_SMM_SAVE_STATE_REGISTER_RANGE
///
#define SMM_REGISTER_RANGE(Start, End) { Start, End, End - Start + 1 }

///
/// Structure used to describe a range of registers
///
typedef struct {
  EFI_SMM_SAVE_STATE_REGISTER  Start;
  EFI_SMM_SAVE_STATE_REGISTER  End;
  UINTN                        Length;
} CPU_SMM_SAVE_STATE_REGISTER_RANGE;

///
/// Structure used to build a lookup table to retrieve the widths and offsets
/// associated with each supported EFI_SMM_SAVE_STATE_REGISTER value
///

#define SMM_SAVE_STATE_REGISTER_SMMREVID_INDEX        1
#define SMM_SAVE_STATE_REGISTER_IOMISC_INDEX          2
#define SMM_SAVE_STATE_REGISTER_IOMEMADDR_INDEX       3
#define SMM_SAVE_STATE_REGISTER_MAX_INDEX             4

typedef struct {
  UINT8   Width32;
  UINT8   Width64;
  UINT16  Offset32;
  UINT16  Offset64Lo;
  UINT16  Offset64Hi;
  BOOLEAN Writeable;
} CPU_SMM_SAVE_STATE_LOOKUP_ENTRY;

///
/// Structure used to build a lookup table for the IOMisc width information
///
typedef struct {
  UINT8                        Width;
  EFI_SMM_SAVE_STATE_IO_WIDTH  IoWidth;
} CPU_SMM_SAVE_STATE_IO_WIDTH;

///
/// Variables from SMI Handler
///
X86_ASSEMBLY_PATCH_LABEL gPatchSmbase;
X86_ASSEMBLY_PATCH_LABEL gPatchSmiStack;
X86_ASSEMBLY_PATCH_LABEL gPatchSmiCr3;
extern volatile UINT8    gcSmiHandlerTemplate[];
extern CONST UINT16      gcSmiHandlerSize;

//
// Variables used by SMI Handler
//
IA32_DESCRIPTOR  gSmiHandlerIdtr;

///
/// Table used by GetRegisterIndex() to convert an EFI_SMM_SAVE_STATE_REGISTER
/// value to an index into a table of type CPU_SMM_SAVE_STATE_LOOKUP_ENTRY
///
CONST CPU_SMM_SAVE_STATE_REGISTER_RANGE mSmmCpuRegisterRanges[] = {
  SMM_REGISTER_RANGE (EFI_SMM_SAVE_STATE_REGISTER_GDTBASE, EFI_SMM_SAVE_STATE_REGISTER_LDTINFO),
  SMM_REGISTER_RANGE (EFI_SMM_SAVE_STATE_REGISTER_ES,      EFI_SMM_SAVE_STATE_REGISTER_RIP),
  SMM_REGISTER_RANGE (EFI_SMM_SAVE_STATE_REGISTER_RFLAGS,  EFI_SMM_SAVE_STATE_REGISTER_CR4),
  { (EFI_SMM_SAVE_STATE_REGISTER)0, (EFI_SMM_SAVE_STATE_REGISTER)0, 0 }
};

///
/// Lookup table used to retrieve the widths and offsets associated with each
/// supported EFI_SMM_SAVE_STATE_REGISTER value
///
CONST CPU_SMM_SAVE_STATE_LOOKUP_ENTRY mSmmCpuWidthOffset[] = {
  {0, 0, 0, 0, 0, FALSE},                                                                                                     //  Reserved

  //
  // Internally defined CPU Save State Registers. Not defined in PI SMM CPU Protocol.
  //
  {4, 4, SMM_CPU_OFFSET (x86.SMMRevId)  , SMM_CPU_OFFSET (x64.SMMRevId)  , 0                                 , FALSE}, // SMM_SAVE_STATE_REGISTER_SMMREVID_INDEX  = 1
  {4, 4, SMM_CPU_OFFSET (x86.IOMisc)    , SMM_CPU_OFFSET (x64.IOMisc)    , 0                                 , FALSE}, // SMM_SAVE_STATE_REGISTER_IOMISC_INDEX    = 2
  {4, 8, SMM_CPU_OFFSET (x86.IOMemAddr) , SMM_CPU_OFFSET (x64.IOMemAddr) , SMM_CPU_OFFSET (x64.IOMemAddr) + 4, FALSE}, // SMM_SAVE_STATE_REGISTER_IOMEMADDR_INDEX = 3

  //
  // CPU Save State registers defined in PI SMM CPU Protocol.
  //
  {0, 8, 0                            , SMM_CPU_OFFSET (x64.GdtBaseLoDword) , SMM_CPU_OFFSET (x64.GdtBaseHiDword), FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_GDTBASE  = 4
  {0, 8, 0                            , SMM_CPU_OFFSET (x64.IdtBaseLoDword) , SMM_CPU_OFFSET (x64.IdtBaseHiDword), FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_IDTBASE  = 5
  {0, 8, 0                            , SMM_CPU_OFFSET (x64.LdtBaseLoDword) , SMM_CPU_OFFSET (x64.LdtBaseHiDword), FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_LDTBASE  = 6
  {0, 0, 0                            , 0                                   , 0                                  , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_GDTLIMIT = 7
  {0, 0, 0                            , 0                                   , 0                                  , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_IDTLIMIT = 8
  {0, 0, 0                            , 0                                   , 0                                  , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_LDTLIMIT = 9
  {0, 0, 0                            , 0                                   , 0                                  , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_LDTINFO  = 10

  {4, 4, SMM_CPU_OFFSET (x86._ES)     , SMM_CPU_OFFSET (x64._ES)     , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_ES       = 20
  {4, 4, SMM_CPU_OFFSET (x86._CS)     , SMM_CPU_OFFSET (x64._CS)     , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_CS       = 21
  {4, 4, SMM_CPU_OFFSET (x86._SS)     , SMM_CPU_OFFSET (x64._SS)     , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_SS       = 22
  {4, 4, SMM_CPU_OFFSET (x86._DS)     , SMM_CPU_OFFSET (x64._DS)     , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_DS       = 23
  {4, 4, SMM_CPU_OFFSET (x86._FS)     , SMM_CPU_OFFSET (x64._FS)     , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_FS       = 24
  {4, 4, SMM_CPU_OFFSET (x86._GS)     , SMM_CPU_OFFSET (x64._GS)     , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_GS       = 25
  {0, 4, 0                            , SMM_CPU_OFFSET (x64._LDTR)   , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_LDTR_SEL = 26
  {4, 4, SMM_CPU_OFFSET (x86._TR)     , SMM_CPU_OFFSET (x64._TR)     , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_TR_SEL   = 27
  {4, 8, SMM_CPU_OFFSET (x86._DR7)    , SMM_CPU_OFFSET (x64._DR7)    , SMM_CPU_OFFSET (x64._DR7)    + 4, FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_DR7      = 28
  {4, 8, SMM_CPU_OFFSET (x86._DR6)    , SMM_CPU_OFFSET (x64._DR6)    , SMM_CPU_OFFSET (x64._DR6)    + 4, FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_DR6      = 29
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R8)     , SMM_CPU_OFFSET (x64._R8)     + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R8       = 30
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R9)     , SMM_CPU_OFFSET (x64._R9)     + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R9       = 31
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R10)    , SMM_CPU_OFFSET (x64._R10)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R10      = 32
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R11)    , SMM_CPU_OFFSET (x64._R11)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R11      = 33
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R12)    , SMM_CPU_OFFSET (x64._R12)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R12      = 34
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R13)    , SMM_CPU_OFFSET (x64._R13)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R13      = 35
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R14)    , SMM_CPU_OFFSET (x64._R14)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R14      = 36
  {0, 8, 0                            , SMM_CPU_OFFSET (x64._R15)    , SMM_CPU_OFFSET (x64._R15)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_R15      = 37
  {4, 8, SMM_CPU_OFFSET (x86._EAX)    , SMM_CPU_OFFSET (x64._RAX)    , SMM_CPU_OFFSET (x64._RAX)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RAX      = 38
  {4, 8, SMM_CPU_OFFSET (x86._EBX)    , SMM_CPU_OFFSET (x64._RBX)    , SMM_CPU_OFFSET (x64._RBX)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RBX      = 39
  {4, 8, SMM_CPU_OFFSET (x86._ECX)    , SMM_CPU_OFFSET (x64._RCX)    , SMM_CPU_OFFSET (x64._RCX)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RCX      = 40
  {4, 8, SMM_CPU_OFFSET (x86._EDX)    , SMM_CPU_OFFSET (x64._RDX)    , SMM_CPU_OFFSET (x64._RDX)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RDX      = 41
  {4, 8, SMM_CPU_OFFSET (x86._ESP)    , SMM_CPU_OFFSET (x64._RSP)    , SMM_CPU_OFFSET (x64._RSP)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RSP      = 42
  {4, 8, SMM_CPU_OFFSET (x86._EBP)    , SMM_CPU_OFFSET (x64._RBP)    , SMM_CPU_OFFSET (x64._RBP)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RBP      = 43
  {4, 8, SMM_CPU_OFFSET (x86._ESI)    , SMM_CPU_OFFSET (x64._RSI)    , SMM_CPU_OFFSET (x64._RSI)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RSI      = 44
  {4, 8, SMM_CPU_OFFSET (x86._EDI)    , SMM_CPU_OFFSET (x64._RDI)    , SMM_CPU_OFFSET (x64._RDI)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RDI      = 45
  {4, 8, SMM_CPU_OFFSET (x86._EIP)    , SMM_CPU_OFFSET (x64._RIP)    , SMM_CPU_OFFSET (x64._RIP)    + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RIP      = 46

  {4, 8, SMM_CPU_OFFSET (x86._EFLAGS) , SMM_CPU_OFFSET (x64._RFLAGS) , SMM_CPU_OFFSET (x64._RFLAGS) + 4, TRUE },  //  EFI_SMM_SAVE_STATE_REGISTER_RFLAGS   = 51
  {4, 8, SMM_CPU_OFFSET (x86._CR0)    , SMM_CPU_OFFSET (x64._CR0)    , SMM_CPU_OFFSET (x64._CR0)    + 4, FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_CR0      = 52
  {4, 8, SMM_CPU_OFFSET (x86._CR3)    , SMM_CPU_OFFSET (x64._CR3)    , SMM_CPU_OFFSET (x64._CR3)    + 4, FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_CR3      = 53
  {0, 4, 0                            , SMM_CPU_OFFSET (x64._CR4)    , 0                               , FALSE},  //  EFI_SMM_SAVE_STATE_REGISTER_CR4      = 54
};

///
/// Lookup table for the IOMisc width information
///
CONST CPU_SMM_SAVE_STATE_IO_WIDTH mSmmCpuIoWidth[] = {
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
CONST EFI_SMM_SAVE_STATE_IO_TYPE mSmmCpuIoType[] = {
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

///
/// The mode of the CPU at the time an SMI occurs
///
UINT8  mSmmSaveStateRegisterLma;

/**
  Read information from the CPU save state.

  @param  Register  Specifies the CPU register to read form the save state.

  @retval 0   Register is not valid
  @retval >0  Index into mSmmCpuWidthOffset[] associated with Register

**/
UINTN
GetRegisterIndex (
  IN EFI_SMM_SAVE_STATE_REGISTER  Register
  )
{
  UINTN  Index;
  UINTN  Offset;

  for (Index = 0, Offset = SMM_SAVE_STATE_REGISTER_MAX_INDEX; mSmmCpuRegisterRanges[Index].Length != 0; Index++) {
    if (Register >= mSmmCpuRegisterRanges[Index].Start && Register <= mSmmCpuRegisterRanges[Index].End) {
      return Register - mSmmCpuRegisterRanges[Index].Start + Offset;
    }
    Offset += mSmmCpuRegisterRanges[Index].Length;
  }
  return 0;
}

/**
  Read a CPU Save State register on the target processor.

  This function abstracts the differences that whether the CPU Save State register is in the
  IA32 CPU Save State Map or X64 CPU Save State Map.

  This function supports reading a CPU Save State register in SMBase relocation handler.

  @param[in]  CpuIndex       Specifies the zero-based index of the CPU save state.
  @param[in]  RegisterIndex  Index into mSmmCpuWidthOffset[] look up table.
  @param[in]  Width          The number of bytes to read from the CPU save state.
  @param[out] Buffer         Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
  @retval EFI_INVALID_PARAMETER  This or Buffer is NULL.

**/
EFI_STATUS
ReadSaveStateRegisterByIndex (
  IN UINTN   CpuIndex,
  IN UINTN   RegisterIndex,
  IN UINTN   Width,
  OUT VOID   *Buffer
  )
{
  SMRAM_SAVE_STATE_MAP  *CpuSaveState;

  if (RegisterIndex == 0) {
    return EFI_NOT_FOUND;
  }

  CpuSaveState = gSmst->CpuSaveState[CpuIndex];

  if (mSmmSaveStateRegisterLma == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
    //
    // If 32-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmCpuWidthOffset[RegisterIndex].Width32 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 32-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmCpuWidthOffset[RegisterIndex].Width32) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write return buffer
    //
    ASSERT(CpuSaveState != NULL);
    CopyMem(Buffer, (UINT8 *)CpuSaveState + mSmmCpuWidthOffset[RegisterIndex].Offset32, Width);
  } else {
    //
    // If 64-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmCpuWidthOffset[RegisterIndex].Width64 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 64-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmCpuWidthOffset[RegisterIndex].Width64) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write at most 4 of the lower bytes of the return buffer
    //
    CopyMem(Buffer, (UINT8 *)CpuSaveState + mSmmCpuWidthOffset[RegisterIndex].Offset64Lo, MIN(4, Width));
    if (Width > 4) {
      //
      // Write at most 4 of the upper bytes of the return buffer
      //
      CopyMem((UINT8 *)Buffer + 4, (UINT8 *)CpuSaveState + mSmmCpuWidthOffset[RegisterIndex].Offset64Hi, Width - 4);
    }
  }
  return EFI_SUCCESS;
}

/**
  Read a CPU Save State register on the target processor.

  This function abstracts the differences that whether the CPU Save State register is in the
  IA32 CPU Save State Map or X64 CPU Save State Map.

  This function supports reading a CPU Save State register in SMBase relocation handler.

  @param[in]  CpuIndex       Specifies the zero-based index of the CPU save state.
  @param[in]  RegisterIndex  Index into mSmmCpuWidthOffset[] look up table.
  @param[in]  Width          The number of bytes to read from the CPU save state.
  @param[out] Buffer         Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
  @retval EFI_INVALID_PARAMETER Buffer is NULL, or Width does not meet requirement per Register type.

**/
EFI_STATUS
EFIAPI
ReadSaveStateRegister (
  IN UINTN                        CpuIndex,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        Width,
  OUT VOID                        *Buffer
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

    *(UINT8 *)Buffer = mSmmSaveStateRegisterLma;

    return EFI_SUCCESS;
  }

  //
  // Check for special EFI_SMM_SAVE_STATE_REGISTER_IO
  //
  if (Register == EFI_SMM_SAVE_STATE_REGISTER_IO) {
    //
    // Get SMM Revision ID
    //
    ReadSaveStateRegisterByIndex (CpuIndex, SMM_SAVE_STATE_REGISTER_SMMREVID_INDEX, sizeof(SmmRevId), &SmmRevId);

    //
    // See if the CPU supports the IOMisc register in the save state
    //
    if (SmmRevId < SMRAM_SAVE_STATE_MIN_REV_ID_IOMISC) {
      return EFI_NOT_FOUND;
    }

    //
    // Get the IOMisc register value
    //
    ReadSaveStateRegisterByIndex (CpuIndex, SMM_SAVE_STATE_REGISTER_IOMISC_INDEX, sizeof(IoMisc.Uint32), &IoMisc.Uint32);

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
        (mSmmCpuIoType[IoMisc.Bits.Type] != EFI_SMM_SAVE_STATE_IO_TYPE_OUTPUT)) {
      return EFI_NOT_FOUND;
    }

    //
    // Compute index for the I/O Length and I/O Type lookup tables
    //
    if (mSmmCpuIoWidth[IoMisc.Bits.Length].Width == 0 || mSmmCpuIoType[IoMisc.Bits.Type] == 0) {
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
    ZeroMem (IoInfo, sizeof(EFI_SMM_SAVE_STATE_IO_INFO));

    //
    // Use lookup tables to help fill in all the fields of the IoInfo structure
    //
    IoInfo->IoPort = (UINT16)IoMisc.Bits.Port;
    IoInfo->IoWidth = mSmmCpuIoWidth[IoMisc.Bits.Length].IoWidth;
    IoInfo->IoType = mSmmCpuIoType[IoMisc.Bits.Type];
    ReadSaveStateRegister (CpuIndex, EFI_SMM_SAVE_STATE_REGISTER_RAX, mSmmCpuIoWidth[IoMisc.Bits.Length].Width, &IoInfo->IoData);
    return EFI_SUCCESS;
  }

  //
  // Convert Register to a register lookup table index
  //
  return ReadSaveStateRegisterByIndex (CpuIndex, GetRegisterIndex (Register), Width, Buffer);
}

/**
  Write value to a CPU Save State register on the target processor.

  This function abstracts the differences that whether the CPU Save State register is in the
  IA32 CPU Save State Map or X64 CPU Save State Map.

  This function supports writing a CPU Save State register in SMBase relocation handler.

  @param[in] CpuIndex       Specifies the zero-based index of the CPU save state.
  @param[in] RegisterIndex  Index into mSmmCpuWidthOffset[] look up table.
  @param[in] Width          The number of bytes to read from the CPU save state.
  @param[in] Buffer         Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS           The register was written to Save State.
  @retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
  @retval EFI_INVALID_PARAMETER  ProcessorIndex or Width is not correct.

**/
EFI_STATUS
EFIAPI
WriteSaveStateRegister (
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
  RegisterIndex = GetRegisterIndex (Register);
  if (RegisterIndex == 0) {
    return EFI_NOT_FOUND;
  }

  CpuSaveState = gSmst->CpuSaveState[CpuIndex];

  //
  // Do not write non-writable SaveState, because it will cause exception.
  //
  if (!mSmmCpuWidthOffset[RegisterIndex].Writeable) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check CPU mode
  //
  if (mSmmSaveStateRegisterLma == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
    //
    // If 32-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmCpuWidthOffset[RegisterIndex].Width32 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 32-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmCpuWidthOffset[RegisterIndex].Width32) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Write SMM State register
    //
    ASSERT (CpuSaveState != NULL);
    CopyMem((UINT8 *)CpuSaveState + mSmmCpuWidthOffset[RegisterIndex].Offset32, Buffer, Width);
  } else {
    //
    // If 64-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmCpuWidthOffset[RegisterIndex].Width64 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 64-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmCpuWidthOffset[RegisterIndex].Width64) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write at most 4 of the lower bytes of SMM State register
    //
    CopyMem((UINT8 *)CpuSaveState + mSmmCpuWidthOffset[RegisterIndex].Offset64Lo, Buffer, MIN (4, Width));
    if (Width > 4) {
      //
      // Write at most 4 of the upper bytes of SMM State register
      //
      CopyMem((UINT8 *)CpuSaveState + mSmmCpuWidthOffset[RegisterIndex].Offset64Hi, (UINT8 *)Buffer + 4, Width - 4);
    }
  }
  return EFI_SUCCESS;
}

/**
  Hook the code executed immediately after an RSM instruction on the currently
  executing CPU.  The mode of code executed immediately after RSM must be
  detected, and the appropriate hook must be selected.  Always clear the auto
  HALT restart flag if it is set.

  @param[in] CpuIndex                 The processor index for the currently
                                      executing CPU.
  @param[in] CpuState                 Pointer to SMRAM Save State Map for the
                                      currently executing CPU.
  @param[in] NewInstructionPointer32  Instruction pointer to use if resuming to
                                      32-bit mode from 64-bit SMM.
  @param[in] NewInstructionPointer    Instruction pointer to use if resuming to
                                      same mode as SMM.

  @retval The value of the original instruction pointer before it was hooked.

**/
UINT64
EFIAPI
HookReturnFromSmm (
  IN UINTN              CpuIndex,
  SMRAM_SAVE_STATE_MAP  *CpuState,
  UINT64                NewInstructionPointer32,
  UINT64                NewInstructionPointer
  )
{
  UINT64  OriginalInstructionPointer;

  OriginalInstructionPointer = SmmCpuFeaturesHookReturnFromSmm (
                                 CpuIndex,
                                 CpuState,
                                 NewInstructionPointer32,
                                 NewInstructionPointer
                                 );
  if (OriginalInstructionPointer != 0) {
    return OriginalInstructionPointer;
  }

  if (mSmmSaveStateRegisterLma == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
    OriginalInstructionPointer = (UINT64)CpuState->x86._EIP;
    CpuState->x86._EIP = (UINT32)NewInstructionPointer;
    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((CpuState->x86.AutoHALTRestart & BIT0) != 0) {
      CpuState->x86.AutoHALTRestart &= ~BIT0;
    }
  } else {
    OriginalInstructionPointer = CpuState->x64._RIP;
    if ((CpuState->x64.IA32_EFER & LMA) == 0) {
      CpuState->x64._RIP = (UINT32)NewInstructionPointer32;
    } else {
      CpuState->x64._RIP = (UINT32)NewInstructionPointer;
    }
    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((CpuState->x64.AutoHALTRestart & BIT0) != 0) {
      CpuState->x64.AutoHALTRestart &= ~BIT0;
    }
  }
  return OriginalInstructionPointer;
}

/**
  Get the size of the SMI Handler in bytes.

  @retval The size, in bytes, of the SMI Handler.

**/
UINTN
EFIAPI
GetSmiHandlerSize (
  VOID
  )
{
  UINTN  Size;

  Size = SmmCpuFeaturesGetSmiHandlerSize ();
  if (Size != 0) {
    return Size;
  }
  return gcSmiHandlerSize;
}

/**
  Install the SMI handler for the CPU specified by CpuIndex.  This function
  is called by the CPU that was elected as monarch during System Management
  Mode initialization.

  @param[in] CpuIndex   The index of the CPU to install the custom SMI handler.
                        The value must be between 0 and the NumberOfCpus field
                        in the System Management System Table (SMST).
  @param[in] SmBase     The SMBASE address for the CPU specified by CpuIndex.
  @param[in] SmiStack   The stack to use when an SMI is processed by the
                        the CPU specified by CpuIndex.
  @param[in] StackSize  The size, in bytes, if the stack used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtBase    The base address of the GDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtSize    The size, in bytes, of the GDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtBase    The base address of the IDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtSize    The size, in bytes, of the IDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] Cr3        The base address of the page tables to use when an SMI
                        is processed by the CPU specified by CpuIndex.
**/
VOID
EFIAPI
InstallSmiHandler (
  IN UINTN   CpuIndex,
  IN UINT32  SmBase,
  IN VOID    *SmiStack,
  IN UINTN   StackSize,
  IN UINTN   GdtBase,
  IN UINTN   GdtSize,
  IN UINTN   IdtBase,
  IN UINTN   IdtSize,
  IN UINT32  Cr3
  )
{
  PROCESSOR_SMM_DESCRIPTOR  *Psd;
  UINT32                    CpuSmiStack;

  //
  // Initialize PROCESSOR_SMM_DESCRIPTOR
  //
  Psd = (PROCESSOR_SMM_DESCRIPTOR *)(VOID *)((UINTN)SmBase + SMM_PSD_OFFSET);
  CopyMem (Psd, &gcPsd, sizeof (gcPsd));
  Psd->SmmGdtPtr = (UINT64)GdtBase;
  Psd->SmmGdtSize = (UINT32)GdtSize;

  if (SmmCpuFeaturesGetSmiHandlerSize () != 0) {
    //
    // Install SMI handler provided by library
    //
    SmmCpuFeaturesInstallSmiHandler (
      CpuIndex,
      SmBase,
      SmiStack,
      StackSize,
      GdtBase,
      GdtSize,
      IdtBase,
      IdtSize,
      Cr3
      );
    return;
  }

  InitShadowStack (CpuIndex, (VOID *)((UINTN)SmiStack + StackSize));

  //
  // Initialize values in template before copy
  //
  CpuSmiStack = (UINT32)((UINTN)SmiStack + StackSize - sizeof (UINTN));
  PatchInstructionX86 (gPatchSmiStack, CpuSmiStack, 4);
  PatchInstructionX86 (gPatchSmiCr3, Cr3, 4);
  PatchInstructionX86 (gPatchSmbase, SmBase, 4);
  gSmiHandlerIdtr.Base  = IdtBase;
  gSmiHandlerIdtr.Limit = (UINT16)(IdtSize - 1);

  //
  // Set the value at the top of the CPU stack to the CPU Index
  //
  *(UINTN*)(UINTN)CpuSmiStack = CpuIndex;

  //
  // Copy template to CPU specific SMI handler location
  //
  CopyMem (
    (VOID*)((UINTN)SmBase + SMM_HANDLER_OFFSET),
    (VOID*)gcSmiHandlerTemplate,
    gcSmiHandlerSize
    );
}
