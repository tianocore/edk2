/** @file
  Processor specific parts of the GDB stub

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <GdbStubInternal.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PrintLib.h>

//
// Array of exception types that need to be hooked by the debugger
// (efi, gdb) //efi number
//
EFI_EXCEPTION_TYPE_ENTRY  gExceptionType[] = {
  { EXCEPT_ARM_SOFTWARE_INTERRUPT, GDB_SIGTRAP }
  //  { EXCEPT_ARM_UNDEFINED_INSTRUCTION, GDB_SIGTRAP },
  //  { EXCEPT_ARM_PREFETCH_ABORT,        GDB_SIGTRAP },
  //  { EXCEPT_ARM_DATA_ABORT,            GDB_SIGEMT  },
  //  { EXCEPT_ARM_RESERVED,              GDB_SIGILL  }
};

UINTN  gRegisterOffsets[] = {
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R0),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R1),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R2),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R3),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R4),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R5),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R6),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R7),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R8),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R9),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R10),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R11),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, R12),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, SP),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, LR),
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, PC),
  0x00000F01,                        // f0
  0x00000F02,
  0x00000F03,
  0x00000F11,                        // f1
  0x00000F12,
  0x00000F13,
  0x00000F21,                        // f2
  0x00000F22,
  0x00000F23,
  0x00000F31,                        // f3
  0x00000F32,
  0x00000F33,
  0x00000F41,                        // f4
  0x00000F42,
  0x00000F43,
  0x00000F51,                        // f5
  0x00000F52,
  0x00000F53,
  0x00000F61,                        // f6
  0x00000F62,
  0x00000F63,
  0x00000F71,                        // f7
  0x00000F72,
  0x00000F73,
  0x00000FFF,                        // fps
  OFFSET_OF (EFI_SYSTEM_CONTEXT_ARM, CPSR)
};

/**
 Return the number of entries in the gExceptionType[]

 @retval  UINTN, the number of entries in the gExceptionType[] array.
 **/
UINTN
MaxEfiException (
  VOID
  )
{
  return sizeof (gExceptionType) / sizeof (EFI_EXCEPTION_TYPE_ENTRY);
}

/**
 Return the number of entries in the gRegisters[]

 @retval  UINTN, the number of entries (registers) in the gRegisters[] array.
 **/
UINTN
MaxRegisterCount (
  VOID
  )
{
  return sizeof (gRegisterOffsets) / sizeof (UINTN);
}

/**
 Check to see if the ISA is supported.
 ISA = Instruction Set Architecture

 @retval TRUE if Isa is supported

**/
BOOLEAN
CheckIsa (
  IN  EFI_INSTRUCTION_SET_ARCHITECTURE  Isa
  )
{
  if (Isa == IsaArm) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
 This takes in the register number and the System Context, and returns a pointer to the RegNumber-th register in gdb ordering
 It is, by default, set to find the register pointer of the ARM member
 @param   SystemContext     Register content at time of the exception
 @param   RegNumber       The register to which we want to find a pointer
 @retval  the pointer to the RegNumber-th pointer
 **/
UINTN *
FindPointerToRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber
  )
{
  UINT8  *TempPtr;

  ASSERT (gRegisterOffsets[RegNumber] < 0xF00);
  TempPtr = ((UINT8 *)SystemContext.SystemContextArm) + gRegisterOffsets[RegNumber];
  return (UINT32 *)TempPtr;
}

/**
 Adds the RegNumber-th register's value to the output buffer, starting at the given OutBufPtr
 @param SystemContext     Register content at time of the exception
 @param   RegNumber       the number of the register that we want to read
 @param   OutBufPtr       pointer to the output buffer's end. the new data will be added from this point on.
 @retval  the pointer to the next character of the output buffer that is available to be written on.
 **/
CHAR8 *
BasicReadRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber,
  IN  CHAR8               *OutBufPtr
  )
{
  UINTN  RegSize;
  CHAR8  Char;

  if (gRegisterOffsets[RegNumber] > 0xF00) {
    AsciiSPrint (OutBufPtr, 9, "00000000");
    OutBufPtr += 8;
    return OutBufPtr;
  }

  RegSize = 0;
  while (RegSize < 32) {
    Char = mHexToStr[(UINT8)((*FindPointerToRegister (SystemContext, RegNumber) >> (RegSize+4)) & 0xf)];
    if ((Char >= 'A') && (Char <= 'F')) {
      Char = Char - 'A' + 'a';
    }

    *OutBufPtr++ = Char;

    Char = mHexToStr[(UINT8)((*FindPointerToRegister (SystemContext, RegNumber) >> RegSize) & 0xf)];
    if ((Char >= 'A') && (Char <= 'F')) {
      Char = Char - 'A' + 'a';
    }

    *OutBufPtr++ = Char;

    RegSize = RegSize + 8;
  }

  return OutBufPtr;
}

/**
 Reads the n-th register's value into an output buffer and sends it as a packet
 @param   SystemContext   Register content at time of the exception
 @param   InBuffer      Pointer to the input buffer received from gdb server
 **/
VOID
ReadNthRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *InBuffer
  )
{
  UINTN  RegNumber;
  CHAR8  OutBuffer[9]; // 1 reg=8 hex chars, and the end '\0' (escape seq)
  CHAR8  *OutBufPtr;   // pointer to the output buffer

  RegNumber = AsciiStrHexToUintn (&InBuffer[1]);

  if (RegNumber >= MaxRegisterCount ()) {
    SendError (GDB_EINVALIDREGNUM);
    return;
  }

  OutBufPtr = OutBuffer;
  OutBufPtr = BasicReadRegister (SystemContext, RegNumber, OutBufPtr);

  *OutBufPtr = '\0';  // the end of the buffer
  SendPacket (OutBuffer);
}

/**
 Reads the general registers into an output buffer  and sends it as a packet
 @param   SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
ReadGeneralRegisters (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN  Index;
  CHAR8  *OutBuffer;
  CHAR8  *OutBufPtr;
  UINTN  RegisterCount = MaxRegisterCount ();

  // It is not safe to allocate pool here....
  OutBuffer = AllocatePool ((RegisterCount * 8) + 1); // 8 bytes per register in string format plus a null to terminate
  OutBufPtr = OutBuffer;
  for (Index = 0; Index < RegisterCount; Index++) {
    OutBufPtr = BasicReadRegister (SystemContext, Index, OutBufPtr);
  }

  *OutBufPtr = '\0';
  SendPacket (OutBuffer);
  FreePool (OutBuffer);
}

/**
 Adds the RegNumber-th register's value to the output buffer, starting at the given OutBufPtr
 @param   SystemContext       Register content at time of the exception
 @param   RegNumber         the number of the register that we want to write
 @param   InBufPtr          pointer to the output buffer. the new data will be extracted from the input buffer from this point on.
 @retval  the pointer to the next character of the input buffer that can be used
 **/
CHAR8
*
BasicWriteRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber,
  IN  CHAR8               *InBufPtr
  )
{
  UINTN   RegSize;
  UINTN   TempValue; // the value transferred from a hex char
  UINT32  NewValue;  // the new value of the RegNumber-th Register

  if (gRegisterOffsets[RegNumber] > 0xF00) {
    return InBufPtr + 8;
  }

  NewValue = 0;
  RegSize  = 0;
  while (RegSize < 32) {
    TempValue = HexCharToInt (*InBufPtr++);

    if ((INTN)TempValue < 0) {
      SendError (GDB_EBADMEMDATA);
      return NULL;
    }

    NewValue += (TempValue << (RegSize+4));
    TempValue = HexCharToInt (*InBufPtr++);

    if ((INTN)TempValue < 0) {
      SendError (GDB_EBADMEMDATA);
      return NULL;
    }

    NewValue += (TempValue << RegSize);
    RegSize   = RegSize + 8;
  }

  *(FindPointerToRegister (SystemContext, RegNumber)) = NewValue;
  return InBufPtr;
}

/** ‘P n...=r...’
 Writes the new value of n-th register received into the input buffer to the n-th register
 @param   SystemContext   Register content at time of the exception
 @param   InBuffer      Pointer to the input buffer received from gdb server
 **/
VOID
WriteNthRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *InBuffer
  )
{
  UINTN  RegNumber;
  CHAR8  RegNumBuffer[MAX_REG_NUM_BUF_SIZE]; // put the 'n..' part of the message into this array
  CHAR8  *RegNumBufPtr;
  CHAR8  *InBufPtr; // pointer to the input buffer

  // find the register number to write
  InBufPtr     = &InBuffer[1];
  RegNumBufPtr = RegNumBuffer;
  while (*InBufPtr != '=') {
    *RegNumBufPtr++ = *InBufPtr++;
  }

  *RegNumBufPtr = '\0';
  RegNumber     = AsciiStrHexToUintn (RegNumBuffer);

  // check if this is a valid Register Number
  if (RegNumber >= MaxRegisterCount ()) {
    SendError (GDB_EINVALIDREGNUM);
    return;
  }

  InBufPtr++;  // skips the '=' character
  BasicWriteRegister (SystemContext, RegNumber, InBufPtr);
  SendSuccess ();
}

/** ‘G XX...’
 Writes the new values received into the input buffer to the general registers
 @param   SystemContext       Register content at time of the exception
 @param   InBuffer          Pointer to the input buffer received from gdb server
 **/
VOID
EFIAPI
WriteGeneralRegisters (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *InBuffer
  )
{
  UINTN  i;
  CHAR8  *InBufPtr; /// pointer to the input buffer
  UINTN  MinLength;
  UINTN  RegisterCount = MaxRegisterCount ();

  MinLength = (RegisterCount * 8) + 1;  // 'G' plus the registers in ASCII format

  if (AsciiStrLen (InBuffer) < MinLength) {
    // Bad message. Message is not the right length
    SendError (GDB_EBADBUFSIZE);
    return;
  }

  InBufPtr = &InBuffer[1];

  // Read the new values for the registers from the input buffer to an array, NewValueArray.
  // The values in the array are in the gdb ordering
  for (i = 0; i < RegisterCount; i++) {
    InBufPtr = BasicWriteRegister (SystemContext, i, InBufPtr);
  }

  SendSuccess ();
}

// What about Thumb?
// Use SWI 0xdbdbdb as the debug instruction
#define GDB_ARM_BKPT  0xefdbdbdb

BOOLEAN  mSingleStepActive = FALSE;
UINT32   mSingleStepPC;
UINT32   mSingleStepData;
UINTN    mSingleStepDataSize;

typedef struct {
  LIST_ENTRY    Link;
  UINT64        Signature;
  UINT32        Address;
  UINT32        Instruction;
} ARM_SOFTWARE_BREAKPOINT;

#define ARM_SOFTWARE_BREAKPOINT_SIGNATURE  SIGNATURE_64('A', 'R', 'M', 'B', 'R', 'K', 'P', 'T')
#define ARM_SOFTWARE_BREAKPOINT_FROM_LINK(a)  CR(a, ARM_SOFTWARE_BREAKPOINT, Link, ARM_SOFTWARE_BREAKPOINT_SIGNATURE)

LIST_ENTRY  BreakpointList;

/**
 Insert Single Step in the SystemContext

 @param SystemContext  Register content at time of the exception
 **/
VOID
AddSingleStep (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  if (mSingleStepActive) {
    // Currently don't support nesting
    return;
  }

  mSingleStepActive = TRUE;

  mSingleStepPC = SystemContext.SystemContextArm->PC;

  mSingleStepDataSize      = sizeof (UINT32);
  mSingleStepData          = (*(UINT32 *)mSingleStepPC);
  *(UINT32 *)mSingleStepPC = GDB_ARM_BKPT;
  if (*(UINT32 *)mSingleStepPC != GDB_ARM_BKPT) {
    // For some reason our breakpoint did not take
    mSingleStepActive = FALSE;
  }

  InvalidateInstructionCacheRange ((VOID *)mSingleStepPC, mSingleStepDataSize);
  // DEBUG((DEBUG_ERROR, "AddSingleStep at 0x%08x (was: 0x%08x is:0x%08x)\n", SystemContext.SystemContextArm->PC, mSingleStepData, *(UINT32 *)mSingleStepPC));
}

/**
 Remove Single Step in the SystemContext

 @param SystemContext  Register content at time of the exception
 **/
VOID
RemoveSingleStep (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  if (!mSingleStepActive) {
    return;
  }

  if (mSingleStepDataSize == sizeof (UINT16)) {
    *(UINT16 *)mSingleStepPC = (UINT16)mSingleStepData;
  } else {
    // DEBUG((DEBUG_ERROR, "RemoveSingleStep at 0x%08x (was: 0x%08x is:0x%08x)\n", SystemContext.SystemContextArm->PC, *(UINT32 *)mSingleStepPC, mSingleStepData));
    *(UINT32 *)mSingleStepPC = mSingleStepData;
  }

  InvalidateInstructionCacheRange ((VOID *)mSingleStepPC, mSingleStepDataSize);
  mSingleStepActive = FALSE;
}

/**
 Continue. addr is Address to resume. If addr is omitted, resume at current
 Address.

 @param   SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
ContinueAtAddress (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN    CHAR8             *PacketData
  )
{
  if (PacketData[1] != '\0') {
    SystemContext.SystemContextArm->PC = AsciiStrHexToUintn (&PacketData[1]);
  }
}

/** ‘s [addr ]’
 Single step. addr is the Address at which to resume. If addr is omitted, resume
 at same Address.

 @param   SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
SingleStep (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN    CHAR8             *PacketData
  )
{
  SendNotSupported ();
}

UINTN
GetBreakpointDataAddress (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               BreakpointNumber
  )
{
  return 0;
}

UINTN
GetBreakpointDetected (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  return 0;
}

BREAK_TYPE
GetBreakpointType (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               BreakpointNumber
  )
{
  return NotSupported;
}

ARM_SOFTWARE_BREAKPOINT *
SearchBreakpointList (
  IN  UINT32  Address
  )
{
  LIST_ENTRY               *Current;
  ARM_SOFTWARE_BREAKPOINT  *Breakpoint;

  Current = GetFirstNode (&BreakpointList);
  while (!IsNull (&BreakpointList, Current)) {
    Breakpoint = ARM_SOFTWARE_BREAKPOINT_FROM_LINK (Current);

    if (Address == Breakpoint->Address) {
      return Breakpoint;
    }

    Current = GetNextNode (&BreakpointList, Current);
  }

  return NULL;
}

VOID
SetBreakpoint (
  IN UINT32  Address
  )
{
  ARM_SOFTWARE_BREAKPOINT  *Breakpoint;

  Breakpoint = SearchBreakpointList (Address);

  if (Breakpoint != NULL) {
    return;
  }

  // create and fill breakpoint structure
  Breakpoint = AllocatePool (sizeof (ARM_SOFTWARE_BREAKPOINT));

  Breakpoint->Signature   = ARM_SOFTWARE_BREAKPOINT_SIGNATURE;
  Breakpoint->Address     = Address;
  Breakpoint->Instruction = *(UINT32 *)Address;

  // Add it to the list
  InsertTailList (&BreakpointList, &Breakpoint->Link);

  // Insert the software breakpoint
  *(UINT32 *)Address = GDB_ARM_BKPT;
  InvalidateInstructionCacheRange ((VOID *)Address, 4);

  // DEBUG((DEBUG_ERROR, "SetBreakpoint at 0x%08x (was: 0x%08x is:0x%08x)\n", Address, Breakpoint->Instruction, *(UINT32 *)Address));
}

VOID
ClearBreakpoint (
  IN UINT32  Address
  )
{
  ARM_SOFTWARE_BREAKPOINT  *Breakpoint;

  Breakpoint = SearchBreakpointList (Address);

  if (Breakpoint == NULL) {
    return;
  }

  // Add it to the list
  RemoveEntryList (&Breakpoint->Link);

  // Restore the original instruction
  *(UINT32 *)Address = Breakpoint->Instruction;
  InvalidateInstructionCacheRange ((VOID *)Address, 4);

  // DEBUG((DEBUG_ERROR, "ClearBreakpoint at 0x%08x (was: 0x%08x is:0x%08x)\n", Address, GDB_ARM_BKPT, *(UINT32 *)Address));

  FreePool (Breakpoint);
}

VOID
EFIAPI
InsertBreakPoint (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *PacketData
  )
{
  UINTN  Type;
  UINTN  Address;
  UINTN  Length;
  UINTN  ErrorCode;

  ErrorCode = ParseBreakpointPacket (PacketData, &Type, &Address, &Length);
  if (ErrorCode > 0) {
    SendError ((UINT8)ErrorCode);
    return;
  }

  switch (Type) {
    case 0:   // Software breakpoint
      break;

    default:
      DEBUG ((DEBUG_ERROR, "Insert breakpoint default: %x\n", Type));
      SendError (GDB_EINVALIDBRKPOINTTYPE);
      return;
  }

  SetBreakpoint (Address);

  SendSuccess ();
}

VOID
EFIAPI
RemoveBreakPoint (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *PacketData
  )
{
  UINTN  Type;
  UINTN  Address;
  UINTN  Length;
  UINTN  ErrorCode;

  // Parse breakpoint packet data
  ErrorCode = ParseBreakpointPacket (PacketData, &Type, &Address, &Length);
  if (ErrorCode > 0) {
    SendError ((UINT8)ErrorCode);
    return;
  }

  switch (Type) {
    case 0:   // Software breakpoint
      break;

    default:
      SendError (GDB_EINVALIDBRKPOINTTYPE);
      return;
  }

  ClearBreakpoint (Address);

  SendSuccess ();
}

VOID
InitializeProcessor (
  VOID
  )
{
  // Initialize breakpoint list
  InitializeListHead (&BreakpointList);
}

BOOLEAN
ValidateAddress (
  IN  VOID  *Address
  )
{
  if ((UINT32)Address < 0x80000000) {
    return FALSE;
  } else {
    return TRUE;
  }
}

BOOLEAN
ValidateException (
  IN  EFI_EXCEPTION_TYPE     ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINT32  ExceptionAddress;
  UINT32  Instruction;

  // Is it a debugger SWI?
  ExceptionAddress = SystemContext.SystemContextArm->PC -= 4;
  Instruction      = *(UINT32 *)ExceptionAddress;
  if (Instruction != GDB_ARM_BKPT) {
    return FALSE;
  }

  // Special for SWI-based exception handling.  SWI sets up the context
  // to return to the instruction following the SWI instruction - NOT what we want
  // for a debugger!
  SystemContext.SystemContextArm->PC = ExceptionAddress;

  return TRUE;
}
