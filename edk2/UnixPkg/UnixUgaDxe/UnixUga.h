/*++

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixUga.h

Abstract:

  Private data for the Uga driver that is bound to the Unix Thunk protocol 

--*/

#ifndef _UNIX_UGA_H_
#define _UNIX_UGA_H_

#include "PiDxe.h"
#include <Guid/EventGroup.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/UgaDraw.h>
#include "Protocol/UnixUgaIo.h"
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "UnixDxe.h"

extern EFI_DRIVER_BINDING_PROTOCOL gUnixUgaDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL gUnixUgaComponentName;

#define UNIX_UGA_CLASS_NAME       L"UnixUgaWindow"

#define UGA_PRIVATE_DATA_SIGNATURE  EFI_SIGNATURE_32 ('S', 'g', 'o', 'N')
typedef struct {
  UINT64                      Signature;

  EFI_HANDLE                  Handle;
  EFI_UGA_DRAW_PROTOCOL       UgaDraw;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL SimpleTextIn;

  EFI_UNIX_THUNK_PROTOCOL   *UnixThunk;

  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;

  //
  // UGA Private Data for GetMode ()
  //
  UINT32                      HorizontalResolution;
  UINT32                      VerticalResolution;
  UINT32                      ColorDepth;
  UINT32                      RefreshRate;

  //
  // UGA Private Data knowing when to start hardware
  //
  BOOLEAN                     HardwareNeedsStarting;

  CHAR16                      *WindowName;

  EFI_UNIX_UGA_IO_PROTOCOL    *UgaIo;

} UGA_PRIVATE_DATA;

#define UGA_DRAW_PRIVATE_DATA_FROM_THIS(a)  \
         CR(a, UGA_PRIVATE_DATA, UgaDraw, UGA_PRIVATE_DATA_SIGNATURE)

#define UGA_PRIVATE_DATA_FROM_TEXT_IN_THIS(a)  \
         CR(a, UGA_PRIVATE_DATA, SimpleTextIn, UGA_PRIVATE_DATA_SIGNATURE)

//
// Global Protocol Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gUnixUgaDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gUnixUgaComponentName;

//
// Uga Hardware abstraction internal worker functions
//
EFI_STATUS
UnixUgaSupported (
  IN  EFI_UNIX_IO_PROTOCOL  *UnixIo
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  UnixIo - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
UnixUgaConstructor (
  IN  UGA_PRIVATE_DATA    *Private
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

EFI_STATUS
UnixUgaDestructor (
  IN  UGA_PRIVATE_DATA    *Private
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

//
// EFI 1.1 driver model prototypes for Win UNIX UGA
//

EFI_STATUS
EFIAPI
UnixUgaInitialize (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
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
UnixUgaDriverBindingSupported (
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
UnixUgaDriverBindingStart (
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
UnixUgaDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
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
UgaPrivateAddQ (
  IN  UGA_PRIVATE_DATA    *Private,
  IN  EFI_INPUT_KEY       Key
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description
  Key     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
UnixUgaInitializeSimpleTextInForWindow (
  IN  UGA_PRIVATE_DATA    *Private
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
