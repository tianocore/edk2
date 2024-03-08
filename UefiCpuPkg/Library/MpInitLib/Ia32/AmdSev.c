/** @file

  AMD SEV helper function.

  Copyright (c) 2021 - 2024, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"

/**
  Create an SEV-SNP AP save area (VMSA) for use in running the vCPU.

  @param[in]  CpuMpData        Pointer to CPU MP Data
  @param[in]  CpuData          Pointer to CPU AP Data
  @param[in]  ApicId           APIC ID of the vCPU
**/
VOID
SevSnpCreateSaveArea (
  IN CPU_MP_DATA  *CpuMpData,
  IN CPU_AP_DATA  *CpuData,
  UINT32          ApicId
  )
{
  //
  // SEV-SNP is not support on 32-bit build.
  //
  ASSERT (FALSE);
}

/**
  Create SEV-SNP APs.

  @param[in]  CpuMpData        Pointer to CPU MP Data
  @param[in]  ProcessorNumber  The handle number of specified processor
                               (-1 for all APs)
**/
VOID
SevSnpCreateAP (
  IN CPU_MP_DATA  *CpuMpData,
  IN INTN         ProcessorNumber
  )
{
  //
  // SEV-SNP is not support on 32-bit build.
  //
  ASSERT (FALSE);
}

/**
  Determine if the SEV-SNP AP Create protocol should be used.

  @param[in]  CpuMpData  Pointer to CPU MP Data

  @retval     TRUE       Use SEV-SNP AP Create protocol
  @retval     FALSE      Do not use SEV-SNP AP Create protocol
**/
BOOLEAN
CanUseSevSnpCreateAP (
  IN  CPU_MP_DATA  *CpuMpData
  )
{
  //
  // SEV-SNP is not supported on 32-bit build.
  //
  return FALSE;
}
