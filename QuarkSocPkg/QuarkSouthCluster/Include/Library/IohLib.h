/** @file
Library that provides Soc specific library services for SouthCluster devices.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __IOH_LIB_H__
#define __IOH_LIB_H__

#include "Ioh.h"

EFI_STATUS
EFIAPI
InitializeIohSsvidSsid (
   IN UINT8   Bus,
   IN UINT8   Device,
   IN UINT8   Func
   );

VOID
EFIAPI
EnableUsbMemIoBusMaster (
   IN UINT8   UsbBusNumber
  );

UINT32
EFIAPI
ReadIohGpioValues (
  VOID
  );

#endif

