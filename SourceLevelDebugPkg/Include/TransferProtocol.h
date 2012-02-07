/** @file
  Transfer protocol defintions used by debug agent and host. It is only
  intended to be used by Debug related module implementation.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TRANSFER_PROTOCOL_H__
#define __TRANSFER_PROTOCOL_H__

#include "ProcessorContext.h"
#include "SoftDebuggerDefinitions.h"

//
// Definitions for break command.
//
#define DEBUG_STARTING_SYMBOL_BREAK     (0x21)  //  '!'
#define DEBUG_STARTING_SYMBOL_BREAK_STRING     ("!")

//
//  Definition for starting symbol of a normal debug packet. Choose a non-ASCII to avoid conflict with other serial output.
//
#define DEBUG_STARTING_SYMBOL_NORMAL    (0xFE)


#pragma pack(1)

//
// Definition for common header for normal debug packets (not including break command)
//
typedef struct {
  UINT8                      StartSymbol;
  UINT8                      Command;
  UINT8                      DataLength;
} DEBUG_COMMAND_HEADER;

//
// Structure to facilitate debug packet header parsing or construction
//
typedef struct {
  UINT8                      Command;
  UINT8                      DataLength;
} DEBUG_COMMAND_HEADER_NO_START_SYMBOL;


//
// Definition for Command field for debug packets
//
#define DEBUG_COMMAND_REQUEST      (0 << 7)
#define DEBUG_COMMAND_RESPONSE     (1 << 7)



#define DEBUG_COMMAND_RESET                       (DEBUG_COMMAND_REQUEST | 0)  // 0

#define DEBUG_COMMAND_GO                          (DEBUG_COMMAND_REQUEST | 1)  // 1

#define DEBUG_COMMAND_BREAK_CAUSE                 (DEBUG_COMMAND_REQUEST | 2)  // 2

#define DEBUG_COMMAND_SET_HW_BREAKPOINT           (DEBUG_COMMAND_REQUEST | 3)  // 3
#define DEBUG_COMMAND_CLEAR_HW_BREAKPOINT         (DEBUG_COMMAND_REQUEST | 4)  // 4

#define DEBUG_COMMAND_SINGLE_STEPPING             (DEBUG_COMMAND_REQUEST | 5)  // 5

#define DEBUG_COMMAND_SET_SW_BREAKPOINT           (DEBUG_COMMAND_REQUEST | 6)  // 6
#define DEBUG_COMMAND_CLEAR_SW_BREAKPOINT         (DEBUG_COMMAND_REQUEST | 7)  // 7

#define DEBUG_COMMAND_READ_MEMORY_8               (DEBUG_COMMAND_REQUEST | 8)  // 8
#define DEBUG_COMMAND_READ_MEMORY_16              (DEBUG_COMMAND_REQUEST | 9)  // 9
#define DEBUG_COMMAND_READ_MEMORY_32              (DEBUG_COMMAND_REQUEST | 10) // 10
#define DEBUG_COMMAND_READ_MEMORY_64              (DEBUG_COMMAND_REQUEST | 11) // 11

#define DEBUG_COMMAND_WRITE_MEMORY_8              (DEBUG_COMMAND_REQUEST | 12) // 12
#define DEBUG_COMMAND_WRITE_MEMORY_16             (DEBUG_COMMAND_REQUEST | 13) // 13
#define DEBUG_COMMAND_WRITE_MEMORY_32             (DEBUG_COMMAND_REQUEST | 14) // 14
#define DEBUG_COMMAND_WRITE_MEMORY_64             (DEBUG_COMMAND_REQUEST | 15) // 15

#define DEBUG_COMMAND_READ_IO                     (DEBUG_COMMAND_REQUEST | 16) // 16
#define DEBUG_COMMAND_WRITE_IO                    (DEBUG_COMMAND_REQUEST | 20) // 20

#define DEBUG_COMMAND_READ_REGISTER               (DEBUG_COMMAND_REQUEST | 24) // 24
#define DEBUG_COMMAND_WRITE_REGISTER              (DEBUG_COMMAND_REQUEST | 26) // 26

#define DEBUG_COMMAND_STEP_OVER                   (DEBUG_COMMAND_REQUEST | 28) // 28
#define DEBUG_COMMAND_STEP_OUT                    (DEBUG_COMMAND_REQUEST | 29) // 29
#define DEBUG_COMMAND_STEP_BRANCH                 (DEBUG_COMMAND_REQUEST | 30) // 30

#define DEBUG_COMMAND_ARCH_MODE                   (DEBUG_COMMAND_REQUEST | 34) // 34

#define DEBUG_COMMAND_READ_MSR                    (DEBUG_COMMAND_REQUEST | 35) // 35
#define DEBUG_COMMAND_WRITE_MSR                   (DEBUG_COMMAND_REQUEST | 36) // 36

#define DEBUG_COMMAND_READ_REGISTER_GROUP         (DEBUG_COMMAND_REQUEST | 37) // 37

#define DEBUG_COMMAND_SET_DEBUG_FLAG              (DEBUG_COMMAND_REQUEST | 38) // 38

#define DEBUG_COMMAND_GET_REVISION                (DEBUG_COMMAND_REQUEST | 39) // 39

#define DEBUG_COMMAND_GET_EXCEPTION               (DEBUG_COMMAND_REQUEST | 40) // 40

#define DEBUG_COMMAND_SET_VIEWPOINT               (DEBUG_COMMAND_REQUEST | 41) // 41

#define DEBUG_COMMAND_GET_VIEWPOINT               (DEBUG_COMMAND_REQUEST | 42) // 42

//
// The below are target side initiated commands.
//
#define DEBUG_COMMAND_INIT_BREAK                  (DEBUG_COMMAND_REQUEST | 63) // 63
#define DEBUG_COMMAND_BREAK_POINT                 (DEBUG_COMMAND_REQUEST | 62) // 62
#define DEBUG_COMMAND_MEMORY_READY                (DEBUG_COMMAND_REQUEST | 61) // 61

#define DEBUG_COMMAND_OK                          (DEBUG_COMMAND_RESPONSE | 0)
#define DEBUG_COMMAND_RESEND                      (DEBUG_COMMAND_RESPONSE | 1)
#define DEBUG_COMMAND_ABORT                       (DEBUG_COMMAND_RESPONSE | 2)

//
// The below 2 commands are used when transferring big data (like > ~250 bytes). The sequence is:
//  Host Macine                      Target Macine
//  Request                =>
//                         <=        IN_PROGRESS with part of the data
//  CONTINUE               =>
//  (could have multiple IN_PROGRESS and CONTINUE interactions)
//                         <=        OK with the last part of data
//  OK (no data as ACK)    =>
//
#define DEBUG_COMMAND_IN_PROGRESS                 (DEBUG_COMMAND_RESPONSE | 3)  // Used when trying to
#define DEBUG_COMMAND_CONTINUE                    (DEBUG_COMMAND_RESPONSE | 4)  // Used when trying to transfer big data (like > ~250 bytes)

//
// The below 2 commands are used to support deferred halt. HALT_DEFERRED will be returned when a halt request received while target is already in inter-active mode.
// HALT_PROCESSED will be return as a possible return value for GO command, if target has a pending halt request.
//
#define DEBUG_COMMAND_HALT_DEFERRED               (DEBUG_COMMAND_RESPONSE | 5)
#define DEBUG_COMMAND_HALT_PROCESSED              (DEBUG_COMMAND_RESPONSE | 6)

#define DEBUG_COMMAND_TIMEOUT                     (DEBUG_COMMAND_RESPONSE | 7)
#define DEBUG_COMMAND_NOT_SUPPORTED               (DEBUG_COMMAND_RESPONSE | 15)

//
// Definition for data field for debug packets
//
#define DEBUG_DATA_MAXIMUM_LENGTH_FOR_SMALL_COMMANDS     20

#define DEBUG_DATA_UPPER_LIMIT                           0xff  // This is the upper limit for the data size, by the limit of the packet header definition.

#define DEBUG_DATA_MAXIMUM_REAL_DATA                     0xf8

#define DEBUG_DEFINITION_MAX_IO_LENGTH                   4

//
// Response data for DEBUG_COMMAND_BREAK_CAUSE
//
typedef struct {
  UINT8       Cause;
  UINT64      StopAddress;
} DEBUG_DATA_RESPONSE_BREAK_CAUSE;

//
// Break type defintions for DEBUG_DATA_BREAK_CAUSE
//
#define DEBUG_DATA_BREAK_CAUSE_UNKNOWN        0
#define DEBUG_DATA_BREAK_CAUSE_HW_BREAKPOINT  1
#define DEBUG_DATA_BREAK_CAUSE_STEPPING       2
#define DEBUG_DATA_BREAK_CAUSE_SW_BREAKPOINT  3
#define DEBUG_DATA_BREAK_CAUSE_USER_HALT      4
#define DEBUG_DATA_BREAK_CAUSE_IMAGE_LOAD     5
#define DEBUG_DATA_BREAK_CAUSE_IMAGE_UNLOAD   6
#define DEBUG_DATA_BREAK_CAUSE_SYSTEM_RESET   7
#define DEBUG_DATA_BREAK_CAUSE_EXCEPTION      8
#define DEBUG_DATA_BREAK_CAUSE_MEMORY_READY   9

//
// Response data for DEBUG_COMMAND_ARCH_MODE, defined as SOFT_DEBUGGER_PROCESSOR_...
//
typedef struct {
  UINT8       CpuMode;
} DEBUG_DATA_RESPONSE_ARCH_MODE;

//
// Cpu architecture defintions for DEBUG_DATA_RESPONSE_ARCH_MODE
//
#define DEBUG_DATA_BREAK_CPU_ARCH_IA16        0
#define DEBUG_DATA_BREAK_CPU_ARCH_IA32        1
#define DEBUG_DATA_BREAK_CPU_ARCH_X64         2

//
// Command and response data for DEBUG_COMMAND_XX_YY_BREAKPOINT
//
typedef struct {
  UINT8  Length:2;                   // Refer to below DEBUG_DATA_BREAKPOINT_LENGTH_XX macros
  UINT8  Access:2;                   // Refer to below DEBUG_DATA_BREAKPOINT_ACCESS_XX macros
  UINT8  Index:2;                    // Index of debug register
  UINT8  Reserved:2;
} DEBUG_DATA_BREAKPOINT_TYPE;

#define DEBUG_DATA_BREAKPOINT_MEMORY_ACCESS    (0x11)
#define DEBUG_DATA_BREAKPOINT_IO_ACCESS        (0x10)
#define DEBUG_DATA_BREAKPOINT_MEMORY_WRITE     (0x01)
#define DEBUG_DATA_BREAKPOINT_MEMORY_EXECUTE   (0x00)

#define DEBUG_DATA_BREAKPOINT_LENGTH_64        (0x11)
#define DEBUG_DATA_BREAKPOINT_LENGTH_32        (0x10)
#define DEBUG_DATA_BREAKPOINT_LENGTH_16        (0x01)
#define DEBUG_DATA_BREAKPOINT_LENGTH_8         (0x00)

//
// Command data for DEBUG_COMMAND_SET_HW_BREAKPOINT
//
typedef struct {
  DEBUG_DATA_BREAKPOINT_TYPE Type;
  UINT64                     Address;
} DEBUG_DATA_SET_HW_BREAKPOINT;

//
// Command data for DEBUG_COMMAND_CLEAR_HW_BREAKPOINT
//
typedef struct {
  UINT8                      IndexMask;  // 0x0f will clear all hw breakpoints
} DEBUG_DATA_CLEAR_HW_BREAKPOINT;

//
// Command data for DEBUG_COMMAND_SET_SW_BREAKPOINT
//
typedef struct {
  UINT64                     Address;
} DEBUG_DATA_SET_SW_BREAKPOINT;

//
// Response data for DEBUG_COMMAND_SET_SW_BREAKPOINT
//
typedef struct {
  UINT8                      OriginalData;
} DEBUG_DATA_RESPONSE_SET_SW_BREAKPOINT;

//
// Command data for DEBUG_COMMAND_CLEAR_SW_BREAKPOINT
//
typedef  DEBUG_DATA_SET_SW_BREAKPOINT DEBUG_DATA_CLEAR_SW_BREAKPOINT;

//
// Command data for DEBUG_COMMAND_READ_MEMORY_XX
//
typedef struct {
  UINT64                     Address;
  UINT16                     Count;
} DEBUG_DATA_READ_MEMORY_8;

typedef DEBUG_DATA_READ_MEMORY_8 DEBUG_DATA_READ_MEMORY_16;

typedef DEBUG_DATA_READ_MEMORY_8 DEBUG_DATA_READ_MEMORY_32;

typedef DEBUG_DATA_READ_MEMORY_8 DEBUG_DATA_READ_MEMORY_64;

//
// Command data for DEBUG_COMMAND_WRITE_MEMORY_XX
//
typedef struct {
  UINT64                     Address;
  UINT16                     Count;
  UINT8                      Data;     // The actual length for this field is decided by Width x Count
} DEBUG_DATA_WRITE_MEMORY_8;

typedef DEBUG_DATA_WRITE_MEMORY_8 DEBUG_DATA_WRITE_MEMORY_16;

typedef DEBUG_DATA_WRITE_MEMORY_8 DEBUG_DATA_WRITE_MEMORY_32;

typedef DEBUG_DATA_WRITE_MEMORY_8 DEBUG_DATA_WRITE_MEMORY_64;

//
// Command data for DEBUG_COMMAND_READ_IO
//
typedef struct {
  UINT16                     Port;
  UINT8                      Width;
} DEBUG_DATA_READ_IO;

//
// Response data for DEBUG_COMMAND_READ_IO
//
typedef struct {
  UINT8                      Data;  // The actual length of this structure will be adjusted according to the Width field
} DEBUG_DATA_RESPONSE_READ_IO;

//
// Command data for DEBUG_COMMAND_WRITE_IO
//
typedef struct {
  UINT16                     Port;
  UINT8                      Width;
  UINT8                      Data; // The actual length of this structure will be adjusted according to the Width field
} DEBUG_DATA_WRITE_IO;

//
// Command data for DEBUG_COMMAND_READ_REGISTER
//
typedef struct {
  UINT8                      Index;   // defined as DEBUG_DEFINITION_REGISTER_XX
  UINT8                      Offset:4;
  UINT8                      Length:4;
} DEBUG_DATA_READ_REGISTER;

//
// Command data for DEBUG_COMMAND_WRITE_REGISTER
//
typedef struct {
  UINT8                      Index;   // defined as DEBUG_DEFINITION_REGISTER_XX
  UINT8                      Offset:4;
  UINT8                      Length:4;
  UINT64                     Value;
} DEBUG_DATA_WRITE_REGISTER;

//
// Command data for DEBUG_COMMAND_READ_MSR
//
typedef struct {
  UINT32                     Index;
} DEBUG_DATA_READ_MSR;

//
// Response data for DEBUG_COMMAND_READ_MSR
//
typedef struct {
  UINT64                     Value;
} DEBUG_DATA_RESPONSE_READ_MSR;

//
// Command data for DEBUG_COMMAND_WRITE_MSR
//
typedef struct {
  UINT32                     Index;
  UINT64                     Value;
} DEBUG_DATA_WRITE_MSR;

//
// Command data for DEBUG_COMMAND_READ_REGISTER_GROUP
//
typedef struct {
  // For possible values, refer to the definition for DEBUG_DEFINITION_REGISTER_GROUP_XXX (in another .h file as it is architecture specific)
  UINT8                     Index;
} DEBUG_DATA_READ_REGISTER_GROUP;

//
// Response data for DEBUG_COMMAND_GET_REVISION
//
typedef struct {
  UINT32                    Revision;
  UINT32                    Capabilities;
} DEBUG_DATA_RESPONSE_GET_REVISION;

//
// Response data for DEBUG_COMMAND_GET_EXCEPTION
//
typedef struct {
  UINT8                     ExceptionNum;
  UINT64                    ExceptionData;
} DEBUG_DATA_RESPONSE_GET_EXCEPTION;

typedef struct {
  UINT8                     DRn;    // The index of DR register which to be used as temporary breakpoint
} DEBUG_DATA_STEP_OVER;

//
// Command data for DEBUG_COMMAND_SET_DEBUG_FLAG
//
typedef struct {
  UINT32                    DebugFlag;    // The index of DR register which to be used as temporary breakpoint
} DEBUG_DATA_SET_DEBUG_FLAG;

//
// Command data for DEBUG_COMMAND_SET_VIEWPOINT
// If viewpoint is changed successfully, DEBUG_COMMAND_OK will be returned.
// If viewpoint is not availabe, DEBUG_COMMAND_NOT_SUPPORTED will be returned.
//
typedef struct {
  UINT32                    ViewPoint;     // The index of viewpoint will be set
} DEBUG_DATA_SET_VIEWPOINT;

//
// Response data for DEBUG_COMMAND_GET_VIEWPOINT
//
typedef struct {
  UINT32                    ViewPoint;     // The index of viewpoint will be returned
} DEBUG_DATA_RESPONSE_GET_VIEWPOINT;

#pragma pack()

#define DEBUG_PACKET_CONSTRUCTOR_WITH_NO_DATA(DebugPacket,ShortCommandType)                              \
  ((DEBUG_COMMAND_HEADER *)DebugPacket)->StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;                     \
  ((DEBUG_COMMAND_HEADER *)DebugPacket)->Command = DEBUG_COMMAND_##ShortCommandType;                     \
  ((DEBUG_COMMAND_HEADER *)DebugPacket)->DataLength = 0;

#define DEBUG_PACKET_CONSTRUCTOR_WITH_DATA(DebugPacket,ShortCommandType, DebugPacketDataPointer, PacketLength)         \
  ((DEBUG_COMMAND_HEADER *)DebugPacket)->StartSymbol = DEBUG_STARTING_SYMBOL_NORMAL;                     \
  ((DEBUG_COMMAND_HEADER *)DebugPacket)->Command = DEBUG_COMMAND_##ShortCommandType;                     \
  ((DEBUG_COMMAND_HEADER *)DebugPacket)->DataLength = sizeof (DEBUG_DATA_##ShortCommandType);            \
  *DebugPacketDataPointer = (DEBUG_DATA_##ShortCommandType *)((DEBUG_COMMAND_HEADER *)DebugPacket+1);    \
  *PacketLength = sizeof (DEBUG_COMMAND_HEADER) + sizeof (DEBUG_DATA_##ShortCommandType);

#endif

