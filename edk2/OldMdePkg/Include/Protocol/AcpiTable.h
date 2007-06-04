/** @file
  This protocol may be used to install or remove an ACPI table from a platform.

  Copyright (c) 2007, Intel Corporation                                                     
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name: AcpiTable.h

  @par Revision Reference:
  ACPI Table Protocol from the UEFI 2.1 specification.

**/

#ifndef __ACPI_TABLE_H__
#define __ACPI_TABLE_H__

//
// Global ID for the Acpi Table Protocol
//
#define EFI_ACPI_TABLE_PROTOCOL_GUID \
  { \
    0xffe06bdd, 0x6107, 0x46a6, {0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c} \
  }

typedef struct _EFI_ACPI_TABLE_PROTOCOL   EFI_ACPI_TABLE_PROTOCOL;

#define UEFI_ACPI_TABLE_SIGNATURE         EFI_SIGNATURE_32 ('U', 'E', 'F', 'I')

/**
  Installs an ACPI table into the RSDT/XSDT.

  @param  This                   Protocol instance pointer. 
  @param  AcpiTableBuffer        A pointer to a buffer containing the ACPI table 
                                 to be installed. 
  @param  AcpiTableBufferSize    Specifies the size, in bytes, of the 
                                 AcpiTableBuffer buffer. 
  @param  TableKey               Reurns a key to refer to the ACPI table. 

  @retval EFI_SUCCESS            The table was successfully inserted. 
  @retval EFI_INVALID_PARAMETER  Either AcpiTableBuffer is NULL, TableKey is 
                                 NULL, or AcpiTableBufferSize  and the size field 
                                 embedded in the ACPI table pointed to by 
                                 AcpiTableBuffer are not in sync. 
  @retval EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the 
                                 request. 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_TABLE_INSTALL_ACPI_TABLE) (
  IN EFI_ACPI_TABLE_PROTOCOL                    *This,
  IN VOID                                       *AcpiTableBuffer,
  IN UINTN                                      AcpiTableBufferSize,
  OUT UINTN                                     *TableKey
  )
;

/**
  Removes an ACPI table from the RSDT/XSDT.

  @param  This                   Protocol instance pointer. 
  @param  TableKey               Specifies the table to uninstall.  The key was 
                                 returned from InstallAcpiTable(). 

  @retval EFI_SUCCESS            The table was successfully uninstalled. 
  @retval EFI_NOT_FOUND          TableKey does not refer to a valid key for a 
                                 table entry. 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE) (
  IN EFI_ACPI_TABLE_PROTOCOL                    *This,
  IN UINTN                                      TableKey
  )
;

//
// Interface structure for the ACPI Table Protocol
//
struct _EFI_ACPI_TABLE_PROTOCOL {
  EFI_ACPI_TABLE_INSTALL_ACPI_TABLE    InstallAcpiTable;
  EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE  UninstallAcpiTable;
};

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiAcpiTableProtocolGuid;

#endif
