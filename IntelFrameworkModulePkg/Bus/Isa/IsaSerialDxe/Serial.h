/**@file
  Include for Serial Driver
  
Copyright (c) 2006 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SERIAL_H
#define _SERIAL_H


#include <PiDxe.h>
#include <FrameworkPei.h>

#include <Protocol/IsaIo.h>
#include <Protocol/SerialIo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL  gSerialControllerDriver;
extern EFI_COMPONENT_NAME_PROTOCOL  gIsaSerialComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gIsaSerialComponentName2;

//
// Internal Data Structures
//
#define SERIAL_DEV_SIGNATURE    EFI_SIGNATURE_32 ('s', 'e', 'r', 'd')
#define SERIAL_MAX_BUFFER_SIZE  16
#define TIMEOUT_STALL_INTERVAL  10

//
//  Name:   SERIAL_DEV_FIFO
//  Purpose:  To define Receive FIFO and Transmit FIFO
//  Context:  Used by serial data transmit and receive
//  Fields:
//      First UINT32: The index of the first data in array Data[]
//      Last  UINT32: The index, which you can put a new data into array Data[]
//      Surplus UINT32: Identify how many data you can put into array Data[]
//      Data[]  UINT8 : An array, which used to store data
//
typedef struct {
  UINT32  First;
  UINT32  Last;
  UINT32  Surplus;
  UINT8   Data[SERIAL_MAX_BUFFER_SIZE];
} SERIAL_DEV_FIFO;

typedef enum {
  UART8250  = 0,
  UART16450 = 1,
  UART16550 = 2,
  UART16550A= 3
} EFI_UART_TYPE;

//
//  Name:   SERIAL_DEV
//  Purpose:  To provide device specific information
//  Context:
//  Fields:
//      Signature UINTN: The identity of the serial device
//      SerialIo  SERIAL_IO_PROTOCOL: Serial I/O protocol interface
//      SerialMode  SERIAL_IO_MODE:
//      DevicePath  EFI_DEVICE_PATH_PROTOCOL *: Device path of the serial device
//      Handle      EFI_HANDLE: The handle instance attached to serial device
//      BaseAddress UINT16: The base address of specific serial device
//      Receive     SERIAL_DEV_FIFO: The FIFO used to store data,
//                  which is received by UART
//      Transmit    SERIAL_DEV_FIFO: The FIFO used to store data,
//                  which you want to transmit by UART
//      SoftwareLoopbackEnable BOOLEAN:
//      Type    EFI_UART_TYPE: Specify the UART type of certain serial device
//
typedef struct {
  UINTN                                  Signature;

  EFI_HANDLE                             Handle;
  EFI_SERIAL_IO_PROTOCOL                 SerialIo;
  EFI_SERIAL_IO_MODE                     SerialMode;
  EFI_DEVICE_PATH_PROTOCOL               *DevicePath;

  EFI_DEVICE_PATH_PROTOCOL               *ParentDevicePath;
  UART_DEVICE_PATH                       UartDevicePath;
  EFI_ISA_IO_PROTOCOL                    *IsaIo;

  UINT16                                 BaseAddress;
  SERIAL_DEV_FIFO                        Receive;
  SERIAL_DEV_FIFO                        Transmit;
  BOOLEAN                                SoftwareLoopbackEnable;
  BOOLEAN                                HardwareFlowControl;
  EFI_UART_TYPE                          Type;
  EFI_UNICODE_STRING_TABLE               *ControllerNameTable;
} SERIAL_DEV;

#include "ComponentName.h"

#define SERIAL_DEV_FROM_THIS(a) CR (a, SERIAL_DEV, SerialIo, SERIAL_DEV_SIGNATURE)

//
// Globale Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gSerialControllerDriver;

//
// Serial Driver Defaults
//
#define SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH  1
#define SERIAL_PORT_DEFAULT_TIMEOUT             1000000

/*
#define SERIAL_PORT_DEFAULT_BAUD_RATE           115200
#define SERIAL_PORT_DEFAULT_PARITY              NoParity
#define SERIAL_PORT_DEFAULT_DATA_BITS           8
#define SERIAL_PORT_DEFAULT_STOP_BITS           1
*/
#define SERIAL_PORT_DEFAULT_CONTROL_MASK        0


//
// (24000000/13)MHz input clock
//
#define SERIAL_PORT_INPUT_CLOCK 1843200

//
// 115200 baud with rounding errors
//
#define SERIAL_PORT_MAX_BAUD_RATE           115400
#define SERIAL_PORT_MIN_BAUD_RATE           50

#define SERIAL_PORT_MAX_RECEIVE_FIFO_DEPTH  16
#define SERIAL_PORT_MIN_TIMEOUT             1         // 1 uS
#define SERIAL_PORT_MAX_TIMEOUT             100000000 // 100 seconds
//
// UART Registers
//
#define SERIAL_REGISTER_THR 0 // WO   Transmit Holding Register
#define SERIAL_REGISTER_RBR 0 // RO   Receive Buffer Register
#define SERIAL_REGISTER_DLL 0 // R/W  Divisor Latch LSB
#define SERIAL_REGISTER_DLM 1 // R/W  Divisor Latch MSB
#define SERIAL_REGISTER_IER 1 // R/W  Interrupt Enable Register
#define SERIAL_REGISTER_IIR 2 // RO   Interrupt Identification Register
#define SERIAL_REGISTER_FCR 2 // WO   FIFO Cotrol Register
#define SERIAL_REGISTER_LCR 3 // R/W  Line Control Register
#define SERIAL_REGISTER_MCR 4 // R/W  Modem Control Register
#define SERIAL_REGISTER_LSR 5 // R/W  Line Status Register
#define SERIAL_REGISTER_MSR 6 // R/W  Modem Status Register
#define SERIAL_REGISTER_SCR 7 // R/W  Scratch Pad Register
#pragma pack(1)
//
//  Name:   SERIAL_PORT_IER_BITS
//  Purpose:  Define each bit in Interrupt Enable Register
//  Context:
//  Fields:
//     RAVIE  Bit0: Receiver Data Available Interrupt Enable
//     THEIE  Bit1: Transmistter Holding Register Empty Interrupt Enable
//     RIE      Bit2: Receiver Interrupt Enable
//     MIE      Bit3: Modem Interrupt Enable
//     Reserved Bit4-Bit7: Reserved
//
typedef struct {
  UINT8 RAVIE : 1;
  UINT8 THEIE : 1;
  UINT8 RIE : 1;
  UINT8 MIE : 1;
  UINT8 Reserved : 4;
} SERIAL_PORT_IER_BITS;

//
//  Name:   SERIAL_PORT_IER
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_IER_BITS:  Bits of the IER
//      Data    UINT8: the value of the IER
//
typedef union {
  SERIAL_PORT_IER_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_IER;

//
//  Name:   SERIAL_PORT_IIR_BITS
//  Purpose:  Define each bit in Interrupt Identification Register
//  Context:
//  Fields:
//      IPS    Bit0: Interrupt Pending Status
//      IIB    Bit1-Bit3: Interrupt ID Bits
//      Reserved Bit4-Bit5: Reserved
//      FIFOES   Bit6-Bit7: FIFO Mode Enable Status
//
typedef struct {
  UINT8 IPS : 1;
  UINT8 IIB : 3;
  UINT8 Reserved : 2;
  UINT8 FIFOES : 2;
} SERIAL_PORT_IIR_BITS;

//
//  Name:   SERIAL_PORT_IIR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_IIR_BITS:  Bits of the IIR
//      Data    UINT8: the value of the IIR
//
typedef union {
  SERIAL_PORT_IIR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_IIR;

//
//  Name:   SERIAL_PORT_FCR_BITS
//  Purpose:  Define each bit in FIFO Control Register
//  Context:
//  Fields:
//      TRFIFOE    Bit0: Transmit and Receive FIFO Enable
//      RESETRF    Bit1: Reset Reciever FIFO
//      RESETTF    Bit2: Reset Transmistter FIFO
//      DMS        Bit3: DMA Mode Select
//      Reserved   Bit4-Bit5: Reserved
//      RTB        Bit6-Bit7: Receive Trigger Bits
//
typedef struct {
  UINT8 TRFIFOE : 1;
  UINT8 RESETRF : 1;
  UINT8 RESETTF : 1;
  UINT8 DMS : 1;
  UINT8 Reserved : 2;
  UINT8 RTB : 2;
} SERIAL_PORT_FCR_BITS;

//
//  Name:   SERIAL_PORT_FCR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_FCR_BITS:  Bits of the FCR
//      Data    UINT8: the value of the FCR
//
typedef union {
  SERIAL_PORT_FCR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_FCR;

//
//  Name:   SERIAL_PORT_LCR_BITS
//  Purpose:  Define each bit in Line Control Register
//  Context:
//  Fields:
//      SERIALDB  Bit0-Bit1: Number of Serial Data Bits
//      STOPB   Bit2: Number of Stop Bits
//      PAREN   Bit3: Parity Enable
//      EVENPAR   Bit4: Even Parity Select
//      STICPAR   Bit5: Sticky Parity
//      BRCON   Bit6: Break Control
//      DLAB    Bit7: Divisor Latch Access Bit
//
typedef struct {
  UINT8 SERIALDB : 2;
  UINT8 STOPB : 1;
  UINT8 PAREN : 1;
  UINT8 EVENPAR : 1;
  UINT8 STICPAR : 1;
  UINT8 BRCON : 1;
  UINT8 DLAB : 1;
} SERIAL_PORT_LCR_BITS;

//
//  Name:   SERIAL_PORT_LCR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_LCR_BITS:  Bits of the LCR
//      Data    UINT8: the value of the LCR
//
typedef union {
  SERIAL_PORT_LCR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_LCR;

//
//  Name:   SERIAL_PORT_MCR_BITS
//  Purpose:  Define each bit in Modem Control Register
//  Context:
//  Fields:
//      DTRC     Bit0: Data Terminal Ready Control
//      RTS      Bit1: Request To Send Control
//      OUT1     Bit2: Output1
//      OUT2     Bit3: Output2, used to disable interrupt
//      LME;     Bit4: Loopback Mode Enable
//      Reserved Bit5-Bit7: Reserved
//
typedef struct {
  UINT8 DTRC : 1;
  UINT8 RTS : 1;
  UINT8 OUT1 : 1;
  UINT8 OUT2 : 1;
  UINT8 LME : 1;
  UINT8 Reserved : 3;
} SERIAL_PORT_MCR_BITS;

//
//  Name:   SERIAL_PORT_MCR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_MCR_BITS:  Bits of the MCR
//      Data    UINT8: the value of the MCR
//
typedef union {
  SERIAL_PORT_MCR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_MCR;

//
//  Name:   SERIAL_PORT_LSR_BITS
//  Purpose:  Define each bit in Line Status Register
//  Context:
//  Fields:
//      DR    Bit0: Receiver Data Ready Status
//      OE    Bit1: Overrun Error Status
//      PE    Bit2: Parity Error Status
//      FE    Bit3: Framing Error Status
//      BI    Bit4: Break Interrupt Status
//      THRE  Bit5: Transmistter Holding Register Status
//      TEMT  Bit6: Transmitter Empty Status
//      FIFOE Bit7: FIFO Error Status
//
typedef struct {
  UINT8 DR : 1;
  UINT8 OE : 1;
  UINT8 PE : 1;
  UINT8 FE : 1;
  UINT8 BI : 1;
  UINT8 THRE : 1;
  UINT8 TEMT : 1;
  UINT8 FIFOE : 1;
} SERIAL_PORT_LSR_BITS;

//
//  Name:   SERIAL_PORT_LSR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_LSR_BITS:  Bits of the LSR
//      Data    UINT8: the value of the LSR
//
typedef union {
  SERIAL_PORT_LSR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_LSR;

//
//  Name:   SERIAL_PORT_MSR_BITS
//  Purpose:  Define each bit in Modem Status Register
//  Context:
//  Fields:
//      DeltaCTS      Bit0: Delta Clear To Send Status
//      DeltaDSR        Bit1: Delta Data Set Ready Status
//      TrailingEdgeRI  Bit2: Trailing Edge of Ring Indicator Status
//      DeltaDCD        Bit3: Delta Data Carrier Detect Status
//      CTS             Bit4: Clear To Send Status
//      DSR             Bit5: Data Set Ready Status
//      RI              Bit6: Ring Indicator Status
//      DCD             Bit7: Data Carrier Detect Status
//
typedef struct {
  UINT8 DeltaCTS : 1;
  UINT8 DeltaDSR : 1;
  UINT8 TrailingEdgeRI : 1;
  UINT8 DeltaDCD : 1;
  UINT8 CTS : 1;
  UINT8 DSR : 1;
  UINT8 RI : 1;
  UINT8 DCD : 1;
} SERIAL_PORT_MSR_BITS;

//
//  Name:   SERIAL_PORT_MSR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_MSR_BITS:  Bits of the MSR
//      Data    UINT8: the value of the MSR
//
typedef union {
  SERIAL_PORT_MSR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_MSR;

#pragma pack()
//
// Define serial register I/O macros
//
#define READ_RBR(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_RBR)
#define READ_DLL(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_DLL)
#define READ_DLM(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_DLM)
#define READ_IER(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_IER)
#define READ_IIR(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_IIR)
#define READ_LCR(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_LCR)
#define READ_MCR(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_MCR)
#define READ_LSR(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_LSR)
#define READ_MSR(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_MSR)
#define READ_SCR(IO, B)     IsaSerialReadPort (IO, B, SERIAL_REGISTER_SCR)

#define WRITE_THR(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_THR, D)
#define WRITE_DLL(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_DLL, D)
#define WRITE_DLM(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_DLM, D)
#define WRITE_IER(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_IER, D)
#define WRITE_FCR(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_FCR, D)
#define WRITE_LCR(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_LCR, D)
#define WRITE_MCR(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_MCR, D)
#define WRITE_LSR(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_LSR, D)
#define WRITE_MSR(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_MSR, D)
#define WRITE_SCR(IO, B, D) IsaSerialWritePort (IO, B, SERIAL_REGISTER_SCR, D)

//
// Prototypes
// Driver model protocol interface
//

EFI_STATUS
EFIAPI
SerialControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SerialControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SerialControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  );

//
// Serial I/O Protocol Interface
//
EFI_STATUS
EFIAPI
IsaSerialReset (
  IN EFI_SERIAL_IO_PROTOCOL         *This
  );

EFI_STATUS
EFIAPI
IsaSerialSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN UINT64                         BaudRate,
  IN UINT32                         ReceiveFifoDepth,
  IN UINT32                         Timeout,
  IN EFI_PARITY_TYPE                Parity,
  IN UINT8                          DataBits,
  IN EFI_STOP_BITS_TYPE             StopBits
  );

EFI_STATUS
EFIAPI
IsaSerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN UINT32                         Control
  );

EFI_STATUS
EFIAPI
IsaSerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  OUT UINT32                        *Control
  );

EFI_STATUS
EFIAPI
IsaSerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN OUT UINTN                      *BufferSize,
  IN VOID                           *Buffer
  );

EFI_STATUS
EFIAPI
IsaSerialRead (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN OUT UINTN                      *BufferSize,
  OUT VOID                          *Buffer
  );

//
// Internal Functions
//
BOOLEAN
IsaSerialPortPresent (
  IN SERIAL_DEV                     *SerialDevice
  );

BOOLEAN
IsaSerialFifoFull (
  IN SERIAL_DEV_FIFO                *Fifo
  );

BOOLEAN
IsaSerialFifoEmpty (
  IN SERIAL_DEV_FIFO                *Fifo
  );

EFI_STATUS
IsaSerialFifoAdd (
  IN SERIAL_DEV_FIFO                *Fifo,
  IN UINT8                          Data
  );

EFI_STATUS
IsaSerialFifoRemove (
  IN  SERIAL_DEV_FIFO               *Fifo,
  OUT UINT8                         *Data
  );

EFI_STATUS
IsaSerialReceiveTransmit (
  IN SERIAL_DEV                     *SerialDevice
  );

UINT8
IsaSerialReadPort (
  IN EFI_ISA_IO_PROTOCOL                    *IsaIo,
  IN UINT16                                 BaseAddress,
  IN UINT32                                 Offset
  );

VOID
IsaSerialWritePort (
  IN EFI_ISA_IO_PROTOCOL                    *IsaIo,
  IN UINT16                                 BaseAddress,
  IN UINT32                                 Offset,
  IN UINT8                                  Data
  );

#endif
