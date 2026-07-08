/** @file
  Device Tree Fixup Protocol.

  Modifies a device tree in memory to align it with the firmware's view of
  the platform.

  Specification:
  https://github.com/U-Boot-EFI/EFI_DT_FIXUP_PROTOCOL

  Copyright (c) 2025, Heinrich Schuchardt.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

//
// {60ed6ba9-dfef-4799-ac7b-75e0f833456c}
//
#define DT_FIXUP_PROTOCOL_GUID \
  { 0x60ed6ba9, 0xdfef, 0x4799, { 0xac, 0x7b, 0x75, 0xe0, 0xf8, 0x33, 0x45, 0x6c } }

#define DT_FIXUP_PROTOCOL_REVISION  0x00010000

typedef struct _DT_FIXUP_PROTOCOL DT_FIXUP_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *DT_FIXUP)(
  IN DT_FIXUP_PROTOCOL *This,
  IN OUT VOID          *Fdt,
  IN OUT UINTN         *BufferSize
  );

struct _DT_FIXUP_PROTOCOL {
  UINT64      Revision;
  DT_FIXUP    Fixup;
};

extern EFI_GUID  gDtFixupProtocolGuid;
