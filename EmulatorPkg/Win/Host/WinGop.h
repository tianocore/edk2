/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  WinGop.h

Abstract:

  Private data for the Gop driver that is bound to the WinNt Thunk protocol


**/

#ifndef _WIN_GOP_H_
#define _WIN_GOP_H_


#include "WinHost.h"

#include <Protocol/EmuIoThunk.h>
#include <Protocol/EmuGraphicsWindow.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/FrameBufferBltLib.h>

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


typedef struct {
  UINT64                        Signature;
  EMU_GRAPHICS_WINDOW_PROTOCOL  GraphicsWindowIo;

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

  UINT32                        Width;
  UINT32                        Height;
  //
  // This screen is used to redraw the scree when windows events happen. It's
  // updated in the main thread and displayed in the windows thread.
  //
  BITMAPV4HEADER                *VirtualScreenInfo;

  FRAME_BUFFER_CONFIGURE        *FrameBufferConfigure;

  //
  // Keyboard Queue used by Simple Text In.
  // QueueForRead:   WinProc thread adds, and main thread removes.
  //
  GOP_QUEUE_FIXED               QueueForRead;

  EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    MakeRegisterdKeyCallback;
  EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    BreakRegisterdKeyCallback;
  VOID                                                *RegisterdKeyCallbackContext;

  EFI_KEY_STATE                     KeyState;
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
} GRAPHICS_PRIVATE_DATA;
#define GRAPHICS_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('g', 'f', 'x', 'd')
#define GRAPHICS_PRIVATE_DATA_FROM_THIS(a)  \
         CR(a, GRAPHICS_PRIVATE_DATA, GraphicsWindowIo, GRAPHICS_PRIVATE_DATA_SIGNATURE)


//
// Gop Hardware abstraction internal worker functions
//

/**
  TODO: Add function description

  @param  Private              TODO: add argument description
  @param  Key                  TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
GopPrivateAddKey (
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN  EFI_INPUT_KEY            Key
  );

EFI_STATUS
EFIAPI
WinNtWndGetKey (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  EFI_KEY_DATA                  *KeyData
  );

EFI_STATUS
EFIAPI
WinNtWndCheckKey (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsIo
  );

EFI_STATUS
EFIAPI
WinNtWndKeySetState (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL   *GraphicsIo,
  IN EFI_KEY_TOGGLE_STATE           *KeyToggleState
  );

EFI_STATUS
EFIAPI
WinNtWndRegisterKeyNotify (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL                        *GraphicsIo,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    MakeCallBack,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    BreakCallBack,
  IN VOID                                                *Context
  );

EFI_STATUS
EFIAPI
WinNtWndCheckPointer (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsIo
  );

EFI_STATUS
EFIAPI
WinNtWndGetPointerState (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  EFI_SIMPLE_POINTER_STATE      *State
  );
EFI_STATUS
GopPrivateCreateQ (
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN GOP_QUEUE_FIXED           *Queue
  );


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateDestroyQ (
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN GOP_QUEUE_FIXED           *Queue
  );
#endif

