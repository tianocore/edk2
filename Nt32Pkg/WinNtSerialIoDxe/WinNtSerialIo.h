/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtSerialIo.h

Abstract:


**/

#ifndef _WIN_NT_SERIAL_IO_
#define _WIN_NT_SERIAL_IO_

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <WinNtDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/WinNtIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/SerialIo.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DevicePath.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>


#define SERIAL_MAX_BUFFER_SIZE  256
#define TIMEOUT_STALL_INTERVAL  10

typedef struct {
  UINT32  First;
  UINT32  Last;
  UINT32  Surplus;
  UINT8   Data[SERIAL_MAX_BUFFER_SIZE];
} SERIAL_DEV_FIFO;

#define WIN_NT_SERIAL_IO_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('N', 'T', 's', 'i')
typedef struct {
  UINT64                    Signature;

  //
  // Protocol data for the new handle we are going to add
  //
  EFI_HANDLE                Handle;
  EFI_SERIAL_IO_PROTOCOL    SerialIo;
  EFI_SERIAL_IO_MODE        SerialIoMode;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  //
  // Private Data
  //
  EFI_HANDLE                ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  UART_DEVICE_PATH          UartDevicePath;
  EFI_WIN_NT_THUNK_PROTOCOL *WinNtThunk;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;

  //
  // Private NT type Data;
  //
  HANDLE                    NtHandle;
  DCB                       NtDCB;
  DWORD                     NtError;
  COMSTAT                   NtComStatus;

  BOOLEAN                   SoftwareLoopbackEnable;
  BOOLEAN                   HardwareFlowControl;
  BOOLEAN                   HardwareLoopbackEnable;

  SERIAL_DEV_FIFO           Fifo;

} WIN_NT_SERIAL_IO_PRIVATE_DATA;

#define WIN_NT_SERIAL_IO_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, WIN_NT_SERIAL_IO_PRIVATE_DATA, SerialIo, WIN_NT_SERIAL_IO_PRIVATE_DATA_SIGNATURE)

//
// Global Protocol Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gWinNtSerialIoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gWinNtSerialIoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gWinNtSerialIoComponentName2;

//
// Macros to convert EFI serial types to NT serial types.
//

//
// one second
//
#define SERIAL_TIMEOUT_DEFAULT  (1000 * 1000) 
#define SERIAL_BAUD_DEFAULT     115200
#define SERIAL_FIFO_DEFAULT     14
#define SERIAL_DATABITS_DEFAULT 8
#define SERIAL_PARITY_DEFAULT   DefaultParity
#define SERIAL_STOPBITS_DEFAULT DefaultStopBits

#define SERIAL_CONTROL_MASK     (EFI_SERIAL_CLEAR_TO_SEND                | \
                                 EFI_SERIAL_DATA_SET_READY               | \
                                 EFI_SERIAL_RING_INDICATE                | \
                                 EFI_SERIAL_CARRIER_DETECT               | \
                                 EFI_SERIAL_REQUEST_TO_SEND              | \
                                 EFI_SERIAL_DATA_TERMINAL_READY          | \
                                 EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE     | \
                                 EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE     | \
                                 EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE | \
                                 EFI_SERIAL_INPUT_BUFFER_EMPTY)

#define ConvertBaud2Nt(x)       (DWORD) x
#define ConvertData2Nt(x)       (BYTE) x

#define ConvertParity2Nt(x)              \
    (BYTE) (                             \
    x == DefaultParity ? NOPARITY    :   \
    x == NoParity      ? NOPARITY    :   \
    x == EvenParity    ? EVENPARITY  :   \
    x == OddParity     ? ODDPARITY   :   \
    x == MarkParity    ? MARKPARITY  :   \
    x == SpaceParity   ? SPACEPARITY : 0 \
    )

#define ConvertStop2Nt(x)                 \
    (BYTE) (                                \
    x == DefaultParity   ? ONESTOPBIT   :   \
    x == OneFiveStopBits ? ONE5STOPBITS :   \
    x == TwoStopBits     ? TWOSTOPBITS  : 0 \
    )

#define ConvertTime2Nt(x) ((x) / 1000)

//
// 115400 baud with rounding errors
//
#define SERIAL_PORT_MAX_BAUD_RATE 115400  

#define SERIAL_PORT_MIN_BAUD_RATE           50
#define SERIAL_PORT_MAX_RECEIVE_FIFO_DEPTH  16

#define SERIAL_PORT_MIN_TIMEOUT             1         // 1 uS
#define SERIAL_PORT_MAX_TIMEOUT             100000000 // 100 seconds

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
InitializeWinNtSerialIo (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageHandle - TODO: add argument description
  SystemTable - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                - TODO: add argument description
  Handle              - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                - TODO: add argument description
  Handle              - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  Handle            - TODO: add argument description
  NumberOfChildren  - TODO: add argument description
  ChildHandleBuffer - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoReset (
  IN EFI_SERIAL_IO_PROTOCOL *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT64                 BaudRate,
  IN UINT32                 ReceiveFifoDepth,
  IN UINT32                 Timeout,
  IN EFI_PARITY_TYPE        Parity,
  IN UINT8                  DataBits,
  IN EFI_STOP_BITS_TYPE     StopBits
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  BaudRate          - TODO: add argument description
  ReceiveFifoDepth  - TODO: add argument description
  Timeout           - TODO: add argument description
  Parity            - TODO: add argument description
  DataBits          - TODO: add argument description
  StopBits          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoSetControl (
  IN EFI_SERIAL_IO_PROTOCOL *This,
  IN UINT32                 Control
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Control - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoGetControl (
  IN  EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                  *Control
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Control - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoWrite (
  IN EFI_SERIAL_IO_PROTOCOL   *This,
  IN OUT UINTN                *BufferSize,
  IN VOID                     *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
WinNtSerialIoRead (
  IN  EFI_SERIAL_IO_PROTOCOL  *This,
  IN  OUT UINTN               *BufferSize,
  OUT VOID                    *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsaSerialFifoFull (
  IN SERIAL_DEV_FIFO *Fifo
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Fifo  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsaSerialFifoEmpty (
  IN SERIAL_DEV_FIFO *Fifo
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Fifo  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
IsaSerialFifoAdd (
  IN SERIAL_DEV_FIFO *Fifo,
  IN UINT8           Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Fifo  - TODO: add argument description
  Data  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
IsaSerialFifoRemove (
  IN  SERIAL_DEV_FIFO *Fifo,
  OUT UINT8           *Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Fifo  - TODO: add argument description
  Data  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
IsaSerialReceiveTransmit (
  WIN_NT_SERIAL_IO_PRIVATE_DATA     *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
