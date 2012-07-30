/** @file

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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


#include <Uefi.h>
#include <WinNtDxe.h>

#include <Guid/EventGroup.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/GraphicsOutput.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

//
// WM_SYSKEYDOWN/WM_SYSKEYUP Notification
// lParam
// bit 24: Specifies whether the key is an extended key,
// such as the right-hand ALT and CTRL keys that appear on
// an enhanced 101- or 102-key keyboard.
// The value is 1 if it is an extended key; otherwise, it is 0.
// bit 29:Specifies the context code.
// The value is 1 if the ALT key is down while the key is pressed/released;
// it is 0 if the WM_SYSKEYDOWN message is posted to the active window
// because no window has the keyboard focus.
#define GOP_EXTENDED_KEY         (0x1 << 24)
#define GOP_ALT_KEY_PRESSED      (0x1 << 29)

#define KEYBOARD_TIMER_INTERVAL         200000  // 0.02s

#define MAX_Q 256

typedef struct {
  UINTN             Front;
  UINTN             Rear;
  EFI_KEY_DATA      Q[MAX_Q];
  CRITICAL_SECTION  Cs;
} GOP_QUEUE_FIXED;

#define WIN_NT_GOP_CLASS_NAME       L"WinNtGopWindow"

#define GOP_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('S', 'g', 'o', 'N')

#define WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('W', 'g', 'S', 'n')

typedef struct _WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY {
  UINTN                                 Signature;
  EFI_KEY_DATA                          KeyData;
  EFI_KEY_NOTIFY_FUNCTION               KeyNotificationFn;
  LIST_ENTRY                            NotifyEntry;
} WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY;

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
  // Keyboard Queue used by Simple Text In.
  // QueueForRead:   WinProc thread adds, and main thread removes.
  // QueueForNotify: WinProc thread adds, and timer thread removes.
  //
  GOP_QUEUE_FIXED               QueueForRead;
  GOP_QUEUE_FIXED               QueueForNotify;

  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL SimpleTextInEx;
  EFI_KEY_STATE                     KeyState;
  LIST_ENTRY                        NotifyList;
  BOOLEAN                           LeftShift;
  BOOLEAN                           RightShift;
  BOOLEAN                           LeftAlt;
  BOOLEAN                           RightAlt;
  BOOLEAN                           LeftCtrl;
  BOOLEAN                           RightCtrl;
  BOOLEAN                           LeftLogo;
  BOOLEAN                           RightLogo;
  BOOLEAN                           Menu;
  BOOLEAN                           SysReq;
  BOOLEAN                           NumLock;
  BOOLEAN                           ScrollLock;
  BOOLEAN                           CapsLock;
  BOOLEAN                           IsPartialKeySupport;
  EFI_EVENT                         TimerEvent;
} GOP_PRIVATE_DATA;

#define GOP_PRIVATE_DATA_FROM_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, GraphicsOutput, GOP_PRIVATE_DATA_SIGNATURE)

#define GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, SimpleTextIn, GOP_PRIVATE_DATA_SIGNATURE)

#define GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS(a)  \
         CR(a, GOP_PRIVATE_DATA, SimpleTextInEx, GOP_PRIVATE_DATA_SIGNATURE)

//
// Global Protocol Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gWinNtGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gWinNtGopComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gWinNtGopComponentName2;


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
  );


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopConstructor (
  IN  GOP_PRIVATE_DATA    *Private
  );


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopDestructor (
  IN  GOP_PRIVATE_DATA    *Private
  );

//
// UEFI 2.0 driver model prototypes for Win NT GOP
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
  );


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
  );


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
  );


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
  );


/**
  TODO: Add function description

  @param  Private              TODO: add argument description
  @param  Key                  TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
GopPrivateAddKey (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  EFI_INPUT_KEY       Key
  );


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopInitializeSimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  );


/**
  TODO: Add function description

  @param  Private              TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopDestroySimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  );



#endif
