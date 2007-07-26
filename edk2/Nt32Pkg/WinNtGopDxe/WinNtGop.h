/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtGop.h

Abstract:

  Private data for the Gop driver that is bound to the WinNt Thunk protocol


**/

#ifndef _WIN_NT_GOP_H_
#define _WIN_NT_GOP_H_

//@MT:#include "EfiWinNT.h"
//@MT:#include "Tiano.h"
//@MT:#include "EfiDriverLib.h"

//
// Driver Consumed Protocols
//
//@MT:#include EFI_PROTOCOL_DEFINITION (DevicePath)
//@MT:#include EFI_PROTOCOL_DEFINITION (WinNtIo)

//
// Driver Produced Protocols
//
//@MT:#include EFI_PROTOCOL_DEFINITION (DriverBinding)
//@MT:#include EFI_PROTOCOL_DEFINITION (ComponentName)
//@MT:#include EFI_PROTOCOL_DEFINITION (GraphicsOutput)
//@MT:#include "LinkedList.h"

#define MAX_Q 256

typedef struct {
  UINTN         Front;
  UINTN         Rear;
  UINTN         Count;
  EFI_INPUT_KEY Q[MAX_Q];
} GOP_QUEUE_FIXED;

#define WIN_NT_GOP_CLASS_NAME       L"WinNtGopWindow"

#define GOP_PRIVATE_DATA_SIGNATURE  EFI_SIGNATURE_32 ('S', 'g', 'o', 'N')

#define GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER 0xffff

typedef struct {
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  UINT32                     ColorDepth;
  UINT32                     RefreshRate;
} GOP_MODE_DATA;

typedef struct {
  UINT64                        Signature;

  EFI_HANDLE                    Handle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  GraphicsOutput;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   SimpleTextIn;

  EFI_WIN_NT_THUNK_PROTOCOL     *WinNtThunk;

  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;

  //
  // GOP Private Data for QueryMode ()
  //
  GOP_MODE_DATA                 *ModeData;

  //
  // GOP Private Data knowing when to start hardware
  //
  BOOLEAN                       HardwareNeedsStarting;

  CHAR16                        *WindowName;
  CHAR16                        Buffer[160];

  HANDLE                        ThreadInited; // Semaphore
  HANDLE                        ThreadHandle; // Thread
  DWORD                         ThreadId;

  HWND                          WindowHandle;
  WNDCLASSEX                    WindowsClass;

  //
  // This screen is used to redraw the scree when windows events happen. It's
  // updated in the main thread and displayed in the windows thread.
  //
  BITMAPV4HEADER                *VirtualScreenInfo;
  RGBQUAD                       *VirtualScreen;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *FillLine;

  //
  // Keyboard Queue used by Simple Text In. WinProc thread adds, and main
  // thread removes.
  //
  CRITICAL_SECTION              QCriticalSection;
  GOP_QUEUE_FIXED               Queue;

} GOP_PRIVATE_DATA;

#define GOP_PRIVATE_DATA_FROM_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, GraphicsOutput, GOP_PRIVATE_DATA_SIGNATURE)

#define GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, SimpleTextIn, GOP_PRIVATE_DATA_SIGNATURE)

//
// Global Protocol Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gWinNtGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gWinNtGopComponentName;

//
// Gop Hardware abstraction internal worker functions
//

/**
  TODO: Add function description

  @param  WinNtIo              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopSupported (
  IN  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo
  )
;


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopConstructor (
  IN  GOP_PRIVATE_DATA    *Private
  )
;


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopDestructor (
  IN  GOP_PRIVATE_DATA    *Private
  )
;

//
// EFI 1.1 driver model prototypes for Win NT GOP
//


/**
  TODO: Add function description

  @param  ImageHandle          TODO: add argument description
  @param  SystemTable          TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
EFIAPI
WinNtGopInitialize (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
;


/**
  TODO: Add function description

  @param  This                 TODO: add argument description
  @param  Handle               TODO: add argument description
  @param  RemainingDevicePath  TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
EFIAPI
WinNtGopDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
;


/**
  TODO: Add function description

  @param  This                 TODO: add argument description
  @param  Handle               TODO: add argument description
  @param  RemainingDevicePath  TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
EFIAPI
WinNtGopDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
;


/**
  TODO: Add function description

  @param  This                 TODO: add argument description
  @param  Handle               TODO: add argument description
  @param  NumberOfChildren     TODO: add argument description
  @param  ChildHandleBuffer    TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
EFIAPI
WinNtGopDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
;


/**
  TODO: Add function description

  @param  Private              TODO: add argument description
  @param  Key                  TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
GopPrivateAddQ (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  EFI_INPUT_KEY       Key
  )
;


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopInitializeSimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  )
;


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopDestroySimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  )
;



#endif
