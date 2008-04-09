/** @file
  HII Library implementation that uses DXE protocols and services.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiDxe.h>

#include <Protocol/DevicePath.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <MdeModuleHii.h>


//
// Hii vendor device path template
//
HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePathTemplate = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (HII_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (HII_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EFI_IFR_TIANO_GUID
    },
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      END_DEVICE_PATH_LENGTH
    }
  }
};


EFI_STATUS
EFIAPI
HiiLibCreateHiiDriverHandle (
  OUT EFI_HANDLE               *DriverHandle
  )
{
  EFI_STATUS                   Status;
  HII_VENDOR_DEVICE_PATH_NODE  *VendorDevicePath;
  UINT64                       MonotonicCount;

  VendorDevicePath = AllocateCopyPool (sizeof (HII_VENDOR_DEVICE_PATH), &mHiiVendorDevicePathTemplate);
  if (VendorDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  gBS->GetNextMonotonicCount (&MonotonicCount);
  VendorDevicePath->MonotonicCount = (UINT32) MonotonicCount;

  *DriverHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  VendorDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


VOID
EFIAPI
HiiLibDestroyHiiDriverHandle (
  IN EFI_HANDLE               DriverHandle
  )
{
  EFI_STATUS                   Status;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;

  Status = gBS->HandleProtocol (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->UninstallProtocolInterface (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  DevicePath
                  );
  ASSERT_EFI_ERROR (Status);

  FreePool (DevicePath);

}



