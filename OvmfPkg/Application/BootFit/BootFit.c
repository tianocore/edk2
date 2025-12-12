/** @file
  BootFit - UEFI Application to boot a FIT Image.

  Copyright (c) 2024, UEFI. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellLib.h>
#include <Library/FdtLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/LoadFile2.h>
#include <Guid/LinuxEfiInitrdMedia.h>

//
// Vendor Media Device Path for Linux Initrd
//
#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH        VendorMedia;
  EFI_DEVICE_PATH_PROTOCOL  End;
} LINUX_INITRD_DEVICE_PATH;
#pragma pack()

LINUX_INITRD_DEVICE_PATH mLinuxInitrdDevicePath;

//
// Context for Initrd LoadFile2
//
typedef struct {
  VOID   *Data;
  UINTN  Size;
} INITRD_CONTEXT;

INITRD_CONTEXT mInitrdContext;

//
// LoadFile2 Callback
//
EFI_STATUS
EFIAPI
LoadFile2Callback (
  IN EFI_LOAD_FILE2_PROTOCOL    *This,
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN BOOLEAN                    BootPolicy,
  IN OUT UINTN                  *BufferSize,
  IN VOID                       *Buffer OPTIONAL
  )
{
  if (BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL || *BufferSize < mInitrdContext.Size) {
    *BufferSize = mInitrdContext.Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Buffer, mInitrdContext.Data, mInitrdContext.Size);
  *BufferSize = mInitrdContext.Size;

  return EFI_SUCCESS;
}

EFI_LOAD_FILE2_PROTOCOL mLoadFile2 = {
  LoadFile2Callback
};

//
// Helper to read file content
//
EFI_STATUS
ReadFileContent (
  IN CHAR16   *FileName,
  OUT VOID    **Buffer,
  OUT UINTN   *Size
  )
{
  EFI_STATUS         Status;
  SHELL_FILE_HANDLE  FileHandle;
  UINT64             FileSize;
  VOID               *FileBuffer;

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ShellGetFileSize (FileHandle, &FileSize);
  if (EFI_ERROR (Status)) {
    ShellCloseFile (&FileHandle);
    return Status;
  }

  FileBuffer = AllocatePool ((UINTN)FileSize);
  if (FileBuffer == NULL) {
    ShellCloseFile (&FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = ShellReadFile (FileHandle, (UINTN *)&FileSize, FileBuffer);
  ShellCloseFile (&FileHandle);

  if (EFI_ERROR (Status)) {
    FreePool (FileBuffer);
    return Status;
  }

  *Buffer = FileBuffer;
  *Size   = (UINTN)FileSize;

  return EFI_SUCCESS;
}

//
// Main Entry Point
//
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  EFI_STATUS                 Status;
  VOID                       *FitBuffer;
  UINTN                      FitSize;
  INT32                      ConfigNode;
  INT32                      ImagesNode;
  INT32                      KernelNode;
  INT32                      RamdiskNode;
  CONST CHAR8                *KernelName;
  CONST CHAR8                *RamdiskName;
  CONST VOID                 *KernelData;
  INT32                      KernelLen;
  CONST VOID                 *RamdiskData;
  INT32                      RamdiskLen;
  CONST CHAR8                *BootArgs;
  INT32                      BootArgsLen;
  EFI_HANDLE                 KernelImageHandle;
  EFI_HANDLE                 InitrdHandle;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     *LoadOptions;
  UINTN                      LoadOptionsSize;

  if (Argc < 2) {
    Print (L"Usage: BootFit <Image.fit>\n");
    return 1;
  }

  //
  // 1. Load FIT Image
  //
  Status = ReadFileContent (Argv[1], &FitBuffer, &FitSize);
  if (EFI_ERROR (Status)) {
    Print (L"Error reading file %s: %r\n", Argv[1], Status);
    return 1;
  }

  if (FdtCheckHeader (FitBuffer) != 0) {
    Print (L"Invalid FIT header\n");
    FreePool (FitBuffer);
    return 1;
  }

  //
  // 2. Parse FIT
  //

  // Find /configurations node
  INT32 ConfigurationsNode = FdtSubnodeOffset (FitBuffer, 0, "configurations");
  if (ConfigurationsNode < 0) {
    Print (L"No configurations node found\n");
    FreePool (FitBuffer);
    return 1;
  }

  // Find default config
  INT32 PropLen;
  CONST VOID *Prop = FdtGetProp (FitBuffer, ConfigurationsNode, "default", &PropLen);
  if (Prop == NULL) {
    Print (L"No default configuration found\n");
    FreePool (FitBuffer);
    return 1;
  }

  // Find the config node
  ConfigNode = FdtSubnodeOffset (FitBuffer, ConfigurationsNode, (CONST CHAR8*)Prop);
  if (ConfigNode < 0) {
    Print (L"Default configuration node '%a' not found\n", (CONST CHAR8*)Prop);
    FreePool (FitBuffer);
    return 1;
  }

  // Get Kernel Name
  KernelName = FdtGetProp (FitBuffer, ConfigNode, "kernel", &PropLen);
  if (KernelName == NULL) {
    Print (L"No kernel specified in configuration\n");
    FreePool (FitBuffer);
    return 1;
  }

  // Get Ramdisk Name (Optional)
  RamdiskName = FdtGetProp (FitBuffer, ConfigNode, "ramdisk", &PropLen);

  // Get Boot Args (Optional)
  // Check "bootargs" in config node first
  BootArgs = FdtGetProp (FitBuffer, ConfigNode, "bootargs", &BootArgsLen);
  // If not in config, check /chosen node? Usually FIT puts it in config or assumes FDT has it.
  // We will assume config overrides or provides it for EFI stub.

  // Find /images node
  ImagesNode = FdtSubnodeOffset (FitBuffer, 0, "images");
  if (ImagesNode < 0) {
    Print (L"No images node found\n");
    FreePool (FitBuffer);
    return 1;
  }

  // Find Kernel Node
  KernelNode = FdtSubnodeOffset (FitBuffer, ImagesNode, KernelName);
  if (KernelNode < 0) {
    Print (L"Kernel image node '%a' not found\n", KernelName);
    FreePool (FitBuffer);
    return 1;
  }

  // Get Kernel Data
  KernelData = FdtGetProp (FitBuffer, KernelNode, "data", &KernelLen);
  if (KernelData == NULL) {
    Print (L"Kernel image has no data\n");
    FreePool (FitBuffer);
    return 1;
  }

  //
  // 3. Load Kernel Image
  //
  Status = gBS->LoadImage (
                  FALSE,
                  gImageHandle,
                  NULL,
                  (VOID*)KernelData,
                  (UINTN)KernelLen,
                  &KernelImageHandle
                  );

  if (EFI_ERROR (Status)) {
    Print (L"Failed to load kernel image: %r\n", Status);
    FreePool (FitBuffer);
    return 1;
  }

  //
  // 4. Set Load Options (Command Line)
  //
  if (BootArgs != NULL && BootArgsLen > 0) {
    Status = gBS->HandleProtocol (
                    KernelImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (!EFI_ERROR (Status)) {
      // Convert Ascii to Unicode
      LoadOptionsSize = (BootArgsLen + 1) * sizeof (CHAR16);
      LoadOptions = AllocatePool (LoadOptionsSize);
      if (LoadOptions != NULL) {
        AsciiStrToUnicodeStrS (BootArgs, LoadOptions, BootArgsLen + 1);
        LoadedImage->LoadOptions = LoadOptions;
        LoadedImage->LoadOptionsSize = (UINT32)StrSize(LoadOptions);
      }
    }
  }

  //
  // 5. Setup Initrd
  //
  if (RamdiskName != NULL) {
    RamdiskNode = FdtSubnodeOffset (FitBuffer, ImagesNode, RamdiskName);
    if (RamdiskNode >= 0) {
      RamdiskData = FdtGetProp (FitBuffer, RamdiskNode, "data", &RamdiskLen);
      if (RamdiskData != NULL) {
        mInitrdContext.Data = (VOID*)RamdiskData;
        mInitrdContext.Size = (UINTN)RamdiskLen;

        mLinuxInitrdDevicePath.VendorMedia.Header.Type = MEDIA_DEVICE_PATH;
        mLinuxInitrdDevicePath.VendorMedia.Header.SubType = MEDIA_VENDOR_DP;
        SetDevicePathNodeLength (&mLinuxInitrdDevicePath.VendorMedia.Header, sizeof (VENDOR_DEVICE_PATH));
        CopyGuid (&mLinuxInitrdDevicePath.VendorMedia.Guid, &gLinuxEfiInitrdMediaGuid);
        SetDevicePathEndNode (&mLinuxInitrdDevicePath.End);

        InitrdHandle = NULL;
        Status = gBS->InstallMultipleProtocolInterfaces (
                        &InitrdHandle,
                        &gEfiDevicePathProtocolGuid,
                        &mLinuxInitrdDevicePath,
                        &gEfiLoadFile2ProtocolGuid,
                        &mLoadFile2,
                        NULL
                        );
        if (EFI_ERROR (Status)) {
          Print (L"Failed to install initrd protocol: %r\n", Status);
          // Proceed anyway?
        }
      }
    }
  }

  //
  // 6. Start Image
  //
  Print (L"Starting Kernel...\n");
  Status = gBS->StartImage (KernelImageHandle, NULL, NULL);

  if (EFI_ERROR (Status)) {
    Print (L"StartImage failed: %r\n", Status);
  }

  // Clean up
  // Note: If StartImage returns, the kernel failed or exited.
  // We can free FitBuffer now.
  // If StartImage succeeds (transfers control), we usually don't return here for OS loaders unless they fail.

  if (InitrdHandle != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
            InitrdHandle,
            &gEfiDevicePathProtocolGuid,
            &mLinuxInitrdDevicePath,
            &gEfiLoadFile2ProtocolGuid,
            &mLoadFile2,
            NULL
            );
  }

  FreePool (FitBuffer);

  return 0;
}
