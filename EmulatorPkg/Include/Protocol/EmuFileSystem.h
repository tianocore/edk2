/** @file
  SimpleFileSystem protocol as defined in the UEFI 2.0 specification.

  The SimpleFileSystem protocol is the programmatic access to the FAT (12,16,32)
  file system specified in UEFI 2.0. It can also be used to abstract a file
  system other than FAT.

  UEFI 2.0 can boot from any valid EFI image contained in a SimpleFileSystem.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EMU_UGA_IO_H_
#define _EMU_UGA_IO_H_

#include <Protocol/SimplePointer.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/UgaDraw.h>

#define EMU_GRAPHICS_WINDOW_PROTOCOL_GUID \
 { 0x30FD316A, 0x6728, 0x2E41, { 0xA6, 0x90, 0x0D, 0x13, 0x33, 0xD8, 0xCA, 0xC1 } }

typedef struct _EMU_GRAPHICS_WINDOW_PROTOCOL EMU_GRAPHICS_WINDOW_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_CLOSE)(
  EMU_GRAPHICS_WINDOW_PROTOCOL *Uga
  );

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_SIZE)(
  EMU_GRAPHICS_WINDOW_PROTOCOL  *Uga,
  UINT32                        Width,
  UINT32                        Height
  );

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_CHECK_KEY)(
  EMU_GRAPHICS_WINDOW_PROTOCOL *Uga
  );

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_GET_KEY)(
  EMU_GRAPHICS_WINDOW_PROTOCOL  *Uga,
  EFI_KEY_DATA                  *key
  );

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_KEY_SET_STATE) (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL   *GraphicsWindows,
  IN EFI_KEY_TOGGLE_STATE           *KeyToggleState
  );


typedef
VOID
(EFIAPI *EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK) (
  IN VOID           *Context,
  IN EFI_KEY_DATA   *KeyData
  );

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_REGISTER_KEY_NOTIFY) (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL                       *GraphicsWindows,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK   CallBack,
  IN VOID                                               *Context
  );


typedef struct {
    UINTN                                   SourceX;
    UINTN                                   SourceY;
    UINTN                                   DestinationX;
    UINTN                                   DestinationY;
    UINTN                                   Width;
    UINTN                                   Height;
    UINTN                                   Delta;
} EMU_GRAPHICS_WINDOWS__BLT_ARGS;

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_BLT)(
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL            *GraphicsWindows,
  IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
  IN  EMU_GRAPHICS_WINDOWS__BLT_ARGS          *Args
  );

typedef
BOOLEAN
(EFIAPI *EMU_GRAPHICS_WINDOWS_IS_KEY_PRESSED) (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindows,
  IN  EFI_KEY_DATA                  *KeyData
  );

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_CHECK_POINTER)(
  EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsWindows
  );

typedef
EFI_STATUS
(EFIAPI *EMU_GRAPHICS_WINDOWS_GET_POINTER_STATE)(
  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindows,
  EFI_SIMPLE_POINTER_STATE      *state
  );

struct _EMU_GRAPHICS_WINDOW_PROTOCOL {
  EMU_GRAPHICS_WINDOWS_SIZE                    Size;
  EMU_GRAPHICS_WINDOWS_CHECK_KEY               CheckKey;
  EMU_GRAPHICS_WINDOWS_KEY_SET_STATE           KeySetState;
  EMU_GRAPHICS_WINDOWS_GET_KEY                 GetKey;
  EMU_GRAPHICS_WINDOWS_REGISTER_KEY_NOTIFY     RegisterKeyNotify;
  EMU_GRAPHICS_WINDOWS_BLT                     Blt;
  EMU_GRAPHICS_WINDOWS_IS_KEY_PRESSED          IsKeyPressed;
  EMU_GRAPHICS_WINDOWS_CHECK_POINTER           CheckPointer;
  EMU_GRAPHICS_WINDOWS_GET_POINTER_STATE       GetPointerState;
};


extern EFI_GUID gEmuGraphicsWindowProtocolGuid;

#endif
