/** @file
Agent Module to load other modules to deploy MM Entry Vector for X86 CPU.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

/**
  Get ACPI S3 enable flag.

**/
VOID
GetAcpiS3EnableFlag (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  MM_ACPI_S3_ENABLE  *MmAcpiS3EnableHob;

  MmAcpiS3EnableHob = NULL;

  //
  // Get MM_ACPI_S3_ENABLE for Standalone MM init.
  //
  GuidHob = GetFirstGuidHob (&gMmAcpiS3EnableHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    MmAcpiS3EnableHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (MmAcpiS3EnableHob != NULL) {
    mAcpiS3Enable = MmAcpiS3EnableHob->AcpiS3Enable;
  }
}
