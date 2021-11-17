/** @file
  Set the level of support for Hardware Error Record Persistence that is
  implemented by the platform.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HwErrRecSupport.h"

/**
  Set the HwErrRecSupport variable contains a binary UINT16 that supplies the
  level of support for Hardware Error Record Persistence that is implemented
  by the platform.

**/
VOID
InitializeHwErrRecSupport (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      HardwareErrorRecordLevel;

  HardwareErrorRecordLevel = PcdGet16 (PcdHardwareErrorRecordLevel);

  if (HardwareErrorRecordLevel != 0) {
    //
    // If level value equal 0, no need set to 0 to variable area because UEFI specification
    // define same behavior between no value or 0 value for L"HwErrRecSupport".
    //
    Status = gRT->SetVariable (
                    L"HwErrRecSupport",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (UINT16),
                    &HardwareErrorRecordLevel
                    );
    ASSERT_EFI_ERROR (Status);
  }
}
