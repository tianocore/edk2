/*++ @file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


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
