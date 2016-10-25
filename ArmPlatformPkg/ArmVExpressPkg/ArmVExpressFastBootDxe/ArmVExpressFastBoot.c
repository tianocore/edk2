/** @file

  Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/*
  Implementation of the Android Fastboot Platform protocol, to be used by the
  Fastboot UEFI application, for ARM Versatile Express platforms.
*/

#include <Protocol/AndroidFastbootPlatform.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define FLASH_DEVICE_PATH_SIZE(DevPath) ( GetDevicePathSize (DevPath) - \
                                            sizeof (EFI_DEVICE_PATH_PROTOCOL))

#define PARTITION_NAME_MAX_LENGTH 72/2

#define IS_ALPHA(Char) (((Char) <= L'z' && (Char) >= L'a') || \
                        ((Char) <= L'Z' && (Char) >= L'Z'))

typedef struct _FASTBOOT_PARTITION_LIST {
  LIST_ENTRY  Link;
  CHAR16      PartitionName[PARTITION_NAME_MAX_LENGTH];
  EFI_HANDLE  PartitionHandle;
} FASTBOOT_PARTITION_LIST;

STATIC LIST_ENTRY mPartitionListHead;

/*
  Helper to free the partition list
*/
STATIC
VOID
FreePartitionList (
  VOID
  )
{
  FASTBOOT_PARTITION_LIST *Entry;
  FASTBOOT_PARTITION_LIST *NextEntry;

  Entry = (FASTBOOT_PARTITION_LIST *) GetFirstNode (&mPartitionListHead);
  while (!IsNull (&mPartitionListHead, &Entry->Link)) {
    NextEntry = (FASTBOOT_PARTITION_LIST *) GetNextNode (&mPartitionListHead, &Entry->Link);

    RemoveEntryList (&Entry->Link);
    FreePool (Entry);

    Entry = NextEntry;
  }
}
/*
  Read the PartitionName fields from the GPT partition entries, putting them
  into an allocated array that should later be freed.
*/
STATIC
EFI_STATUS
ReadPartitionEntries (
  IN  EFI_BLOCK_IO_PROTOCOL *BlockIo,
  OUT EFI_PARTITION_ENTRY  **PartitionEntries
  )
{
  UINTN                       EntrySize;
  UINTN                       NumEntries;
  UINTN                       BufferSize;
  UINT32                      MediaId;
  EFI_PARTITION_TABLE_HEADER *GptHeader;
  EFI_STATUS                  Status;

  MediaId = BlockIo->Media->MediaId;

  //
  // Read size of Partition entry and number of entries from GPT header
  //

  GptHeader = AllocatePool (BlockIo->Media->BlockSize);
  if (GptHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BlockIo->ReadBlocks (BlockIo, MediaId, 1, BlockIo->Media->BlockSize, (VOID *) GptHeader);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check there is a GPT on the media
  if (GptHeader->Header.Signature != EFI_PTAB_HEADER_ID ||
      GptHeader->MyLBA != 1) {
    DEBUG ((EFI_D_ERROR,
      "Fastboot platform: No GPT on flash. "
      "Fastboot on Versatile Express does not support MBR.\n"
      ));
    return EFI_DEVICE_ERROR;
  }

  EntrySize = GptHeader->SizeOfPartitionEntry;
  NumEntries = GptHeader->NumberOfPartitionEntries;

  FreePool (GptHeader);

  ASSERT (EntrySize != 0);
  ASSERT (NumEntries != 0);

  BufferSize = ALIGN_VALUE (EntrySize * NumEntries, BlockIo->Media->BlockSize);
  *PartitionEntries = AllocatePool (BufferSize);
  if (PartitionEntries == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BlockIo->ReadBlocks (BlockIo, MediaId, 2, BufferSize, (VOID *) *PartitionEntries);
  if (EFI_ERROR (Status)) {
    FreePool (PartitionEntries);
    return Status;
  }

  return Status;
}


/*
  Do any initialisation that needs to be done in order to be able to respond to
  commands.

  @retval EFI_SUCCESS   Initialised successfully.
  @retval !EFI_SUCCESS  Error in initialisation.
*/
STATIC
EFI_STATUS
ArmFastbootPlatformInit (
  VOID
  )
{
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL           *FlashDevicePath;
  EFI_DEVICE_PATH_PROTOCOL           *FlashDevicePathDup;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL           *NextNode;
  HARDDRIVE_DEVICE_PATH              *PartitionNode;
  UINTN                               NumHandles;
  EFI_HANDLE                         *AllHandles;
  UINTN                               LoopIndex;
  EFI_HANDLE                          FlashHandle;
  EFI_BLOCK_IO_PROTOCOL              *FlashBlockIo;
  EFI_PARTITION_ENTRY                *PartitionEntries;
  FASTBOOT_PARTITION_LIST            *Entry;

  InitializeListHead (&mPartitionListHead);

  //
  // Get EFI_HANDLES for all the partitions on the block devices pointed to by
  // PcdFastbootFlashDevicePath, also saving their GPT partition labels.
  // We will use these labels as the key in ArmFastbootPlatformFlashPartition.
  // There's no way to find all of a device's children, so we get every handle
  // in the system supporting EFI_BLOCK_IO_PROTOCOL and then filter out ones
  // that don't represent partitions on the flash device.
  //

  FlashDevicePath = ConvertTextToDevicePath ((CHAR16*)FixedPcdGetPtr (PcdAndroidFastbootNvmDevicePath));

  //
  // Open the Disk IO protocol on the flash device - this will be used to read
  // partition names out of the GPT entries
  //
  // Create another device path pointer because LocateDevicePath will modify it.
  FlashDevicePathDup = FlashDevicePath;
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &FlashDevicePathDup, &FlashHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Warning: Couldn't locate Android NVM device (status: %r)\n", Status));
    // Failing to locate partitions should not prevent to do other Android FastBoot actions
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  FlashHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &FlashBlockIo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Fastboot platform: Couldn't open Android NVM device (status: %r)\n", Status));
    return EFI_DEVICE_ERROR;
  }

  // Read the GPT partition entry array into memory so we can get the partition names
  Status = ReadPartitionEntries (FlashBlockIo, &PartitionEntries);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Warning: Failed to read partitions from Android NVM device (status: %r)\n", Status));
    // Failing to locate partitions should not prevent to do other Android FastBoot actions
    return EFI_SUCCESS;
  }

  // Get every Block IO protocol instance installed in the system
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &NumHandles,
                  &AllHandles
                  );
  ASSERT_EFI_ERROR (Status);

  // Filter out handles that aren't children of the flash device
  for (LoopIndex = 0; LoopIndex < NumHandles; LoopIndex++) {
    // Get the device path for the handle
    Status = gBS->OpenProtocol (
                    AllHandles[LoopIndex],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &DevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT_EFI_ERROR (Status);

    // Check if it is a sub-device of the flash device
    if (!CompareMem (DevicePath, FlashDevicePath, FLASH_DEVICE_PATH_SIZE (FlashDevicePath))) {
      // Device path starts with path of flash device. Check it isn't the flash
      // device itself.
      NextNode = NextDevicePathNode (DevicePath);
      if (IsDevicePathEndType (NextNode)) {
        continue;
      }

      // Assert that this device path node represents a partition.
      ASSERT (NextNode->Type == MEDIA_DEVICE_PATH &&
              NextNode->SubType == MEDIA_HARDDRIVE_DP);

      PartitionNode = (HARDDRIVE_DEVICE_PATH *) NextNode;

      // Assert that the partition type is GPT. ReadPartitionEntries checks for
      // presence of a GPT, so we should never find MBR partitions.
      // ("MBRType" is a misnomer - this field is actually called "Partition
      //  Format")
      ASSERT (PartitionNode->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER);

      // The firmware may install a handle for "partition 0", representing the
      // whole device. Ignore it.
      if (PartitionNode->PartitionNumber == 0) {
        continue;
      }

      //
      // Add the partition handle to the list
      //

      // Create entry
      Entry = AllocatePool (sizeof (FASTBOOT_PARTITION_LIST));
      if (Entry == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        FreePartitionList ();
        goto Exit;
      }

      // Copy handle and partition name
      Entry->PartitionHandle = AllHandles[LoopIndex];
      CopyMem (
        Entry->PartitionName,
        PartitionEntries[PartitionNode->PartitionNumber - 1].PartitionName, // Partition numbers start from 1.
        PARTITION_NAME_MAX_LENGTH
        );
      InsertTailList (&mPartitionListHead, &Entry->Link);

      // Print a debug message if the partition label is empty or looks like
      // garbage.
      if (!IS_ALPHA (Entry->PartitionName[0])) {
        DEBUG ((EFI_D_ERROR,
          "Warning: Partition %d doesn't seem to have a GPT partition label. "
          "You won't be able to flash it with Fastboot.\n",
          PartitionNode->PartitionNumber
          ));
      }
    }
  }

Exit:
  FreePool (PartitionEntries);
  FreePool (FlashDevicePath);
  FreePool (AllHandles);
  return Status;

}

/*
  To be called when Fastboot is finished and we aren't rebooting or booting an
  image. Undo initialisation, free resrouces.
*/
STATIC
VOID
ArmFastbootPlatformUnInit (
  VOID
  )
{
  FreePartitionList ();
}

/*
  Flash the partition named (according to a platform-specific scheme)
  PartitionName, with the image pointed to by Buffer, whose size is BufferSize.

  @param[in] PartitionName  Null-terminated name of partition to write.
  @param[in] BufferSize     Size of Buffer in byets.
  @param[in] Buffer         Data to write to partition.

  @retval EFI_NOT_FOUND     No such partition.
  @retval EFI_DEVICE_ERROR  Flashing failed.
*/
STATIC
EFI_STATUS
ArmFastbootPlatformFlashPartition (
  IN CHAR8  *PartitionName,
  IN UINTN   Size,
  IN VOID   *Image
  )
{
  EFI_STATUS               Status;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  EFI_DISK_IO_PROTOCOL    *DiskIo;
  UINT32                   MediaId;
  UINTN                    PartitionSize;
  FASTBOOT_PARTITION_LIST *Entry;
  CHAR16                   PartitionNameUnicode[60];
  BOOLEAN                  PartitionFound;

  AsciiStrToUnicodeStrS (PartitionName, PartitionNameUnicode,
    ARRAY_SIZE (PartitionNameUnicode));

  PartitionFound = FALSE;
  Entry = (FASTBOOT_PARTITION_LIST *) GetFirstNode (&(mPartitionListHead));
  while (!IsNull (&mPartitionListHead, &Entry->Link)) {
    // Search the partition list for the partition named by PartitionName
    if (StrCmp (Entry->PartitionName, PartitionNameUnicode) == 0) {
      PartitionFound = TRUE;
      break;
    }

   Entry = (FASTBOOT_PARTITION_LIST *) GetNextNode (&mPartitionListHead, &(Entry)->Link);
  }
  if (!PartitionFound) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->OpenProtocol (
                  Entry->PartitionHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlockIo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Fastboot platform: couldn't open Block IO for flash: %r\n", Status));
    return EFI_NOT_FOUND;
  }

  // Check image will fit on device
  PartitionSize = (BlockIo->Media->LastBlock + 1) * BlockIo->Media->BlockSize;
  if (PartitionSize < Size) {
    DEBUG ((EFI_D_ERROR, "Partition not big enough.\n"));
    DEBUG ((EFI_D_ERROR, "Partition Size:\t%d\nImage Size:\t%d\n", PartitionSize, Size));

    return EFI_VOLUME_FULL;
  }

  MediaId = BlockIo->Media->MediaId;

  Status = gBS->OpenProtocol (
                  Entry->PartitionHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **) &DiskIo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = DiskIo->WriteDisk (DiskIo, MediaId, 0, Size, Image);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BlockIo->FlushBlocks(BlockIo);

  return Status;
}

/*
  Erase the partition named PartitionName.

  @param[in] PartitionName  Null-terminated name of partition to erase.

  @retval EFI_NOT_FOUND     No such partition.
  @retval EFI_DEVICE_ERROR  Erasing failed.
*/
STATIC
EFI_STATUS
ArmFastbootPlatformErasePartition (
  IN CHAR8 *Partition
  )
{
  return EFI_SUCCESS;
}

/*
  If the variable referred to by Name exists, copy it (as a null-terminated
  string) into Value. If it doesn't exist, put the Empty string in Value.

  Variable names and values may not be larger than 60 bytes, excluding the
  terminal null character. This is a limitation of the Fastboot protocol.

  The Fastboot application will handle platform-nonspecific variables
  (Currently "version" is the only one of these.)

  @param[in]  Name   Null-terminated name of Fastboot variable to retrieve.
  @param[out] Value  Caller-allocated buffer for null-terminated value of
                     variable.

  @retval EFI_SUCCESS       The variable was retrieved, or it doesn't exist.
  @retval EFI_DEVICE_ERROR  There was an error looking up the variable. This
                            does _not_ include the variable not existing.
*/
STATIC
EFI_STATUS
ArmFastbootPlatformGetVar (
  IN  CHAR8   *Name,
  OUT CHAR8   *Value
  )
{
  if (AsciiStrCmp (Name, "product")) {
    AsciiStrCpyS (Value, 61, FixedPcdGetPtr (PcdFirmwareVendor));
  } else {
    *Value = '\0';
  }
  return EFI_SUCCESS;
}

/*
  React to an OEM-specific command.

  Future versions of this function might want to allow the platform to do some
  extra communication with the host. A way to do this would be to add a function
  to the FASTBOOT_TRANSPORT_PROTOCOL that allows the implementation of
  DoOemCommand to replace the ReceiveEvent with its own, and to restore the old
  one when it's finished.

  However at the moment although the specification allows it, the AOSP fastboot
  host application doesn't handle receiving any data from the client, and it
  doesn't support a data phase for OEM commands.

  @param[in] Command    Null-terminated command string.

  @retval EFI_SUCCESS       The command executed successfully.
  @retval EFI_NOT_FOUND     The command wasn't recognised.
  @retval EFI_DEVICE_ERROR  There was an error executing the command.
*/
STATIC
EFI_STATUS
ArmFastbootPlatformOemCommand (
  IN  CHAR8   *Command
  )
{
  CHAR16 CommandUnicode[65];

  AsciiStrToUnicodeStrS (Command, CommandUnicode, ARRAY_SIZE (CommandUnicode));

  if (AsciiStrCmp (Command, "Demonstrate") == 0) {
    DEBUG ((EFI_D_ERROR, "ARM OEM Fastboot command 'Demonstrate' received.\n"));
    return EFI_SUCCESS;
  } else {
    DEBUG ((EFI_D_ERROR,
      "VExpress: Unrecognised Fastboot OEM command: %s\n",
      CommandUnicode
      ));
    return EFI_NOT_FOUND;
  }
}

STATIC FASTBOOT_PLATFORM_PROTOCOL mPlatformProtocol = {
  ArmFastbootPlatformInit,
  ArmFastbootPlatformUnInit,
  ArmFastbootPlatformFlashPartition,
  ArmFastbootPlatformErasePartition,
  ArmFastbootPlatformGetVar,
  ArmFastbootPlatformOemCommand
};

EFI_STATUS
EFIAPI
ArmAndroidFastbootPlatformEntryPoint (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  return gBS->InstallProtocolInterface (
                &ImageHandle,
                &gAndroidFastbootPlatformProtocolGuid,
                EFI_NATIVE_INTERFACE,
                &mPlatformProtocol
                );
}
