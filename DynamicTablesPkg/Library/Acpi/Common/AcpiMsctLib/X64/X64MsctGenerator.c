/** @file
  X64 specific ACPI Maximum System Characteristics Table (MSCT) Generator

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
  This macro is used to get the object information for the Local APIC X2APIC
  Affinity object.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjLocalApicX2ApicAffinityInfo,
  CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO
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
  CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO  *CmProcDomain;
  EFI_STATUS                              Status;
  UINT32                                  Index;
  UINT32                                  ProcDomainCount;
  UINT32                                  *ProcDomain;

  if ((CfgMgrProtocol == NULL) ||
      (ProcDomainInfo == NULL) ||
      (ProcDomainInfoCount == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  CmProcDomain    = NULL;
  ProcDomain      = NULL;
  ProcDomainCount = 0;
  Status          = GetEX64ObjLocalApicX2ApicAffinityInfo (
                      CfgMgrProtocol,
                      CM_NULL_TOKEN,
                      &CmProcDomain,
                      &ProcDomainCount
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MSCT: Failed to get processor affinity information. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  if ((ProcDomainCount == 0) || (CmProcDomain == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "MSCT: No Local Apic/X2Apic Affinity information found.\n"
      ));
    Status = EFI_NOT_FOUND;
    goto error_handler;
  }

  ProcDomain = AllocateZeroPool (
                 sizeof (UINT32) *
                 ProcDomainCount
                 );
  if (ProcDomain == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto error_handler;
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
  EFI_STATUS                              Status;
  UINT32                                  ProximityDomain;
  UINT32                                  CmCount;
  UINT32                                  Index;
  UINT32                                  MaxClockProximityDomain;
  CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO  *CmX2ApicAffinity;

  if ((CfgMgrProtocol == NULL) || (MaxClockDomain == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CmX2ApicAffinity = NULL;
  CmCount          = 0;
  Status           = GetEX64ObjLocalApicX2ApicAffinityInfo (
                       CfgMgrProtocol,
                       CM_NULL_TOKEN,
                       &CmX2ApicAffinity,
                       &CmCount
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: Failed to get Local Apic/X2Apic Affinity Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (CmCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: Local Apic/X2Apic Affinity information not provided.\n"
      ));
    return EFI_NOT_FOUND;
  }

  MaxClockProximityDomain = 0;
  for (Index = 0; Index < CmCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               CmX2ApicAffinity[Index].ClockDomain,
               CmX2ApicAffinity[Index].ClockDomainToken,
               &ProximityDomain
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SRAT: Failed to get Clock Domain ID for index %d. Status = %r\n",
        Index,
        Status
        ));
      return Status;
    }

    if (ProximityDomain > MaxClockProximityDomain) {
      MaxClockProximityDomain = ProximityDomain;
    }
  }

  *MaxClockDomain = MaxClockProximityDomain;
  return EFI_SUCCESS;
}
