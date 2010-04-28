/*++

Copyright (c) 1999 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  AcpiSupport.h

Abstract:

  Definition of the ACPI Support protocol.  This is defined in the 
  Tiano ACPI External Product Specification, revision 0.5.1.

--*/

#ifndef _ACPI_SUPPORT_PROTOCOL_H_
#define _ACPI_SUPPORT_PROTOCOL_H_

//
// Includes
//
#include "Tiano.h"

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_ACPI_SUPPORT_PROTOCOL);

//
// ACPI Support Protocol GUID
//
#define EFI_ACPI_SUPPORT_GUID \
  { \
    0xdbff9d55, 0x89b7, 0x46da, {0xbd, 0xdf, 0x67, 0x7d, 0x3d, 0xc0, 0x24, 0x1d} \
  }

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiAcpiSupportGuid;

//
// Protocol Data Definitions
//
//
// ACPI Version bitmap definition:
//
// EFI_ACPI_TABLE_VERSION_1_0B - ACPI Version 1.0b
// EFI_ACPI_TABLE_VERSION_2_0 - ACPI Version 2.0
// EFI_ACPI_TABLE_VERSION_3_0 - ACPI Version 3.0
// EFI_ACPI_TABLE_VERSION_NONE - No ACPI Versions.  This might be used
//  to create memory-based operation regions or other information
//  that is not part of the ACPI "tree" but must still be found
//  in ACPI memory space and/or managed by the core ACPI driver.
//
// Note that EFI provides discrete GUIDs for each version of ACPI
// that is supported.  It is expected that each EFI GUIDed
// version of ACPI will also have a corresponding bitmap
// definition.  This allows maintenance of separate ACPI trees
// for each distinctly different version of ACPI.
//
#define EFI_ACPI_TABLE_VERSION      UINT32

#define EFI_ACPI_TABLE_VERSION_NONE (1 << 0)
#define EFI_ACPI_TABLE_VERSION_1_0B (1 << 1)
#define EFI_ACPI_TABLE_VERSION_2_0  (1 << 2)
#define EFI_ACPI_TABLE_VERSION_3_0  (1 << 3)

//
// Protocol Member Functions
//
//
// Retrieve a copy of an ACPI table and the handle of the table.
//
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_GET_ACPI_TABLE) (
  IN EFI_ACPI_SUPPORT_PROTOCOL            * This,
  IN INTN                                 Index,
  OUT VOID                                **Table,
  OUT EFI_ACPI_TABLE_VERSION              * Version,
  OUT UINTN                               *Handle
  );

//
// Add, update, or remove a table.
//
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_SET_ACPI_TABLE) (
  IN EFI_ACPI_SUPPORT_PROTOCOL            * This,
  IN VOID                                 *Table OPTIONAL,
  IN BOOLEAN                              Checksum,
  IN EFI_ACPI_TABLE_VERSION               Version,
  IN OUT UINTN                            *Handle
  );

//
// Publish tables to the outside world
//
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_PUBLISH_TABLES) (
  IN EFI_ACPI_SUPPORT_PROTOCOL            * This,
  IN EFI_ACPI_TABLE_VERSION               Version
  );

//
// ACPI Support Protocol
//
struct _EFI_ACPI_SUPPORT_PROTOCOL {
  EFI_ACPI_GET_ACPI_TABLE GetAcpiTable;
  EFI_ACPI_SET_ACPI_TABLE SetAcpiTable;
  EFI_ACPI_PUBLISH_TABLES PublishTables;
};

#endif
