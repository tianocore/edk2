/*++ @file

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010,Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UGA_H_
#define __UGA_H_

#include <PiDxe.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/EmuIoThunk.h>
#include <Protocol/EmuGraphicsWindow.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/KeyMapLib.h>

#define MAX_Q  256

typedef struct {
  UINTN            Front;
  UINTN            Rear;
  UINTN            Count;
  EFI_INPUT_KEY    Q[MAX_Q];
} GOP_QUEUE_FIXED;

#define EMU_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE  SIGNATURE_32 ('U', 'g', 'S', 'n')
typedef struct _EMU_GOP_SIMPLE_TEXTIN_EX_NOTIFY {
  UINTN                      Signature;
  EFI_HANDLE                 NotifyHandle;
  EFI_KEY_DATA               KeyData;
  EFI_KEY_NOTIFY_FUNCTION    KeyNotificationFn;
  EFI_EVENT                  Event;
  LIST_ENTRY                 NotifyEntry;
} EMU_GOP_SIMPLE_TEXTIN_EX_NOTIFY;

#define GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER  0xffff

typedef struct {
  UINT32    HorizontalResolution;
  UINT32    VerticalResolution;
  UINT32    ColorDepth;
  UINT32    RefreshRate;
} GOP_MODE_DATA;

extern EFI_DRIVER_BINDING_PROTOCOL  gEmuGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gEmuGopComponentName;

#define EMU_UGA_CLASS_NAME  L"EmuGopWindow"

#define GOP_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('G', 'o', 'p', 'N')
typedef struct {
  UINT64                               Signature;

  EFI_HANDLE                           Handle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL         GraphicsOutput;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL       SimpleTextIn;
  EFI_SIMPLE_POINTER_PROTOCOL          SimplePointer;

  EMU_IO_THUNK_PROTOCOL                *EmuIoThunk;
  EMU_GRAPHICS_WINDOW_PROTOCOL         *EmuGraphicsWindow;

  EFI_UNICODE_STRING_TABLE             *ControllerNameTable;

  EFI_SIMPLE_POINTER_MODE              PointerMode;
  //
  // GOP Private Data for QueryMode ()
  //
  GOP_MODE_DATA                        *ModeData;

  //
  // UGA Private Data knowing when to start hardware
  //
  BOOLEAN                              HardwareNeedsStarting;

  CHAR16                               *WindowName;

  GOP_QUEUE_FIXED                      Queue;

  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    SimpleTextInEx;
  EFI_KEY_STATE                        KeyState;
  LIST_ENTRY                           NotifyList;
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
extern EFI_DRIVER_BINDING_PROTOCOL   gEmuGopDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gEmuGopComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gEmuGopComponentName2;

//
// Gop Hardware abstraction internal worker functions
//
EFI_STATUS
EmuGopSupported (
  IN  EMU_IO_THUNK_PROTOCOL  *EmuIoThunk
  );

EFI_STATUS
EmuGopConstructor (
  IN  GOP_PRIVATE_DATA  *Private
  );

EFI_STATUS
EmuGopDestructor (
  IN  GOP_PRIVATE_DATA  *Private
  );

EFI_STATUS
GopPrivateAddQ (
  IN  GOP_PRIVATE_DATA  *Private,
  IN  EFI_INPUT_KEY     Key
  );

EFI_STATUS
EmuGopInitializeSimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA  *Private
  );

EFI_STATUS
EmuGopInitializeSimplePointerForWindow (
  IN  GOP_PRIVATE_DATA  *Private
  );

EFI_STATUS
EmuGopStartWindow (
  IN  GOP_PRIVATE_DATA  *Private,
  IN  UINT32            HorizontalResolution,
  IN  UINT32            VerticalResolution,
  IN  UINT32            ColorDepth,
  IN  UINT32            RefreshRate
  );

VOID
EFIAPI
ShutdownGopEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

VOID
EFIAPI
GopPrivateMakeCallbackFunction (
  IN VOID          *Context,
  IN EFI_KEY_DATA  *KeyData
  );

VOID
EFIAPI
GopPrivateBreakCallbackFunction (
  IN VOID          *Context,
  IN EFI_KEY_DATA  *KeyData
  );

#endif
