/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtThunk.c

Abstract:

  Produce WinNtThunk protocol and it's associated device path and controller 
  state protocols. WinNtThunk is to the NT emulation environment as 
  PCI_ROOT_BRIGE is to real hardware. The WinNtBusDriver is the child of this
  driver.

  Since we are a root hardware abstraction we do not install a Driver Binding
  protocol on this handle. This driver can only support one one WinNtThunk protocol
  in the system, since the device path is hard coded.

**/

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <WinNtDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/WinNtThunk.h>
#include <Protocol/DevicePath.h>
//
// The Library classes this module consumes
//
#include <Library/UefiDriverEntryPoint.h>
#include <Library/WinNtLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include "WinNtThunk.h"

//
// WinNtThunk Device Path Protocol Instance
//
WIN_NT_THUNK_DEVICE_PATH mWinNtThunkDevicePath = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    EFI_WIN_NT_THUNK_PROTOCOL_GUID,
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    END_DEVICE_PATH_LENGTH,
    0
  }
};


EFI_STATUS
EFIAPI
InitializeWinNtThunk (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:
  Install WinNtThunk Protocol and it's associated Device Path protocol

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_SUCEESS - WinNtThunk protocol is added or error status from 
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
                  &gEfiWinNtThunkProtocolGuid,
                  gWinNt,
                  &gEfiDevicePathProtocolGuid,
                  &mWinNtThunkDevicePath,
                  NULL
                  );

  return Status;
}
