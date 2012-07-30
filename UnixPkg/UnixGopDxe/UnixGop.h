/*++

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010, Apple, Inc. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixGop.h

Abstract:

  Private data for the Gop driver that is bound to the Unix Thunk protocol 

--*/

#ifndef _UNIX_UGA_H_
#define _UNIX_UGA_H_

#include <PiDxe.h>
#include "UnixDxe.h"
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimplePointer.h>
#include "Protocol/UnixUgaIo.h"

#include <Guid/EventGroup.h>



#define MAX_Q 256

typedef struct {
  UINTN         Front;
  UINTN         Rear;
  UINTN         Count;
  EFI_INPUT_KEY Q[MAX_Q];
} GOP_QUEUE_FIXED;

#define UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('U', 'g', 'S', 'n')
typedef struct _UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY {
  UINTN                                 Signature;
  EFI_KEY_DATA                          KeyData;
  EFI_KEY_NOTIFY_FUNCTION               KeyNotificationFn;
  EFI_EVENT                             Event;
  LIST_ENTRY                            NotifyEntry;
} UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY;
  
#define GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER 0xffff

typedef struct {
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  UINT32                     ColorDepth;
  UINT32                     RefreshRate;
} GOP_MODE_DATA;



extern EFI_DRIVER_BINDING_PROTOCOL gUnixGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL gUnixGopComponentName;

#define UNIX_UGA_CLASS_NAME       L"UnixGopWindow"

#define GOP_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('G', 'o', 'p', 'N')
typedef struct {
  UINT64                      Signature;

  EFI_HANDLE                        Handle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL      GraphicsOutput;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL    SimpleTextIn;
  EFI_SIMPLE_POINTER_PROTOCOL       SimplePointer;

  EFI_UNIX_THUNK_PROTOCOL         *UnixThunk;
  EFI_UNIX_UGA_IO_PROTOCOL        *UgaIo;

  EFI_UNICODE_STRING_TABLE        *ControllerNameTable;

  EFI_SIMPLE_POINTER_MODE         PointerMode;
  //
  // GOP Private Data for QueryMode ()
  //
  GOP_MODE_DATA                   *ModeData;


  //
  // UGA Private Data knowing when to start hardware
  //
  BOOLEAN                           HardwareNeedsStarting;

  CHAR16                            *WindowName;

  GOP_QUEUE_FIXED                   Queue;

  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL SimpleTextInEx;
  EFI_KEY_STATE                     KeyState;
  LIST_ENTRY                        NotifyList;


} GOP_PRIVATE_DATA;


#define GOP_PRIVATE_DATA_FROM_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, GraphicsOutput, GOP_PRIVATE_DATA_SIGNATURE)

#define GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, SimpleTextIn, GOP_PRIVATE_DATA_SIGNATURE)

#define GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, SimpleTextInEx, GOP_PRIVATE_DATA_SIGNATURE)

#define GOP_PRIVATE_DATA_FROM_POINTER_MODE_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, SimplePointer, GOP_PRIVATE_DATA_SIGNATURE)


//
// Global Protocol Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gUnixGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gUnixGopComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gUnixGopComponentName2;

//
// Gop Hardware abstraction internal worker functions
//
EFI_STATUS
UnixGopSupported (
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
UnixGopConstructor (
  IN  GOP_PRIVATE_DATA    *Private
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
UnixGopDestructor (
  IN  GOP_PRIVATE_DATA    *Private
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
UnixGopInitialize (
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
UnixGopDriverBindingSupported (
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
UnixGopDriverBindingStart (
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
UnixGopDriverBindingStop (
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
GopPrivateAddQ (
  IN  GOP_PRIVATE_DATA    *Private,
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
UnixGopInitializeSimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
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
UnixGopInitializeSimplePointerForWindow (
  IN  GOP_PRIVATE_DATA    *Private
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
