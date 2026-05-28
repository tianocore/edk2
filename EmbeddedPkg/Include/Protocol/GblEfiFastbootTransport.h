/** @file
  GBL EFI Fastboot Transport Protocol.

  Enables firmware to expose a transport interface for Android Fastboot.

  Related docs:
  https://android.googlesource.com/platform/bootable/libbootloader/+/refs/heads/gbl-mainline/gbl/docs/gbl_efi_fastboot_transport_protocol.md

  Copyright (c) 2025, The Android Open Source Project.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

//
// {edade92c-5c48-440d-849c-e2a0c7e55143}
//
#define GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_GUID \
  { 0xedade92c, 0x5c48, 0x440d, { 0x84, 0x9c, 0xe2, 0xa0, 0xc7, 0xe5, 0x51, 0x43 } }

#define GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_REVISION  0x00010000

typedef struct _GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL;

typedef enum {
  GblEfiFastbootRxModeSinglePacket = 0,
  GblEfiFastbootRxModeFixedLength
} GBL_EFI_FASTBOOT_RX_MODE;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_TRANSPORT_START)(
  IN GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_TRANSPORT_STOP)(
  IN GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL *This
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_TRANSPORT_RECEIVE)(
  IN GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL *This,
  IN OUT UINTN                           *BufferSize,
  OUT VOID                               *Buffer,
  IN UINT32                              Mode // GBL_EFI_FASTBOOT_RX_MODE
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_TRANSPORT_SEND)(
  IN GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL *This,
  IN OUT UINTN                           *BufferSize,
  IN CONST VOID                          *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FASTBOOT_TRANSPORT_FLUSH)(
  IN GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL *This
  );

struct _GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL {
  UINT64                                Revision;
  CONST CHAR8                           *Description;
  GBL_EFI_FASTBOOT_TRANSPORT_START      Start;
  GBL_EFI_FASTBOOT_TRANSPORT_STOP       Stop;
  GBL_EFI_FASTBOOT_TRANSPORT_RECEIVE    Receive;
  GBL_EFI_FASTBOOT_TRANSPORT_SEND       Send;
  GBL_EFI_FASTBOOT_TRANSPORT_FLUSH      Flush;
};

extern EFI_GUID  gGblEfiFastbootTransportProtocolGuid;
