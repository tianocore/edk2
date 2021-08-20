/**  @file
  X86 specific implementation of QemuLoadImageLib library class interface
  with support for loading mixed mode images and non-EFI stub images

  Note that this implementation reads the cmdline (and possibly kernel, setup
  data, and initrd in the legacy boot mode) from fw_cfg directly.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/QemuKernelLoaderFsMedia.h>
#include <Library/DebugLib.h>
#include <Library/LoadLinuxLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuLoadImageLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OvmfLoadedX86LinuxKernel.h>

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

STATIC
VOID
FreeLegacyImage (
  IN  OVMF_LOADED_X86_LINUX_KERNEL *LoadedImage
  )
{
  if (LoadedImage->SetupBuf != NULL) {
    FreePages (LoadedImage->SetupBuf,
      EFI_SIZE_TO_PAGES (LoadedImage->SetupSize));
  }
  if (LoadedImage->KernelBuf != NULL) {
    FreePages (LoadedImage->KernelBuf,
      EFI_SIZE_TO_PAGES (LoadedImage->KernelInitialSize));
  }
  if (LoadedImage->CommandLine != NULL) {
    FreePages (LoadedImage->CommandLine,
      EFI_SIZE_TO_PAGES (LoadedImage->CommandLineSize));
  }
  if (LoadedImage->InitrdData != NULL) {
    FreePages (LoadedImage->InitrdData,
      EFI_SIZE_TO_PAGES (LoadedImage->InitrdSize));
  }
}

STATIC
EFI_STATUS
QemuLoadLegacyImage (
  OUT EFI_HANDLE                  *ImageHandle
  )
{
  EFI_STATUS                      Status;
  UINTN                           KernelSize;
  UINTN                           SetupSize;
  OVMF_LOADED_X86_LINUX_KERNEL    *LoadedImage;

  QemuFwCfgSelectItem (QemuFwCfgItemKernelSize);
  KernelSize = (UINTN)QemuFwCfgRead32 ();

  QemuFwCfgSelectItem (QemuFwCfgItemKernelSetupSize);
  SetupSize = (UINTN)QemuFwCfgRead32 ();

  if (KernelSize == 0 || SetupSize == 0) {
    DEBUG ((DEBUG_INFO, "qemu -kernel was not used.\n"));
    return EFI_NOT_FOUND;
  }

  LoadedImage = AllocateZeroPool (sizeof (*LoadedImage));
  if (LoadedImage == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  LoadedImage->SetupSize = SetupSize;
  LoadedImage->SetupBuf = LoadLinuxAllocateKernelSetupPages (
                            EFI_SIZE_TO_PAGES (LoadedImage->SetupSize));
  if (LoadedImage->SetupBuf == NULL) {
    DEBUG ((DEBUG_ERROR, "Unable to allocate memory for kernel setup!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeImageDesc;
  }

  DEBUG ((DEBUG_INFO, "Setup size: 0x%x\n", (UINT32)LoadedImage->SetupSize));
  DEBUG ((DEBUG_INFO, "Reading kernel setup image ..."));
  QemuFwCfgSelectItem (QemuFwCfgItemKernelSetupData);
  QemuFwCfgReadBytes (LoadedImage->SetupSize, LoadedImage->SetupBuf);
  DEBUG ((DEBUG_INFO, " [done]\n"));

  Status = LoadLinuxCheckKernelSetup (LoadedImage->SetupBuf,
             LoadedImage->SetupSize);
  if (EFI_ERROR (Status)) {
    goto FreeImage;
  }

  Status = LoadLinuxInitializeKernelSetup (LoadedImage->SetupBuf);
  if (EFI_ERROR (Status)) {
    goto FreeImage;
  }

  LoadedImage->KernelInitialSize = LoadLinuxGetKernelSize (
                                     LoadedImage->SetupBuf, KernelSize);
  if (LoadedImage->KernelInitialSize == 0) {
    Status = EFI_UNSUPPORTED;
    goto FreeImage;
  }

  LoadedImage->KernelBuf = LoadLinuxAllocateKernelPages (
                             LoadedImage->SetupBuf,
                             EFI_SIZE_TO_PAGES (LoadedImage->KernelInitialSize)
                             );
  if (LoadedImage->KernelBuf == NULL) {
    DEBUG ((DEBUG_ERROR, "Unable to allocate memory for kernel!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeImage;
  }

  DEBUG ((DEBUG_INFO, "Kernel size: 0x%x\n", (UINT32)KernelSize));
  DEBUG ((DEBUG_INFO, "Reading kernel image ..."));
  QemuFwCfgSelectItem (QemuFwCfgItemKernelData);
  QemuFwCfgReadBytes (KernelSize, LoadedImage->KernelBuf);
  DEBUG ((DEBUG_INFO, " [done]\n"));

  QemuFwCfgSelectItem (QemuFwCfgItemCommandLineSize);
  LoadedImage->CommandLineSize = (UINTN)QemuFwCfgRead32 ();

  if (LoadedImage->CommandLineSize > 0) {
    LoadedImage->CommandLine = LoadLinuxAllocateCommandLinePages (
                                 EFI_SIZE_TO_PAGES (
                                   LoadedImage->CommandLineSize));
    if (LoadedImage->CommandLine == NULL) {
      DEBUG ((DEBUG_ERROR, "Unable to allocate memory for kernel command line!\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeImage;
    }
    QemuFwCfgSelectItem (QemuFwCfgItemCommandLineData);
    QemuFwCfgReadBytes (LoadedImage->CommandLineSize, LoadedImage->CommandLine);
  }

  Status = LoadLinuxSetCommandLine (LoadedImage->SetupBuf,
             LoadedImage->CommandLine);
  if (EFI_ERROR (Status)) {
    goto FreeImage;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemInitrdSize);
  LoadedImage->InitrdSize = (UINTN)QemuFwCfgRead32 ();

  if (LoadedImage->InitrdSize > 0) {
    LoadedImage->InitrdData = LoadLinuxAllocateInitrdPages (
                                LoadedImage->SetupBuf,
                                EFI_SIZE_TO_PAGES (LoadedImage->InitrdSize));
    if (LoadedImage->InitrdData == NULL) {
      DEBUG ((DEBUG_ERROR, "Unable to allocate memory for initrd!\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeImage;
    }
    DEBUG ((DEBUG_INFO, "Initrd size: 0x%x\n",
      (UINT32)LoadedImage->InitrdSize));
    DEBUG ((DEBUG_INFO, "Reading initrd image ..."));
    QemuFwCfgSelectItem (QemuFwCfgItemInitrdData);
    QemuFwCfgReadBytes (LoadedImage->InitrdSize, LoadedImage->InitrdData);
    DEBUG ((DEBUG_INFO, " [done]\n"));
  }

  Status = LoadLinuxSetInitrd (LoadedImage->SetupBuf, LoadedImage->InitrdData,
             LoadedImage->InitrdSize);
  if (EFI_ERROR (Status)) {
    goto FreeImage;
  }

  *ImageHandle = NULL;
  Status = gBS->InstallProtocolInterface (ImageHandle,
                  &gOvmfLoadedX86LinuxKernelProtocolGuid, EFI_NATIVE_INTERFACE,
                  LoadedImage);
  if (EFI_ERROR (Status)) {
    goto FreeImage;
  }
  return EFI_SUCCESS;

FreeImage:
  FreeLegacyImage (LoadedImage);
FreeImageDesc:
  FreePool (LoadedImage);
  return Status;
}

STATIC
EFI_STATUS
QemuStartLegacyImage (
  IN  EFI_HANDLE                ImageHandle
  )
{
  EFI_STATUS                    Status;
  OVMF_LOADED_X86_LINUX_KERNEL  *LoadedImage;

  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gOvmfLoadedX86LinuxKernelProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,                  // AgentHandle
                  NULL,                          // ControllerHandle
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  return LoadLinux (LoadedImage->KernelBuf, LoadedImage->SetupBuf);
}

STATIC
EFI_STATUS
QemuUnloadLegacyImage (
  IN  EFI_HANDLE          ImageHandle
  )
{
  EFI_STATUS                    Status;
  OVMF_LOADED_X86_LINUX_KERNEL  *LoadedImage;

  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gOvmfLoadedX86LinuxKernelProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,                  // AgentHandle
                  NULL,                          // ControllerHandle
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->UninstallProtocolInterface (ImageHandle,
                  &gOvmfLoadedX86LinuxKernelProtocolGuid, LoadedImage);
  ASSERT_EFI_ERROR (Status);

  FreeLegacyImage (LoadedImage);
  FreePool (LoadedImage);
  return EFI_SUCCESS;
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

  @return                       Error codes from any of the underlying
                                functions.
**/
EFI_STATUS
EFIAPI
QemuLoadKernelImage (
  OUT EFI_HANDLE            *ImageHandle
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                KernelImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL *KernelLoadedImage;
  UINTN                     CommandLineSize;
  CHAR8                     *CommandLine;
  UINTN                     InitrdSize;

  //
  // Redundant assignment to work around GCC48/GCC49 limitations.
  //
  CommandLine = NULL;

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

  case EFI_NOT_FOUND:
    //
    // The image does not exist - no -kernel image was supplied via the
    // command line so no point in invoking the legacy fallback
    //
    return EFI_NOT_FOUND;

  case EFI_SECURITY_VIOLATION:
    //
    // Since the image has been loaded, we need to unload it before proceeding
    // to the EFI_ACCESS_DENIED case below.
    //
    gBS->UnloadImage (KernelImageHandle);
    //
    // Fall through
    //
  case EFI_ACCESS_DENIED:
    //
    // We are running with UEFI secure boot enabled, and the image failed to
    // authenticate. For compatibility reasons, we fall back to the legacy
    // loader in this case.
    //
    // Fall through
    //
  case EFI_UNSUPPORTED:
    //
    // The image is not natively supported or cross-type supported. Let's try
    // loading it using the loader that parses the bzImage metadata directly.
    //
    Status = QemuLoadLegacyImage (&KernelImageHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: QemuLoadLegacyImage(): %r\n", __FUNCTION__,
        Status));
      return Status;
    }
    *ImageHandle = KernelImageHandle;
    return EFI_SUCCESS;

  default:
    DEBUG ((DEBUG_ERROR, "%a: LoadImage(): %r\n", __FUNCTION__, Status));
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

  QemuFwCfgSelectItem (QemuFwCfgItemCommandLineSize);
  CommandLineSize = (UINTN)QemuFwCfgRead32 ();

  if (CommandLineSize == 0) {
    KernelLoadedImage->LoadOptionsSize = 0;
  } else {
    CommandLine = AllocatePool (CommandLineSize);
    if (CommandLine == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto UnloadImage;
    }

    QemuFwCfgSelectItem (QemuFwCfgItemCommandLineData);
    QemuFwCfgReadBytes (CommandLineSize, CommandLine);

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
    KernelLoadedImage->LoadOptionsSize = (UINT32) ((CommandLineSize - 1) * 2);
  }

  QemuFwCfgSelectItem (QemuFwCfgItemInitrdSize);
  InitrdSize = (UINTN)QemuFwCfgRead32 ();

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
  IN  OUT EFI_HANDLE            *ImageHandle
  )
{
  EFI_STATUS                    Status;
  OVMF_LOADED_X86_LINUX_KERNEL  *LoadedImage;

  Status = gBS->OpenProtocol (
                  *ImageHandle,
                  &gOvmfLoadedX86LinuxKernelProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,                  // AgentHandle
                  NULL,                          // ControllerHandle
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return QemuStartLegacyImage (*ImageHandle);
  }

  Status = gBS->StartImage (
                  *ImageHandle,
                  NULL,              // ExitDataSize
                  NULL               // ExitData
                  );
#ifdef MDE_CPU_IA32
  if (Status == EFI_UNSUPPORTED) {
    EFI_HANDLE KernelImageHandle;

    //
    // On IA32, EFI_UNSUPPORTED means that the image's machine type is X64 while
    // we are expecting a IA32 one, and the StartImage () boot service is unable
    // to handle it, either because the image does not have the special .compat
    // PE/COFF section that Linux specifies for mixed mode capable images, or
    // because we are running without the support code for that. So load the
    // image again, using the legacy loader, and unload the normally loaded
    // image before starting the legacy one.
    //
    Status = QemuLoadLegacyImage (&KernelImageHandle);
    if (EFI_ERROR (Status)) {
      //
      // Note: no change to (*ImageHandle), the caller will release it.
      //
      return Status;
    }
    //
    // Swap in the legacy-loaded image.
    //
    QemuUnloadKernelImage (*ImageHandle);
    *ImageHandle = KernelImageHandle;
    return QemuStartLegacyImage (KernelImageHandle);
  }
#endif
  return Status;
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
  if (Status == EFI_UNSUPPORTED) {
    //
    // The handle exists but does not have an instance of the standard loaded
    // image protocol installed on it. Attempt to unload it as a legacy image
    // instead.
    //
    return QemuUnloadLegacyImage (ImageHandle);
  }

  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We are unloading a normal, non-legacy loaded image, either on behalf of
  // an external caller, or called from QemuStartKernelImage() on IA32, while
  // switching from the normal to the legacy method to load and start a X64
  // image.
  //
  if (KernelLoadedImage->LoadOptions != NULL) {
    FreePool (KernelLoadedImage->LoadOptions);
    KernelLoadedImage->LoadOptions = NULL;
  }
  KernelLoadedImage->LoadOptionsSize = 0;

  return gBS->UnloadImage (ImageHandle);
}
