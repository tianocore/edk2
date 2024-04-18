/** @file
  This PEIM creates SMM_CPU_FEATURE_INFO_HOB.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_CPI_FEATURE_INFO_PEI_H_
#define _SMM_CPI_FEATURE_INFO_PEI_H_

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Guid/SmmCpuFeatureInfo.h>
#include <Guid/SmmProfileDataHob.h>

#include <Register/Intel/Cpuid.h>
#include <Register/Intel/ArchitecturalMsr.h>

#endif
