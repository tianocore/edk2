/*++

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved. 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  Ps2Mouse.h

Abstract:

  PS/2 Mouse driver header file

Revision History

--*/

#ifndef _PS2MOUSE_H
#define _PS2MOUSE_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

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
#define PS2_MOUSE_DEV_SIGNATURE EFI_SIGNATURE_32 ('p', 's', '2', 'm')

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
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gPS2MouseDriver;
extern EFI_COMPONENT_NAME_PROTOCOL  gPs2MouseComponentName;

//
// Function prototypes
//
EFI_STATUS
EFIAPI
PS2MouseDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
PS2MouseDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

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
EFI_STATUS
EFIAPI
Ps2MouseComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
Ps2MouseComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

EFI_STATUS
EFIAPI
MouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  );

EFI_STATUS
EFIAPI
MouseGetState (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN OUT EFI_SIMPLE_POINTER_STATE   *State
  );

VOID
EFIAPI
MouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

VOID
EFIAPI
PollMouse (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

EFI_STATUS
In8042Data (
  IN EFI_ISA_IO_PROTOCOL                  *IsaIo,
  IN OUT UINT8                            *Data
  );
BOOLEAN
CheckMouseConnect (
  IN  PS2_MOUSE_DEV     *MouseDev
  );

#endif
