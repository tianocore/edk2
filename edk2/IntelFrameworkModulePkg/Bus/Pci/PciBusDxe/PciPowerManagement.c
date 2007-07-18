/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PciPowerManagement.c

Abstract:

  PCI Bus Driver

Revision History

--*/

#include "pcibus.h"

EFI_STATUS
ResetPowerManagementFeature (
  IN PCI_IO_DEVICE *PciIoDevice
  )
/*++

Routine Description:

  This function is intended to turn off PWE assertion and
  put the device to D0 state if the device supports
  PCI Power Management.

Arguments:

Returns:
  
  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS  Status;
  UINT8       PowerManagementRegBlock;
  UINT16      PMCSR;

  PowerManagementRegBlock = 0;

  Status = LocateCapabilityRegBlock (
            PciIoDevice,
            EFI_PCI_CAPABILITY_ID_PMI,
            &PowerManagementRegBlock,
            NULL
            );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Turn off the PWE assertion and put the device into D0 State
  //
  PMCSR = 0x8000;

  //
  // Write PMCSR
  //
  PciIoWrite (
               &PciIoDevice->PciIo,
               EfiPciIoWidthUint16,
               PowerManagementRegBlock + 4,
               1,
               &PMCSR
             );

  return EFI_SUCCESS;
}
