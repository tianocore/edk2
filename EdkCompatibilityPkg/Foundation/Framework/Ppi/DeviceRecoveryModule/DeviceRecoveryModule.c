/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  DeviceRecoveryModule.c

Abstract:

  Device Recovery Module PPI GUID as defined in PEI EAS

--*/

#include "Tiano.h"
#include "PeiBind.h"
#include "PeiApi.h"
#include EFI_PPI_DEFINITION (DeviceRecoveryModule)

EFI_GUID  gPeiDeviceRecoveryModulePpiGuid = PEI_DEVICE_RECOVERY_MODULE_INTERFACE_PPI;

EFI_GUID_STRING(&gPeiDeviceRecoveryModulePpiGuid, "DeviceRecoveryModule", "Device Recovery Module PPI");
