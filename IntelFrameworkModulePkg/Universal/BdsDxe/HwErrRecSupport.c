/** @file
  Set the level of support for Hardware Error Record Persistence that is
  implemented by the platform.

Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  EFI_STATUS    Status;
  UINT16        HardwareErrorRecordLevel;
  
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
    ASSERT_EFI_ERROR(Status);
  }
}
