/** @file
  Processor specific parts of the GDB stub

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <GdbStubInternal.h>

//
// Array of exception types that need to be hooked by the debugger
//
EFI_EXCEPTION_TYPE_ENTRY gExceptionType[] = {
  { EXCEPT_X64_DIVIDE_ERROR,    GDB_SIGFPE  },
  { EXCEPT_X64_DEBUG,           GDB_SIGTRAP },
  { EXCEPT_X64_NMI,             GDB_SIGEMT  },
  { EXCEPT_X64_BREAKPOINT,      GDB_SIGTRAP },
  { EXCEPT_X64_OVERFLOW,        GDB_SIGSEGV },
  { EXCEPT_X64_BOUND,           GDB_SIGSEGV },
  { EXCEPT_X64_INVALID_OPCODE,  GDB_SIGILL  },
  { EXCEPT_X64_DOUBLE_FAULT,    GDB_SIGEMT  },
  { EXCEPT_X64_STACK_FAULT,     GDB_SIGSEGV },
  { EXCEPT_X64_GP_FAULT,        GDB_SIGSEGV },
  { EXCEPT_X64_PAGE_FAULT,      GDB_SIGSEGV },
  { EXCEPT_X64_FP_ERROR,        GDB_SIGEMT  },
  { EXCEPT_X64_ALIGNMENT_CHECK, GDB_SIGEMT  },
  { EXCEPT_X64_MACHINE_CHECK,   GDB_SIGEMT  }
};


// The offsets of registers SystemContextX64.
// The fields in the array are in the gdb ordering.
// HAVE TO DOUBLE-CHECK THE ORDER of the 24 regs
//
UINTN gRegisterOffsets[] = {
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rax),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rcx),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rdx),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rbx),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rsp),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rbp),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rsi),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rdi),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rip),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Rflags),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Cs),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Ss),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Ds),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Es),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Fs),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, Gs),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R8),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R9),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R10),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R11),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R12),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R13),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R14),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_X64, R15)
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
  return sizeof (gExceptionType)/sizeof (EFI_EXCEPTION_TYPE_ENTRY);
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
  return sizeof (gRegisterOffsets)/sizeof (UINTN);
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
  return (BOOLEAN)(Isa == IsaX64);
}


/**
 This takes in the register number and the System Context, and returns a pointer to the RegNumber-th register in gdb ordering
 It is, by default, set to find the register pointer of the X64 member
 @param   SystemContext     Register content at time of the exception
 @param   RegNumber       The register to which we want to find a pointer
 @retval  the pointer to the RegNumber-th pointer
 **/
UINTN *
FindPointerToRegister(
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber
  )
{
  UINT8 *TempPtr;
  TempPtr = ((UINT8 *)SystemContext.SystemContextX64) + gRegisterOffsets[RegNumber];
  return (UINTN *)TempPtr;
}


/**
 Adds the RegNumber-th register's value to the output buffer, starting at the given OutBufPtr
 @param   SystemContext     Register content at time of the exception
 @param   RegNumber       the number of the register that we want to read
 @param   OutBufPtr       pointer to the output buffer's end. the new data will be added from this point on.
 @retval  the pointer to the next character of the output buffer that is available to be written on.
 **/
CHAR8 *
BasicReadRegister (
  IN  EFI_SYSTEM_CONTEXT      SystemContext,
  IN  UINTN           RegNumber,
  IN  CHAR8           *OutBufPtr
  )
{
  UINTN RegSize;

  RegSize = 0;
  while (RegSize < 64) {
    *OutBufPtr++ = mHexToStr[((*FindPointerToRegister(SystemContext, RegNumber) >> (RegSize+4)) & 0xf)];
    *OutBufPtr++ = mHexToStr[((*FindPointerToRegister(SystemContext, RegNumber) >> RegSize) & 0xf)];
    RegSize = RegSize + 8;
  }
  return OutBufPtr;
}


/** ‘p n’
 Reads the n-th register's value into an output buffer and sends it as a packet
 @param   SystemContext   Register content at time of the exception
 @param   InBuffer      Pointer to the input buffer received from gdb server
 **/
VOID
ReadNthRegister (
  IN  EFI_SYSTEM_CONTEXT   SystemContext,
  IN  CHAR8                *InBuffer
  )
{
  UINTN RegNumber;
  CHAR8 OutBuffer[17];  // 1 reg=16 hex chars, and the end '\0' (escape seq)
  CHAR8 *OutBufPtr;   // pointer to the output buffer

  RegNumber = AsciiStrHexToUintn (&InBuffer[1]);

  if ((RegNumber < 0) || (RegNumber >= MaxRegisterCount())) {
    SendError (GDB_EINVALIDREGNUM);
    return;
  }

  OutBufPtr = OutBuffer;
  OutBufPtr = BasicReadRegister(SystemContext, RegNumber, OutBufPtr);

  *OutBufPtr = '\0';  // the end of the buffer
  SendPacket (OutBuffer);
}


/** ‘g’
 Reads the general registers into an output buffer  and sends it as a packet

 @param   SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
ReadGeneralRegisters (
  IN  EFI_SYSTEM_CONTEXT      SystemContext
  )
{
  UINTN   i;
  CHAR8 OutBuffer[385]; // 24 regs, 16 hex chars each, and the end '\0' (escape seq)
  CHAR8 *OutBufPtr;   // pointer to the output buffer

  OutBufPtr = OutBuffer;
  for(i = 0 ; i < MaxRegisterCount() ; i++) {  // there are only 24 registers to read
    OutBufPtr = BasicReadRegister(SystemContext, i, OutBufPtr);
  }

  *OutBufPtr = '\0';  // the end of the buffer
  SendPacket (OutBuffer);
}


/**
 Adds the RegNumber-th register's value to the output buffer, starting at the given OutBufPtr

 @param   SystemContext       Register content at time of the exception
 @param   RegNumber         the number of the register that we want to write
 @param   InBufPtr          pointer to the output buffer. the new data will be extracted from the input buffer from this point on.
 @retval  the pointer to the next character of the input buffer that can be used
 **/
CHAR8 *
BasicWriteRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber,
  IN  CHAR8               *InBufPtr
  )
{
  UINTN RegSize;
  UINTN TempValue; // the value transferred from a hex char
  UINT64 NewValue; // the new value of the RegNumber-th Register

  NewValue = 0;
  RegSize = 0;
  while (RegSize < 64) {
    TempValue = HexCharToInt(*InBufPtr++);

    if (TempValue < 0) {
      SendError (GDB_EBADMEMDATA);
      return NULL;
    }

    NewValue += (TempValue << (RegSize+4));
    TempValue = HexCharToInt(*InBufPtr++);

    if (TempValue < 0) {
      SendError (GDB_EBADMEMDATA);
      return NULL;
  }

    NewValue += (TempValue << RegSize);
    RegSize = RegSize + 8;
  }
  *(FindPointerToRegister(SystemContext, RegNumber)) = NewValue;
  return InBufPtr;
}


/** ‘P n...=r...’
 Writes the new value of n-th register received into the input buffer to the n-th register

 @param   SystemContext   Register content at time of the exception
 @param   InBuffer      Ponter to the input buffer received from gdb server
 **/
VOID
EFIAPI
WriteNthRegister (
  IN  EFI_SYSTEM_CONTEXT      SystemContext,
  IN  CHAR8           *InBuffer
  )
{
  UINTN RegNumber;
  CHAR8 RegNumBuffer[MAX_REG_NUM_BUF_SIZE];  // put the 'n..' part of the message into this array
  CHAR8 *RegNumBufPtr;
  CHAR8 *InBufPtr; // pointer to the input buffer

  // find the register number to write
  InBufPtr = &InBuffer[1];
  RegNumBufPtr = RegNumBuffer;
  while (*InBufPtr != '=') {
    *RegNumBufPtr++ = *InBufPtr++;
  }
  *RegNumBufPtr = '\0';
  RegNumber = AsciiStrHexToUintn (RegNumBuffer);

  // check if this is a valid Register Number
  if ((RegNumber < 0) || (RegNumber >= MaxRegisterCount())) {
    SendError (GDB_EINVALIDREGNUM);
    return;
  }
  InBufPtr++;  // skips the '=' character
  BasicWriteRegister (SystemContext, RegNumber, InBufPtr);
  SendSuccess();
}


/** ‘G XX...’
 Writes the new values received into the input buffer to the general registers

 @param   SystemContext       Register content at time of the exception
 @param   InBuffer          Pointer to the input buffer received from gdb server
 **/
VOID
EFIAPI
WriteGeneralRegisters (
  IN  EFI_SYSTEM_CONTEXT    SystemContext,
  IN  CHAR8                 *InBuffer
  )
{
  UINTN  i;
  CHAR8 *InBufPtr; /// pointer to the input buffer

  // check to see if the buffer is the right size which is
  // 1 (for 'G') + 16 (for 16 registers) * 8 ( for 8 hex chars each) = 385
  if (AsciiStrLen(InBuffer) != 385) { // 24 regs, 16 hex chars each, and the end '\0' (escape seq)
    //Bad message. Message is not the right length
    SendError (GDB_EBADBUFSIZE);
    return;
  }

  InBufPtr = &InBuffer[1];

  // Read the new values for the registers from the input buffer to an array, NewValueArray.
  // The values in the array are in the gdb ordering
  for(i=0; i < MaxRegisterCount(); i++) {  // there are only 16 registers to write
    InBufPtr = BasicWriteRegister(SystemContext, i, InBufPtr);
  }

  SendSuccess();
}


 /**
 Insert Single Step in the SystemContext

 @param SystemContext Register content at time of the exception
 **/
VOID
AddSingleStep (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  SystemContext.SystemContextX64->Rflags |= TF_BIT; //Setting the TF bit.
}



/**
 Remove Single Step in the SystemContext

 @param SystemContext Register content at time of the exception
 **/
VOID
RemoveSingleStep (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  SystemContext.SystemContextX64->Rflags &= ~TF_BIT;  // clearing the TF bit.
}



/** ‘c [addr ]’
 Continue. addr is Address to resume. If addr is omitted, resume at current
 Address.

 @param   SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
ContinueAtAddress (
  IN  EFI_SYSTEM_CONTEXT      SystemContext,
  IN    CHAR8                 *PacketData
  )
{
  if (PacketData[1] != '\0') {
    SystemContext.SystemContextX64->Rip = AsciiStrHexToUintn(&PacketData[1]);
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
  IN  EFI_SYSTEM_CONTEXT      SystemContext,
  IN    CHAR8                 *PacketData
  )
{
  if (PacketData[1] != '\0') {
    SystemContext.SystemContextX64->Rip = AsciiStrHexToUintn (&PacketData[1]);
  }

  AddSingleStep (SystemContext);
}


/**
  Returns breakpoint data address from DR0-DR3 based on the input breakpoint
  number

  @param  SystemContext      Register content at time of the exception
  @param  BreakpointNumber   Breakpoint number

  @retval Address            Data address from DR0-DR3 based on the
                             breakpoint number.

**/
UINTN
GetBreakpointDataAddress (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               BreakpointNumber
  )
{
  UINTN Address;

  if (BreakpointNumber == 1) {
    Address = SystemContext.SystemContextIa32->Dr0;
  } else if (BreakpointNumber == 2) {
    Address = SystemContext.SystemContextIa32->Dr1;
  } else if (BreakpointNumber == 3) {
    Address = SystemContext.SystemContextIa32->Dr2;
  } else if (BreakpointNumber == 4) {
    Address = SystemContext.SystemContextIa32->Dr3;
  } else {
    Address = 0;
  }

  return Address;
}

/**
  Returns currently detected breakpoint value based on the register
  DR6 B0-B3 field.
  If no breakpoint is detected then it returns 0.

  @param  SystemContext  Register content at time of the exception

  @retval {1-4}          Currently detected breakpoint value
  @retval 0              No breakpoint detected.

**/
UINTN
GetBreakpointDetected (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  IA32_DR6 Dr6;
  UINTN BreakpointNumber;

  Dr6.UintN = SystemContext.SystemContextIa32->Dr6;

  if (Dr6.Bits.B0 == 1) {
    BreakpointNumber = 1;
  } else if (Dr6.Bits.B1 == 1) {
    BreakpointNumber = 2;
  } else if (Dr6.Bits.B2 == 1) {
    BreakpointNumber = 3;
  } else if (Dr6.Bits.B3 == 1) {
    BreakpointNumber = 4;
  } else {
    BreakpointNumber = 0;  //No breakpoint detected
  }

  return BreakpointNumber;
}

/**
  Returns Breakpoint type (InstructionExecution, DataWrite, DataRead
  or DataReadWrite) based on the Breakpoint number

  @param  SystemContext      Register content at time of the exception
  @param  BreakpointNumber   Breakpoint number

  @retval BREAK_TYPE         Breakpoint type value read from register DR7 RWn
                             field. For unknown value, it returns NotSupported.

**/
BREAK_TYPE
GetBreakpointType (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               BreakpointNumber
  )
{
  IA32_DR7 Dr7;
  BREAK_TYPE Type = NotSupported;  //Default is NotSupported type

  Dr7.UintN = SystemContext.SystemContextIa32->Dr7;

  if (BreakpointNumber == 1) {
    Type = (BREAK_TYPE) Dr7.Bits.RW0;
  } else if (BreakpointNumber == 2) {
    Type = (BREAK_TYPE) Dr7.Bits.RW1;
  } else if (BreakpointNumber == 3) {
    Type = (BREAK_TYPE) Dr7.Bits.RW2;
  } else if (BreakpointNumber == 4) {
    Type = (BREAK_TYPE) Dr7.Bits.RW3;
  }

  return Type;
}


/**
  Parses Length and returns the length which DR7 LENn field accepts.
  For example: If we receive 1-Byte length then we should return 0.
               Zero gets written to DR7 LENn field.

  @param  Length  Breakpoint length in Bytes (1 byte, 2 byte, 4 byte)

  @retval Length  Appropriate converted values which DR7 LENn field accepts.

**/
UINTN
ConvertLengthData (
  IN     UINTN   Length
  )
{
  if (Length == 1) {         //1-Byte length
    return 0;
  } else if (Length == 2) {  //2-Byte length
    return 1;
  } else if (Length == 4) {  //4-Byte length
    return 3;
  } else {                   //Undefined or 8-byte length
    return 2;
  }
}


/**
  Finds the next free debug register. If all the registers are occupied then
  EFI_OUT_OF_RESOURCES is returned.

  @param  SystemContext  Register content at time of the exception
  @param  Register       Register value (0 - 3 for the first free debug register)

  @retval EFI_STATUS     Appropriate status value.

**/
EFI_STATUS
FindNextFreeDebugRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  OUT UINTN               *Register
  )
{
  IA32_DR7 Dr7;

  Dr7.UintN = SystemContext.SystemContextIa32->Dr7;

  if (Dr7.Bits.G0 == 0) {
    *Register = 0;
  } else if (Dr7.Bits.G1 == 0) {
    *Register = 1;
  } else if (Dr7.Bits.G2 == 0) {
    *Register = 2;
  } else if (Dr7.Bits.G3 == 0) {
    *Register = 3;
  } else {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}


/**
  Enables the debug register. Writes Address value to appropriate DR0-3 register.
  Sets LENn, Gn, RWn bits in DR7 register.

  @param  SystemContext   Register content at time of the exception
  @param  Register        Register value (0 - 3)
  @param  Address         Breakpoint address value
  @param  Type            Breakpoint type (Instruction, Data write,
                          Data read or write etc.)

  @retval EFI_STATUS      Appropriate status value.

**/
EFI_STATUS
EnableDebugRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               Register,
  IN  UINTN               Address,
  IN  UINTN               Length,
  IN  UINTN               Type
  )
{
  IA32_DR7  Dr7;

  //Convert length data
  Length = ConvertLengthData (Length);

  //For Instruction execution, length should be 0
  //(Ref. Intel reference manual 18.2.4)
  if ((Type == 0) && (Length != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //Hardware doesn't support ReadWatch (z3 packet) type. GDB can handle
  //software breakpoint. We should send empty packet in both these cases.
  if ((Type == (BREAK_TYPE)DataRead) ||
      (Type == (BREAK_TYPE)SoftwareBreakpoint))  {
    return EFI_UNSUPPORTED;
  }

  //Read DR7 so appropriate Gn, RWn and LENn bits can be modified.
  Dr7.UintN = SystemContext.SystemContextIa32->Dr7;

  if (Register == 0) {
    SystemContext.SystemContextIa32->Dr0 = Address;
    Dr7.Bits.G0 = 1;
    Dr7.Bits.RW0 = Type;
    Dr7.Bits.LEN0 = Length;
  } else if (Register == 1) {
    SystemContext.SystemContextIa32->Dr1 = Address;
    Dr7.Bits.G1 = 1;
    Dr7.Bits.RW1 = Type;
    Dr7.Bits.LEN1 = Length;
  } else if (Register == 2) {
    SystemContext.SystemContextIa32->Dr2 = Address;
    Dr7.Bits.G2 = 1;
    Dr7.Bits.RW2 = Type;
    Dr7.Bits.LEN2 = Length;
  } else if (Register == 3) {
    SystemContext.SystemContextIa32->Dr3 = Address;
    Dr7.Bits.G3 = 1;
    Dr7.Bits.RW3 = Type;
    Dr7.Bits.LEN3 = Length;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  //Update Dr7 with appropriate Gn, RWn and LENn bits
  SystemContext.SystemContextIa32->Dr7 = Dr7.UintN;

  return EFI_SUCCESS;
}


/**
  Returns register number 0 - 3 for the maching debug register.
  This function compares incoming Address, Type, Length and
  if there is a match then it returns the appropriate register number.
  In case of mismatch, function returns EFI_NOT_FOUND message.

  @param  SystemContext  Register content at time of the exception
  @param  Address        Breakpoint address value
  @param  Length         Breakpoint length value
  @param  Type           Breakpoint type (Instruction, Data write, Data read
                         or write etc.)
  @param  Register       Register value to be returned

  @retval EFI_STATUS     Appropriate status value.

**/
EFI_STATUS
FindMatchingDebugRegister (
 IN  EFI_SYSTEM_CONTEXT  SystemContext,
 IN  UINTN               Address,
 IN  UINTN               Length,
 IN  UINTN               Type,
 OUT UINTN               *Register
 )
{
  IA32_DR7 Dr7;

  //Hardware doesn't support ReadWatch (z3 packet) type. GDB can handle
  //software breakpoint. We should send empty packet in both these cases.
  if ((Type == (BREAK_TYPE)DataRead) ||
      (Type == (BREAK_TYPE)SoftwareBreakpoint)) {
    return EFI_UNSUPPORTED;
  }

  //Convert length data
  Length = ConvertLengthData(Length);

  Dr7.UintN = SystemContext.SystemContextIa32->Dr7;

  if ((Dr7.Bits.G0 == 1) &&
      (Dr7.Bits.LEN0 == Length) &&
      (Dr7.Bits.RW0 == Type) &&
      (Address == SystemContext.SystemContextIa32->Dr0)) {
    *Register = 0;
  } else if ((Dr7.Bits.G1 == 1) &&
             (Dr7.Bits.LEN1 == Length) &&
             (Dr7.Bits.RW1 == Type) &&
             (Address == SystemContext.SystemContextIa32->Dr1)) {
    *Register = 1;
  } else if ((Dr7.Bits.G2 == 1) &&
             (Dr7.Bits.LEN2 == Length) &&
             (Dr7.Bits.RW2 == Type) &&
             (Address == SystemContext.SystemContextIa32->Dr2)) {
    *Register = 2;
  } else if ((Dr7.Bits.G3 == 1) &&
             (Dr7.Bits.LEN3 == Length) &&
             (Dr7.Bits.RW3 == Type) &&
             (Address == SystemContext.SystemContextIa32->Dr3)) {
    *Register = 3;
  } else {
    Print ((CHAR16 *)L"No match found..\n");
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}


/**
  Disables the particular debug register.

  @param  SystemContext   Register content at time of the exception
  @param  Register        Register to be disabled

  @retval EFI_STATUS      Appropriate status value.

**/
EFI_STATUS
DisableDebugRegister (
 IN  EFI_SYSTEM_CONTEXT  SystemContext,
 IN  UINTN               Register
 )
{
  IA32_DR7  Dr7;
  UINTN Address = 0;

  //Read DR7 register so appropriate Gn, RWn and LENn bits can be turned off.
  Dr7.UintN = SystemContext.SystemContextIa32->Dr7;

  if (Register == 0) {
    SystemContext.SystemContextIa32->Dr0 = Address;
    Dr7.Bits.G0 = 0;
    Dr7.Bits.RW0 = 0;
    Dr7.Bits.LEN0 = 0;
  } else if (Register == 1) {
    SystemContext.SystemContextIa32->Dr1 = Address;
    Dr7.Bits.G1 = 0;
    Dr7.Bits.RW1 = 0;
    Dr7.Bits.LEN1 = 0;
  } else if (Register == 2) {
    SystemContext.SystemContextIa32->Dr2 = Address;
    Dr7.Bits.G2 = 0;
    Dr7.Bits.RW2 = 0;
    Dr7.Bits.LEN2 = 0;
  } else if (Register == 3) {
    SystemContext.SystemContextIa32->Dr3 = Address;
    Dr7.Bits.G3 = 0;
    Dr7.Bits.RW3 = 0;
    Dr7.Bits.LEN3 = 0;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  //Update DR7 register so appropriate Gn, RWn and LENn bits can be turned off.
  SystemContext.SystemContextIa32->Dr7 = Dr7.UintN;

  return EFI_SUCCESS;
}

/**
  ‘Z1, [addr], [length]’
  ‘Z2, [addr], [length]’
  ‘Z3, [addr], [length]’
  ‘Z4, [addr], [length]’

  Insert hardware breakpoint/watchpoint at address addr of size length

  @param SystemContext  Register content at time of the exception
  @param *PacketData    Pointer to the Payload data for the packet

**/
VOID
EFIAPI
InsertBreakPoint (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8              *PacketData
  )
{
  UINTN Type;
  UINTN Address;
  UINTN Length;
  UINTN Register;
  EFI_STATUS Status;
  BREAK_TYPE BreakType = NotSupported;
  UINTN ErrorCode;

  ErrorCode = ParseBreakpointPacket (PacketData, &Type, &Address, &Length);
  if (ErrorCode > 0) {
    SendError ((UINT8)ErrorCode);
    return;
  }

  switch (Type) {

    case    0:   //Software breakpoint
      BreakType = SoftwareBreakpoint;
      break;

    case    1:   //Hardware breakpoint
      BreakType = InstructionExecution;
      break;

    case    2:   //Write watchpoint
      BreakType = DataWrite;
      break;

    case    3:   //Read watchpoint
      BreakType = DataRead;
      break;

    case    4:   //Access watchpoint
      BreakType = DataReadWrite;
      break;

    default  :
      Print ((CHAR16 *)L"Insert breakpoint default: %x\n", Type);
      SendError (GDB_EINVALIDBRKPOINTTYPE);
      return;
  }

  // Find next free debug register
  Status = FindNextFreeDebugRegister (SystemContext, &Register);
  if (EFI_ERROR(Status)) {
    Print ((CHAR16 *)L"No space left on device\n");
    SendError (GDB_ENOSPACE);
    return;
  }

  // Write Address, length data at particular DR register
  Status = EnableDebugRegister (SystemContext, Register, Address, Length, (UINTN)BreakType);
  if (EFI_ERROR(Status)) {

    if (Status == EFI_UNSUPPORTED) {
      Print ((CHAR16 *)L"Not supported\n");
      SendNotSupported();
      return;
    }

    Print ((CHAR16 *)L"Invalid argument\n");
    SendError (GDB_EINVALIDARG);
    return;
  }

  SendSuccess ();
}


/**
  ‘z1, [addr], [length]’
  ‘z2, [addr], [length]’
  ‘z3, [addr], [length]’
  ‘z4, [addr], [length]’

  Remove hardware breakpoint/watchpoint at address addr of size length

  @param *PacketData    Pointer to the Payload data for the packet

**/
VOID
EFIAPI
RemoveBreakPoint (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *PacketData
  )
{
  UINTN      Type;
  UINTN      Address;
  UINTN      Length;
  UINTN      Register;
  BREAK_TYPE BreakType = NotSupported;
  EFI_STATUS Status;
  UINTN      ErrorCode;

  //Parse breakpoint packet data
  ErrorCode = ParseBreakpointPacket (PacketData, &Type, &Address, &Length);
  if (ErrorCode > 0) {
    SendError ((UINT8)ErrorCode);
    return;
  }

  switch (Type) {

    case    0:   //Software breakpoint
      BreakType = SoftwareBreakpoint;
      break;

    case    1:   //Hardware breakpoint
      BreakType = InstructionExecution;
      break;

    case    2:   //Write watchpoint
      BreakType = DataWrite;
      break;

    case    3:   //Read watchpoint
      BreakType = DataRead;
      break;

    case    4:   //Access watchpoint
      BreakType = DataReadWrite;
      break;

    default  :
      SendError (GDB_EINVALIDBRKPOINTTYPE);
      return;
  }

  //Find matching debug register
  Status = FindMatchingDebugRegister (SystemContext, Address, Length, (UINTN)BreakType, &Register);
  if (EFI_ERROR(Status)) {

    if (Status == EFI_UNSUPPORTED) {
      Print ((CHAR16 *)L"Not supported.\n");
      SendNotSupported();
      return;
    }

    Print ((CHAR16 *)L"No matching register found.\n");
    SendError (GDB_ENOSPACE);
    return;
  }

  //Remove breakpoint
  Status = DisableDebugRegister(SystemContext, Register);
  if (EFI_ERROR(Status)) {
    Print ((CHAR16 *)L"Invalid argument.\n");
    SendError (GDB_EINVALIDARG);
    return;
  }

  SendSuccess ();
}


VOID
InitializeProcessor (
  VOID
  )
{
}

BOOLEAN
ValidateAddress (
  IN  VOID  *Address
  )
{
  return TRUE;
}

BOOLEAN
ValidateException (
  IN  EFI_EXCEPTION_TYPE    ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT SystemContext
  )
{
  return TRUE;
}

