/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  DebugPort.h

Abstract:
  Definitions and prototypes for DebugPort driver

--*/

#ifndef __DEBUGPORT_H__
#define __DEBUGPORT_H__


//
// local type definitions
//
#define DEBUGPORT_DEVICE_SIGNATURE  EFI_SIGNATURE_32 ('D', 'B', 'G', 'P')

//
// Device structure used by driver
//
typedef struct {
  UINT32                      Signature;
  EFI_HANDLE                  DriverBindingHandle;
  EFI_HANDLE                  DebugPortDeviceHandle;
  VOID                        *DebugPortVariable;

  EFI_DRIVER_BINDING_PROTOCOL DriverBindingInterface;
  EFI_COMPONENT_NAME_PROTOCOL ComponentNameInterface;
  EFI_DEVICE_PATH_PROTOCOL    *DebugPortDevicePath;
  EFI_DEBUGPORT_PROTOCOL      DebugPortInterface;

  EFI_HANDLE                  SerialIoDeviceHandle;
  EFI_SERIAL_IO_PROTOCOL      *SerialIoBinding;
  UINT64                      BaudRate;
  UINT32                      ReceiveFifoDepth;
  UINT32                      Timeout;
  EFI_PARITY_TYPE             Parity;
  UINT8                       DataBits;
  EFI_STOP_BITS_TYPE          StopBits;
} DEBUGPORT_DEVICE;

#define DEBUGPORT_DEVICE_FROM_THIS(a)     CR (a, DEBUGPORT_DEVICE, DebugPortInterface, DEBUGPORT_DEVICE_SIGNATURE)

#define EFI_ACPI_PC_COMPORT_HID           EISA_PNP_ID (0x0500)
#define EFI_ACPI_16550UART_HID            EISA_PNP_ID (0x0501)

#define DEBUGPORT_UART_DEFAULT_BAUDRATE   115200
#define DEBUGPORT_UART_DEFAULT_PARITY     0
#define DEBUGPORT_UART_DEFAULT_FIFO_DEPTH 16
#define DEBUGPORT_UART_DEFAULT_TIMEOUT    50000 // 5 ms
#define DEBUGPORT_UART_DEFAULT_DATA_BITS  8
#define DEBUGPORT_UART_DEFAULT_STOP_BITS  1

#define DEBUGPORT_DRIVER_VERSION          1

#define EfiIsUartDevicePath(dp)           (DevicePathType (dp) == MESSAGING_DEVICE_PATH && DevicePathSubType (dp) == MSG_UART_DP)

//
// globals
//
extern DEBUGPORT_DEVICE *gDebugPortDevice;

//
// Driver binding interface functions...
//
EFI_STATUS
DebugPortEntryPoint (
  IN EFI_HANDLE                     ImageHandle,
  IN EFI_SYSTEM_TABLE               *SystemTable
  )
;

EFI_STATUS
EFIAPI
DebugPortSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
;

EFI_STATUS
EFIAPI
DebugPortStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
;

EFI_STATUS
EFIAPI
DebugPortStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
;

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
DebugPortComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  )
;

EFI_STATUS
EFIAPI
DebugPortComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                      *This,
  IN  EFI_HANDLE                                       ControllerHandle,
  IN  EFI_HANDLE                                       ChildHandle        OPTIONAL,
  IN  CHAR8                                            *Language,
  OUT CHAR16                                           **ControllerName
  )
;

//
// DebugPort member functions
//
EFI_STATUS
EFIAPI
DebugPortReset (
  IN EFI_DEBUGPORT_PROTOCOL         *This
  )
;

EFI_STATUS
EFIAPI
DebugPortRead (
  IN EFI_DEBUGPORT_PROTOCOL         *This,
  IN UINT32                         Timeout,
  IN OUT UINTN                      *BufferSize,
  IN VOID                           *Buffer
  )
;

EFI_STATUS
EFIAPI
DebugPortWrite (
  IN EFI_DEBUGPORT_PROTOCOL         *This,
  IN UINT32                         Timeout,
  IN OUT UINTN                      *BufferSize,
  OUT VOID                          *Buffer
  )
;

EFI_STATUS
EFIAPI
DebugPortPoll (
  IN EFI_DEBUGPORT_PROTOCOL         *This
  )
;

#endif
