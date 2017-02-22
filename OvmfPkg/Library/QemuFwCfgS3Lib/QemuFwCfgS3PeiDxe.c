/** @file
  Shared code for the PEI fw_cfg and DXE fw_cfg instances of the QemuFwCfgS3Lib
  class.

  Copyright (C) 2017, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgS3Lib.h>

/**
  Determine if S3 support is explicitly enabled.

  @retval  TRUE   If S3 support is explicitly enabled. Other functions in this
                  library may be called (subject to their individual
                  restrictions).

           FALSE  Otherwise. This includes unavailability of the firmware
                  configuration interface. No other function in this library
                  must be called.
**/
BOOLEAN
EFIAPI
QemuFwCfgS3Enabled (
  VOID
  )
{
  RETURN_STATUS        Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;
  UINT8                SystemStates[6];

  Status = QemuFwCfgFindFile ("etc/system-states", &FwCfgItem, &FwCfgSize);
  if (Status != RETURN_SUCCESS || FwCfgSize != sizeof SystemStates) {
    return FALSE;
  }
  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof SystemStates, SystemStates);
  return (BOOLEAN) (SystemStates[3] & BIT7);
}
