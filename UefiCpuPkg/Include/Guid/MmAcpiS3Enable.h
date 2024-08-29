/** @file
  This file defines ACPI_S3_ENABLE structure which indicates to x86 standalone MM whether S3 is enabled.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_ACPI_S3_ENABLE_H_
#define MM_ACPI_S3_ENABLE_H_

///
/// The GUID of the MmAcpiS3Enable GUIDed HOB.
///
#define MM_ACPI_S3_ENABLE_HOB_GUID \
  { \
    0xe7402821, 0x2654, 0x4c1b, {0x99, 0x0e, 0x04, 0x8f, 0x8d, 0x82, 0xcf, 0x67}  \
  }

///
/// The structure defines the data layout of the MmAcpiS3Enable GUIDed HOB.
///
typedef struct {
  ///
  /// Whether ACPI S3 is enabled.
  /// The value shall match with the PcdAcpiS3Enable.
  ///
  BOOLEAN    AcpiS3Enable;
} MM_ACPI_S3_ENABLE;

extern EFI_GUID  gMmAcpiS3EnableHobGuid;

#endif
