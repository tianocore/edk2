/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/DevicePathFromText.h>

#include "LinuxLoader.h"

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS            The entry point was executed successfully.
  @retval  EFI_NOT_FOUND          Protocol not found.
  @retval  EFI_NOT_FOUND          Path to the Linux kernel not found.
  @retval  EFI_ABORTED            The initialisation of the Shell Library failed.
  @retval  EFI_INVALID_PARAMETER  At least one parameter is not valid or there is a
                                  conflict between two parameters.
  @retval  EFI_OUT_OF_RESOURCES   A memory allocation failed.

**/
EFI_STATUS
EFIAPI
LinuxLoaderEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;
  EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters;
  CHAR16                              *KernelPath;
  CHAR16                              *FdtPath;
  CHAR16                              *InitrdPath;
  CHAR16                              *KernelTextDevicePath;
  CHAR16                              *FdtTextDevicePath;
  CHAR16                              *InitrdTextDevicePath;
  CHAR16                              *LinuxCommandLine;
  UINTN                               AtagMachineType;
  EFI_DEVICE_PATH                     *KernelDevicePath;
  EFI_DEVICE_PATH                     *FdtDevicePath;
  EFI_DEVICE_PATH                     *InitrdDevicePath;
  CHAR8                               *AsciiLinuxCommandLine;
  LIST_ENTRY                          ResourceList;
  LIST_ENTRY                          *ResourceLink;
  SYSTEM_MEMORY_RESOURCE              *Resource;
  EFI_PHYSICAL_ADDRESS                SystemMemoryBase;

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathFromTextProtocolGuid,
                  NULL,
                  (VOID **)&EfiDevicePathFromTextProtocol
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Register the strings for the user interface in the HII Database.
  // This shows the way to the multi-language support, even if
  // only the English language is actually supported. The strings to register
  // are stored in the "LinuxLoaderStrings[]" array. This array is
  // built by the building process from the "*.uni" file associated to
  // the present application (cf. LinuxLoader.inf). Examine the Build
  // folder of the application and you will find the array defined in the
  // LinuxLoaderStrDefs.h file.
  //
  mLinuxLoaderHiiHandle = HiiAddPackages (
                             &mLinuxLoaderHiiGuid,
                             ImageHandle,
                             LinuxLoaderStrings,
                             NULL
                             );
  if (mLinuxLoaderHiiHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID**)&ShellParameters
                  );

  KernelDevicePath      = NULL;
  FdtDevicePath         = NULL;
  InitrdDevicePath      = NULL;
  AsciiLinuxCommandLine = NULL;

  //
  // Call the proper function to handle the command line
  // depending on whether the application has been called
  // from the Shell or not.
  //

  if (!EFI_ERROR (Status)) {
    KernelTextDevicePath = NULL;
    FdtTextDevicePath    = NULL;
    InitrdTextDevicePath = NULL;

    Status = ProcessShellParameters (
               &KernelPath, &FdtPath, &InitrdPath, &LinuxCommandLine, &AtagMachineType
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    KernelDevicePath = gEfiShellProtocol->GetDevicePathFromFilePath (KernelPath);
    if (KernelDevicePath != NULL) {
      FreePool (KernelPath);
    } else {
      KernelTextDevicePath = KernelPath;
    }

    if (FdtPath != NULL) {
      FdtDevicePath = gEfiShellProtocol->GetDevicePathFromFilePath (FdtPath);
      if (FdtDevicePath != NULL) {
        FreePool (FdtPath);
      } else {
        FdtTextDevicePath = FdtPath;
      }
    }

    if (InitrdPath != NULL) {
      InitrdDevicePath = gEfiShellProtocol->GetDevicePathFromFilePath (InitrdPath);
      if (InitrdDevicePath != NULL) {
        FreePool (InitrdPath);
      } else {
        InitrdTextDevicePath = InitrdPath;
      }
    }

  } else {
    Status = ProcessAppCommandLine (
               &KernelTextDevicePath, &FdtTextDevicePath,
               &InitrdTextDevicePath, &LinuxCommandLine, &AtagMachineType
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }
  }

  Status = EFI_INVALID_PARAMETER;
  if (KernelTextDevicePath != NULL) {
    KernelDevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (
                                                        KernelTextDevicePath
                                                        );
    if (KernelDevicePath == NULL) {
      goto Error;
    }
  }
  if (FdtTextDevicePath != NULL) {
    FdtDevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (
                                                     FdtTextDevicePath
                                                     );
    if (FdtDevicePath == NULL) {
      goto Error;
    }
  }
  if (InitrdTextDevicePath != NULL) {
    InitrdDevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (
                                                        InitrdTextDevicePath
                                                        );
    if (InitrdDevicePath == NULL) {
      goto Error;
    }
  }

  if (LinuxCommandLine != NULL) {
    AsciiLinuxCommandLine = AllocatePool ((StrLen (LinuxCommandLine) + 1) * sizeof (CHAR8));
    if (AsciiLinuxCommandLine == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }
    UnicodeStrToAsciiStr (LinuxCommandLine, AsciiLinuxCommandLine);
  }

  //
  // Find Base of System Memory - we keep the lowest physical address
  //
  SystemMemoryBase = ~0;
  GetSystemMemoryResources (&ResourceList);
  ResourceLink = ResourceList.ForwardLink;
  while (ResourceLink != NULL && ResourceLink != &ResourceList) {
    Resource = (SYSTEM_MEMORY_RESOURCE*)ResourceLink;
    if (Resource->PhysicalStart < SystemMemoryBase) {
      SystemMemoryBase = Resource->PhysicalStart;
    }
    ResourceLink = ResourceLink->ForwardLink;
  }

  if (AtagMachineType != ARM_FDT_MACHINE_TYPE) {
    Status = BootLinuxAtag (SystemMemoryBase, KernelDevicePath, InitrdDevicePath, AsciiLinuxCommandLine, AtagMachineType);
  } else {
    Status = BootLinuxFdt (SystemMemoryBase, KernelDevicePath, InitrdDevicePath, FdtDevicePath, AsciiLinuxCommandLine);
  }

Error:
  if (KernelTextDevicePath != NULL) {
    FreePool (KernelTextDevicePath);
  }
  if (FdtTextDevicePath != NULL) {
    FreePool (FdtTextDevicePath);
  }
  if (InitrdTextDevicePath != NULL) {
    FreePool (InitrdTextDevicePath);
  }
  if (LinuxCommandLine != NULL) {
    FreePool (LinuxCommandLine);
  }
  if (KernelDevicePath != NULL) {
    FreePool (KernelDevicePath);
  }
  if (FdtDevicePath != NULL) {
    FreePool (FdtDevicePath);
  }
  if (InitrdDevicePath != NULL) {
    FreePool (InitrdDevicePath);
  }
  if (AsciiLinuxCommandLine != NULL) {
    FreePool (AsciiLinuxCommandLine);
  }

  if (EFI_ERROR (Status)) {
    PrintHii (NULL, STRING_TOKEN (STR_ERROR), Status);
  }

  HiiRemovePackages (mLinuxLoaderHiiHandle);

  return Status;
}
