/** @file
  Library functions which relate with boot option description.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalBm.h"

#define VENDOR_IDENTIFICATION_OFFSET   3
#define VENDOR_IDENTIFICATION_LENGTH   8
#define PRODUCT_IDENTIFICATION_OFFSET  11
#define PRODUCT_IDENTIFICATION_LENGTH  16

CONST UINT16  mBmUsbLangId    = 0x0409; // English
CHAR16        mBmUefiPrefix[] = L"UEFI ";

CHAR16  mBootDescGenericManufacturer[] = L"Generic";
CHAR16  mBootDescSd[]                  = L"SD Device";
CHAR16  mBootDescEmmc[]                = L"eMMC Device";
CHAR16  mBootDescEmmcUserData[]        = L"eMMC User Data";
CHAR16  mBootDescEmmcBoot1[]           = L"eMMC Boot 1";
CHAR16  mBootDescEmmcBoot2[]           = L"eMMC Boot 2";
CHAR16  mBootDescEmmcGp1[]             = L"eMMC GP 1";
CHAR16  mBootDescEmmcGp2[]             = L"eMMC GP 2";
CHAR16  mBootDescEmmcGp3[]             = L"eMMC GP 3";
CHAR16  mBootDescEmmcGp4[]             = L"eMMC GP 4";

typedef struct {
  UINT8     Id;
  CHAR16    *Name;
} BM_SDMMC_MANUFACTURER;

BM_SDMMC_MANUFACTURER  mSdManufacturers[] = {
  { 0x01, L"Panasonic"               },
  { 0x02, L"Toshiba/Kingston/Viking" },
  { 0x03, L"SanDisk"                 },
  { 0x08, L"Silicon Power"           },
  { 0x18, L"Infineon"                },
  { 0x1b, L"Transcend/Samsung"       },
  { 0x1c, L"Transcend"               },
  { 0x1d, L"Corsair/AData"           },
  { 0x1e, L"Transcend"               },
  { 0x1f, L"Kingston"                },
  { 0x27, L"Delkin/Phison"           },
  { 0x28, L"Lexar"                   },
  { 0x30, L"SanDisk"                 },
  { 0x31, L"Silicon Power"           },
  { 0x33, L"STMicroelectronics"      },
  { 0x41, L"Kingston"                },
  { 0x6f, L"STMicroelectronics"      },
  { 0x74, L"Transcend"               },
  { 0x76, L"Patriot"                 },
  { 0x82, L"Gobe/Sony"               },
  { 0x9c, L"Angelbird/Hoodman"       },
};

BM_SDMMC_MANUFACTURER  mMmcManufacturers[] = {
  { 0x00, L"SanDisk"          },
  { 0x02, L"Kingston/SanDisk" },
  { 0x03, L"Toshiba"          },
  { 0x11, L"Toshiba"          },
  { 0x13, L"Micron"           },
  { 0x15, L"Samsung"          },
  { 0x37, L"KingMax"          },
  { 0x44, L"ATP"              },
  { 0x45, L"SanDisk"          },
  { 0x2c, L"Kingston"         },
  { 0x70, L"Kingston"         },
  { 0x88, L"Foresee"          },
  { 0x9b, L"YMTC"             },
  { 0xd6, L"Foresee"          },
  { 0xfe, L"Micron"           },
};

LIST_ENTRY  mPlatformBootDescriptionHandlers = INITIALIZE_LIST_HEAD_VARIABLE (mPlatformBootDescriptionHandlers);

/**
  For a bootable Device path, return its boot type.

  @param  DevicePath        The bootable device Path to check

  @retval AcpiFloppyBoot    If given device path contains ACPI_DEVICE_PATH type device path node
                            which HID is floppy device.
  @retval MessageAtapiBoot  If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_ATAPI_DP.
  @retval MessageSataBoot   If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_SATA_DP.
  @retval MessageScsiBoot   If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_SCSI_DP.
  @retval MessageUsbBoot    If given device path contains MESSAGING_DEVICE_PATH type device path node
                            and its last device path node's subtype is MSG_USB_DP.
  @retval BmMiscBoot        If tiven device path doesn't match the above condition.

**/
BM_BOOT_TYPE
BmDevicePathType (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;

  ASSERT (DevicePath != NULL);

  for (Node = DevicePath; !IsDevicePathEndType (Node); Node = NextDevicePathNode (Node)) {
    switch (DevicePathType (Node)) {
      case ACPI_DEVICE_PATH:
        if (EISA_ID_TO_NUM (((ACPI_HID_DEVICE_PATH *)Node)->HID) == 0x0604) {
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
                 (DevicePathSubType (NextNode) == MSG_DEVICE_LOGICAL_UNIT_DP)
                 );

        //
        // If the device path not only point to driver device, it is not a messaging device path,
        //
        if (!IsDevicePathEndType (NextNode)) {
          continue;
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
        }
    }
  }

  return BmMiscBoot;
}

/**
  Eliminate the extra spaces in the Str to one space.

  @param    Str     Input string info.
**/
VOID
BmEliminateExtraSpaces (
  IN CHAR16  *Str
  )
{
  UINTN  Index;
  UINTN  ActualIndex;

  for (Index = 0, ActualIndex = 0; Str[Index] != L'\0'; Index++) {
    if ((Str[Index] != L' ') || ((ActualIndex > 0) && (Str[ActualIndex - 1] != L' '))) {
      Str[ActualIndex++] = Str[Index];
    }
  }

  Str[ActualIndex] = L'\0';
}

/**
  Swap a byte array.

  @param    Source     Input byte array.
  @param    Length     The size of Source in bytes.
**/
VOID
BmSwapBytes (
  IN UINT8  *Source,
  IN UINTN  Length
  )
{
  UINTN  Index;
  UINT8  Temp;
  UINTN  Count;

  Count = Length / 2;
  for (Index = 0; Index < Count; ++Index) {
    Temp                       = Source[Index];
    Source[Index]              = Source[Length - 1 - Index];
    Source[Length - 1 - Index] = Temp;
  }
}

/**
  Get the SD/MMC manufacturer name from an ID.

  @param    Id         Manufacturer ID.
  @param    IsMmc      Boolean indicating whether the ID is for SD or eMMC.

  @return  The manufacturer string.
**/
CHAR16 *
BmGetSdMmcManufacturerName (
  IN UINT8    Id,
  IN BOOLEAN  IsMmc
  )
{
  BM_SDMMC_MANUFACTURER  *List;
  UINT8                  Count;
  UINTN                  Index;

  List  = IsMmc ? mMmcManufacturers : mSdManufacturers;
  Count = IsMmc ? ARRAY_SIZE (mMmcManufacturers)
                : ARRAY_SIZE (mSdManufacturers);

  for (Index = 0; Index < Count; ++Index) {
    if (List[Index].Id == Id) {
      return List[Index].Name;
    }
  }

  return mBootDescGenericManufacturer;
}

/**
  Get the eMMC partition type from a controller path.

  @param    DevicePath       Pointer to a CONTROLLER_DEVICE_PATH.

  @return  The description string.
**/
CHAR16 *
BmGetEmmcTypeDescription (
  CONTROLLER_DEVICE_PATH  *DevicePath
  )
{
  switch (DevicePath->ControllerNumber) {
    case EmmcPartitionUserData:
      return mBootDescEmmcUserData;
    case EmmcPartitionBoot1:
      return mBootDescEmmcBoot1;
    case EmmcPartitionBoot2:
      return mBootDescEmmcBoot2;
    case EmmcPartitionGP1:
      return mBootDescEmmcGp1;
    case EmmcPartitionGP2:
      return mBootDescEmmcGp2;
    case EmmcPartitionGP3:
      return mBootDescEmmcGp3;
    case EmmcPartitionGP4:
      return mBootDescEmmcGp4;
    default:
      break;
  }

  return mBootDescEmmc;
}

/**
  Get an SD/MMC boot description.

  @param    ManufacturerName           Manufacturer name string.
  @param    ProductName                Product name from CID.
  @param    ProductNameLength          Length of ProductName.
  @param    SerialNumber               Serial number from CID.
  @param    DeviceType                 Device type string (e.g. SD or an eMMC partition).

  @return  The description string.
**/
CHAR16 *
BmGetSdMmcDescription (
  IN CHAR16  *ManufacturerName,
  IN UINT8   *ProductName,
  IN UINT8   ProductNameLength,
  IN UINT8   SerialNumber[4],
  IN CHAR16  *DeviceType
  )
{
  CHAR16  *Desc;
  UINTN   DescSize;

  DescSize = StrSize (ManufacturerName) - sizeof (CHAR16)             // "Samsung"
             + sizeof (CHAR16)                                        // " "
             + ProductNameLength * sizeof (CHAR16)                    // "BJTD4R"
             + sizeof (CHAR16)                                        // " "
             + sizeof (UINT32) * 2 * sizeof (CHAR16)                  // "00000000"
             + sizeof (CHAR16)                                        // " "
             + StrSize (DeviceType);                                  // "eMMC User Data\0"

  Desc = AllocateZeroPool (DescSize);
  if (Desc == NULL) {
    return NULL;
  }

  BmSwapBytes (ProductName, ProductNameLength);

  UnicodeSPrint (
    Desc,
    DescSize,
    L"%s %.*a %02x%02x%02x%02x %s",
    ManufacturerName,
    ProductNameLength,
    ProductName,
    SerialNumber[0],
    SerialNumber[1],
    SerialNumber[2],
    SerialNumber[3],
    DeviceType
    );

  return Desc;
}

/**
  Try to get the controller's ATA/ATAPI description.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetDescriptionFromDiskInfo (
  IN EFI_HANDLE  Handle
  )
{
  UINTN                     Index;
  EFI_STATUS                Status;
  EFI_DISK_INFO_PROTOCOL    *DiskInfo;
  UINT32                    BufferSize;
  EFI_ATAPI_IDENTIFY_DATA   IdentifyData;
  EFI_SCSI_INQUIRY_DATA     InquiryData;
  SD_CID                    SdCid;
  EMMC_CID                  EmmcCid;
  CHAR16                    *Description;
  UINTN                     Length;
  CONST UINTN               ModelNameLength    = 40;
  CONST UINTN               SerialNumberLength = 20;
  CHAR8                     *StrPtr;
  UINT8                     Temp;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  Description = NULL;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDiskInfoProtocolGuid,
                  (VOID **)&DiskInfo
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoAhciInterfaceGuid) ||
      CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoIdeInterfaceGuid))
  {
    BufferSize = sizeof (EFI_ATAPI_IDENTIFY_DATA);
    Status     = DiskInfo->Identify (
                             DiskInfo,
                             &IdentifyData,
                             &BufferSize
                             );
    if (!EFI_ERROR (Status)) {
      Description = AllocateZeroPool ((ModelNameLength + SerialNumberLength + 2) * sizeof (CHAR16));
      if (Description == NULL) {
        ASSERT (Description != NULL);
        return NULL;
      }

      for (Index = 0; Index + 1 < ModelNameLength; Index += 2) {
        Description[Index]     = (CHAR16)IdentifyData.ModelName[Index + 1];
        Description[Index + 1] = (CHAR16)IdentifyData.ModelName[Index];
      }

      Length                = Index;
      Description[Length++] = L' ';

      for (Index = 0; Index + 1 < SerialNumberLength; Index += 2) {
        Description[Length + Index]     = (CHAR16)IdentifyData.SerialNo[Index + 1];
        Description[Length + Index + 1] = (CHAR16)IdentifyData.SerialNo[Index];
      }

      Length               += Index;
      Description[Length++] = L'\0';
      ASSERT (Length == ModelNameLength + SerialNumberLength + 2);

      BmEliminateExtraSpaces (Description);
    }
  } else if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoScsiInterfaceGuid) ||
             CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoUfsInterfaceGuid))
  {
    BufferSize = sizeof (EFI_SCSI_INQUIRY_DATA);
    Status     = DiskInfo->Inquiry (
                             DiskInfo,
                             &InquiryData,
                             &BufferSize
                             );
    if (!EFI_ERROR (Status)) {
      Description = AllocateZeroPool ((VENDOR_IDENTIFICATION_LENGTH + PRODUCT_IDENTIFICATION_LENGTH + 2) * sizeof (CHAR16));
      if (Description == NULL) {
        ASSERT (Description != NULL);
        return NULL;
      }

      //
      // Per SCSI spec, EFI_SCSI_INQUIRY_DATA.Reserved_5_95[3 - 10] save the Verdor identification
      // EFI_SCSI_INQUIRY_DATA.Reserved_5_95[11 - 26] save the product identification,
      // Here combine the vendor identification and product identification to the description.
      //
      StrPtr                               = (CHAR8 *)(&InquiryData.Reserved_5_95[VENDOR_IDENTIFICATION_OFFSET]);
      Temp                                 = StrPtr[VENDOR_IDENTIFICATION_LENGTH];
      StrPtr[VENDOR_IDENTIFICATION_LENGTH] = '\0';
      AsciiStrToUnicodeStrS (StrPtr, Description, VENDOR_IDENTIFICATION_LENGTH + 1);
      StrPtr[VENDOR_IDENTIFICATION_LENGTH] = Temp;

      //
      // Add one space at the middle of vendor information and product information.
      //
      Description[VENDOR_IDENTIFICATION_LENGTH] = L' ';

      StrPtr                                = (CHAR8 *)(&InquiryData.Reserved_5_95[PRODUCT_IDENTIFICATION_OFFSET]);
      StrPtr[PRODUCT_IDENTIFICATION_LENGTH] = '\0';
      AsciiStrToUnicodeStrS (StrPtr, Description + VENDOR_IDENTIFICATION_LENGTH + 1, PRODUCT_IDENTIFICATION_LENGTH + 1);

      BmEliminateExtraSpaces (Description);
    }
  } else if (CompareGuid (&DiskInfo->Interface, &gEfiDiskInfoSdMmcInterfaceGuid)) {
    DevicePath = DevicePathFromHandle (Handle);
    if (DevicePath == NULL) {
      return NULL;
    }

    while (!IsDevicePathEnd (DevicePath) && (DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH)) {
      DevicePath = NextDevicePathNode (DevicePath);
    }

    if (IsDevicePathEnd (DevicePath)) {
      return NULL;
    }

    if (DevicePathSubType (DevicePath) == MSG_SD_DP) {
      BufferSize = sizeof (SD_CID);
      Status     = DiskInfo->Inquiry (DiskInfo, &SdCid, &BufferSize);
      if (EFI_ERROR (Status)) {
        return NULL;
      }

      Description = BmGetSdMmcDescription (
                      BmGetSdMmcManufacturerName (SdCid.ManufacturerId, FALSE),
                      SdCid.ProductName,
                      ARRAY_SIZE (SdCid.ProductName),
                      SdCid.ProductSerialNumber,
                      mBootDescSd
                      );
    } else if (DevicePathSubType (DevicePath) == MSG_EMMC_DP) {
      BufferSize = sizeof (EMMC_CID);
      Status     = DiskInfo->Inquiry (DiskInfo, &EmmcCid, &BufferSize);
      if (EFI_ERROR (Status)) {
        return NULL;
      }

      Description = mBootDescEmmc;

      DevicePath = NextDevicePathNode (DevicePath);
      if (DevicePath->SubType == HW_CONTROLLER_DP) {
        Description = BmGetEmmcTypeDescription ((CONTROLLER_DEVICE_PATH *)DevicePath);
      }

      Description = BmGetSdMmcDescription (
                      BmGetSdMmcManufacturerName (EmmcCid.ManufacturerId, TRUE),
                      EmmcCid.ProductName,
                      ARRAY_SIZE (EmmcCid.ProductName),
                      EmmcCid.ProductSerialNumber,
                      Description
                      );
    } else {
      return NULL;
    }

    Description = AllocateCopyPool (StrSize (Description), Description);
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
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                 Status;
  EFI_USB_IO_PROTOCOL        *UsbIo;
  CHAR16                     NullChar;
  CHAR16                     *Manufacturer;
  CHAR16                     *Product;
  CHAR16                     *SerialNumber;
  CHAR16                     *Description;
  EFI_USB_DEVICE_DESCRIPTOR  DevDesc;
  UINTN                      DescMaxSize;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo
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
      )
  {
    return NULL;
  }

  DescMaxSize = StrSize (Manufacturer) + StrSize (Product) + StrSize (SerialNumber);
  Description = AllocateZeroPool (DescMaxSize);
  if (Description == NULL) {
    ASSERT (Description != NULL);
    return NULL;
  }

  StrCatS (Description, DescMaxSize/sizeof (CHAR16), Manufacturer);
  StrCatS (Description, DescMaxSize/sizeof (CHAR16), L" ");

  StrCatS (Description, DescMaxSize/sizeof (CHAR16), Product);
  StrCatS (Description, DescMaxSize/sizeof (CHAR16), L" ");

  StrCatS (Description, DescMaxSize/sizeof (CHAR16), SerialNumber);

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
  Return the description for network boot device.

  @param Handle                Controller handle.

  @return  The description string or NULL if the string could not be created.
**/
CHAR16 *
BmGetNetworkDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  MAC_ADDR_DEVICE_PATH      *Mac;
  VLAN_DEVICE_PATH          *Vlan;
  EFI_DEVICE_PATH_PROTOCOL  *Ip;
  EFI_DEVICE_PATH_PROTOCOL  *Uri;
  CHAR16                    *Description;
  UINTN                     DescriptionSize;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiLoadFileProtocolGuid,
                  NULL,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status) || (DevicePath == NULL)) {
    return NULL;
  }

  //
  // The PXE device path is like:
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv4(...)
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)
  //
  // The HTTP device path is like:
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv4(...)[/Dns(...)][/Uri(...)]/Uri(...)
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)[/Dns(...)][/Uri(...)]/Uri(...)
  //
  while (!IsDevicePathEnd (DevicePath) &&
         ((DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH) ||
          (DevicePathSubType (DevicePath) != MSG_MAC_ADDR_DP))
         )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  if (IsDevicePathEnd (DevicePath)) {
    return NULL;
  }

  Mac        = (MAC_ADDR_DEVICE_PATH *)DevicePath;
  DevicePath = NextDevicePathNode (DevicePath);

  //
  // Locate the optional Vlan node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_VLAN_DP)
      )
  {
    Vlan       = (VLAN_DEVICE_PATH *)DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Vlan = NULL;
  }

  //
  // Skip the optional Wi-Fi node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_WIFI_DP)
      )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Locate the IP node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      ((DevicePathSubType (DevicePath) == MSG_IPv4_DP) ||
       (DevicePathSubType (DevicePath) == MSG_IPv6_DP))
      )
  {
    Ip         = DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Ip = NULL;
  }

  //
  // Skip the optional DNS node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_DNS_DP)
      )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Locate the URI node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_URI_DP)
      )
  {
    Uri        = DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Uri = NULL;
  }

  //
  // Build description like below:
  //   "PXEv6 (MAC:112233445566 VLAN1)"
  //   "HTTPv4 (MAC:112233445566)"
  //
  DescriptionSize = sizeof (L"HTTPv6 (MAC:112233445566 VLAN65535)");
  Description     = AllocatePool (DescriptionSize);
  if (Description == NULL) {
    ASSERT (Description != NULL);
    return NULL;
  }

  UnicodeSPrint (
    Description,
    DescriptionSize,
    (Vlan == NULL) ?
    L"%sv%d (MAC:%02x%02x%02x%02x%02x%02x)" :
    L"%sv%d (MAC:%02x%02x%02x%02x%02x%02x VLAN%d)",
    (Uri == NULL) ? L"PXE" : L"HTTP",
    ((Ip == NULL) || (DevicePathSubType (Ip) == MSG_IPv4_DP)) ? 4 : 6,
    Mac->MacAddress.Addr[0],
    Mac->MacAddress.Addr[1],
    Mac->MacAddress.Addr[2],
    Mac->MacAddress.Addr[3],
    Mac->MacAddress.Addr[4],
    Mac->MacAddress.Addr[5],
    (Vlan == NULL) ? 0 : Vlan->VlanId
    );
  return Description;
}

/**
  Return the boot description for LoadFile

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetLoadFileDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode;
  CHAR16                    *Description;
  EFI_LOAD_FILE_PROTOCOL    *LoadFile;

  Status = gBS->HandleProtocol (Handle, &gEfiLoadFileProtocolGuid, (VOID **)&LoadFile);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get the file name
  //
  Description = NULL;
  Status      = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&FilePath);
  if (!EFI_ERROR (Status)) {
    DevicePathNode = FilePath;
    while (!IsDevicePathEnd (DevicePathNode)) {
      if ((DevicePathNode->Type == MEDIA_DEVICE_PATH) && (DevicePathNode->SubType == MEDIA_FILEPATH_DP)) {
        Description = (CHAR16 *)(DevicePathNode + 1);
        break;
      }

      DevicePathNode = NextDevicePathNode (DevicePathNode);
    }
  }

  if (Description != NULL) {
    return AllocateCopyPool (StrSize (Description), Description);
  }

  return NULL;
}

/**
  Return the boot description for NVME boot device.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetNvmeDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                                Status;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL        *NvmePassthru;
  EFI_DEV_PATH_PTR                          DevicePath;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  NVME_ADMIN_CONTROLLER_DATA                ControllerData;
  CHAR16                                    *Description;
  CHAR16                                    *Char;
  UINTN                                     Index;

  Status = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath.DevPath);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->LocateDevicePath (&gEfiNvmExpressPassThruProtocolGuid, &DevicePath.DevPath, &Handle);
  if (EFI_ERROR (Status) ||
      (DevicePathType (DevicePath.DevPath) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (DevicePath.DevPath) != MSG_NVME_NAMESPACE_DP))
  {
    //
    // Do not return description when the Handle is not a child of NVME controller.
    //
    return NULL;
  }

  //
  // Send ADMIN_IDENTIFY command to NVME controller to get the model and serial number.
  //
  Status = gBS->HandleProtocol (Handle, &gEfiNvmExpressPassThruProtocolGuid, (VOID **)&NvmePassthru);
  ASSERT_EFI_ERROR (Status);

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  //
  // According to Nvm Express 1.1 spec Figure 38, When not used, the field shall be cleared to 0h.
  // For the Identify command, the Namespace Identifier is only used for the Namespace data structure.
  //
  Command.Nsid                 = 0;
  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.TransferBuffer = &ControllerData;
  CommandPacket.TransferLength = sizeof (ControllerData);
  CommandPacket.CommandTimeout = EFI_TIMER_PERIOD_SECONDS (5);
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a controller
  //
  Command.Cdw10 = 1;
  Command.Flags = CDW10_VALID;

  Status = NvmePassthru->PassThru (
                           NvmePassthru,
                           0,
                           &CommandPacket,
                           NULL
                           );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Description = AllocateZeroPool (
                  (ARRAY_SIZE (ControllerData.Mn) + 1
                   + ARRAY_SIZE (ControllerData.Sn) + 1
                   + MAXIMUM_VALUE_CHARACTERS + 1
                  ) * sizeof (CHAR16)
                  );
  if (Description != NULL) {
    Char = Description;
    for (Index = 0; Index < ARRAY_SIZE (ControllerData.Mn); Index++) {
      *(Char++) = (CHAR16)ControllerData.Mn[Index];
    }

    *(Char++) = L' ';
    for (Index = 0; Index < ARRAY_SIZE (ControllerData.Sn); Index++) {
      *(Char++) = (CHAR16)ControllerData.Sn[Index];
    }

    *(Char++) = L' ';
    UnicodeValueToStringS (
      Char,
      sizeof (CHAR16) * (MAXIMUM_VALUE_CHARACTERS + 1),
      0,
      DevicePath.NvmeNamespace->NamespaceId,
      0
      );
    BmEliminateExtraSpaces (Description);
  }

  return Description;
}

/**
  Return the boot description for the controller based on the type.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetMiscDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *Description;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;

  switch (BmDevicePathType (DevicePathFromHandle (Handle))) {
    case BmAcpiFloppyBoot:
      Description = L"Floppy";
      break;

    case BmMessageAtapiBoot:
    case BmMessageSataBoot:
      Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
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
      Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
      if (!EFI_ERROR (Status)) {
        Description = BlockIo->Media->RemovableMedia ? L"Removable Disk" : L"Hard Drive";
      } else {
        Description = L"Misc Device";
      }

      break;

    default:
      Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
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
  LIST_ENTRY                 *Link;
  BM_BOOT_DESCRIPTION_ENTRY  *Entry;

  for ( Link = GetFirstNode (&mPlatformBootDescriptionHandlers)
        ; !IsNull (&mPlatformBootDescriptionHandlers, Link)
        ; Link = GetNextNode (&mPlatformBootDescriptionHandlers, Link)
        )
  {
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

BM_GET_BOOT_DESCRIPTION  mBmBootDescriptionHandlers[] = {
  BmGetUsbDescription,
  BmGetDescriptionFromDiskInfo,
  BmGetNetworkDescription,
  BmGetLoadFileDescription,
  BmGetNvmeDescription,
  BmGetMiscDescription
};

/**
  Return the boot description for the controller.

  @param Handle                Controller handle.

  @return  The description string or NULL if the string could not be created.
**/
CHAR16 *
BmGetBootDescription (
  IN EFI_HANDLE  Handle
  )
{
  LIST_ENTRY                 *Link;
  BM_BOOT_DESCRIPTION_ENTRY  *Entry;
  CHAR16                     *Description;
  CHAR16                     *DefaultDescription;
  CHAR16                     *Temp;
  UINTN                      Index;

  //
  // Firstly get the default boot description
  //
  DefaultDescription = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mBmBootDescriptionHandlers); Index++) {
    DefaultDescription = mBmBootDescriptionHandlers[Index](Handle);
    if (DefaultDescription != NULL) {
      //
      // Avoid description confusion between UEFI & Legacy boot option by adding "UEFI " prefix
      // ONLY for core provided boot description handler.
      //
      Temp = AllocatePool (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix));
      if (Temp == NULL) {
        ASSERT (Temp != NULL);
        return NULL;
      }

      StrCpyS (Temp, (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix)) / sizeof (CHAR16), mBmUefiPrefix);
      StrCatS (Temp, (StrSize (DefaultDescription) + sizeof (mBmUefiPrefix)) / sizeof (CHAR16), DefaultDescription);
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
        )
  {
    Entry       = CR (Link, BM_BOOT_DESCRIPTION_ENTRY, Link, BM_BOOT_DESCRIPTION_ENTRY_SIGNATURE);
    Description = Entry->Handler (Handle, DefaultDescription);
    if (Description != NULL) {
      FreePool (DefaultDescription);
      return Description;
    }
  }

  return DefaultDescription;
}

/**
  Enumerate all boot option descriptions and append " 2"/" 3"/... to make
  unique description.

  @param BootOptions            Array of boot options.
  @param BootOptionCount        Count of boot options.
**/
VOID
BmMakeBootOptionDescriptionUnique (
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  UINTN                         BootOptionCount
  )
{
  UINTN    Base;
  UINTN    Index;
  UINTN    DescriptionSize;
  UINTN    MaxSuffixSize;
  BOOLEAN  *Visited;
  UINTN    MatchCount;

  if (BootOptionCount == 0) {
    return;
  }

  //
  // Calculate the maximum buffer size for the number suffix.
  // The initial sizeof (CHAR16) is for the blank space before the number.
  //
  MaxSuffixSize = sizeof (CHAR16);
  for (Index = BootOptionCount; Index != 0; Index = Index / 10) {
    MaxSuffixSize += sizeof (CHAR16);
  }

  Visited = AllocateZeroPool (sizeof (BOOLEAN) * BootOptionCount);
  if (Visited == NULL) {
    ASSERT (Visited != NULL);
    return;
  }

  for (Base = 0; Base < BootOptionCount; Base++) {
    if (!Visited[Base]) {
      MatchCount      = 1;
      Visited[Base]   = TRUE;
      DescriptionSize = StrSize (BootOptions[Base].Description);
      for (Index = Base + 1; Index < BootOptionCount; Index++) {
        if (!Visited[Index] && (StrCmp (BootOptions[Base].Description, BootOptions[Index].Description) == 0)) {
          Visited[Index] = TRUE;
          MatchCount++;
          FreePool (BootOptions[Index].Description);
          BootOptions[Index].Description = AllocatePool (DescriptionSize + MaxSuffixSize);
          UnicodeSPrint (
            BootOptions[Index].Description,
            DescriptionSize + MaxSuffixSize,
            L"%s %d",
            BootOptions[Base].Description,
            MatchCount
            );
        }
      }
    }
  }

  FreePool (Visited);
}
