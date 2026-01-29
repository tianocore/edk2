/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI AVF Protocol.
  Supplies GBL with vendor DICE handover and Secret Keeper public key
  needed for Android Virtualization Framework.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_efi_avf_protocol.md
*/

#ifndef GBL_EFI_AVF_PROTOCOL_H_
#define GBL_EFI_AVF_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {e7f1c4a6-0a52-4f61-bd98-9e60b559452a}
//
#define GBL_EFI_AVF_PROTOCOL_GUID \
  { 0xe7f1c4a6, 0x0a52, 0x4f61, { 0xbd, 0x98, 0x9e, 0x60, 0xb5, 0x59, 0x45, 0x2a } }

#define GBL_EFI_AVF_PROTOCOL_REVISION  0x00000001

typedef struct _GBL_EFI_AVF_PROTOCOL GBL_EFI_AVF_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVF_READ_VENDOR_DICE_HANDOVER)(
  IN GBL_EFI_AVF_PROTOCOL *This,
  IN OUT UINTN            *HandoverSize,
  OUT UINT8               *Handover
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVF_READ_SECRET_KEEPER_PUBLIC_KEY)(
  IN GBL_EFI_AVF_PROTOCOL *This,
  IN OUT UINTN            *PublicKeySize,
  OUT UINT8               *PublicKey
  );

struct _GBL_EFI_AVF_PROTOCOL {
  UINT64                                       Revision;
  GBL_EFI_AVF_READ_VENDOR_DICE_HANDOVER        ReadVendorDiceHandover;
  GBL_EFI_AVF_READ_SECRET_KEEPER_PUBLIC_KEY    ReadSecretKeeperPublicKey;
};

#endif // GBL_EFI_AVF_PROTOCOL_H_
