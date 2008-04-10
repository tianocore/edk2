/** @file
  Definitions and prototypes for DebugPort driver.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEBUGPORT_H__
#define __DEBUGPORT_H__


#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SerialIo.h>
#include <Protocol/DebugPort.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL  gDebugPortDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gDebugPortComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gDebugPortComponentName2;

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
extern DEBUGPORT_DEVICE  *gDebugPortDevice;

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
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 3066 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
DebugPortComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 3066 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
DebugPortComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


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
