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

/** Get the count of unique architecture clock domains.

  Enumerates all GIC CPU interfaces and returns the number of unique clock
  proximity domain IDs in the system.

  @param [in]      CfgMgrProtocol    Pointer to the Configuration Manager
                                     Protocol.
  @param [out]     ClockDomainCount  On success, the cardinal count of unique
                                     clock domains.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Other                 Other EFI_STATUS error from called functions.
**/
EFI_STATUS
EFIAPI
GetArchClockDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       UINT32                                        *ClockDomainCount
  )
{
  EFI_STATUS        Status;
  UINT32            ProximityDomain;
  UINT32            CmCount;
  UINT32            Index;
  UINT32            UniqueIndex;
  CM_ARM_GICC_INFO  *CmGiccInfo;
  UINT32            *ClockDomains;
  UINT32            UniqueCount;
  BOOLEAN           IsUnique;

  if ((CfgMgrProtocol == NULL) ||
      (ClockDomainCount == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  CmGiccInfo   = NULL;
  ClockDomains = NULL;
  UniqueCount  = 0;
  CmCount      = 0;

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

  if ((CmGiccInfo == NULL) || (CmCount == 0)) {
    *ClockDomainCount = 0;
    return EFI_SUCCESS;
  }

  // Allocate worst-case storage: every CPU could be in its own clock domain.
  ClockDomains = AllocateZeroPool (sizeof (UINT32) * CmCount);
  if (ClockDomains == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (Index = 0; Index < CmCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               CmGiccInfo[Index].ClockDomain,
               CmGiccInfo[Index].ClockDomainToken,
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
      goto error_handler;
    }

    // Record the proximity domain ID only if not already seen.
    IsUnique = TRUE;
    for (UniqueIndex = 0; UniqueIndex < UniqueCount; UniqueIndex++) {
      if (ClockDomains[UniqueIndex] == ProximityDomain) {
        IsUnique = FALSE;
        break;
      }
    }

    if (IsUnique) {
      ClockDomains[UniqueCount++] = ProximityDomain;
    }
  }

  *ClockDomainCount = UniqueCount;
  Status            = EFI_SUCCESS;

error_handler:
  if (ClockDomains != NULL) {
    FreePool (ClockDomains);
  }

  return Status;
}
