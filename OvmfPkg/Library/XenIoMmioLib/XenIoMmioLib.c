/** @file
*  Manage XenBus device path and I/O handles
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/XenIoMmioLib.h>

#include <Protocol/XenIo.h>
#include <Guid/XenBusRootDevice.h>

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH          Vendor;
  EFI_PHYSICAL_ADDRESS        GrantTableAddress;
  EFI_DEVICE_PATH_PROTOCOL    End;
} XENBUS_ROOT_DEVICE_PATH;
#pragma pack ()

STATIC CONST XENBUS_ROOT_DEVICE_PATH  mXenBusRootDevicePathTemplate = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH) + sizeof (EFI_PHYSICAL_ADDRESS), 0 }
    },
    XENBUS_ROOT_DEVICE_GUID,
  },
  0,
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL),                           0 }
  }
};

/**

  Install the XENBUS_ROOT_DEVICE_PATH and XENIO_PROTOCOL protocols on
  the handle pointed to by @Handle, or on a new handle if it points to
  NULL

  @param  Handle                Pointer to the handle to install the protocols
                                on, may point to a NULL handle.

  @param  GrantTableAddress     The address of the Xen grant table

  @retval EFI_SUCCESS           Protocols were installed successfully

  @retval EFI_OUT_OF_RESOURCES  The function failed to allocate memory required
                                by the XenIo MMIO and device path protocols

  @return                       Status code returned by the boot service
                                InstallMultipleProtocolInterfaces ()

**/
EFI_STATUS
XenIoMmioInstall (
  IN OUT   EFI_HANDLE            *Handle,
  IN       EFI_PHYSICAL_ADDRESS  GrantTableAddress
  )
{
  EFI_STATUS               Status;
  XENIO_PROTOCOL           *XenIo;
  XENBUS_ROOT_DEVICE_PATH  *XenBusDevicePath;
  EFI_HANDLE               OutHandle;

  ASSERT (Handle != NULL);

  OutHandle = *Handle;

  XenIo = AllocateZeroPool (sizeof *XenIo);
  if (!XenIo) {
    return EFI_OUT_OF_RESOURCES;
  }

  XenIo->GrantTableAddress = GrantTableAddress;

  XenBusDevicePath = AllocateCopyPool (
                       sizeof *XenBusDevicePath,
                       &mXenBusRootDevicePathTemplate
                       );
  if (!XenBusDevicePath) {
    DEBUG ((DEBUG_ERROR, "%a: Out of memory\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeXenIo;
  }

  XenBusDevicePath->GrantTableAddress = GrantTableAddress;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &OutHandle,
                  &gEfiDevicePathProtocolGuid,
                  XenBusDevicePath,
                  &gXenIoProtocolGuid,
                  XenIo,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    *Handle = OutHandle;
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: Failed to install the EFI_DEVICE_PATH and "
    "XENIO_PROTOCOL protocols on handle %p (Status == %r)\n",
    __FUNCTION__,
    OutHandle,
    Status
    ));

  FreePool (XenBusDevicePath);

FreeXenIo:
  FreePool (XenIo);
  return Status;
}

/**

  Uninstall the XENBUS_ROOT_DEVICE_PATH and XENIO_PROTOCOL protocols

  @param  Handle          Handle onto which the protocols have been installed
                          earlier by XenIoMmioInstall ()

  @retval EFI_SUCCESS     Protocols were uninstalled successfully

  @return                 Status code returned by the boot service
                          UninstallMultipleProtocolInterfaces ()

**/
EFI_STATUS
XenIoMmioUninstall (
  IN       EFI_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  VOID        *XenIo;
  VOID        *XenBusDevicePath;

  XenBusDevicePath = NULL;
  gBS->OpenProtocol (
         Handle,
         &gEfiDevicePathProtocolGuid,
         &XenBusDevicePath,
         NULL,
         NULL,
         EFI_OPEN_PROTOCOL_GET_PROTOCOL
         );

  XenIo = NULL;
  gBS->OpenProtocol (
         Handle,
         &gXenIoProtocolGuid,
         &XenIo,
         NULL,
         NULL,
         EFI_OPEN_PROTOCOL_GET_PROTOCOL
         );

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  XenBusDevicePath,
                  &gXenIoProtocolGuid,
                  XenIo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  FreePool (XenBusDevicePath);
  FreePool (XenIo);

  return EFI_SUCCESS;
}
