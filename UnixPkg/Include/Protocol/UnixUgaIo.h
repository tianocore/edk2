/*++

Copyright (c) 2006, Tristan Gingold. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the
BSD License which accompanies this distribution.  The full text of the
license may be found at http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UnixUgaIo.h

Abstract:

--*/

#ifndef _UNIX_UGA_IO_H_
#define _UNIX_UGA_IO_H_

#include <Protocol/SimplePointer.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/UgaDraw.h>

#define EFI_UNIX_UGA_IO_PROTOCOL_GUID {0xf2e5e2c6, 0x8985, 0x11db, {0xa1, 0x91, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } }

typedef struct _EFI_UNIX_UGA_IO_PROTOCOL EFI_UNIX_UGA_IO_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *UGAClose)(
  EFI_UNIX_UGA_IO_PROTOCOL *Uga
  );

typedef
EFI_STATUS
(EFIAPI *UGASize)(
  EFI_UNIX_UGA_IO_PROTOCOL *Uga, 
  UINT32                   Width, 
  UINT32                   Height
  );

typedef
EFI_STATUS
(EFIAPI *UGACheckKey)(
  EFI_UNIX_UGA_IO_PROTOCOL *Uga
  );

typedef
EFI_STATUS
(EFIAPI *UGAGetKey)(
  EFI_UNIX_UGA_IO_PROTOCOL *Uga, 
  EFI_KEY_DATA             *key
  );

typedef
EFI_STATUS
(EFIAPI *UGAKeySetState) (
  IN EFI_UNIX_UGA_IO_PROTOCOL   *UgaIo, 
  IN EFI_KEY_TOGGLE_STATE       *KeyToggleState
  );


typedef 
VOID
(EFIAPI *UGA_REGISTER_KEY_NOTIFY_CALLBACK) (
  IN VOID           *Context,
  IN EFI_KEY_DATA   *KeyData
  );

typedef
EFI_STATUS
(EFIAPI *UGARegisterKeyNotify) (
  IN EFI_UNIX_UGA_IO_PROTOCOL           *UgaIo, 
  IN UGA_REGISTER_KEY_NOTIFY_CALLBACK   CallBack,
  IN VOID                               *Context
  );


typedef struct {
    UINTN                                   SourceX;
    UINTN                                   SourceY;
    UINTN                                   DestinationX;
    UINTN                                   DestinationY;
    UINTN                                   Width;
    UINTN                                   Height;
    UINTN                                   Delta;
} UGA_BLT_ARGS;

typedef
EFI_STATUS
(EFIAPI *UGABlt)(
    IN  EFI_UNIX_UGA_IO_PROTOCOL                *Uga,
    IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
    IN  EFI_UGA_BLT_OPERATION                   BltOperation,
    IN  UGA_BLT_ARGS                            *Args
    );

typedef
BOOLEAN
(EFIAPI *UGAIsKeyPressed) ( 
  IN  EFI_UNIX_UGA_IO_PROTOCOL  *UgaIo, 
  IN  EFI_KEY_DATA              *KeyData
  );

typedef
EFI_STATUS
(EFIAPI *UGACheckPointer)(
  EFI_UNIX_UGA_IO_PROTOCOL *Uga
  );

typedef
EFI_STATUS
(EFIAPI *UGAGetPointerState)(
  EFI_UNIX_UGA_IO_PROTOCOL *Uga,
  EFI_SIMPLE_POINTER_STATE *state
  );

struct _EFI_UNIX_UGA_IO_PROTOCOL {
  VOID                                *Private;
  UGAClose                            UgaClose;
  UGASize                             UgaSize;
  UGACheckKey                         UgaCheckKey;
  UGAKeySetState                      UgaKeySetState;
  UGAGetKey                           UgaGetKey;
  UGARegisterKeyNotify                UgaRegisterKeyNotify;
  UGABlt                              UgaBlt;
  UGAIsKeyPressed                     UgaIsKeyPressed;
  UGACheckPointer                     UgaCheckPointer;
  UGAGetPointerState                  UgaGetPointerState;
};


extern EFI_GUID gEfiUnixUgaIoProtocolGuid;

#endif
