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

/**
  Check if HOST is connected based on Mailbox.

  @retval TRUE        HOST is connected.
  @retval FALSE       HOST is not connected.

**/
BOOLEAN
IsHostConnected (
  VOID
  )
{
  DEBUG_AGENT_MAILBOX          *Mailbox;

  Mailbox = GetMailboxPointer ();

  if (Mailbox->DebugFlag.Bits.HostPresent == 1) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Set HOST connect flag in Mailbox.

**/
VOID
SetHostConnectedFlag (
  VOID
  )
{
  DEBUG_AGENT_MAILBOX          *Mailbox;

  Mailbox = GetMailboxPointer ();

  Mailbox->DebugFlag.Bits.HostPresent = 1;
}

/**
  Set debug flag of Debug Agent in Mailbox.

  @param DebugFlag       Debug Flag defined by transfer protocol.

**/
VOID
SetDebugFlag (
  IN UINT32               DebugFlag
  )
{
  DEBUG_AGENT_MAILBOX          *Mailbox;

  Mailbox = GetMailboxPointer ();

  if ((DebugFlag & SOFT_DEBUGGER_SETTING_SMM_ENTRY_BREAK) != 0) {
    Mailbox->DebugFlag.Bits.BreakOnNextSmi = 1;
  } else {
    Mailbox->DebugFlag.Bits.BreakOnNextSmi = 0;
  }
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
  Dr7Value &= ~(0xf0000 << (RegisterIndex * 4));
  Dr7Value |= (SetHwBreakpoint->Type.Length | SetHwBreakpoint->Type.Access) << (RegisterIndex * 4);
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
  Send acknowledge packet to HOST.

  @param[in] AckCommand    Type of Acknowledge packet.

**/
VOID
SendAckPacket (
  IN UINT8                AckCommand
  )
{
  DEBUG_COMMAND_HEADER      DebugCommonHeader;
  DEBUG_PORT_HANDLE         Handle;

  Handle = GetDebugPortHandle();

  DebugCommonHeader.StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;
  DebugCommonHeader.Command     = AckCommand;
  DebugCommonHeader.DataLength  = 0;

  DebugPortWriteBuffer (Handle, (UINT8 *) &DebugCommonHeader, sizeof (DEBUG_COMMAND_HEADER));
}

/**
  Receive acknowledge packet from HOST in specified time.

  @param[out] Ack           Returned acknowlege type from HOST.
  @param[in]  Timeout       Time out value to wait for acknowlege from HOST.
                            The unit is microsecond.
  @param[out] BreakReceived If BreakReceived is not NULL,
                            TRUE is retured if break-in symbol received.
                            FALSE is retured if break-in symbol not received.

  @retval  RETRUEN_SUCCESS  Succeed to receive acknowlege packet from HOST,
                            the type of acknowlege packet saved in Ack.
  @retval  RETURN_TIMEOUT   Specified timeout value was up.

**/
RETURN_STATUS
ReceiveAckPacket (
  OUT UINT8                     *Ack,
  IN  UINTN                     Timeout,
  OUT BOOLEAN                   *BreakReceived OPTIONAL
  )
{
  DEBUG_COMMAND_HEADER      DebugCommonHeader;
  DEBUG_PORT_HANDLE         Handle;

  Handle = GetDebugPortHandle();

  while (TRUE) {
    if (DebugPortReadBuffer (Handle, (UINT8 *) &DebugCommonHeader.StartSymbol, 1, Timeout) == 0) {
      return RETURN_TIMEOUT;
    }
    if (DebugCommonHeader.StartSymbol == DEBUG_STARTING_SYMBOL_BREAK) {
      if (BreakReceived != NULL) {
        SendAckPacket (DEBUG_COMMAND_HALT_DEFERRED);
        *BreakReceived = TRUE;
      }
    }
    if (DebugCommonHeader.StartSymbol == DEBUG_STARTING_SYMBOL_NORMAL) {
      break;
    }
  }
  if (DebugPortReadBuffer (Handle, (UINT8 *)&DebugCommonHeader.Command, sizeof (DEBUG_COMMAND_HEADER) - 1, Timeout) == 0) {
    return RETURN_TIMEOUT;
  }

  *Ack = DebugCommonHeader.Command;
  return RETURN_SUCCESS;
}

/**
  Receive acknowledge packet OK from HOST in specified time.

  @param[in]  Timeout       Time out value to wait for acknowlege from HOST.
                            The unit is microsecond.
  @param[out] BreakReceived If BreakReceived is not NULL,
                            TRUE is retured if break-in symbol received.
                            FALSE is retured if break-in symbol not received.

  @retval  RETRUEN_SUCCESS  Succeed to receive acknowlege packet from HOST,
                            the type of acknowlege packet saved in Ack.
  @retval  RETURN_TIMEOUT   Specified timeout value was up.

**/
RETURN_STATUS
WaitForAckPacketOK (
  IN  UINTN                     Timeout,
  OUT BOOLEAN                   *BreakReceived OPTIONAL
  )
{
  RETURN_STATUS             Status;
  UINT8                     Ack;

  while (TRUE) {
    Status = ReceiveAckPacket (&Ack, Timeout, BreakReceived);
    if ((Status == RETURN_SUCCESS && Ack == DEBUG_COMMAND_OK) ||
         Status == RETURN_TIMEOUT) {
      break;
    }
  }

  return Status;
}

/**
  Receive valid packet from HOST.

  @param[out] InputPacket    Buffer to receive packet.
  @param[out] BreakReceived  TRUE means break-in symbol received.
                             FALSE means break-in symbol not received.

  @retval RETURN_SUCCESS   A valid package was reveived in InputPacket.
  @retval RETURN_NOT_READY No valid start symbol received.
  @retval RETURN_TIMEOUT   Timeout occurs.

**/
RETURN_STATUS
ReceivePacket (
  OUT UINT8             *InputPacket,
  OUT BOOLEAN           *BreakReceived
  )
{
  DEBUG_COMMAND_HEADER  *DebugHeader;
  UINTN                 Received;
  DEBUG_PORT_HANDLE     Handle;

  Handle = GetDebugPortHandle();
  //
  // Find the valid start symbol
  //
  DebugPortReadBuffer (Handle, InputPacket, 1, 0);

  if (*InputPacket == DEBUG_STARTING_SYMBOL_BREAK) {
    *BreakReceived = TRUE;
    SendAckPacket (DEBUG_COMMAND_HALT_DEFERRED);
  }

  if (*InputPacket != DEBUG_STARTING_SYMBOL_NORMAL) {
    return RETURN_NOT_READY;
  }

  //
  // Read Package header
  //
  Received = DebugPortReadBuffer (Handle, InputPacket + 1, sizeof(DEBUG_COMMAND_HEADER_NO_START_SYMBOL), 0);
  if (Received == 0) {
    return RETURN_TIMEOUT;
  }

  DebugHeader = (DEBUG_COMMAND_HEADER *) InputPacket;
  //
  // Read the payload if has
  //
  if (DebugHeader->DataLength > 0 && DebugHeader->DataLength < (DEBUG_DATA_MAXIMUM_REAL_DATA - sizeof(DEBUG_COMMAND_HEADER))) {
    InputPacket = InputPacket + 1 + Received;
    Received = DebugPortReadBuffer (Handle, InputPacket, DebugHeader->DataLength, 0);

    if (Received == 0) {
      return RETURN_TIMEOUT;
    }
  }

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
      Cause = DEBUG_DATA_BREAK_CAUSE_EXCEPTION;
    }
    break;
  }

  return Cause;
}

/**
  Send packet with response data to HOST.

  @param[in] CpuContext  Pointer to saved CPU context.
  @param[in] Data        Pointer to response data buffer.
  @param[in] DataSize    Size of response data in byte.

  @retval RETURN_SUCCESS      Response data was sent successfully.
  @retval RETURN_DEVICE_ERROR Cannot receive DEBUG_COMMAND_OK from HOST.

**/
RETURN_STATUS
SendDataResponsePacket (
  IN DEBUG_CPU_CONTEXT    *CpuContext,
  IN UINT8                *Data,
  IN UINT16               DataSize
  )
{
  UINT8                PacketHeader[DEBUG_DATA_MAXIMUM_LENGTH_FOR_SMALL_COMMANDS];
  BOOLEAN              LastPacket;
  UINT8                Ack;
  UINT8                PacketData[DEBUG_DATA_MAXIMUM_REAL_DATA];
  DEBUG_PORT_HANDLE    Handle;

  Handle = GetDebugPortHandle();

  ((DEBUG_COMMAND_HEADER *)PacketHeader)->StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;

  while (TRUE) {
    if (DataSize <= DEBUG_DATA_MAXIMUM_REAL_DATA) {
      LastPacket = TRUE;
      ((DEBUG_COMMAND_HEADER *)PacketHeader)->Command     = DEBUG_COMMAND_OK;
      ((DEBUG_COMMAND_HEADER *)PacketHeader)->DataLength  = (UINT8) DataSize;
      CopyMem (PacketData, Data, DataSize);

    } else {
      LastPacket = FALSE;
      ((DEBUG_COMMAND_HEADER *)PacketHeader)->Command     = DEBUG_COMMAND_IN_PROGRESS;
      ((DEBUG_COMMAND_HEADER *)PacketHeader)->DataLength  = DEBUG_DATA_MAXIMUM_REAL_DATA;
      CopyMem (PacketData, Data, DEBUG_DATA_MAXIMUM_REAL_DATA);
    }

    DebugPortWriteBuffer (Handle, PacketHeader, sizeof (DEBUG_COMMAND_HEADER));
    DebugPortWriteBuffer (Handle, PacketData, ((DEBUG_COMMAND_HEADER *)PacketHeader)->DataLength);

    ReceiveAckPacket(&Ack, 0, NULL);
    switch (Ack) {
    case DEBUG_COMMAND_RESEND:
      //
      // Send the packet again
      //
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

  return SendDataResponsePacket (CpuContext, (UINT8 *) &DebugDataBreakCause, (UINT16) sizeof (DEBUG_DATA_RESPONSE_BREAK_CAUSE));
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
  RETURN_STATUS                 Status;
  UINT8                         InputPacketBuffer[DEBUG_DATA_MAXIMUM_LENGTH_FOR_SMALL_COMMANDS];
  DEBUG_COMMAND_HEADER          *DebugHeader;
  UINT8                         Data8;
  UINT32                        Data32;
  UINT64                        Data64;
  UINTN                         DataN;
  DEBUG_DATA_READ_MEMORY_8      *MemoryRead;
  DEBUG_DATA_WRITE_MEMORY_8     *MemoryWrite;
  DEBUG_DATA_READ_IO            *IoRead;
  DEBUG_DATA_WRITE_IO           *IoWrite;
  DEBUG_DATA_READ_REGISTER      *RegisterRead;
  DEBUG_DATA_WRITE_REGISTER     *RegisterWrite;
  UINT8                         *RegisterBuffer;
  DEBUG_DATA_READ_MSR           *MsrRegisterRead;
  DEBUG_DATA_WRITE_MSR          *MsrRegisterWrite;
  DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM   RegisterGroupSegLim;
  DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE  RegisterGroupSegBase;
  DEBUG_DATA_RESPONSE_GET_REVISION DebugAgentRevision;
  BOOLEAN                       HaltDeferred;
  DEBUG_DATA_RESPONSE_GET_EXCEPTION  Exception;
  UINT32                        ProcessorIndex;
  DEBUG_PORT_HANDLE             Handle;

  Handle = GetDebugPortHandle();

  ProcessorIndex  = 0;
  HaltDeferred = BreakReceived;

  if (MultiProcessorDebugSupport) {
    ProcessorIndex = GetProcessorIndex ();
    SetCpuStopFlagByIndex (ProcessorIndex, TRUE);
  }

  while (TRUE) {

    if (MultiProcessorDebugSupport) {
      if (mDebugMpContext.ViewPointIndex != ProcessorIndex) {
        if (mDebugMpContext.RunCommandSet) {
          SetCpuStopFlagByIndex (ProcessorIndex, FALSE);
          CommandGo (CpuContext);
          break;
        } else {
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

    DebugHeader =(DEBUG_COMMAND_HEADER *) InputPacketBuffer;
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
      if (!HaltDeferred) {
        //
        // If no HALT command received when being in-active mode
        //
        if (MultiProcessorDebugSupport) {
          Data32 = FindCpuNotRunning ();
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
            // Send break packet to HOST and exit to wait for command packet from HOST.
            //
            SendAckPacket (DEBUG_COMMAND_BREAK_POINT);
            WaitForAckPacketOK (0, &BreakReceived);
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
        Data8 = GetBreakCause (Vector, CpuContext);
        if (Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD || Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD) {
          CpuContext->Dr0 = 0;
          CpuContext->Dr3 = 0;
        }

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

      mDebugMpContext.BreakAtCpuIndex = (UINT32) (-1);

      ReleaseDebugPortControl ();
      //
      // Executing stepping command directly without sending ACK packet.
      //
      return;

    case DEBUG_COMMAND_SET_SW_BREAKPOINT:
      Data64 = (UINTN) (((DEBUG_DATA_SET_SW_BREAKPOINT *) (DebugHeader + 1))->Address);
      Data8 = *(UINT8 *) (UINTN) Data64;
      *(UINT8 *) (UINTN) Data64 = DEBUG_SW_BREAKPOINT_SYMBOL;
      Status = SendDataResponsePacket (CpuContext, (UINT8 *) &Data8, (UINT16) sizeof (UINT8));
      break;

    case DEBUG_COMMAND_READ_MEMORY_64:
      Data8 *= 2;
    case DEBUG_COMMAND_READ_MEMORY_32:
      Data8 *= 2;
    case DEBUG_COMMAND_READ_MEMORY_16:
      Data8 *= 2;
	  case DEBUG_COMMAND_READ_MEMORY_8:
      MemoryRead = (DEBUG_DATA_READ_MEMORY_8 *) (DebugHeader + 1);
      Status = SendDataResponsePacket (CpuContext, (UINT8 *) (UINTN) MemoryRead->Address, (UINT16) (MemoryRead->Count * Data8));
      break;

    case DEBUG_COMMAND_WRITE_MEMORY_64:
      Data8 *= 2;
    case DEBUG_COMMAND_WRITE_MEMORY_32:
      Data8 *= 2;
    case DEBUG_COMMAND_WRITE_MEMORY_16:
      Data8 *= 2;
    case DEBUG_COMMAND_WRITE_MEMORY_8:
      MemoryWrite = (DEBUG_DATA_WRITE_MEMORY_8 *) (DebugHeader + 1);
      CopyMem ((VOID *) (UINTN) MemoryWrite->Address, &MemoryWrite->Data, MemoryWrite->Count * Data8);
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_READ_IO:
      IoRead = (DEBUG_DATA_READ_IO *) (DebugHeader + 1);
      switch (IoRead->Width) {
      case 1:
        Data64  = IoRead8 (IoRead->Port);
        break;
      case 2:
        Data64  = IoRead16 (IoRead->Port);
        break;
      case 4:
        Data64  = IoRead32 (IoRead->Port);
        break;
      case 8:
        Data64  = IoRead64 (IoRead->Port);
        break;
      default:
        Data64  = (UINT64) -1;
      }
      Status = SendDataResponsePacket (CpuContext, (UINT8 *) &Data64, IoRead->Width);
      break;

    case DEBUG_COMMAND_WRITE_IO:
      IoWrite = (DEBUG_DATA_WRITE_IO *) (DebugHeader + 1);
      switch (IoWrite->Width) {
      case 1:
        Data64  = IoWrite8 (IoWrite->Port, *(UINT8 *) &IoWrite->Data);
        break;
      case 2:
        Data64  = IoWrite16 (IoWrite->Port, *(UINT16 *) &IoWrite->Data);
        break;
      case 4:
        Data64  = IoWrite32 (IoWrite->Port, *(UINT32 *) &IoWrite->Data);
        break;
      case 8:
        Data64  = IoWrite64 (IoWrite->Port, *(UINT64 *) &IoWrite->Data);
        break;
      default:
        Data64  = (UINT64) -1;
      }
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_READ_REGISTER:
      RegisterRead = (DEBUG_DATA_READ_REGISTER *) (DebugHeader + 1);

      if (RegisterRead->Index < SOFT_DEBUGGER_REGISTER_OTHERS_BASE) {
        Data8 = RegisterRead->Length;
        RegisterBuffer = ArchReadRegisterBuffer (CpuContext, RegisterRead->Index, RegisterRead->Offset, &Data8);
        Status = SendDataResponsePacket (CpuContext, RegisterBuffer, Data8);
        break;
      }

      if (RegisterRead->Index <= SOFT_DEBUGGER_REGISTER_TSS_LIM) {
        ReadRegisterGroupSegLim (CpuContext, &RegisterGroupSegLim);
        DataN = * ((UINTN *) &RegisterGroupSegLim + (RegisterRead->Index - SOFT_DEBUGGER_REGISTER_CS_LIM));
        Status = SendDataResponsePacket (CpuContext, (UINT8 *) &DataN, (UINT16) sizeof (UINTN));
      } else if (RegisterRead->Index <= SOFT_DEBUGGER_REGISTER_TSS_BAS) {
        ReadRegisterGroupSegBase (CpuContext, &RegisterGroupSegBase);
        DataN = * ((UINTN *) &RegisterGroupSegBase + (RegisterRead->Index - SOFT_DEBUGGER_REGISTER_CS_BAS));
        Status = SendDataResponsePacket (CpuContext, (UINT8 *) &DataN, (UINT16) sizeof (UINTN));
      } else if (RegisterRead->Index < SOFT_DEBUGGER_REGISTER_IDT_LIM) {
        Data64 = ReadRegisterSelectorByIndex (CpuContext, RegisterRead->Index);
        Status = SendDataResponsePacket (CpuContext, (UINT8 *) &Data64, (UINT16) sizeof (UINT64));
      } else {
        switch (RegisterRead->Index) {
        case SOFT_DEBUGGER_REGISTER_IDT_LIM:
          DataN = (UINTN) (CpuContext->Idtr[0] & 0xffff);
          SendDataResponsePacket (CpuContext, (UINT8 *) &DataN, (UINT16) sizeof (UINTN));
          break;
        case SOFT_DEBUGGER_REGISTER_GDT_LIM:
          DataN = (UINTN) (CpuContext->Gdtr[0] & 0xffff);
          SendDataResponsePacket (CpuContext, (UINT8 *) &DataN, (UINT16) sizeof (UINTN));
          break;
        case SOFT_DEBUGGER_REGISTER_IDT_BAS:
          DataN = (UINTN) RShiftU64 (CpuContext->Idtr[0], 16);
          DataN |= (UINTN) LShiftU64 (CpuContext->Idtr[1], (UINT16) (sizeof (UINTN) * 8 - 16));
          SendDataResponsePacket (CpuContext, (UINT8 *) &DataN, (UINT16) sizeof (UINTN));
          break;
        case SOFT_DEBUGGER_REGISTER_GDT_BAS:
          DataN = (UINTN) RShiftU64 (CpuContext->Gdtr[0], 16);
          DataN |= (UINTN) LShiftU64 (CpuContext->Gdtr[1], (UINT16) (sizeof (UINTN) * 8 - 16));
          SendDataResponsePacket (CpuContext, (UINT8 *) &DataN, (UINT16) sizeof (UINTN));
          break;
        }
      }
      break;

    case DEBUG_COMMAND_WRITE_REGISTER:
      RegisterWrite = (DEBUG_DATA_WRITE_REGISTER *) (DebugHeader + 1);
      ArchWriteRegisterBuffer (CpuContext, RegisterWrite->Index, RegisterWrite->Offset, RegisterWrite->Length, (UINT8 *)&RegisterWrite->Value);
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_ARCH_MODE:
      Data8 = DEBUG_ARCH_SYMBOL;
      Status = SendDataResponsePacket (CpuContext, (UINT8 *) &Data8, (UINT16) sizeof (UINT8));
      break;

    case DEBUG_COMMAND_READ_MSR:
      MsrRegisterRead = (DEBUG_DATA_READ_MSR *) (DebugHeader + 1);
      Data64 = AsmReadMsr64 (MsrRegisterRead->Index);
      Status = SendDataResponsePacket (CpuContext, (UINT8 *) &Data64, (UINT16) sizeof (UINT64));
      break;

    case DEBUG_COMMAND_WRITE_MSR:
      MsrRegisterWrite = (DEBUG_DATA_WRITE_MSR *) (DebugHeader + 1);
      AsmWriteMsr64 (MsrRegisterWrite->Index, MsrRegisterWrite->Value);
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_READ_REGISTER_GROUP:
      Data8 = *(UINT8 *) (DebugHeader + 1);
      Status = ArchReadRegisterGroup (CpuContext, Data8);
      break;

    case DEBUG_COMMAND_SET_DEBUG_FLAG:
      Data32 = *(UINT32 *) (DebugHeader + 1);
      SetDebugFlag (Data32);
      SendAckPacket (DEBUG_COMMAND_OK);
      break;

    case DEBUG_COMMAND_GET_REVISION:
      DebugAgentRevision.Revision = DEBUG_AGENT_REVISION;
      DebugAgentRevision.Capabilities = DEBUG_AGENT_CAPABILITIES;
      Status = SendDataResponsePacket (CpuContext, (UINT8 *) &DebugAgentRevision, (UINT16) sizeof (DEBUG_DATA_RESPONSE_GET_REVISION));
      break;

    case DEBUG_COMMAND_GET_EXCEPTION:
      Exception.ExceptionNum  = (UINT8) Vector;
      Exception.ExceptionData = 0;
      Status = SendDataResponsePacket (CpuContext, (UINT8 *) &Exception, (UINT16) sizeof (DEBUG_DATA_RESPONSE_GET_EXCEPTION));
      break;

    case DEBUG_COMMAND_SET_VIEWPOINT:
      Data32 = *(UINT32 *) (DebugHeader + 1);

      if (MultiProcessorDebugSupport) {
        if (IsCpuStopped (Data32)) {
          SetDebugViewPoint (Data32);
          SendAckPacket (DEBUG_COMMAND_OK);
        } else {
          //
          // If CPU is not halted
          //
          SendAckPacket (DEBUG_COMMAND_NOT_SUPPORTED);
        }
      } else if (Data32 == 0) {
        SendAckPacket (DEBUG_COMMAND_OK);

      } else {
        SendAckPacket (DEBUG_COMMAND_NOT_SUPPORTED);
      }

      break;

    case DEBUG_COMMAND_GET_VIEWPOINT:
      Data32 = mDebugMpContext.ViewPointIndex;
      SendDataResponsePacket(CpuContext, (UINT8 *) &Data32, (UINT16) sizeof (UINT32));
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
  UINT8                     InputCharacter;
  UINT8                     BreakCause;
  UINTN                     SavedEip;
  BOOLEAN                   BreakReceived;
  UINT32                    ProcessorIndex;
  UINT32                    CurrentDebugTimerInitCount;
  DEBUG_PORT_HANDLE         Handle;
  UINT8                     Data8;

  Handle = GetDebugPortHandle();

  ProcessorIndex = 0;
  BreakReceived  = FALSE;

  if (MultiProcessorDebugSupport) {
    ProcessorIndex = GetProcessorIndex ();
    while (mDebugMpContext.RunCommandSet);
  }

  switch (Vector) {
  case DEBUG_INT1_VECTOR:
  case DEBUG_INT3_VECTOR:

    BreakCause = GetBreakCause (Vector, CpuContext);

    if (BreakCause == DEBUG_DATA_BREAK_CAUSE_SYSTEM_RESET) {

      //
      // Init break, if no ack received after 200ms, return
      //
      SendAckPacket (DEBUG_COMMAND_INIT_BREAK);
      if (WaitForAckPacketOK (200 * 1000, &BreakReceived) != RETURN_SUCCESS) {
        break;
      }

      SetHostConnectedFlag ();
      CommandCommunication (Vector, CpuContext, BreakReceived);

    } else if (BreakCause == DEBUG_DATA_BREAK_CAUSE_STEPPING) {

      //
      // Stepping is finished, send Ack package.
      //
      if (MultiProcessorDebugSupport) {
        mDebugMpContext.BreakAtCpuIndex = ProcessorIndex;
      }
      SendAckPacket (DEBUG_COMMAND_OK);
      CommandCommunication (Vector, CpuContext, BreakReceived);

    } else if (BreakCause == DEBUG_DATA_BREAK_CAUSE_MEMORY_READY) {

      //
      // Memory is ready
      //
      SendAckPacket (DEBUG_COMMAND_MEMORY_READY);
      WaitForAckPacketOK (0, &BreakReceived);
      CommandCommunication (Vector, CpuContext, BreakReceived);

    } else {

      if (BreakCause == DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD || BreakCause == DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD) {
        
        //
        // Set AL to DEBUG_AGENT_IMAGE_CONTINUE
        //
        Data8 = DEBUG_AGENT_IMAGE_CONTINUE;
        ArchWriteRegisterBuffer (CpuContext, SOFT_DEBUGGER_REGISTER_AX, 0, 1, &Data8);

        if (!IsHostConnected ()) {
          //
          // If HOST is not connected, return
          //
          break;
        }
      }

      AcquireDebugPortControl ();

      if (MultiProcessorDebugSupport) {
        if(!IsAllCpuRunning ()) {
          //
          // If other processors have been stopped
          //
          SetCpuBreakFlagByIndex (ProcessorIndex, TRUE);
        } else {
          //
          // If no any processor was stopped, try to halt other processors
          //
          HaltOtherProcessors (ProcessorIndex);
          SendAckPacket (DEBUG_COMMAND_BREAK_POINT);
          WaitForAckPacketOK (0, &BreakReceived);
        }
      } else {
        SendAckPacket (DEBUG_COMMAND_BREAK_POINT);
        WaitForAckPacketOK (0, &BreakReceived);
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
    }

    break;

  case DEBUG_TIMER_VECTOR:

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

    AcquireDebugPortControl ();
    
    while (DebugPortPollBuffer (Handle)) {
      //
      // If there is data in debug port, will check whether it is break-in symbol,
      // If yes, go into communication mode with HOST.
      // If no, exit interrupt process.
      //
      DebugPortReadBuffer (Handle, &InputCharacter, 1, 0);
      if (InputCharacter == DEBUG_STARTING_SYMBOL_BREAK) {
        SendAckPacket (DEBUG_COMMAND_OK);
        if (MultiProcessorDebugSupport) {
          if(FindCpuNotRunning () != -1) {
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

      AcquireDebugPortControl ();

      if (MultiProcessorDebugSupport) {
        if(FindCpuNotRunning () != -1) {
          SetCpuBreakFlagByIndex (ProcessorIndex, TRUE);
        } else {
          HaltOtherProcessors (ProcessorIndex);
        }
      }
      SendAckPacket (DEBUG_COMMAND_BREAK_POINT);
      WaitForAckPacketOK (0, &BreakReceived);
      ReleaseDebugPortControl ();
      CommandCommunication (Vector, CpuContext, BreakReceived);
    }
    break;
  }

  if (MultiProcessorDebugSupport) {
    //
    // Clear flag and wait for all processors run here
    //
    SetIpiSentByApFlag (FALSE);
    while (mDebugMpContext.RunCommandSet);
  }

  return;
}

