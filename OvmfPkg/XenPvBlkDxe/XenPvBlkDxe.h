/** @file
  Main header for XenPvBlkDxe

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_XEN_PV_BLK_DXE_H__
#define __EFI_XEN_PV_BLK_DXE_H__

#include <Uefi.h>

#define xen_mb()   MemoryFence()
#define xen_rmb()  MemoryFence()
#define xen_wmb()  MemoryFence()

//
// Libraries
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>

//
// UEFI Driver Model Protocols
//
#include <Protocol/DriverBinding.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/ComponentName.h>

//
// Consumed Protocols
//
#include <Protocol/XenBus.h>

//
// Produced Protocols
//
#include <Protocol/BlockIo.h>

//
// Driver Version
//
#define XEN_PV_BLK_DXE_VERSION  0x00000010

//
// Protocol instances
//
extern EFI_DRIVER_BINDING_PROTOCOL   gXenPvBlkDxeDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL  gXenPvBlkDxeComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL   gXenPvBlkDxeComponentName;

//
// Include files with function prototypes
//
#include "DriverBinding.h"
#include "ComponentName.h"
#include "BlockIo.h"

#endif
