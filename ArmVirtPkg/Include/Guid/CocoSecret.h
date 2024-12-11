/** @file
  CoCo Secret GUID and defnitions.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>

/**
  A macro identifying the GUID for the Confidential Computing Secret Data
  table header.
*/
#define GRUB_EFI_SECRET_TABLE_HEADER_GUID \
  { 0x1e74f542, 0x71dd, 0x4d66, \
    { 0x96, 0x3e, 0xef, 0x42, 0x87, 0xff, 0x17, 0x3b } \
  }

/**
  A macro identifying the GUID for the Confidential Computing Secret Data
  table entry describing the disk decryption secret/password utilised by
  GRUB.
*/
#define GRUB_EFI_DISKPASSWD_GUID \
  { 0x736869e5, 0x84f0, 0x4973, \
    { 0x92, 0xec, 0x06, 0x87, 0x9c, 0xe3, 0xda, 0x0b } \
  }

/**
  A structure representing the Secret Header in
  the EFI COCO Secret table data.
**/
typedef struct {
  /// The confidential computing secret header GUID.
  EFI_GUID    Guid;
  /// Length of the entire secret table.
  UINT32      Length;
} COCO_SECRET_HEADER;

/**
  A structure representing the Secret Entry in
  the EFI COCO Secret table data.
**/
typedef struct {
  /// The confidential computing secret entry GUID representing the
  /// password for the encrypted disk utilised by GRUB.
  EFI_GUID    Guid;
  /// Length of the secret entry.
  UINT32      Length;
  // UINT8    Data[Length];
} COCO_SECRET_ENTRY;

/**
  A GUID identifying the Confidential Computing Secret Data table header.
*/
extern EFI_GUID  gConfidentialComputingSecretHeader;

/**
  A GUID identifying the Confidential Computing Secret Data table entry
  describing the disk decryption secret/password utilised by GRUB.
*/
extern EFI_GUID  gConfidentialComputingSecretEntryGrubEfiDiskPassword;
