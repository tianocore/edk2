/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2017, Linaro. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <libfdt.h>
#include <Library/AndroidBootImgLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/AndroidBootImg.h>
#include <Protocol/LoadedImage.h>

#include <libfdt.h>

#define FDT_ADDITIONAL_ENTRIES_SIZE 0x400

typedef struct {
  MEMMAP_DEVICE_PATH                      Node1;
  EFI_DEVICE_PATH_PROTOCOL                End;
} MEMORY_DEVICE_PATH;

STATIC ANDROID_BOOTIMG_PROTOCOL                 *mAndroidBootImg;

STATIC CONST MEMORY_DEVICE_PATH mMemoryDevicePathTemplate =
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

EFI_STATUS
AndroidBootImgGetImgSize (
  IN  VOID    *BootImg,
  OUT UINTN   *ImgSize
  )
{
  ANDROID_BOOTIMG_HEADER   *Header;

  Header = (ANDROID_BOOTIMG_HEADER *) BootImg;

  if (AsciiStrnCmp ((CONST CHAR8 *)Header->BootMagic, ANDROID_BOOT_MAGIC,
                    ANDROID_BOOT_MAGIC_LENGTH) != 0) {
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
  IN  VOID    *BootImg,
  OUT VOID   **Kernel,
  OUT UINTN   *KernelSize
  )
{
  ANDROID_BOOTIMG_HEADER   *Header;

  Header = (ANDROID_BOOTIMG_HEADER *) BootImg;

  if (AsciiStrnCmp ((CONST CHAR8 *)Header->BootMagic, ANDROID_BOOT_MAGIC,
                    ANDROID_BOOT_MAGIC_LENGTH) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Header->KernelSize == 0) {
    return EFI_NOT_FOUND;
  }

  ASSERT (IS_VALID_ANDROID_PAGE_SIZE (Header->PageSize));

  *KernelSize = Header->KernelSize;
  *Kernel = (VOID *)((UINTN)BootImg + Header->PageSize);
  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgGetRamdiskInfo (
  IN  VOID    *BootImg,
  OUT VOID   **Ramdisk,
  OUT UINTN   *RamdiskSize
  )
{
  ANDROID_BOOTIMG_HEADER   *Header;

  Header = (ANDROID_BOOTIMG_HEADER *)BootImg;

  if (AsciiStrnCmp ((CONST CHAR8 *)Header->BootMagic, ANDROID_BOOT_MAGIC,
                    ANDROID_BOOT_MAGIC_LENGTH) != 0) {
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
  IN  VOID    *BootImg,
  OUT VOID   **Second,
  OUT UINTN   *SecondSize
  )
{
  ANDROID_BOOTIMG_HEADER   *Header;

  Header = (ANDROID_BOOTIMG_HEADER *)BootImg;

  if (AsciiStrnCmp ((CONST CHAR8 *)Header->BootMagic, ANDROID_BOOT_MAGIC,
                    ANDROID_BOOT_MAGIC_LENGTH) != 0) {
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
  IN  VOID    *BootImg,
  OUT CHAR8   *KernelArgs
  )
{
  ANDROID_BOOTIMG_HEADER   *Header;

  Header = (ANDROID_BOOTIMG_HEADER *) BootImg;
  AsciiStrnCpyS (KernelArgs, ANDROID_BOOTIMG_KERNEL_ARGS_SIZE, Header->KernelArgs,
    ANDROID_BOOTIMG_KERNEL_ARGS_SIZE);

  return EFI_SUCCESS;
}

EFI_STATUS
AndroidBootImgGetFdt (
  IN  VOID                  *BootImg,
  IN  VOID                 **FdtBase
  )
{
  UINTN                      SecondLoaderSize;
  EFI_STATUS                 Status;

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
  IN  VOID                  *BootImg,
  OUT VOID                  *KernelArgs
  )
{
  CHAR8                      ImageKernelArgs[ANDROID_BOOTIMG_KERNEL_ARGS_SIZE];
  EFI_STATUS                 Status;

  // Get kernel arguments from Android boot image
  Status = AndroidBootImgGetKernelArgs (BootImg, ImageKernelArgs);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  AsciiStrToUnicodeStrS (ImageKernelArgs, KernelArgs,
                         ANDROID_BOOTIMG_KERNEL_ARGS_SIZE >> 1);
  // Append platform kernel arguments
  if(mAndroidBootImg->AppendArgs) {
    Status = mAndroidBootImg->AppendArgs (KernelArgs,
                                ANDROID_BOOTIMG_KERNEL_ARGS_SIZE);
  }
  return Status;
}

EFI_STATUS
AndroidBootImgLocateFdt (
  IN  VOID                  *BootImg,
  IN  VOID                 **FdtBase
  )
{
  INTN                       Err;
  EFI_STATUS                 Status;

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
    DEBUG ((DEBUG_ERROR, "ERROR: Device Tree header not valid (Err:%d)\n",
           Err));
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

INTN
AndroidBootImgGetChosenNode (
  IN  INTN   UpdatedFdtBase
  )
{
  INTN                   ChosenNode;

  ChosenNode = fdt_subnode_offset ((CONST VOID *)UpdatedFdtBase, 0, "chosen");
  if (ChosenNode < 0) {
    ChosenNode = fdt_add_subnode((VOID *)UpdatedFdtBase, 0, "chosen");
      if (ChosenNode < 0) {
        DEBUG ((DEBUG_ERROR, "Fail to find fdt node chosen!\n"));
        return 0;
    }
  }
  return ChosenNode;
}

EFI_STATUS
AndroidBootImgSetProperty64 (
  IN  INTN                   UpdatedFdtBase,
  IN  INTN                   ChosenNode,
  IN  CHAR8                 *PropertyName,
  IN  UINT64                 Val
  )
{
  INTN                      Err;
  struct fdt_property      *Property;
  int                       Len;

  Property = fdt_get_property_w((VOID *)UpdatedFdtBase, ChosenNode,
                            PropertyName, &Len);
  if (NULL == Property && Len == -FDT_ERR_NOTFOUND) {
    Val = cpu_to_fdt64(Val);
    Err = fdt_appendprop ((VOID *)UpdatedFdtBase, ChosenNode,
                          PropertyName, &Val, sizeof (UINT64));
    if (Err) {
      DEBUG ((DEBUG_ERROR, "fdt_appendprop() fail: %a\n", fdt_strerror (Err)));
      return EFI_INVALID_PARAMETER;
    }
  } else if (Property != NULL) {
    Err = fdt_setprop_u64((VOID *)UpdatedFdtBase, ChosenNode,
                          PropertyName, Val);
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
  IN  VOID                  *BootImg,
  IN  VOID                  *FdtBase,
  IN  VOID                  *RamdiskData,
  IN  UINTN                  RamdiskSize
  )
{
  INTN                       ChosenNode, Err, NewFdtSize;
  EFI_STATUS                 Status;
  EFI_PHYSICAL_ADDRESS       UpdatedFdtBase, NewFdtBase;

  NewFdtSize = (UINTN)fdt_totalsize (FdtBase)
               + FDT_ADDITIONAL_ENTRIES_SIZE;
  Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (NewFdtSize), &UpdatedFdtBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Warning: Failed to reallocate FDT, err %d.\n",
           Status));
    return Status;
  }

  // Load the Original FDT tree into the new region
  Err = fdt_open_into(FdtBase, (VOID*)(INTN)UpdatedFdtBase, NewFdtSize);
  if (Err) {
    DEBUG ((DEBUG_ERROR, "fdt_open_into(): %a\n", fdt_strerror (Err)));
    Status = EFI_INVALID_PARAMETER;
    goto Fdt_Exit;
  }

  ChosenNode = AndroidBootImgGetChosenNode(UpdatedFdtBase);
  if (!ChosenNode) {
    goto Fdt_Exit;
  }

  Status = AndroidBootImgSetProperty64 (UpdatedFdtBase, ChosenNode,
                                        "linux,initrd-start",
                                        (UINTN)RamdiskData);
  if (EFI_ERROR (Status)) {
    goto Fdt_Exit;
  }

  Status = AndroidBootImgSetProperty64 (UpdatedFdtBase, ChosenNode,
                                        "linux,initrd-end",
                                        (UINTN)RamdiskData + RamdiskSize);
  if (EFI_ERROR (Status)) {
    goto Fdt_Exit;
  }

  if (mAndroidBootImg->UpdateDtb) {
    Status = mAndroidBootImg->UpdateDtb (UpdatedFdtBase, &NewFdtBase);
    if (EFI_ERROR (Status)) {
      goto Fdt_Exit;
    }

    Status = gBS->InstallConfigurationTable (
                    &gFdtTableGuid,
                    (VOID *)(UINTN)NewFdtBase
                    );
  }

  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

Fdt_Exit:
  gBS->FreePages (UpdatedFdtBase, EFI_SIZE_TO_PAGES (NewFdtSize));
  return Status;
}

EFI_STATUS
AndroidBootImgBoot (
  IN VOID                            *Buffer,
  IN UINTN                            BufferSize
  )
{
  EFI_STATUS                          Status;
  VOID                               *Kernel;
  UINTN                               KernelSize;
  MEMORY_DEVICE_PATH                  KernelDevicePath;
  EFI_HANDLE                          ImageHandle;
  VOID                               *NewKernelArg;
  EFI_LOADED_IMAGE_PROTOCOL          *ImageInfo;
  VOID                               *RamdiskData;
  UINTN                               RamdiskSize;
  IN  VOID                           *FdtBase;

  Status = gBS->LocateProtocol (&gAndroidBootImgProtocolGuid, NULL,
                                (VOID **) &mAndroidBootImg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AndroidBootImgGetKernelInfo (
            Buffer,
            &Kernel,
            &KernelSize
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NewKernelArg = AllocateZeroPool (ANDROID_BOOTIMG_KERNEL_ARGS_SIZE);
  if (NewKernelArg == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AndroidBootImgUpdateArgs (Buffer, NewKernelArg);
  if (EFI_ERROR (Status)) {
    FreePool (NewKernelArg);
    return Status;
  }

  Status = AndroidBootImgGetRamdiskInfo (
            Buffer,
            &RamdiskData,
            &RamdiskSize
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AndroidBootImgLocateFdt (Buffer, &FdtBase);
  if (EFI_ERROR (Status)) {
    FreePool (NewKernelArg);
    return Status;
  }

  Status = AndroidBootImgUpdateFdt (Buffer, FdtBase, RamdiskData, RamdiskSize);
  if (EFI_ERROR (Status)) {
    FreePool (NewKernelArg);
    return Status;
  }

  KernelDevicePath = mMemoryDevicePathTemplate;

  KernelDevicePath.Node1.StartingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) Kernel;
  KernelDevicePath.Node1.EndingAddress   = (EFI_PHYSICAL_ADDRESS)(UINTN) Kernel
                                           + KernelSize;

  Status = gBS->LoadImage (TRUE, gImageHandle,
                           (EFI_DEVICE_PATH *)&KernelDevicePath,
                           (VOID*)(UINTN)Kernel, KernelSize, &ImageHandle);
  if (EFI_ERROR (Status)) {
    //
    // With EFI_SECURITY_VIOLATION retval, the Image was loaded and an ImageHandle was created
    // with a valid EFI_LOADED_IMAGE_PROTOCOL, but the image can not be started right now.
    // If the caller doesn't have the option to defer the execution of an image, we should
    // unload image for the EFI_SECURITY_VIOLATION to avoid resource leak.
    //
    if (Status == EFI_SECURITY_VIOLATION) {
      gBS->UnloadImage (ImageHandle);
    }
    return Status;
  }

  // Set kernel arguments
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid,
                                (VOID **) &ImageInfo);
  ImageInfo->LoadOptions = NewKernelArg;
  ImageInfo->LoadOptionsSize = StrLen (NewKernelArg) * sizeof (CHAR16);

  // Before calling the image, enable the Watchdog Timer for  the 5 Minute period
  gBS->SetWatchdogTimer (5 * 60, 0x10000, 0, NULL);
  // Start the image
  Status = gBS->StartImage (ImageHandle, NULL, NULL);
  // Clear the Watchdog Timer if the image returns
  gBS->SetWatchdogTimer (0, 0x10000, 0, NULL);
  return EFI_SUCCESS;
}
