/** @file
  This file defines a ACPI_S3_ENABLE structure.

  ACPI_S3_ENABLE structure includes the AcpiS3Enable parameter. It
  indicates ACPI S3 enabled or not. The value shall match with the
  PcdAcpiS3Enable.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_S3_ENABLE_H_
#define ACPI_S3_ENABLE_H_

#include <PiPei.h>

#define ACPI_S3_ENABLE_GUID \
  { \
    0xe7402821, 0x2654, 0x4c1b, {0x99, 0x0e, 0x04, 0x8f, 0x8d, 0x82, 0xcf, 0x67}  \
  }

#pragma pack(1)
typedef struct {
  BOOLEAN    AcpiS3Enable;
} ACPI_S3_ENABLE;
#pragma pack()

extern EFI_GUID  gEdkiiAcpiS3EnableHobGuid;

#endif
