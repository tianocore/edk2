/** @file
  PS/2 Mouse driver header file.
  
Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PS2MOUSE_H_
#define _PS2MOUSE_H_

#include <Uefi.h>

#include <Protocol/SimplePointer.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gPS2MouseDriver;
extern EFI_COMPONENT_NAME_PROTOCOL   gPs2MouseComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPs2MouseComponentName2;

//
// PS/2 mouse sample rate
//
typedef enum {
  SampleRate10,
  SampleRate20,
  SampleRate40,
  SampleRate60,
  SampleRate80,
  SampleRate100,
  SampleRate200,
  MaxSampleRate
} MOUSE_SR;

//
// PS/2 mouse resolution
//
typedef enum {
  MouseResolution1,
  MouseResolution2,
  MouseResolution4,
  MouseResolution8,
  MaxResolution
} MOUSE_RE;

//
// PS/2 mouse scaling
//
typedef enum {
  Scaling1,
  Scaling2
} MOUSE_SF;

//
// Driver Private Data
//
#define PS2_MOUSE_DEV_SIGNATURE SIGNATURE_32 ('p', 's', '2', 'm')

typedef struct {
  UINTN                               Signature;

  EFI_HANDLE                          Handle;
  EFI_SIMPLE_POINTER_PROTOCOL         SimplePointerProtocol;
  EFI_SIMPLE_POINTER_STATE            State;
  EFI_SIMPLE_POINTER_MODE             Mode;
  BOOLEAN                             StateChanged;

  //
  // PS2 Mouse device specific information
  //
  MOUSE_SR                            SampleRate;
  MOUSE_RE                            Resolution;
  MOUSE_SF                            Scaling;
  UINT8                               DataPackageSize;

  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  EFI_EVENT                           TimerEvent;

  EFI_UNICODE_STRING_TABLE            *ControllerNameTable;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
} PS2_MOUSE_DEV;

#define PS2_MOUSE_DEV_FROM_THIS(a)  CR (a, PS2_MOUSE_DEV, SimplePointerProtocol, PS2_MOUSE_DEV_SIGNATURE)

//
// Function prototypes
//
/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a IsaIo protocol can be supported.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
PS2MouseDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Start this driver on ControllerHandle by opening a IsaIo
  protocol, creating PS2_MOUSE_ABSOLUTE_POINTER_DEV device and install gEfiAbsolutePointerProtocolGuid
  finnally.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
PS2MouseDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
PS2MouseDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN EFI_HANDLE                    Controller,
  IN UINTN                         NumberOfChildren,
  IN EFI_HANDLE                    *ChildHandleBuffer
  );

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
                                in RFC 4646 or ISO 639-2 language code format.

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
Ps2MouseComponentNameGetDriverName (
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
                                RFC 4646 or ISO 639-2 language code format.

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

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

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
Ps2MouseComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

/**
  Reset the Mouse and do BAT test for it, if ExtendedVerification is TRUE and there is a mouse device connected to system

  @param This                 - Pointer of simple pointer Protocol.
  @param ExtendedVerification - Whether configure mouse parameters. True: do; FALSE: skip.


  @retval EFI_SUCCESS         - The command byte is written successfully.
  @retval EFI_DEVICE_ERROR    - Errors occurred during resetting keyboard.

**/
EFI_STATUS
EFIAPI
MouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  );

/**
  Get and Clear mouse status.
  
  @param This                 - Pointer of simple pointer Protocol.
  @param State                - Output buffer holding status.

  @retval EFI_INVALID_PARAMETER Output buffer is invalid.
  @retval EFI_NOT_READY         Mouse is not changed status yet.
  @retval EFI_SUCCESS           Mouse status is changed and get successful.
**/
EFI_STATUS
EFIAPI
MouseGetState (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN OUT EFI_SIMPLE_POINTER_STATE   *State
  );

/**

  Event notification function for SIMPLE_POINTER.WaitForInput event.
  Signal the event if there is input from mouse.

  @param Event    event object
  @param Context  event context

**/
VOID
EFIAPI
MouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

/**
  Event notification function for TimerEvent event.
  If mouse device is connected to system, try to get the mouse packet data.

  @param Event      -  TimerEvent in PS2_MOUSE_DEV
  @param Context    -  Pointer to PS2_MOUSE_DEV structure

**/
VOID
EFIAPI
PollMouse (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  I/O work flow of in 8042 data.
  
  @param IsaIo   Pointer to instance of EFI_ISA_IO_PROTOCOL
  @param Data    Data value
  
  @retval EFI_SUCCESS Success to execute I/O work flow
  @retval EFI_TIMEOUT Keyboard controller time out.
**/
EFI_STATUS
In8042Data (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN OUT UINT8                            *Data
  );

/**
  Check whether there is Ps/2 mouse device in system

  @param MouseDev   - Mouse Private Data Structure

  @retval TRUE      - Keyboard in System.
  @retval FALSE     - Keyboard not in System.

**/
BOOLEAN
CheckMouseConnect (
  IN  PS2_MOUSE_DEV     *MouseDev
  );

#endif
