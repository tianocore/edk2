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
  ACPI_S3_ENABLE     *AcpiS3EnableHob;

  AcpiS3EnableHob = NULL;

  //
  // Get ACPI_S3_ENABLE_HOB for Standalone MM init.
  //
  GuidHob = GetFirstGuidHob (&gEdkiiAcpiS3EnableHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    AcpiS3EnableHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (AcpiS3EnableHob != NULL) {
    mAcpiS3Enable = AcpiS3EnableHob->AcpiS3Enable;
  }
}
