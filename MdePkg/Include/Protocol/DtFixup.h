/** @file

  Copyright (c) 2025, Heinrich Schuchardt.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  DT Fix-up Protocol.
  Lets firmware patch a flattened-device-tree and reserve memory ranges.

  Related docs:
  https://github.com/U-Boot-EFI/EFI_DT_FIXUP_PROTOCOL
*/

#ifndef DT_FIXUP_PROTOCOL_H_
#define DT_FIXUP_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {e617d64c-fe08-46da-f4dc-bbd5870c7300}
//
#define DT_FIXUP_PROTOCOL_GUID \
  { 0xe617d64c, 0xfe08, 0x46da, { 0xf4, 0xdc, 0xbb, 0xd5, 0x87, 0x0c, 0x73, 0x00 } }

#define DT_FIXUP_PROTOCOL_REVISION  0x00010000

typedef struct _DT_FIXUP_PROTOCOL DT_FIXUP_PROTOCOL;

#define DT_FIXUP_APPLY_FIXUPS    0x00000001
#define DT_FIXUP_RESERVE_MEMORY  0x00000002
#define DT_FIXUP_ALL             (DT_FIXUP_APPLY_FIXUPS | DT_FIXUP_RESERVE_MEMORY)

typedef
EFI_STATUS
(EFIAPI *DT_FIXUP)(
  IN DT_FIXUP_PROTOCOL *This,
  IN OUT VOID          *Fdt,
  IN OUT UINTN         *BufferSize,
  IN UINT32            Flags
  );

struct _DT_FIXUP_PROTOCOL {
  UINT64      Revision;
  DT_FIXUP    Fixup;
};

#endif // DT_FIXUP_PROTOCOL_H_
