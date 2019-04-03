/** @file
  FirmwareInterfaceTable (FIT) related definitions.

  @todo update document/spec reference

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FIRMWARE_INTERFACE_TABLE_H__
#define __FIRMWARE_INTERFACE_TABLE_H__

//
// FIT Entry type definitions
//
#define FIT_TYPE_00_HEADER                  0x00
#define FIT_TYPE_01_MICROCODE               0x01
#define FIT_TYPE_02_STARTUP_ACM             0x02
#define FIT_TYPE_07_BIOS_STARTUP_MODULE     0x07
#define FIT_TYPE_08_TPM_POLICY              0x08
#define FIT_TYPE_09_BIOS_POLICY             0x09
#define FIT_TYPE_0A_TXT_POLICY              0x0A
#define FIT_TYPE_0B_KEY_MANIFEST            0x0B
#define FIT_TYPE_0C_BOOT_POLICY_MANIFEST    0x0C
#define FIT_TYPE_10_CSE_SECURE_BOOT         0x10
#define FIT_TYPE_2D_TXTSX_POLICY            0x2D
#define FIT_TYPE_2F_JMP_DEBUG_POLICY        0x2F
#define FIT_TYPE_7F_SKIP                    0x7F

#define FIT_POINTER_ADDRESS                 0xFFFFFFC0 ///< Fixed address at 4G - 40h

#define FIT_TYPE_VERSION                    0x0100

#define FIT_TYPE_00_SIGNATURE  SIGNATURE_64 ('_', 'F', 'I', 'T', '_', ' ', ' ', ' ')

#pragma pack(1)

typedef struct {
  /**
    Address is the base address of the firmware component
    must be aligned on 16 byte boundary
  **/
  UINT64 Address;
  UINT8  Size[3];   ///< Size is the span of the component in multiple of 16 bytes
  UINT8  Reserved;  ///< Reserved must be set to 0
  /**
    Component's version number in binary coded decimal (BCD) format.
    For the FIT header entry, the value in this field will indicate the revision
    number of the FIT data structure. The upper byte of the revision field
    indicates the major revision and the lower byte indicates the minor revision.
  **/
  UINT16 Version;
  UINT8  Type : 7;  ///< FIT types 0x00 to 0x7F
  ///
  /// Checksum Valid indicates whether component has valid checksum.
  ///
  UINT8  C_V  : 1;
  /**
    Component's checksum. The modulo sum of all the bytes in the component and
    the value in this field (Chksum) must add up to zero. This field is only
    valid if the C_V flag is non-zero.
  **/
  UINT8  Chksum;
} FIRMWARE_INTERFACE_TABLE_ENTRY;

#pragma pack()

#endif
