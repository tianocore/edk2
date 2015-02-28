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
#include <Protocol/XenIo.h>


//
// Produced Protocols
//
#include <Protocol/XenBus.h>


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
#include <IndustryStandard/Xen/xen.h>

typedef struct _XENBUS_DEVICE_PATH XENBUS_DEVICE_PATH;
typedef struct _XENBUS_DEVICE XENBUS_DEVICE;

// Have the state of the driver.
#define XENBUS_DEVICE_SIGNATURE SIGNATURE_32 ('X','B','s','t')
struct _XENBUS_DEVICE {
  UINT32                        Signature;
  EFI_DRIVER_BINDING_PROTOCOL   *This;
  EFI_HANDLE                    ControllerHandle;
  XENIO_PROTOCOL                *XenIo;
  EFI_EVENT                     ExitBootEvent;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  LIST_ENTRY                    ChildList;

  shared_info_t                 *SharedInfo;
};

// There is one of this struct allocated for every child.
#define XENBUS_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('X', 'B', 'p', 'd')
typedef struct {
    UINTN Signature;
    LIST_ENTRY Link;
    EFI_HANDLE Handle;
    XENBUS_PROTOCOL XenBusIo;
    XENBUS_DEVICE *Dev;
    XENBUS_DEVICE_PATH *DevicePath;
} XENBUS_PRIVATE_DATA;

#define XENBUS_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, XENBUS_PRIVATE_DATA, XenBusIo, XENBUS_PRIVATE_DATA_SIGNATURE)
#define XENBUS_PRIVATE_DATA_FROM_LINK(a) \
  CR (a, XENBUS_PRIVATE_DATA, Link, XENBUS_PRIVATE_DATA_SIGNATURE)

/*
 * Helpers
 */

/**
  Atomically test and clear a bit.

  @param Bit      Bit index to test in *Address
  @param Address  The Address to the buffer that contain the bit to test.

  @return Value of the Bit before it was cleared.
**/
INT32
EFIAPI
TestAndClearBit (
  IN INT32 Bit,
  IN VOID  *Address
  );

CHAR8*
AsciiStrDup (
  IN CONST CHAR8* Str
  );

#endif
