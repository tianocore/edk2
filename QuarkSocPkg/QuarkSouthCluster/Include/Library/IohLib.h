/** @file
Library that provides Soc specific library services for SouthCluster devices.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

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

