/** @file

  KCS instance of Manageability Transport Library

  Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <IndustryStandard/IpmiKcs.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/MemoryDiscovered.h>

#include "ManageabilityTransportKcs.h"

typedef struct {
  MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO    HardwareInfo;
  MANAGEABILITY_TRANSPORT_KCS                  *SessionToken;
  BOOLEAN                                      PermanentMemory;
} KCS_INFO_HOB;

/**
  Return the current HOB and abandon temporary-memory ownership on transition.

  Private allocation graphs are not rebased by PEI Core. Never inspect or free
  the old SessionToken: temporary RAM may already be unavailable. Latch the
  generation before returning so later lookups preserve a new permanent token.

  @retval NULL   HOB allocation or memory-discovery lookup failed.
  @retval Other  Current KCS state, with session ownership in this generation.
**/
STATIC
KCS_INFO_HOB *
GetKcsInfoHob (
  VOID
  )
{
  EFI_STATUS    Status;
  VOID          *MemoryDiscovered;
  VOID          *GuidHob;
  KCS_INFO_HOB  *HobInstance;

  Status = PeiServicesLocatePpi (&gEfiPeiMemoryDiscoveredPpiGuid, 0, NULL, &MemoryDiscovered);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "%a: Memory Discovered lookup failed - %r\n", __func__, Status));
    return NULL;
  }

  GuidHob = GetFirstGuidHob (&gManageabilityKcsInfoHobGuid);

  if (GuidHob == NULL) {
    HobInstance = (KCS_INFO_HOB *)BuildGuidHob (&gManageabilityKcsInfoHobGuid, sizeof (KCS_INFO_HOB));
    if (HobInstance == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to build guid hob.\n", __func__));
      return NULL;
    }

    ZeroMem (HobInstance, sizeof (*HobInstance));
  } else {
    HobInstance = GET_GUID_HOB_DATA (GuidHob);
  }

  if (!EFI_ERROR (Status) && !HobInstance->PermanentMemory) {
    HobInstance->SessionToken    = NULL;
    HobInstance->PermanentMemory = TRUE;
    DEBUG ((DEBUG_INFO, "%a: KCS session ownership moved to permanent memory.\n", __func__));
  }

  return HobInstance;
}

/**
  Get the KCS hardware information stored in the KCS HOB.

  @retval NULL   The KCS HOB is unavailable.
  @retval Other  Pointer to the KCS hardware information.
**/
MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *
EFIAPI
KcsGetHardwareInfo (
  VOID
  )
{
  KCS_INFO_HOB  *KcsInfo;

  KcsInfo = GetKcsInfoHob ();
  if (KcsInfo == NULL) {
    return NULL;
  }

  return &KcsInfo->HardwareInfo;
}

/**
  Get the KCS session token of the current memory generation.

  @retval NULL   No session is acquired, or the KCS HOB is unavailable.
  @retval Other  Pointer to the current session token.
**/
MANAGEABILITY_TRANSPORT_KCS *
EFIAPI
KcsGetSessionToken (
  VOID
  )
{
  KCS_INFO_HOB  *KcsInfo;

  KcsInfo = GetKcsInfoHob ();
  if (KcsInfo == NULL) {
    return NULL;
  }

  return KcsInfo->SessionToken;
}

/**
  Record the KCS session token in the KCS HOB.

  @param[in]  SessionToken  Session token to record, or NULL to clear it.

  @retval EFI_SUCCESS       The session token was recorded.
  @retval EFI_DEVICE_ERROR  The KCS HOB is unavailable.
**/
EFI_STATUS
EFIAPI
KcsSetSessionToken (
  IN  MANAGEABILITY_TRANSPORT_KCS  *SessionToken
  )
{
  KCS_INFO_HOB  *KcsInfo;

  KcsInfo = GetKcsInfoHob ();
  if (KcsInfo == NULL) {
    return EFI_DEVICE_ERROR;
  }

  KcsInfo->SessionToken = SessionToken;

  return EFI_SUCCESS;
}
