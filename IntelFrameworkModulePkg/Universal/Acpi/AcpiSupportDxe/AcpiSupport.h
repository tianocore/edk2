/** @file
  This is an implementation of the ACPI Support protocol.
  It is in compliance with the 0.9 definition of the protocol.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ACPI_SUPPORT_H_
#define _ACPI_SUPPORT_H_


#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Guid/Acpi.h>
#include <Protocol/AcpiSupport.h>

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
  LIST_ENTRY          Link;
  EFI_ACPI_TABLE_VERSION  Version;
  EFI_ACPI_COMMON_HEADER  *Table;
  EFI_PHYSICAL_ADDRESS    PageAddress;
  UINTN                   NumberOfPages;
  UINTN                   Handle;
} EFI_ACPI_TABLE_LIST;

//
// Containment record for linked list.
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
#define EFI_ACPI_OEM_TABLE_ID     0x2020204F4E414954ULL  // "TIANO   "
#define EFI_ACPI_OEM_REVISION     0x00000002
#define EFI_ACPI_CREATOR_ID       0x20202020
#define EFI_ACPI_CREATOR_REVISION 0x01000013

//
// Protocol private structure definition
//
//
// ACPI support protocol instance signature definition.
//
#define EFI_ACPI_SUPPORT_SIGNATURE  SIGNATURE_32 ('S', 'S', 'A', 'E')

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
  LIST_ENTRY                                TableList;
  UINTN                                         NumberOfTableEntries1;  // Number of ACPI 1.0 tables
  UINTN                                         NumberOfTableEntries3;  // Number of ACPI 3.0 tables
  UINTN                                         CurrentHandle;
  BOOLEAN                                       TablesInstalled1;       // ACPI 1.0 tables published
  BOOLEAN                                       TablesInstalled3;       // ACPI 3.0 tables published
  EFI_ACPI_SUPPORT_PROTOCOL                     AcpiSupport;
  EFI_ACPI_TABLE_PROTOCOL                       AcpiTableProtocol;
} EFI_ACPI_SUPPORT_INSTANCE;

//
// ACPI support protocol instance containing record macro
//
#define EFI_ACPI_SUPPORT_INSTANCE_FROM_ACPI_SUPPORT_THIS(a) \
  CR (a, \
      EFI_ACPI_SUPPORT_INSTANCE, \
      AcpiSupport, \
      EFI_ACPI_SUPPORT_SIGNATURE \
      )
//
// ACPI table protocol instance containing record macro
//
#define EFI_ACPI_TABLE_INSTANCE_FROM_ACPI_SUPPORT_THIS(a) \
  CR (a, \
      EFI_ACPI_SUPPORT_INSTANCE, \
      AcpiTableProtocol, \
      EFI_ACPI_SUPPORT_SIGNATURE \
      )
      
/**
  Constructor for the ACPI support protocol.  
  
  Constructor for the ACPI support protocol to initializes instance data.
  
  @param AcpiSupportInstance   Instance to construct

  @retval EFI_SUCCESS           Instance initialized.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate required resources.
**/
EFI_STATUS
AcpiSupportAcpiSupportConstructor (
  IN EFI_ACPI_SUPPORT_INSTANCE                 *AcpiSupportInstance
  );
/**
  Entry point of the ACPI support driver. This function creates and initializes an instance of the ACPI Support 
  Protocol and installs it on a new handle.

  @param ImageHandle   A handle for the image that is initializing this driver
  @param SystemTable   A pointer to the EFI system table        

  @retval EFI_SUCCESS              Driver initialized successfully
  @retval EFI_LOAD_ERROR           Failed to Initialize or has been loaded 
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources
**/
EFI_STATUS
EFIAPI
InstallAcpiSupport (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );
#endif
