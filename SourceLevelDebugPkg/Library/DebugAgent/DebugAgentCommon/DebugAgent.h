/** @file
  Command header of for Debug Agent library instance.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DEBUG_AGENT_H_
#define _DEBUG_AGENT_H_

#include <Register/LocalApic.h>
#include <Guid/DebugAgentGuid.h>
#include <Guid/VectorHandoffTable.h>
#include <Ppi/VectorHandoffInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/DebugCommunicationLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/LocalApicLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/PrintLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffExtraActionLib.h>

#include <TransferProtocol.h>
#include <ImageDebugSupport.h>

#include "DebugMp.h"
#include "DebugTimer.h"
#include "ArchDebugSupport.h"
#include "DebugException.h"

//
// These macros may be already defined in DebugAgentLib.h
//
#define DEBUG_AGENT_INIT_PEI                     9
#define DEBUG_AGENT_INIT_DXE_LOAD               10
#define DEBUG_AGENT_INIT_DXE_UNLOAD             11
#define DEBUG_AGENT_INIT_THUNK_PEI_IA32TOX64    12

#define DEBUG_INT1_VECTOR               DEBUG_EXCEPT_DEBUG
#define DEBUG_INT3_VECTOR               DEBUG_EXCEPT_BREAKPOINT
#define DEBUG_TIMER_VECTOR              32
#define DEBUG_MAILBOX_VECTOR            33

//
//  Timeout value for reading packet (unit is microsecond)
//
#define READ_PACKET_TIMEOUT     (500 * 1000)
#define DEBUG_TIMER_INTERVAL    (100 * 1000)

#define SOFT_INTERRUPT_SIGNATURE    SIGNATURE_32('S','O','F','T')
#define SYSTEM_RESET_SIGNATURE      SIGNATURE_32('S','Y','S','R')
#define MEMORY_READY_SIGNATURE      SIGNATURE_32('M','E','M','R')

extern UINTN  Exception0Handle;
extern UINTN  TimerInterruptHandle;
extern UINT32 ExceptionStubHeaderSize;
extern BOOLEAN mSkipBreakpoint;
extern EFI_VECTOR_HANDOFF_INFO mVectorHandoffInfoDebugAgent[];
extern UINTN                   mVectorHandoffInfoCount;

//
// CPU exception information issued by debug agent
//
typedef struct {
  //
  // This field is used to save CPU content before executing HOST command
  //
  BASE_LIBRARY_JUMP_BUFFER            JumpBuffer;
  //
  // This field returns the exception information issued by the HOST command
  //
  DEBUG_DATA_RESPONSE_GET_EXCEPTION   ExceptionContent;
} DEBUG_AGENT_EXCEPTION_BUFFER;

#define DEBUG_AGENT_FLAG_HOST_ATTACHED         BIT0
#define DEBUG_AGENT_FLAG_AGENT_IN_PROGRESS     BIT1
#define DEBUG_AGENT_FLAG_MEMORY_READY          BIT2
#define DEBUG_AGENT_FLAG_STEPPING              BIT3
#define DEBUG_AGENT_FLAG_CHECK_MAILBOX_IN_HOB  BIT4
#define DEBUG_AGENT_FLAG_INIT_ARCH             BIT5|BIT6
#define DEBUG_AGENT_FLAG_INTERRUPT_FLAG        BIT7
#define DEBUG_AGENT_FLAG_BREAK_ON_NEXT_SMI     BIT32
#define DEBUG_AGENT_FLAG_PRINT_ERROR_LEVEL     (BIT33|BIT34|BIT35|BIT36)
#define DEBUG_AGENT_FLAG_BREAK_BOOT_SCRIPT     BIT37

#define DEBUG_MAILBOX_DEBUG_FLAG_INDEX                1
#define DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX         2
#define DEBUG_MAILBOX_EXCEPTION_BUFFER_POINTER_INDEX  3
#define DEBUG_MAILBOX_LAST_ACK                        4
#define DEBUG_MAILBOX_SEQUENCE_NO_INDEX               5
#define DEBUG_MAILBOX_HOST_SEQUENCE_NO_INDEX          6
#define DEBUG_MAILBOX_DEBUG_TIMER_FREQUENCY           7

#pragma pack(1)
typedef union {
  struct {
    //
    // Lower 32 bits to store the status of DebugAgent
    //
    UINT32  HostAttached      : 1;   // 1: HOST is attached
    UINT32  AgentInProgress   : 1;   // 1: Debug Agent is communicating with HOST
    UINT32  MemoryReady       : 1;   // 1: Memory is ready
    UINT32  SteppingFlag      : 1;   // 1: Agent is running stepping command
    UINT32  CheckMailboxInHob : 1;   // 1: Need to check mailbox saved in HOB
    UINT32  InitArch          : 2;   // value of DEBUG_DATA_RESPONSE_ARCH_MODE
    UINT32  InterruptFlag     : 1;   // 1: EFLAGS.IF is set
    UINT32  Reserved1         : 24;
    //
    // Higher 32bits to control the behavior of DebugAgent
    //
    UINT32  BreakOnNextSmi    : 1;   // 1: Break on next SMI
    UINT32  PrintErrorLevel   : 4;   // Bitmask of print error level for debug message
    UINT32  BreakOnBootScript : 1;   // 1: Break before executing boot script 
    UINT32  Reserved2         : 26;
  } Bits;
  UINT64  Uint64;
} DEBUG_AGENT_FLAG;

typedef struct {
  DEBUG_AGENT_FLAG           DebugFlag;
  UINT64                     DebugPortHandle;
  //
  // Pointer to DEBUG_AGENT_EXCEPTION_BUFFER
  //
  UINT64                     ExceptionBufferPointer;
  UINT8                      LastAck;      // The last ack packet type
  UINT8                      SequenceNo;
  UINT8                      HostSequenceNo;
  UINT32                     DebugTimerFrequency;
  UINT8                      CheckSum;     // Mailbox checksum
  UINT8                      ToBeCheckSum; // To be Mailbox checksum at the next
} DEBUG_AGENT_MAILBOX;
#pragma pack()

///
/// Byte packed structure for an IA-32 Interrupt Gate Descriptor.
///
typedef union {
  struct {
    UINT32  OffsetLow:16;   ///< Offset bits 15..0.
    UINT32  Selector:16;    ///< Selector.
    UINT32  Reserved_0:8;   ///< Reserved.
    UINT32  GateType:8;     ///< Gate Type.  See #defines above.
    UINT32  OffsetHigh:16;  ///< Offset bits 31..16.
  } Bits;
  UINT64  Uint64;
} IA32_IDT_ENTRY;


typedef union {
  struct {
    UINT32  LimitLow    : 16;
    UINT32  BaseLow     : 16;
    UINT32  BaseMid     : 8;
    UINT32  Type        : 4;
    UINT32  System      : 1;
    UINT32  Dpl         : 2;
    UINT32  Present     : 1;
    UINT32  LimitHigh   : 4;
    UINT32  Software    : 1;
    UINT32  Reserved    : 1;
    UINT32  DefaultSize : 1;
    UINT32  Granularity : 1;
    UINT32  BaseHigh    : 8;
  } Bits;
  UINT64  Uint64;
} IA32_GDT;

/**
  Initialize IDT entries to support source level debug.

**/
VOID
InitializeDebugIdt (
  VOID
  );

/**
  Read register value from saved CPU context.

  @param[in] CpuContext         Pointer to saved CPU context.
  @param[in] Index              Register index value.
  @param[in] Width              Data width to read.

  @return The address of register value.

**/
UINT8 *
ArchReadRegisterBuffer (
  IN DEBUG_CPU_CONTEXT               *CpuContext,
  IN UINT8                           Index,
  IN UINT8                           *Width
  );

/**
  Send packet with response data to HOST.

  @param[in]      Data        Pointer to response data buffer.
  @param[in]      DataSize    Size of response data in byte.
  @param[in, out] DebugHeader Pointer to a buffer for creating response packet and receiving ACK packet,
                              to minimize the stack usage.

  @retval RETURN_SUCCESS      Response data was sent successfully.
  @retval RETURN_DEVICE_ERROR Cannot receive DEBUG_COMMAND_OK from HOST.

**/
RETURN_STATUS
SendDataResponsePacket (
  IN UINT8                   *Data,
  IN UINT16                  DataSize,
  IN OUT DEBUG_PACKET_HEADER *DebugHeader
  );

/**
  Check if HOST is attached based on Mailbox.

  @retval TRUE        HOST is attached.
  @retval FALSE       HOST is not attached.

**/
BOOLEAN
IsHostAttached (
  VOID
  );

/**
  Get Debug Agent Mailbox pointer.

  @return Mailbox pointer.

**/
DEBUG_AGENT_MAILBOX *
GetMailboxPointer (
  VOID
  );

/**
  Get debug port handle.

  @return Debug port handle.

**/
DEBUG_PORT_HANDLE
GetDebugPortHandle (
  VOID
  );

/**
  Read the Attach/Break-in symbols from the debug port.

  @param[in]  Handle         Pointer to Debug Port handle.
  @param[out] BreakSymbol    Returned break symbol.

  @retval EFI_SUCCESS        Read the symbol in BreakSymbol.
  @retval EFI_NOT_FOUND      No read the break symbol.

**/
EFI_STATUS
DebugReadBreakSymbol (
  IN  DEBUG_PORT_HANDLE      Handle,
  OUT UINT8                  *BreakSymbol
  );

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
  );

/**
  Trigger one software interrupt to debug agent to handle it.

  @param[in] Signature       Software interrupt signature.

**/
VOID
TriggerSoftInterrupt (
  IN UINT32                 Signature
  );

/**
  Check if debug agent support multi-processor.

  @retval TRUE    Multi-processor is supported.
  @retval FALSE   Multi-processor is not supported.

**/
BOOLEAN
MultiProcessorDebugSupport (
  VOID
  );

/**
  Find and report module image info to HOST.
  
  @param[in] AlignSize      Image aligned size.
  
**/
VOID 
FindAndReportModuleImageInfo (
  IN UINTN          AlignSize                   
  );

/**
  Read IDT entry to check if IDT entries are setup by Debug Agent.

  @retval  TRUE     IDT entries were setup by Debug Agent.
  @retval  FALSE    IDT entries were not setup by Debug Agent.

**/
BOOLEAN 
IsDebugAgentInitialzed (
  VOID
  );

/**
  Calculate Mailbox checksum and update the checksum field.

  @param[in]  Mailbox  Debug Agent Mailbox pointer.

**/
VOID
UpdateMailboxChecksum (
  IN DEBUG_AGENT_MAILBOX    *Mailbox
  );

/**
  Verify Mailbox checksum.

  If checksum error, print debug message and run init dead loop.

  @param[in]  Mailbox  Debug Agent Mailbox pointer.

**/
VOID 
VerifyMailboxChecksum (
  IN DEBUG_AGENT_MAILBOX    *Mailbox
  );

/**
  Set debug flag in mailbox.

  @param[in]  FlagMask      Debug flag mask value.
  @param[in]  FlagValue     Debug flag value.

**/
VOID 
SetDebugFlag (
  IN UINT64                 FlagMask,
  IN UINT32                 FlagValue                          
  );

/**
  Get debug flag in mailbox.

  @param[in]  FlagMask      Debug flag mask value.
  
  @return Debug flag value.

**/
UINT32
GetDebugFlag (
  IN UINT64                 FlagMask
  );

/**
  Update Mailbox content by index.

  @param[in]  Mailbox  Debug Agent Mailbox pointer.
  @param[in]  Index    Mailbox content index.
  @param[in]  Value    Value to be set into mail box.
  
**/
VOID
UpdateMailboxContent ( 
  IN DEBUG_AGENT_MAILBOX    *Mailbox,
  IN UINTN                  Index,
  IN UINT64                 Value
  );

/**
  Retrieve exception handler from IDT table by ExceptionNum.

  @param[in]  ExceptionNum    Exception number
 
  @return Exception handler

**/
VOID *
GetExceptionHandlerInIdtEntry (
  IN UINTN             ExceptionNum
  );

/**
  Set exception handler in IDT table by ExceptionNum.

  @param[in]  ExceptionNum      Exception number
  @param[in]  ExceptionHandler  Exception Handler to be set 

**/
VOID
SetExceptionHandlerInIdtEntry (
  IN UINTN             ExceptionNum,
  IN VOID              *ExceptionHandler
  );

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
  );

/**
  Read remaing debug packet except for the start symbol

  @param[in]      Handle        Pointer to Debug Port handle.
  @param[in, out] DebugHeader   Debug header buffer including start symbol.

  @retval EFI_SUCCESS        Read the symbol in BreakSymbol.
  @retval EFI_CRC_ERROR      CRC check fail.
  @retval EFI_TIMEOUT        Timeout occurs when reading debug packet.

**/
EFI_STATUS
ReadRemainingBreakPacket (
  IN     DEBUG_PORT_HANDLE      Handle,
  IN OUT DEBUG_PACKET_HEADER    *DebugHeader
  );

/**
  Read data from debug channel and save the data in buffer.

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
  IN     DEBUG_PORT_HANDLE     Handle,
  IN OUT UINT8                 *Buffer,
  IN     UINTN                 NumberOfBytes,
  IN     UINTN                 Timeout
  );

#endif

