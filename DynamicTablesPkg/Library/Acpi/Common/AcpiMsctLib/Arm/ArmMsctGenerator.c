/** @file
  ARM specific ACPI Maximum System Characteristics Table (MSCT) Generator

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Library/CmObjHelperLib.h>

#include "MsctGenerator.h"

/**
  ARM standard MSCT Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
    - EArmObjGicCInfo (OPTIONAL)
*/

/** This macro expands to a function that retrieves the GIC
    CPU interface Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicCInfo,
  CM_ARM_GICC_INFO
  );

/** Get architecture processor domain information.

  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     ProcDomainInfo       Pointer to the processor domain information.
  @param [out]     ProcDomainInfoCount  Pointer to the count of processor domain
                                        information structures.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Other                 Other EFI_STATUS error from called functions.
**/
EFI_STATUS
EFIAPI
GetArchProcessorDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        **ProcDomainInfo,
  OUT       UINT32                                        *ProcDomainInfoCount
  )
{
  CM_ARM_GICC_INFO  *CmProcDomain;
  EFI_STATUS        Status;
  UINT32            Index;
  UINT32            ProcDomainCount;
  UINT32            *ProcDomain;

  if ((CfgMgrProtocol == NULL) ||
      (ProcDomainInfo == NULL) ||
      (ProcDomainInfoCount == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  CmProcDomain    = NULL;
  ProcDomain      = NULL;
  ProcDomainCount = 0;

  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CmProcDomain,
             &ProcDomainCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Failed to get GICC Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  ProcDomain = AllocateZeroPool (
                 sizeof (UINT32) *
                 ProcDomainCount
                 );
  if (ProcDomain == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (Index = 0; Index < ProcDomainCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               CmProcDomain[Index].ProximityDomain,
               CmProcDomain[Index].ProximityDomainToken,
               &ProcDomain[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "MSCT: Failed to get Proximity Domain ID for index %d. Status = %r\n",
        Index,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }
  }

  *ProcDomainInfo      = ProcDomain;
  *ProcDomainInfoCount = ProcDomainCount;
  return EFI_SUCCESS;

error_handler:

  if (ProcDomain != NULL) {
    FreePool (ProcDomain);
  }

  return Status;
}

/** Get architecture clock domain information.

  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     MaxClockDomain      Pointer to the maximum clock domain
                                        information structures.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Other                 Other return status.
**/
EFI_STATUS
EFIAPI
GetArchClockDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        *MaxClockDomain
  )
{
  EFI_STATUS        Status;
  UINT32            ProximityDomain;
  UINT32            CmCount;
  UINT32            Index;
  CM_ARM_GICC_INFO  *CmGiccInfo;
  UINT32            MaxClockProximityDomain;

  if ((CfgMgrProtocol == NULL) || (MaxClockDomain == NULL)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  if ((CfgMgrProtocol == NULL) ||
      (MaxClockDomain == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  CmGiccInfo = NULL;
  CmCount    = 0;

  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CmGiccInfo,
             &CmCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Failed to get GICC Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  MaxClockProximityDomain = 0;
  for (Index = 0; Index < CmCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               CmGiccInfo->ClockDomain,
               CmGiccInfo->ClockDomainToken,
               &ProximityDomain
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: MSCT: Failed to get Clock Domain ID for index %d. Status = %r\n",
        Index,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    if (ProximityDomain > MaxClockProximityDomain) {
      MaxClockProximityDomain = ProximityDomain;
    }
  }

  *MaxClockDomain = MaxClockProximityDomain;
  return EFI_SUCCESS;
}
