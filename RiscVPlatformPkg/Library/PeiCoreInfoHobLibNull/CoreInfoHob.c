/** @file
  Build up platform processor information.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

#include <ProcessorSpecificHobData.h>


/**
  Build processor and platform information for a generic RISC-V

  @return EFI_SUCCESS     Status.

**/
EFI_STATUS
BuildRiscVSmbiosHobs (
  VOID
)
{
  return EFI_SUCCESS;
}
