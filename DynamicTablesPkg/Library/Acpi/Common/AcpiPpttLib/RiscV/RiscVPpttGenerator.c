/** @file
  RISC-V PPTT Table Generator

  Copyright (c) 2026, Ilya Somkin. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.4 Specification, January 2021

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/

#include <Library/DebugLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "PpttGenerator.h"

/**
  This macro expands to a function that retrieves the RINTC CPU interface
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjRintcInfo,
  CM_RISCV_RINTC_INFO
  );

/** Get the ACPI Processor UID of the CPU interface objects.

  The CPU interface objects are the GICC objects on Arm and the RINTC objects on
  RISC-V.

  @param [in]  CfgMgrProtocol    Pointer to the Configuration Manager Protocol
                                 Interface.
  @param [in]  Token             Token of the CPU interface object to get or
                                 CM_NULL_TOKEN to get all of them.
  @param [out] AcpiProcessorUid  ACPI Processor UID of the first CPU interface
                                 object found.
  @param [out] Count             Number of CPU interface objects found.

  @retval EFI_SUCCESS            The CPU interface information was returned.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_NOT_FOUND          No CPU interface object was found.
  @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration Manager
                                 is less than the object size.
**/
EFI_STATUS
EFIAPI
GetCpuInterfaceInfo (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        CM_OBJECT_TOKEN                              Token,
  OUT       UINT32                                       *AcpiProcessorUid,
  OUT       UINT32                                       *Count
  )
{
  EFI_STATUS           Status;
  CM_RISCV_RINTC_INFO  *RintcInfo;

  RintcInfo = NULL;

  Status = GetERiscVObjRintcInfo (CfgMgrProtocol, Token, &RintcInfo, Count);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *AcpiProcessorUid = (*Count != 0) ? RintcInfo->AcpiProcessorUid : 0;

  return EFI_SUCCESS;
}
