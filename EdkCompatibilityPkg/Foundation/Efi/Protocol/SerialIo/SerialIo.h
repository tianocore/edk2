/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SerialIo.h

Abstract:

  Serial IO protocol as defined in the EFI 1.0 specification.

  Abstraction of a basic serial device. Targeted at 16550 UART, but
  could be much more generic.

--*/

#ifndef _SERIAL_IO_H_
#define _SERIAL_IO_H_

#define EFI_SERIAL_IO_PROTOCOL_GUID \
  { \
    0xBB25CF6F, 0xF1D4, 0x11D2, {0x9A, 0x0C, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0xFD} \
  }

EFI_FORWARD_DECLARATION (EFI_SERIAL_IO_PROTOCOL);

//
// Serial IO Data structures
//
typedef enum {
  DefaultParity,
  NoParity,
  EvenParity,
  OddParity,
  MarkParity,
  SpaceParity
} EFI_PARITY_TYPE;

typedef enum {
  DefaultStopBits,
  OneStopBit,
  OneFiveStopBits,
  TwoStopBits
} EFI_STOP_BITS_TYPE;

//
// define for Control bits, grouped by read only, write only, and read write
//
//
// Read Only
//
#define EFI_SERIAL_CLEAR_TO_SEND        0x00000010
#define EFI_SERIAL_DATA_SET_READY       0x00000020
#define EFI_SERIAL_RING_INDICATE        0x00000040
#define EFI_SERIAL_CARRIER_DETECT       0x00000080
#define EFI_SERIAL_INPUT_BUFFER_EMPTY   0x00000100
#define EFI_SERIAL_OUTPUT_BUFFER_EMPTY  0x00000200

//
// Write Only
//
#define EFI_SERIAL_REQUEST_TO_SEND      0x00000002
#define EFI_SERIAL_DATA_TERMINAL_READY  0x00000001

//
// Read Write
//
#define EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE     0x00001000
#define EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE     0x00002000
#define EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE 0x00004000

//
// Serial IO Member Functions
//
typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_RESET) (
  IN EFI_SERIAL_IO_PROTOCOL * This
  )
/*++

  Routine Description:
    Reset the serial device.

  Arguments:
    This     - Protocol instance pointer.

  Returns:
    EFI_SUCCESS      - The device was reset.
    EFI_DEVICE_ERROR - The serial device could not be reset.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_SET_ATTRIBUTES) (
  IN EFI_SERIAL_IO_PROTOCOL         * This,
  IN UINT64                         BaudRate,
  IN UINT32                         ReceiveFifoDepth,
  IN UINT32                         Timeout,
  IN EFI_PARITY_TYPE                Parity,
  IN UINT8                          DataBits,
  IN EFI_STOP_BITS_TYPE             StopBits
  )
/*++

  Routine Description:
    Sets the baud rate, receive FIFO depth, transmit/receice time out, parity, 
    data buts, and stop bits on a serial device.

  Arguments:
    This     - Protocol instance pointer.
    BaudRate - The requested baud rate. A BaudRate value of 0 will use the the
                device's default interface speed.
    ReveiveFifoDepth - The requested depth of the FIFO on the receive side of the
                       serial interface. A ReceiveFifoDepth value of 0 will use 
                       the device's dfault FIFO depth.
    Timeout - The requested time out for a single character in microseconds. 
              This timeout applies to both the transmit and receive side of the
              interface. A Timeout value of 0 will use the device's default time
              out value.
    Parity  - The type of parity to use on this serial device. A Parity value of
               DefaultParity will use the device's default parity value. 
    DataBits - The number of data bits to use on the serial device. A DataBits
                vaule of 0 will use the device's default data bit setting.
    StopBits - The number of stop bits to use on this serial device. A StopBits
                value of DefaultStopBits will use the device's default number of
                stop bits. 

  Returns:
    EFI_SUCCESS      - The device was reset.
    EFI_DEVICE_ERROR - The serial device could not be reset.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_SET_CONTROL_BITS) (
  IN EFI_SERIAL_IO_PROTOCOL         * This,
  IN UINT32                         Control
  )
/*++

  Routine Description:
    Set the control bits on a serial device

  Arguments:
    This    - Protocol instance pointer.
    Control - Set the bits of Control that are settable.

  Returns:
    EFI_SUCCESS      - The new control bits were set on the serial device.
    EFI_UNSUPPORTED  - The serial device does not support this operation.
    EFI_DEVICE_ERROR - The serial device is not functioning correctly.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_GET_CONTROL_BITS) (
  IN EFI_SERIAL_IO_PROTOCOL         * This,
  OUT UINT32                        *Control
  )
/*++

  Routine Description:
    Retrieves the status of thecontrol bits on a serial device

  Arguments:
    This    - Protocol instance pointer.
    Control - A pointer to return the current Control signals from the serial
               device.

  Returns:
    EFI_SUCCESS      - The control bits were read from the serial device.
    EFI_DEVICE_ERROR - The serial device is not functioning correctly.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_WRITE) (
  IN EFI_SERIAL_IO_PROTOCOL         * This,
  IN OUT UINTN                      *BufferSize,
  IN VOID                           *Buffer
  )
/*++

  Routine Description:
    Writes data to a serial device.

  Arguments:
    This    - Protocol instance pointer.
    BufferSize - On input, the size of the Buffer. On output, the amount of 
                  data actually written.
    Buffer - The buffer of data to write

  Returns:
    EFI_SUCCESS      - The data was written.
    EFI_DEVICE_ERROR - The device reported an error.
    EFI_TIMEOUT      - The data write was stopped due to a timeout.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_READ) (
  IN EFI_SERIAL_IO_PROTOCOL         * This,
  IN OUT UINTN                      *BufferSize,
  OUT VOID                          *Buffer
  )
/*++

  Routine Description:
    Writes data to a serial device.

  Arguments:
    This    - Protocol instance pointer.
    BufferSize - On input, the size of the Buffer. On output, the amount of 
                  data returned in Buffer.
    Buffer - The buffer to return the data into.

  Returns:
    EFI_SUCCESS      - The data was read.
    EFI_DEVICE_ERROR - The device reported an error.
    EFI_TIMEOUT      - The data write was stopped due to a timeout.

--*/
;

/*++

  The data values in SERIAL_IO_MODE are read-only and are updated by the code 
  that produces the SERIAL_IO_PROTOCOL member functions.

  ControlMask - A mask fo the Control bits that the device supports. The device
                 must always support the Input Buffer Empty control bit.
  TimeOut  - If applicable, the number of microseconds to wait before timing out
              a Read or Write operation.
  BaudRate - If applicable, the current baud rate setting of the device; otherwise,
              baud rate has the value of zero to indicate that device runs at the
              device's designed speed.
  ReceiveFifoDepth - The number of characters the device will buffer on input
  DataBits - The number of characters the device will buffer on input
  Parity   - If applicable, this is the EFI_PARITY_TYPE that is computed or 
             checked as each character is transmitted or reveived. If the device
             does not support parity the value is the default parity value.
  StopBits - If applicable, the EFI_STOP_BITS_TYPE number of stop bits per
              character. If the device does not support stop bits the value is
              the default stop bit values.

--*/
typedef struct {
  UINT32  ControlMask;

  //
  // current Attributes
  //
  UINT32  Timeout;
  UINT64  BaudRate;
  UINT32  ReceiveFifoDepth;
  UINT32  DataBits;
  UINT32  Parity;
  UINT32  StopBits;
} EFI_SERIAL_IO_MODE;

#define SERIAL_IO_INTERFACE_REVISION  0x00010000

struct _EFI_SERIAL_IO_PROTOCOL {
  UINT32                      Revision;
  EFI_SERIAL_RESET            Reset;
  EFI_SERIAL_SET_ATTRIBUTES   SetAttributes;
  EFI_SERIAL_SET_CONTROL_BITS SetControl;
  EFI_SERIAL_GET_CONTROL_BITS GetControl;
  EFI_SERIAL_WRITE            Write;
  EFI_SERIAL_READ             Read;

  EFI_SERIAL_IO_MODE          *Mode;
};

extern EFI_GUID gEfiSerialIoProtocolGuid;

#endif
