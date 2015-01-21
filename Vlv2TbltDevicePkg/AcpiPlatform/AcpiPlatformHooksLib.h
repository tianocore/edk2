/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  AcpiPlatformHooksLib.h

Abstract:

  This is an implementation of the ACPI platform driver.  Requirements for
  this driver are defined in the Tiano ACPI External Product Specification,
  revision 0.3.6.

--*/

#ifndef _ACPI_PLATFORM_HOOKS_LIB_H_
#define _ACPI_PLATFORM_HOOKS_LIB_H_

//
// Statements that include other header files.
//
#include <IndustryStandard/Acpi.h>

/**
  Returns the ACPI table version that the platform wants.

  @param[in]  None

  @retval  EFI_ACPI_TABLE_VERSION_NONE  if ACPI is to be disabled.
  @retval  EFI_ACPI_TABLE_VERSION_1_0B  if 1.0b.
  @retval  EFI_ACPI_TABLE_VERSION_2_00  if 2.00.
**/
EFI_ACPI_TABLE_VERSION
AcpiPlatformHooksGetAcpiTableVersion (
  VOID
  );

/**
  Returns the OEMID, OEM Table ID, OEM Revision.

  @param[in]  None

  @retval  OemId        OEM ID string for ACPI tables, maximum 6 ASCII characters.
                        This is an OEM-supplied string that identifies the OEM.
  @retval  OemTableId   An OEM-supplied string that the OEM uses to identify
                        the particular data table. This field is particularly useful
                        when defining a definition block to distinguish definition block
                        functions. The OEM assigns each dissimilar table a new OEM Table ID.
  @retval  OemRevision  An OEM-supplied revision number for ACPI tables.
                        Larger numbers are assumed to be newer revisions.

**/
EFI_STATUS
AcpiPlatformHooksGetOemFields (
  OUT UINT8   *OemId,
  OUT UINT64  *OemTableId,
  OUT UINT32  *OemRevision
  );

 /**
  Called for every ACPI table found in the BIOS flash.
  Returns whether a table is active or not. Inactive tables
  are not published in the ACPI table list. This hook can be
  used to implement optional SSDT tables or enabling/disabling
  specific functionality (e.g. SPCR table) based on a setup
  switch or platform preference. In case of optional SSDT tables,
  the platform flash will include all the SSDT tables but will
  return EFI_SUCCESS only for those tables that need to be
  published.
  This hook can also be used to update the table data. The header
  is updated by the common code. For example, if a platform wants
  to use an SSDT table to export some platform settings to the
  ACPI code, it needs to update the data inside that SSDT based
  on platform preferences in this hook.

  @param[in]  None

  @retval  EFI_SUCCESS      if the table is active.
  @retval  EFI_UNSUPPORTED  if the table is not active.
**/
EFI_STATUS
AcpiPlatformHooksIsActiveTable (
  IN OUT EFI_ACPI_COMMON_HEADER     *Table
  );

#endif
