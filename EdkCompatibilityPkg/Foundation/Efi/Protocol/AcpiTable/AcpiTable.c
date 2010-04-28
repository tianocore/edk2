/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  AcpiTable.c

Abstract:

  ACPI Table Protocol from the UEFI 2.1 specification.

  This protocol may be used to install or remove an ACPI table from a platform.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (AcpiTable)

EFI_GUID  gEfiAcpiTableProtocolGuid = EFI_ACPI_TABLE_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiAcpiTableProtocolGuid, "UEFI ACPI Table Protocol", "UEFI ACPI Table Protocol");
