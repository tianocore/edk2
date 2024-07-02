/** @file
  This Tcg2 Acpi Communicate Buffer HOB is used to store the address
  of a buffer reserved for Tcg2Acpi driver. The buffer will be used to
  retrive information from standalone mm environment.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TCG2_ACPI_COMMUNICATE_BUFFER_H_
#define TCG2_ACPI_COMMUNICATE_BUFFER_H_

#define TCG2_ACPI_COMMUNICATE_BUFFER_HOB_REVISION  1

#define TCG2_ACPI_COMMUNICATE_BUFFER_GUID \
  { \
    0xcefea14f, 0x9f1a, 0x4774, {0x8d, 0x18, 0x79, 0x93, 0x8d, 0x48, 0xfe, 0x7d}  \
  }

typedef struct {
  ///
  /// Base address of the buffer reserved for Tcg2Acpi driver.
  /// Tcg2Acpi will use it to exchange information with Tcg2StandaloneMm.
  ///
  EFI_PHYSICAL_ADDRESS    Tcg2AcpiCommunicateBuffer;
  UINT64                  Pages;
} TCG2_ACPI_COMMUNICATE_BUFFER;

extern EFI_GUID  gEdkiiTcg2AcpiCommunicateBufferHobGuid;

#endif
