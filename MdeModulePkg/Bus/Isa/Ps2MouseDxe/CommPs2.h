/** @file
  PS2 Mouse Communication Interface

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _COMMPS2_H_
#define _COMMPS2_H_

#include "Ps2Mouse.h"

#define PS2_PACKET_LENGTH  3
#define PS2_SYNC_MASK      0xc
#define PS2_SYNC_BYTE      0x8

#define IS_PS2_SYNC_BYTE(byte)  ((byte & PS2_SYNC_MASK) == PS2_SYNC_BYTE)

#define PS2_READ_BYTE_ONE   0
#define PS2_READ_DATA_BYTE  1
#define PS2_PROCESS_PACKET  2

#define TIMEOUT      50000
#define BAT_TIMEOUT  500000

//
// 8042 I/O Port
//
#define KBC_DATA_PORT     0x60
#define KBC_CMD_STS_PORT  0x64

//
// 8042 Command
//
#define READ_CMD_BYTE   0x20
#define WRITE_CMD_BYTE  0x60
#define DISABLE_AUX     0xa7
#define ENABLE_AUX      0xa8
#define SELF_TEST       0xaa
#define DISABLE_KB      0xad
#define ENABLE_KB       0xae
#define WRITE_AUX_DEV   0xd4

#define CMD_SYS_FLAG  0x04
#define CMD_KB_STS    0x10
#define CMD_KB_DIS    0x10
#define CMD_KB_EN     0x0

//
// 8042 Auxiliary Device Command
//
#define SETSF1_CMD   0xe6
#define SETSF2_CMD   0xe7
#define SETRE_CMD    0xe8
#define READ_CMD     0xeb
#define SETRM_CMD    0xf0
#define SETSR_CMD    0xf3
#define ENABLE_CMD   0xf4
#define DISABLE_CMD  0xf5
#define RESET_CMD    0xff

//
// return code
//
#define PS2_ACK        0xfa
#define PS2_RESEND     0xfe
#define PS2MOUSE_BAT1  0xaa
#define PS2MOUSE_BAT2  0x0

//
// Keyboard Controller Status
//
///
/// Parity Error
///
#define KBC_PARE  0x80
///
/// General Time Out
///
#define KBC_TIM  0x40
///
/// Output buffer for auxiliary device (PS/2):
///    0 - Holds keyboard data
///    1 - Holds data for auxiliary device
///
#define KBC_AUXB  0x20
///
/// Keyboard lock status:
///    0 - keyboard locked
///    1 - keyboard free
///
#define KBC_KEYL  0x10
///
/// Command/Data:
///    0 - data byte written via port 60h
///    1 - command byte written via port 64h
///
#define KBC_CD  0x08
///
/// System Flag:
///    0 - power-on reset
///    1 - self-test successful
///
#define KBC_SYSF  0x04
///
/// Input Buffer Status :
///    0 - input buffer empty
///    1 - CPU data in input buffer
///
#define KBC_INPB  0x02
///
/// Output Buffer Status :
///    0 - output buffer empty
///    1 - keyboard controller data in output buffer
///
#define KBC_OUTB  0x01

/**
  Issue self test command via IsaIo interface.

  @return EFI_SUCCESS  Success to do keyboard self testing.
  @return others       Fail to do keyboard self testing.
**/
EFI_STATUS
KbcSelfTest (
  VOID
  );

/**
  Issue command to enable keyboard AUX functionality.

  @return Status of command issuing.
**/
EFI_STATUS
KbcEnableAux (
  VOID
  );

/**
  Issue command to disable keyboard AUX functionality.

  @return Status of command issuing.
**/
EFI_STATUS
KbcDisableAux (
  VOID
  );

/**
  Issue command to enable keyboard.

  @return Status of command issuing.
**/
EFI_STATUS
KbcEnableKb (
  VOID
  );

/**
  Issue command to disable keyboard.

  @return Status of command issuing.
**/
EFI_STATUS
KbcDisableKb (
  VOID
  );

/**
  Issue command to check keyboard status.

  @param KeyboardEnable return whether keyboard is enable.

  @return Status of command issuing.
**/
EFI_STATUS
CheckKbStatus (
  OUT BOOLEAN  *KeyboardEnable
  );

/**
  Issue command to reset keyboard.

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseReset (
  VOID
  );

/**
  Issue command to set mouse's sample rate

  @param SampleRate value of sample rate

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetSampleRate (
  IN MOUSE_SR  SampleRate
  );

/**
  Issue command to set mouse's resolution.

  @param Resolution value of resolution

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetResolution (
  IN MOUSE_RE  Resolution
  );

/**
  Issue command to set mouse's scaling.

  @param Scaling value of scaling

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseSetScaling (
  IN MOUSE_SF  Scaling
  );

/**
  Issue command to enable Ps2 mouse.

  @return Status of command issuing.
**/
EFI_STATUS
PS2MouseEnable (
  VOID
  );

/**
  Get mouse packet . Only care first 3 bytes

  @param MouseDev  Pointer of PS2 Mouse Private Data Structure

  @retval EFI_NOT_READY  Mouse Device not ready to input data packet, or some error happened during getting the packet
  @retval EFI_SUCCESS    The data packet is gotten successfully.

**/
EFI_STATUS
PS2MouseGetPacket (
  PS2_MOUSE_DEV  *MouseDev
  );

/**
  Read data via IsaIo protocol with given number.

  @param Buffer  Buffer receive data of mouse
  @param BufSize The size of buffer
  @param State   Check input or read data

  @return status of reading mouse data.
**/
EFI_STATUS
PS2MouseRead (
  OUT UINT8     *Buffer,
  IN OUT UINTN  *BufSize,
  IN  UINTN     State
  );

//
// 8042 I/O function
//

/**
  I/O work flow of outing 8042 command.

  @param Command I/O command.

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042Command (
  IN UINT8  Command
  );

/**
  I/O work flow of in 8042 data.

  @param Data    Data value

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
In8042Data (
  IN OUT UINT8  *Data
  );

/**
  I/O work flow of outing 8042 data.

  @param Data    Data value

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042Data (
  IN UINT8  Data
  );

/**
  I/O work flow of outing 8042 Aux command.

  @param Command Aux I/O command
  @param Resend  Whether need resend the Aux command.

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042AuxCommand (
  IN UINT8    Command,
  IN BOOLEAN  Resend
  );

/**
  I/O work flow of in 8042 Aux data.

  @param Data    Buffer holding return value.

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
In8042AuxData (
  IN OUT UINT8  *Data
  );

/**
  I/O work flow of outing 8042 Aux data.

  @param Data    Buffer holding return value

  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
Out8042AuxData (
  IN UINT8  Data
  );

/**
  Check keyboard controller status, if it is output buffer full and for auxiliary device.

  @retval EFI_SUCCESS   Keyboard controller is ready
  @retval EFI_NOT_READY Keyboard controller is not ready
**/
EFI_STATUS
CheckForInput (
  VOID
  );

/**
  I/O work flow to wait input buffer empty in given time.

  @param Timeout Waiting time.

  @retval EFI_TIMEOUT if input is still not empty in given time.
  @retval EFI_SUCCESS input is empty.
**/
EFI_STATUS
WaitInputEmpty (
  IN UINTN  Timeout
  );

/**
  I/O work flow to wait output buffer full in given time.

  @param Timeout given time

  @retval EFI_TIMEOUT  output is not full in given time
  @retval EFI_SUCCESS  output is full in given time.
**/
EFI_STATUS
WaitOutputFull (
  IN UINTN  Timeout
  );

#endif
