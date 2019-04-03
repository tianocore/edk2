/*++ @file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#include <PiDxe.h>

#include <Protocol/DevicePath.h>
#include <Protocol/EmuThunk.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/EmuThunkLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

//
// EmuThunk Device Path Protocol Instance
//
EMU_THUNK_DEVICE_PATH mEmuThunkDevicePath = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EMU_THUNK_PROTOCOL_GUID
    },
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};


EFI_STATUS
EFIAPI
InitializeEmuThunk (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:
  Install UnixThunk Protocol and it's associated Device Path protocol

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_SUCEESS - UnixThunk protocol is added or error status from
                gBS->InstallMultiProtocolInterfaces().

**/
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEmuThunkProtocolGuid,       gEmuThunk,
                  &gEfiDevicePathProtocolGuid,  &mEmuThunkDevicePath,
                  NULL
                  );

  return Status;
}
