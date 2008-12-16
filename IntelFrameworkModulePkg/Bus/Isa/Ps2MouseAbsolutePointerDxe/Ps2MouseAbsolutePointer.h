/**@file
  A Ps2MouseAbsolutePointer driver header file
  
Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PS2MOUSEABSOLUTEPOINTER_H
#define _PS2MOUSEABSOLUTEPOINTER_H

#include <PiDxe.h>
#include <Framework/StatusCode.h>

#include <Protocol/AbsolutePointer.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>

//
// PS/2 mouse sample rate
//
typedef enum {
  SSR_10,
  SSR_20,
  SSR_40,
  SSR_60,
  SSR_80,
  SSR_100,
  SSR_200,
  MAX_SR
} MOUSE_SR;

//
// PS/2 mouse resolution
//
typedef enum {
  CMR1,
  CMR2,
  CMR4,
  CMR8,
  MAX_CMR
} MOUSE_RE;

//
// PS/2 mouse scaling
//
typedef enum {
  SF1,
  SF2
} MOUSE_SF;

//
// Driver Private Data
//
#define PS2_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE SIGNATURE_32 ('p', '2', 's', 't')

typedef struct {
  UINTN                               Signature;

  EFI_HANDLE                          Handle;
  EFI_ABSOLUTE_POINTER_PROTOCOL       AbsolutePointerProtocol;
  EFI_ABSOLUTE_POINTER_STATE          State;
  EFI_ABSOLUTE_POINTER_MODE           Mode;
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
} PS2_MOUSE_ABSOLUTE_POINTER_DEV;

#define PS2_MOUSE_ABSOLUTE_POINTER_DEV_FROM_THIS(a)  CR (a, PS2_MOUSE_ABSOLUTE_POINTER_DEV, AbsolutePointerProtocol, PS2_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gPS2MouseAbsolutePointerDriver;
extern EFI_COMPONENT_NAME_PROTOCOL   gPs2MouseAbsolutePointerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPs2MouseAbsolutePointerComponentName2;

//
// Function prototypes
//
EFI_STATUS
EFIAPI
PS2MouseAbsolutePointerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
PS2MouseAbsolutePointerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
PS2MouseAbsolutePointerDriverStop (
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
Ps2MouseAbsolutePointerComponentNameGetDriverName (
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
Ps2MouseAbsolutePointerComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


EFI_STATUS
EFIAPI
MouseAbsolutePointerReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                        ExtendedVerification
  );

EFI_STATUS
EFIAPI
MouseAbsolutePointerGetState (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  IN OUT EFI_ABSOLUTE_POINTER_STATE   *State
  );

VOID
EFIAPI
MouseAbsolutePointerWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

VOID
EFIAPI
PollMouseAbsolutePointer (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

EFI_STATUS
In8042Data (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN OUT UINT8                            *Data
  );
BOOLEAN
CheckMouseAbsolutePointerConnect (
  IN  PS2_MOUSE_ABSOLUTE_POINTER_DEV     *MouseAbsolutePointerDev
  );

#endif
