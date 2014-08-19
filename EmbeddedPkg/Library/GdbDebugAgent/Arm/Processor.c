/** @file
  Processor specific parts of the GDB stub

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <GdbDebugAgent.h>
#include <Library/PrintLib.h>
#include <Library/ArmLib.h>

//
// Externs from the exception handler assembly file
//
VOID
ExceptionHandlersStart (
  VOID
  );

VOID
ExceptionHandlersEnd (
  VOID
  );

VOID
CommonExceptionEntry (
  VOID
  );

VOID
AsmCommonExceptionEntry (
  VOID
  );


//
// Array of exception types that need to be hooked by the debugger
// (efi, gdb) //efi number
//
EFI_EXCEPTION_TYPE_ENTRY gExceptionType[] = {
  { EXCEPT_ARM_SOFTWARE_INTERRUPT,    GDB_SIGTRAP },
  { EXCEPT_ARM_UNDEFINED_INSTRUCTION, GDB_SIGTRAP },
  { EXCEPT_ARM_PREFETCH_ABORT,        GDB_SIGTRAP },
  { EXCEPT_ARM_DATA_ABORT,            GDB_SIGTRAP },    // GDB_SIGEMT
  { EXCEPT_ARM_RESERVED,              GDB_SIGTRAP },    // GDB_SIGILL
  { EXCEPT_ARM_FIQ,                   GDB_SIGINT }      // Used for ctrl-c
};

// Shut up some annoying RVCT warnings
#ifdef __CC_ARM
#pragma diag_suppress 1296
#endif

UINTN gRegisterOffsets[] = {
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R0),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R1),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R2),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R3),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R4),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R5),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R6),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R7),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R8),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R9),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R10),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R11),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, R12),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, SP),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, LR),
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, PC),
  0x00000F01,                               // f0
  0x00000F02,
  0x00000F03,
  0x00000F11,                               // f1
  0x00000F12,
  0x00000F13,
  0x00000F21,                               // f2
  0x00000F22,
  0x00000F23,
  0x00000F31,                               // f3
  0x00000F32,
  0x00000F33,
  0x00000F41,                               // f4
  0x00000F42,
  0x00000F43,
  0x00000F51,                               // f5
  0x00000F52,
  0x00000F53,
  0x00000F61,                               // f6
  0x00000F62,
  0x00000F63,
  0x00000F71,                               // f7
  0x00000F72,
  0x00000F73,
  0x00000FFF,                               // fps
  OFFSET_OF(EFI_SYSTEM_CONTEXT_ARM, CPSR)
};

// restore warnings for RVCT
#ifdef __CC_ARM
#pragma diag_default 1296
#endif


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
FindPointerToRegister(
  IN  EFI_SYSTEM_CONTEXT      SystemContext,
  IN  UINTN           RegNumber
  )
{
  UINT8 *TempPtr;
  ASSERT(gRegisterOffsets[RegNumber] < 0xF00);
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
  UINTN RegSize;
  CHAR8 Char;

  if (gRegisterOffsets[RegNumber] > 0xF00) {
    AsciiSPrint(OutBufPtr, 9, "00000000");
    OutBufPtr += 8;
    return OutBufPtr;
  }

  RegSize = 0;
  while (RegSize < 32) {
    Char = mHexToStr[(UINT8)((*FindPointerToRegister(SystemContext, RegNumber) >> (RegSize+4)) & 0xf)];
    if ((Char >= 'A') && (Char <= 'F')) {
      Char = Char - 'A' + 'a';
    }
    *OutBufPtr++ = Char;

    Char = mHexToStr[(UINT8)((*FindPointerToRegister(SystemContext, RegNumber) >> RegSize) & 0xf)];
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
  UINTN RegNumber;
  CHAR8 OutBuffer[9]; // 1 reg=8 hex chars, and the end '\0' (escape seq)
  CHAR8 *OutBufPtr;   // pointer to the output buffer

  RegNumber = AsciiStrHexToUintn (&InBuffer[1]);

  if (RegNumber >= (sizeof (gRegisterOffsets)/sizeof (UINTN))) {
    SendError (GDB_EINVALIDREGNUM);
    return;
  }

  OutBufPtr = OutBuffer;
  OutBufPtr = BasicReadRegister (SystemContext, RegNumber, OutBufPtr);

  *OutBufPtr = '\0';  // the end of the buffer
  SendPacket(OutBuffer);
}


/**
 Reads the general registers into an output buffer  and sends it as a packet
 @param   SystemContext     Register content at time of the exception
 **/
VOID
EFIAPI
ReadGeneralRegisters (
  IN  EFI_SYSTEM_CONTEXT      SystemContext
  )
{
  UINTN   Index;
  // a UINT32 takes 8 ascii characters
  CHAR8   OutBuffer[(sizeof (gRegisterOffsets) * 2) + 1];
  CHAR8   *OutBufPtr;

  // It is not safe to allocate pool here....
  OutBufPtr = OutBuffer;
  for (Index = 0; Index < (sizeof (gRegisterOffsets)/sizeof (UINTN)); Index++) {
    OutBufPtr = BasicReadRegister (SystemContext, Index, OutBufPtr);
  }

  *OutBufPtr = '\0';
  SendPacket(OutBuffer);
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
  IN  EFI_SYSTEM_CONTEXT      SystemContext,
  IN  UINTN           RegNumber,
  IN  CHAR8           *InBufPtr
  )
{
  UINTN RegSize;
  UINTN TempValue; // the value transferred from a hex char
  UINT32 NewValue; // the new value of the RegNumber-th Register

  if (gRegisterOffsets[RegNumber] > 0xF00) {
    return InBufPtr + 8;
  }

  NewValue = 0;
  RegSize = 0;
  while (RegSize < 32) {
    TempValue = HexCharToInt(*InBufPtr++);

    if ((INTN)TempValue < 0) {
      SendError (GDB_EBADMEMDATA);
      return NULL;
    }

    NewValue += (TempValue << (RegSize+4));
    TempValue = HexCharToInt(*InBufPtr++);

    if ((INTN)TempValue < 0) {
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
  if (RegNumber >= (sizeof (gRegisterOffsets)/sizeof (UINTN))) {
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
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *InBuffer
  )
{
  UINTN  i;
  CHAR8  *InBufPtr; /// pointer to the input buffer
  UINTN  MinLength;
  UINTN  RegisterCount = (sizeof (gRegisterOffsets)/sizeof (UINTN));

  MinLength = (RegisterCount * 8) + 1;  // 'G' plus the registers in ASCII format

  if (AsciiStrLen(InBuffer) < MinLength) {
    //Bad message. Message is not the right length
    SendError (GDB_EBADBUFSIZE);
    return;
  }

  InBufPtr = &InBuffer[1];

  // Read the new values for the registers from the input buffer to an array, NewValueArray.
  // The values in the array are in the gdb ordering
  for(i = 0; i < RegisterCount; i++) {
    InBufPtr = BasicWriteRegister (SystemContext, i, InBufPtr);
  }

  SendSuccess ();
}




/**
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
    SystemContext.SystemContextArm->PC = AsciiStrHexToUintn(&PacketData[1]);
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
  IN    CHAR8                       *PacketData
  )
{
  SendNotSupported();
}


VOID
EFIAPI
InsertBreakPoint (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8              *PacketData
  )
{
  SendNotSupported ();
}

VOID
EFIAPI
RemoveBreakPoint (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *PacketData
  )
{
  SendNotSupported ();
}


/**
 Send the T signal with the given exception type (in gdb order) and possibly
 with n:r pairs related to the watchpoints

 @param  SystemContext        Register content at time of the exception
 @param  GdbExceptionType     GDB exception type
 **/
VOID
ProcessorSendTSignal (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINT8               GdbExceptionType,
  IN  OUT CHAR8           *TSignalPtr,
  IN  UINTN               SizeOfBuffer
  )
{
  *TSignalPtr = '\0';
}

/**
 FIQ state is only changed by FIQ exception. We don't want to take FIQ
 ticks in the GDB stub. The stub disables FIQ on entry, but this is the
 third instruction that executes in the execption handler. Thus we have
 a crack we need to test for.

 @param PC     PC of execption

 @return  TRUE  We are in the GDB stub exception preamble
 @return  FALSE We are not in GDB stub code
 **/
BOOLEAN
InFiqCrack (
  IN UINT32 PC
  )
{
  UINT32 VectorBase = PcdGet32 (PcdCpuVectorBaseAddress);
  UINT32 Length     = (UINTN)ExceptionHandlersEnd - (UINTN)ExceptionHandlersStart;

  if ((PC >= VectorBase) && (PC <= (VectorBase + Length))) {
    return TRUE;
  }

  return FALSE;
}


/**
 Check to see if this exception is related to ctrl-c handling.

 In this scheme we dedicate FIQ to the ctrl-c handler so it is
 independent of the rest of the system.

 SaveAndSetDebugTimerInterrupt () can be used to

 @param ExceptionType     Exception that is being processed
 @param SystemContext     Register content at time of the exception

 @return  TRUE  This was a ctrl-c check that did not find a ctrl-c
 @return  FALSE This was not a ctrl-c check or some one hit ctrl-c
 **/
BOOLEAN
ProcessorControlC (
  IN  EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT     SystemContext
  )
{
  CHAR8     Char;
  BOOLEAN   Return = TRUE;

  if (ExceptionType != EXCEPT_ARM_FIQ) {
    // Skip it as it is not related to ctrl-c
    return FALSE;
  }

  if (InFiqCrack (SystemContext.SystemContextArm->PC)) {
    // We are in our own interrupt preable, so skip this tick.
    // We never want to let gdb see the debug stub running if we can help it
    return FALSE;
  }

  while (TRUE) {
    if (!GdbIsCharAvailable ()) {
      //
      // No characters are pending so exit the loop
      //
      Return = TRUE;
      break;
    }

    Char = GdbGetChar ();
    if (Char == 0x03) {
      //
      // We have a ctrl-c so exit and process exception for ctrl-c
      //
      Return = FALSE;
      break;
    }
  }

  DebugAgentTimerEndOfInterrupt ();

  //  Force an exit from the exception handler as we are done
  return Return;
}


/**
  Enable/Disable the interrupt of debug timer and return the interrupt state
  prior to the operation.

  If EnableStatus is TRUE, enable the interrupt of debug timer.
  If EnableStatus is FALSE, disable the interrupt of debug timer.

  @param[in] EnableStatus    Enable/Disable.

  @retval TRUE  Debug timer interrupt were enabled on entry to this call.
  @retval FALSE Debug timer interrupt were disabled on entry to this call.

**/
BOOLEAN
EFIAPI
SaveAndSetDebugTimerInterrupt (
  IN BOOLEAN                EnableStatus
  )
{
  BOOLEAN              FiqEnabled;

  FiqEnabled = ArmGetFiqState ();

  if (EnableStatus) {
    DebugAgentTimerSetPeriod (PcdGet32 (PcdGdbTimerPeriodMilliseconds));
    ArmEnableFiq ();
  } else {
    DebugAgentTimerSetPeriod (0);
    ArmDisableFiq ();
  }

  return FiqEnabled;
}



VOID
GdbFPutString (
  IN CHAR8  *String
  );

/**
  Initialize debug agent.

  This function is used to set up debug environment to support source level debugging.
  If certain Debug Agent Library instance has to save some private data in the stack,
  this function must work on the mode that doesn't return to the caller, then
  the caller needs to wrap up all rest of logic after InitializeDebugAgent() into one
  function and pass it into InitializeDebugAgent(). InitializeDebugAgent() is
  responsible to invoke the passing-in function at the end of InitializeDebugAgent().

  If the parameter Function is not NULL, Debug Agent Libary instance will invoke it by
  passing in the Context to be its parameter.

  If Function() is NULL, Debug Agent Library instance will return after setup debug
  environment.

  @param[in] InitFlag     Init flag is used to decide the initialize process.
  @param[in] Context      Context needed according to InitFlag; it was optional.
  @param[in] Function     Continue function called by debug agent library; it was
                          optional.

**/
VOID
EFIAPI
InitializeDebugAgent (
  IN UINT32                InitFlag,
  IN VOID                  *Context, OPTIONAL
  IN DEBUG_AGENT_CONTINUE  Function  OPTIONAL
  )
{
  UINTN                Offset;
  UINTN                Length;
  BOOLEAN              IrqEnabled;
  UINT32               *VectorBase;


  //
  // Disable interrupts
  //
  IrqEnabled = ArmGetInterruptState ();
  ArmDisableInterrupts ();
  ArmDisableFiq ();

  //
  // Copy an implementation of the ARM exception vectors to PcdCpuVectorBaseAddress.
  //
  Length = (UINTN)ExceptionHandlersEnd - (UINTN)ExceptionHandlersStart;

  //
  // Reserve space for the exception handlers
  //
  VectorBase = (UINT32 *)(UINTN)PcdGet32 (PcdCpuVectorBaseAddress);


  // Copy our assembly code into the page that contains the exception vectors.
  CopyMem ((VOID *)VectorBase, (VOID *)ExceptionHandlersStart, Length);

  //
  // Patch in the common Assembly exception handler
  //
  Offset = (UINTN)CommonExceptionEntry - (UINTN)ExceptionHandlersStart;
  *(UINTN *) (((UINT8 *)VectorBase) + Offset) = (UINTN)AsmCommonExceptionEntry;

  // Flush Caches since we updated executable stuff
  InvalidateInstructionCacheRange ((VOID *)PcdGet32(PcdCpuVectorBaseAddress), Length);

  // setup a timer so gdb can break in via ctrl-c
  DebugAgentTimerIntialize ();

  if (IrqEnabled) {
    ArmEnableInterrupts ();
  }

  if (Function != NULL) {
    Function (Context);
  }

  return;
}

