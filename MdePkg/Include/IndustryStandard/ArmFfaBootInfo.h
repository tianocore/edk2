/** @file
  Boot information protocol definitions as specified in the
  FF-A v1.2 specification.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - FF-A - Firmware Framework for Arm A-profile

  @par Reference(s):
    - FF-A Version 1.2 [https://developer.arm.com/documentation/den0077/latest/]

**/

#ifndef ARM_FFA_BOOT_INFO_H_
#define ARM_FFA_BOOT_INFO_H_

#define FFA_BOOT_INFO_SIGNATURE  0x00000FFA

#define FFA_BOOT_INFO_NAME_SIZE  16

/* Boot information type. */
#define FFA_BOOT_INFO_TYPE_STD     0x0U
#define FFA_BOOT_INFO_TYPE_IMPDEF  0x1U

#define FFA_BOOT_INFO_TYPE_MASK   0x1U
#define FFA_BOOT_INFO_TYPE_SHIFT  0x7U
#define FFA_BOOT_INFO_TYPE(type) \
  (((type) >> FFA_BOOT_INFO_TYPE_SHIFT) & FFA_BOOT_INFO_TYPE_MASK)

/* Boot information identifier. */
#define FFA_BOOT_INFO_TYPE_ID_FDT  0x0U
#define FFA_BOOT_INFO_TYPE_ID_HOB  0x1U

#define FFA_BOOT_INFO_TYPE_ID_MASK   0x3FU
#define FFA_BOOT_INFO_TYPE_ID_SHIFT  0x0U
#define FFA_BOOT_INFO_TYPE_ID(type) \
  (((type) >> FFA_BOOT_INFO_TYPE_ID_SHIFT) & FFA_BOOT_INFO_TYPE_ID_MASK)

/* Format of Flags Name field. */
#define FFA_BOOT_INFO_FLAG_NAME_STRING  0x0U
#define FFA_BOOT_INFO_FLAG_NAME_UUID    0x1U

#define FFA_BOOT_INFO_FLAG_NAME_MASK   0x3U
#define FFA_BOOT_INFO_FLAG_NAME_SHIFT  0x0U
#define FFA_BOOT_INFO_FLAG_NAME(flag) \
  (((flag) >> FFA_BOOT_INFO_FLAG_NAME_SHIFT) & FFA_BOOT_INFO_FLAG_NAME_MASK)

/* Format of Flags Contents field. */
#define FFA_BOOT_INFO_FLAG_CONTENT_ADDR  0x0U
#define FFA_BOOT_INFO_FLAG_CONTENT_VAL   0x1U

#define FFA_BOOT_INFO_FLAG_CONTENT_MASK   0x1U
#define FFA_BOOT_INFO_FLAG_CONTENT_SHIFT  0x2U
#define FFA_BOOT_INFO_FLAG_CONTENT(flag) \
  (((flag) >> FFA_BOOT_INFO_FLAG_CONTENT_SHIFT) & FFA_BOOT_INFO_FLAG_CONTENT_MASK)

/** Descriptor to pass boot information as per the FF-A v1.2 spec.
 */
typedef struct {
  /// Name of Boot information
  UINT8     Name[FFA_BOOT_INFO_NAME_SIZE];

  /// Type of boot information
  UINT8     Type;

  /// Reserved
  UINT8     Reserved;

  /// Flags to describe properties of boot information
  UINT16    Flags;

  /// Size (in bytes) of boot information
  UINT32    Size;

  /// Address of boot information
  UINT64    Content;
} EFI_FFA_BOOT_INFO_DESC;

/** Descriptor that contains boot info blobs size, number of desc it contains
 *  size of each descriptor and offset to the first descriptor.
 */
typedef struct {
  /// Hexadecimal value FFA_BOOT_INFO_SIGNATURE to identify the header
  UINT32    Magic;

  /// Version of the boot information blob
  UINT32    Version;

  /// Size of boot information blob
  UINT32    SizeBootInfoBlob;

  /// Size of boot information descriptor
  UINT32    SizeBootInfoDesc;

  /// Count of boot information descriptor
  UINT32    CountBootInfoDesc;

  ///  Offset to array of boot information descriptors
  UINT32    OffsetBootInfoDesc;

  /// Reserved
  UINT64    Reserved;

  /// Optional Padding
  /// Boot information descriptor array
} EFI_FFA_BOOT_INFO_HEADER;

#endif
