/** @file
  Function declaration and internal data for XenBusDxe.

  Copyright (C) 2014, Citrix Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_XENBUS_DXE_H__
#define __EFI_XENBUS_DXE_H__

#include <Uefi.h>

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


//
// Consumed Protocols
//
#include <Protocol/PciIo.h>


//
// Produced Protocols
//


//
// Driver Version
//
#define XENBUS_DXE_VERSION  0x00000010


//
// Protocol instances
//
extern EFI_DRIVER_BINDING_PROTOCOL  gXenBusDxeDriverBinding;
extern EFI_COMPONENT_NAME2_PROTOCOL  gXenBusDxeComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL  gXenBusDxeComponentName;


//
// Include files with function prototypes
//
#include "DriverBinding.h"
#include "ComponentName.h"

//
// Other stuff
//
#define PCI_VENDOR_ID_XEN                0x5853
#define PCI_DEVICE_ID_XEN_PLATFORM       0x0001


#endif
