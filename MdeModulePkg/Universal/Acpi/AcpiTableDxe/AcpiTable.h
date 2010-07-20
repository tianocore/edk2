/** @file
  ACPI Table Protocol Driver

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ACPI_TABLE_H_
#define _ACPI_TABLE_H_


#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Guid/Acpi.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/DxeSmmReadyToLock.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

//
// Statements that include other files
//
#include <IndustryStandard/Acpi.h>

#include "AcpiSdt.h"

//
// From Protocol/AcpiSupport.h
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
// Private Driver Data
//
//
// ACPI Table Linked List Signature.
//
#define EFI_ACPI_TABLE_LIST_SIGNATURE SIGNATURE_32 ('E', 'A', 'T', 'L')

//
// ACPI Table Linked List Entry definition.
//
//  Signature must be set to EFI_ACPI_TABLE_LIST_SIGNATURE
//  Link is the linked list data.
//  Version is the versions of the ACPI tables that this table belongs in.
//  Table is a pointer to the table.
//  PageAddress is the address of the pages allocated for the table.
//  NumberOfPages is the number of pages allocated at PageAddress.
//  Handle is used to identify a particular table.
//
typedef struct {
  UINT32                  Signature;
  LIST_ENTRY              Link;
  EFI_ACPI_TABLE_VERSION  Version;
  EFI_ACPI_COMMON_HEADER  *Table;
  EFI_PHYSICAL_ADDRESS    PageAddress;
  UINTN                   NumberOfPages;
  UINTN                   Handle;
} EFI_ACPI_TABLE_LIST;

//
// Containment record for ACPI Table linked list.
//
#define EFI_ACPI_TABLE_LIST_FROM_LINK(_link)  CR (_link, EFI_ACPI_TABLE_LIST, Link, EFI_ACPI_TABLE_LIST_SIGNATURE)

//
// The maximum number of tables this driver supports
//
#define EFI_ACPI_MAX_NUM_TABLES 20

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID           "INTEL "
#define EFI_ACPI_OEM_TABLE_ID     SIGNATURE_64('E', 'D', 'K', '2', ' ', ' ', ' ', ' ')
#define EFI_ACPI_OEM_REVISION     0x00000002
#define EFI_ACPI_CREATOR_ID       0x20202020
#define EFI_ACPI_CREATOR_REVISION 0x01000013

//
// Protocol private structure definition
//
//
// ACPI support protocol instance signature definition.
//
#define EFI_ACPI_TABLE_SIGNATURE  SIGNATURE_32 ('S', 'T', 'A', 'E')

//
// ACPI support protocol instance data structure
//
typedef struct {
  UINTN                                         Signature;
  EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp1;                 // Pointer to RSD_PTR structure
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp3;                 // Pointer to RSD_PTR structure
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt1;                 // Pointer to RSDT table header
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt3;                 // Pointer to RSDT table header
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;                  // Pointer to XSDT table header
  EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt1;                 // Pointer to FADT table header
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt3;                 // Pointer to FADT table header
  EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs1;                 // Pointer to FACS table header
  EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs3;                 // Pointer to FACS table header
  EFI_ACPI_DESCRIPTION_HEADER                   *Dsdt1;                 // Pointer to DSDT table header
  EFI_ACPI_DESCRIPTION_HEADER                   *Dsdt3;                 // Pointer to DSDT table header
  LIST_ENTRY                                    TableList;
  UINTN                                         NumberOfTableEntries1;  // Number of ACPI 1.0 tables
  UINTN                                         NumberOfTableEntries3;  // Number of ACPI 3.0 tables
  UINTN                                         CurrentHandle;
  BOOLEAN                                       TablesInstalled1;       // ACPI 1.0 tables published
  BOOLEAN                                       TablesInstalled3;       // ACPI 3.0 tables published
  EFI_ACPI_TABLE_PROTOCOL                       AcpiTableProtocol;
  EFI_ACPI_SDT_PROTOCOL                         AcpiSdtProtocol;
  LIST_ENTRY                                    NotifyList;
} EFI_ACPI_TABLE_INSTANCE;

//
// ACPI table protocol instance containing record macro
//
#define EFI_ACPI_TABLE_INSTANCE_FROM_THIS(a) \
  CR (a, \
      EFI_ACPI_TABLE_INSTANCE, \
      AcpiTableProtocol, \
      EFI_ACPI_TABLE_SIGNATURE \
      )

//
// Protocol Constructor functions
//

/**
  Constructor for the ACPI support protocol.  Initializes instance
  data.

  @param  AcpiTableInstance       Instance to construct

  @return EFI_SUCCESS             Instance initialized.
  @return EFI_OUT_OF_RESOURCES    Unable to allocate required resources.

**/
EFI_STATUS
AcpiTableAcpiTableConstructor (
  EFI_ACPI_TABLE_INSTANCE                 *AcpiTableInstance
  );


/**
  Entry point of the ACPI table driver.
  Creates and initializes an instance of the ACPI Table 
  Protocol and installs it on a new handle.

  @param  ImageHandle   A handle for the image that is initializing this driver
  @param  SystemTable   A pointer to the EFI system table

  @return EFI_SUCCESS           Driver initialized successfully
  @return EFI_LOAD_ERROR        Failed to Initialize or has been loaded 
  @return EFI_OUT_OF_RESOURCES  Could not allocate needed resources

**/
EFI_STATUS
EFIAPI
InitializeAcpiTableDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

/**

  This function finds the table specified by the handle and returns a pointer to it.
  If the handle is not found, EFI_NOT_FOUND is returned and the contents of Table are
  undefined.

  @param[in]  Handle      Table to find.
  @param[in]  TableList   Table list to search
  @param[out] Table       Pointer to table found. 

  @retval EFI_SUCCESS              The function completed successfully.
  @retval EFI_NOT_FOUND            No table found matching the handle specified.

**/
EFI_STATUS
FindTableByHandle (
  IN UINTN                                Handle,
  IN LIST_ENTRY                           *TableList,
  OUT EFI_ACPI_TABLE_LIST                 **Table
  );

/**

  This function calculates and updates an UINT8 checksum.

  @param[in]  Buffer          Pointer to buffer to checksum
  @param[in]  Size            Number of bytes to checksum
  @param[in]  ChecksumOffset  Offset to place the checksum result in

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
AcpiPlatformChecksum (
  IN VOID       *Buffer,
  IN UINTN      Size,
  IN UINTN      ChecksumOffset
  );

/**
  This function invokes ACPI notification.

  @param[in]  AcpiTableInstance          Instance to AcpiTable
  @param[in]  Version                    Version(s) to set.
  @param[in]  Handle                     Handle of the table.
**/
VOID
SdtNotifyAcpiList (
  IN EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance,
  IN EFI_ACPI_TABLE_VERSION    Version,
  IN UINTN                     Handle
  );

/**
  This function initializes AcpiSdt protocol in ACPI table instance.

  @param[in]  AcpiTableInstance       Instance to construct
**/
VOID
SdtAcpiTableAcpiSdtConstructor (
  IN EFI_ACPI_TABLE_INSTANCE   *AcpiTableInstance
  );

//
// export PrivateData symbol, because we need that in AcpiSdtProtol implementation
//
extern EFI_HANDLE                mHandle;
extern EFI_ACPI_TABLE_INSTANCE   *mPrivateData;

#endif
