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

#include <TransferProtocol.h>
#include <ImageDebugSupport.h>

#include "DebugMp.h"
#include "DebugTimer.h"
#include "ArchDebugSupport.h"

#define DEBUG_AGENT_REVISION            ((0 << 16) | 01)
#define DEBUG_AGENT_CAPABILITIES        0

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

typedef union {
  struct {
    UINT32  HostPresent      : 1;
    UINT32  BreakOnNextSmi   : 1;
    UINT32  Reserved         : 30;
  } Bits;
  UINT32  Uint32;
} DEBUG_AGENT_FLAG;

#pragma pack(1)
typedef struct {
  DEBUG_AGENT_FLAG           DebugFlag;
  UINT64                     DebugPortHandle;
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
  Write specified register into save CPU context.

  @param[in] CpuContext         Pointer to saved CPU context.
  @param[in] Index              Register index value.
  @param[in] Offset             Offset in register address range
  @param[in] Width              Data width to read.
  @param[in] RegisterBuffer     Pointer to input buffer with data.

**/
VOID
ArchWriteRegisterBuffer (
  IN DEBUG_CPU_CONTEXT               *CpuContext,
  IN UINT8                           Index,
  IN UINT8                           Offset,
  IN UINT8                           Width,
  IN UINT8                           *RegisterBuffer
  );

/**
  Read register value from saved CPU context.

  @param[in] CpuContext         Pointer to saved CPU context.
  @param[in] Index              Register index value.
  @param[in] Offset             Offset in register address range
  @param[in] Width              Data width to read.

  @return The address of register value.

**/
UINT8 *
ArchReadRegisterBuffer (
  IN DEBUG_CPU_CONTEXT               *CpuContext,
  IN UINT8                           Index,
  IN UINT8                           Offset,
  IN UINT8                           *Width
  );

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
  );

/**
  Read segment selector by register index.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] RegisterIndex        Register Index.

  @return Value of segment selector.

**/
UINT64
ReadRegisterSelectorByIndex (
  IN DEBUG_CPU_CONTEXT                       *CpuContext,
  IN UINT8                                   RegisterIndex
  );

/**
  Read group register of common registers.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] RegisterGroup        Pointer to Group registers.

**/
VOID
ReadRegisterGroup (
  IN DEBUG_CPU_CONTEXT                       *CpuContext,
  IN DEBUG_DATA_REPONSE_READ_REGISTER_GROUP  *RegisterGroup
  );

/**
  Read group register of Segment Base.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] RegisterGroupSegBase Pointer to Group registers.

**/
VOID
ReadRegisterGroupSegBase (
  IN DEBUG_CPU_CONTEXT                              *CpuContext,
  IN DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE *RegisterGroupSegBase
  );

/**
  Read gourp register of Segment Limit.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] RegisterGroupSegLim  Pointer to Group registers.

**/
VOID
ReadRegisterGroupSegLim (
  IN DEBUG_CPU_CONTEXT                             *CpuContext,
  IN DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM *RegisterGroupSegLim
  );

/**
  Read group register by group index.

  @param[in] CpuContext           Pointer to saved CPU context.
  @param[in] GroupIndex           Group Index.

  @retval RETURN_SUCCESS         Read successfully.
  @retval RETURN_NOT_SUPPORTED   Group index cannot be supported.

**/
RETURN_STATUS
ArchReadRegisterGroup (
  IN DEBUG_CPU_CONTEXT                             *CpuContext,
  IN UINT8                                         GroupIndex
  );

/**
  Send acknowledge packet to HOST.

  @param AckCommand    Type of Acknowledge packet.

**/
VOID
SendAckPacket (
  IN UINT8                AckCommand
  );

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
  );

/**
  Check if HOST is connected based on Mailbox.

  @retval TRUE        HOST is connected.
  @retval FALSE       HOST is not connected.

**/
BOOLEAN
IsHostConnected (
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

#endif

