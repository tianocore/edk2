/** @file
  Library functions which relates with booting.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalBm.h"

#define VENDOR_IDENTIFICATION_OFFSET     3
#define VENDOR_IDENTIFICATION_LENGTH     8
#define PRODUCT_IDENTIFICATION_OFFSET    11
#define PRODUCT_IDENTIFICATION_LENGTH    16

CONST UINT16 mBmUsbLangId    = 0x0409; // English
CHAR16       mBmUefiPrefix[] = L"UEFI ";

EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION  mBmRefreshLegacyBootOption = NULL;
EFI_BOOT_MANAGER_LEGACY_BOOT                 mBmLegacyBoot              = NULL;

LIST_ENTRY mPlatformBootDescriptionHandlers = INITIALIZE_LIST_HEAD_VARIABLE (mPlatformBootDescriptionHandlers);

///
/// This GUID is used for an EFI Variable that stores the front device pathes
/// for a partial device path that starts with the HD node.
///
EFI_GUID mBmHardDriveBootVariableGuid = { 0xfab7e9e1, 0x39dd, 0x4f2b, { 0x84, 0x08, 0xe2, 0x0e, 0x90, 0x6c, 0xb6, 0xde } };
EFI_GUID mBmAutoCreateBootOptionGuid  = { 0x8108ac4e, 0x9f11, 0x4d59, { 0x85, 0x0e, 0xe2, 0x1a, 0x52, 0x2c, 0x59, 0xb2 } };

/**
  The function registers the legacy boot support capabilities.

  @param RefreshLegacyBootOption The function pointer to create all the legacy boot options.
  @param LegacyBoot              The function pointer to boot the legacy boot option.
**/
VOID
EFIAPI
EfiBootManagerRegisterLegacyBootSupport (
  EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION   RefreshLegacyBootOption,
  EFI_BOOT_MANAGER_LEGACY_BOOT                  LegacyBoot
  )
{
  mBmRefreshLegacyBootOption = RefreshLegacyBootOption;
  mBmLegacyBoot              = LegacyBoot;
}

/**
  For a bootable Device path, return its boot type.

  @param  DevicePath                   The bootable device Path to check

  @retval AcpiFloppyBoot               If given device path contains ACPI_DEVICE_PATH type device path node
                                       which HID is floppy device.
  @retval MessageAtapiBoot             If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_ATAPI_DP.
  @retval MessageSataBoot              If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_SATA_DP.
  @retval MessageScsiBoot              If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_SCSI_DP.
  @retval MessageUsbBoot               If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_USB_DP.
  @retval MessageNetworkBoot           If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_MAC_ADDR_DP, MSG_VLAN_DP,
                                       MSG_IPv4_DP or MSG_IPv6_DP.
  @retval UnsupportedBoot              If tiven device path doesn't match the above condition, it's not supported.

**/
BM_BOOT_TYPE
BmDevicePathType (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *Node;
  EFI_DEVICE_PATH_PROTOCOL      *NextNode;

  ASSERT (DevicePath != NULL);

  for (Node = DevicePath; !IsDevicePathEndType (Node); Node = NextDevicePathNode (Node)) {
    switch (DevicePathType (Node)) {

      case ACPI_DEVICE_PATH:
        if (EISA_ID_TO_NUM (((ACPI_HID_DEVICE_PATH *) Node)->HID) == 0x0604) {
          return BmAcpiFloppyBoot;
        }
        break;

      case HARDWARE_DEVICE_PATH:
        if (DevicePathSubType (Node) == HW_CONTROLLER_DP) {
          return BmHardwareDeviceBoot;
        }
        break;

      case MESSAGING_DEVICE_PATH:
        //
        // Skip LUN device node
        //
        NextNode = Node;
        do {
          NextNode = NextDevicePathNode (NextNode);
        } while (
            (DevicePathType (NextNode) == MESSAGING_DEVICE_PATH) &&
            (DevicePathSubType(NextNode) == MSG_DEVICE_LOGICAL_UNIT_DP)
            );

        //
        // If the device path not only point to driver device, it is not a messaging device path,
        //
        if (!IsDevicePathEndType (NextNode)) {
          break;
        }

        switch (DevicePathSubType (Node)) {
        case MSG_ATAPI_DP:
          return BmMessageAtapiBoot;
          break;

        case MSG_SATA_DP:
          return BmMessageSataBoot;
          break;

        case MSG_USB_DP:
          return BmMessageUsbBoot;
          break;

        case MSG_SCSI_DP:
          return BmMessageScsiBoot;
          break;

        case MSG_MAC_ADDR_DP:
        case MSG_VLAN_DP:
        case MSG_IPv4_DP:
        case MSG_IPv6_DP:
          return BmMessageNetworkBoot;
          break;
        }
    }
  }

  return BmMiscBoot;
}

/**
  Find the boot option in the NV storage and return the option number.

  @param OptionToFind  Boot option to be checked.

  @return   The option number of the found boot option.

**/
UINTN
BmFindBootOptionInVariable (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION             *OptionToFind
  )
{
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_LOAD_OPTION BootOption;
  UINTN                        OptionNumber;
  CHAR16                       OptionName[BM_OPTION_NAME_LEN];
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOptions;
  UINTN                        BootOptionCount;
  UINTN                        Index;
  
  OptionNumber = LoadOptionNumberUnassigned;

  //
  // Try to match the variable exactly if the option number is assigned
  //
  if (OptionToFind->OptionNumber != LoadOptionNumberUnassigned) {
    UnicodeSPrint (
      OptionName, sizeof (OptionName), L"%s%04x",
      mBmLoadOptionName[OptionToFind->OptionType], OptionToFind->OptionNumber
      );
    Status = EfiBootManagerVariableToLoadOption (OptionName, &BootOption);

    if (!EFI_ERROR (Status)) {
      ASSERT (OptionToFind->OptionNumber == BootOption.OptionNumber);
      if ((OptionToFind->Attributes == BootOption.Attributes) &&
          (StrCmp (OptionToFind->Description, BootOption.Description) == 0) &&
          (CompareMem (OptionToFind->FilePath, BootOption.FilePath, GetDevicePathSize (OptionToFind->FilePath)) == 0) &&
          (OptionToFind->OptionalDataSize == BootOption.OptionalDataSize) &&
          (CompareMem (OptionToFind->OptionalData, BootOption.OptionalData, OptionToFind->OptionalDataSize) == 0)
         ) {
        OptionNumber = OptionToFind->OptionNumber;
      }
      EfiBootManagerFreeLoadOption (&BootOption);
    }
  }

  //
  // The option number assigned is either incorrect or unassigned.
  //
  if (OptionNumber == LoadOptionNumberUnassigned) {
    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

    Index = BmFindLoadOption (OptionToFind, BootOptions, BootOptionCount);
    if (Index != -1) {
      OptionNumber = BootOptions[Index].OptionNumber;
    }

    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  }

  return OptionNumber;
}

/**
  Get the file buffer using a Memory Mapped Device Path.

  FV address may change across reboot. This routine promises the FV file device path is right.

  @param  DevicePath   The Memory Mapped Device Path to get the file buffer.
  @param  FullPath     Receive the updated FV Device Path pointint to the file.
  @param  FileSize     Receive the file buffer size.

  @return  The file buffer.
**/
VOID *
BmGetFileBufferByMemmapFv (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL     **FullPath,
  OUT UINTN                        *FileSize
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  EFI_DEVICE_PATH_PROTOCOL      *FvFileNode;
  EFI_HANDLE                    FvHandle;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  UINT32                        AuthenticationStatus;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandles;
  EFI_DEVICE_PATH_PROTOCOL      *NewDevicePath;
  VOID                          *FileBuffer;
  
  FvFileNode = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &FvFileNode, &FvHandle);
  if (!EFI_ERROR (Status)) {
    FileBuffer = GetFileBufferByFilePath (TRUE, DevicePath, FileSize, &AuthenticationStatus);
    if (FileBuffer != NULL) {
      *FullPath = DuplicateDevicePath (DevicePath);
    }
    return FileBuffer;
  }

  FvFileNode = NextDevicePathNode (DevicePath);

  //
  // Firstly find the FV file in current FV
  //
  gBS->HandleProtocol (
         gImageHandle,
         &gEfiLoadedImageProtocolGuid,
         (VOID **) &LoadedImage
         );
  NewDevicePath = AppendDevicePathNode (DevicePathFromHandle (LoadedImage->DeviceHandle), FvFileNode);
  FileBuffer = BmGetFileBufferByMemmapFv (NewDevicePath, FullPath, FileSize);
  FreePool (NewDevicePath);

  if (FileBuffer != NULL) {
    return FileBuffer;
  }

  //
  // Secondly find the FV file in all other FVs
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiFirmwareVolume2ProtocolGuid,
         NULL,
         &FvHandleCount,
         &FvHandles
         );
  for (Index = 0; (Index < FvHandleCount) && (FileBuffer == NULL); Index++) {
    if (FvHandles[Index] == LoadedImage->DeviceHandle) {
      //
      // Skip current FV
      //
      continue;
    }
    NewDevicePath = AppendDevicePathNode (DevicePathFromHandle (FvHandles[Index]), FvFileNode);
    FileBuffer = BmGetFileBufferByMemmapFv (NewDevicePath, FullPath, FileSize);
    FreePool (NewDevicePath);
  }
  
  if (FvHandles != NULL) {
    FreePool (FvHandles);
  }
  return FileBuffer;
}

/**
  Check if it's a Memory Mapped FV Device Path.
  
  The function doesn't garentee the device path points to existing FV file.

  @param  DevicePath     Input device path.

  @retval TRUE   The device path is a Memory Mapped FV Device Path.
  @retval FALSE  The device path is NOT a Memory Mapped FV Device Path.
**/
BOOLEAN
BmIsMemmapFvFilePath (
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL   *FileNode;

  if ((DevicePathType (DevicePath) == HARDWARE_DEVICE_PATH) && (DevicePathSubType (DevicePath) == HW_MEMMAP_DP)) {
    FileNode = NextDevicePathNode (DevicePath);
    if ((DevicePathType (FileNode) == MEDIA_DEVICE_PATH) && (DevicePathSubType (FileNode) == MEDIA_PIWG_FW_FILE_DP)) {
      return IsDevicePathEnd (NextDevicePathNode (FileNode));
    }
  }

  return FALSE;
}

/**
  Check whether a USB device match the specified USB Class device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbClass    The USB Class device path to match.

  @retval TRUE       The USB device match the USB Class device path.
  @retval FALSE      The USB device does not match the USB Class device path.

**/
BOOLEAN
BmMatchUsbClass (
  IN EFI_USB_IO_PROTOCOL        *UsbIo,
  IN USB_CLASS_DEVICE_PATH      *UsbClass
  )
{
  EFI_STATUS                    Status;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IfDesc;
  UINT8                         DeviceClass;
  UINT8                         DeviceSubClass;
  UINT8                         DeviceProtocol;

  if ((DevicePathType (UsbClass) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbClass) != MSG_USB_CLASS_DP)){
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((UsbClass->VendorId != 0xffff) &&
      (UsbClass->VendorId != DevDesc.IdVendor)) {
    return FALSE;
  }

  if ((UsbClass->ProductId != 0xffff) &&
      (UsbClass->ProductId != DevDesc.IdProduct)) {
    return FALSE;
  }

  DeviceClass    = DevDesc.DeviceClass;
  DeviceSubClass = DevDesc.DeviceSubClass;
  DeviceProtocol = DevDesc.DeviceProtocol;
  if (DeviceClass == 0) {
    //
    // If Class in Device Descriptor is set to 0, use the Class, SubClass and
    // Protocol in Interface Descriptor instead.
    //
    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    DeviceClass    = IfDesc.InterfaceClass;
    DeviceSubClass = IfDesc.InterfaceSubClass;
    DeviceProtocol = IfDesc.InterfaceProtocol;
  }

  //
  // Check Class, SubClass and Protocol.
  //
  if ((UsbClass->DeviceClass != 0xff) &&
      (UsbClass->DeviceClass != DeviceClass)) {
    return FALSE;
  }

  if ((UsbClass->DeviceSubClass != 0xff) &&
      (UsbClass->DeviceSubClass != DeviceSubClass)) {
    return FALSE;
  }

  if ((UsbClass->DeviceProtocol != 0xff) &&
      (UsbClass->DeviceProtocol != DeviceProtocol)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Eliminate the extra spaces in the Str to one space.

  @param    Str     Input string info.
**/
VOID
BmEliminateExtraSpaces (
  IN CHAR16                    *Str
  )
{
  UINTN                        Index;
  UINTN                        ActualIndex;

  for (Index = 0, ActualIndex = 0; Str[Index] != L'\0'; Index++) {
    if ((Str[Index] != L' ') || ((ActualIndex > 0) && (Str[ActualIndex - 1] != L' '))) {
      Str[ActualIndex++] = Str[Index];
    }
  }
  Str[ActualIndex] = L'\0';
}

/**
  Try to get the controller's ATA/ATAPI description.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetDescriptionFromDiskInfo (
  IN EFI_HANDLE                Handle
  )
{
  UINTN                        Index;
  EFI_STATUS                   Status;
  EFI_DISK_INFO_PROTOCOL       *DiskInfo;
  UINT32                       BufferSize;
  EFI_ATAPI_IDENTIFY_DATA      IdentifyData;
  EFI_SCSI_INQUIRY_DATA        InquiryData;
  CHAR16                       *Description;
  UINTN                        Length;
  CONST UINTN                  ModelNameLength    = 40;
  CONST UINTN                  SerialNumberLength = 20;
  CHAR8                        *StrPtr;
  UINT8                        Temp;

  Description  = NULL;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID **) &DiskInfo
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid) || 
      CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid)) {
    BufferSize   = sizeof (EFI_ATAPI_IDENTIFY_DATA);
    Status = DiskInfo->Identify (
                         DiskInfo,
                         &IdentifyData,
                         &BufferSize
                         );
    if (!EFI_ERROR (Status)) {
      Description = AllocateZeroPool ((ModelNameLength + SerialNumberLength + 2) * sizeof (CHAR16));
      ASSERT (Description != NULL);
      for (Index = 0; Index + 1 < ModelNameLength; Index += 2) {
        Description[Index]     = (CHAR16) IdentifyData.ModelName[Index + 1];
        Description[Index + 1] = (CHAR16) IdentifyData.ModelName[Index];
      }

      Length = Index;
      Description[Length++] = L' ';

      for (Index = 0; Index + 1 < SerialNumberLength; Index += 2) {
        Description[Length + Index]     = (CHAR16) IdentifyData.SerialNo[Index + 1];
        Description[Length + Index + 1] = (CHAR16) IdentifyData.SerialNo[Index];
      }
      Length += Index;
      Description[Length++] = L'\0';
      ASSERT (Length == ModelNameLength + SerialNumberLength + 2);

      BmEliminateExtraSpaces (Description);
    }
  } else if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoScsiInterfaceGuid)) {
    BufferSize   = sizeof (EFI_SCSI_INQUIRY_DATA);
    Status = DiskInfo->Inquiry (
                         DiskInfo,
                         &InquiryData,
                         &BufferSize
                         );
    if (!EFI_ERROR (Status)) {
      Description = AllocateZeroPool ((VENDOR_IDENTIFICATION_LENGTH + PRODUCT_IDENTIFICATION_LENGTH + 2) * sizeof (CHAR16));
      ASSERT (Description != NULL);

      //
      // Per SCSI spec, EFI_SCSI_INQUIRY_DATA.Reserved_5_95[3 - 10] save the Verdor identification
      // EFI_SCSI_INQUIRY_DATA.Reserved_5_95[11 - 26] save the product identification, 
      // Here combine the vendor identification and product identification to the description.
      //
      StrPtr = (CHAR8 *) (&InquiryData.Reserved_5_95[VENDOR_IDENTIFICATION_OFFSET]);
      Temp = StrPtr[VENDOR_IDENTIFICATION_LENGTH];
      StrPtr[VENDOR_IDENTIFICATION_LENGTH] = '\0';
      AsciiStrToUnicodeStr (StrPtr, Description);
      StrPtr[VENDOR_IDENTIFICATION_LENGTH] = Temp;

      //
      // Add one space at the middle of vendor information and product information.
      //
      Description[VENDOR_IDENTIFICATION_LENGTH] = L' ';

      StrPtr = (CHAR8 *) (&InquiryData.Reserved_5_95[PRODUCT_IDENTIFICATION_OFFSET]);
      StrPtr[PRODUCT_IDENTIFICATION_LENGTH] = '\0';
      AsciiStrToUnicodeStr (StrPtr, Description + VENDOR_IDENTIFICATION_LENGTH + 1);

      BmEliminateExtraSpaces (Description);
    }
  }

  return Description;
}

/**
  Try to get the controller's USB description.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetUsbDescription (
  IN EFI_HANDLE                Handle
  )
{
  EFI_STATUS                   Status;
  EFI_USB_IO_PROTOCOL          *UsbIo;
  CHAR16                       NullChar;
  CHAR16                       *Manufacturer;
  CHAR16                       *Product;
  CHAR16                       *SerialNumber;
  CHAR16                       *Description;
  EFI_USB_DEVICE_DESCRIPTOR    DevDesc;
  UINTN                        DescMaxSize;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  NullChar = L'\0';

  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    mBmUsbLangId,
                    DevDesc.StrManufacturer,
                    &Manufacturer
                    );
  if (EFI_ERROR (Status)) {
    Manufacturer = &NullChar;
  }
  
  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    mBmUsbLangId,
                    DevDesc.StrProduct,
                    &Product
                    );
  if (EFI_ERROR (Status)) {
    Product = &NullChar;
  }
  
  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    mBmUsbLangId,
                    DevDesc.StrSerialNumber,
                    &SerialNumber
                    );
  if (EFI_ERROR (Status)) {
    SerialNumber = &NullChar;
  }

  if ((Manufacturer == &NullChar) &&
      (Product == &NullChar) &&
      (SerialNumber == &NullChar)
      ) {
    return NULL;
  }

  DescMaxSize = StrSize (Manufacturer) + StrSize (Product) + StrSize (SerialNumber);
  Description = AllocateZeroPool (DescMaxSize);
  ASSERT (Description != NULL);
  StrCatS (Description, DescMaxSize/sizeof(CHAR16), Manufacturer);
  StrCatS (Description, DescMaxSize/sizeof(CHAR16), L" ");

  StrCatS (Description, DescMaxSize/sizeof(CHAR16), Product);  
  StrCatS (Description, DescMaxSize/sizeof(CHAR16), L" ");

  StrCatS (Description, DescMaxSize/sizeof(CHAR16), SerialNumber);

  if (Manufacturer != &NullChar) {
    FreePool (Manufacturer);
  }
  if (Product != &NullChar) {
    FreePool (Product);
  }
  if (SerialNumber != &NullChar) {
    FreePool (SerialNumber);
  }

  BmEliminateExtraSpaces (Description);

  return Description;
}

/**
  Return the boot description for the controller based on the type.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetMiscDescription (
  IN EFI_HANDLE                  Handle
  )
{
  EFI_STATUS                      Status;
  CHAR16                          *Description;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;

  switch (BmDevicePathType (DevicePathFromHandle (Handle))) {
  case BmAcpiFloppyBoot:
    Description = L"Floppy";
    break;

  case BmMessageAtapiBoot:
  case BmMessageSataBoot:
    Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
    ASSERT_EFI_ERROR (Status);
    //
    // Assume a removable SATA device should be the DVD/CD device
    //
    Description = BlockIo->Media->RemovableMedia ? L"DVD/CDROM" : L"Hard Drive";
    break;

  case BmMessageUsbBoot:
    Description = L"USB Device";
    break;

  case BmMessageScsiBoot:
    Description = L"SCSI Device";
    break;

  case BmHardwareDeviceBoot:
    Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
    if (!EFI_ERROR (Status)) {
      Description = BlockIo->Media->RemovableMedia ? L"Removable Disk" : L"Hard Drive";
    } else {
      Description = L"Misc Device";
    }
    break;

  case BmMessageNetworkBoot:
    Description = L"Network";
    break;

  default:
    Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **) &Fs);
    if (!EFI_ERROR (Status)) {
      Description = L"Non-Block Boot Device";
    } else {
      Description = L"Misc Device";
    }
    break;
  }

  return AllocateCopyPool (StrSize (Description), Description);
}

/**
  Register the platform provided boot description handler.

  @param Handler  The platform provided boot description handler

  @retval EFI_SUCCESS          The handler was registered successfully.
  @retval EFI_ALREADY_STARTED  The handler was already registered.
  @retval EFI_OUT_OF_RESOURCES There is not enough resource to perform the registration.
**/
EFI_STATUS
EFIAPI
EfiBootManagerRegisterBootDescriptionHandler (
  IN EFI_BOOT_MANAGER_BOOT_DESCRIPTION_HANDLER  Handler
  )
{
  LIST_ENTRY                                    *Link;
  BM_BOOT_DESCRIPTION_ENTRY                    *Entry;

  for ( Link = GetFirstNode (&mPlatformBootDescriptionHandlers)
      ; !IsNull (&mPlatformBootDescriptionHandlers, Link)
      ; Link = GetNextNode (&mPlatformBootDescriptionHandlers, Link)
      ) {
    Entry = CR (Link, BM_BOOT_DESCRIPTION_ENTRY, Link, BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE);
    if (Entry->Handler == Handler) {
      return EFI_ALREADY_STARTED;
    }
  }

  Entry = AllocatePool (sizeof (BM_BOOT_DESCRIPTION_ENTRY));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Signature = BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE;
  Entry->Handler   = Handler;
  InsertTailList (&mPlatformBootDescriptionHandlers, &Entry->Link);
  return EFI_SUCCESS;
}

BM_GET_BOOT_DESCRIPTION mBmBootDescriptionHandlers[] = {
  BmGetUsbDescription,
  BmGetDescriptionFromDiskInfo,
  BmGetMiscDescription
};

/**
  Return the boot description for the controller.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetBootDescription (
  IN EFI_HANDLE                  Handle
  )
{
  LIST_ENTRY                     *Link;
  BM_BOOT_DESCRIPTION_ENTRY      *Entry;
  CHAR16                         *Description;
  CHAR16                         *DefaultDescription;
  CHAR16                         *Temp;
  UINTN                          Index;

  //
  // Firstly get the default boot description
  //
  DefaultDescription = NULL;
  for (Index = 0; Index < sizeof (mBmBootDescriptionHandlers) / sizeof (mBmBootDescriptionHandlers[0]); Index++) {
    DefaultDescription = mBmBootDescriptionHandlers[Index] (Handle);
    if (DefaultDescription != NULL) {
      //
      // Avoid description confusion between UEFI & Legacy boot option by adding "UEFI " prefix
      // ONLY for core provided boot description handler.
      //
      Temp = AllocatePool (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix)); 
      ASSERT (Temp != NULL);
      StrCpyS ( Temp, 
                (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix))/sizeof(CHAR16), 
                mBmUefiPrefix
                );
      StrCatS ( Temp, 
                (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix))/sizeof(CHAR16), 
                DefaultDescription
                );
      FreePool (DefaultDescription);
      DefaultDescription = Temp;
      break;
    }
  }
  ASSERT (DefaultDescription != NULL);

  //
  // Secondly query platform for the better boot description
  //
  for ( Link = GetFirstNode (&mPlatformBootDescriptionHandlers)
      ; !IsNull (&mPlatformBootDescriptionHandlers, Link)
      ; Link = GetNextNode (&mPlatformBootDescriptionHandlers, Link)
      ) {
    Entry = CR (Link, BM_BOOT_DESCRIPTION_ENTRY, Link, BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE);
    Description = Entry->Handler (Handle, DefaultDescription);
    if (Description != NULL) {
      FreePool (DefaultDescription);
      return Description;
    }
  }

  return DefaultDescription;
}

/**
  Check whether a USB device match the specified USB WWID device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbWwid     The USB WWID device path to match.

  @retval TRUE       The USB device match the USB WWID device path.
  @retval FALSE      The USB device does not match the USB WWID device path.

**/
BOOLEAN
BmMatchUsbWwid (
  IN EFI_USB_IO_PROTOCOL        *UsbIo,
  IN USB_WWID_DEVICE_PATH       *UsbWwid
  )
{
  EFI_STATUS                   Status;
  EFI_USB_DEVICE_DESCRIPTOR    DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR IfDesc;
  UINT16                       *LangIdTable;
  UINT16                       TableSize;
  UINT16                       Index;
  CHAR16                       *CompareStr;
  UINTN                        CompareLen;
  CHAR16                       *SerialNumberStr;
  UINTN                        Length;

  if ((DevicePathType (UsbWwid) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbWwid) != MSG_USB_WWID_DP)) {
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  if ((DevDesc.IdVendor != UsbWwid->VendorId) ||
      (DevDesc.IdProduct != UsbWwid->ProductId)) {
    return FALSE;
  }

  //
  // Check Interface Number.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  if (IfDesc.InterfaceNumber != UsbWwid->InterfaceNumber) {
    return FALSE;
  }

  //
  // Check Serial Number.
  //
  if (DevDesc.StrSerialNumber == 0) {
    return FALSE;
  }

  //
  // Get all supported languages.
  //
  TableSize = 0;
  LangIdTable = NULL;
  Status = UsbIo->UsbGetSupportedLanguages (UsbIo, &LangIdTable, &TableSize);
  if (EFI_ERROR (Status) || (TableSize == 0) || (LangIdTable == NULL)) {
    return FALSE;
  }

  //
  // Serial number in USB WWID device path is the last 64-or-less UTF-16 characters.
  //
  CompareStr = (CHAR16 *) (UINTN) (UsbWwid + 1);
  CompareLen = (DevicePathNodeLength (UsbWwid) - sizeof (USB_WWID_DEVICE_PATH)) / sizeof (CHAR16);
  if (CompareStr[CompareLen - 1] == L'\0') {
    CompareLen--;
  }

  //
  // Compare serial number in each supported language.
  //
  for (Index = 0; Index < TableSize / sizeof (UINT16); Index++) {
    SerialNumberStr = NULL;
    Status = UsbIo->UsbGetStringDescriptor (
                      UsbIo,
                      LangIdTable[Index],
                      DevDesc.StrSerialNumber,
                      &SerialNumberStr
                      );
    if (EFI_ERROR (Status) || (SerialNumberStr == NULL)) {
      continue;
    }

    Length = StrLen (SerialNumberStr);
    if ((Length >= CompareLen) &&
        (CompareMem (SerialNumberStr + Length - CompareLen, CompareStr, CompareLen * sizeof (CHAR16)) == 0)) {
      FreePool (SerialNumberStr);
      return TRUE;
    }

    FreePool (SerialNumberStr);
  }

  return FALSE;
}

/**
  Find a USB device which match the specified short-form device path start with 
  USB Class or USB WWID device path. If ParentDevicePath is NULL, this function
  will search in all USB devices of the platform. If ParentDevicePath is not NULL,
  this function will only search in its child devices.

  @param DevicePath           The device path that contains USB Class or USB WWID device path.
  @param ParentDevicePathSize The length of the device path before the USB Class or 
                              USB WWID device path.
  @param UsbIoHandleCount     A pointer to the count of the returned USB IO handles.

  @retval NULL       The matched USB IO handles cannot be found.
  @retval other      The matched USB IO handles.

**/
EFI_HANDLE *
BmFindUsbDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  UINTN                     ParentDevicePathSize,
  OUT UINTN                     *UsbIoHandleCount
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *UsbIoHandles;
  EFI_DEVICE_PATH_PROTOCOL  *UsbIoDevicePath;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINTN                     Index;
  UINTN                     UsbIoDevicePathSize;
  BOOLEAN                   Matched;

  ASSERT (UsbIoHandleCount != NULL);  

  //
  // Get all UsbIo Handles.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  UsbIoHandleCount,
                  &UsbIoHandles
                  );
  if (EFI_ERROR (Status)) {
    *UsbIoHandleCount = 0;
    UsbIoHandles      = NULL;
  }

  for (Index = 0; Index < *UsbIoHandleCount; ) {
    //
    // Get the Usb IO interface.
    //
    Status = gBS->HandleProtocol(
                    UsbIoHandles[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **) &UsbIo
                    );
    UsbIoDevicePath = DevicePathFromHandle (UsbIoHandles[Index]);
    Matched         = FALSE;
    if (!EFI_ERROR (Status) && (UsbIoDevicePath != NULL)) {
      UsbIoDevicePathSize = GetDevicePathSize (UsbIoDevicePath) - END_DEVICE_PATH_LENGTH;

      //
      // Compare starting part of UsbIoHandle's device path with ParentDevicePath.
      //
      if (CompareMem (UsbIoDevicePath, DevicePath, ParentDevicePathSize) == 0) {
        if (BmMatchUsbClass (UsbIo, (USB_CLASS_DEVICE_PATH *) ((UINTN) DevicePath + ParentDevicePathSize)) ||
            BmMatchUsbWwid (UsbIo, (USB_WWID_DEVICE_PATH *) ((UINTN) DevicePath + ParentDevicePathSize))) {
          Matched = TRUE;
        }
      }
    }

    if (!Matched) {
      (*UsbIoHandleCount) --;
      CopyMem (&UsbIoHandles[Index], &UsbIoHandles[Index + 1], (*UsbIoHandleCount - Index) * sizeof (EFI_HANDLE));
    } else {
      Index++;
    }
  }

  return UsbIoHandles;
}

/**
  Expand USB Class or USB WWID device path node to be full device path of a USB
  device in platform.

  This function support following 4 cases:
  1) Boot Option device path starts with a USB Class or USB WWID device path,
     and there is no Media FilePath device path in the end.
     In this case, it will follow Removable Media Boot Behavior.
  2) Boot Option device path starts with a USB Class or USB WWID device path,
     and ended with Media FilePath device path.
  3) Boot Option device path starts with a full device path to a USB Host Controller,
     contains a USB Class or USB WWID device path node, while not ended with Media
     FilePath device path. In this case, it will follow Removable Media Boot Behavior.
  4) Boot Option device path starts with a full device path to a USB Host Controller,
     contains a USB Class or USB WWID device path node, and ended with Media
     FilePath device path.

  @param FilePath      The device path pointing to a load option.
                       It could be a short-form device path.
  @param FullPath      Return the full device path of the load option after
                       short-form device path expanding.
                       Caller is responsible to free it.
  @param FileSize      Return the load option size.
  @param ShortformNode Pointer to the USB short-form device path node in the FilePath buffer.

  @return The load option buffer. Caller is responsible to free the memory.
**/
VOID *
BmExpandUsbDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FullPath,
  OUT UINTN                     *FileSize,
  IN EFI_DEVICE_PATH_PROTOCOL   *ShortformNode
  )
{
  UINTN                             ParentDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          *FullDevicePath;
  EFI_HANDLE                        *Handles;
  UINTN                             HandleCount;
  UINTN                             Index;
  VOID                              *FileBuffer;

  ParentDevicePathSize = (UINTN) ShortformNode - (UINTN) FilePath;
  RemainingDevicePath = NextDevicePathNode (ShortformNode);
  FileBuffer = NULL;
  Handles = BmFindUsbDevice (FilePath, ParentDevicePathSize, &HandleCount);

  for (Index = 0; (Index < HandleCount) && (FileBuffer == NULL); Index++) {
    FullDevicePath = AppendDevicePath (DevicePathFromHandle (Handles[Index]), RemainingDevicePath);
    FileBuffer = BmGetLoadOptionBuffer (FullDevicePath, FullPath, FileSize);
    FreePool (FullDevicePath);
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  return FileBuffer;
}

/**
  Save the partition DevicePath to the CachedDevicePath as the first instance.

  @param CachedDevicePath  The device path cache.
  @param DevicePath        The partition device path to be cached.
**/
VOID
BmCachePartitionDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL **CachedDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;
  UINTN                           Count;
  
  if (BmMatchDevicePaths (*CachedDevicePath, DevicePath)) {
    TempDevicePath = *CachedDevicePath;
    *CachedDevicePath = BmDelPartMatchInstance (*CachedDevicePath, DevicePath);
    FreePool (TempDevicePath);
  }

  if (*CachedDevicePath == NULL) {
    *CachedDevicePath = DuplicateDevicePath (DevicePath);
    return;
  }

  TempDevicePath = *CachedDevicePath;
  *CachedDevicePath = AppendDevicePathInstance (DevicePath, *CachedDevicePath);
  if (TempDevicePath != NULL) {
    FreePool (TempDevicePath);
  }

  //
  // Here limit the device path instance number to 12, which is max number for a system support 3 IDE controller
  // If the user try to boot many OS in different HDs or partitions, in theory, the 'HDDP' variable maybe become larger and larger.
  //
  Count = 0;
  TempDevicePath = *CachedDevicePath;
  while (!IsDevicePathEnd (TempDevicePath)) {
    TempDevicePath = NextDevicePathNode (TempDevicePath);
    //
    // Parse one instance
    //
    while (!IsDevicePathEndType (TempDevicePath)) {
      TempDevicePath = NextDevicePathNode (TempDevicePath);
    }
    Count++;
    //
    // If the CachedDevicePath variable contain too much instance, only remain 12 instances.
    //
    if (Count == 12) {
      SetDevicePathEndNode (TempDevicePath);
      break;
    }
  }
}

/**
  Expand a device path that starts with a hard drive media device path node to be a
  full device path that includes the full hardware path to the device. We need
  to do this so it can be booted. As an optimization the front match (the part point
  to the partition node. E.g. ACPI() /PCI()/ATA()/Partition() ) is saved in a variable
  so a connect all is not required on every boot. All successful history device path
  which point to partition node (the front part) will be saved.

  @param FilePath      The device path pointing to a load option.
                       It could be a short-form device path.
  @param FullPath      Return the full device path of the load option after
                       short-form device path expanding.
                       Caller is responsible to free it.
  @param FileSize      Return the load option size.

  @return The load option buffer. Caller is responsible to free the memory.
**/
VOID *
BmExpandPartitionDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FullPath,
  OUT UINTN                     *FileSize
  )
{
  EFI_STATUS                Status;
  UINTN                     BlockIoHandleCount;
  EFI_HANDLE                *BlockIoBuffer;
  VOID                      *FileBuffer;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoDevicePath;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *CachedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  UINTN                     CachedDevicePathSize;
  BOOLEAN                   NeedAdjust;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  UINTN                     Size;

  FileBuffer = NULL;
  //
  // Check if there is prestore 'HDDP' variable.
  // If exist, search the front path which point to partition node in the variable instants.
  // If fail to find or 'HDDP' not exist, reconnect all and search in all system
  //
  GetVariable2 (L"HDDP", &mBmHardDriveBootVariableGuid, (VOID **) &CachedDevicePath, &CachedDevicePathSize);

  //
  // Delete the invalid 'HDDP' variable.
  //
  if ((CachedDevicePath != NULL) && !IsDevicePathValid (CachedDevicePath, CachedDevicePathSize)) {
    FreePool (CachedDevicePath);
    CachedDevicePath = NULL;
    Status = gRT->SetVariable (
                    L"HDDP",
                    &mBmHardDriveBootVariableGuid,
                    0,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  if (CachedDevicePath != NULL) {
    TempNewDevicePath = CachedDevicePath;
    NeedAdjust = FALSE;
    do {
      //
      // Check every instance of the variable
      // First, check whether the instance contain the partition node, which is needed for distinguishing  multi
      // partial partition boot option. Second, check whether the instance could be connected.
      //
      Instance  = GetNextDevicePathInstance (&TempNewDevicePath, &Size);
      if (BmMatchPartitionDevicePathNode (Instance, (HARDDRIVE_DEVICE_PATH *) FilePath)) {
        //
        // Connect the device path instance, the device path point to hard drive media device path node
        // e.g. ACPI() /PCI()/ATA()/Partition()
        //
        Status = EfiBootManagerConnectDevicePath (Instance, NULL);
        if (!EFI_ERROR (Status)) {
          TempDevicePath = AppendDevicePath (Instance, NextDevicePathNode (FilePath));
          FileBuffer = BmGetLoadOptionBuffer (TempDevicePath, FullPath, FileSize);
          FreePool (TempDevicePath);

          if (FileBuffer != NULL) {
            //
            // Adjust the 'HDDP' instances sequence if the matched one is not first one.
            //
            if (NeedAdjust) {
              BmCachePartitionDevicePath (&CachedDevicePath, Instance);
              //
              // Save the matching Device Path so we don't need to do a connect all next time
              // Failing to save only impacts performance next time expanding the short-form device path
              //
              Status = gRT->SetVariable (
                L"HDDP",
                &mBmHardDriveBootVariableGuid,
                EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                GetDevicePathSize (CachedDevicePath),
                CachedDevicePath
                );
            }

            FreePool (Instance);
            FreePool (CachedDevicePath);
            return FileBuffer;
          }
        }
      }
      //
      // Come here means the first instance is not matched
      //
      NeedAdjust = TRUE;
      FreePool(Instance);
    } while (TempNewDevicePath != NULL);
  }

  //
  // If we get here we fail to find or 'HDDP' not exist, and now we need
  // to search all devices in the system for a matched partition
  //
  EfiBootManagerConnectAll ();
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BlockIoHandleCount, &BlockIoBuffer);
  if (EFI_ERROR (Status)) {
    BlockIoHandleCount = 0;
    BlockIoBuffer      = NULL;
  }
  //
  // Loop through all the device handles that support the BLOCK_IO Protocol
  //
  for (Index = 0; Index < BlockIoHandleCount; Index++) {
    BlockIoDevicePath = DevicePathFromHandle (BlockIoBuffer[Index]);
    if (BlockIoDevicePath == NULL) {
      continue;
    }

    if (BmMatchPartitionDevicePathNode (BlockIoDevicePath, (HARDDRIVE_DEVICE_PATH *) FilePath)) {
      //
      // Find the matched partition device path
      //
      TempDevicePath = AppendDevicePath (BlockIoDevicePath, NextDevicePathNode (FilePath));
      FileBuffer = BmGetLoadOptionBuffer (TempDevicePath, FullPath, FileSize);
      FreePool (TempDevicePath);

      if (FileBuffer != NULL) {
        BmCachePartitionDevicePath (&CachedDevicePath, BlockIoDevicePath);

        //
        // Save the matching Device Path so we don't need to do a connect all next time
        // Failing to save only impacts performance next time expanding the short-form device path
        //
        Status = gRT->SetVariable (
                        L"HDDP",
                        &mBmHardDriveBootVariableGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                        GetDevicePathSize (CachedDevicePath),
                        CachedDevicePath
                        );

        break;
      }
    }
  }

  if (CachedDevicePath != NULL) {
    FreePool (CachedDevicePath);
  }
  if (BlockIoBuffer != NULL) {
    FreePool (BlockIoBuffer);
  }
  return FileBuffer;
}

/**
  Expand the media device path which points to a BlockIo or SimpleFileSystem instance
  by appending EFI_REMOVABLE_MEDIA_FILE_NAME.

  @param DevicePath  The media device path pointing to a BlockIo or SimpleFileSystem instance.
  @param FullPath    Return the full device path pointing to the load option.
  @param FileSize    Return the size of the load option.

  @return  The load option buffer.
**/
VOID *
BmExpandMediaDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL        **FullPath,
  OUT UINTN                           *FileSize
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          Handle;
  EFI_BLOCK_IO_PROTOCOL               *BlockIo;
  VOID                                *Buffer;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  UINTN                               Size;
  UINTN                               TempSize;
  EFI_HANDLE                          *SimpleFileSystemHandles;
  UINTN                               NumberSimpleFileSystemHandles;
  UINTN                               Index;
  VOID                                *FileBuffer;
  UINT32                              AuthenticationStatus;

  //
  // Check whether the device is connected
  //
  TempDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &TempDevicePath, &Handle);
  if (!EFI_ERROR (Status)) {
    ASSERT (IsDevicePathEnd (TempDevicePath));

    TempDevicePath = FileDevicePath (Handle, EFI_REMOVABLE_MEDIA_FILE_NAME);
    FileBuffer = GetFileBufferByFilePath (TRUE, TempDevicePath, FileSize, &AuthenticationStatus);
    if (FileBuffer == NULL) {
      FreePool (TempDevicePath);
      TempDevicePath = NULL;
    }
    *FullPath = TempDevicePath;
    return FileBuffer;
  }

  //
  // For device boot option only pointing to the removable device handle, 
  // should make sure all its children handles (its child partion or media handles) are created and connected. 
  //
  gBS->ConnectController (Handle, NULL, NULL, TRUE);

  //
  // Issue a dummy read to the device to check for media change.
  // When the removable media is changed, any Block IO read/write will
  // cause the BlockIo protocol be reinstalled and EFI_MEDIA_CHANGED is
  // returned. After the Block IO protocol is reinstalled, subsequent
  // Block IO read/write will success.
  //
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &TempDevicePath, &Handle);
  ASSERT_EFI_ERROR (Status);
  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
  ASSERT_EFI_ERROR (Status);
  Buffer = AllocatePool (BlockIo->Media->BlockSize);
  if (Buffer != NULL) {
    BlockIo->ReadBlocks (
      BlockIo,
      BlockIo->Media->MediaId,
      0,
      BlockIo->Media->BlockSize,
      Buffer
      );
    FreePool (Buffer);
  }

  //
  // Detect the the default boot file from removable Media
  //
  FileBuffer = NULL;
  *FullPath = NULL;
  Size = GetDevicePathSize (DevicePath) - END_DEVICE_PATH_LENGTH;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &NumberSimpleFileSystemHandles,
         &SimpleFileSystemHandles
         );
  for (Index = 0; Index < NumberSimpleFileSystemHandles; Index++) {
    //
    // Get the device path size of SimpleFileSystem handle
    //
    TempDevicePath = DevicePathFromHandle (SimpleFileSystemHandles[Index]);
    TempSize = GetDevicePathSize (TempDevicePath) - END_DEVICE_PATH_LENGTH;
    //
    // Check whether the device path of boot option is part of the SimpleFileSystem handle's device path
    //
    if ((Size <= TempSize) && (CompareMem (TempDevicePath, DevicePath, Size) == 0)) {
      TempDevicePath = FileDevicePath (SimpleFileSystemHandles[Index], EFI_REMOVABLE_MEDIA_FILE_NAME);
      FileBuffer = GetFileBufferByFilePath (TRUE, TempDevicePath, FileSize, &AuthenticationStatus);
      if (FileBuffer != NULL) {
        *FullPath = TempDevicePath;
        break;
      }
      FreePool (TempDevicePath);
    }
  }

  if (SimpleFileSystemHandles != NULL) {
    FreePool (SimpleFileSystemHandles);
  }

  return FileBuffer;
}

/**
  Get the load option by its device path.

  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  Return the full device path of the load option after
                   short-form device path expanding.
                   Caller is responsible to free it.
  @param FileSize  Return the load option size.

  @return The load option buffer. Caller is responsible to free the memory.
**/
VOID *
BmGetLoadOptionBuffer (
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL          **FullPath,
  OUT UINTN                             *FileSize
  )
{
  EFI_HANDLE                      Handle;
  VOID                            *FileBuffer;
  UINT32                          AuthenticationStatus;
  EFI_DEVICE_PATH_PROTOCOL        *Node;
  EFI_STATUS                      Status;

  ASSERT ((FilePath != NULL) && (FullPath != NULL) && (FileSize != NULL));

  EfiBootManagerConnectDevicePath (FilePath, NULL);

  *FullPath  = NULL;
  *FileSize  = 0;
  FileBuffer = NULL;

  //
  // Boot from media device by adding a default file name \EFI\BOOT\BOOT{machine type short-name}.EFI
  //
  Node = FilePath;
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &Node, &Handle);
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &Node, &Handle);
  }

  if (!EFI_ERROR (Status) && IsDevicePathEnd (Node)) {
    return BmExpandMediaDevicePath (FilePath, FullPath, FileSize);
  }

  //
  // Expand the short-form device path to full device path
  //
  if ((DevicePathType (FilePath) == MEDIA_DEVICE_PATH) &&
      (DevicePathSubType (FilePath) == MEDIA_HARDDRIVE_DP)) {
    //
    // Expand the Harddrive device path
    //
    return BmExpandPartitionDevicePath (FilePath, FullPath, FileSize);
  } else {
    for (Node = FilePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node)) {
      if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) &&
          ((DevicePathSubType (Node) == MSG_USB_CLASS_DP) || (DevicePathSubType (Node) == MSG_USB_WWID_DP))) {
        break;
      }
    }

    if (!IsDevicePathEnd (Node)) {
      //
      // Expand the USB WWID/Class device path
      //
      FileBuffer = BmExpandUsbDevicePath (FilePath, FullPath, FileSize, Node);
      if ((FileBuffer == NULL) && (FilePath == Node)) {
        //
        // Boot Option device path starts with USB Class or USB WWID device path.
        // For Boot Option device path which doesn't begin with the USB Class or
        // USB WWID device path, it's not needed to connect again here.
        //
        BmConnectUsbShortFormDevicePath (FilePath);
        FileBuffer = BmExpandUsbDevicePath (FilePath, FullPath, FileSize, Node);
      }
      return FileBuffer;
    }
  }

  //
  // Fix up the boot option path if it points to a FV in memory map style of device path
  //
  if (BmIsMemmapFvFilePath (FilePath)) {
    return BmGetFileBufferByMemmapFv (FilePath, FullPath, FileSize);
  }

  //
  // Directly reads the load option when it doesn't reside in simple file system instance (LoadFile/LoadFile2),
  //   or it directly points to a file in simple file system instance.
  //
  Node   = FilePath;
  Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &Node, &Handle);
  FileBuffer = GetFileBufferByFilePath (TRUE, FilePath, FileSize, &AuthenticationStatus);
  if (FileBuffer != NULL) {
    if (EFI_ERROR (Status)) {
      *FullPath = DuplicateDevicePath (FilePath);
    } else {
      //
      // LoadFile () may cause the device path of the Handle be updated.
      //
      *FullPath = AppendDevicePath (DevicePathFromHandle (Handle), Node);
    }
  }

  return FileBuffer;
}

/**
  Attempt to boot the EFI boot option. This routine sets L"BootCurent" and
  also signals the EFI ready to boot event. If the device path for the option
  starts with a BBS device path a legacy boot is attempted via the registered 
  gLegacyBoot function. Short form device paths are also supported via this 
  rountine. A device path starting with MEDIA_HARDDRIVE_DP, MSG_USB_WWID_DP,
  MSG_USB_CLASS_DP gets expaned out to find the first device that matches.
  If the BootOption Device Path fails the removable media boot algorithm 
  is attempted (\EFI\BOOTIA32.EFI, \EFI\BOOTX64.EFI,... only one file type 
  is tried per processor type)

  @param  BootOption    Boot Option to try and boot.
                        On return, BootOption->Status contains the boot status.
                        EFI_SUCCESS     BootOption was booted
                        EFI_UNSUPPORTED A BBS device path was found with no valid callback
                                        registered via EfiBootManagerInitialize().
                        EFI_NOT_FOUND   The BootOption was not found on the system
                        !EFI_SUCCESS    BootOption failed with this error status

**/
VOID
EFIAPI
EfiBootManagerBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION             *BootOption
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  UINT16                    Uint16;
  UINTN                     OptionNumber;
  UINTN                     OriginalOptionNumber;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_HANDLE                FvHandle;
  VOID                      *FileBuffer;
  UINTN                     FileSize;
  EFI_BOOT_LOGO_PROTOCOL    *BootLogo;
  EFI_EVENT                 LegacyBootEvent;

  if (BootOption == NULL) {
    return;
  }

  if (BootOption->FilePath == NULL || BootOption->OptionType != LoadOptionTypeBoot) {
    BootOption->Status = EFI_INVALID_PARAMETER;
    return;
  }

  //
  // 1. Create Boot#### for a temporary boot if there is no match Boot#### (i.e. a boot by selected a EFI Shell using "Boot From File")
  //
  OptionNumber = BmFindBootOptionInVariable (BootOption);
  if (OptionNumber == LoadOptionNumberUnassigned) {
    Status = BmGetFreeOptionNumber (LoadOptionTypeBoot, &Uint16);
    if (!EFI_ERROR (Status)) {
      //
      // Save the BootOption->OptionNumber to restore later
      //
      OptionNumber             = Uint16;
      OriginalOptionNumber     = BootOption->OptionNumber;
      BootOption->OptionNumber = OptionNumber;
      Status = EfiBootManagerLoadOptionToVariable (BootOption);
      BootOption->OptionNumber = OriginalOptionNumber;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[Bds] Failed to create Boot#### for a temporary boot - %r!\n", Status));
      BootOption->Status = Status;
      return ;
    }
  }

  //
  // 2. Set BootCurrent
  //
  Uint16 = (UINT16) OptionNumber;
  BmSetVariableAndReportStatusCodeOnError (
    L"BootCurrent",
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    sizeof (UINT16),
    &Uint16
    );

  //
  // 3. Signal the EVT_SIGNAL_READY_TO_BOOT event when we are about to load and execute
  //    the boot option.
  //
  Node   = BootOption->FilePath;
  Status = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &Node, &FvHandle);
  if (!EFI_ERROR (Status) && CompareGuid (
        EfiGetNameGuidFromFwVolDevicePathNode ((CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) Node),
        PcdGetPtr (PcdBootManagerMenuFile)
        )) {
    DEBUG ((EFI_D_INFO, "[Bds] Booting Boot Manager Menu.\n"));
    BmStopHotkeyService (NULL, NULL);
  } else {
    EfiSignalEventReadyToBoot();
    //
    // Report Status Code to indicate ReadyToBoot was signalled
    //
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT));
    //
    // 4. Repair system through DriverHealth protocol
    //
    BmRepairAllControllers ();
  }

  PERF_START_EX (gImageHandle, "BdsAttempt", NULL, 0, (UINT32) OptionNumber);

  //
  // 5. Load EFI boot option to ImageHandle
  //
  ImageHandle = NULL;
  if (DevicePathType (BootOption->FilePath) != BBS_DEVICE_PATH) {
    Status     = EFI_NOT_FOUND;
    FileBuffer = BmGetLoadOptionBuffer (BootOption->FilePath, &FilePath, &FileSize);
    DEBUG_CODE (
      if (FileBuffer != NULL && CompareMem (BootOption->FilePath, FilePath, GetDevicePathSize (FilePath)) != 0) {
        DEBUG ((EFI_D_INFO, "[Bds] DevicePath expand: "));
        BmPrintDp (BootOption->FilePath);
        DEBUG ((EFI_D_INFO, " -> "));
        BmPrintDp (FilePath);
        DEBUG ((EFI_D_INFO, "\n"));
      }
    );
    if (BmIsLoadOptionPeHeaderValid (BootOption->OptionType, FileBuffer, FileSize)) {
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, PcdGet32 (PcdProgressCodeOsLoaderLoad));
      Status = gBS->LoadImage (
                      TRUE,
                      gImageHandle,
                      FilePath,
                      FileBuffer,
                      FileSize,
                      &ImageHandle
                      );
    }
    if (FileBuffer != NULL) {
      FreePool (FileBuffer);
    }
    if (FilePath != NULL) {
      FreePool (FilePath);
    }

    if (EFI_ERROR (Status)) {
      //
      // Report Status Code to indicate that the failure to load boot option
      //
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR)
        );
      BootOption->Status = Status;
      return;
    }
  }

  //
  // 6. Adjust the different type memory page number just before booting
  //    and save the updated info into the variable for next boot to use
  //
  if ((BootOption->Attributes & LOAD_OPTION_CATEGORY) == LOAD_OPTION_CATEGORY_BOOT) {
    if (PcdGetBool (PcdResetOnMemoryTypeInformationChange)) {
      BmSetMemoryTypeInformationVariable ();
    }
  }

  DEBUG_CODE_BEGIN();
    if (BootOption->Description == NULL) {
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "[Bds]Booting from unknown device path\n"));
    } else {
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "[Bds]Booting %s\n", BootOption->Description));
    }
  DEBUG_CODE_END();

  //
  // Check to see if we should legacy BOOT. If yes then do the legacy boot
  // Write boot to OS performance data for Legacy boot
  //
  if ((DevicePathType (BootOption->FilePath) == BBS_DEVICE_PATH) && (DevicePathSubType (BootOption->FilePath) == BBS_BBS_DP)) {
    if (mBmLegacyBoot != NULL) {
      //
      // Write boot to OS performance data for legacy boot.
      //
      PERF_CODE (
        //
        // Create an event to be signalled when Legacy Boot occurs to write performance data.
        //
        Status = EfiCreateEventLegacyBootEx(
                   TPL_NOTIFY,
                   BmWriteBootToOsPerformanceData,
                   NULL, 
                   &LegacyBootEvent
                   );
        ASSERT_EFI_ERROR (Status);
      );

      mBmLegacyBoot (BootOption);
    } else {
      BootOption->Status = EFI_UNSUPPORTED;
    }

    PERF_END_EX (gImageHandle, "BdsAttempt", NULL, 0, (UINT32) OptionNumber);
    return;
  }
 
  //
  // Provide the image with its load options
  //
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);
  ASSERT_EFI_ERROR (Status);

  ImageInfo->LoadOptionsSize  = BootOption->OptionalDataSize;
  ImageInfo->LoadOptions      = BootOption->OptionalData;

  //
  // Clean to NULL because the image is loaded directly from the firmwares boot manager.
  //
  ImageInfo->ParentHandle = NULL;

  //
  // Before calling the image, enable the Watchdog Timer for 5 minutes period
  //
  gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

  //
  // Write boot to OS performance data for UEFI boot
  //
  PERF_CODE (
    BmWriteBootToOsPerformanceData (NULL, NULL);
  );

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, PcdGet32 (PcdProgressCodeOsLoaderStart));

  Status = gBS->StartImage (ImageHandle, &BootOption->ExitDataSize, &BootOption->ExitData);
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Image Return Status = %r\n", Status));
  BootOption->Status = Status;
  if (EFI_ERROR (Status)) {
    //
    // Report Status Code to indicate that boot failure
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED)
      );
  }
  PERF_END_EX (gImageHandle, "BdsAttempt", NULL, 0, (UINT32) OptionNumber);

  //
  // Clear the Watchdog Timer after the image returns
  //
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  //
  // Set Logo status invalid after trying one boot option
  //
  BootLogo = NULL;
  Status = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **) &BootLogo);
  if (!EFI_ERROR (Status) && (BootLogo != NULL)) {
    Status = BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Clear Boot Current
  //
  Status = gRT->SetVariable (
                  L"BootCurrent",
                  &gEfiGlobalVariableGuid,
                  0,
                  0,
                  NULL
                  );
  //
  // Deleting variable with current variable implementation shouldn't fail.
  // When BootXXXX (e.g.: BootManagerMenu) boots BootYYYY, exiting BootYYYY causes BootCurrent deleted,
  // exiting BootXXXX causes deleting BootCurrent returns EFI_NOT_FOUND.
  //
  ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);
}

/**
  Check whether there is a instance in BlockIoDevicePath, which contain multi device path
  instances, has the same partition node with HardDriveDevicePath device path

  @param  BlockIoDevicePath      Multi device path instances which need to check
  @param  HardDriveDevicePath    A device path which starts with a hard drive media
                                 device path.

  @retval TRUE                   There is a matched device path instance.
  @retval FALSE                  There is no matched device path instance.

**/
BOOLEAN
BmMatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  )
{
  HARDDRIVE_DEVICE_PATH     *Node;

  if ((BlockIoDevicePath == NULL) || (HardDriveDevicePath == NULL)) {
    return FALSE;
  }

  //
  // find the partition device path node
  //
  while (!IsDevicePathEnd (BlockIoDevicePath)) {
    if ((DevicePathType (BlockIoDevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (BlockIoDevicePath) == MEDIA_HARDDRIVE_DP)
        ) {
      break;
    }

    BlockIoDevicePath = NextDevicePathNode (BlockIoDevicePath);
  }

  if (IsDevicePathEnd (BlockIoDevicePath)) {
    return FALSE;
  }

  //
  // See if the harddrive device path in blockio matches the orig Hard Drive Node
  //
  Node = (HARDDRIVE_DEVICE_PATH *) BlockIoDevicePath;

  //
  // Match Signature and PartitionNumber.
  // Unused bytes in Signature are initiaized with zeros.
  //
  return (BOOLEAN) (
    (Node->PartitionNumber == HardDriveDevicePath->PartitionNumber) &&
    (Node->MBRType == HardDriveDevicePath->MBRType) &&
    (Node->SignatureType == HardDriveDevicePath->SignatureType) &&
    (CompareMem (Node->Signature, HardDriveDevicePath->Signature, sizeof (Node->Signature)) == 0)
    );
}

/**
  Emuerate all possible bootable medias in the following order:
  1. Removable BlockIo            - The boot option only points to the removable media
                                    device, like USB key, DVD, Floppy etc.
  2. Fixed BlockIo                - The boot option only points to a Fixed blockIo device,
                                    like HardDisk.
  3. Non-BlockIo SimpleFileSystem - The boot option points to a device supporting
                                    SimpleFileSystem Protocol, but not supporting BlockIo
                                    protocol.
  4. LoadFile                     - The boot option points to the media supporting 
                                    LoadFile protocol.
  Reference: UEFI Spec chapter 3.3 Boot Option Variables Default Boot Behavior

  @param BootOptionCount   Return the boot option count which has been found.

  @retval   Pointer to the boot option array.
**/
EFI_BOOT_MANAGER_LOAD_OPTION *
BmEnumerateBootOptions (
  UINTN                                 *BootOptionCount
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MANAGER_LOAD_OPTION          *BootOptions;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *Handles;
  EFI_BLOCK_IO_PROTOCOL                 *BlkIo;
  UINTN                                 Removable;
  UINTN                                 Index;
  CHAR16                                *Description;

  ASSERT (BootOptionCount != NULL);

  *BootOptionCount = 0;
  BootOptions      = NULL;

  //
  // Parse removable block io followed by fixed block io
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiBlockIoProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );

  for (Removable = 0; Removable < 2; Removable++) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      Handles[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **) &BlkIo
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Skip the logical partitions
      //
      if (BlkIo->Media->LogicalPartition) {
        continue;
      }

      //
      // Skip the fixed block io then the removable block io
      //
      if (BlkIo->Media->RemovableMedia == ((Removable == 0) ? FALSE : TRUE)) {
        continue;
      }

      Description = BmGetBootDescription (Handles[Index]);
      BootOptions = ReallocatePool (
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                      BootOptions
                      );
      ASSERT (BootOptions != NULL);

      Status = EfiBootManagerInitializeLoadOption (
                 &BootOptions[(*BootOptionCount)++],
                 LoadOptionNumberUnassigned,
                 LoadOptionTypeBoot,
                 LOAD_OPTION_ACTIVE,
                 Description,
                 DevicePathFromHandle (Handles[Index]),
                 NULL,
                 0
                 );
      ASSERT_EFI_ERROR (Status);

      FreePool (Description);
    }
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse simple file system not based on block io
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
     if (!EFI_ERROR (Status)) {
      //
      //  Skip if the file system handle supports a BlkIo protocol, which we've handled in above
      //
      continue;
    }
    Description = BmGetBootDescription (Handles[Index]);
    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               Description,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (Description);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse load file, assuming UEFI Network boot option
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiLoadFileProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {

    Description = BmGetBootDescription (Handles[Index]);
    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               Description,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (Description);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  return BootOptions;
}

/**
  The function enumerates all boot options, creates them and registers them in the BootOrder variable.
**/
VOID
EFIAPI
EfiBootManagerRefreshAllBootOption (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  *NvBootOptions;
  UINTN                         NvBootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;
  UINTN                         Index;

  //
  // Optionally refresh the legacy boot option
  //
  if (mBmRefreshLegacyBootOption != NULL) {
    mBmRefreshLegacyBootOption ();
  }

  BootOptions   = BmEnumerateBootOptions (&BootOptionCount);
  NvBootOptions = EfiBootManagerGetLoadOptions (&NvBootOptionCount, LoadOptionTypeBoot);

  //
  // Mark the boot option as added by BDS by setting OptionalData to a special GUID
  //
  for (Index = 0; Index < BootOptionCount; Index++) {
    BootOptions[Index].OptionalData     = AllocateCopyPool (sizeof (EFI_GUID), &mBmAutoCreateBootOptionGuid);
    BootOptions[Index].OptionalDataSize = sizeof (EFI_GUID);
  }

  //
  // Remove invalid EFI boot options from NV
  //
  for (Index = 0; Index < NvBootOptionCount; Index++) {
    if (((DevicePathType (NvBootOptions[Index].FilePath) != BBS_DEVICE_PATH) || 
         (DevicePathSubType (NvBootOptions[Index].FilePath) != BBS_BBS_DP)
        ) &&
        (NvBootOptions[Index].OptionalDataSize == sizeof (EFI_GUID)) &&
        CompareGuid ((EFI_GUID *) NvBootOptions[Index].OptionalData, &mBmAutoCreateBootOptionGuid)
       ) {
      //
      // Only check those added by BDS
      // so that the boot options added by end-user or OS installer won't be deleted
      //
      if (BmFindLoadOption (&NvBootOptions[Index], BootOptions, BootOptionCount) == (UINTN) -1) {
        Status = EfiBootManagerDeleteLoadOptionVariable (NvBootOptions[Index].OptionNumber, LoadOptionTypeBoot);
        //
        // Deleting variable with current variable implementation shouldn't fail.
        //
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  //
  // Add new EFI boot options to NV
  //
  for (Index = 0; Index < BootOptionCount; Index++) {
    if (BmFindLoadOption (&BootOptions[Index], NvBootOptions, NvBootOptionCount) == (UINTN) -1) {
      EfiBootManagerAddLoadOptionVariable (&BootOptions[Index], (UINTN) -1);
      //
      // Try best to add the boot options so continue upon failure.
      //
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions,   BootOptionCount);
  EfiBootManagerFreeLoadOptions (NvBootOptions, NvBootOptionCount);
}

/**
  This function is called to create the boot option for the Boot Manager Menu.

  The Boot Manager Menu is shown after successfully booting a boot option.
  Assume the BootManagerMenuFile is in the same FV as the module links to this library.

  @param  BootOption    Return the boot option of the Boot Manager Menu

  @retval EFI_SUCCESS   Successfully register the Boot Manager Menu.
  @retval Status        Return status of gRT->SetVariable (). BootOption still points
                        to the Boot Manager Menu even the Status is not EFI_SUCCESS.
**/
EFI_STATUS
BmRegisterBootManagerMenu (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION   *BootOption
  )
{
  EFI_STATUS                         Status;
  CHAR16                             *Description;
  UINTN                              DescriptionLength;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;

  Status = GetSectionFromFv (
             PcdGetPtr (PcdBootManagerMenuFile),
             EFI_SECTION_USER_INTERFACE,
             0,
             (VOID **) &Description,
             &DescriptionLength
             );
  if (EFI_ERROR (Status)) {
    Description = NULL;
  }

  EfiInitializeFwVolDevicepathNode (&FileNode, PcdGetPtr (PcdBootManagerMenuFile));
  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);
  DevicePath = AppendDevicePathNode (
                 DevicePathFromHandle (LoadedImage->DeviceHandle),
                 (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
                 );
  ASSERT (DevicePath != NULL);

  Status = EfiBootManagerInitializeLoadOption (
             BootOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN,
             (Description != NULL) ? Description : L"Boot Manager Menu",
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);
  if (Description != NULL) {
    FreePool (Description);
  }

  DEBUG_CODE (
    EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions;
    UINTN                           BootOptionCount;

    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
    ASSERT (BmFindLoadOption (BootOption, BootOptions, BootOptionCount) == -1);
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
    );

  return EfiBootManagerAddLoadOptionVariable (BootOption, 0);
}

/**
  Return the boot option corresponding to the Boot Manager Menu.
  It may automatically create one if the boot option hasn't been created yet.
  
  @param BootOption    Return the Boot Manager Menu.

  @retval EFI_SUCCESS   The Boot Manager Menu is successfully returned.
  @retval Status        Return status of gRT->SetVariable (). BootOption still points
                        to the Boot Manager Menu even the Status is not EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
EfiBootManagerGetBootManagerMenu (
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOption
  )
{
  EFI_STATUS                   Status;
  UINTN                        BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOptions;
  UINTN                        Index;
  EFI_DEVICE_PATH_PROTOCOL     *Node;
  EFI_HANDLE                   FvHandle;
  
  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
    Node   = BootOptions[Index].FilePath;
    Status = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &Node, &FvHandle);
    if (!EFI_ERROR (Status)) {
      if (CompareGuid (
            EfiGetNameGuidFromFwVolDevicePathNode ((CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) Node),
            PcdGetPtr (PcdBootManagerMenuFile)
            )
          ) {        
        Status = EfiBootManagerInitializeLoadOption (
                   BootOption,
                   BootOptions[Index].OptionNumber,
                   BootOptions[Index].OptionType,
                   BootOptions[Index].Attributes,
                   BootOptions[Index].Description,
                   BootOptions[Index].FilePath,
                   BootOptions[Index].OptionalData,
                   BootOptions[Index].OptionalDataSize
                   );
        ASSERT_EFI_ERROR (Status);
        break;
      }
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);

  //
  // Automatically create the Boot#### for Boot Manager Menu when not found.
  //
  if (Index == BootOptionCount) {
    return BmRegisterBootManagerMenu (BootOption);
  } else {
    return EFI_SUCCESS;
  }
}

