/** @file
  Commond Debug Agent library implementition. It mainly includes
  the first C function called by exception/interrupt handlers,
  read/write debug packet to communication with HOST based on transfer
  protocol.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DebugAgent.h"
#include "Ia32/DebugException.h"

#define INIT_BREAK_ACK_TIMEOUT  (200 * 1000)

CHAR8 mErrorMsgVersionAlert[]   = "\rThe SourceLevelDebugPkg you are using requires a newer version of the Intel(R) UDK Debugger Tool.\r\n";
CHAR8 mErrorMsgSendInitPacket[] = "\rSend INIT break packet to HOST ...\r\n";
CHAR8 mErrorMsgConnectOK[]      = "HOST connection is successful!\r\n";
CHAR8 mErrorMsgConnectFail[]    = "HOST connection is failed!\r\n";

/**
  Send a debug message packet to the debug port.

  @param[in] Buffer  The debug message.
  @param[in] Length  The length of debug message.

**/
VOID
SendDebugMsgPacket (
  IN CHAR8         *Buffer,
  IN UINTN         Length         
  )
{
  DEBUG_PACKET_HEADER  DebugHeader;
  DEBUG_PORT_HANDLE    Handle;
  
  Handle = GetDebugPortHandle();

  DebugHeader.StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;
  DebugHeader.Command     = DEBUG_COMMAND_PRINT_MESSAGE;
  DebugHeader.Length      = sizeof (DEBUG_PACKET_HEADER) + (UINT8) Length;
  DebugHeader.CheckSum    = 0;
  DebugHeader.CheckSum    = CalculateCheckSum8 ((UINT8 *)&DebugHeader, sizeof (DEBUG_PACKET_HEADER));

  DebugPortWriteBuffer (Handle, (UINT8 *)&DebugHeader, sizeof (DEBUG_PACKET_HEADER));
  DebugPortWriteBuffer (Handle, (UINT8 *)Buffer, Length);
}

/**
  Prints a debug message to the debug port if the specified error level is enabled.

  If any bit in ErrorLevel is also set in Mainbox, then print the message specified
  by Format and the associated variable argument list to the debug port.

  @param[in] ErrorLevel  The error level of the debug message.
  @param[in] Format      Format string for the debug message to print.
  @param[in] ...         Variable argument list whose contents are accessed 
                         based on the format string specified by Format.

**/
VOID
EFIAPI
DebugAgentMsgPrint (
  IN UINT8         ErrorLevel,
  IN CHAR8         *Format,
  ...
  )
{
  DEBUG_AGENT_MAILBOX  *Mailbox;
  CHAR8                Buffer[DEBUG_DATA_MAXIMUM_REAL_DATA];
  VA_LIST              Marker;

  Mailbox = GetMailboxPointer ();
  //
  // Check driver debug mask value and global mask
  //
  if ((ErrorLevel & Mailbox->DebugFlag.PrintErrorLevel) == 0) {
    return;
  }

  //
  // Convert the DEBUG() message to an ASCII String
  //
  VA_START (Marker, Format);
  AsciiVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  SendDebugMsgPacket (Buffer, AsciiStrLen (Buffer));
}

/**
  Prints a debug message to the debug output device if the specified error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function 
  GetDebugPrintErrorLevel (), then print the message specified by Format and the 
  associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param[in] ErrorLevel  The error level of the debug message.
  @param[in] IsSend      Flag of debug message to declare that the data is being sent or being received.
  @param[in] Data        Variable argument list whose contents are accessed 
  @param[in] Length      based on the format string specified by Format.

**/
VOID
EFIAPI
DebugAgentDataMsgPrint (
  IN UINT8             ErrorLevel,
  IN BOOLEAN           IsSend,
  IN UINT8             *Data,
  IN UINT8             Length  
  )
{
  DEBUG_AGENT_MAILBOX  *Mailbox;
  CHAR8                Buffer[DEBUG_DATA_MAXIMUM_REAL_DATA];
  CHAR8                *DestBuffer;
  UINTN                Index;

  Mailbox = GetMailboxPointer ();
  //
  // Check driver debug mask value and global mask
  //
  if ((ErrorLevel & Mailbox->DebugFlag.PrintErrorLevel) == 0) {
    return;
  }

  DestBuffer = Buffer;
  if (IsSend) {
    DestBuffer += AsciiSPrint (DestBuffer, DEBUG_DATA_MAXIMUM_REAL_DATA, "Sent data [ ");
  } else {
    DestBuffer += AsciiSPrint (DestBuffer, DEBUG_DATA_MAXIMUM_REAL_DATA, "Received data [ ");
  }

  Index = 0;
  while (TRUE) {
    if (DestBuffer - Buffer > DEBUG_DATA_MAXIMUM_REAL_DATA - 6) {
      //
      // If there was no enough space in buffer, send out the debug message, 
      // reserving 6 bytes is for the last data and end characters "]\n".
      //
      SendDebugMsgPacket (Buffer, DestBuffer - Buffer);
      DestBuffer = Buffer;
    }
    DestBuffer += AsciiSPrint (DestBuffer, DEBUG_DATA_MAXIMUM_REAL_DATA - (DestBuffer - Buffer), "%02x ", Data[Index]);
    Index ++;
    if (Index >= Length) {
      //s
      // The last character of debug message has been foramtted in buffer
      //
      DestBuffer += AsciiSPrint(DestBuffer, DEBUG_DATA_MAXIMUM_REAL_DATA - (DestBuffer - Buffer), "]\n");
      SendDebugMsgPacket (Buffer, DestBuffer - Buffer);
      break;
    }
  }
}


/**
  Check if HOST is attached based on Mailbox.

  @retval TRUE        HOST is attached.
  @retval FALSE       HOST is not attached.

**/
BOOLEAN
IsHostAttached (
  VOID
  )
{
  return (BOOLEAN) (GetMailboxPointer ()->DebugFlag.HostAttached == 1);
}

/**
  Set HOST connect flag in Mailbox.

  @param[in] Attached        Attach status.
  
**/
VOID
SetHostAttached (
  IN BOOLEAN                      Attached
  )
{
  DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Attach status is %d\n", Attached);
  GetMailboxPointer ()->DebugFlag.HostAttached = Attached;
}

/**
  Set debug setting of Debug Agent in Mailbox.

  @param DebugSetting         Pointer to Debug Setting defined by transfer protocol.
  
  @retval RETURN_SUCCESS      The setting is set successfully.
  @retval RETURN_UNSUPPORTED  The Key value is not supported.

**/
RETURN_STATUS
SetDebugSetting (
  IN DEBUG_DATA_SET_DEBUG_SETTING  *DebugSetting               
  )
{
  RETURN_STATUS                Status;
  DEBUG_AGENT_MAILBOX          *Mailbox;

  Mailbox = GetMailboxPointer ();

  Status = RETURN_SUCCESS;
  switch (DebugSetting->Key) {
  case DEBUG_AGENT_SETTING_SMM_ENTRY_BREAK:
    Mailbox->DebugFlag.BreakOnNextSmi = DebugSetting->Value;
    break;
  case DEBUG_AGENT_SETTING_PRINT_ERROR_LEVEL:
    Mailbox->DebugFlag.PrintErrorLevel = DebugSetting->Value;
    break;
  default:
    Status = RETURN_UNSUPPORTED;
  }
  return Status;
}

/**
  Exectue GO command.

  @param[in] CpuContext        Pointer to saved CPU context.

**/
VOID
CommandGo (
  IN DEBUG_CPU_CONTEXT         *CpuContext
  )
{
  IA32_EFLAGS32                *Eflags;

  Eflags = (IA32_EFLAGS32 *) &CpuContext->Eflags;
  Eflags->Bits.TF = 0;
  Eflags->Bits.RF = 1;
}

/**
  Exectue Stepping command.

  @param[in] CpuContext        Pointer to saved CPU context.

**/
VOID
CommandStepping (
  IN DEBUG_CPU_CONTEXT          *CpuContext
  )
{
  IA32_EFLAGS32                *Eflags;

  Eflags = (IA32_EFLAGS32 *) &CpuContext->Eflags;
  Eflags->Bits.TF = 1;
  Eflags->Bits.RF = 1;
}

/**
  Set debug register for hardware breakpoint.

  @param[in] CpuContext      Pointer to saved CPU context.
  @param[in] SetHwBreakpoint Hardware breakpoint to be set.

**/
VOID
SetDebugRegister (
  IN DEBUG_CPU_CONTEXT             *CpuContext,
  IN DEBUG_DATA_SET_HW_BREAKPOINT  *SetHwBreakpoint
  )
{
  UINT8                      RegisterIndex;
  UINTN                      Dr7Value;

  RegisterIndex = SetHwBreakpoint->Type.Index;

  //
  // Set debug address
  //
  * ((UINTN *) &CpuContext->Dr0 + RegisterIndex) = (UINTN) SetHwBreakpoint->Address;

  Dr7Value = CpuContext->Dr7;

  //
  // Enable Gx, Lx
  //
  Dr7Value |= 0x3 << (RegisterIndex * 2);
  //
  // Set RWx and Lenx
  //
  Dr7Value &= ~(0xf << (16 + RegisterIndex * 4));
  Dr7Value |= (UINTN) ((SetHwBreakpoint->Type.Length << 2) | SetHwBreakpoint->Type.Access) << (16 + RegisterIndex * 4);
  //
  // Enable GE, LE
  //
  Dr7Value |= 0x300;

  CpuContext->Dr7 = Dr7Value;
}

/**
  Clear debug register for hardware breakpoint.

  @param[in] CpuContext        Pointer to saved CPU context.
  @param[in] ClearHwBreakpoint Hardware breakpoint to be cleared.

**/
VOID
ClearDebugRegister (
  IN DEBUG_CPU_CONTEXT                 *CpuContext,
  IN DEBUG_DATA_CLEAR_HW_BREAKPOINT    *ClearHwBreakpoint
  )
{
  if ((ClearHwBreakpoint->IndexMask & BIT0) != 0) {
    CpuContext->Dr0 = 0;
    CpuContext->Dr7 &= ~(0x3 << 0);
  }
  if ((ClearHwBreakpoint->IndexMask & BIT1) != 0) {
    CpuContext->Dr1 = 0;
    CpuContext->Dr7 &= ~(0x3 << 2);
  }
  if ((ClearHwBreakpoint->IndexMask & BIT2) != 0) {
    CpuContext->Dr2 = 0;
    CpuContext->Dr7 &= ~(0x3 << 4);
  }
  if ((ClearHwBreakpoint->IndexMask & BIT3) != 0) {
    CpuContext->Dr3 = 0;
    CpuContext->Dr7 &= ~(0x3 << 6);
  }
}


/**
  Return the offset of FP / MMX / XMM registers in the FPU saved state by register index.

  @param[in]  Index    Register index.
  @param[out] Width    Register width returned.

  @return Offset in the FPU Save State.

**/
UINT16
ArchReadFxStatOffset (
  IN  UINT8                     Index,
  OUT UINT8                     *Width
  )
{
  if (Index < SOFT_DEBUGGER_REGISTER_ST0) {
    switch (Index) {
    case SOFT_DEBUGGER_REGISTER_FP_FCW:
      *Width = (UINT8) sizeof (UINT16);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Fcw);

    case SOFT_DEBUGGER_REGISTER_FP_FSW:
      *Width = (UINT8) sizeof (UINT16);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Fsw);

    case SOFT_DEBUGGER_REGISTER_FP_FTW:
      *Width = (UINT8) sizeof (UINT16);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Ftw);

    case SOFT_DEBUGGER_REGISTER_FP_OPCODE:
      *Width = (UINT8) sizeof (UINT16);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Opcode);

    case SOFT_DEBUGGER_REGISTER_FP_EIP:
      *Width = (UINT8) sizeof (UINT32);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Eip);

    case SOFT_DEBUGGER_REGISTER_FP_CS:
      *Width = (UINT8) sizeof (UINT16);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Cs);

    case SOFT_DEBUGGER_REGISTER_FP_DATAOFFSET:
      *Width = (UINT8) sizeof (UINT32);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, DataOffset);

    case SOFT_DEBUGGER_REGISTER_FP_DS:
      *Width = (UINT8) sizeof (UINT16);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Ds);

    case SOFT_DEBUGGER_REGISTER_FP_MXCSR:
      *Width = (UINT8) sizeof (UINT32);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Mxcsr);

    case SOFT_DEBUGGER_REGISTER_FP_MXCSR_MASK:
      *Width = (UINT8) sizeof (UINT32);
      return OFFSET_OF(DEBUG_DATA_FX_SAVE_STATE, Mxcsr_Mask);
    }
  }

  if (Index <= SOFT_DEBUGGER_REGISTER_ST7) {
    *Width = 10;
  } else if (Index <= SOFT_DEBUGGER_REGISTER_XMM15) {
    *Width = 16;
  } else {
    //
    // MMX register
    //
    *Width = 8;
    Index -= SOFT_DEBUGGER_REGISTER_MM0 - SOFT_DEBUGGER_REGISTER_ST0;
  }

  return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, St0Mm0) + (Index - SOFT_DEBUGGER_REGISTER_ST0) * 16;
}

/**
  Return the pointer of the register value in the CPU saved context.

  @param[in]  CpuContext         Pointer to saved CPU context.
  @param[in]  Index              Register index value.
  @param[out] Width              Data width to read.

  @return The pointer in the CPU saved context.

**/
UINT8 *
ArchReadRegisterBuffer (
  IN DEBUG_CPU_CONTEXT               *CpuContext,
  IN UINT8                           Index,
  OUT UINT8                          *Width
  )
{
  UINT8           *Buffer;

  if (Index < SOFT_DEBUGGER_REGISTER_FP_BASE) {
    Buffer = (UINT8 *) CpuContext + OFFSET_OF (DEBUG_CPU_CONTEXT, Dr0) + Index * sizeof (UINTN);
    *Width = (UINT8) sizeof (UINTN);
  } else {
    //
    // FPU/MMX/XMM registers
    //
    Buffer = (UINT8 *) CpuContext + OFFSET_OF (DEBUG_CPU_CONTEXT, FxSaveState) + ArchReadFxStatOffset (Index, Width);
  }

  return Buffer;
}

/**
  Send the packet without data to HOST.

  @param[in] CommandType    Type of Command.

**/
VOID
SendPacketWithoutData (
  IN UINT8                  CommandType
  )
{
  DEBUG_PACKET_HEADER       DebugHeader;
  DEBUG_PORT_HANDLE         Handle;

  Handle = GetDebugPortHandle();

  DebugHeader.StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;
  DebugHeader.Command     = CommandType;
  DebugHeader.Length      = sizeof (DEBUG_PACKET_HEADER);
  DebugHeader.CheckSum    = 0;
  DebugHeader.CheckSum    = CalculateCheckSum8 ((UINT8 *)&DebugHeader, sizeof (DEBUG_PACKET_HEADER));

  DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, TRUE, (UINT8 *) &DebugHeader, DebugHeader.Length);
  DebugPortWriteBuffer (Handle, (UINT8 *) &DebugHeader, DebugHeader.Length);
}

/**
  Send acknowledge packet to HOST.

  @param[in] AckCommand    Type of Acknowledge packet.

**/
VOID
SendAckPacket (
  IN UINT8                AckCommand
  )
{
  if (AckCommand != DEBUG_COMMAND_OK) {
    DebugAgentMsgPrint (DEBUG_AGENT_ERROR, "Send ACK(%d)\n", AckCommand);
  }
  SendPacketWithoutData (AckCommand);
}

/**
  Receive acknowledge packet from HOST in specified time.

  @param[out] Ack            Returned acknowlege type from HOST.
  @param[in]  Timeout        Time out value to wait for acknowlege from HOST.
                             The unit is microsecond.
  @param[out] BreakReceived  If BreakReceived is not NULL,
                             TRUE is retured if break-in symbol received.
                             FALSE is retured if break-in symbol not received.
  @param[out] CheckSumStatus If CheckSumStatus is not NULL,
                             RETURN_SUCCESS   CheckSum is OK.
                             RETURN_NOT_FOUND Not find the CheckSum field.

  @retval  RETRUEN_SUCCESS  Succeed to receive acknowlege packet from HOST,
                            the type of acknowlege packet saved in Ack.
  @retval  RETURN_TIMEOUT   Specified timeout value was up.

**/
RETURN_STATUS
ReceiveAckPacket (
  OUT UINT8                     *Ack,
  IN  UINTN                     Timeout,
  OUT BOOLEAN                   *BreakReceived, OPTIONAL
  OUT RETURN_STATUS             *CheckSumStatus OPTIONAL
  )
{
  DEBUG_PACKET_HEADER       DebugHeader;
  DEBUG_PORT_HANDLE         Handle;

  Handle = GetDebugPortHandle();

  while (TRUE) {
    if (DebugPortReadBuffer (Handle, (UINT8 *) &DebugHeader.StartSymbol, sizeof (DebugHeader.StartSymbol), Timeout) == 0) {
      return RETURN_TIMEOUT;
    }
    if (DebugHeader.StartSymbol == DEBUG_STARTING_SYMBOL_BREAK) {
      if (BreakReceived != NULL) {
        SendAckPacket (DEBUG_COMMAND_HALT_DEFERRED);
        *BreakReceived = TRUE;
      }
    }
    if (DebugHeader.StartSymbol == DEBUG_STARTING_SYMBOL_NORMAL) {
      break;
    }
    DebugAgentMsgPrint (DEBUG_AGENT_ERROR, "Invalid start symbol received [%02x]\n", DebugHeader.StartSymbol);
  }
  //
  // Read ACK packet header till field Length (not including StartSymbol and CheckSum)
  //
  DebugHeader.Length = 0;
  if (DebugPortReadBuffer (
        Handle,
        (UINT8 *)&DebugHeader.Command,
        OFFSET_OF (DEBUG_PACKET_HEADER, Length) + sizeof (DebugHeader.Length) - sizeof (DebugHeader.StartSymbol),
        Timeout
        ) == 0) {
    return RETURN_TIMEOUT;
  }

  if (DebugHeader.Length == 0) {
    //
    // The CheckSum field does not exist
    //
    if (CheckSumStatus != NULL) {
      *CheckSumStatus = RETURN_NOT_FOUND;
    }
  } else {
    if (CheckSumStatus != NULL) {
      *CheckSumStatus = RETURN_SUCCESS;
    }
    if (DebugPortReadBuffer (Handle, &DebugHeader.CheckSum, sizeof (DebugHeader.CheckSum), Timeout) == 0) {
      return RETURN_TIMEOUT;
    }
  }

  DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, FALSE, (UINT8 *)&DebugHeader, DebugHeader.Length);
  *Ack = DebugHeader.Command;
  return RETURN_SUCCESS;
}

/**
  Receive acknowledge packet OK from HOST in specified time.

  @param[in]  Command        The command type issued by TARGET.
  @param[in]  Timeout        Time out value to wait for acknowlege from HOST.
                             The unit is microsecond.
  @param[out] BreakReceived  If BreakReceived is not NULL,
                             TRUE is retured if break-in symbol received.
                             FALSE is retured if break-in symbol not received.
  @param[out] CheckSumStatus If CheckSumStatus is not NULL,
                             RETURN_SUCCESS   CheckSum is OK.
                             RETURN_NOT_FOUND Not find the CheckSum field.

  @retval  RETRUEN_SUCCESS  Succeed to receive acknowlege packet from HOST,
                            the type of acknowlege packet saved in Ack.
  @retval  RETURN_TIMEOUT   Specified timeout value was up.

**/
RETURN_STATUS
SendCommandAndWaitForAckOK (
  IN  UINT8               Command,
  IN  UINTN               Timeout,
  OUT BOOLEAN             *BreakReceived, OPTIONAL
  OUT RETURN_STATUS       *CheckSumStatus OPTIONAL
  )
{
  RETURN_STATUS           Status;
  UINT8                   Ack;
  
  while (TRUE) {
    SendPacketWithoutData (Command);
    while (TRUE) {
      Status = ReceiveAckPacket (&Ack, Timeout, BreakReceived, CheckSumStatus);
      if (Status == RETURN_SUCCESS && Ack == DEBUG_COMMAND_RESEND) {
        //
        // Resend the last command
        //
        break;
      } 
      if ((Status == RETURN_SUCCESS && Ack == DEBUG_COMMAND_OK) ||
           Status == RETURN_TIMEOUT) {
        //
        // Received Ack OK or timeout
        //
        return Status;
      }  
    }
  }
}

/**
  Receive valid packet from HOST.

  @param[out] InputPacket    Buffer to receive packet.
  @param[out] BreakReceived  TRUE means break-in symbol received.
                             FALSE means break-in symbol not received.

  @retval RETURN_SUCCESS   A valid package was reveived in InputPacket.
  @retval RETURN_TIMEOUT   Timeout occurs.

**/
RETURN_STATUS
ReceivePacket (
  OUT UINT8             *InputPacket,
  OUT BOOLEAN           *BreakReceived
  )
{
  DEBUG_PACKET_HEADER   *DebugHeader;
  UINTN                 Received;
  DEBUG_PORT_HANDLE     Handle;
  UINT8                 CheckSum;

  Handle = GetDebugPortHandle();
  
  DebugHeader = (DEBUG_PACKET_HEADER *) InputPacket;
  while (TRUE) {
    //
    // Find the valid start symbol
    //
    DebugPortReadBuffer (Handle, &DebugHeader->StartSymbol, sizeof (DebugHeader->StartSymbol), 0);

    if (DebugHeader->StartSymbol == DEBUG_STARTING_SYMBOL_BREAK) {
      *BreakReceived = TRUE;
      SendAckPacket (DEBUG_COMMAND_HALT_DEFERRED);
    }

    if (DebugHeader->StartSymbol != DEBUG_STARTING_SYMBOL_NORMAL) {
      DebugAgentMsgPrint (DEBUG_AGENT_ERROR, "Invalid start symbol received [%02x]\n", DebugHeader->StartSymbol);
      continue;
    }

    //
    // Read Package header except for checksum
    //
    Received = DebugPortReadBuffer (
                 Handle,
                 &DebugHeader->Command,
                 OFFSET_OF (DEBUG_PACKET_HEADER, Length) + sizeof (DebugHeader->Length) - sizeof (DebugHeader->StartSymbol),
                 0
                 );
    if (Received == 0) {
      return RETURN_TIMEOUT;
    }

    //
    // Read the payload data include the checksum
    //
    Received = DebugPortReadBuffer (Handle, &DebugHeader->CheckSum, DebugHeader->Length - OFFSET_OF (DEBUG_PACKET_HEADER, CheckSum), 0);
    if (Received == 0) {
      return RETURN_TIMEOUT;
    }
    //
    // Calculate the checksum of Debug Packet
    //
    CheckSum = CalculateCheckSum8 ((UINT8 *) DebugHeader, DebugHeader->Length);
    if (CheckSum == 0) {
      break;
    }
    DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "CheckSum Error (Caculated checksum is %x, received checksum is %x\n", CheckSum, DebugHeader->CheckSum);
    DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "Send DEBUG_COMMAND_RESEND command.\n");
    SendAckPacket (DEBUG_COMMAND_RESEND);
  }

  DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, FALSE, (UINT8 *) DebugHeader, DebugHeader->Length);

  return RETURN_SUCCESS;
}

/**
  Get current break cause.

  @param[in] Vector      Vector value of exception or interrupt.
  @param[in] CpuContext  Pointer to save CPU context.

  @return The type of break cause defined by XXXX

**/
UINT8
GetBreakCause (
  IN UINTN                    Vector,
  IN DEBUG_CPU_CONTEXT        *CpuContext
  )
{
  UINT8                    Cause;

  Cause = DEBUG_DATA_BREAK_CAUSE_UNKNOWN;

  switch (Vector) {
  case DEBUG_INT1_VECTOR:
  case DEBUG_INT3_VECTOR:

    if (Vector == DEBUG_INT1_VECTOR) {
      //
      // INT 1
      //
      if ((CpuContext->Dr6 & BIT14) != 0) {
        Cause = DEBUG_DATA_BREAK_CAUSE_STEPPING;
        //
        // If it's single step, no need to check DR0, to ensure single step work in PeCoffExtraActionLib
        // (right after triggering a breakpoint to report image load/unload).
        //
        return Cause;

      } else {
        Cause = DEBUG_DATA_BREAK_CAUSE_HW_BREAKPOINT;
      }
    } else {
      //
      // INT 3
      //
      Cause = DEBUG_DATA_BREAK_CAUSE_SW_BREAKPOINT;
    }

    switch (CpuContext->Dr0) {
    case IMAGE_LOAD_SIGNATURE:
    case IMAGE_UNLOAD_SIGNATURE:

      if (CpuContext->Dr3 == IO_PORT_BREAKPOINT_ADDRESS) {

        Cause = (UINT8) ((CpuContext->Dr0 == IMAGE_LOAD_SIGNATURE) ? 
          DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD : DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD);
      }
      break;

    case SOFT_INTERRUPT_SIGNATURE:
   
      if (CpuContext->Dr1 == MEMORY_READY_SIGNATURE) {
        Cause = DEBUG_DATA_BREAK_CAUSE_MEMORY_READY;
        CpuContext->Dr0 = 0;
      } else if (CpuContext->Dr1 == SYSTEM_RESET_SIGNATURE) {
        Cause = DEBUG_DATA_BREAK_CAUSE_SYSTEM_RESET;
        CpuContext->Dr0 = 0;
      }
      break;

    default:
      break;

    }

    break;

  case DEBUG_TIMER_VECTOR:
    Cause = DEBUG_DATA_BREAK_CAUSE_USER_HALT;
    break;

  default:
    if (Vector < 20) {
      if (GetMailboxPointer()->DebugFlag.SteppingFlag == 1) {
        //
        // If stepping command is executing
        //
        Cause = DEBUG_DATA_BREAK_CAUSE_STEPPING;
      } else {
        Cause = DEBUG_DATA_BREAK_CAUSE_EXCEPTION;
      }
    }
    break;
  }

  return Cause;
}

/**
  Send command packet with data to HOST.

  @param[in] Command     Command type.
  @param[in] Data        Pointer to response data buffer.
  @param[in] DataSize    Size of response data in byte.

  @retval RETURN_SUCCESS      Response data was sent successfully.
  @retval RETURN_DEVICE_ERROR Cannot receive DEBUG_COMMAND_OK from HOST.

**/
RETURN_STATUS
SendCommandWithDataPacket (
  IN UINT8                Command,
  IN UINT8                *Data,
  IN UINT16               DataSize
  )
{
  DEBUG_PACKET_HEADER  *DebugHeader;
  BOOLEAN              LastPacket;
  UINT8                Ack;
  UINT8                DebugPacket[DEBUG_DATA_UPPER_LIMIT];
  DEBUG_PORT_HANDLE    Handle;

  Handle = GetDebugPortHandle();

  DebugHeader = (DEBUG_PACKET_HEADER *) &DebugPacket;
  DebugHeader->StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;

  while (TRUE) {
    if (DataSize <= DEBUG_DATA_MAXIMUM_REAL_DATA) {
      LastPacket = TRUE;
      DebugHeader->Command  = Command;
      DebugHeader->Length   = (UINT8) (DataSize + sizeof (DEBUG_PACKET_HEADER));
      DebugHeader->CheckSum = 0;
      CopyMem (DebugHeader + 1, Data, DataSize);

    } else {
      LastPacket = FALSE;
      DebugHeader->Command  = DEBUG_COMMAND_IN_PROGRESS;
      DebugHeader->Length   = DEBUG_DATA_MAXIMUM_REAL_DATA + sizeof (DEBUG_PACKET_HEADER);
      DebugHeader->CheckSum = 0;
      CopyMem (DebugHeader + 1, Data, DEBUG_DATA_MAXIMUM_REAL_DATA);
    }

    //
    // Calculate and fill the checksum
    //
    DebugHeader->CheckSum = CalculateCheckSum8 ((UINT8 *) DebugHeader, DebugHeader->Length);

    DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, TRUE, (UINT8 *) DebugHeader, DebugHeader->Length);
    
    DebugPortWriteBuffer (Handle, (UINT8 *) DebugHeader, DebugHeader->Length);

    ReceiveAckPacket(&Ack, 0, NULL, NULL);
    switch (Ack) {
    case DEBUG_COMMAND_RESEND:
      //
      // Send the packet again
      //
      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "Received DEBUG_COMMAND_RESEND.\n");
      break;

    case DEBUG_COMMAND_CONTINUE:
      //
      // Send the rest packet
      //
      Data     += DEBUG_DATA_MAXIMUM_REAL_DATA;
      DataSize -= DEBUG_DATA_MAXIMUM_REAL_DATA;
      break;

    case DEBUG_COMMAND_OK:
      if (LastPacket) {
        //
        // If this is the last packet, return RETURN_SUCCESS.
        //
        return RETURN_SUCCESS;
      } else {
        return RETURN_DEVICE_ERROR;
      }

    default:
      return RETURN_DEVICE_ERROR;

    }
  }
}

/**
  Send packet with response data to HOST.

  @param[in] Data        Pointer to response data buffer.
  @param[in] DataSize    Size of response data in byte.

  @retval RETURN_SUCCESS      Response data was sent successfully.
  @retval RETURN_DEVICE_ERROR Cannot receive DEBUG_COMMAND_OK from HOST.

**/
RETURN_STATUS
SendDataResponsePacket (
  IN UINT8                *Data,
  IN UINT16               DataSize
  )
{
  return SendCommandWithDataPacket (DEBUG_COMMAND_OK, Data, DataSize);
}

/**
  Send break cause packet to HOST.

  @param[in] Vector      Vector value of exception or interrutp.
  @param[in] CpuContext  Pointer to save CPU context.

  @retval RETURN_SUCCESS      Response data was sent successfully.
  @retval RETURN_DEVICE_ERROR Cannot receive DEBUG_COMMAND_OK from HOST.

**/
RETURN_STATUS
SendBreakCausePacket (
  IN UINTN                    Vector,
  IN DEBUG_CPU_CONTEXT        *CpuContext
  )
{
  DEBUG_DATA_RESPONSE_BREAK_CAUSE    DebugDataBreakCause;

  DebugDataBreakCause.StopAddress = CpuContext->Eip;
  DebugDataBreakCause.Cause       = GetBreakCause (Vector, CpuContext);

  return SendDataResponsePacket ((UINT8 *) &DebugDataBreakCause, (UINT16) sizeof (DEBUG_DATA_RESPONSE_BREAK_CAUSE));
}

/**
  Try to attach the HOST.
  
  Send init break packet to HOST:
  If no acknowlege received in specified Timeout, return RETURN_TIMEOUT. 
  If received acknowlege, check the revision of HOST. 
  Set Attach Flag if attach successfully.  
  
  @param[in]  Timeout        Time out value to wait for acknowlege from HOST.
                             The unit is microsecond.
  @param[out] BreakReceived  If BreakReceived is not NULL,
                             TRUE is retured if break-in symbol received.
                             FALSE is retured if break-in symbol not received.
**/
RETURN_STATUS
AttachHost (
  IN  UINTN                Timeout,
  OUT BOOLEAN              *BreakReceived
  )
{
  RETURN_STATUS                    Status;
  DEBUG_PORT_HANDLE                Handle;
  RETURN_STATUS                    CheckSumStatus;

  Handle = GetDebugPortHandle();
    
  //
  // Send init break and wait ack in Timeout
  //
  DebugPortWriteBuffer (Handle, (UINT8 *) mErrorMsgSendInitPacket, AsciiStrLen (mErrorMsgSendInitPacket));
  Status = SendCommandAndWaitForAckOK (DEBUG_COMMAND_INIT_BREAK, Timeout, BreakReceived, &CheckSumStatus);
  if (RETURN_ERROR (Status)) {
    DebugPortWriteBuffer (Handle, (UINT8 *) mErrorMsgConnectFail, AsciiStrLen (mErrorMsgConnectFail));
    return Status;
  }
  
  if (CheckSumStatus == RETURN_NOT_FOUND) {
    //
    // If the CheckSum field does not exist in Debug Packet,
    // the HOST should be running with 0.1 transfer protocol.
    // It could be UDK Debugger for Windows v1.1 or for Linux v0.8.
    //
    DebugPortWriteBuffer (Handle, (UINT8 *) mErrorMsgVersionAlert, AsciiStrLen (mErrorMsgVersionAlert));
    CpuDeadLoop ();
  }

  DebugPortWriteBuffer (Handle, (UINT8 *) mErrorMsgConnectOK, AsciiStrLen (mErrorMsgConnectOK));
  //
  // Set Attach flag
  //
  SetHostAttached (TRUE);

  return Status;
}

/**
  Send Break point packet to HOST. 
  
  Only the first breaking processor could sent BREAK_POINT packet.
 
  @param[in]  ProcessorIndex Processor index value.
  @param[out] BreakReceived  If BreakReceived is not NULL,
                             TRUE is retured if break-in symbol received.
                             FALSE is retured if break-in symbol not received.
                            
**/
VOID
SendBreakPacketToHost (
  IN  UINT32               ProcessorIndex,
  OUT BOOLEAN              *BreakReceived
  )
{
  UINT8                 InputCharacter;
  DEBUG_PORT_HANDLE     Handle;
  
  Handle = GetDebugPortHandle();
  
  if (IsHostAttached ()) {
    DebugAgentMsgPrint (DEBUG_AGENT_INFO, "processor[%x]:Send Break Packet to HOST.\n", ProcessorIndex);
    SendCommandAndWaitForAckOK (DEBUG_COMMAND_BREAK_POINT, 0, BreakReceived, NULL);
  } else {
    DebugAgentMsgPrint (DEBUG_AGENT_INFO, "processor[%x]:Try to attach HOST.\n", ProcessorIndex);
    //
    // If HOST is not attached, try to attach it firstly.
    //
    //
    // Poll Attach symbols from HOST and ack OK
    //  
    do {
      DebugPortReadBuffer (Handle, &InputCharacter, 1, 0);
    } while (InputCharacter != DEBUG_STARTING_SYMBOL_ATTACH);
    SendAckPacket (DEBUG_COMMAND_OK);
    
    //
    // Try to attach HOST
    //
    while (AttachHost (0, NULL) != RETURN_SUCCESS);
   
  }
}

/**
  The main function to process communication with HOST.

  It received the command packet from HOST, and sent response data packet to HOST.

  @param[in]      Vector         Vector value of exception or interrutp.
  @param[in, out] CpuContext     Pointer to saved CPU context.
  @param[in]      BreakReceived  TRUE means break-in symbol received.
                                 FALSE means break-in symbol not received.

**/
VOID
CommandCommunication (
  IN     UINTN                   Vector,
  IN OUT DEBUG_CPU_CONTEXT       *CpuContext,
  IN     BOOLEAN                 BreakReceived
  )
{
  RETURN_STATUS                     Status;
  UINT8                             InputPacketBuffer[DEBUG_DATA_UPPER_LIMIT];
  DEBUG_PACKET_HEADER               *DebugHeader;
  UINT8                             Width;
  UINT8                             Data8;
  UINT32                            Data32;
  UINT64                            Data64;
  DEBUG_DATA_READ_MEMORY            *MemoryRead;
  DEBUG_DATA_WRITE_MEMORY           *MemoryWrite;
  DEBUG_DATA_READ_IO                *IoRead;
  DEBUG_DATA_WRITE_IO               *IoWrite;
  DEBUG_DATA_READ_REGISTER          *RegisterRead;
  DEBUG_DATA_WRITE_REGISTER         *RegisterWrite;
  UINT8                             *RegisterBuffer;
  DEBUG_DATA_READ_MSR               *MsrRegisterRead;
  DEBUG_DATA_WRITE_MSR              *MsrRegisterWrite;
  DEBUG_DATA_CPUID                  *Cpuid;
  DEBUG_DATA_RESPONSE_CPUID         CpuidResponse;
  DEBUG_DATA_SEARCH_SIGNATURE       *SearchSignature;
  DEBUG_DATA_RESPONSE_GET_EXCEPTION Exception;
  DEBUG_DATA_RESPONSE_GET_REVISION  DebugAgentRevision;
  DEBUG_DATA_SET_VIEWPOINT          *SetViewPoint;
  BOOLEAN                           HaltDeferred;
  UINT32                            ProcessorIndex;
  DEBUG_PORT_HANDLE                 Handle;
  DEBUG_AGENT_EXCEPTION_BUFFER      AgentExceptionBuffer;
  UINT32                            IssuedViewPoint;

  ProcessorIndex  = 0;
  IssuedViewPoint = 0;
  HaltDeferred    = BreakReceived;

  if (MultiProcessorDebugSupport) {
    ProcessorIndex = GetProcessorIndex ();
    SetCpuStopFlagByIndex (ProcessorIndex, TRUE);
    if (mDebugMpContext.ViewPointIndex == ProcessorIndex) {
      //
      // Only the current view processor could set AgentInProgress Flag. 
      //
      IssuedViewPoint = ProcessorIndex;
    }
  }

  if (IssuedViewPoint == ProcessorIndex) {
    //
    // Set AgentInProgress Flag.
    //
    GetMailboxPointer()->DebugFlag.AgentInProgress = 1;
  }  

  Handle = GetDebugPortHandle();

  while (TRUE) {

    if (MultiProcessorDebugSupport) {
      //
      // Check if the current processor is HOST view point
      //
      if (mDebugMpContext.ViewPointIndex != ProcessorIndex) {
        if (mDebugMpContext.RunCommandSet) {
          //
          // If HOST view point sets RUN flag, run GO command to leave
          //
          SetCpuStopFlagByIndex (ProcessorIndex, FALSE);
          CommandGo (CpuContext);
          break;
        } else {
          //
          // Run into loop again
          //
          CpuPause ();
          continue;
        }
      }
    }

    AcquireDebugPortControl ();

    Status = ReceivePacket (InputPacketBuffer, &BreakReceived);

    if (BreakReceived) {
      HaltDeferred = TRUE;
      BreakReceived = FALSE;
    }

    if (Status != RETURN_SUCCESS) {
      ReleaseDebugPortControl ();
      continue;
    }

    Data8 = 1;

    DebugHeader =(DEBUG_PACKET_HEADER *) InputPacketBuffer;

    GetMailboxPointer()->ExceptionBufferPointer = (UINT64)(UINTN) &AgentExceptionBuffer.JumpBuffer;
    //
    // Save CPU content before executing HOST commond
    //
    if (SetJump (&AgentExceptionBuffer.JumpBuffer) != 0) {
      //
      // If HOST command failed, continue to wait for HOST's next command
      // If needed, agent could send exception info to HOST.
      //
      SendAckPacket (DEBUG_COMMAND_ABORT);
      ReleaseDebugPortControl ();
      continue;
    }

    DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Processor[%x]:Received one command(%x)\n", mDebugMpContext.ViewPointIndex, DebugHeader->Command);
    
    switch (DebugHeader->Command) {

    case DEBUG_COMMAND_RESET:
      SendAckPacket (DEBUG_COMMAND_OK);
      ReleaseDebugPortControl ();

      ResetCold ();
      //
      // Assume system resets in 2 seconds, otherwise send TIMEOUT packet.
      // PCD can be used if 2 seconds isn't long enough for some platforms.
      //
      MicroSecondDelay (2000000);
      SendAckPacket (DEBUG_COMMAND_TIMEOUT);
      break;

    case DEBUG_COMMAND_GO:
      CommandGo (CpuContext);
      //
      // Clear Dr0 to avoid to be recognized as IMAGE_LOAD/_UNLOAD again when hitting a breakpoint after GO
      // If HOST changed Dr0 before GO, we will not change Dr0 here
      //
      Data8 = GetBreakCause (Vector, CpuContext);
      if (Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD || Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD) {
        CpuContext->Dr0 = 0;
      }
      //
      // Clear Stepping Flag
      //
      GetMailboxPointer()->DebugFlag.SteppingFlag = 0;
      
      if (!HaltDeferred) {
        //
        // If no HALT command received when being in-active mode
        //
        if (MultiProcessorDebugSupport) {
          Data32 = FindNextPendingBreakCpu ();
          if (Data32 != -1) {
            //
            // If there are still others processors being in break state,          
            // send OK packet to HOST to finish this go command
            //
            SendAckPacket (DEBUG_COMMAND_OK);
            CpuPause ();
            //
            // Set current view to the next breaking processor
            //
            mDebugMpContext.ViewPointIndex = Data32;
            mDebugMpContext.BreakAtCpuIndex = mDebugMpContext.ViewPointIndex;
            SetCpuBreakFlagByIndex (mDebugMpContext.ViewPointIndex, FALSE);
            //
            // Send break packet to HOST to let HOST break again
            //
            SendBreakPacketToHost (0, &BreakReceived);
            //
            // Continue to run into loop to read command packet from HOST
            //
            ReleaseDebugPortControl (); 
            break;
          }

          //
          // If no else processor break, set stop bitmask,
          // and set Running flag for all processors.
          //
          SetCpuStopFlagByIndex (ProcessorIndex, FALSE);
          SetCpuRunningFlag (TRUE);
          CpuPause ();
          //
          // Wait for all processors are in running state 
          //
          while (TRUE) {
            if (IsAllCpuRunning ()) {
              break;
            }
          }
          //
          // Set BSP to be current view point.
          //
          SetDebugViewPoint (mDebugMpContext.BspIndex);
          CpuPause ();
          //
          // Clear breaking processor index and running flag
          //
          mDebugMpContext.BreakAtCpuIndex = (UINT32) (-1);
          SetCpuRunningFlag (FALSE);
        }

        //
        // Send OK packet to HOST to finish this go command
        //
        SendAckPacket (DEBUG_COMMAND_OK);

        ReleaseDebugPortControl ();

        return;

      } else {
        //
        // If reveived HALT command, need to defer the GO command
        //
        SendAckPacket (DEBUG_COMMAND_HALT_PROCESSED);
        HaltDeferred = FALSE;

        Vector = DEBUG_TIMER_VECTOR;
      }
      break;

    case DEBUG_COMMAND_BREAK_CAUSE:

      if (MultiProcessorDebugSupport && ProcessorIndex != mDebugMpContext.BreakAtCpuIndex) {
        Status = SendBreakCausePacket (DEBUG_TIMER_VECTOR, CpuContext);

      } else {
        Status = SendBreakCausePacket (Vector, CpuContext);
      }

      break;

    case DEBUG_COMMAND_SET_HW_BREAKPOINT:
      SetDebugRegister (CpuContext, (DEBUG_DATA_SET_HW_BREAKPOINT *) (DebugHeader + 1));
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_CLEAR_HW_BREAKPOINT:
      ClearDebugRegister (CpuContext, (DEBUG_DATA_CLEAR_HW_BREAKPOINT *) (DebugHeader + 1));
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_SINGLE_STEPPING:
      CommandStepping (CpuContext);
      //
      // Clear Dr0 to avoid to be recognized as IMAGE_LOAD/_UNLOAD again when hitting a breakpoint after GO
      // If HOST changed Dr0 before GO, we will not change Dr0 here
      //
      Data8 = GetBreakCause (Vector, CpuContext);
      if (Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD || Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD) {
        CpuContext->Dr0 = 0;
      }

      mDebugMpContext.BreakAtCpuIndex = (UINT32) (-1);
      //
      // Set Stepping Flag
      //
      GetMailboxPointer()->DebugFlag.SteppingFlag = 1;
      ReleaseDebugPortControl ();
      //
      // Executing stepping command directly without sending ACK packet,
      // ACK packet will be sent after stepping done.
      //
      return;

    case DEBUG_COMMAND_SET_SW_BREAKPOINT:
      Data64 = (UINTN) (((DEBUG_DATA_SET_SW_BREAKPOINT *) (DebugHeader + 1))->Address);
      Data8 = *(UINT8 *) (UINTN) Data64;
      *(UINT8 *) (UINTN) Data64 = DEBUG_SW_BREAKPOINT_SYMBOL;
      Status = SendDataResponsePacket ((UINT8 *) &Data8, (UINT16) sizeof (UINT8));
      break;

    case DEBUG_COMMAND_READ_MEMORY:
      MemoryRead = (DEBUG_DATA_READ_MEMORY *) (DebugHeader + 1);
      Status = SendDataResponsePacket ((UINT8 *) (UINTN) MemoryRead->Address, (UINT16) (MemoryRead->Count * MemoryRead->Width));
      break;

    case DEBUG_COMMAND_WRITE_MEMORY:
      MemoryWrite = (DEBUG_DATA_WRITE_MEMORY *) (DebugHeader + 1);
      CopyMem ((VOID *) (UINTN) MemoryWrite->Address, &MemoryWrite->Data, MemoryWrite->Count * MemoryWrite->Width);
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_READ_IO:
      IoRead = (DEBUG_DATA_READ_IO *) (DebugHeader + 1);
      switch (IoRead->Width) {
      case 1:
        Data64  = IoRead8 ((UINTN) IoRead->Port);
        break;
      case 2:
        Data64  = IoRead16 ((UINTN) IoRead->Port);
        break;
      case 4:
        Data64  = IoRead32 ((UINTN) IoRead->Port);
        break;
      case 8:
        Data64  = IoRead64 ((UINTN) IoRead->Port);
        break;
      default:
        Data64  = (UINT64) -1;
      }
      Status = SendDataResponsePacket ((UINT8 *) &Data64, IoRead->Width);
      break;

    case DEBUG_COMMAND_WRITE_IO:
      IoWrite = (DEBUG_DATA_WRITE_IO *) (DebugHeader + 1);
      switch (IoWrite->Width) {
      case 1:
        Data64  = IoWrite8 ((UINTN) IoWrite->Port, *(UINT8 *) &IoWrite->Data);
        break;
      case 2:
        Data64  = IoWrite16 ((UINTN) IoWrite->Port, *(UINT16 *) &IoWrite->Data);
        break;
      case 4:
        Data64  = IoWrite32 ((UINTN) IoWrite->Port, *(UINT32 *) &IoWrite->Data);
        break;
      case 8:
        Data64  = IoWrite64 ((UINTN) IoWrite->Port, *(UINT64 *) &IoWrite->Data);
        break;
      default:
        Data64  = (UINT64) -1;
      }
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_READ_ALL_REGISTERS:
      Status = SendDataResponsePacket ((UINT8 *) CpuContext, sizeof (*CpuContext));
      break;

    case DEBUG_COMMAND_READ_REGISTER:
      RegisterRead = (DEBUG_DATA_READ_REGISTER *) (DebugHeader + 1);

      if (RegisterRead->Index <= SOFT_DEBUGGER_REGISTER_MAX) {
        RegisterBuffer = ArchReadRegisterBuffer (CpuContext, RegisterRead->Index, &Width);
        Status = SendDataResponsePacket (RegisterBuffer, Width);
      } else {
        Status = RETURN_UNSUPPORTED;
      }
      break;

    case DEBUG_COMMAND_WRITE_REGISTER:
      RegisterWrite = (DEBUG_DATA_WRITE_REGISTER *) (DebugHeader + 1);
      if (RegisterWrite->Index <= SOFT_DEBUGGER_REGISTER_MAX) {
        RegisterBuffer = ArchReadRegisterBuffer (CpuContext, RegisterWrite->Index, &Width);
        ASSERT (Width == RegisterWrite->Length);
        CopyMem (RegisterBuffer, RegisterWrite->Data, Width);
        SendAckPacket (DEBUG_COMMAND_OK);
      } else {
        Status = RETURN_UNSUPPORTED;
      }
      break;

    case DEBUG_COMMAND_ARCH_MODE:
      Data8 = DEBUG_ARCH_SYMBOL;
      Status = SendDataResponsePacket ((UINT8 *) &Data8, (UINT16) sizeof (UINT8));
      break;

    case DEBUG_COMMAND_READ_MSR:
      MsrRegisterRead = (DEBUG_DATA_READ_MSR *) (DebugHeader + 1);
      Data64 = AsmReadMsr64 (MsrRegisterRead->Index);
      Status = SendDataResponsePacket ((UINT8 *) &Data64, (UINT16) sizeof (UINT64));
      break;

    case DEBUG_COMMAND_WRITE_MSR:
      MsrRegisterWrite = (DEBUG_DATA_WRITE_MSR *) (DebugHeader + 1);
      AsmWriteMsr64 (MsrRegisterWrite->Index, MsrRegisterWrite->Value);
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_SET_DEBUG_SETTING:
      Status = SetDebugSetting ((DEBUG_DATA_SET_DEBUG_SETTING *)(DebugHeader + 1));
      if (Status == RETURN_SUCCESS) {
        SendAckPacket (DEBUG_COMMAND_OK);
      }
      break;

    case DEBUG_COMMAND_GET_REVISION:
      DebugAgentRevision.Revision = DEBUG_AGENT_REVISION;
      DebugAgentRevision.Capabilities = DEBUG_AGENT_CAPABILITIES;
      Status = SendDataResponsePacket ((UINT8 *) &DebugAgentRevision, (UINT16) sizeof (DEBUG_DATA_RESPONSE_GET_REVISION));
      break;

    case DEBUG_COMMAND_GET_EXCEPTION:
      Exception.ExceptionNum  = (UINT8) Vector;
      Exception.ExceptionData = (UINT32) CpuContext->ExceptionData;
      Status = SendDataResponsePacket ((UINT8 *) &Exception, (UINT16) sizeof (DEBUG_DATA_RESPONSE_GET_EXCEPTION));
      break;

    case DEBUG_COMMAND_SET_VIEWPOINT:
      SetViewPoint = (DEBUG_DATA_SET_VIEWPOINT *) (DebugHeader + 1);
      if (MultiProcessorDebugSupport) {
        if (IsCpuStopped (SetViewPoint->ViewPoint)) {
          SetDebugViewPoint (SetViewPoint->ViewPoint);
          SendAckPacket (DEBUG_COMMAND_OK);
        } else {
          //
          // If CPU is not halted
          //
          SendAckPacket (DEBUG_COMMAND_NOT_SUPPORTED);
        }
      } else if (SetViewPoint->ViewPoint == 0) {
        SendAckPacket (DEBUG_COMMAND_OK);

      } else {
        SendAckPacket (DEBUG_COMMAND_NOT_SUPPORTED);
      }

      break;

    case DEBUG_COMMAND_GET_VIEWPOINT:
      Data32 = mDebugMpContext.ViewPointIndex;
      SendDataResponsePacket((UINT8 *) &Data32, (UINT16) sizeof (UINT32));
      break;

    case DEBUG_COMMAND_MEMORY_READY:
      Data8 = (UINT8) GetMailboxPointer ()->DebugFlag.MemoryReady;
      SendDataResponsePacket (&Data8, (UINT16) sizeof (UINT8));
      break;

    case DEBUG_COMMAND_DETACH:
      SetHostAttached (FALSE);
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_CPUID:
      Cpuid = (DEBUG_DATA_CPUID *) (DebugHeader + 1);
      AsmCpuidEx (
        Cpuid->Eax, Cpuid->Ecx, 
        &CpuidResponse.Eax, &CpuidResponse.Ebx,
        &CpuidResponse.Ecx, &CpuidResponse.Edx
        );
      SendDataResponsePacket ((UINT8 *) &CpuidResponse, (UINT16) sizeof (CpuidResponse));
      break;

   case DEBUG_COMMAND_SEARCH_SIGNATURE:
      SearchSignature = (DEBUG_DATA_SEARCH_SIGNATURE *) (DebugHeader + 1);
      if ((SearchSignature->Alignment != 0) && 
          (SearchSignature->Alignment == GetPowerOfTwo32 (SearchSignature->Alignment))
         ) {
        if (SearchSignature->Positive) {
          for (
            Data64 = ALIGN_VALUE ((UINTN) SearchSignature->Start, SearchSignature->Alignment);
            Data64 <= SearchSignature->Start + SearchSignature->Count - SearchSignature->DataLength;
            Data64 += SearchSignature->Alignment
              ) {
            if (CompareMem ((VOID *) (UINTN) Data64, &SearchSignature->Data, SearchSignature->DataLength) == 0) {
              break;
            }
          }
          if (Data64 > SearchSignature->Start + SearchSignature->Count - SearchSignature->DataLength) {
            Data64 = (UINT64) -1;
          }
        } else {
          for (
            Data64 = ALIGN_VALUE ((UINTN) SearchSignature->Start - SearchSignature->Alignment, SearchSignature->Alignment);
            Data64 >= SearchSignature->Start - SearchSignature->Count;
            Data64 -= SearchSignature->Alignment
              ) {
            if (CompareMem ((VOID *) (UINTN) Data64, &SearchSignature->Data, SearchSignature->DataLength) == 0) {
              break;
            }
          }
          if (Data64 < SearchSignature->Start - SearchSignature->Count) {
            Data64 = (UINT64) -1;
          }
        }
        SendDataResponsePacket ((UINT8 *) &Data64, (UINT16) sizeof (Data64));
      } else {
        Status = RETURN_UNSUPPORTED;
      }
      break;

    default:
      SendAckPacket (DEBUG_COMMAND_NOT_SUPPORTED);
      break;
    }

    if (Status == RETURN_UNSUPPORTED) {
      SendAckPacket (DEBUG_COMMAND_NOT_SUPPORTED);
    } else if (Status != RETURN_SUCCESS) {
      SendAckPacket (DEBUG_COMMAND_ABORT);
    }

    ReleaseDebugPortControl ();
    CpuPause ();
  }
}

/**
  C function called in interrupt handler.

  @param[in] Vector      Vector value of exception or interrutp.
  @param[in] CpuContext  Pointer to save CPU context.

**/
VOID
EFIAPI
InterruptProcess (
  IN UINT32                          Vector,
  IN DEBUG_CPU_CONTEXT               *CpuContext
  )
{
  UINT8                            InputCharacter;
  UINT8                            BreakCause;
  UINTN                            SavedEip;
  BOOLEAN                          BreakReceived;
  UINT32                           ProcessorIndex;
  UINT32                           CurrentDebugTimerInitCount;
  DEBUG_PORT_HANDLE                Handle;
  UINT8                            Data8;
  UINT8                            *Al;
  UINT32                           IssuedViewPoint;
  DEBUG_AGENT_EXCEPTION_BUFFER     *ExceptionBuffer;

  ProcessorIndex  = 0;
  IssuedViewPoint = 0;
  BreakReceived   = FALSE;

  if (MultiProcessorDebugSupport) {
    ProcessorIndex = GetProcessorIndex ();
    //
    // If this processor has alreay halted before, need to check it later
    //
    if (IsCpuStopped (ProcessorIndex)) {
      IssuedViewPoint = ProcessorIndex;
    }
  }

  if (IssuedViewPoint == ProcessorIndex) {
    //
    // Check if this exception is issued by Debug Agent itself
    // If yes, fill the debug agent exception buffer and LongJump() back to
    // the saved CPU content in CommandCommunication()
    //
    if (GetMailboxPointer()->DebugFlag.AgentInProgress == 1) {
      DebugAgentMsgPrint (DEBUG_AGENT_ERROR, "Debug agent meet one Exception, ExceptionNum is %d.\n", Vector);
      ExceptionBuffer = (DEBUG_AGENT_EXCEPTION_BUFFER *) (UINTN) GetMailboxPointer()->ExceptionBufferPointer;
      ExceptionBuffer->ExceptionContent.ExceptionNum  = (UINT8) Vector;
      ExceptionBuffer->ExceptionContent.ExceptionData = (UINT32) CpuContext->ExceptionData;
      LongJump ((BASE_LIBRARY_JUMP_BUFFER *)(UINTN)(GetMailboxPointer()->ExceptionBufferPointer), 1);
    }
  }

  if (MultiProcessorDebugSupport) {
    //
    // If RUN commmand is executing, wait for it done.  
    //
    while (mDebugMpContext.RunCommandSet) {
      CpuPause ();
    }
  }

  Handle = GetDebugPortHandle();

  switch (Vector) {
  case DEBUG_INT1_VECTOR:
  case DEBUG_INT3_VECTOR:
    BreakCause = GetBreakCause (Vector, CpuContext);

    switch (BreakCause) {
    case DEBUG_DATA_BREAK_CAUSE_SYSTEM_RESET:
      if (AttachHost (INIT_BREAK_ACK_TIMEOUT, &BreakReceived) != RETURN_SUCCESS) {
        //
        // Try to connect HOST, return if fails
        //
        break;
      }
      CommandCommunication (Vector, CpuContext, BreakReceived);
      break;

    case DEBUG_DATA_BREAK_CAUSE_STEPPING:
      //
      // Stepping is finished, send Ack package.
      //
      if (MultiProcessorDebugSupport) {
        mDebugMpContext.BreakAtCpuIndex = ProcessorIndex;
      }
      SendAckPacket (DEBUG_COMMAND_OK);
      CommandCommunication (Vector, CpuContext, BreakReceived);
      break;

    case DEBUG_DATA_BREAK_CAUSE_MEMORY_READY:
      //
      // Memory is ready
      //
      SendCommandAndWaitForAckOK (DEBUG_COMMAND_MEMORY_READY, 0, &BreakReceived, NULL);
      CommandCommunication (Vector, CpuContext, BreakReceived);
      break;

    case DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD:
    case DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD:
      //
      // Set AL to DEBUG_AGENT_IMAGE_CONTINUE
      //
      Al = ArchReadRegisterBuffer (CpuContext, SOFT_DEBUGGER_REGISTER_AX, &Data8);
      *Al = DEBUG_AGENT_IMAGE_CONTINUE;

      if (!IsHostAttached ()) {
        //
        // If HOST is not connected for image load/unload, return
        //
        break;
      }
      //
      // Continue to run the following common code
      //
      
    case DEBUG_DATA_BREAK_CAUSE_HW_BREAKPOINT:
    case DEBUG_DATA_BREAK_CAUSE_SW_BREAKPOINT:
    default:
      //
      // Send Break packet to HOST
      //
      AcquireDebugPortControl ();
      //
      // Only the first breaking processor could send BREAK_POINT to HOST
      // 
      if (IsFirstBreakProcessor (ProcessorIndex)) {
        SendBreakPacketToHost (ProcessorIndex, &BreakReceived);
      }
      ReleaseDebugPortControl ();
      
      if (Vector == DEBUG_INT3_VECTOR) {
        //
        // go back address located "0xCC"
        //
        CpuContext->Eip--;
        SavedEip = CpuContext->Eip;
        CommandCommunication (Vector, CpuContext, BreakReceived);
        if ((SavedEip == CpuContext->Eip) &&
            (*(UINT8 *) (UINTN) CpuContext->Eip == DEBUG_SW_BREAKPOINT_SYMBOL)) {
          //
          // If this is not a software breakpoint set by HOST,
          // restore EIP
          //
          CpuContext->Eip++;
        }
      } else {
        CommandCommunication (Vector, CpuContext, BreakReceived);
      }
      break;
    }

    break;

  case DEBUG_TIMER_VECTOR:

    AcquireDebugPortControl ();

    if (MultiProcessorDebugSupport) {
      if (IsBsp (ProcessorIndex)) {
        //
        // If current processor is BSP, check Apic timer's init count if changed,
        // it may be re-written when switching BSP.
        // If it changed, re-initialize debug timer
        //
        CurrentDebugTimerInitCount = GetApicTimerInitCount ();
        if (mDebugMpContext.DebugTimerInitCount != CurrentDebugTimerInitCount) {
          InitializeDebugTimer ();
        }
      }

      if (!IsBsp (ProcessorIndex) || mDebugMpContext.IpiSentByAp) {
        ReleaseDebugPortControl ();
        //
        // If current processor is not BSP or this is one IPI sent by AP
        //
        if (mDebugMpContext.BreakAtCpuIndex != (UINT32) (-1)) {
          CommandCommunication (Vector, CpuContext, FALSE);
        }

        //
        // Clear EOI before exiting interrupt process routine.
        //
        SendApicEoi ();
        break;
      }
    }

    //
    // Only BSP could run here
    //
    while (TRUE) {
      //
      // If there is data in debug port, will check whether it is break(attach/break-in) symbol,
      // If yes, go into communication mode with HOST.
      // If no, exit interrupt process.
      //
      if (DebugReadBreakSymbol (Handle, &InputCharacter) == EFI_NOT_FOUND) {
        break;
      }
      if ((!IsHostAttached () && (InputCharacter == DEBUG_STARTING_SYMBOL_ATTACH)) ||
          (IsHostAttached () && (InputCharacter == DEBUG_STARTING_SYMBOL_BREAK))
         ) {
        DebugAgentMsgPrint (DEBUG_AGENT_VERBOSE, "Received data [%02x]\n", InputCharacter);
        //
        // Ack OK for break-in symbol
        //
        SendAckPacket (DEBUG_COMMAND_OK);

        if (!IsHostAttached ()) {
          //
          // Try to attach HOST, if no ack received after 200ms, return
          //
          if (AttachHost (INIT_BREAK_ACK_TIMEOUT, &BreakReceived) != RETURN_SUCCESS) {
            break;
          }
        }

        if (MultiProcessorDebugSupport) {
          if(FindNextPendingBreakCpu  () != -1) {
            SetCpuBreakFlagByIndex (ProcessorIndex, TRUE);
          } else {
            HaltOtherProcessors (ProcessorIndex);
          }
        }
        ReleaseDebugPortControl ();
        CommandCommunication (Vector, CpuContext, BreakReceived);
        AcquireDebugPortControl ();
        break;
      }
    }

    //
    // Clear EOI before exiting interrupt process routine.
    //
    SendApicEoi ();

    ReleaseDebugPortControl ();

    break;

  default:

    if (Vector <= DEBUG_EXCEPT_SIMD) {
      BreakCause = GetBreakCause (Vector, CpuContext);
      if (BreakCause == DEBUG_DATA_BREAK_CAUSE_STEPPING) {
        //
        // Stepping is finished, send Ack package.
        //
        if (MultiProcessorDebugSupport) {
          mDebugMpContext.BreakAtCpuIndex = ProcessorIndex;
        }
        SendAckPacket (DEBUG_COMMAND_OK);
      } else {
        //
        // Exception occurs, send Break packet to HOST
        //
        AcquireDebugPortControl ();
        //
        // Only the first breaking processor could send BREAK_POINT to HOST
        // 
        if (IsFirstBreakProcessor (ProcessorIndex)) {
          SendBreakPacketToHost (ProcessorIndex, &BreakReceived);
        }
        ReleaseDebugPortControl ();
      }
      
      CommandCommunication (Vector, CpuContext, BreakReceived);
    }
    break;
  }

  if (MultiProcessorDebugSupport) {
    //
    // Clear flag and wait for all processors run here
    //
    SetIpiSentByApFlag (FALSE);
    while (mDebugMpContext.RunCommandSet) {
      CpuPause ();
    }

    //
    // Only current (view) processor could clean up AgentInProgress flag.
    //
    if (mDebugMpContext.ViewPointIndex == ProcessorIndex) {
      IssuedViewPoint = mDebugMpContext.ViewPointIndex;
    }
  }

  if (IssuedViewPoint == ProcessorIndex) {
    //
    // Clean up AgentInProgress flag
    //
    GetMailboxPointer()->DebugFlag.AgentInProgress = 0;
  }

  return;
}

