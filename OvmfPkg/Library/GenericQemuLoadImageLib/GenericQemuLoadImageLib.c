/**  @file
  Generic implementation of QemuLoadImageLib library class interface.

  Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Base.h>
#include <Guid/QemuKernelLoaderFsMedia.h>
#include <Library/DebugLib.h>
#include <Library/FileHandleLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/QemuLoadImageLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#pragma pack (1)
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  FilePathHeader;
  CHAR16                    FilePath[ARRAY_SIZE (L"kernel")];
} KERNEL_FILE_DEVPATH;

typedef struct {
  VENDOR_DEVICE_PATH        VenMediaNode;
  KERNEL_FILE_DEVPATH       FileNode;
  EFI_DEVICE_PATH_PROTOCOL  EndNode;
} KERNEL_VENMEDIA_FILE_DEVPATH;

typedef struct {
  VENDOR_DEVICE_PATH       VenMediaNode;
  EFI_DEVICE_PATH_PROTOCOL EndNode;
} SINGLE_VENMEDIA_NODE_DEVPATH;
#pragma pack ()

STATIC CONST KERNEL_VENMEDIA_FILE_DEVPATH mKernelDevicePath = {
  {
    {
      MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH) }
    },
    QEMU_KERNEL_LOADER_FS_MEDIA_GUID
  }, {
    {
      MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP,
      { sizeof (KERNEL_FILE_DEVPATH) }
    },
    L"kernel",
  }, {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL) }
  }
};

STATIC CONST SINGLE_VENMEDIA_NODE_DEVPATH mQemuKernelLoaderFsDevicePath = {
  {
    {
      MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH) }
    },
    QEMU_KERNEL_LOADER_FS_MEDIA_GUID
  }, {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL) }
  }
};

STATIC
EFI_STATUS
GetQemuKernelLoaderBlobSize (
  IN  EFI_FILE_HANDLE     Root,
  IN  CHAR16              *FileName,
  OUT UINTN               *Size
  )
{
  EFI_STATUS      Status;
  EFI_FILE_HANDLE FileHandle;
  UINT64          FileSize;

  Status = Root->Open (Root, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = FileHandleGetSize (FileHandle, &FileSize);
  if (EFI_ERROR (Status)) {
    goto CloseFile;
  }
  if (FileSize > MAX_UINTN) {
    Status = EFI_UNSUPPORTED;
    goto CloseFile;
  }
  *Size = (UINTN)FileSize;
  Status = EFI_SUCCESS;
CloseFile:
  FileHandle->Close (FileHandle);
  return Status;
}

STATIC
EFI_STATUS
ReadWholeQemuKernelLoaderBlob (
  IN  EFI_FILE_HANDLE     Root,
  IN  CHAR16              *FileName,
  IN  UINTN               Size,
  OUT VOID                *Buffer
  )
{
  EFI_STATUS      Status;
  EFI_FILE_HANDLE FileHandle;
  UINTN           ReadSize;

  Status = Root->Open (Root, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ReadSize = Size;
  Status = FileHandle->Read (FileHandle, &ReadSize, Buffer);
  if (EFI_ERROR (Status)) {
    goto CloseFile;
  }
  if (ReadSize != Size) {
    Status = EFI_PROTOCOL_ERROR;
    goto CloseFile;
  }
  Status = EFI_SUCCESS;
CloseFile:
  FileHandle->Close (FileHandle);
  return Status;
}

/**
  Download the kernel, the initial ramdisk, and the kernel command line from
  QEMU's fw_cfg. The kernel will be instructed via its command line to load
  the initrd from the same Simple FileSystem where the kernel was loaded from.

  @param[out] ImageHandle       The image handle that was allocated for
                                loading the image

  @retval EFI_SUCCESS           The image was loaded successfully.
  @retval EFI_NOT_FOUND         Kernel image was not found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_PROTOCOL_ERROR    Unterminated kernel command line.
  @retval EFI_ACCESS_DENIED     The underlying LoadImage boot service call
                                returned EFI_SECURITY_VIOLATION, and the image
                                was unloaded again.

  @return                       Error codes from any of the underlying
                                functions.
**/
EFI_STATUS
EFIAPI
QemuLoadKernelImage (
  OUT EFI_HANDLE                  *ImageHandle
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      KernelImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL       *KernelLoadedImage;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePathNode;
  EFI_HANDLE                      FsVolumeHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FsProtocol;
  EFI_FILE_HANDLE                 Root;
  UINTN                           CommandLineSize;
  CHAR8                           *CommandLine;
  UINTN                           InitrdSize;

  //
  // Load the image. This should call back into the QEMU EFI loader file system.
  //
  Status = gBS->LoadImage (
                  FALSE,                    // BootPolicy: exact match required
                  gImageHandle,             // ParentImageHandle
                  (EFI_DEVICE_PATH_PROTOCOL *)&mKernelDevicePath,
                  NULL,                     // SourceBuffer
                  0,                        // SourceSize
                  &KernelImageHandle
                  );
  switch (Status) {
  case EFI_SUCCESS:
    break;

  case EFI_SECURITY_VIOLATION:
    //
    // In this case, the image was loaded but failed to authenticate.
    //
    Status = EFI_ACCESS_DENIED;
    goto UnloadImage;

  default:
    DEBUG ((Status == EFI_NOT_FOUND ? DEBUG_INFO : DEBUG_ERROR,
      "%a: LoadImage(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  //
  // Construct the kernel command line.
  //
  Status = gBS->OpenProtocol (
                  KernelImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&KernelLoadedImage,
                  gImageHandle,                  // AgentHandle
                  NULL,                          // ControllerHandle
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Open the Qemu Kernel Loader abstract filesystem (volume) which will be
  // used to query the "initrd" and to read the "cmdline" synthetic files.
  //
  DevicePathNode = (EFI_DEVICE_PATH_PROTOCOL *)&mQemuKernelLoaderFsDevicePath;
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &DevicePathNode,
                  &FsVolumeHandle
                  );
  if (EFI_ERROR (Status)) {
    goto UnloadImage;
  }

  Status = gBS->HandleProtocol (
                  FsVolumeHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FsProtocol
                  );
  if (EFI_ERROR (Status)) {
    goto UnloadImage;
  }

  Status = FsProtocol->OpenVolume (FsVolumeHandle, &Root);
  if (EFI_ERROR (Status)) {
    goto UnloadImage;
  }

  Status = GetQemuKernelLoaderBlobSize (Root, L"cmdline", &CommandLineSize);
  if (EFI_ERROR (Status)) {
    goto CloseRoot;
  }

  if (CommandLineSize == 0) {
    KernelLoadedImage->LoadOptionsSize = 0;
  } else {
    CommandLine = AllocatePool (CommandLineSize);
    if (CommandLine == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto CloseRoot;
    }

    Status = ReadWholeQemuKernelLoaderBlob (Root, L"cmdline", CommandLineSize,
               CommandLine);
    if (EFI_ERROR (Status)) {
      goto FreeCommandLine;
    }

    //
    // Verify NUL-termination of the command line.
    //
    if (CommandLine[CommandLineSize - 1] != '\0') {
      DEBUG ((DEBUG_ERROR, "%a: kernel command line is not NUL-terminated\n",
        __FUNCTION__));
      Status = EFI_PROTOCOL_ERROR;
      goto FreeCommandLine;
    }

    //
    // Drop the terminating NUL, convert to UTF-16.
    //
    KernelLoadedImage->LoadOptionsSize = (UINT32)((CommandLineSize - 1) * 2);
  }

  Status = GetQemuKernelLoaderBlobSize (Root, L"initrd", &InitrdSize);
  if (EFI_ERROR (Status)) {
    goto FreeCommandLine;
  }

  if (InitrdSize > 0) {
    //
    // Append ' initrd=initrd' in UTF-16.
    //
    KernelLoadedImage->LoadOptionsSize += sizeof (L" initrd=initrd") - 2;
  }

  if (KernelLoadedImage->LoadOptionsSize == 0) {
    KernelLoadedImage->LoadOptions = NULL;
  } else {
    //
    // NUL-terminate in UTF-16.
    //
    KernelLoadedImage->LoadOptionsSize += 2;

    KernelLoadedImage->LoadOptions = AllocatePool (
                                       KernelLoadedImage->LoadOptionsSize);
    if (KernelLoadedImage->LoadOptions == NULL) {
      KernelLoadedImage->LoadOptionsSize = 0;
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeCommandLine;
    }

    UnicodeSPrintAsciiFormat (
      KernelLoadedImage->LoadOptions,
      KernelLoadedImage->LoadOptionsSize,
      "%a%a",
      (CommandLineSize == 0) ?  "" : CommandLine,
      (InitrdSize == 0)      ?  "" : " initrd=initrd"
      );
    DEBUG ((DEBUG_INFO, "%a: command line: \"%s\"\n", __FUNCTION__,
      (CHAR16 *)KernelLoadedImage->LoadOptions));
  }

  *ImageHandle = KernelImageHandle;
  Status = EFI_SUCCESS;

FreeCommandLine:
  if (CommandLineSize > 0) {
    FreePool (CommandLine);
  }
CloseRoot:
  Root->Close (Root);
UnloadImage:
  if (EFI_ERROR (Status)) {
    gBS->UnloadImage (KernelImageHandle);
  }

  return Status;
}

/**
  Transfer control to a kernel image loaded with QemuLoadKernelImage ()

  @param[in,out]  ImageHandle     Handle of image to be started. May assume a
                                  different value on return if the image was
                                  reloaded.

  @retval EFI_INVALID_PARAMETER   ImageHandle is either an invalid image handle
                                  or the image has already been initialized with
                                  StartImage
  @retval EFI_SECURITY_VIOLATION  The current platform policy specifies that the
                                  image should not be started.

  @return                         Error codes returned by the started image
**/
EFI_STATUS
EFIAPI
QemuStartKernelImage (
  IN  OUT EFI_HANDLE          *ImageHandle
  )
{
  return gBS->StartImage (
                *ImageHandle,
                NULL,              // ExitDataSize
                NULL               // ExitData
                );
}

/**
  Unloads an image loaded with QemuLoadKernelImage ().

  @param  ImageHandle             Handle that identifies the image to be
                                  unloaded.

  @retval EFI_SUCCESS             The image has been unloaded.
  @retval EFI_UNSUPPORTED         The image has been started, and does not
                                  support unload.
  @retval EFI_INVALID_PARAMETER   ImageHandle is not a valid image handle.

  @return                         Exit code from the image's unload function.
**/
EFI_STATUS
EFIAPI
QemuUnloadKernelImage (
  IN  EFI_HANDLE          ImageHandle
  )
{
  EFI_LOADED_IMAGE_PROTOCOL   *KernelLoadedImage;
  EFI_STATUS                  Status;

  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&KernelLoadedImage,
                  gImageHandle,                  // AgentHandle
                  NULL,                          // ControllerHandle
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (KernelLoadedImage->LoadOptions != NULL) {
    FreePool (KernelLoadedImage->LoadOptions);
    KernelLoadedImage->LoadOptions = NULL;
  }
  KernelLoadedImage->LoadOptionsSize = 0;

  return gBS->UnloadImage (ImageHandle);
}
