/** @file
  Transfer protocol defintions used by debug agent and host. It is only
  intended to be used by Debug related module implementation.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
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

//
// Current revision of transfer protocol
// 0.4: Packet compression and decompression.
//
#define DEBUG_AGENT_REVISION_03         ((0 << 16) | 03)
#define DEBUG_AGENT_REVISION_04         ((0 << 16) | 04)
#define DEBUG_AGENT_REVISION            DEBUG_AGENT_REVISION_04
#define DEBUG_AGENT_CAPABILITIES        0

//
// Definitions for the (A)ttach command
//
#define DEBUG_STARTING_SYMBOL_ATTACH    (0xFA)

//
// Definition for starting symbol of a normal debug packet. Choose a non-ASCII to avoid conflict with other serial output.
//
#define DEBUG_STARTING_SYMBOL_NORMAL    (0xFE)

//
// Definition for starting symbol of a (C)ompressed debug packet. Choose a non-ASCII to avoid conflict with other serial output.
//
#define DEBUG_STARTING_SYMBOL_COMPRESS  (0xFC)

#pragma pack(1)

//
// Definition for debug packet header for debug packets (not including attach command)
//
typedef struct {
  UINT8                      StartSymbol;
  UINT8                      Command;
  UINT8                      Length;    // Length of Debug Packet including header and payload in byte
  UINT8                      SequenceNo;
  UINT16                     Crc;
} DEBUG_PACKET_HEADER;

//
// Definition for Command field for debug packets
//
#define DEBUG_COMMAND_REQUEST      (0 << 7)
#define DEBUG_COMMAND_RESPONSE     (1 << 7)

#define IS_REQUEST(x)              (((x)->Command & DEBUG_COMMAND_RESPONSE) == 0)

//
// HOST initiated commands
//
#define DEBUG_COMMAND_RESET                       (DEBUG_COMMAND_REQUEST | 0x00)
#define DEBUG_COMMAND_GO                          (DEBUG_COMMAND_REQUEST | 0x01)
#define DEBUG_COMMAND_BREAK_CAUSE                 (DEBUG_COMMAND_REQUEST | 0x02)
#define DEBUG_COMMAND_SET_HW_BREAKPOINT           (DEBUG_COMMAND_REQUEST | 0x03)
#define DEBUG_COMMAND_CLEAR_HW_BREAKPOINT         (DEBUG_COMMAND_REQUEST | 0x04)
#define DEBUG_COMMAND_SINGLE_STEPPING             (DEBUG_COMMAND_REQUEST | 0x05)
#define DEBUG_COMMAND_SET_SW_BREAKPOINT           (DEBUG_COMMAND_REQUEST | 0x06)
#define DEBUG_COMMAND_READ_MEMORY                 (DEBUG_COMMAND_REQUEST | 0x07)
#define DEBUG_COMMAND_WRITE_MEMORY                (DEBUG_COMMAND_REQUEST | 0x08)
#define DEBUG_COMMAND_READ_IO                     (DEBUG_COMMAND_REQUEST | 0x09)
#define DEBUG_COMMAND_WRITE_IO                    (DEBUG_COMMAND_REQUEST | 0x0A)
#define DEBUG_COMMAND_READ_REGISTER               (DEBUG_COMMAND_REQUEST | 0x0B)
#define DEBUG_COMMAND_WRITE_REGISTER              (DEBUG_COMMAND_REQUEST | 0x0C)
#define DEBUG_COMMAND_READ_ALL_REGISTERS          (DEBUG_COMMAND_REQUEST | 0x0D)
#define DEBUG_COMMAND_ARCH_MODE                   (DEBUG_COMMAND_REQUEST | 0x0E)
#define DEBUG_COMMAND_READ_MSR                    (DEBUG_COMMAND_REQUEST | 0x0F)
#define DEBUG_COMMAND_WRITE_MSR                   (DEBUG_COMMAND_REQUEST | 0x10)
#define DEBUG_COMMAND_SET_DEBUG_SETTING           (DEBUG_COMMAND_REQUEST | 0x11)
#define DEBUG_COMMAND_GET_REVISION                (DEBUG_COMMAND_REQUEST | 0x12)
#define DEBUG_COMMAND_GET_EXCEPTION               (DEBUG_COMMAND_REQUEST | 0x13)
#define DEBUG_COMMAND_SET_VIEWPOINT               (DEBUG_COMMAND_REQUEST | 0x14)
#define DEBUG_COMMAND_GET_VIEWPOINT               (DEBUG_COMMAND_REQUEST | 0x15)
#define DEBUG_COMMAND_DETACH                      (DEBUG_COMMAND_REQUEST | 0x16)
#define DEBUG_COMMAND_CPUID                       (DEBUG_COMMAND_REQUEST | 0x17)
#define DEBUG_COMMAND_SEARCH_SIGNATURE            (DEBUG_COMMAND_REQUEST | 0x18)
#define DEBUG_COMMAND_HALT                        (DEBUG_COMMAND_REQUEST | 0x19)

//
// TARGET initiated commands
//
#define DEBUG_COMMAND_INIT_BREAK                  (DEBUG_COMMAND_REQUEST | 0x3F)
#define DEBUG_COMMAND_BREAK_POINT                 (DEBUG_COMMAND_REQUEST | 0x3E)
#define DEBUG_COMMAND_MEMORY_READY                (DEBUG_COMMAND_REQUEST | 0x3D)
#define DEBUG_COMMAND_PRINT_MESSAGE               (DEBUG_COMMAND_REQUEST | 0x3C)
#define DEBUG_COMMAND_ATTACH_BREAK                (DEBUG_COMMAND_REQUEST | 0x3B)

//
// Response commands
//
#define DEBUG_COMMAND_OK                          (DEBUG_COMMAND_RESPONSE | 0x00)
#define DEBUG_COMMAND_RESEND                      (DEBUG_COMMAND_RESPONSE | 0x01)
#define DEBUG_COMMAND_ABORT                       (DEBUG_COMMAND_RESPONSE | 0x02)
//
// The below 2 commands are used when transferring big data (like > ~250 bytes).
// The sequence is:
//   HOST                             TARGET
//   Request                =>
//                          <=        IN_PROGRESS with partial data
//   CONTINUE               =>
//   (could have multiple IN_PROGRESS and CONTINUE interactions)
//                          <=        OK with the last part of data
//   OK (no data as ACK)    =>
//
#define DEBUG_COMMAND_IN_PROGRESS                 (DEBUG_COMMAND_RESPONSE | 0x03)
#define DEBUG_COMMAND_CONTINUE                    (DEBUG_COMMAND_RESPONSE | 0x04)
//
// The below 2 commands are used to support deferred halt:
// TARGET returns HALT_DEFERRED when it receives a HALT request in inter-active mode.
// TARGET returns HALT_PROCESSED when it receives a GO request and has a pending HALT request.
//
#define DEBUG_COMMAND_HALT_DEFERRED               (DEBUG_COMMAND_RESPONSE | 0x05)
#define DEBUG_COMMAND_HALT_PROCESSED              (DEBUG_COMMAND_RESPONSE | 0x06)

#define DEBUG_COMMAND_TIMEOUT                     (DEBUG_COMMAND_RESPONSE | 0x07)
#define DEBUG_COMMAND_NOT_SUPPORTED               (DEBUG_COMMAND_RESPONSE | 0x0F)

//
// Definition for data field for debug packets
//
#define DEBUG_DATA_UPPER_LIMIT                    0xff  // Upper limit for the data size, by the limit of the packet header definition.

#define DEBUG_DATA_MAXIMUM_REAL_DATA              (DEBUG_DATA_UPPER_LIMIT - sizeof (DEBUG_PACKET_HEADER))

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

typedef struct {
  UINT8  Length:2;                   // Refer to below DEBUG_DATA_BREAKPOINT_LENGTH_XX macros
  UINT8  Access:2;                   // Refer to below DEBUG_DATA_BREAKPOINT_ACCESS_XX macros
  UINT8  Index:2;                    // Index of debug register
  UINT8  Reserved:2;
} DEBUG_DATA_BREAKPOINT_TYPE;
#define DEBUG_DATA_BREAKPOINT_MEMORY_ACCESS    (0x3)
#define DEBUG_DATA_BREAKPOINT_IO_ACCESS        (0x2)
#define DEBUG_DATA_BREAKPOINT_MEMORY_WRITE     (0x1)
#define DEBUG_DATA_BREAKPOINT_MEMORY_EXECUTE   (0x0)
#define DEBUG_DATA_BREAKPOINT_LENGTH_32        (0x3)
#define DEBUG_DATA_BREAKPOINT_LENGTH_64        (0x2)
#define DEBUG_DATA_BREAKPOINT_LENGTH_16        (0x1)
#define DEBUG_DATA_BREAKPOINT_LENGTH_8         (0x0)

//
// Request data for DEBUG_COMMAND_SET_HW_BREAKPOINT
//
typedef struct {
  DEBUG_DATA_BREAKPOINT_TYPE Type;
  UINT64                     Address;
} DEBUG_DATA_SET_HW_BREAKPOINT;

//
// Request data for DEBUG_COMMAND_CLEAR_HW_BREAKPOINT
//
typedef struct {
  UINT8                      IndexMask;  // 0x0f will clear all hw breakpoints
} DEBUG_DATA_CLEAR_HW_BREAKPOINT;

//
// Request and response data for DEBUG_COMMAND_SET_SW_BREAKPOINT
//
typedef struct {
  UINT64                     Address;
} DEBUG_DATA_SET_SW_BREAKPOINT;

typedef struct {
  UINT8                      OriginalData;
} DEBUG_DATA_RESPONSE_SET_SW_BREAKPOINT;

//
// Request data for DEBUG_COMMAND_READ_MEMORY
//
typedef struct {
  UINT64                     Address;
  UINT8                      Width;
  UINT16                     Count;
} DEBUG_DATA_READ_MEMORY;

//
// Request data for DEBUG_COMMAND_WRITE_MEMORY
//
typedef struct {
  UINT64                     Address;
  UINT8                      Width;
  UINT16                     Count;
  UINT8                      Data[1];  // The actual length is (Width * Count)
} DEBUG_DATA_WRITE_MEMORY;

//
// Request and response data for DEBUG_COMMAND_READ_IO
//
typedef struct {
  UINT64                     Port;
  UINT8                      Width;
} DEBUG_DATA_READ_IO;

typedef struct {
  UINT8                      Data[1];  // The actual length depends on the packet header
} DEBUG_DATA_RESPONSE_READ_IO;

//
// Request data for DEBUG_COMMAND_WRITE_IO
//
typedef struct {
  UINT64                     Port;
  UINT8                      Width;
  UINT8                      Data[1];  // The actual length is Width
} DEBUG_DATA_WRITE_IO;

//
// Request data for DEBUG_COMMAND_READ_REGISTER
//
typedef struct {
  UINT8                      Index;   // defined as SOFT_DEBUGGER_REGISTER_XX
} DEBUG_DATA_READ_REGISTER;

//
// Request data for DEBUG_COMMAND_WRITE_REGISTER
//
typedef struct {
  UINT8                      Index;   // defined as SOFT_DEBUGGER_REGISTER_XX
  UINT8                      Length;
  UINT8                      Data[1]; // The actual length is Length
} DEBUG_DATA_WRITE_REGISTER;

//
// Request and response data for DEBUG_COMMAND_READ_MSR
//
typedef struct {
  UINT32                     Index;
} DEBUG_DATA_READ_MSR;

typedef struct {
  UINT64                     Value;
} DEBUG_DATA_RESPONSE_READ_MSR;

//
// Request data for DEBUG_COMMAND_WRITE_MSR
//
typedef struct {
  UINT32                     Index;
  UINT64                     Value;
} DEBUG_DATA_WRITE_MSR;

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
  UINT32                    ExceptionData;
} DEBUG_DATA_RESPONSE_GET_EXCEPTION;

//
// Request data for DEBUG_DATA_SET_DEBUG_SETTING
//
typedef struct {
  UINT8                    Key;
  UINT8                    Value;
} DEBUG_DATA_SET_DEBUG_SETTING;
//
// Supported keys
//
#define DEBUG_AGENT_SETTING_SMM_ENTRY_BREAK          1
#define DEBUG_AGENT_SETTING_PRINT_ERROR_LEVEL        2
#define DEBUG_AGENT_SETTING_BOOT_SCRIPT_ENTRY_BREAK  3
//
// Bitmask of print error level for debug message
//
#define DEBUG_AGENT_ERROR     BIT0
#define DEBUG_AGENT_WARNING   BIT1
#define DEBUG_AGENT_INFO      BIT2
#define DEBUG_AGENT_VERBOSE   BIT3

//
// Request data for DEBUG_COMMAND_SET_VIEWPOINT
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

//
// Request and response data for DEBUG_COMMAND_CPUID
//
typedef struct {
  UINT32                    Eax;           // The value of EAX prior to invoking the CPUID instruction
  UINT32                    Ecx;           // The value of ECX prior to invoking the CPUID instruction
} DEBUG_DATA_CPUID;

typedef struct {
  UINT32                    Eax;           // The value of EAX returned by the CPUID instruction
  UINT32                    Ebx;           // The value of EBX returned by the CPUID instruction
  UINT32                    Ecx;           // The value of ECX returned by the CPUID instruction
  UINT32                    Edx;           // The value of EDX returned by the CPUID instruction
} DEBUG_DATA_RESPONSE_CPUID;

//
// Request and response data for DEBUG_COMMAND_SEARCH_SIGNATURE
//
typedef struct {
  UINT64                    Start;
  UINT32                    Count;
  UINT32                    Alignment;
  BOOLEAN                   Positive;      // TRUE to search in higher address memory
  UINT8                     DataLength;
  UINT8                     Data[1];
} DEBUG_DATA_SEARCH_SIGNATURE;

typedef struct {
  UINT64                    Address;       // -1 indicates not found
} DEBUG_DATA_RESPONSE_SEARCH_SIGNATURE;

#pragma pack()

#endif

