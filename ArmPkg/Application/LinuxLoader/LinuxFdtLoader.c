/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "LinuxInternal.h"

#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  LINUX_LOADER_OPTIONAL_DATA*  LinuxOptionalData;
  EFI_DEVICE_PATH*             DevicePathKernel;
  EFI_DEVICE_PATH*             InitrdDevicePath;
  CHAR16*                      OptionalDataInitrd;
  CHAR8*                       OptionalDataArguments;
  CHAR16*                      Initrd;

  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  ASSERT_EFI_ERROR (Status);

  if (LoadedImage->LoadOptionsSize == 0) {
    Status = LinuxLoaderConfig (LoadedImage);
  } else {
    // Ensure the signature is correct
    LinuxOptionalData = (LINUX_LOADER_OPTIONAL_DATA*)LoadedImage->LoadOptions;
    if (LinuxOptionalData->Signature != LINUX_LOADER_SIGNATURE) {
      return EFI_UNSUPPORTED;
    }

    // Generate the File Path Node for the Linux Kernel
    DevicePathKernel = FileDevicePath (LoadedImage->DeviceHandle, LINUX_KERNEL_NAME);

    if (LinuxOptionalData->CmdLineLength > 0) {
      OptionalDataArguments = (CHAR8*)LinuxOptionalData + sizeof(LINUX_LOADER_OPTIONAL_DATA);
    } else {
      OptionalDataArguments = NULL;
    }

    if (LinuxOptionalData->InitrdPathListLength > 0) {
      if (OptionalDataArguments != NULL) {
        OptionalDataInitrd = (CHAR16*)(OptionalDataArguments + LinuxOptionalData->CmdLineLength);
      } else {
        OptionalDataInitrd = (CHAR16*)LinuxOptionalData + sizeof(LINUX_LOADER_OPTIONAL_DATA);
      }

      // If pointer not aligned
      if ((UINTN)OptionalDataInitrd & 0x1) {
        Initrd = (CHAR16*)AllocateCopyPool (LinuxOptionalData->InitrdPathListLength, OptionalDataInitrd);
      } else {
        Initrd = OptionalDataInitrd;
      }

      InitrdDevicePath = FileDevicePath (LoadedImage->DeviceHandle, Initrd);
    } else {
      OptionalDataInitrd = NULL;
      InitrdDevicePath   = NULL;
      Initrd             = NULL;
    }

    // Load and Start the Linux Kernel (we should never return)
    Status = BdsBootLinuxFdt (DevicePathKernel, InitrdDevicePath, OptionalDataArguments);

    if ((UINTN)OptionalDataInitrd & 0x1) {
      FreePool (Initrd);
    }

    FreePool (DevicePathKernel);
    if (InitrdDevicePath) {
      FreePool (InitrdDevicePath);
    }
  }

  return Status;
}
