/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixThunk.c

Abstract:

  Produce UnixThunk protocol and it's associated device path and controller 
  state protocols. UnixThunk is to the emulation environment as 
  PCI_ROOT_BRIGE is to real hardware. The UnixBusDriver is the child of this
  driver.

  Since we are a root hardware abstraction we do not install a Driver Binding
  protocol on this handle. This driver can only support one one UnixThunk protocol
  in the system, since the device path is hard coded.

--*/
#include "PiDxe.h"
#include "UnixDxe.h"
#include "UnixThunk.h"
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UnixLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

//
// WinNtThunk Device Path Protocol Instance
//
UNIX_THUNK_DEVICE_PATH mUnixThunkDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_UNIX_THUNK_PROTOCOL_GUID,
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
InitializeUnixThunk (
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

--*/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS  Status;
  EFI_HANDLE  ControllerHandle;

  ControllerHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiUnixThunkProtocolGuid,
                  gUnix,
                  &gEfiDevicePathProtocolGuid,
                  &mUnixThunkDevicePath,
                  NULL
                  );

  return Status;
}
