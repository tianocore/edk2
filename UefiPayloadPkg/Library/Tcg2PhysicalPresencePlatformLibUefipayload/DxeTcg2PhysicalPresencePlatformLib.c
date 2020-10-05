/** @file
  Returns the platform specific configuration for the QEMU PPI.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

Copyright (C) 2018, Red Hat, Inc.
Copyright (c) 2018, IBM Corporation. All rights reserved.<BR>
Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/QemuTpm.h>

#include <Library/Tcg2PhysicalPresencePlatformLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>

#include <Guid/TcgPhysicalPresenceGuid.h>

/**
  Reads QEMU PPI config from TcgPhysicalPresenceInfoHobGuid.

  @param[out]  The Config structure to read to.
  @param[out]  The PPIinMMIO is True when the PPI is in MMIO memory space

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_PROTOCOL_ERROR    Invalid HOB entry.
**/
EFI_STATUS
TpmPPIPlatformReadConfig (
  OUT QEMU_FWCFG_TPM_CONFIG *Config,
  OUT BOOLEAN               *PPIinMMIO
  )
{
  EFI_HOB_GUID_TYPE                       *GuidHob;
  TCG_PHYSICAL_PRESENCE_INFO              *pPPInfo;

  //
  // Find the TPM Physical Presence HOB
  //
  GuidHob = GetFirstGuidHob (&gEfiTcgPhysicalPresenceInfoHobGuid);

  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  pPPInfo = (TCG_PHYSICAL_PRESENCE_INFO *)GET_GUID_HOB_DATA (GuidHob);

  if (pPPInfo->PpiAddress == 0 || pPPInfo->PpiAddress == ~0) {
    return EFI_NOT_FOUND;
  } else {
    Config->PpiAddress = pPPInfo->PpiAddress;
  }

  if (pPPInfo->TpmVersion == UEFIPAYLOAD_TPM_VERSION_1_2) {
    Config->TpmVersion = QEMU_TPM_VERSION_1_2;
  } else if (pPPInfo->TpmVersion == UEFIPAYLOAD_TPM_VERSION_2) {
    Config->TpmVersion = QEMU_TPM_VERSION_2;
  } else {
    return EFI_UNSUPPORTED;
  }

  if (pPPInfo->PpiVersion == UEFIPAYLOAD_TPM_PPI_VERSION_NONE) {
    Config->PpiVersion = QEMU_TPM_PPI_VERSION_NONE;
  } else if (pPPInfo->PpiVersion == UEFIPAYLOAD_TPM_PPI_VERSION_1_30) {
    Config->PpiVersion = QEMU_TPM_PPI_VERSION_1_30;
  } else {
    return EFI_UNSUPPORTED;
  }

  *PPIinMMIO = FALSE;

  return EFI_SUCCESS;
}
