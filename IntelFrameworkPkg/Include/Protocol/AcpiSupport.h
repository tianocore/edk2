/** @file
  This protocol provides some basic services to support publishing ACPI system tables. The
  services handle many of the more mundane tasks that are required to publish a set of tables. The
  services will:
        - Generate common tables.
  - Update the table links.
  - Ensure that tables are properly aligned and use correct types of memory.
  - Update checksum values and IDs.
  - Complete the final installation of the tables.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This Protocol is defined in Framework ACPI Specification.
  Version 0.9.

**/

#ifndef _ACPI_SUPPORT_PROTOCOL_H_
#define _ACPI_SUPPORT_PROTOCOL_H_

#include <Protocol/AcpiSystemDescriptionTable.h>

typedef struct _EFI_ACPI_SUPPORT_PROTOCOL EFI_ACPI_SUPPORT_PROTOCOL;

//
// ACPI Support Protocol GUID
//
#define EFI_ACPI_SUPPORT_GUID \
  { \
    0xdbff9d55, 0x89b7, 0x46da, {0xbd, 0xdf, 0x67, 0x7d, 0x3d, 0xc0, 0x24, 0x1d } \
  }


//
// Protocol Member Functions
//

/**
  Returns a requested ACPI table.

  @param  This                  A pointer to the EFI_ACPI_SUPPORT_PROTOCOL instance.
  @param  Index                 The zero-based index of the table to retrieve.
  @param  Table                 The pointer for returning the table buffer.
  @param  Version               Updated with the ACPI versions to which this table belongs.
  @param  Handle                The pointer for identifying the table.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The requested index is too large and a table was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_GET_ACPI_TABLE)(
  IN EFI_ACPI_SUPPORT_PROTOCOL            *This,
  IN INTN                                 Index,
  OUT VOID                                **Table,
  OUT EFI_ACPI_TABLE_VERSION              *Version,
  OUT UINTN                               *Handle
  );

/**
  Used to add, remove, or update ACPI tables.

  @param  This                  A pointer to the EFI_ACPI_SUPPORT_PROTOCOL instance.
  @param  Table                 The pointer to the new table to add or update.
  @param  Checksum              If TRUE, indicates that the checksum should be
                                calculated for this table.
  @param  Version               Indicates to which version(s) of ACPI the table should be added.
  @param  Handle                The pointer to the handle of the table to remove or update.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER *Handle was zero and Table was NULL.
  @retval EFI_ABORTED           Could not complete the desired action.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_SET_ACPI_TABLE)(
  IN EFI_ACPI_SUPPORT_PROTOCOL            *This,
  IN VOID                                 *Table OPTIONAL,
  IN BOOLEAN                              Checksum,
  IN EFI_ACPI_TABLE_VERSION               Version,
  IN OUT UINTN                            *Handle
  );

/**
  Causes one or more versions of the ACPI tables to be published in
  the EFI system configuration tables.

  The PublishTables() function installs the ACPI tables for the versions that are specified in
  Version. No tables are published for Version equal to EFI_ACPI_VERSION_NONE. Once
  published, tables will continue to be updated as tables are modified with
  EFI_ACPI_SUPPORT_PROTOCOL.SetAcpiTable().

  @param  This                  A pointer to the EFI_ACPI_SUPPORT_PROTOCOL instance.
  @param  Version               Indicates to which version(s) of ACPI the table should be published.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ABORTED           An error occurred and the function could not complete successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_PUBLISH_TABLES)(
  IN EFI_ACPI_SUPPORT_PROTOCOL            *This,
  IN EFI_ACPI_TABLE_VERSION               Version
  );

//
// ACPI Support Protocol
//
/**
  This protocol provides some basic services to support publishing ACPI system
  tables. The services handle many of the more mundane tasks that are required
  to publish a set of tables.
**/
struct _EFI_ACPI_SUPPORT_PROTOCOL {
  ///
  /// Returns a table specified by an index if it exists.
  ///
  EFI_ACPI_GET_ACPI_TABLE GetAcpiTable;

  ///
  /// Adds, removes, or updates ACPI tables.
  ///
  EFI_ACPI_SET_ACPI_TABLE SetAcpiTable;

  ///
  /// Publishes the ACPI tables.
  ///
  EFI_ACPI_PUBLISH_TABLES PublishTables;
};

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiAcpiSupportProtocolGuid;

#endif

