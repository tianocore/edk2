/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2017, Linaro. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <libfdt.h>
#include <Library/AndroidBootImgLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/AndroidBootImg.h>
#include <Protocol/LoadFile2.h>
#include <Protocol/LoadedImage.h>

#include <Guid/LinuxEfiInitrdMedia.h>

#define FDT_ADDITIONAL_ENTRIES_SIZE  0x400

typedef struct {
  MEMMAP_DEVICE_PATH          Node1;
  EFI_DEVICE_PATH_PROTOCOL    End;
} MEMORY_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH          VendorMediaNode;
  EFI_DEVICE_PATH_PROTOCOL    EndNode;
} RAMDISK_DEVICE_PATH;

STATIC ANDROID_BOOTIMG_PROTOCOL  *mAndroidBootImg;
STATIC VOID                      *mRamdiskData          = NULL;
STATIC UINTN                     mRamdiskSize           = 0;
STATIC EFI_HANDLE                mRamDiskLoadFileHandle = NULL;

STATIC CONST MEMORY_DEVICE_PATH  mMemoryDevicePathTemplate =
{
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      {
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
        (UINT8)((sizeof (MEMMAP_DEVICE_PATH)) >> 8),
      },
    }, // Header
    0, // StartingAddress (set at runtime)
    0  // EndingAddress   (set at runtime)
  }, // Node1
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  } // End
};

STATIC CONST RAMDISK_DEVICE_PATH  mRamdiskDevicePath =
{
  {
    {
      MEDIA_DEVICE_PATH,
      MEDIA_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH),       0 }
    },
    LINUX_EFI_INITRD_MEDIA_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

/**
  Causes the driver to load a specified file.

  @param  This       Protocol instance pointer.
  @param  FilePath   The device specific path of the file to load.
  @param  BootPolicy Should always be FALSE.
  @param  BufferSize On input the size of Buffer in bytes. On output with a return
                     code of EFI_SUCCESS, the amount of data transferred to
                     Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                     the size of Buffer required to retrieve the requested file.
  @param  Buffer     The memory buffer to transfer the file to. IF Buffer is NULL,
                     then no the size of the requested file is returned in
                     BufferSize.

  @retval EFI_SUCCESS           The file was loaded.
  @retval EFI_UNSUPPORTED       BootPolicy is TRUE.
  @retval EFI_INVALID_PARAMETER FilePath is not a valid device path, or
                                BufferSize is NULL.
  @retval EFI_NO_MEDIA          No medium was present to load the file.
  @retval EFI_DEVICE_ERROR      The file was not loaded due to a device error.
  @retval EFI_NO_RESPONSE       The remote system did not respond.
  @retval EFI_NOT_FOUND         The file was not found
  @retval EFI_ABORTED           The file load process was manually canceled.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to read the current
                                directory entry. BufferSize has been updated with
                                the size needed to complete the request.


**/
EFI_STATUS
EFIAPI
AndroidBootImgLoadFile2 (
  IN EFI_LOAD_FILE2_PROTOCOL   *This,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN BOOLEAN                   BootPolicy,
  IN OUT UINTN                 *BufferSize,
  IN VOID                      *Buffer OPTIONAL
  )

{
  // Verify if the valid parameters
  if ((This == NULL) ||
      (BufferSize == NULL) ||
      (FilePath == NULL) ||
      !IsDevicePathValid (FilePath, 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  // Check if the given buffer size is big enough
  // EFI_BUFFER_TOO_SMALL to allow caller to allocate a bigger buffer
  if (mRamdiskSize == 0) {
    return EFI_NOT_FOUND;
  }

  if ((Buffer == NULL) || (*BufferSize < mRamdiskSize)) {
    *BufferSize = mRamdiskSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  // Copy InitRd
  CopyMem (Buffer, mRamdiskData, mRamdiskSize);
  *BufferSize = mRamdiskSize;

  return EFI_SUCCESS;
}

///
/// Load File Protocol instance
///
STATIC EFI_LOAD_FILE2_PROTOCOL  mAndroidBootImgLoadFile2 = {
  AndroidBootImgLoadFile2
};

EFI_STATUS
AndroidBootImgGetImgSize (
  IN  VOID   *BootImg,
  OUT UINTN  *ImgSize
  )
{
  ANDROID_BOOTIMG_HEADER  *Header;

  Header = (ANDROID_BOOTIMG_HEADER *)BootImg;

  if (AsciiStrnCmp (
        (CONST CHAR8 *)Header->BootMagic,
        ANDROID_BOOT_MAGIC,
        ANDROID_BOOT_MAGIC_LENGTH
        ) != 0)
  {
    return EFI_INVALID_PARAMETER;
  }

  /* The page size is not specified, but it should be power of 2 at least */
  ASSERT (IS_VALID_ANDROID_PAGE_SIZE (Header->PageSize));

  /* Get real size of abootimg */
  *ImgSize = ALIGN_VALUE (Header->KernelSize, Header->PageSize) +
             ALIGN_VALUE (Header->RamdiskSize, Header->PageSize) +
             ALIGN_VALUE (Header->SecondStageBootloaderSize, Header->PageSize) +
             Header->PageSize;
  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgGetKernelInfo (
  IN  VOID   *BootImg,
  OUT VOID   **Kernel,
  OUT UINTN  *KernelSize
  )
{
  ANDROID_BOOTIMG_HEADER  *Header;

  Header = (ANDROID_BOOTIMG_HEADER *)BootImg;

  if (AsciiStrnCmp (
        (CONST CHAR8 *)Header->BootMagic,
        ANDROID_BOOT_MAGIC,
        ANDROID_BOOT_MAGIC_LENGTH
        ) != 0)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Header->KernelSize == 0) {
    return EFI_NOT_FOUND;
  }

  ASSERT (IS_VALID_ANDROID_PAGE_SIZE (Header->PageSize));

  *KernelSize = Header->KernelSize;
  *Kernel     = (VOID *)((UINTN)BootImg + Header->PageSize);
  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgGetRamdiskInfo (
  IN  VOID   *BootImg,
  OUT VOID   **Ramdisk,
  OUT UINTN  *RamdiskSize
  )
{
  ANDROID_BOOTIMG_HEADER  *Header;

  Header = (ANDROID_BOOTIMG_HEADER *)BootImg;

  if (AsciiStrnCmp (
        (CONST CHAR8 *)Header->BootMagic,
        ANDROID_BOOT_MAGIC,
        ANDROID_BOOT_MAGIC_LENGTH
        ) != 0)
  {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (IS_VALID_ANDROID_PAGE_SIZE (Header->PageSize));

  *RamdiskSize = Header->RamdiskSize;

  if (Header->RamdiskSize != 0) {
    *Ramdisk = (VOID *)((INTN)BootImg
                        + Header->PageSize
                        + ALIGN_VALUE (Header->KernelSize, Header->PageSize));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgGetSecondBootLoaderInfo (
  IN  VOID   *BootImg,
  OUT VOID   **Second,
  OUT UINTN  *SecondSize
  )
{
  ANDROID_BOOTIMG_HEADER  *Header;

  Header = (ANDROID_BOOTIMG_HEADER *)BootImg;

  if (AsciiStrnCmp (
        (CONST CHAR8 *)Header->BootMagic,
        ANDROID_BOOT_MAGIC,
        ANDROID_BOOT_MAGIC_LENGTH
        ) != 0)
  {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (IS_VALID_ANDROID_PAGE_SIZE (Header->PageSize));

  *SecondSize = Header->SecondStageBootloaderSize;

  if (Header->SecondStageBootloaderSize != 0) {
    *Second = (VOID *)((UINTN)BootImg
                       + Header->PageSize
                       + ALIGN_VALUE (Header->KernelSize, Header->PageSize)
                       + ALIGN_VALUE (Header->RamdiskSize, Header->PageSize));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgGetKernelArgs (
  IN  VOID   *BootImg,
  OUT CHAR8  *KernelArgs
  )
{
  ANDROID_BOOTIMG_HEADER  *Header;

  Header = (ANDROID_BOOTIMG_HEADER *)BootImg;
  AsciiStrnCpyS (
    KernelArgs,
    ANDROID_BOOTIMG_KERNEL_ARGS_SIZE,
    Header->KernelArgs,
    ANDROID_BOOTIMG_KERNEL_ARGS_SIZE
    );

  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgGetFdt (
  IN  VOID  *BootImg,
  IN  VOID  **FdtBase
  )
{
  UINTN       SecondLoaderSize;
  EFI_STATUS  Status;

  /* Check whether FDT is located in second boot region as some vendor do so,
   * because second loader is never used as far as I know. */
  Status = AndroidBootImgGetSecondBootLoaderInfo (
             BootImg,
             FdtBase,
             &SecondLoaderSize
             );
  return Status;
}

EFI_STATUS
AndroidBootImgUpdateArgs (
  IN  VOID  *BootImg,
  OUT VOID  *KernelArgs
  )
{
  CHAR8       ImageKernelArgs[ANDROID_BOOTIMG_KERNEL_ARGS_SIZE];
  EFI_STATUS  Status;

  // Get kernel arguments from Android boot image
  Status = AndroidBootImgGetKernelArgs (BootImg, ImageKernelArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AsciiStrToUnicodeStrS (
    ImageKernelArgs,
    KernelArgs,
    ANDROID_BOOTIMG_KERNEL_ARGS_SIZE >> 1
    );
  // Append platform kernel arguments
  if (mAndroidBootImg->AppendArgs) {
    Status = mAndroidBootImg->AppendArgs (
                                KernelArgs,
                                ANDROID_BOOTIMG_KERNEL_ARGS_SIZE
                                );
  }

  return Status;
}

EFI_STATUS
AndroidBootImgInstallLoadFile2 (
  IN  VOID   *RamdiskData,
  IN  UINTN  RamdiskSize
  )
{
  mRamDiskLoadFileHandle = NULL;
  mRamdiskData           = RamdiskData;
  mRamdiskSize           = RamdiskSize;
  return gBS->InstallMultipleProtocolInterfaces (
                &mRamDiskLoadFileHandle,
                &gEfiLoadFile2ProtocolGuid,
                &mAndroidBootImgLoadFile2,
                &gEfiDevicePathProtocolGuid,
                &mRamdiskDevicePath,
                NULL
                );
}

EFI_STATUS
AndroidBootImgUninstallLoadFile2 (
  VOID
  )
{
  EFI_STATUS  Status;

  Status       = EFI_SUCCESS;
  mRamdiskData = NULL;
  mRamdiskSize = 0;
  if (mRamDiskLoadFileHandle != NULL) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    mRamDiskLoadFileHandle,
                    &gEfiLoadFile2ProtocolGuid,
                    &mAndroidBootImgLoadFile2,
                    &gEfiDevicePathProtocolGuid,
                    &mRamdiskDevicePath,
                    NULL
                    );
    mRamDiskLoadFileHandle = NULL;
  }

  return Status;
}

BOOLEAN
AndroidBootImgAcpiSupported (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *AcpiTable;

  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, &AcpiTable);
  return !EFI_ERROR (Status);
}

EFI_STATUS
AndroidBootImgLocateFdt (
  IN  VOID  *BootImg,
  IN  VOID  **FdtBase
  )
{
  INTN        Err;
  EFI_STATUS  Status;

  Status = EfiGetSystemConfigurationTable (&gFdtTableGuid, FdtBase);
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  Status = AndroidBootImgGetFdt (BootImg, FdtBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Err = fdt_check_header (*FdtBase);
  if (Err != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Device Tree header not valid (Err:%d)\n",
      Err
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

INTN
AndroidBootImgGetChosenNode (
  IN  INTN  UpdatedFdtBase
  )
{
  INTN  ChosenNode;

  ChosenNode = fdt_subnode_offset ((CONST VOID *)UpdatedFdtBase, 0, "chosen");
  if (ChosenNode < 0) {
    ChosenNode = fdt_add_subnode ((VOID *)UpdatedFdtBase, 0, "chosen");
    if (ChosenNode < 0) {
      DEBUG ((DEBUG_ERROR, "Fail to find fdt node chosen!\n"));
      return 0;
    }
  }

  return ChosenNode;
}

EFI_STATUS
AndroidBootImgSetProperty64 (
  IN  INTN    UpdatedFdtBase,
  IN  INTN    ChosenNode,
  IN  CHAR8   *PropertyName,
  IN  UINT64  Val
  )
{
  INTN                 Err;
  struct fdt_property  *Property;
  int                  Len;

  Property = fdt_get_property_w (
               (VOID *)UpdatedFdtBase,
               ChosenNode,
               PropertyName,
               &Len
               );
  if ((NULL == Property) && (Len == -FDT_ERR_NOTFOUND)) {
    Val = cpu_to_fdt64 (Val);
    Err = fdt_appendprop (
            (VOID *)UpdatedFdtBase,
            ChosenNode,
            PropertyName,
            &Val,
            sizeof (UINT64)
            );
    if (Err) {
      DEBUG ((DEBUG_ERROR, "fdt_appendprop() fail: %a\n", fdt_strerror (Err)));
      return EFI_INVALID_PARAMETER;
    }
  } else if (Property != NULL) {
    Err = fdt_setprop_u64 (
            (VOID *)UpdatedFdtBase,
            ChosenNode,
            PropertyName,
            Val
            );
    if (Err) {
      DEBUG ((DEBUG_ERROR, "fdt_setprop_u64() fail: %a\n", fdt_strerror (Err)));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Failed to set fdt Property %a\n", PropertyName));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgUpdateFdt (
  IN  VOID   *BootImg,
  IN  VOID   *FdtBase,
  IN  VOID   *RamdiskData,
  IN  UINTN  RamdiskSize
  )
{
  INTN                  ChosenNode, Err, NewFdtSize;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  UpdatedFdtBase, NewFdtBase;

  NewFdtSize = (UINTN)fdt_totalsize (FdtBase)
               + FDT_ADDITIONAL_ENTRIES_SIZE;
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (NewFdtSize),
                  &UpdatedFdtBase
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "Warning: Failed to reallocate FDT, err %d.\n",
      Status
      ));
    return Status;
  }

  // Load the Original FDT tree into the new region
  Err = fdt_open_into (FdtBase, (VOID *)(INTN)UpdatedFdtBase, NewFdtSize);
  if (Err) {
    DEBUG ((DEBUG_ERROR, "fdt_open_into(): %a\n", fdt_strerror (Err)));
    Status = EFI_INVALID_PARAMETER;
    goto Fdt_Exit;
  }

  if (FeaturePcdGet (PcdAndroidBootLoadFile2)) {
    Status = AndroidBootImgInstallLoadFile2 (RamdiskData, RamdiskSize);
    if (EFI_ERROR (Status)) {
      goto Fdt_Exit;
    }
  } else {
    ChosenNode = AndroidBootImgGetChosenNode (UpdatedFdtBase);
    if (!ChosenNode) {
      goto Fdt_Exit;
    }

    Status = AndroidBootImgSetProperty64 (
               UpdatedFdtBase,
               ChosenNode,
               "linux,initrd-start",
               (UINTN)RamdiskData
               );
    if (EFI_ERROR (Status)) {
      goto Fdt_Exit;
    }

    Status = AndroidBootImgSetProperty64 (
               UpdatedFdtBase,
               ChosenNode,
               "linux,initrd-end",
               (UINTN)RamdiskData + RamdiskSize
               );
    if (EFI_ERROR (Status)) {
      goto Fdt_Exit;
    }
  }

  if (mAndroidBootImg->UpdateDtb) {
    Status = mAndroidBootImg->UpdateDtb (UpdatedFdtBase, &NewFdtBase);
    if (EFI_ERROR (Status)) {
      goto Fdt_Exit;
    }
  } else {
    NewFdtBase = UpdatedFdtBase;
  }

  Status = gBS->InstallConfigurationTable (
                  &gFdtTableGuid,
                  (VOID *)(UINTN)NewFdtBase
                  );

  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

Fdt_Exit:
  gBS->FreePages (UpdatedFdtBase, EFI_SIZE_TO_PAGES (NewFdtSize));
  return Status;
}

EFI_STATUS
AndroidBootImgBoot (
  IN VOID   *Buffer,
  IN UINTN  BufferSize
  )
{
  EFI_STATUS                 Status;
  VOID                       *Kernel;
  UINTN                      KernelSize;
  MEMORY_DEVICE_PATH         KernelDevicePath;
  EFI_HANDLE                 ImageHandle;
  VOID                       *NewKernelArg;
  EFI_LOADED_IMAGE_PROTOCOL  *ImageInfo;
  VOID                       *RamdiskData;
  UINTN                      RamdiskSize;
  IN  VOID                   *FdtBase;

  NewKernelArg = NULL;
  ImageHandle  = NULL;

  Status = gBS->LocateProtocol (
                  &gAndroidBootImgProtocolGuid,
                  NULL,
                  (VOID **)&mAndroidBootImg
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = AndroidBootImgGetKernelInfo (
             Buffer,
             &Kernel,
             &KernelSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  NewKernelArg = AllocateZeroPool (ANDROID_BOOTIMG_KERNEL_ARGS_SIZE);
  if (NewKernelArg == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Status = AndroidBootImgUpdateArgs (Buffer, NewKernelArg);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = AndroidBootImgGetRamdiskInfo (
             Buffer,
             &RamdiskData,
             &RamdiskSize
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (AndroidBootImgAcpiSupported ()) {
    Status = AndroidBootImgInstallLoadFile2 (RamdiskData, RamdiskSize);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  } else {
    Status = AndroidBootImgLocateFdt (Buffer, &FdtBase);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = AndroidBootImgUpdateFdt (Buffer, FdtBase, RamdiskData, RamdiskSize);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  KernelDevicePath = mMemoryDevicePathTemplate;

  KernelDevicePath.Node1.StartingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Kernel;
  KernelDevicePath.Node1.EndingAddress   = (EFI_PHYSICAL_ADDRESS)(UINTN)Kernel
                                           + KernelSize;

  Status = gBS->LoadImage (
                  TRUE,
                  gImageHandle,
                  (EFI_DEVICE_PATH *)&KernelDevicePath,
                  (VOID *)(UINTN)Kernel,
                  KernelSize,
                  &ImageHandle
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  // Set kernel arguments
  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&ImageInfo
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  ImageInfo->LoadOptions     = NewKernelArg;
  ImageInfo->LoadOptionsSize = StrLen (NewKernelArg) * sizeof (CHAR16);

  // Before calling the image, enable the Watchdog Timer for  the 5 Minute period
  gBS->SetWatchdogTimer (5 * 60, 0x10000, 0, NULL);
  // Start the image
  Status = gBS->StartImage (ImageHandle, NULL, NULL);
  // Clear the Watchdog Timer if the image returns
  gBS->SetWatchdogTimer (0, 0x10000, 0, NULL);

Exit:
  // Unload image as it will not be used anymore
  if (ImageHandle != NULL) {
    gBS->UnloadImage (ImageHandle);
    ImageHandle = NULL;
  }

  if (EFI_ERROR (Status)) {
    if (NewKernelArg != NULL) {
      FreePool (NewKernelArg);
      NewKernelArg = NULL;
    }
  }

  AndroidBootImgUninstallLoadFile2 ();
  return Status;
}
