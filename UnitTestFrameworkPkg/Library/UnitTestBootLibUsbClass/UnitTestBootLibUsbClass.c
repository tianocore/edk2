/**
  Implement UnitTestBootLib using USB Class Boot option.  This should be
  industry standard and should work on all platforms

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/DevicePath.h>
#include <Library/MemoryAllocationLib.h>

/**
  Set the boot manager to boot from a specific device on the next boot. This
  should be set only for the next boot and shouldn't require any manual clean up

  @retval EFI_SUCCESS      Boot device for next boot was set.
  @retval EFI_UNSUPPORTED  Setting the boot device for the next boot is not
                           supportted.
  @retval Other            Boot device for next boot can not be set.
**/
EFI_STATUS
EFIAPI
SetBootNextDevice (
   VOID
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  NewOption;
  UINT32                        Attributes;
  UINT8                         *OptionalData;
  UINT32                        OptionalDataSize;
  UINT16                        BootNextValue;
  USB_CLASS_DEVICE_PATH         UsbDp;
  EFI_DEVICE_PATH_PROTOCOL      *DpEnd;
  EFI_DEVICE_PATH_PROTOCOL      *Dp;
  BOOLEAN                       NewOptionValid;

  OptionalData     = NULL;
  OptionalDataSize = 0;
  BootNextValue    = 0xABCD;  // this should be a safe number...
  DpEnd            = NULL;
  Dp               = NULL;
  NewOptionValid   = FALSE;

  UsbDp.Header.Length[0] = (UINT8)(sizeof(USB_CLASS_DEVICE_PATH) & 0xff);
  UsbDp.Header.Length[1] = (UINT8)(sizeof(USB_CLASS_DEVICE_PATH) >> 8);
  UsbDp.Header.Type      = MESSAGING_DEVICE_PATH;
  UsbDp.Header.SubType   = MSG_USB_CLASS_DP;
  UsbDp.VendorId         = 0xFFFF;
  UsbDp.ProductId        = 0xFFFF;
  UsbDp.DeviceClass      = 0xFF;
  UsbDp.DeviceSubClass   = 0xFF;
  UsbDp.DeviceProtocol   = 0xFF;

  Attributes = LOAD_OPTION_ACTIVE;

  DpEnd = AppendDevicePathNode (NULL, NULL);
  if (DpEnd == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Unable to create device path.  DpEnd is NULL.\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto CLEANUP;
  }

  //@MRT --- Is this memory leak because we lose the old Dp memory
  Dp = AppendDevicePathNode (
         DpEnd,
         (EFI_DEVICE_PATH_PROTOCOL *)&UsbDp
         );
  if (Dp == NULL) {
    DEBUG((DEBUG_ERROR, "%a: Unable to create device path.  Dp is NULL.\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto CLEANUP;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             (UINTN) BootNextValue,
             LoadOptionTypeBoot,
             Attributes,
             L"Generic USB Class Device",
             Dp,
             OptionalData,
             OptionalDataSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error creating load option.  Status = %r\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  NewOptionValid = TRUE;
  DEBUG ((DEBUG_VERBOSE, "%a: Generic USB Class Device boot option created.\n", __FUNCTION__));
  Status = EfiBootManagerLoadOptionToVariable (&NewOption);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error Saving boot option NV variable. Status = %r\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  //
  // Set Boot Next
  //
  Status = gRT->SetVariable (
                  L"BootNext",
                  &gEfiGlobalVariableGuid,
                  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE),
                  sizeof(BootNextValue),
                  &(BootNextValue)
                  );

  DEBUG((DEBUG_VERBOSE, "%a - Set BootNext Status (%r)\n", __FUNCTION__, Status));

CLEANUP:
  if (Dp != NULL) {
    FreePool (Dp);
  }
  if (DpEnd != NULL) {
    FreePool (DpEnd);
  }
  if (NewOptionValid) {
    EfiBootManagerFreeLoadOption (&NewOption);
  }
  return Status;
}
