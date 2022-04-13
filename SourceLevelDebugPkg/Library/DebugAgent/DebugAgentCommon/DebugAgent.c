/** @file
  Commond Debug Agent library implementation. It mainly includes
  the first C function called by exception/interrupt handlers,
  read/write debug packet to communication with HOST based on transfer
  protocol.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DebugAgent.h"
#include "Ia32/DebugException.h"

GLOBAL_REMOVE_IF_UNREFERENCED CHAR8  mErrorMsgVersionAlert[]       = "\rThe SourceLevelDebugPkg you are using requires a newer version of the Intel(R) UDK Debugger Tool.\r\n";
GLOBAL_REMOVE_IF_UNREFERENCED CHAR8  mErrorMsgSendInitPacket[]     = "\rSend INIT break packet and try to connect the HOST (Intel(R) UDK Debugger Tool v1.5) ...\r\n";
GLOBAL_REMOVE_IF_UNREFERENCED CHAR8  mErrorMsgConnectOK[]          = "HOST connection is successful!\r\n";
GLOBAL_REMOVE_IF_UNREFERENCED CHAR8  mErrorMsgConnectFail[]        = "HOST connection is failed!\r\n";
GLOBAL_REMOVE_IF_UNREFERENCED CHAR8  mWarningMsgIngoreBreakpoint[] = "Ignore break point in SMM for SMI issued during DXE debugging!\r\n";

//
// Vector Handoff Info list used by Debug Agent for persist
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_VECTOR_HANDOFF_INFO  mVectorHandoffInfoDebugAgent[] = {
  {
    DEBUG_EXCEPT_DIVIDE_ERROR,         // Vector 0
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_DEBUG,                // Vector 1
    EFI_VECTOR_HANDOFF_DO_NOT_HOOK,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_NMI,                  // Vector 2
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_BREAKPOINT,           // Vector 3
    EFI_VECTOR_HANDOFF_DO_NOT_HOOK,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_OVERFLOW,             // Vector 4
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_BOUND,                // Vector 5
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_INVALID_OPCODE,       // Vector 6
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_DOUBLE_FAULT,         // Vector 8
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_INVALID_TSS,          // Vector 10
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_SEG_NOT_PRESENT,      // Vector 11
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_STACK_FAULT,          // Vector 12
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_GP_FAULT,             // Vector 13
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_PAGE_FAULT,           // Vector 14
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_FP_ERROR,             // Vector 16
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_ALIGNMENT_CHECK,      // Vector 17
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_MACHINE_CHECK,        // Vector 18
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_EXCEPT_SIMD,                 // Vector 19
    EFI_VECTOR_HANDOFF_HOOK_BEFORE,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_TIMER_VECTOR,                // Vector 32
    EFI_VECTOR_HANDOFF_DO_NOT_HOOK,
    EFI_DEBUG_AGENT_GUID
  },
  {
    DEBUG_MAILBOX_VECTOR,              // Vector 33
    EFI_VECTOR_HANDOFF_DO_NOT_HOOK,
    EFI_DEBUG_AGENT_GUID
  },
  {
    0,
    EFI_VECTOR_HANDOFF_LAST_ENTRY,
    { 0 }
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mVectorHandoffInfoCount = sizeof (mVectorHandoffInfoDebugAgent) / sizeof (EFI_VECTOR_HANDOFF_INFO);

/**
  Calculate CRC16 for target data.

  @param[in]  Data              The target data.
  @param[in]  DataSize          The target data size.
  @param[in]  Crc               Initial CRC.

  @return UINT16     The CRC16 value.

**/
UINT16
CalculateCrc16 (
  IN UINT8   *Data,
  IN UINTN   DataSize,
  IN UINT16  Crc
  )
{
  UINTN  Index;
  UINTN  BitIndex;

  for (Index = 0; Index < DataSize; Index++) {
    Crc ^= (UINT16)Data[Index];
    for (BitIndex = 0; BitIndex < 8; BitIndex++) {
      if ((Crc & 0x8000) != 0) {
        Crc <<= 1;
        Crc  ^= 0x1021;
      } else {
        Crc <<= 1;
      }
    }
  }

  return Crc;
}

/**
  Read IDT entry to check if IDT entries are setup by Debug Agent.

  @retval  TRUE     IDT entries were setup by Debug Agent.
  @retval  FALSE    IDT entries were not setup by Debug Agent.

**/
BOOLEAN
IsDebugAgentInitialzed (
  VOID
  )
{
  UINTN  InterruptHandler;

  InterruptHandler = (UINTN)GetExceptionHandlerInIdtEntry (0);
  if ((InterruptHandler >= 4) &&  (*(UINT32 *)(InterruptHandler - 4) == AGENT_HANDLER_SIGNATURE)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Find and report module image info to HOST.

  @param[in] AlignSize      Image aligned size.

**/
VOID
FindAndReportModuleImageInfo (
  IN UINTN  AlignSize
  )
{
  UINTN                         Pe32Data;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;

  //
  // Find Image Base
  //
  Pe32Data = PeCoffSearchImageBase ((UINTN)mErrorMsgVersionAlert);
  if (Pe32Data != 0) {
    ImageContext.ImageAddress = Pe32Data;
    ImageContext.PdbPointer   = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageContext.ImageAddress);
    PeCoffLoaderRelocateImageExtraAction (&ImageContext);
  }
}

/**
  Trigger one software interrupt to debug agent to handle it.

  @param[in] Signature       Software interrupt signature.

**/
VOID
TriggerSoftInterrupt (
  IN UINT32  Signature
  )
{
  UINTN  Dr0;
  UINTN  Dr1;

  //
  // Save Debug Register State
  //
  Dr0 = AsmReadDr0 ();
  Dr1 = AsmReadDr1 ();

  //
  // DR0 = Signature
  //
  AsmWriteDr0 (SOFT_INTERRUPT_SIGNATURE);
  AsmWriteDr1 (Signature);

  //
  // Do INT3 to communicate with HOST side
  //
  CpuBreakpoint ();

  //
  // Restore Debug Register State only when Host didn't change it inside exception handler.
  //   Dr registers can only be changed by setting the HW breakpoint.
  //
  AsmWriteDr0 (Dr0);
  AsmWriteDr1 (Dr1);
}

/**
  Calculate Mailbox checksum and update the checksum field.

  @param[in]  Mailbox  Debug Agent Mailbox pointer.

**/
VOID
UpdateMailboxChecksum (
  IN DEBUG_AGENT_MAILBOX  *Mailbox
  )
{
  Mailbox->CheckSum = CalculateCheckSum8 ((UINT8 *)Mailbox, sizeof (DEBUG_AGENT_MAILBOX) - 2);
}

/**
  Verify Mailbox checksum.

  If checksum error, print debug message and run init dead loop.

  @param[in]  Mailbox  Debug Agent Mailbox pointer.

**/
VOID
VerifyMailboxChecksum (
  IN DEBUG_AGENT_MAILBOX  *Mailbox
  )
{
  UINT8  CheckSum;

  CheckSum = CalculateCheckSum8 ((UINT8 *)Mailbox, sizeof (DEBUG_AGENT_MAILBOX) - 2);
  //
  // The checksum updating process may be disturbed by hardware SMI, we need to check CheckSum field
  // and ToBeCheckSum field to validate the mail box.
  //
  if ((CheckSum != Mailbox->CheckSum) && (CheckSum != Mailbox->ToBeCheckSum)) {
    DEBUG ((DEBUG_ERROR, "DebugAgent: Mailbox checksum error, stack or heap crashed!\n"));
    DEBUG ((DEBUG_ERROR, "DebugAgent: CheckSum = %x, Mailbox->CheckSum = %x, Mailbox->ToBeCheckSum = %x\n", CheckSum, Mailbox->CheckSum, Mailbox->ToBeCheckSum));
    CpuDeadLoop ();
  }
}

/**
  Update Mailbox content by index.

  @param[in]  Mailbox  Debug Agent Mailbox pointer.
  @param[in]  Index    Mailbox content index.
  @param[in]  Value    Value to be set into Mailbox.

**/
VOID
UpdateMailboxContent (
  IN DEBUG_AGENT_MAILBOX  *Mailbox,
  IN UINTN                Index,
  IN UINT64               Value
  )
{
  AcquireMpSpinLock (&mDebugMpContext.MailboxSpinLock);
  switch (Index) {
    case DEBUG_MAILBOX_DEBUG_FLAG_INDEX:
      Mailbox->ToBeCheckSum = Mailbox->CheckSum + CalculateSum8 ((UINT8 *)&Mailbox->DebugFlag.Uint64, sizeof (UINT64))
                              - CalculateSum8 ((UINT8 *)&Value, sizeof (UINT64));
      Mailbox->DebugFlag.Uint64 = Value;
      break;
    case DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX:
      Mailbox->ToBeCheckSum = Mailbox->CheckSum + CalculateSum8 ((UINT8 *)&Mailbox->DebugPortHandle, sizeof (UINTN))
                              - CalculateSum8 ((UINT8 *)&Value, sizeof (UINTN));
      Mailbox->DebugPortHandle = (UINTN)Value;
      break;
    case DEBUG_MAILBOX_EXCEPTION_BUFFER_POINTER_INDEX:
      Mailbox->ToBeCheckSum = Mailbox->CheckSum + CalculateSum8 ((UINT8 *)&Mailbox->ExceptionBufferPointer, sizeof (UINTN))
                              - CalculateSum8 ((UINT8 *)&Value, sizeof (UINTN));
      Mailbox->ExceptionBufferPointer = (UINTN)Value;
      break;
    case DEBUG_MAILBOX_LAST_ACK:
      Mailbox->ToBeCheckSum = Mailbox->CheckSum + CalculateSum8 ((UINT8 *)&Mailbox->LastAck, sizeof (UINT8))
                              - CalculateSum8 ((UINT8 *)&Value, sizeof (UINT8));
      Mailbox->LastAck = (UINT8)Value;
      break;
    case DEBUG_MAILBOX_SEQUENCE_NO_INDEX:
      Mailbox->ToBeCheckSum = Mailbox->CheckSum + CalculateSum8 ((UINT8 *)&Mailbox->SequenceNo, sizeof (UINT8))
                              - CalculateSum8 ((UINT8 *)&Value, sizeof (UINT8));
      Mailbox->SequenceNo = (UINT8)Value;
      break;
    case DEBUG_MAILBOX_HOST_SEQUENCE_NO_INDEX:
      Mailbox->ToBeCheckSum = Mailbox->CheckSum + CalculateSum8 ((UINT8 *)&Mailbox->HostSequenceNo, sizeof (UINT8))
                              - CalculateSum8 ((UINT8 *)&Value, sizeof (UINT8));
      Mailbox->HostSequenceNo = (UINT8)Value;
      break;
    case DEBUG_MAILBOX_DEBUG_TIMER_FREQUENCY:
      Mailbox->ToBeCheckSum = Mailbox->CheckSum + CalculateSum8 ((UINT8 *)&Mailbox->DebugTimerFrequency, sizeof (UINT32))
                              - CalculateSum8 ((UINT8 *)&Value, sizeof (UINT32));
      Mailbox->DebugTimerFrequency = (UINT32)Value;
      break;
  }

  UpdateMailboxChecksum (Mailbox);
  ReleaseMpSpinLock (&mDebugMpContext.MailboxSpinLock);
}

/**
  Read data from debug device and save the data in buffer.

  Reads NumberOfBytes data bytes from a debug device into the buffer
  specified by Buffer. The number of bytes actually read is returned.
  If the return value is less than NumberOfBytes, then the rest operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Handle           Debug port handle.
  @param  Buffer           Pointer to the data buffer to store the data read from the debug device.
  @param  NumberOfBytes    Number of bytes which will be read.
  @param  Timeout          Timeout value for reading from debug device. It unit is Microsecond.

  @retval 0                Read data failed, no data is to be read.
  @retval >0               Actual number of bytes read from debug device.

**/
UINTN
DebugAgentReadBuffer (
  IN DEBUG_PORT_HANDLE  Handle,
  IN UINT8              *Buffer,
  IN UINTN              NumberOfBytes,
  IN UINTN              Timeout
  )
{
  UINTN   Index;
  UINT32  Begin;
  UINT32  TimeoutTicker;
  UINT32  TimerRound;
  UINT32  TimerFrequency;
  UINT32  TimerCycle;

  Begin          = 0;
  TimeoutTicker  = 0;
  TimerRound     = 0;
  TimerFrequency = GetMailboxPointer ()->DebugTimerFrequency;
  TimerCycle     = GetApicTimerInitCount ();

  if (Timeout != 0) {
    Begin         = GetApicTimerCurrentCount ();
    TimeoutTicker = (UINT32)DivU64x32 (
                              MultU64x64 (
                                TimerFrequency,
                                Timeout
                                ),
                              1000000u
                              );
    TimerRound = (UINT32)DivU64x32Remainder (TimeoutTicker, TimerCycle / 2, &TimeoutTicker);
  }

  Index = 0;
  while (Index < NumberOfBytes) {
    if (DebugPortPollBuffer (Handle)) {
      DebugPortReadBuffer (Handle, Buffer + Index, 1, 0);
      Index++;
      continue;
    }

    if (Timeout != 0) {
      if (TimerRound == 0) {
        if (IsDebugTimerTimeout (TimerCycle, Begin, TimeoutTicker)) {
          //
          // If time out occurs.
          //
          return 0;
        }
      } else {
        if (IsDebugTimerTimeout (TimerCycle, Begin, TimerCycle / 2)) {
          TimerRound--;
          Begin = GetApicTimerCurrentCount ();
        }
      }
    }
  }

  return Index;
}

/**
  Set debug flag in mailbox.

  @param[in]  FlagMask      Debug flag mask value.
  @param[in]  FlagValue     Debug flag value.

**/
VOID
SetDebugFlag (
  IN UINT64  FlagMask,
  IN UINT32  FlagValue
  )
{
  DEBUG_AGENT_MAILBOX  *Mailbox;
  UINT64               Data64;

  Mailbox = GetMailboxPointer ();
  Data64  = (Mailbox->DebugFlag.Uint64 & ~FlagMask) |
            (LShiftU64 ((UINT64)FlagValue, LowBitSet64 (FlagMask)) & FlagMask);
  UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_DEBUG_FLAG_INDEX, Data64);
}

/**
  Get debug flag in mailbox.

  @param[in]  FlagMask      Debug flag mask value.

  @return Debug flag value.

**/
UINT32
GetDebugFlag (
  IN UINT64  FlagMask
  )
{
  DEBUG_AGENT_MAILBOX  *Mailbox;
  UINT32               DebugFlag;

  Mailbox   = GetMailboxPointer ();
  DebugFlag = (UINT32)RShiftU64 (Mailbox->DebugFlag.Uint64 & FlagMask, LowBitSet64 (FlagMask));

  return DebugFlag;
}

/**
  Send a debug message packet to the debug port.

  @param[in] Buffer  The debug message.
  @param[in] Length  The length of debug message.

**/
VOID
SendDebugMsgPacket (
  IN CHAR8  *Buffer,
  IN UINTN  Length
  )
{
  DEBUG_PACKET_HEADER  DebugHeader;
  DEBUG_PORT_HANDLE    Handle;

  Handle = GetDebugPortHandle ();

  DebugHeader.StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;
  DebugHeader.Command     = DEBUG_COMMAND_PRINT_MESSAGE;
  DebugHeader.Length      = sizeof (DEBUG_PACKET_HEADER) + (UINT8)Length;
  DebugHeader.SequenceNo  = 0xEE;
  DebugHeader.Crc         = 0;
  DebugHeader.Crc         = CalculateCrc16 (
                              (UINT8 *)Buffer,
                              Length,
                              CalculateCrc16 ((UINT8 *)&DebugHeader, sizeof (DEBUG_PACKET_HEADER), 0)
                              );

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
  IN UINT8  ErrorLevel,
  IN CHAR8  *Format,
  ...
  )
{
  CHAR8    Buffer[DEBUG_DATA_MAXIMUM_REAL_DATA];
  VA_LIST  Marker;

  //
  // Check driver debug mask value and global mask
  //
  if ((ErrorLevel & GetDebugFlag (DEBUG_AGENT_FLAG_PRINT_ERROR_LEVEL)) == 0) {
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
  IN UINT8    ErrorLevel,
  IN BOOLEAN  IsSend,
  IN UINT8    *Data,
  IN UINT8    Length
  )
{
  CHAR8  Buffer[DEBUG_DATA_MAXIMUM_REAL_DATA];
  CHAR8  *DestBuffer;
  UINTN  Index;

  //
  // Check driver debug mask value and global mask
  //
  if ((ErrorLevel & GetDebugFlag (DEBUG_AGENT_FLAG_PRINT_ERROR_LEVEL)) == 0) {
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
    Index++;
    if (Index >= Length) {
      //
      // The last character of debug message has been formatted in buffer
      //
      DestBuffer += AsciiSPrint (DestBuffer, DEBUG_DATA_MAXIMUM_REAL_DATA - (DestBuffer - Buffer), "]\n");
      SendDebugMsgPacket (Buffer, DestBuffer - Buffer);
      break;
    }
  }
}

/**
  Read remaing debug packet except for the start symbol

  @param[in]      Handle        Pointer to Debug Port handle.
  @param[in, out] DebugHeader   Debug header buffer including start symbol.

  @retval EFI_SUCCESS        Read the symbol in BreakSymbol.
  @retval EFI_CRC_ERROR      CRC check fail.
  @retval EFI_TIMEOUT        Timeout occurs when reading debug packet.
  @retval EFI_DEVICE_ERROR   Receive the old or responsed packet.

**/
EFI_STATUS
ReadRemainingBreakPacket (
  IN     DEBUG_PORT_HANDLE    Handle,
  IN OUT DEBUG_PACKET_HEADER  *DebugHeader
  )
{
  UINT16               Crc;
  DEBUG_AGENT_MAILBOX  *Mailbox;

  //
  // Has received start symbol, try to read the rest part
  //
  if (DebugAgentReadBuffer (Handle, (UINT8 *)DebugHeader + OFFSET_OF (DEBUG_PACKET_HEADER, Command), sizeof (DEBUG_PACKET_HEADER) - OFFSET_OF (DEBUG_PACKET_HEADER, Command), READ_PACKET_TIMEOUT) == 0) {
    //
    // Timeout occur, exit
    //
    DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "Timeout in Debug Timer interrupt\n");
    return EFI_TIMEOUT;
  }

  Crc              = DebugHeader->Crc;
  DebugHeader->Crc = 0;
  if (CalculateCrc16 ((UINT8 *)DebugHeader, DebugHeader->Length, 0) != Crc) {
    DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "Debug Timer CRC (%x) against (%x)\n", Crc, CalculateCrc16 ((UINT8 *)&DebugHeader, DebugHeader->Length, 0));
    DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, FALSE, (UINT8 *)DebugHeader, DebugHeader->Length);
    return EFI_CRC_ERROR;
  }

  Mailbox = GetMailboxPointer ();
  if (IS_REQUEST (DebugHeader)) {
    if (DebugHeader->SequenceNo == (UINT8)(Mailbox->HostSequenceNo + 1)) {
      //
      // Only updagte HostSequenceNo for new command packet
      //
      UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_HOST_SEQUENCE_NO_INDEX, DebugHeader->SequenceNo);
      return EFI_SUCCESS;
    }

    if (DebugHeader->SequenceNo == Mailbox->HostSequenceNo) {
      return EFI_SUCCESS;
    }
  }

  return EFI_DEVICE_ERROR;
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
  return (BOOLEAN)(GetDebugFlag (DEBUG_AGENT_FLAG_HOST_ATTACHED) == 1);
}

/**
  Set HOST connect flag in Mailbox.

  @param[in] Attached        Attach status.

**/
VOID
SetHostAttached (
  IN BOOLEAN  Attached
  )
{
  DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Attach status is %d\n", Attached);
  SetDebugFlag (DEBUG_AGENT_FLAG_HOST_ATTACHED, (UINT32)Attached);
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
  RETURN_STATUS  Status;

  Status = RETURN_SUCCESS;
  switch (DebugSetting->Key) {
    case DEBUG_AGENT_SETTING_SMM_ENTRY_BREAK:
      SetDebugFlag (DEBUG_AGENT_FLAG_BREAK_ON_NEXT_SMI, DebugSetting->Value);
      break;
    case DEBUG_AGENT_SETTING_PRINT_ERROR_LEVEL:
      SetDebugFlag (DEBUG_AGENT_FLAG_PRINT_ERROR_LEVEL, DebugSetting->Value);
      break;
    case DEBUG_AGENT_SETTING_BOOT_SCRIPT_ENTRY_BREAK:
      SetDebugFlag (DEBUG_AGENT_FLAG_BREAK_BOOT_SCRIPT, DebugSetting->Value);
      break;
    default:
      Status = RETURN_UNSUPPORTED;
  }

  return Status;
}

/**
  Execute GO command.

  @param[in] CpuContext        Pointer to saved CPU context.

**/
VOID
CommandGo (
  IN DEBUG_CPU_CONTEXT  *CpuContext
  )
{
  IA32_EFLAGS32  *Eflags;

  Eflags          = (IA32_EFLAGS32 *)&CpuContext->Eflags;
  Eflags->Bits.TF = 0;
  Eflags->Bits.RF = 1;
}

/**
  Execute Stepping command.

  @param[in] CpuContext        Pointer to saved CPU context.

**/
VOID
CommandStepping (
  IN DEBUG_CPU_CONTEXT  *CpuContext
  )
{
  IA32_EFLAGS32  *Eflags;

  Eflags          = (IA32_EFLAGS32 *)&CpuContext->Eflags;
  Eflags->Bits.TF = 1;
  Eflags->Bits.RF = 1;
  //
  // Save and clear EFLAGS.IF to avoid interrupt happen when executing Stepping
  //
  SetDebugFlag (DEBUG_AGENT_FLAG_INTERRUPT_FLAG, Eflags->Bits.IF);
  Eflags->Bits.IF = 0;
  //
  // Set Stepping Flag
  //
  SetDebugFlag (DEBUG_AGENT_FLAG_STEPPING, 1);
}

/**
  Do some cleanup after Stepping command done.

  @param[in] CpuContext        Pointer to saved CPU context.

**/
VOID
CommandSteppingCleanup (
  IN DEBUG_CPU_CONTEXT  *CpuContext
  )
{
  IA32_EFLAGS32  *Eflags;

  Eflags = (IA32_EFLAGS32 *)&CpuContext->Eflags;
  //
  // Restore EFLAGS.IF
  //
  Eflags->Bits.IF = GetDebugFlag (DEBUG_AGENT_FLAG_INTERRUPT_FLAG);
  //
  // Clear Stepping flag
  //
  SetDebugFlag (DEBUG_AGENT_FLAG_STEPPING, 0);
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
  UINT8  RegisterIndex;
  UINTN  Dr7Value;

  RegisterIndex = SetHwBreakpoint->Type.Index;

  //
  // Set debug address
  //
  *((UINTN *)&CpuContext->Dr0 + RegisterIndex) = (UINTN)SetHwBreakpoint->Address;

  Dr7Value = CpuContext->Dr7;

  //
  // Enable Gx, Lx
  //
  Dr7Value |= (UINTN)(0x3 << (RegisterIndex * 2));
  //
  // Set RWx and Lenx
  //
  Dr7Value &= (UINTN)(~(0xf << (16 + RegisterIndex * 4)));
  Dr7Value |= (UINTN)((SetHwBreakpoint->Type.Length << 2) | SetHwBreakpoint->Type.Access) << (16 + RegisterIndex * 4);
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
  IN DEBUG_CPU_CONTEXT               *CpuContext,
  IN DEBUG_DATA_CLEAR_HW_BREAKPOINT  *ClearHwBreakpoint
  )
{
  if ((ClearHwBreakpoint->IndexMask & BIT0) != 0) {
    CpuContext->Dr0  = 0;
    CpuContext->Dr7 &= (UINTN)(~(0x3 << 0));
  }

  if ((ClearHwBreakpoint->IndexMask & BIT1) != 0) {
    CpuContext->Dr1  = 0;
    CpuContext->Dr7 &= (UINTN)(~(0x3 << 2));
  }

  if ((ClearHwBreakpoint->IndexMask & BIT2) != 0) {
    CpuContext->Dr2  = 0;
    CpuContext->Dr7 &= (UINTN)(~(0x3 << 4));
  }

  if ((ClearHwBreakpoint->IndexMask & BIT3) != 0) {
    CpuContext->Dr3  = 0;
    CpuContext->Dr7 &= (UINTN)(~(0x3 << 6));
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
  IN  UINT8  Index,
  OUT UINT8  *Width
  )
{
  if (Index < SOFT_DEBUGGER_REGISTER_ST0) {
    switch (Index) {
      case SOFT_DEBUGGER_REGISTER_FP_FCW:
        *Width = (UINT8)sizeof (UINT16);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Fcw);

      case SOFT_DEBUGGER_REGISTER_FP_FSW:
        *Width = (UINT8)sizeof (UINT16);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Fsw);

      case SOFT_DEBUGGER_REGISTER_FP_FTW:
        *Width = (UINT8)sizeof (UINT16);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Ftw);

      case SOFT_DEBUGGER_REGISTER_FP_OPCODE:
        *Width = (UINT8)sizeof (UINT16);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Opcode);

      case SOFT_DEBUGGER_REGISTER_FP_EIP:
        *Width = (UINT8)sizeof (UINT32);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Eip);

      case SOFT_DEBUGGER_REGISTER_FP_CS:
        *Width = (UINT8)sizeof (UINT16);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Cs);

      case SOFT_DEBUGGER_REGISTER_FP_DATAOFFSET:
        *Width = (UINT8)sizeof (UINT32);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, DataOffset);

      case SOFT_DEBUGGER_REGISTER_FP_DS:
        *Width = (UINT8)sizeof (UINT16);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Ds);

      case SOFT_DEBUGGER_REGISTER_FP_MXCSR:
        *Width = (UINT8)sizeof (UINT32);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Mxcsr);

      case SOFT_DEBUGGER_REGISTER_FP_MXCSR_MASK:
        *Width = (UINT8)sizeof (UINT32);
        return OFFSET_OF (DEBUG_DATA_FX_SAVE_STATE, Mxcsr_Mask);
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
  IN DEBUG_CPU_CONTEXT  *CpuContext,
  IN UINT8              Index,
  OUT UINT8             *Width
  )
{
  UINT8  *Buffer;

  if (Index < SOFT_DEBUGGER_REGISTER_FP_BASE) {
    Buffer = (UINT8 *)CpuContext + OFFSET_OF (DEBUG_CPU_CONTEXT, Dr0) + Index * sizeof (UINTN);
    *Width = (UINT8)sizeof (UINTN);
  } else {
    //
    // FPU/MMX/XMM registers
    //
    Buffer = (UINT8 *)CpuContext + OFFSET_OF (DEBUG_CPU_CONTEXT, FxSaveState) + ArchReadFxStatOffset (Index, Width);
  }

  return Buffer;
}

/**
  Send the packet without data to HOST.

  @param[in] CommandType    Type of Command.
  @param[in] SequenceNo     Sequence number.

**/
VOID
SendPacketWithoutData (
  IN UINT8  CommandType,
  IN UINT8  SequenceNo
  )
{
  DEBUG_PACKET_HEADER  DebugHeader;
  DEBUG_PORT_HANDLE    Handle;

  Handle = GetDebugPortHandle ();

  DebugHeader.StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;
  DebugHeader.Command     = CommandType;
  DebugHeader.Length      = sizeof (DEBUG_PACKET_HEADER);
  DebugHeader.SequenceNo  = SequenceNo;
  DebugHeader.Crc         = 0;
  DebugHeader.Crc         = CalculateCrc16 ((UINT8 *)&DebugHeader, sizeof (DEBUG_PACKET_HEADER), 0);

  DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, TRUE, (UINT8 *)&DebugHeader, DebugHeader.Length);
  DebugPortWriteBuffer (Handle, (UINT8 *)&DebugHeader, DebugHeader.Length);
}

/**
  Send acknowledge packet to HOST.

  @param[in] AckCommand    Type of Acknowledge packet.

**/
VOID
SendAckPacket (
  IN UINT8  AckCommand
  )
{
  UINT8                SequenceNo;
  DEBUG_AGENT_MAILBOX  *Mailbox;

  if (AckCommand != DEBUG_COMMAND_OK) {
    //
    // This is not ACK OK packet
    //
    DebugAgentMsgPrint (DEBUG_AGENT_ERROR, "Send ACK(%d)\n", AckCommand);
  }

  Mailbox    = GetMailboxPointer ();
  SequenceNo = Mailbox->HostSequenceNo;
  DebugAgentMsgPrint (DEBUG_AGENT_INFO, "SendAckPacket: SequenceNo = %x\n", SequenceNo);
  SendPacketWithoutData (AckCommand, SequenceNo);
  UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_LAST_ACK, AckCommand);
}

/**
  Decompress the Data in place.

  @param[in, out] Data   The compressed data buffer.
                         The buffer is assumed large enough to hold the uncompressed data.
  @param[in]      Length The length of the compressed data buffer.

  @return   The length of the uncompressed data buffer.
**/
UINT8
DecompressDataInPlace (
  IN OUT UINT8  *Data,
  IN UINTN      Length
  )
{
  UINTN   Index;
  UINT16  LastChar;
  UINTN   LastCharCount;
  UINT8   CurrentChar;

  LastChar      = (UINT16)-1;
  LastCharCount = 0;
  for (Index = 0; Index < Length; Index++) {
    CurrentChar = Data[Index];
    if (LastCharCount == 2) {
      LastCharCount = 0;
      CopyMem (&Data[Index + CurrentChar], &Data[Index + 1], Length - Index - 1);
      SetMem (&Data[Index], CurrentChar, (UINT8)LastChar);
      LastChar = (UINT16)-1;
      Index   += CurrentChar - 1;
      Length  += CurrentChar - 1;
    } else {
      if (LastChar != CurrentChar) {
        LastCharCount = 0;
      }

      LastCharCount++;
      LastChar = CurrentChar;
    }
  }

  ASSERT (Length <= DEBUG_DATA_MAXIMUM_REAL_DATA);

  return (UINT8)Length;
}

/**
  Receive valid packet from HOST.

  @param[out] InputPacket         Buffer to receive packet.
  @param[out] BreakReceived       TRUE means break-in symbol received.
                                  FALSE means break-in symbol not received.
  @param[out] IncompatibilityFlag If IncompatibilityFlag is not NULL, return
                                  TRUE:  Compatible packet received.
                                  FALSE: Incompatible packet received.
  @param[in]  Timeout             Time out value to wait for acknowledge from HOST.
                                  The unit is microsecond.
  @param[in]  SkipStartSymbol     TRUE:  Skip time out when reading start symbol.
                                  FALSE: Does not Skip time out when reading start symbol.

  @retval RETURN_SUCCESS   A valid package was received in InputPacket.
  @retval RETURN_TIMEOUT   Timeout occurs.

**/
RETURN_STATUS
ReceivePacket (
  OUT UINT8    *InputPacket,
  OUT BOOLEAN  *BreakReceived,
  OUT BOOLEAN  *IncompatibilityFlag  OPTIONAL,
  IN  UINTN    Timeout,
  IN  BOOLEAN  SkipStartSymbol
  )
{
  DEBUG_PACKET_HEADER  *DebugHeader;
  UINTN                Received;
  DEBUG_PORT_HANDLE    Handle;
  UINT16               Crc;
  UINTN                TimeoutForStartSymbol;

  Handle = GetDebugPortHandle ();
  if (SkipStartSymbol) {
    TimeoutForStartSymbol = 0;
  } else {
    TimeoutForStartSymbol = Timeout;
  }

  DebugHeader = (DEBUG_PACKET_HEADER *)InputPacket;
  while (TRUE) {
    //
    // Find the valid start symbol
    //
    Received = DebugAgentReadBuffer (Handle, &DebugHeader->StartSymbol, sizeof (DebugHeader->StartSymbol), TimeoutForStartSymbol);
    if (Received < sizeof (DebugHeader->StartSymbol)) {
      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "DebugAgentReadBuffer(StartSymbol) timeout\n");
      return RETURN_TIMEOUT;
    }

    if ((DebugHeader->StartSymbol != DEBUG_STARTING_SYMBOL_NORMAL) && (DebugHeader->StartSymbol != DEBUG_STARTING_SYMBOL_COMPRESS)) {
      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "Invalid start symbol received [%02x]\n", DebugHeader->StartSymbol);
      continue;
    }

    //
    // Read Package header till field Length
    //
    Received = DebugAgentReadBuffer (
                 Handle,
                 (UINT8 *)DebugHeader + OFFSET_OF (DEBUG_PACKET_HEADER, Command),
                 OFFSET_OF (DEBUG_PACKET_HEADER, Length) + sizeof (DebugHeader->Length) - sizeof (DebugHeader->StartSymbol),
                 Timeout
                 );
    if (Received == 0) {
      DebugAgentMsgPrint (DEBUG_AGENT_ERROR, "DebugAgentReadBuffer(Command) timeout\n");
      return RETURN_TIMEOUT;
    }

    if (DebugHeader->Length < sizeof (DEBUG_PACKET_HEADER)) {
      if (IncompatibilityFlag != NULL) {
        //
        // This is one old version debug packet format, set Incompatibility flag
        //
        *IncompatibilityFlag = TRUE;
      } else {
        //
        // Skip the bad small packet
        //
        continue;
      }
    } else {
      //
      // Read the payload data include the CRC field
      //
      Received = DebugAgentReadBuffer (Handle, &DebugHeader->SequenceNo, (UINT8)(DebugHeader->Length - OFFSET_OF (DEBUG_PACKET_HEADER, SequenceNo)), Timeout);
      if (Received == 0) {
        DebugAgentMsgPrint (DEBUG_AGENT_ERROR, "DebugAgentReadBuffer(SequenceNo) timeout\n");
        return RETURN_TIMEOUT;
      }

      //
      // Calculate the CRC of Debug Packet
      //
      Crc              = DebugHeader->Crc;
      DebugHeader->Crc = 0;
      if (Crc == CalculateCrc16 ((UINT8 *)DebugHeader, DebugHeader->Length, 0)) {
        break;
      }

      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "CRC Error (received CRC is %x)\n", Crc);
      DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, FALSE, (UINT8 *)DebugHeader, DebugHeader->Length);
    }
  }

  DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, FALSE, (UINT8 *)DebugHeader, DebugHeader->Length);

  if (DebugHeader->StartSymbol == DEBUG_STARTING_SYMBOL_COMPRESS) {
    DebugHeader->StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;
    DebugHeader->Length      = DecompressDataInPlace (
                                 (UINT8 *)(DebugHeader + 1),
                                 DebugHeader->Length - sizeof (DEBUG_PACKET_HEADER)
                                 ) + sizeof (DEBUG_PACKET_HEADER);
  }

  return RETURN_SUCCESS;
}

/**
  Receive acknowledge packet OK from HOST in specified time.

  @param[in]  Command             The command type issued by TARGET.
  @param[in]  Timeout             Time out value to wait for acknowledge from HOST.
                                  The unit is microsecond.
  @param[out] BreakReceived       If BreakReceived is not NULL,
                                  TRUE is returned if break-in symbol received.
                                  FALSE is returned if break-in symbol not received.
  @param[out] IncompatibilityFlag If IncompatibilityFlag is not NULL, return
                                  TRUE:  Compatible packet received.
                                  FALSE: Incompatible packet received.

  @retval  RETURN_SUCCESS   Succeed to receive acknowledge packet from HOST,
                            the type of acknowledge packet saved in Ack.
  @retval  RETURN_TIMEOUT   Specified timeout value was up.

**/
RETURN_STATUS
SendCommandAndWaitForAckOK (
  IN  UINT8    Command,
  IN  UINTN    Timeout,
  OUT BOOLEAN  *BreakReceived  OPTIONAL,
  OUT BOOLEAN  *IncompatibilityFlag OPTIONAL
  )
{
  RETURN_STATUS        Status;
  UINT8                InputPacketBuffer[DEBUG_DATA_UPPER_LIMIT];
  DEBUG_PACKET_HEADER  *DebugHeader;
  UINT8                SequenceNo;
  UINT8                HostSequenceNo;
  UINT8                RetryCount;

  RetryCount  = 3;
  DebugHeader = (DEBUG_PACKET_HEADER *)InputPacketBuffer;
  Status      = RETURN_TIMEOUT;
  while (RetryCount > 0) {
    SequenceNo     = GetMailboxPointer ()->SequenceNo;
    HostSequenceNo = GetMailboxPointer ()->HostSequenceNo;
    SendPacketWithoutData (Command, SequenceNo);
    Status = ReceivePacket ((UINT8 *)DebugHeader, BreakReceived, IncompatibilityFlag, Timeout, FALSE);
    if (Status == RETURN_TIMEOUT) {
      if (Command == DEBUG_COMMAND_INIT_BREAK) {
        RetryCount--;
      } else {
        DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "TARGET: Timeout when waiting for ACK packet.\n");
      }

      continue;
    }

    ASSERT_EFI_ERROR (Status);
    //
    // Status == RETURN_SUCCESS
    //
    if ((DebugHeader->Command == DEBUG_COMMAND_OK) && (DebugHeader->SequenceNo == SequenceNo)) {
      //
      // Received Ack OK
      //
      UpdateMailboxContent (GetMailboxPointer (), DEBUG_MAILBOX_SEQUENCE_NO_INDEX, ++SequenceNo);
      return Status;
    }

    if ((DebugHeader->Command == DEBUG_COMMAND_GO) && ((DebugHeader->SequenceNo == HostSequenceNo) || (Command == DEBUG_COMMAND_INIT_BREAK))) {
      //
      // Received Old GO
      //
      if (Command == DEBUG_COMMAND_INIT_BREAK) {
        DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "TARGET: Receive GO() in last boot\n");
      }

      SendPacketWithoutData (DEBUG_COMMAND_OK, DebugHeader->SequenceNo);
    }
  }

  ASSERT (Command == DEBUG_COMMAND_INIT_BREAK);
  return Status;
}

/**
  Get current break cause.

  @param[in] Vector      Vector value of exception or interrupt.
  @param[in] CpuContext  Pointer to save CPU context.

  @return The type of break cause defined by XXXX

**/
UINT8
GetBreakCause (
  IN UINTN              Vector,
  IN DEBUG_CPU_CONTEXT  *CpuContext
  )
{
  UINT8  Cause;

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
          // DR6.BIT14 Indicates (when set) that the debug exception was
          // triggered by the single step execution mode.
          // The single-step mode is the highest priority debug exception.
          // This is single step, no need to check DR0, to ensure single step
          // work in PeCoffExtraActionLib (right after triggering a breakpoint
          // to report image load/unload).
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
            Cause = (UINT8)((CpuContext->Dr0 == IMAGE_LOAD_SIGNATURE) ?
                            DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD : DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD);
          }

          break;

        case SOFT_INTERRUPT_SIGNATURE:

          if (CpuContext->Dr1 == MEMORY_READY_SIGNATURE) {
            Cause           = DEBUG_DATA_BREAK_CAUSE_MEMORY_READY;
            CpuContext->Dr0 = 0;
          } else if (CpuContext->Dr1 == SYSTEM_RESET_SIGNATURE) {
            Cause           = DEBUG_DATA_BREAK_CAUSE_SYSTEM_RESET;
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
        if (GetDebugFlag (DEBUG_AGENT_FLAG_STEPPING) == 1) {
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
  Copy memory from source to destination with specified width.

  @param[out] Dest        A pointer to the destination buffer of the memory copy.
  @param[in]  Src         A pointer to the source buffer of the memory copy.
  @param[in]  Count       The number of data with specified width to copy from source to destination.
  @param[in]  Width       Data width in byte.

**/
VOID
CopyMemByWidth (
  OUT UINT8   *Dest,
  IN  UINT8   *Src,
  IN  UINT16  Count,
  IN  UINT8   Width
  )
{
  UINT8  *Destination;
  UINT8  *Source;
  INT8   Step;

  if (Src > Dest) {
    Destination = Dest;
    Source      = Src;
    Step        = Width;
  } else {
    //
    // Copy memory from tail to avoid memory overlap
    //
    Destination = Dest + (Count - 1) * Width;
    Source      = Src  + (Count - 1) * Width;
    Step        = -Width;
  }

  while (Count-- != 0) {
    switch (Width) {
      case 1:
        *(UINT8 *)Destination = MmioRead8 ((UINTN)Source);
        break;
      case 2:
        *(UINT16 *)Destination = MmioRead16 ((UINTN)Source);
        break;
      case 4:
        *(UINT32 *)Destination = MmioRead32 ((UINTN)Source);
        break;
      case 8:
        *(UINT64 *)Destination = MmioRead64 ((UINTN)Source);
        break;
      default:
        ASSERT (FALSE);
    }

    Source      += Step;
    Destination += Step;
  }
}

/**
  Compress the data buffer but do not modify the original buffer.

  The compressed data is directly send to the debug channel.
  Compressing in place doesn't work because the data may become larger
  during compressing phase. ("3 3 ..." --> "3 3 0 ...")
  The routine is expected to be called three times:
  1. Compute the length of the compressed data buffer;
  2. Compute the CRC of the compressed data buffer;
  3. Compress the data and send to the debug channel.

  @param[in]  Handle           The debug channel handle to send the compressed data buffer.
  @param[in]  Data             The data buffer.
  @param[in]  Length           The length of the data buffer.
  @param[in]  Send             TRUE to send the compressed data buffer.
  @param[out] CompressedLength Return the length of the compressed data buffer.
                               It may be larger than the Length in some cases.
  @param[out] CompressedCrc    Return the CRC of the compressed data buffer.
**/
VOID
CompressData (
  IN  DEBUG_PORT_HANDLE  Handle,
  IN  UINT8              *Data,
  IN  UINT8              Length,
  IN  BOOLEAN            Send,
  OUT UINTN              *CompressedLength   OPTIONAL,
  OUT UINT16             *CompressedCrc      OPTIONAL
  )
{
  UINTN  Index;
  UINT8  LastChar;
  UINT8  LastCharCount;
  UINT8  CurrentChar;
  UINTN  CompressedIndex;

  ASSERT (Length > 0);
  LastChar      = Data[0] + 1; // Just ensure it's different from the first byte.
  LastCharCount = 0;

  for (Index = 0, CompressedIndex = 0; Index <= Length; Index++) {
    if (Index < Length) {
      CurrentChar = Data[Index];
    } else {
      CurrentChar = (UINT8)LastChar + 1;  // just ensure it's different from LastChar
    }

    if (LastChar != CurrentChar) {
      if (LastCharCount == 1) {
        CompressedIndex++;
        if (CompressedCrc != NULL) {
          *CompressedCrc = CalculateCrc16 (&LastChar, 1, *CompressedCrc);
        }

        if (Send) {
          DebugPortWriteBuffer (Handle, &LastChar, 1);
        }
      } else if (LastCharCount >= 2) {
        CompressedIndex += 3;
        LastCharCount   -= 2;
        if (CompressedCrc != NULL) {
          *CompressedCrc = CalculateCrc16 (&LastChar, 1, *CompressedCrc);
          *CompressedCrc = CalculateCrc16 (&LastChar, 1, *CompressedCrc);
          *CompressedCrc = CalculateCrc16 (&LastCharCount, 1, *CompressedCrc);
        }

        if (Send) {
          DebugPortWriteBuffer (Handle, &LastChar, 1);
          DebugPortWriteBuffer (Handle, &LastChar, 1);
          DebugPortWriteBuffer (Handle, &LastCharCount, 1);
        }
      }

      LastCharCount = 0;
    }

    LastCharCount++;
    LastChar = CurrentChar;
  }

  if (CompressedLength != NULL) {
    *CompressedLength = CompressedIndex;
  }
}

/**
  Read memory with specified width and send packet with response data to HOST.

  @param[in] Data        Pointer to response data buffer.
  @param[in] Count       The number of data with specified Width.
  @param[in] Width       Data width in byte.
  @param[in] DebugHeader Pointer to a buffer for creating response packet and receiving ACK packet,
                         to minimize the stack usage.

  @retval RETURN_SUCCESS      Response data was sent successfully.

**/
RETURN_STATUS
ReadMemoryAndSendResponsePacket (
  IN UINT8                *Data,
  IN UINT16               Count,
  IN UINT8                Width,
  IN DEBUG_PACKET_HEADER  *DebugHeader
  )
{
  RETURN_STATUS      Status;
  BOOLEAN            LastPacket;
  DEBUG_PORT_HANDLE  Handle;
  UINT8              SequenceNo;
  UINTN              RemainingDataSize;
  UINT8              CurrentDataSize;
  UINTN              CompressedDataSize;

  Handle = GetDebugPortHandle ();

  RemainingDataSize = Count * Width;
  while (TRUE) {
    SequenceNo = GetMailboxPointer ()->HostSequenceNo;
    if (RemainingDataSize <= DEBUG_DATA_MAXIMUM_REAL_DATA) {
      //
      // If the remaining data is less one real packet size, this is the last data packet
      //
      CurrentDataSize      = (UINT8)RemainingDataSize;
      LastPacket           = TRUE;
      DebugHeader->Command = DEBUG_COMMAND_OK;
    } else {
      //
      // Data is too larger to be sent in one packet, calculate the actual data size could
      // be sent in one Maximum data packet
      //
      CurrentDataSize      = (DEBUG_DATA_MAXIMUM_REAL_DATA / Width) * Width;
      LastPacket           = FALSE;
      DebugHeader->Command = DEBUG_COMMAND_IN_PROGRESS;
    }

    //
    // Construct the rest Debug header
    //
    DebugHeader->StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;
    DebugHeader->Length      = CurrentDataSize + sizeof (DEBUG_PACKET_HEADER);
    DebugHeader->SequenceNo  = SequenceNo;
    DebugHeader->Crc         = 0;
    CopyMemByWidth ((UINT8 *)(DebugHeader + 1), Data, CurrentDataSize / Width, Width);

    //
    // Compression/decompression support was added since revision 0.4.
    // Revision 0.3 shouldn't compress the packet.
    //
    if (PcdGet32 (PcdTransferProtocolRevision) >= DEBUG_AGENT_REVISION_04) {
      //
      // Get the compressed data size without modifying the packet.
      //
      CompressData (
        Handle,
        (UINT8 *)(DebugHeader + 1),
        CurrentDataSize,
        FALSE,
        &CompressedDataSize,
        NULL
        );
    } else {
      CompressedDataSize = CurrentDataSize;
    }

    if (CompressedDataSize < CurrentDataSize) {
      DebugHeader->Length      = (UINT8)CompressedDataSize + sizeof (DEBUG_PACKET_HEADER);
      DebugHeader->StartSymbol = DEBUG_STARTING_SYMBOL_COMPRESS;
      //
      // Compute the CRC of the packet head without modifying the packet.
      //
      DebugHeader->Crc = CalculateCrc16 ((UINT8 *)DebugHeader, sizeof (DEBUG_PACKET_HEADER), 0);
      CompressData (
        Handle,
        (UINT8 *)(DebugHeader + 1),
        CurrentDataSize,
        FALSE,
        NULL,
        &DebugHeader->Crc
        );
      //
      // Send out the packet head.
      //
      DebugPortWriteBuffer (Handle, (UINT8 *)DebugHeader, sizeof (DEBUG_PACKET_HEADER));
      //
      // Compress and send out the packet data.
      //
      CompressData (
        Handle,
        (UINT8 *)(DebugHeader + 1),
        CurrentDataSize,
        TRUE,
        NULL,
        NULL
        );
    } else {
      //
      // Calculate and fill the checksum, DebugHeader->Crc should be 0 before invoking CalculateCrc16 ()
      //
      DebugHeader->Crc = CalculateCrc16 ((UINT8 *)DebugHeader, DebugHeader->Length, 0);

      DebugAgentDataMsgPrint (DEBUG_AGENT_VERBOSE, TRUE, (UINT8 *)DebugHeader, DebugHeader->Length);

      DebugPortWriteBuffer (Handle, (UINT8 *)DebugHeader, DebugHeader->Length);
    }

    while (TRUE) {
      Status = ReceivePacket ((UINT8 *)DebugHeader, NULL, NULL, READ_PACKET_TIMEOUT, FALSE);
      if (Status == RETURN_TIMEOUT) {
        DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "TARGET: Timeout in SendDataResponsePacket()\n");
        break;
      }

      if ((DebugHeader->Command == DEBUG_COMMAND_OK) && (DebugHeader->SequenceNo == SequenceNo) && LastPacket) {
        //
        // If this is the last packet, return RETURN_SUCCESS.
        //
        return RETURN_SUCCESS;
      }

      if ((DebugHeader->Command == DEBUG_COMMAND_CONTINUE) && (DebugHeader->SequenceNo == (UINT8)(SequenceNo + 1))) {
        //
        // Calculate the rest data size
        //
        Data              += CurrentDataSize;
        RemainingDataSize -= CurrentDataSize;
        UpdateMailboxContent (GetMailboxPointer (), DEBUG_MAILBOX_HOST_SEQUENCE_NO_INDEX, DebugHeader->SequenceNo);
        break;
      }

      if (DebugHeader->SequenceNo >= SequenceNo) {
        DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "TARGET: Received one old or new command(SequenceNo is %x, last SequenceNo is %x)\n", SequenceNo, DebugHeader->SequenceNo);
        break;
      }
    }
  }
}

/**
  Send packet with response data to HOST.

  @param[in]      Data        Pointer to response data buffer.
  @param[in]      DataSize    Size of response data in byte.
  @param[in, out] DebugHeader Pointer to a buffer for creating response packet and receiving ACK packet,
                              to minimize the stack usage.

  @retval RETURN_SUCCESS      Response data was sent successfully.

**/
RETURN_STATUS
SendDataResponsePacket (
  IN UINT8                    *Data,
  IN UINT16                   DataSize,
  IN OUT DEBUG_PACKET_HEADER  *DebugHeader
  )
{
  return ReadMemoryAndSendResponsePacket (Data, DataSize, 1, DebugHeader);
}

/**
  Try to attach the HOST.

  Send init break packet to HOST:
  If no acknowledge received in specified Timeout, return RETURN_TIMEOUT.
  If received acknowledge, check the revision of HOST.
  Set Attach Flag if attach successfully.

  @param[in]  BreakCause     Break cause of this break event.
  @param[in]  Timeout        Time out value to wait for acknowledge from HOST.
                             The unit is microsecond.
  @param[out] BreakReceived  If BreakReceived is not NULL,
                             TRUE is returned if break-in symbol received.
                             FALSE is returned if break-in symbol not received.
**/
RETURN_STATUS
AttachHost (
  IN  UINT8    BreakCause,
  IN  UINTN    Timeout,
  OUT BOOLEAN  *BreakReceived
  )
{
  RETURN_STATUS      Status;
  DEBUG_PORT_HANDLE  Handle;
  BOOLEAN            IncompatibilityFlag;

  IncompatibilityFlag = FALSE;
  Handle              = GetDebugPortHandle ();

  //
  // Send init break and wait ack in Timeout
  //
  DebugPortWriteBuffer (Handle, (UINT8 *)mErrorMsgSendInitPacket, AsciiStrLen (mErrorMsgSendInitPacket));
  if (BreakCause == DEBUG_DATA_BREAK_CAUSE_SYSTEM_RESET) {
    Status = SendCommandAndWaitForAckOK (DEBUG_COMMAND_INIT_BREAK, Timeout, BreakReceived, &IncompatibilityFlag);
  } else {
    Status = SendCommandAndWaitForAckOK (DEBUG_COMMAND_ATTACH_BREAK, Timeout, BreakReceived, &IncompatibilityFlag);
  }

  if (IncompatibilityFlag) {
    //
    // If the incompatible Debug Packet received, the HOST should be running transfer protocol before PcdTransferProtocolRevision.
    // It could be UDK Debugger for Windows v1.1/v1.2 or for Linux v0.8/v1.2.
    //
    DebugPortWriteBuffer (Handle, (UINT8 *)mErrorMsgVersionAlert, AsciiStrLen (mErrorMsgVersionAlert));
    CpuDeadLoop ();
  }

  if (RETURN_ERROR (Status)) {
    DebugPortWriteBuffer (Handle, (UINT8 *)mErrorMsgConnectFail, AsciiStrLen (mErrorMsgConnectFail));
  } else {
    DebugPortWriteBuffer (Handle, (UINT8 *)mErrorMsgConnectOK, AsciiStrLen (mErrorMsgConnectOK));
    //
    // Set Attach flag
    //
    SetHostAttached (TRUE);
  }

  return Status;
}

/**
  Send Break point packet to HOST.

  Only the first breaking processor could sent BREAK_POINT packet.

  @param[in]  BreakCause     Break cause of this break event.
  @param[in]  ProcessorIndex Processor index value.
  @param[out] BreakReceived  If BreakReceived is not NULL,
                             TRUE is returned if break-in symbol received.
                             FALSE is returned if break-in symbol not received.

**/
VOID
SendBreakPacketToHost (
  IN  UINT8    BreakCause,
  IN  UINT32   ProcessorIndex,
  OUT BOOLEAN  *BreakReceived
  )
{
  UINT8              InputCharacter;
  DEBUG_PORT_HANDLE  Handle;

  Handle = GetDebugPortHandle ();

  if (IsHostAttached ()) {
    DebugAgentMsgPrint (DEBUG_AGENT_INFO, "processor[%x]:Send Break Packet to HOST.\n", ProcessorIndex);
    SendCommandAndWaitForAckOK (DEBUG_COMMAND_BREAK_POINT, READ_PACKET_TIMEOUT, BreakReceived, NULL);
  } else {
    DebugAgentMsgPrint (DEBUG_AGENT_INFO, "processor[%x]:Try to attach HOST.\n", ProcessorIndex);
    //
    // If HOST is not attached, try to attach it firstly.
    //
    //
    // Poll Attach symbols from HOST and ack OK
    //
    do {
      DebugAgentReadBuffer (Handle, &InputCharacter, 1, 0);
    } while (InputCharacter != DEBUG_STARTING_SYMBOL_ATTACH);

    SendAckPacket (DEBUG_COMMAND_OK);

    //
    // Try to attach HOST
    //
    while (AttachHost (BreakCause, 0, NULL) != RETURN_SUCCESS) {
    }
  }
}

/**
  The main function to process communication with HOST.

  It received the command packet from HOST, and sent response data packet to HOST.

  @param[in]      Vector         Vector value of exception or interrupt.
  @param[in, out] CpuContext     Pointer to saved CPU context.
  @param[in]      BreakReceived  TRUE means break-in symbol received.
                                 FALSE means break-in symbol not received.

**/
VOID
CommandCommunication (
  IN     UINTN              Vector,
  IN OUT DEBUG_CPU_CONTEXT  *CpuContext,
  IN     BOOLEAN            BreakReceived
  )
{
  RETURN_STATUS                      Status;
  UINT8                              InputPacketBuffer[DEBUG_DATA_UPPER_LIMIT + sizeof (UINT64) - 1];
  DEBUG_PACKET_HEADER                *DebugHeader;
  UINT8                              Width;
  UINT8                              Data8;
  UINT32                             Data32;
  UINT64                             Data64;
  DEBUG_DATA_READ_MEMORY             *MemoryRead;
  DEBUG_DATA_WRITE_MEMORY            *MemoryWrite;
  DEBUG_DATA_READ_IO                 *IoRead;
  DEBUG_DATA_WRITE_IO                *IoWrite;
  DEBUG_DATA_READ_REGISTER           *RegisterRead;
  DEBUG_DATA_WRITE_REGISTER          *RegisterWrite;
  UINT8                              *RegisterBuffer;
  DEBUG_DATA_READ_MSR                *MsrRegisterRead;
  DEBUG_DATA_WRITE_MSR               *MsrRegisterWrite;
  DEBUG_DATA_CPUID                   *Cpuid;
  DEBUG_DATA_RESPONSE_BREAK_CAUSE    BreakCause;
  DEBUG_DATA_RESPONSE_CPUID          CpuidResponse;
  DEBUG_DATA_SEARCH_SIGNATURE        *SearchSignature;
  DEBUG_DATA_RESPONSE_GET_EXCEPTION  Exception;
  DEBUG_DATA_RESPONSE_GET_REVISION   DebugAgentRevision;
  DEBUG_DATA_SET_VIEWPOINT           *SetViewPoint;
  BOOLEAN                            HaltDeferred;
  UINT32                             ProcessorIndex;
  DEBUG_AGENT_EXCEPTION_BUFFER       AgentExceptionBuffer;
  UINT32                             IssuedViewPoint;
  DEBUG_AGENT_MAILBOX                *Mailbox;
  UINT8                              *AlignedDataPtr;

  ProcessorIndex  = 0;
  IssuedViewPoint = 0;
  HaltDeferred    = BreakReceived;

  if (MultiProcessorDebugSupport ()) {
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
    SetDebugFlag (DEBUG_AGENT_FLAG_AGENT_IN_PROGRESS, 1);
  }

  while (TRUE) {
    if (MultiProcessorDebugSupport ()) {
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

    AcquireMpSpinLock (&mDebugMpContext.DebugPortSpinLock);

    DebugHeader = (DEBUG_PACKET_HEADER *)InputPacketBuffer;

    DebugAgentMsgPrint (DEBUG_AGENT_INFO, "TARGET: Try to get command from HOST...\n");
    Status = ReceivePacket ((UINT8 *)DebugHeader, &BreakReceived, NULL, READ_PACKET_TIMEOUT, TRUE);
    if ((Status != RETURN_SUCCESS) || !IS_REQUEST (DebugHeader)) {
      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "TARGET: Get command[%x] sequenceno[%x] returned status is [%x] \n", DebugHeader->Command, DebugHeader->SequenceNo, Status);
      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "TARGET: Get command failed or it's response packet not expected! \n");
      ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
      continue;
    }

    Mailbox = GetMailboxPointer ();
    if (DebugHeader->SequenceNo == Mailbox->HostSequenceNo) {
      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "TARGET: Receive one old command[%x] against command[%x]\n", DebugHeader->SequenceNo, Mailbox->HostSequenceNo);
      SendAckPacket (Mailbox->LastAck);
      ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
      continue;
    } else if (DebugHeader->SequenceNo == (UINT8)(Mailbox->HostSequenceNo + 1)) {
      UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_HOST_SEQUENCE_NO_INDEX, (UINT8)DebugHeader->SequenceNo);
    } else {
      DebugAgentMsgPrint (DEBUG_AGENT_WARNING, "Receive one invalid command[%x] against command[%x]\n", DebugHeader->SequenceNo, Mailbox->HostSequenceNo);
      ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
      continue;
    }

    //
    // Save CPU content before executing HOST command
    //
    UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_EXCEPTION_BUFFER_POINTER_INDEX, (UINT64)(UINTN)&AgentExceptionBuffer.JumpBuffer);
    if (SetJump (&AgentExceptionBuffer.JumpBuffer) != 0) {
      //
      // If HOST command failed, continue to wait for HOST's next command
      // If needed, agent could send exception info to HOST.
      //
      SendAckPacket (DEBUG_COMMAND_ABORT);
      ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
      continue;
    }

    DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Processor[%x]:Received one command(%x)\n", mDebugMpContext.ViewPointIndex, DebugHeader->Command);

    switch (DebugHeader->Command) {
      case DEBUG_COMMAND_HALT:
        SendAckPacket (DEBUG_COMMAND_HALT_DEFERRED);
        HaltDeferred  = TRUE;
        BreakReceived = FALSE;
        Status        = RETURN_SUCCESS;
        break;

      case DEBUG_COMMAND_RESET:
        SendAckPacket (DEBUG_COMMAND_OK);
        SendAckPacket (DEBUG_COMMAND_OK);
        SendAckPacket (DEBUG_COMMAND_OK);
        ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);

        ResetCold ();
        //
        // Assume system resets in 2 seconds, otherwise send TIMEOUT packet.
        // PCD can be used if 2 seconds isn't long enough for some platforms.
        //
        MicroSecondDelay (2000000);
        UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_HOST_SEQUENCE_NO_INDEX, Mailbox->HostSequenceNo + 1);
        SendAckPacket (DEBUG_COMMAND_TIMEOUT);
        SendAckPacket (DEBUG_COMMAND_TIMEOUT);
        SendAckPacket (DEBUG_COMMAND_TIMEOUT);
        break;

      case DEBUG_COMMAND_GO:
        CommandGo (CpuContext);
        //
        // Clear Dr0 to avoid to be recognized as IMAGE_LOAD/_UNLOAD again when hitting a breakpoint after GO
        // If HOST changed Dr0 before GO, we will not change Dr0 here
        //
        Data8 = GetBreakCause (Vector, CpuContext);
        if ((Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD) || (Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD)) {
          CpuContext->Dr0 = 0;
        }

        if (!HaltDeferred) {
          //
          // If no HALT command received when being in-active mode
          //
          if (MultiProcessorDebugSupport ()) {
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
              mDebugMpContext.ViewPointIndex  = Data32;
              mDebugMpContext.BreakAtCpuIndex = mDebugMpContext.ViewPointIndex;
              SetCpuBreakFlagByIndex (mDebugMpContext.ViewPointIndex, FALSE);
              //
              // Send break packet to HOST to let HOST break again
              //
              SendBreakPacketToHost (DEBUG_DATA_BREAK_CAUSE_UNKNOWN, mDebugMpContext.BreakAtCpuIndex, &BreakReceived);
              //
              // Continue to run into loop to read command packet from HOST
              //
              ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
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
            mDebugMpContext.BreakAtCpuIndex = (UINT32)(-1);
            SetCpuRunningFlag (FALSE);
          }

          //
          // Send OK packet to HOST to finish this go command
          //
          SendAckPacket (DEBUG_COMMAND_OK);

          ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);

          if (!IsHostAttached ()) {
            UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_SEQUENCE_NO_INDEX, 0);
            UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_HOST_SEQUENCE_NO_INDEX, 0);
          }

          return;
        } else {
          //
          // If received HALT command, need to defer the GO command
          //
          SendAckPacket (DEBUG_COMMAND_HALT_PROCESSED);
          HaltDeferred = FALSE;

          Vector = DEBUG_TIMER_VECTOR;
        }

        break;

      case DEBUG_COMMAND_BREAK_CAUSE:
        BreakCause.StopAddress = CpuContext->Eip;
        if (MultiProcessorDebugSupport () && (ProcessorIndex != mDebugMpContext.BreakAtCpuIndex)) {
          BreakCause.Cause = GetBreakCause (DEBUG_TIMER_VECTOR, CpuContext);
        } else {
          BreakCause.Cause = GetBreakCause (Vector, CpuContext);
        }

        SendDataResponsePacket ((UINT8 *)&BreakCause, (UINT16)sizeof (DEBUG_DATA_RESPONSE_BREAK_CAUSE), DebugHeader);
        break;

      case DEBUG_COMMAND_SET_HW_BREAKPOINT:
        SetDebugRegister (CpuContext, (DEBUG_DATA_SET_HW_BREAKPOINT *)(DebugHeader + 1));
        SendAckPacket (DEBUG_COMMAND_OK);
        break;

      case DEBUG_COMMAND_CLEAR_HW_BREAKPOINT:
        ClearDebugRegister (CpuContext, (DEBUG_DATA_CLEAR_HW_BREAKPOINT *)(DebugHeader + 1));
        SendAckPacket (DEBUG_COMMAND_OK);
        break;

      case DEBUG_COMMAND_SINGLE_STEPPING:
        CommandStepping (CpuContext);
        //
        // Clear Dr0 to avoid to be recognized as IMAGE_LOAD/_UNLOAD again when hitting a breakpoint after GO
        // If HOST changed Dr0 before GO, we will not change Dr0 here
        //
        Data8 = GetBreakCause (Vector, CpuContext);
        if ((Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD) || (Data8 == DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD)) {
          CpuContext->Dr0 = 0;
        }

        mDebugMpContext.BreakAtCpuIndex = (UINT32)(-1);
        ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
        //
        // Executing stepping command directly without sending ACK packet,
        // ACK packet will be sent after stepping done.
        //
        return;

      case DEBUG_COMMAND_SET_SW_BREAKPOINT:
        Data64                  = (UINTN)(((DEBUG_DATA_SET_SW_BREAKPOINT *)(DebugHeader + 1))->Address);
        Data8                   = *(UINT8 *)(UINTN)Data64;
        *(UINT8 *)(UINTN)Data64 = DEBUG_SW_BREAKPOINT_SYMBOL;
        Status                  = SendDataResponsePacket ((UINT8 *)&Data8, (UINT16)sizeof (UINT8), DebugHeader);
        break;

      case DEBUG_COMMAND_READ_MEMORY:
        MemoryRead = (DEBUG_DATA_READ_MEMORY *)(DebugHeader + 1);
        Status     = ReadMemoryAndSendResponsePacket ((UINT8 *)(UINTN)MemoryRead->Address, MemoryRead->Count, MemoryRead->Width, DebugHeader);
        break;

      case DEBUG_COMMAND_WRITE_MEMORY:
        MemoryWrite = (DEBUG_DATA_WRITE_MEMORY *)(DebugHeader + 1);
        //
        // Copy data into one memory with 8-byte alignment address
        //
        AlignedDataPtr = ALIGN_POINTER ((UINT8 *)&MemoryWrite->Data, sizeof (UINT64));
        if (AlignedDataPtr != (UINT8 *)&MemoryWrite->Data) {
          CopyMem (AlignedDataPtr, (UINT8 *)&MemoryWrite->Data, MemoryWrite->Count * MemoryWrite->Width);
        }

        CopyMemByWidth ((UINT8 *)(UINTN)MemoryWrite->Address, AlignedDataPtr, MemoryWrite->Count, MemoryWrite->Width);
        SendAckPacket (DEBUG_COMMAND_OK);
        break;

      case DEBUG_COMMAND_READ_IO:
        IoRead = (DEBUG_DATA_READ_IO *)(DebugHeader + 1);
        switch (IoRead->Width) {
          case 1:
            Data64 = IoRead8 ((UINTN)IoRead->Port);
            break;
          case 2:
            Data64 = IoRead16 ((UINTN)IoRead->Port);
            break;
          case 4:
            Data64 = IoRead32 ((UINTN)IoRead->Port);
            break;
          case 8:
            Data64 = IoRead64 ((UINTN)IoRead->Port);
            break;
          default:
            Data64 = (UINT64)-1;
        }

        Status = SendDataResponsePacket ((UINT8 *)&Data64, IoRead->Width, DebugHeader);
        break;

      case DEBUG_COMMAND_WRITE_IO:
        IoWrite = (DEBUG_DATA_WRITE_IO *)(DebugHeader + 1);
        switch (IoWrite->Width) {
          case 1:
            Data64 = IoWrite8 ((UINTN)IoWrite->Port, *(UINT8 *)&IoWrite->Data);
            break;
          case 2:
            Data64 = IoWrite16 ((UINTN)IoWrite->Port, *(UINT16 *)&IoWrite->Data);
            break;
          case 4:
            Data64 = IoWrite32 ((UINTN)IoWrite->Port, *(UINT32 *)&IoWrite->Data);
            break;
          case 8:
            Data64 = IoWrite64 ((UINTN)IoWrite->Port, *(UINT64 *)&IoWrite->Data);
            break;
          default:
            Data64 = (UINT64)-1;
        }

        SendAckPacket (DEBUG_COMMAND_OK);
        break;

      case DEBUG_COMMAND_READ_ALL_REGISTERS:
        Status = SendDataResponsePacket ((UINT8 *)CpuContext, sizeof (*CpuContext), DebugHeader);
        break;

      case DEBUG_COMMAND_READ_REGISTER:
        RegisterRead = (DEBUG_DATA_READ_REGISTER *)(DebugHeader + 1);

        if (RegisterRead->Index <= SOFT_DEBUGGER_REGISTER_MAX) {
          RegisterBuffer = ArchReadRegisterBuffer (CpuContext, RegisterRead->Index, &Width);
          Status         = SendDataResponsePacket (RegisterBuffer, Width, DebugHeader);
        } else {
          Status = RETURN_UNSUPPORTED;
        }

        break;

      case DEBUG_COMMAND_WRITE_REGISTER:
        RegisterWrite = (DEBUG_DATA_WRITE_REGISTER *)(DebugHeader + 1);
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
        Data8  = DEBUG_ARCH_SYMBOL;
        Status = SendDataResponsePacket ((UINT8 *)&Data8, (UINT16)sizeof (UINT8), DebugHeader);
        break;

      case DEBUG_COMMAND_READ_MSR:
        MsrRegisterRead = (DEBUG_DATA_READ_MSR *)(DebugHeader + 1);
        Data64          = AsmReadMsr64 (MsrRegisterRead->Index);
        Status          = SendDataResponsePacket ((UINT8 *)&Data64, (UINT16)sizeof (UINT64), DebugHeader);
        break;

      case DEBUG_COMMAND_WRITE_MSR:
        MsrRegisterWrite = (DEBUG_DATA_WRITE_MSR *)(DebugHeader + 1);
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
        DebugAgentRevision.Revision     = PcdGet32 (PcdTransferProtocolRevision);
        DebugAgentRevision.Capabilities = DEBUG_AGENT_CAPABILITIES;
        Status                          = SendDataResponsePacket ((UINT8 *)&DebugAgentRevision, (UINT16)sizeof (DEBUG_DATA_RESPONSE_GET_REVISION), DebugHeader);
        break;

      case DEBUG_COMMAND_GET_EXCEPTION:
        Exception.ExceptionNum  = (UINT8)Vector;
        Exception.ExceptionData = (UINT32)CpuContext->ExceptionData;
        Status                  = SendDataResponsePacket ((UINT8 *)&Exception, (UINT16)sizeof (DEBUG_DATA_RESPONSE_GET_EXCEPTION), DebugHeader);
        break;

      case DEBUG_COMMAND_SET_VIEWPOINT:
        SetViewPoint = (DEBUG_DATA_SET_VIEWPOINT *)(DebugHeader + 1);
        if (MultiProcessorDebugSupport ()) {
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
        SendDataResponsePacket ((UINT8 *)&Data32, (UINT16)sizeof (UINT32), DebugHeader);
        break;

      case DEBUG_COMMAND_MEMORY_READY:
        Data8 = (UINT8)GetDebugFlag (DEBUG_AGENT_FLAG_MEMORY_READY);
        SendDataResponsePacket (&Data8, (UINT16)sizeof (UINT8), DebugHeader);
        break;

      case DEBUG_COMMAND_DETACH:
        SetHostAttached (FALSE);
        SendAckPacket (DEBUG_COMMAND_OK);
        break;

      case DEBUG_COMMAND_CPUID:
        Cpuid = (DEBUG_DATA_CPUID *)(DebugHeader + 1);
        AsmCpuidEx (
          Cpuid->Eax,
          Cpuid->Ecx,
          &CpuidResponse.Eax,
          &CpuidResponse.Ebx,
          &CpuidResponse.Ecx,
          &CpuidResponse.Edx
          );
        SendDataResponsePacket ((UINT8 *)&CpuidResponse, (UINT16)sizeof (CpuidResponse), DebugHeader);
        break;

      case DEBUG_COMMAND_SEARCH_SIGNATURE:
        SearchSignature = (DEBUG_DATA_SEARCH_SIGNATURE *)(DebugHeader + 1);
        if ((SearchSignature->Alignment != 0) &&
            (SearchSignature->Alignment == GetPowerOfTwo32 (SearchSignature->Alignment))
            )
        {
          if (SearchSignature->Positive) {
            for (
                 Data64 = ALIGN_VALUE ((UINTN)SearchSignature->Start, SearchSignature->Alignment);
                 Data64 <= SearchSignature->Start + SearchSignature->Count - SearchSignature->DataLength;
                 Data64 += SearchSignature->Alignment
                 )
            {
              if (CompareMem ((VOID *)(UINTN)Data64, &SearchSignature->Data, SearchSignature->DataLength) == 0) {
                break;
              }
            }

            if (Data64 > SearchSignature->Start + SearchSignature->Count - SearchSignature->DataLength) {
              Data64 = (UINT64)-1;
            }
          } else {
            for (
                 Data64 = ALIGN_VALUE ((UINTN)SearchSignature->Start - SearchSignature->Alignment, SearchSignature->Alignment);
                 Data64 >= SearchSignature->Start - SearchSignature->Count;
                 Data64 -= SearchSignature->Alignment
                 )
            {
              if (CompareMem ((VOID *)(UINTN)Data64, &SearchSignature->Data, SearchSignature->DataLength) == 0) {
                break;
              }
            }

            if (Data64 < SearchSignature->Start - SearchSignature->Count) {
              Data64 = (UINT64)-1;
            }
          }

          SendDataResponsePacket ((UINT8 *)&Data64, (UINT16)sizeof (Data64), DebugHeader);
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

    ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
    CpuPause ();
  }
}

/**
  C function called in interrupt handler.

  @param[in] Vector      Vector value of exception or interrupt.
  @param[in] CpuContext  Pointer to save CPU context.

**/
VOID
EFIAPI
InterruptProcess (
  IN UINT32             Vector,
  IN DEBUG_CPU_CONTEXT  *CpuContext
  )
{
  UINT8                         InputCharacter;
  UINT8                         BreakCause;
  UINTN                         SavedEip;
  BOOLEAN                       BreakReceived;
  UINT32                        ProcessorIndex;
  UINT32                        CurrentDebugTimerInitCount;
  DEBUG_PORT_HANDLE             Handle;
  UINT8                         Data8;
  UINT8                         *Al;
  UINT32                        IssuedViewPoint;
  DEBUG_AGENT_EXCEPTION_BUFFER  *ExceptionBuffer;

  InputCharacter  = 0;
  ProcessorIndex  = 0;
  IssuedViewPoint = 0;
  BreakReceived   = FALSE;

  if (mSkipBreakpoint) {
    //
    // If Skip Breakpoint flag is set, means communication is disturbed by hardware SMI, we need to ignore the break points in SMM
    //
    if ((Vector == DEBUG_INT1_VECTOR) || (Vector == DEBUG_INT3_VECTOR)) {
      DebugPortWriteBuffer (GetDebugPortHandle (), (UINT8 *)mWarningMsgIngoreBreakpoint, AsciiStrLen (mWarningMsgIngoreBreakpoint));
      return;
    }
  }

  if (MultiProcessorDebugSupport ()) {
    ProcessorIndex = GetProcessorIndex ();
    //
    // If this processor has already halted before, need to check it later
    //
    if (IsCpuStopped (ProcessorIndex)) {
      IssuedViewPoint = ProcessorIndex;
    }
  }

  if ((IssuedViewPoint == ProcessorIndex) && (GetDebugFlag (DEBUG_AGENT_FLAG_STEPPING) != 1)) {
    //
    // Check if this exception is issued by Debug Agent itself
    // If yes, fill the debug agent exception buffer and LongJump() back to
    // the saved CPU content in CommandCommunication()
    // If exception is issued when executing Stepping, will be handled in
    // exception handle procedure.
    //
    if (GetDebugFlag (DEBUG_AGENT_FLAG_AGENT_IN_PROGRESS) == 1) {
      DebugAgentMsgPrint (
        DEBUG_AGENT_ERROR,
        "Debug agent meet one Exception, ExceptionNum is %d, EIP = 0x%x.\n",
        Vector,
        (UINTN)CpuContext->Eip
        );
      ExceptionBuffer                                 = (DEBUG_AGENT_EXCEPTION_BUFFER *)(UINTN)GetMailboxPointer ()->ExceptionBufferPointer;
      ExceptionBuffer->ExceptionContent.ExceptionNum  = (UINT8)Vector;
      ExceptionBuffer->ExceptionContent.ExceptionData = (UINT32)CpuContext->ExceptionData;
      LongJump ((BASE_LIBRARY_JUMP_BUFFER *)(UINTN)(ExceptionBuffer), 1);
    }
  }

  if (MultiProcessorDebugSupport ()) {
    //
    // If RUN command is executing, wait for it done.
    //
    while (mDebugMpContext.RunCommandSet) {
      CpuPause ();
    }
  }

  Handle     = GetDebugPortHandle ();
  BreakCause = GetBreakCause (Vector, CpuContext);
  switch (Vector) {
    case DEBUG_INT1_VECTOR:
    case DEBUG_INT3_VECTOR:
      switch (BreakCause) {
        case DEBUG_DATA_BREAK_CAUSE_SYSTEM_RESET:
          if (AttachHost (BreakCause, READ_PACKET_TIMEOUT, &BreakReceived) != RETURN_SUCCESS) {
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
          if (MultiProcessorDebugSupport ()) {
            mDebugMpContext.BreakAtCpuIndex = ProcessorIndex;
          }

          //
          // Clear Stepping Flag and restore EFLAGS.IF
          //
          CommandSteppingCleanup (CpuContext);
          SendAckPacket (DEBUG_COMMAND_OK);
          CommandCommunication (Vector, CpuContext, BreakReceived);
          break;

        case DEBUG_DATA_BREAK_CAUSE_MEMORY_READY:
          //
          // Memory is ready
          //
          SendCommandAndWaitForAckOK (DEBUG_COMMAND_MEMORY_READY, READ_PACKET_TIMEOUT, &BreakReceived, NULL);
          CommandCommunication (Vector, CpuContext, BreakReceived);
          break;

        case DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD:
        case DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD:
          //
          // Set AL to DEBUG_AGENT_IMAGE_CONTINUE
          //
          Al  = ArchReadRegisterBuffer (CpuContext, SOFT_DEBUGGER_REGISTER_AX, &Data8);
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
          AcquireMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
          //
          // Only the first breaking processor could send BREAK_POINT to HOST
          //
          if (IsFirstBreakProcessor (ProcessorIndex)) {
            SendBreakPacketToHost (BreakCause, ProcessorIndex, &BreakReceived);
          }

          ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);

          if (Vector == DEBUG_INT3_VECTOR) {
            //
            // go back address located "0xCC"
            //
            CpuContext->Eip--;
            SavedEip = CpuContext->Eip;
            CommandCommunication (Vector, CpuContext, BreakReceived);
            if ((SavedEip == CpuContext->Eip) &&
                (*(UINT8 *)(UINTN)CpuContext->Eip == DEBUG_SW_BREAKPOINT_SYMBOL))
            {
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

      AcquireMpSpinLock (&mDebugMpContext.DebugPortSpinLock);

      if (MultiProcessorDebugSupport ()) {
        if (DebugAgentIsBsp (ProcessorIndex)) {
          //
          // If current processor is BSP, check Apic timer's init count if changed,
          // it may be re-written when switching BSP.
          // If it changed, re-initialize debug timer
          //
          CurrentDebugTimerInitCount = GetApicTimerInitCount ();
          if (mDebugMpContext.DebugTimerInitCount != CurrentDebugTimerInitCount) {
            InitializeDebugTimer (NULL, FALSE);
            SaveAndSetDebugTimerInterrupt (TRUE);
          }
        }

        if (!DebugAgentIsBsp (ProcessorIndex) || mDebugMpContext.IpiSentByAp) {
          ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
          //
          // If current processor is not BSP or this is one IPI sent by AP
          //
          if (mDebugMpContext.BreakAtCpuIndex != (UINT32)(-1)) {
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
            (IsHostAttached () && (InputCharacter == DEBUG_COMMAND_HALT)) ||
            (IsHostAttached () && (InputCharacter == DEBUG_COMMAND_GO))
            )
        {
          DebugAgentMsgPrint (DEBUG_AGENT_VERBOSE, "Received data [%02x]\n", InputCharacter);
          //
          // Ack OK for break-in symbol
          //
          SendAckPacket (DEBUG_COMMAND_OK);

          //
          // If receive GO command in Debug Timer, means HOST may lost ACK packet before.
          //
          if (InputCharacter == DEBUG_COMMAND_GO) {
            break;
          }

          if (!IsHostAttached ()) {
            //
            // Try to attach HOST, if no ack received after 200ms, return
            //
            if (AttachHost (BreakCause, READ_PACKET_TIMEOUT, &BreakReceived) != RETURN_SUCCESS) {
              break;
            }
          }

          if (MultiProcessorDebugSupport ()) {
            if (FindNextPendingBreakCpu () != -1) {
              SetCpuBreakFlagByIndex (ProcessorIndex, TRUE);
            } else {
              HaltOtherProcessors (ProcessorIndex);
            }
          }

          ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
          CommandCommunication (Vector, CpuContext, BreakReceived);
          AcquireMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
          break;
        }
      }

      //
      // Clear EOI before exiting interrupt process routine.
      //
      SendApicEoi ();

      ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);

      break;

    default:
      if (Vector <= DEBUG_EXCEPT_SIMD) {
        DebugAgentMsgPrint (
          DEBUG_AGENT_ERROR,
          "Exception happened, ExceptionNum is %d, EIP = 0x%x.\n",
          Vector,
          (UINTN)CpuContext->Eip
          );
        if (BreakCause == DEBUG_DATA_BREAK_CAUSE_STEPPING) {
          //
          // If exception happened when executing Stepping, send Ack package.
          // HOST consider Stepping command was finished.
          //
          if (MultiProcessorDebugSupport ()) {
            mDebugMpContext.BreakAtCpuIndex = ProcessorIndex;
          }

          //
          // Clear Stepping flag and restore EFLAGS.IF
          //
          CommandSteppingCleanup (CpuContext);
          SendAckPacket (DEBUG_COMMAND_OK);
        } else {
          //
          // Exception occurs, send Break packet to HOST
          //
          AcquireMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
          //
          // Only the first breaking processor could send BREAK_POINT to HOST
          //
          if (IsFirstBreakProcessor (ProcessorIndex)) {
            SendBreakPacketToHost (BreakCause, ProcessorIndex, &BreakReceived);
          }

          ReleaseMpSpinLock (&mDebugMpContext.DebugPortSpinLock);
        }

        CommandCommunication (Vector, CpuContext, BreakReceived);
      }

      break;
  }

  if (MultiProcessorDebugSupport ()) {
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

  if ((IssuedViewPoint == ProcessorIndex) && (GetDebugFlag (DEBUG_AGENT_FLAG_STEPPING) != 1)) {
    //
    // If the command is not stepping, clean up AgentInProgress flag
    //
    SetDebugFlag (DEBUG_AGENT_FLAG_AGENT_IN_PROGRESS, 0);
  }

  return;
}
