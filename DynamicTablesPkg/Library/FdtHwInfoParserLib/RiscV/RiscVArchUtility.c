/** @file
  RISC-V Arch utility functions.

  Copyright (c) 2026, Ilya Somkin. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "ArchUtility.h"
#include <Library/DebugLib.h>

/**
  Return the token of the CPU interface object describing a CPU.

  The CPU interface object (e.g. GICC or RINTC) is referenced by the processor
  hierarchy node of the CPU.

  @param[in] CpuIndex  Index of the CPU device node below the "/cpus" node.

  @return The token of the CPU interface object.
**/
CM_OBJECT_TOKEN
GetCpuToken (
  IN UINT32  CpuIndex
  )
{
  ASSERT ((CpuIndex & ~CM_ABSTRACT_TOKEN_ID_MASK) == 0);
  return (CM_OBJECT_TOKEN)CM_ABSTRACT_TOKEN_MAKE (
                            ETokenNameSpaceFdtHwInfo,
                            EFdtHwInfoRintcObject,
                            CpuIndex
                            );
}
