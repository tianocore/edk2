/*++

Copyright (c) 2006, Tristan Gingold
All rights reserved. This program and the accompanying materials
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

#define EFI_UNIX_UGA_IO_PROTOCOL_GUID \
  { \
    0xf2e5e2c6, 0x8985, 0x11db, {0xa1, 0x91, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

struct _EFI_UNIX_UGA_IO_PROTOCOL;
typedef struct _EFI_UNIX_UGA_IO_PROTOCOL EFI_UNIX_UGA_IO_PROTOCOL;

typedef
EFI_STATUS
(*UGAClose)(EFI_UNIX_UGA_IO_PROTOCOL *Uga);

typedef
EFI_STATUS
(*UGASize)(EFI_UNIX_UGA_IO_PROTOCOL *Uga, UINT32 Width, UINT32 Height);

typedef
EFI_STATUS
(*UGACheckKey)(EFI_UNIX_UGA_IO_PROTOCOL *Uga);

typedef
EFI_STATUS
(*UGAGetKey)(EFI_UNIX_UGA_IO_PROTOCOL *Uga, EFI_INPUT_KEY *key);

typedef
EFI_STATUS
(*UGABlt)(EFI_UNIX_UGA_IO_PROTOCOL *Uga,
	  IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
	  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
	  IN  UINTN                                   SourceX,
	  IN  UINTN                                   SourceY,
	  IN  UINTN                                   DestinationX,
	  IN  UINTN                                   DestinationY,
	  IN  UINTN                                   Width,
	  IN  UINTN                                   Height,
	  IN  UINTN                                   Delta OPTIONAL);

struct _EFI_UNIX_UGA_IO_PROTOCOL {
  VOID                                *Private;
  UGAClose                            UgaClose;
  UGASize                             UgaSize;
  UGACheckKey                         UgaCheckKey;
  UGAGetKey                           UgaGetKey;
  UGABlt                              UgaBlt;
};


extern EFI_GUID gEfiUnixUgaIoProtocolGuid;

#endif
