/** @file
  Definition of the ACPI Support protocol.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  AcpiSupport.h

  @par Revision Reference:
  This is defined in the ACPI Specification 0.9.
  
**/

#ifndef _ACPI_SUPPORT_PROTOCOL_H_
#define _ACPI_SUPPORT_PROTOCOL_H_

typedef struct _EFI_ACPI_SUPPORT_PROTOCOL EFI_ACPI_SUPPORT_PROTOCOL;

//
// ACPI Support Protocol GUID
//
#define EFI_ACPI_SUPPORT_GUID \
  { \
    0xdbff9d55, 0x89b7, 0x46da, {0xbd, 0xdf, 0x67, 0x7d, 0x3d, 0xc0, 0x24, 0x1d } \
  }

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

/**
  Returns a requested ACPI table.

  @param  This A pointer to the EFI_ACPI_SUPPORT_PROTOCOL instance.
  
  @param  Index The zero-based index of the table to retrieve.
  
  @param  Table Pointer for returning the table buffer.
  
  @param  Version Updated with the ACPI versions to which this table belongs.
  
  @param  Handle Pointer for identifying the table.

  @retval EFI_SUCCESS The function completed successfully.
  
  @retval EFI_NOT_FOUND The requested index is too large and a table was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_GET_ACPI_TABLE) (
  IN EFI_ACPI_SUPPORT_PROTOCOL            *This,
  IN INTN                                 Index,
  OUT VOID                                **Table,
  OUT EFI_ACPI_TABLE_VERSION              *Version,
  OUT UINTN                               *Handle
  );

/**
  Used to add, remove, or update ACPI tables.

  @param  This A pointer to the EFI_ACPI_SUPPORT_PROTOCOL instance.
  
  @param  Table Pointer to the new table to add or update.
  
  @param  Checksum If TRUE, indicates that the checksum should be 
  calculated for this table.
  
  @param  Version Indicates to which version(s) of ACPI the table should be added.
  
  @param  Pointer to the handle of the table to remove or update.

  @retval EFI_SUCCESS The function completed successfully.
  
  @retval EFI_INVALID_PARAMETER *Handle was zero and Table was NULL.
  
  @retval EFI_ABORTED Could not complete the desired action.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_SET_ACPI_TABLE) (
  IN EFI_ACPI_SUPPORT_PROTOCOL            *This,
  IN VOID                                 *Table OPTIONAL,
  IN BOOLEAN                              Checksum,
  IN EFI_ACPI_TABLE_VERSION               Version,
  IN OUT UINTN                            *Handle
  );

/**
  Causes one or more versions of the ACPI tables to be published in 
  the EFI system configuration tables.

  @param  This A pointer to the EFI_ACPI_SUPPORT_PROTOCOL instance.
  
  @param  Version Indicates to which version(s) of ACPI that the table should be published. 

  @retval EFI_SUCCESS The function completed successfully.
  
  @retval EFI_ABORTED An error occurred and the function could not complete successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_PUBLISH_TABLES) (
  IN EFI_ACPI_SUPPORT_PROTOCOL            *This,
  IN EFI_ACPI_TABLE_VERSION               Version
  );

//
// ACPI Support Protocol
//
/**
  @par Protocol Description:
  This protocol provides some basic services to support publishing ACPI system 
  tables. The services handle many of the more mundane tasks that are required 
  to publish a set of tables. 

  @param GetAcpiTable
  Returns a table specified by an index if it exists.

  @param SetAcpiTable
  Adds, removes, or updates ACPI tables

  @param PublishTables
  Publishes the ACPI tables.

**/
struct _EFI_ACPI_SUPPORT_PROTOCOL {
  EFI_ACPI_GET_ACPI_TABLE GetAcpiTable;
  EFI_ACPI_SET_ACPI_TABLE SetAcpiTable;
  EFI_ACPI_PUBLISH_TABLES PublishTables;
};

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiAcpiSupportProtocolGuid;

#endif
