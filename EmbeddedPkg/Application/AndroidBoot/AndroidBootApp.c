/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2017, Linaro. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AndroidBootImgLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePathFromText.h>

/* Validate the node is media hard drive type */
EFI_STATUS
ValidateAndroidMediaDevicePath (
  IN EFI_DEVICE_PATH  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node, *NextNode;

  NextNode = DevicePath;
  while (NextNode != NULL) {
    Node = NextNode;
    if ((Node->Type == MEDIA_DEVICE_PATH) &&
        (Node->SubType == MEDIA_HARDDRIVE_DP))
    {
      return EFI_SUCCESS;
    }

    NextNode = NextDevicePathNode (Node);
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
AndroidBootAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                          Status;
  CHAR16                              *BootPathStr;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;
  EFI_DEVICE_PATH                     *DevicePath;
  EFI_BLOCK_IO_PROTOCOL               *BlockIo;
  UINT32                              MediaId, BlockSize;
  VOID                                *Buffer;
  EFI_HANDLE                          Handle;
  UINTN                               BootImgSize;

  BootPathStr = (CHAR16 *)PcdGetPtr (PcdAndroidBootDevicePath);
  ASSERT (BootPathStr != NULL);
  Status = gBS->LocateProtocol (
                  &gEfiDevicePathFromTextProtocolGuid,
                  NULL,
                  (VOID **)&EfiDevicePathFromTextProtocol
                  );
  ASSERT_EFI_ERROR (Status);
  DevicePath = (EFI_DEVICE_PATH *)EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (BootPathStr);
  ASSERT (DevicePath != NULL);

  Status = ValidateAndroidMediaDevicePath (DevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &DevicePath,
                  &Handle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get BlockIo: %r\n", Status));
    return Status;
  }

  MediaId   = BlockIo->Media->MediaId;
  BlockSize = BlockIo->Media->BlockSize;
  Buffer    = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (ANDROID_BOOTIMG_HEADER)));
  if (Buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  /* Load header of boot.img */
  Status = BlockIo->ReadBlocks (
                      BlockIo,
                      MediaId,
                      0,
                      BlockSize,
                      Buffer
                      );
  Status = AndroidBootImgGetImgSize (Buffer, &BootImgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get AndroidBootImg Size: %r\n", Status));
    return Status;
  }

  BootImgSize = ALIGN_VALUE (BootImgSize, BlockSize);
  FreePages (Buffer, EFI_SIZE_TO_PAGES (sizeof (ANDROID_BOOTIMG_HEADER)));

  /* Both PartitionStart and PartitionSize are counted as block size. */
  Buffer = AllocatePages (EFI_SIZE_TO_PAGES (BootImgSize));
  if (Buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  /* Load header of boot.img */
  Status = BlockIo->ReadBlocks (
                      BlockIo,
                      MediaId,
                      0,
                      BootImgSize,
                      Buffer
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to read blocks: %r\n", Status));
    goto EXIT;
  }

  Status = AndroidBootImgBoot (Buffer, BootImgSize);

EXIT:
  return Status;
}
