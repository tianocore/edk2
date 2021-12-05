/** @file
  Private include file for GDB stub

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GDB_STUB_INTERNAL__
#define __GDB_STUB_INTERNAL__

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/GdbSerialLib.h>
#include <Library/PrintLib.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/SerialIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/LoadedImage.h>
#include <Guid/DebugImageInfoTable.h>
#include <IndustryStandard/PeImage.h>

extern CONST CHAR8  mHexToStr[];

// maximum size of input and output buffers
// This value came from the show remote command of the gdb we tested against
#define MAX_BUF_SIZE  2000

// maximum size of address buffer
#define MAX_ADDR_SIZE  32

// maximum size of register number buffer
#define MAX_REG_NUM_BUF_SIZE  32

// maximum size of length buffer
#define MAX_LENGTH_SIZE  32

// maximum size of T signal members
#define MAX_T_SIGNAL_SIZE  64

// the mask used to clear all the cache
#define TF_BIT  0x00000100

//
// GDB Signal definitions - generic names for interrupts
//
#define GDB_SIGILL   4     // Illegal instruction
#define GDB_SIGTRAP  5     // Trace Trap (Breakpoint and SingleStep)
#define GDB_SIGEMT   7     // Emulator Trap
#define GDB_SIGFPE   8     // Floating point exception
#define GDB_SIGSEGV  11    // Segment violation, page fault

//
// GDB File I/O Error values, zero means no error
// Includes all general GDB Unix like error values
//
#define GDB_EBADMEMADDRBUFSIZE    11  // the buffer that stores memory Address to be read from/written to is not the right size
#define GDB_EBADMEMLENGBUFSIZE    12  // the buffer that stores Length is not the right size
#define GDB_EBADMEMLENGTH         13  // Length, the given number of bytes to read or write, is not the right size
#define GDB_EBADMEMDATA           14  // one of the bytes or nibbles of the memory is less than 0
#define GDB_EBADMEMDATASIZE       15  // the memory data, 'XX..', is too short or too long
#define GDB_EBADBUFSIZE           21  // the buffer created is not the correct size
#define GDB_EINVALIDARG           31  // argument is invalid
#define GDB_ENOSPACE              41  //
#define GDB_EINVALIDBRKPOINTTYPE  51  // the breakpoint type is not recognized
#define GDB_EINVALIDREGNUM        61  // given register number is not valid: either <0 or >=Number of Registers
#define GDB_EUNKNOWN              255 // unknown

//
// These devices are open by GDB so we can just read and write to them
//
#define GDB_STDIN   0x00
#define GDB_STDOUT  0x01
#define GDB_STDERR  0x02

//
// Define Register size for different architectures
//
#if defined (MDE_CPU_IA32)
#define REG_SIZE  32
#elif defined (MDE_CPU_X64)
#define REG_SIZE  64
#elif defined (MDE_CPU_ARM)
#define REG_SIZE  32
#endif

#define GDB_SERIAL_DEV_SIGNATURE  SIGNATURE_32 ('g', 'd', 'b', 's')

typedef struct {
  VENDOR_DEVICE_PATH          VendorDevice;
  UINT32                      Index;                    // Support more than one
  EFI_DEVICE_PATH_PROTOCOL    End;
} GDB_SERIAL_DEVICE_PATH;

//
//  Name:   SERIAL_DEV
//  Purpose:  To provide device specific information
//  Fields:
//      Signature UINTN: The identity of the serial device
//      SerialIo  SERIAL_IO_PROTOCOL: Serial I/O protocol interface
//      SerialMode  SERIAL_IO_MODE:
//      DevicePath  EFI_DEVICE_PATH_PROTOCOL *: Device path of the serial device
//
typedef struct {
  UINTN                     Signature;
  EFI_HANDLE                Handle;
  EFI_SERIAL_IO_PROTOCOL    SerialIo;
  EFI_SERIAL_IO_MODE        SerialMode;
  GDB_SERIAL_DEVICE_PATH    DevicePath;
  INTN                      InFileDescriptor;
  INTN                      OutFileDescriptor;
} GDB_SERIAL_DEV;

#define GDB_SERIAL_DEV_FROM_THIS(a)  CR (a, GDB_SERIAL_DEV, SerialIo, GDB_SERIAL_DEV_SIGNATURE)

typedef struct {
  EFI_EXCEPTION_TYPE    Exception;
  UINT8                 SignalNo;
} EFI_EXCEPTION_TYPE_ENTRY;

#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)

//
// Byte packed structure for DR6
// 32-bits on IA-32
// 64-bits on X64.  The upper 32-bits on X64 are reserved
//
typedef union {
  struct {
    UINT32    B0         : 1;  // Breakpoint condition detected
    UINT32    B1         : 1;  // Breakpoint condition detected
    UINT32    B2         : 1;  // Breakpoint condition detected
    UINT32    B3         : 1;  // Breakpoint condition detected
    UINT32    Reserved_1 : 9;  // Reserved
    UINT32    BD         : 1;  // Debug register access detected
    UINT32    BS         : 1;  // Single step
    UINT32    BT         : 1;  // Task switch
    UINT32    Reserved_2 : 16; // Reserved
  } Bits;
  UINTN    UintN;
} IA32_DR6;

//
// Byte packed structure for DR7
// 32-bits on IA-32
// 64-bits on X64.  The upper 32-bits on X64 are reserved
//
typedef union {
  struct {
    UINT32    L0         : 1; // Local breakpoint enable
    UINT32    G0         : 1; // Global breakpoint enable
    UINT32    L1         : 1; // Local breakpoint enable
    UINT32    G1         : 1; // Global breakpoint enable
    UINT32    L2         : 1; // Local breakpoint enable
    UINT32    G2         : 1; // Global breakpoint enable
    UINT32    L3         : 1; // Local breakpoint enable
    UINT32    G3         : 1; // Global breakpoint enable
    UINT32    LE         : 1; // Local exact breakpoint enable
    UINT32    GE         : 1; // Global exact breakpoint enable
    UINT32    Reserved_1 : 3; // Reserved
    UINT32    GD         : 1; // Global detect enable
    UINT32    Reserved_2 : 2; // Reserved
    UINT32    RW0        : 2; // Read/Write field
    UINT32    LEN0       : 2; // Length field
    UINT32    RW1        : 2; // Read/Write field
    UINT32    LEN1       : 2; // Length field
    UINT32    RW2        : 2; // Read/Write field
    UINT32    LEN2       : 2; // Length field
    UINT32    RW3        : 2; // Read/Write field
    UINT32    LEN3       : 2; // Length field
  } Bits;
  UINTN    UintN;
} IA32_DR7;

#endif /* if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64) */

typedef enum {
  InstructionExecution,   // Hardware breakpoint
  DataWrite,              // watch
  DataRead,               // rwatch
  DataReadWrite,          // awatch
  SoftwareBreakpoint,     // Software breakpoint
  NotSupported
} BREAK_TYPE;

//
// Array of exception types that need to be hooked by the debugger
//
extern EFI_EXCEPTION_TYPE_ENTRY  gExceptionType[];

//
// Set TRUE if F Reply package signals a ctrl-c. We can not process the Ctrl-c
// here we need to wait for the periodic callback to do this.
//
extern BOOLEAN  gCtrlCBreakFlag;

//
// If the periodic callback is called while we are processing an F packet we need
// to let the callback know to not read from the serial stream as it could steal
// characters from the F response packet
//
extern BOOLEAN  gProcessingFPacket;

// The offsets of registers SystemContext.
// The fields in the array are in the gdb ordering.
//
extern UINTN  gRegisterOffsets[];

/**
 Return the number of entries in the gExceptionType[]

 @retval    UINTN, the number of entries in the gExceptionType[] array.
 **/
UINTN
MaxEfiException (
  VOID
  );

/**
 Return the number of entries in the gRegisters[]

 @retval    UINTN, the number of entries (registers) in the gRegisters[] array.
 **/
UINTN
MaxRegisterCount (
  VOID
  );

/**
 Check to see if the ISA is supported.
 ISA = Instruction Set Architecture

 @retval    TRUE if Isa is supported,
 FALSE otherwise.
 **/
BOOLEAN
CheckIsa (
  IN    EFI_INSTRUCTION_SET_ARCHITECTURE  Isa
  );

/**
 Send the T signal with the given exception type (in gdb order) and possibly with n:r pairs related to the watchpoints

 @param  SystemContext        Register content at time of the exception
 @param  GdbExceptionType     GDB exception type
 **/

VOID
GdbSendTSignal (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINT8               GdbExceptionType
  );

/**
 Translates the EFI mapping to GDB mapping

 @param     EFIExceptionType        EFI Exception that is being processed
 @retval    UINTN that corresponds to EFIExceptionType's GDB exception type number
 **/
UINT8
ConvertEFItoGDBtype (
  IN  EFI_EXCEPTION_TYPE  EFIExceptionType
  );

/**
 Empties the given buffer
 @param *Buf pointer to the first element in buffer to be emptied
 **/
VOID
EmptyBuffer (
  IN CHAR8  *Buf
  );

/**
 Converts an 8-bit Hex Char into a INTN.

 @param     Char  - the hex character to be converted into UINTN
 @retval    a INTN, from 0 to 15, that corresponds to Char
 -1 if Char is not a hex character
 **/
INTN
HexCharToInt (
  IN  CHAR8  Char
  );

/** 'E NN'
 Send an error with the given error number after converting to hex.
 The error number is put into the buffer in hex. '255' is the biggest errno we can send.
 ex: 162 will be sent as A2.

 @param   errno    the error number that will be sent
 **/
VOID
EFIAPI
SendError (
  IN  UINT8  ErrorNum
  );

/**
 Send 'OK' when the function is done executing successfully.
 **/
VOID
EFIAPI
SendSuccess (
  VOID
  );

/**
 Send empty packet to specify that particular command/functionality is not supported.
 **/
VOID
EFIAPI
SendNotSupported (
  VOID
  );

/** ‘p n’
 Reads the n-th register's value into an output buffer and sends it as a packet
 @param     SystemContext       Register content at time of the exception
 @param     InBuffer            This is the input buffer received from gdb server
 **/
VOID
ReadNthRegister (
  IN    EFI_SYSTEM_CONTEXT  SystemContext,
  IN    CHAR8               *InBuffer
  );

/** ‘g’
 Reads the general registers into an output buffer  and sends it as a packet
 @param     SystemContext           Register content at time of the exception
 **/
VOID
EFIAPI
ReadGeneralRegisters (
  IN    EFI_SYSTEM_CONTEXT  SystemContext
  );

/** ‘P n...=r...’
 Writes the new value of n-th register received into the input buffer to the n-th register
 @param     SystemContext       Register content at time of the exception
 @param     InBuffer            This is the input buffer received from gdb server
 **/
VOID
EFIAPI
WriteNthRegister (
  IN    EFI_SYSTEM_CONTEXT  SystemContext,
  IN    CHAR8               *InBuffer
  );

/** ‘G XX...’
 Writes the new values received into the input buffer to the general registers
 @param     SystemContext               Register content at time of the exception
 @param     InBuffer                    Pointer to the input buffer received from gdb server
 **/

VOID
EFIAPI
WriteGeneralRegisters (
  IN    EFI_SYSTEM_CONTEXT  SystemContext,
  IN    CHAR8               *InBuffer
  );

/** ‘m addr,length ’
 Find the Length of the area to read and the start address. Finally, pass them to
 another function, TransferFromMemToOutBufAndSend, that will read from that memory space and
 send it as a packet.

 @param  *PacketData  Pointer to Payload data for the packet
 **/
VOID
EFIAPI
ReadFromMemory (
  IN  CHAR8  *PacketData
  );

/** ‘M addr,length :XX...’
 Find the Length of the area in bytes to write and the start address. Finally, pass them to
 another function, TransferFromInBufToMem, that will write to that memory space the info in
 the input buffer.

 @param   PacketData     Pointer to Payload data for the packet
 **/
VOID
EFIAPI
WriteToMemory (
  IN CHAR8  *PacketData
  );

/** ‘c [addr ]’
 Continue. addr is Address to resume. If addr is omitted, resume at current
 Address.

 @param SystemContext Register content at time of the exception
 @param *PacketData   Pointer to PacketData
 **/

VOID
EFIAPI
ContinueAtAddress (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *PacketData
  );

/** ‘s [addr ]’
 Single step. addr is the Address at which to resume. If addr is omitted, resume
 at same Address.

 @param SystemContext   Register content at time of the exception
 @param PacketData      Pointer to Payload data for the packet
 **/
VOID
EFIAPI
SingleStep (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *PacketData
  );

/**
 Insert Single Step in the SystemContext

 @param SystemContext   Register content at time of the exception
 **/
VOID
AddSingleStep (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
 Remove Single Step in the SystemContext

 @param SystemContext   Register content at time of the exception
 **/
VOID
RemoveSingleStep (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  );

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
  IN  CHAR8               *PacketData
  );

/**
  ‘z1, [addr], [length]’
  ‘z2, [addr], [length]’
  ‘z3, [addr], [length]’
  ‘z4, [addr], [length]’

  Remove hardware breakpoint/watchpoint at address addr of size length

  @param SystemContext  Register content at time of the exception
  @param *PacketData    Pointer to the Payload data for the packet

**/
VOID
EFIAPI
RemoveBreakPoint (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  CHAR8               *PacketData
  );

/**
 Exception Handler for GDB. It will be called for all exceptions
 registered via the gExceptionType[] array.

 @param ExceptionType   Exception that is being processed
 @param SystemContext   Register content at time of the exception

 **/
VOID
EFIAPI
GdbExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
 Periodic callback for GDB. This function is used to catch a ctrl-c or other
 break in type command from GDB.

 @param SystemContext           Register content at time of the call

 **/
VOID
EFIAPI
GdbPeriodicCallBack (
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Make two serial consoles: 1) StdIn and StdOut via GDB. 2) StdErr via GDB.

  These console show up on the remote system running GDB

**/

VOID
GdbInitializeSerialConsole (
  VOID
  );

/**
  Send a GDB Remote Serial Protocol Packet

  $PacketData#checksum PacketData is passed in and this function adds the packet prefix '$',
  the packet terminating character '#' and the two digit checksum.

  If an ack '+' is not sent resend the packet, but timeout eventually so we don't end up
  in an infinite loop. This is so if you unplug the debugger code just keeps running

  @param PacketData   Payload data for the packet

  @retval             Number of bytes of packet data sent.

**/
UINTN
SendPacket (
  IN  CHAR8  *PacketData
  );

/**
 Receive a GDB Remote Serial Protocol Packet

 $PacketData#checksum PacketData is passed in and this function adds the packet prefix '$',
 the packet terminating character '#' and the two digit checksum.

 If host re-starts sending a packet without ending the previous packet, only the last valid packet is processed.
 (In other words, if received packet is '$12345$12345$123456#checksum', only '$123456#checksum' will be processed.)

 If an ack '+' is not sent resend the packet

 @param PacketData   Payload data for the packet

 @retval             Number of bytes of packet data received.

 **/
UINTN
ReceivePacket (
  OUT  CHAR8  *PacketData,
  IN   UINTN  PacketDataSize
  );

/**
  Read data from a FileDescriptor. On success number of bytes read is returned. Zero indicates
  the end of a file. On error -1 is returned. If count is zero, GdbRead returns zero.

  @param  FileDescriptor   Device to talk to.
  @param  Buffer           Buffer to hold Count bytes that were read
  @param  Count            Number of bytes to transfer.

  @retval -1               Error
  @retval {other}          Number of bytes read.

**/
INTN
GdbRead (
  IN  INTN   FileDescriptor,
  OUT VOID   *Buffer,
  IN  UINTN  Count
  );

/**
  Write data to a FileDescriptor. On success number of bytes written is returned. Zero indicates
  nothing was written. On error -1 is returned.

  @param  FileDescriptor   Device to talk to.
  @param  Buffer           Buffer to hold Count bytes that are to be written
  @param  Count            Number of bytes to transfer.

  @retval -1               Error
  @retval {other}          Number of bytes written.

**/
INTN
GdbWrite (
  IN  INTN        FileDescriptor,
  OUT CONST VOID  *Buffer,
  IN  UINTN       Count
  );

UINTN *
FindPointerToRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber
  );

CHAR8 *
BasicReadRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber,
  IN  CHAR8               *OutBufPtr
  );

VOID
TransferFromInBufToMem (
  IN  UINTN  Length,
  IN  UINT8  *Address,
  IN  CHAR8  *NewData
  );

VOID
TransferFromMemToOutBufAndSend (
  IN  UINTN  Length,
  IN  UINT8  *Address
  );

CHAR8 *
BasicWriteRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               RegNumber,
  IN  CHAR8               *InBufPtr
  );

VOID
PrintReg (
  EFI_SYSTEM_CONTEXT  SystemContext
  );

UINTN
ParseBreakpointPacket (
  IN  CHAR8  *PacketData,
  OUT UINTN  *Type,
  OUT UINTN  *Address,
  OUT UINTN  *Length
  );

UINTN
GetBreakpointDataAddress (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               BreakpointNumber
  );

UINTN
GetBreakpointDetected (
  IN  EFI_SYSTEM_CONTEXT  SystemContext
  );

BREAK_TYPE
GetBreakpointType (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               BreakpointNumber
  );

UINTN
ConvertLengthData (
  IN  UINTN  Length
  );

EFI_STATUS
FindNextFreeDebugRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  OUT UINTN               *Register
  );

EFI_STATUS
EnableDebugRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               Register,
  IN  UINTN               Address,
  IN  UINTN               Length,
  IN  UINTN               Type
  );

EFI_STATUS
FindMatchingDebugRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               Address,
  IN  UINTN               Length,
  IN  UINTN               Type,
  OUT UINTN               *Register
  );

EFI_STATUS
DisableDebugRegister (
  IN  EFI_SYSTEM_CONTEXT  SystemContext,
  IN  UINTN               Register
  );

VOID
InitializeProcessor (
  VOID
  );

BOOLEAN
ValidateAddress (
  IN  VOID  *Address
  );

BOOLEAN
ValidateException (
  IN  EFI_EXCEPTION_TYPE     ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

#endif
