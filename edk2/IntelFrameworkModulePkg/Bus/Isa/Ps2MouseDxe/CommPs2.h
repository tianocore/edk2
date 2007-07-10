/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  CommPs2.h
    
Abstract:   

  PS2 Mouse Communication Interface 


Revision History

--*/

#ifndef _COMMPS2_H_
#define _COMMPS2_H_

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#define PS2_PACKET_LENGTH       3
#define PS2_SYNC_MASK           0xc
#define PS2_SYNC_BYTE           0x8

#define IS_PS2_SYNC_BYTE(byte)  ((byte & PS2_SYNC_MASK) == PS2_SYNC_BYTE)

#define PS2_READ_BYTE_ONE       0
#define PS2_READ_DATA_BYTE      1
#define PS2_PROCESS_PACKET      2

#define TIMEOUT                 50000
#define BAT_TIMEOUT             500000

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

#define CMD_SYS_FLAG    0x04
#define CMD_KB_STS      0x10
#define CMD_KB_DIS      0x10
#define CMD_KB_EN       0x0

//
// 8042 Auxiliary Device Command
//
#define SETSF1_CMD  0xe6
#define SETSF2_CMD  0xe7
#define SETRE_CMD   0xe8
#define READ_CMD    0xeb
#define SETRM_CMD   0xf0
#define SETSR_CMD   0xf3
#define ENABLE_CMD  0xf4
#define DISABLE_CMD 0xf5
#define RESET_CMD   0xff

//
// return code
//
#define PS2_ACK       0xfa
#define PS2_RESEND    0xfe
#define PS2MOUSE_BAT1 0xaa
#define PS2MOUSE_BAT2 0x0

//
// Keyboard Controller Status
//
#define KBC_PARE  0x80  // Parity Error
#define KBC_TIM   0x40  // General Time Out
#define KBC_AUXB  0x20  // Output buffer for auxiliary device (PS/2):
//    0 - Holds keyboard data
//    1 - Holds data for auxiliary device
//
#define KBC_KEYL  0x10  // Keyboard lock status:
//    0 - keyboard locked
//    1 - keyboard free
//
#define KBC_CD  0x08  // Command/Data:
//    0 - data byte written via port 60h
//    1 - command byte written via port 64h
//
#define KBC_SYSF  0x04  // System Flag:
//    0 - power-on reset
//    1 - self-test successful
//
#define KBC_INPB  0x02  // Input Buffer Status :
//    0 - input buffer empty
//    1 - CPU data in input buffer
//
#define KBC_OUTB  0x01  // Output Buffer Status :
//    0 - output buffer empty
//    1 - keyboard controller data in output buffer
//
EFI_STATUS
KbcSelfTest (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
KbcEnableAux (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
KbcDisableAux (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
KbcEnableKb (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
KbcDisableKb (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
CheckKbStatus (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  OUT BOOLEAN                             *KeyboardEnable
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo           - GC_TODO: add argument description
  KeyboardEnable  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
PS2MouseReset (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
PS2MouseSetSampleRate (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN MOUSE_SR                             SampleRate
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo       - GC_TODO: add argument description
  SampleRate  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
PS2MouseSetResolution (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN MOUSE_RE                             Resolution
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo       - GC_TODO: add argument description
  Resolution  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
PS2MouseSetScaling (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN MOUSE_SF                             Scaling
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo   - GC_TODO: add argument description
  Scaling - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
PS2MouseEnable (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  IsaIo - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
PS2MouseGetPacket (
  PS2_MOUSE_DEV     *MouseDev
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  MouseDev  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
PS2MouseRead (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  OUT VOID                                *Buffer,
  IN OUT UINTN                            *BufSize,
  IN  UINTN                               State
  );

//
// 8042 I/O function
//
EFI_STATUS
Out8042Command (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Command
  );

EFI_STATUS
In8042Data (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN OUT UINT8                            *Data
  );

EFI_STATUS
Out8042Data (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Data
  );

EFI_STATUS
Out8042AuxCommand (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Command,
  IN BOOLEAN                              Resend
  );

EFI_STATUS
In8042AuxData (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN OUT UINT8                            *Data
  );

EFI_STATUS
Out8042AuxData (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINT8                                Data
  );

EFI_STATUS
CheckForInput (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo
  );

EFI_STATUS
WaitInputEmpty (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINTN                                Timeout
  );

EFI_STATUS
WaitOutputFull (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN UINTN                                Timeout
  );

#endif // _COMMPS2_H_
