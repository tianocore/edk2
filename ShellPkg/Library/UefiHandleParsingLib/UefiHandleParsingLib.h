/** @file
  Provides interface to advanced shell functionality for parsing both handle and protocol database.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2013-2016 Hewlett-Packard Development Company, L.P.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UEFI_HANDLE_PARSING_LIB_INTERNAL_H_
#define _UEFI_HANDLE_PARSING_LIB_INTERNAL_H_

#include <Uefi.h>

#include <Guid/FileInfo.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/StandardErrorDevice.h>
#include <Guid/GlobalVariable.h>
#include <Guid/Gpt.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/ShellLibHiiGuid.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DriverConfiguration2.h>
#include <Protocol/DriverConfiguration.h>
#include <Protocol/DriverDiagnostics2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/ComponentName.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/DevicePathUtilities.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/SerialIo.h>
#include <Protocol/AbsolutePointer.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidActive.h>
#include <Protocol/EdidOverride.h>
#include <Protocol/LoadFile.h>
#include <Protocol/LoadFile2.h>
#include <Protocol/TapeIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/ScsiPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/ScsiIo.h>
#include <Protocol/IScsiInitiatorName.h>
#include <Protocol/UsbIo.h>
#include <Protocol/UsbHostController.h>
#include <Protocol/Usb2HostController.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/DebugPort.h>
#include <Protocol/Decompress.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/Ebc.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/PxeBaseCodeCallBack.h>
#include <Protocol/Bis.h>
#include <Protocol/ManagedNetwork.h>
#include <Protocol/Arp.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Ip4.h>
#include <Protocol/Ip4Config.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/Udp4.h>
#include <Protocol/Mtftp4.h>
#include <Protocol/AuthenticationInfo.h>
#include <Protocol/Hash.h>
#include <Protocol/HiiFont.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/DeviceIo.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/UgaIo.h>
#include <Protocol/DriverConfiguration.h>
#include <Protocol/DriverConfiguration2.h>
#include <Protocol/DevicePathUtilities.h>
//#include <Protocol/FirmwareVolume.h>
//#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DriverFamilyOverride.h>
#include <Protocol/Pcd.h>
#include <Protocol/TcgService.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/Ip6.h>
#include <Protocol/Ip6Config.h>
#include <Protocol/Mtftp6.h>
#include <Protocol/Dhcp6.h>
#include <Protocol/Udp6.h>
#include <Protocol/Tcp6.h>
#include <Protocol/VlanConfig.h>
#include <Protocol/Eap.h>
#include <Protocol/EapManagement.h>
#include <Protocol/Ftp4.h>
#include <Protocol/IpSecConfig.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/DeferredImageLoad.h>
#include <Protocol/UserCredential.h>
#include <Protocol/UserManager.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/IpSec.h>
#include <Protocol/Kms.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/StorageSecurityCommand.h>
#include <Protocol/UserCredential2.h>
#include <Protocol/IdeControllerInit.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/AdapterInformation.h>
#include <Protocol/ShellDynamicCommand.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/PartitionInfo.h>

#include <Library/HandleParsingLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#define   EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V1   1
#define   EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION_V2   2

///
/// EFI_FIRMWARE_IMAGE_DESCRIPTOR in UEFI spec < 2.4a
///
typedef struct {
  ///
  /// A unique number identifying the firmware image within the device.  The number is
  /// between 1 and DescriptorCount.
  ///
  UINT8                            ImageIndex;
  ///
  /// A unique number identifying the firmware image type.
  ///
  EFI_GUID                         ImageTypeId;
  ///
  /// A unique number identifying the firmware image.
  ///
  UINT64                           ImageId;
  ///
  /// A pointer to a null-terminated string representing the firmware image name.
  ///
  CHAR16                           *ImageIdName;
  ///
  /// Identifies the version of the device firmware. The format is vendor specific and new
  /// version must have a greater value than an old version.
  ///
  UINT32                           Version;
  ///
  /// A pointer to a null-terminated string representing the firmware image version name.
  ///
  CHAR16                           *VersionName;
  ///
  /// Size of the image in bytes.  If size=0, then only ImageIndex and ImageTypeId are valid.
  ///
  UINTN                            Size;
  ///
  /// Image attributes that are supported by this device.  See 'Image Attribute Definitions'
  /// for possible returned values of this parameter.  A value of 1 indicates the attribute is
  /// supported and the current setting value is indicated in AttributesSetting.  A
  /// value of 0 indicates the attribute is not supported and the current setting value in
  /// AttributesSetting is meaningless.
  ///
  UINT64                           AttributesSupported;
  ///
  /// Image attributes.  See 'Image Attribute Definitions' for possible returned values of
  /// this parameter.
  ///
  UINT64                           AttributesSetting;
  ///
  /// Image compatibilities.  See 'Image Compatibility Definitions' for possible returned
  /// values of this parameter.
  ///
  UINT64                           Compatibilities;
} EFI_FIRMWARE_IMAGE_DESCRIPTOR_V1;


///
/// EFI_FIRMWARE_IMAGE_DESCRIPTOR in UEFI spec > 2.4a and < 2.5
///
typedef struct {
  ///
  /// A unique number identifying the firmware image within the device.  The number is
  /// between 1 and DescriptorCount.
  ///
  UINT8                            ImageIndex;
  ///
  /// A unique number identifying the firmware image type.
  ///
  EFI_GUID                         ImageTypeId;
  ///
  /// A unique number identifying the firmware image.
  ///
  UINT64                           ImageId;
  ///
  /// A pointer to a null-terminated string representing the firmware image name.
  ///
  CHAR16                           *ImageIdName;
  ///
  /// Identifies the version of the device firmware. The format is vendor specific and new
  /// version must have a greater value than an old version.
  ///
  UINT32                           Version;
  ///
  /// A pointer to a null-terminated string representing the firmware image version name.
  ///
  CHAR16                           *VersionName;
  ///
  /// Size of the image in bytes.  If size=0, then only ImageIndex and ImageTypeId are valid.
  ///
  UINTN                            Size;
  ///
  /// Image attributes that are supported by this device.  See 'Image Attribute Definitions'
  /// for possible returned values of this parameter.  A value of 1 indicates the attribute is
  /// supported and the current setting value is indicated in AttributesSetting.  A
  /// value of 0 indicates the attribute is not supported and the current setting value in
  /// AttributesSetting is meaningless.
  ///
  UINT64                           AttributesSupported;
  ///
  /// Image attributes.  See 'Image Attribute Definitions' for possible returned values of
  /// this parameter.
  ///
  UINT64                           AttributesSetting;
  ///
  /// Image compatibilities.  See 'Image Compatibility Definitions' for possible returned
  /// values of this parameter.
  ///
  UINT64                           Compatibilities;
  ///
  /// Describes the lowest ImageDescriptor version that the device will accept. Only
  /// present in version 2 or higher.
  UINT32                           LowestSupportedImageVersion;
} EFI_FIRMWARE_IMAGE_DESCRIPTOR_V2;

typedef struct {
  LIST_ENTRY  Link;
  EFI_HANDLE  TheHandle;
  UINTN       TheIndex;
}HANDLE_LIST;

typedef struct {
  HANDLE_LIST   List;
  UINTN         NextIndex;
} HANDLE_INDEX_LIST;

typedef
CHAR16 *
(EFIAPI *DUMP_PROTOCOL_INFO)(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose
  );

typedef struct _GUID_INFO_BLOCK{
  EFI_STRING_ID                 StringId;
  EFI_GUID                      *GuidId;
  DUMP_PROTOCOL_INFO            DumpInfo;
} GUID_INFO_BLOCK;

#endif

