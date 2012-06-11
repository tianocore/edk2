/** @file
  Command header of for Debug Agent library instance.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
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

#include <TransferProtocol.h>
#include <ImageDebugSupport.h>

#include "DebugMp.h"
#include "DebugTimer.h"
#include "ArchDebugSupport.h"

#define DEBUG_INT1_VECTOR               1
#define DEBUG_INT3_VECTOR               3
#define DEBUG_TIMER_VECTOR              32
#define DEBUG_MAILBOX_VECTOR            33

#define SOFT_INTERRUPT_SIGNATURE    SIGNATURE_32('S','O','F','T')
#define SYSTEM_RESET_SIGNATURE      SIGNATURE_32('S','Y','S','R')
#define MEMORY_READY_SIGNATURE      SIGNATURE_32('M','E','M','R')

extern UINTN  Exception0Handle;
extern UINTN  TimerInterruptHandle;
extern UINT16 ExceptionStubHeaderSize;

//
// CPU exception information issued by debug agent
//
typedef struct {
  //
  // This field is used to save CPU content before executing HOST command
  //
  BASE_LIBRARY_JUMP_BUFFER            JumpBuffer;
  //
  // This filed returens the exception information issued by HOST command
  //
  DEBUG_DATA_RESPONSE_GET_EXCEPTION   ExceptionContent;
} DEBUG_AGENT_EXCEPTION_BUFFER;

#pragma pack(1)
typedef struct {
  //
  // Lower 32 bits to store the status of DebugAgent
  //
  UINT32  HostAttached    : 1;   // 1: HOST is attached
  UINT32  AgentInProgress : 1;   // 1: Debug Agent is communicating with HOST
  UINT32  MemoryReady     : 1;   // 1: Memory is ready
  UINT32  SteppingFlag    : 1;   // 1: Agent is running stepping command
  UINT32  Reserved1       : 28;

  //
  // Higher 32bits to control the behavior of DebugAgent
  //
  UINT32  BreakOnNextSmi  : 1;   // 1: Break on next SMI
  UINT32  PrintErrorLevel : 8;   // Bitmask of print error level for debug message
  UINT32  Reserved2       : 23;
} DEBUG_AGENT_FLAG;

typedef struct {
  DEBUG_AGENT_FLAG           DebugFlag;
  UINT64                     DebugPortHandle;
  //
  // Pointer to DEBUG_AGENT_EXCEPTION_BUFFER
  //
  UINT64                     ExceptionBufferPointer;
} DEBUG_AGENT_MAILBOX;
#pragma pack()

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
  Caller provided function to be invoked at the end of DebugPortInitialize().

  Refer to the descrption for DebugPortInitialize() for more details.

  @param[in] Context           The first input argument of DebugPortInitialize().
  @param[in] DebugPortHandle   Debug port handle created by Debug Communication Libary.

**/
VOID
EFIAPI
InitializeDebugAgentPhase2 (
  IN VOID                  *Context,
  IN DEBUG_PORT_HANDLE     DebugPortHandle
  );

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

  @param[in] Data        Pointer to response data buffer.
  @param[in] DataSize    Size of response data in byte.

  @retval RETURN_SUCCESS      Response data was sent successfully.
  @retval RETURN_DEVICE_ERROR Cannot receive DEBUG_COMMAND_OK from HOST.

**/
RETURN_STATUS
SendDataResponsePacket (
  IN UINT8                *Data,
  IN UINT16               DataSize
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
#endif

