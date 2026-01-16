/** @file
  Provides firmware device specific services to support updates of a firmware
  image stored in a firmware device.

  This implementation uses SmmStoreLib to update the whole flash chip.  For
  this to work correctly Intel ME or an equivalent need to be disabled and all
  flash chip protections need to be lifted (rather, not applied) if coreboot
  finds any capsules during initialization.  It is done to allow updating ME or
  changing chip's layout in a new firmware (e.g., to allocate more space for
  the BIOS region).  In the future, this can be made more flexible, say, by
  parsing coreboot's FMAP and only updating some regions.  Such functionality
  probably need embedding more knowledge about specific firmware image via a
  new library to be customized for specific use-cases.

  Copyright (c) Microsoft Corporation.<BR>
  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, 3mdeb Sp. z o.o. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Guid/FirmwareInfoGuid.h>
#include <Guid/SystemResourceTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpDeviceLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmStoreLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Coreboot.h>

//
// Minimal FMAP parsing to locate and compare the flash map before updating.
//
#define FMAP_SIGNATURE  "__FMAP__"

#pragma pack(1)
typedef struct {
  CHAR8   Signature[8];
  UINT8   VerMajor;
  UINT8   VerMinor;
  UINT64  Base;
  UINT32  Size;
  CHAR8   Name[32];
  UINT16  AreaCount;
} FMAP_HEADER;

typedef struct {
  UINT32  Offset;
  UINT32  Size;
  CHAR8   Name[32];
  UINT16  Flags;
} FMAP_AREA;
#pragma pack()

//
// Simple manifest that lists FMAP region names to be flashed. The manifest is
// appended at the end of the capsule payload and is ignored when absent.
//
#define REGION_MANIFEST_SIGNATURE  SIGNATURE_32('R','M','A','P')
#define REGION_MANIFEST_VERSION    1

typedef struct {
  UINT32  Signature;
  UINT16  Version;
  UINT16  EntryCount;
} REGION_MANIFEST_TRAILER;

typedef struct {
  CHAR8  RegionName[16];
} REGION_MANIFEST_ENTRY;

/**
  This function requests firmware information on the first call, caches it and
  returns on all calls afterwards.

  @param[out] Info  Place to store a pointer to firmware information.

  @retval EFI_SUCCESS            Info points to firmware information.
  @retval EFI_INVALID_PARAMETER  Info is NULL.
**/
STATIC
EFI_STATUS
GetFwInfo (
  OUT FIRMWARE_INFO  **Info
  )
{
  STATIC FIRMWARE_INFO  Storage;
  STATIC BOOLEAN        Initialized;

  EFI_HOB_GUID_TYPE  *GuidHob;

  if (Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Initialized) {
    GuidHob = GetFirstGuidHob (&gEfiFirmwareInfoHobGuid);
    if (GuidHob == NULL) {
      DEBUG ((DEBUG_ERROR, "%a(): failed to query firmware information\n", __func__));
      return EFI_UNSUPPORTED;
    }

    Storage     = *(FIRMWARE_INFO *)GET_GUID_HOB_DATA (GuidHob);
    Initialized = TRUE;
  }

  *Info = &Storage;
  return EFI_SUCCESS;
}

/**
  Locate an FMAP header within a memory buffer.

  @param[in]  Image       Start of the buffer to scan.
  @param[in]  ImageSize   Size of the buffer.
  @param[out] FmapOffset  Offset of the FMAP header if found.
  @param[out] FmapLength  Total length of the FMAP (header + areas).

  @retval EFI_SUCCESS      FMAP was found and validated.
  @retval EFI_NOT_FOUND    FMAP not present or malformed.
**/
STATIC
EFI_STATUS
LocateFmapInImage (
  IN  CONST UINT8  *Image,
  IN  UINTN        ImageSize,
  OUT UINTN        *FmapOffset,
  OUT UINTN        *FmapLength
  )
{
  UINTN         Offset;
  CONST UINTN   HeaderSize = sizeof (FMAP_HEADER);
  CONST UINTN   AreaSize   = sizeof (FMAP_AREA);
  CONST UINT32  MinAreas   = 1;

  if ((Image == NULL) || (FmapOffset == NULL) || (FmapLength == NULL)) {
    return EFI_NOT_FOUND;
  }

  if (ImageSize < HeaderSize) {
    return EFI_NOT_FOUND;
  }

  for (Offset = 0; Offset + HeaderSize <= ImageSize; ++Offset) {
    CONST FMAP_HEADER  *Hdr = (CONST FMAP_HEADER *)(Image + Offset);

    if (CompareMem (Hdr->Signature, FMAP_SIGNATURE, sizeof (Hdr->Signature)) != 0) {
      continue;
    }

    if (Hdr->AreaCount < MinAreas) {
      continue;
    }

    //
    // Validate that all areas fit in the buffer.
    //
    if (Offset + HeaderSize + (Hdr->AreaCount * AreaSize) > ImageSize) {
      continue;
    }

    *FmapOffset = Offset;
    *FmapLength = HeaderSize + (Hdr->AreaCount * AreaSize);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Locate the manifest at the end of the image.

  @param[in]  Image                 Capsule payload buffer.
  @param[in]  ImageSize             Size of the payload buffer.
  @param[out] EntryCount            Number of manifest entries.
  @param[out] Entries               Pointer to the first manifest entry.
  @param[out] FirmwareImageSize     Size of the firmware image excluding the manifest.

  @retval EFI_SUCCESS     Manifest found and validated.
  @retval EFI_NOT_FOUND   Manifest not present or malformed.
**/
STATIC
EFI_STATUS
LocateRegionManifest (
  IN  CONST UINT8               *Image,
  IN  UINTN                     ImageSize,
  OUT UINTN                     *EntryCount,
  OUT CONST REGION_MANIFEST_ENTRY  **Entries,
  OUT UINTN                     *FirmwareImageSize
  )
{
  CONST REGION_MANIFEST_TRAILER  *Trailer;
  UINTN                          EntriesSize;
  UINTN                          ManifestStart;

  if ((Image == NULL) || (EntryCount == NULL) || (Entries == NULL) || (FirmwareImageSize == NULL)) {
    return EFI_NOT_FOUND;
  }

  if (ImageSize < sizeof (REGION_MANIFEST_TRAILER)) {
    return EFI_NOT_FOUND;
  }

  Trailer = (CONST REGION_MANIFEST_TRAILER *)(Image + ImageSize - sizeof (REGION_MANIFEST_TRAILER));
  if ((Trailer->Signature != REGION_MANIFEST_SIGNATURE) || (Trailer->Version != REGION_MANIFEST_VERSION)) {
    return EFI_NOT_FOUND;
  }

  EntriesSize   = (UINTN)Trailer->EntryCount * sizeof (REGION_MANIFEST_ENTRY);
  ManifestStart = ImageSize - sizeof (REGION_MANIFEST_TRAILER) - EntriesSize;
  if (ManifestStart > ImageSize) {
    return EFI_NOT_FOUND;
  }

  *EntryCount        = Trailer->EntryCount;
  *Entries           = (CONST REGION_MANIFEST_ENTRY *)(Image + ManifestStart);
  *FirmwareImageSize = ManifestStart;

  return EFI_SUCCESS;
}

/**
  Find an FMAP area by name.

  @param[in]  FmapHeader     Pointer to FMAP header.
  @param[in]  Areas          Pointer to the first FMAP area.
  @param[in]  AreaCount      Number of FMAP areas.
  @param[in]  Name           Region name to look for (16-byte, zero padded).
  @param[out] RegionOffset   Flash offset of the region.
  @param[out] RegionSize     Size of the region.

  @retval EFI_SUCCESS     Region found.
  @retval EFI_NOT_FOUND   Region not found.
**/
STATIC
EFI_STATUS
FindFmapRegion (
  IN  CONST FMAP_HEADER  *FmapHeader,
  IN  CONST FMAP_AREA    *Areas,
  IN  UINTN              AreaCount,
  IN  CONST CHAR8        Name[16],
  OUT UINTN              *RegionOffset,
  OUT UINTN              *RegionSize
  )
{
  UINTN  Index;
  UINTN  CharIndex;

  for (Index = 0; Index < AreaCount; ++Index) {
    //
    // Manifest names are 16-byte, zero-padded strings, while FMAP uses 32-byte
    // names. Compare the first 16 bytes, ensuring the FMAP name does not have
    // additional non-zero characters beyond the manifest name.
    //
    for (CharIndex = 0; CharIndex < 16; ++CharIndex) {
      if (Areas[Index].Name[CharIndex] != Name[CharIndex]) {
        break;
      }

      if (Name[CharIndex] == 0) {
        break;
      }
    }

    if (CharIndex == 16) {
      if (Areas[Index].Name[16] != 0) {
        continue;
      }
    } else {
      if (Areas[Index].Name[CharIndex] != 0) {
        continue;
      }

      for (; CharIndex < 16; ++CharIndex) {
        if (Name[CharIndex] != 0) {
          break;
        }
      }

      if (CharIndex != 16) {
        continue;
      }
    }

    *RegionOffset = (UINTN)FmapHeader->Base + Areas[Index].Offset;
    *RegionSize   = Areas[Index].Size;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Provide a function to install the Firmware Management Protocol instance onto a
  device handle when the device is managed by a driver that follows the UEFI
  Driver Model.  If the device is not managed by a driver that follows the UEFI
  Driver Model, then EFI_UNSUPPORTED is returned.

  @param[in] FmpInstaller  Function that installs the Firmware Management
                           Protocol.

  @retval EFI_SUCCESS      The device is managed by a driver that follows the
                           UEFI Driver Model.  FmpInstaller must be called on
                           each Driver Binding Start().
  @retval EFI_UNSUPPORTED  The device is not managed by a driver that follows
                           the UEFI Driver Model.
  @retval other            The Firmware Management Protocol for this firmware
                           device is not installed.  The firmware device is
                           still locked using FmpDeviceLock().
**/
EFI_STATUS
EFIAPI
RegisterFmpInstaller (
  IN FMP_DEVICE_LIB_REGISTER_FMP_INSTALLER  Function
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Provide a function to uninstall the Firmware Management Protocol instance from a
  device handle when the device is managed by a driver that follows the UEFI
  Driver Model.  If the device is not managed by a driver that follows the UEFI
  Driver Model, then EFI_UNSUPPORTED is returned.

  @param[in] FmpUninstaller  Function that installs the Firmware Management
                             Protocol.

  @retval EFI_SUCCESS      The device is managed by a driver that follows the
                           UEFI Driver Model.  FmpUninstaller must be called on
                           each Driver Binding Stop().
  @retval EFI_UNSUPPORTED  The device is not managed by a driver that follows
                           the UEFI Driver Model.
  @retval other            The Firmware Management Protocol for this firmware
                           device is not installed.  The firmware device is
                           still locked using FmpDeviceLock().
**/
EFI_STATUS
EFIAPI
RegisterFmpUninstaller (
  IN FMP_DEVICE_LIB_REGISTER_FMP_UNINSTALLER  Function
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Set the device context for the FmpDeviceLib services when the device is
  managed by a driver that follows the UEFI Driver Model.  If the device is not
  managed by a driver that follows the UEFI Driver Model, then EFI_UNSUPPORTED
  is returned.  Once a device context is set, the FmpDeviceLib services
  operate on the currently set device context.

  @param[in]      Handle   Device handle for the FmpDeviceLib services.
                           If Handle is NULL, then Context is freed.
  @param[in, out] Context  Device context for the FmpDeviceLib services.
                           If Context is NULL, then a new context is allocated
                           for Handle and the current device context is set and
                           returned in Context.  If Context is not NULL, then
                           the current device context is set.

  @retval EFI_SUCCESS      The device is managed by a driver that follows the
                           UEFI Driver Model.
  @retval EFI_UNSUPPORTED  The device is not managed by a driver that follows
                           the UEFI Driver Model.
  @retval other            The Firmware Management Protocol for this firmware
                           device is not installed.  The firmware device is
                           still locked using FmpDeviceLock().
**/
EFI_STATUS
EFIAPI
FmpDeviceSetContext (
  IN EFI_HANDLE  Handle,
  IN OUT VOID    **Context
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Returns the size, in bytes, of the firmware image currently stored in the
  firmware device.  This function is used to by the GetImage() and
  GetImageInfo() services of the Firmware Management Protocol.  If the image
  size can not be determined from the firmware device, then 0 must be returned.

  @param[out] Size  Pointer to the size, in bytes, of the firmware image
                    currently stored in the firmware device.

  @retval EFI_SUCCESS            The size of the firmware image currently
                                 stored in the firmware device was returned.
  @retval EFI_INVALID_PARAMETER  Size is NULL.
  @retval EFI_UNSUPPORTED        The firmware device does not support reporting
                                 the size of the currently stored firmware image.
  @retval EFI_DEVICE_ERROR       An error occurred attempting to determine the
                                 size of the firmware image currently stored in
                                 in the firmware device.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetSize (
  OUT UINTN  *Size
  )
{
  EFI_STATUS     Status;
  FIRMWARE_INFO  *FwInfo;

  if (Size == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFwInfo (&FwInfo);
  if (!EFI_ERROR (Status)) {
    *Size = FwInfo->ImageSize;
  }

  return Status;
}

/**
  Returns the GUID value used to fill in the ImageTypeId field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  If EFI_UNSUPPORTED is returned,
  then the ImageTypeId field is set to gEfiCallerIdGuid.  If EFI_SUCCESS is
  returned, then ImageTypeId is set to the Guid returned from this function.

  @param[out] Guid  Double pointer to a GUID value that is updated to point to
                    to a GUID value.  The GUID value is not allocated and must
                    not be modified or freed by the caller.

  @retval EFI_SUCCESS      EFI_FIRMWARE_IMAGE_DESCRIPTOR ImageTypeId GUID is set
                           to the returned Guid value.
  @retval EFI_UNSUPPORTED  EFI_FIRMWARE_IMAGE_DESCRIPTOR ImageTypeId GUID is set
                           to gEfiCallerIdGuid.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetImageTypeIdGuidPtr (
  OUT EFI_GUID  **Guid
  )
{
  EFI_STATUS     Status;
  FIRMWARE_INFO  *FwInfo;

  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFwInfo (&FwInfo);
  if (!EFI_ERROR (Status)) {
    *Guid = &FwInfo->Type;
  }

  return Status;
}

/**
  Returns values used to fill in the AttributesSupported and AttributesSettings
  fields of the EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the
  GetImageInfo() service of the Firmware Management Protocol.  The following
  bit values from the Firmware Management Protocol may be combined:
    IMAGE_ATTRIBUTE_IMAGE_UPDATABLE
    IMAGE_ATTRIBUTE_RESET_REQUIRED
    IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED
    IMAGE_ATTRIBUTE_IN_USE
    IMAGE_ATTRIBUTE_UEFI_IMAGE

  @param[out] Supported  Attributes supported by this firmware device.
  @param[out] Setting    Attributes settings for this firmware device.

  @retval EFI_SUCCESS            The attributes supported by the firmware
                                 device were returned.
  @retval EFI_INVALID_PARAMETER  Supported is NULL.
  @retval EFI_INVALID_PARAMETER  Setting is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetAttributes (
  OUT UINT64  *Supported,
  OUT UINT64  *Setting
  )
{
  if ((Supported == NULL) || (Setting == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Supported = IMAGE_ATTRIBUTE_IMAGE_UPDATABLE |
               IMAGE_ATTRIBUTE_RESET_REQUIRED |
               IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED |
               IMAGE_ATTRIBUTE_IN_USE;
  *Setting = *Supported;
  return EFI_SUCCESS;
}

/**
  Returns the value used to fill in the LowestSupportedVersion field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  If EFI_SUCCESS is returned, then
  the firmware device supports a method to report the LowestSupportedVersion
  value from the currently stored firmware image.  If the value can not be
  reported for the firmware image currently stored in the firmware device, then
  EFI_UNSUPPORTED must be returned.  EFI_DEVICE_ERROR is returned if an error
  occurs attempting to retrieve the LowestSupportedVersion value for the
  currently stored firmware image.

  @note It is recommended that all firmware devices support a method to report
        the LowestSupportedVersion value from the currently stored firmware
        image.

  @param[out] LowestSupportedVersion  LowestSupportedVersion value retrieved
                                      from the currently stored firmware image.

  @retval EFI_SUCCESS       The lowest supported version of currently stored
                            firmware image was returned in LowestSupportedVersion.
  @retval EFI_UNSUPPORTED   The firmware device does not support a method to
                            report the lowest supported version of the currently
                            stored firmware image.
  @retval EFI_DEVICE_ERROR  An error occurred attempting to retrieve the lowest
                            supported version of the currently stored firmware
                            image.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetLowestSupportedVersion (
  OUT UINT32  *LowestSupportedVersion
  )
{
  EFI_STATUS     Status;
  FIRMWARE_INFO  *FwInfo;

  if (LowestSupportedVersion == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFwInfo (&FwInfo);
  if (!EFI_ERROR (Status)) {
    *LowestSupportedVersion = FwInfo->LowestSupportedVersion;
  }

  return Status;
}

/**
  Returns the Null-terminated Unicode string that is used to fill in the
  VersionName field of the EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is
  returned by the GetImageInfo() service of the Firmware Management Protocol.
  The returned string must be allocated using EFI_BOOT_SERVICES.AllocatePool().

  @note It is recommended that all firmware devices support a method to report
        the VersionName string from the currently stored firmware image.

  @param[out] VersionString  The version string retrieved from the currently
                             stored firmware image.

  @retval EFI_SUCCESS            The version string of currently stored
                                 firmware image was returned in Version.
  @retval EFI_INVALID_PARAMETER  VersionString is NULL.
  @retval EFI_UNSUPPORTED        The firmware device does not support a method
                                 to report the version string of the currently
                                 stored firmware image.
  @retval EFI_DEVICE_ERROR       An error occurred attempting to retrieve the
                                 version string of the currently stored
                                 firmware image.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to allocate the
                                 buffer for the version string of the currently
                                 stored firmware image.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetVersionString (
  OUT CHAR16  **VersionString
  )
{
  EFI_STATUS     Status;
  FIRMWARE_INFO  *FwInfo;

  if (VersionString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFwInfo (&FwInfo);
  if (!EFI_ERROR (Status)) {
    *VersionString = (CHAR16 *)AllocateCopyPool (
                                 sizeof (FwInfo->VersionStr),
                                 FwInfo->VersionStr
                                 );
    if (*VersionString == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  return Status;
}

/**
  Returns the value used to fill in the Version field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  If EFI_SUCCESS is returned, then
  the firmware device supports a method to report the Version value from the
  currently stored firmware image.  If the value can not be reported for the
  firmware image currently stored in the firmware device, then EFI_UNSUPPORTED
  must be returned.  EFI_DEVICE_ERROR is returned if an error occurs attempting
  to retrieve the LowestSupportedVersion value for the currently stored firmware
  image.

  @note It is recommended that all firmware devices support a method to report
        the Version value from the currently stored firmware image.

  @param[out] Version  The version value retrieved from the currently stored
                       firmware image.

  @retval EFI_SUCCESS       The version of currently stored firmware image was
                            returned in Version.
  @retval EFI_UNSUPPORTED   The firmware device does not support a method to
                            report the version of the currently stored firmware
                            image.
  @retval EFI_DEVICE_ERROR  An error occurred attempting to retrieve the version
                            of the currently stored firmware image.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetVersion (
  OUT UINT32  *Version
  )
{
  EFI_STATUS     Status;
  FIRMWARE_INFO  *FwInfo;

  if (Version == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetFwInfo (&FwInfo);
  if (!EFI_ERROR (Status)) {
    *Version = FwInfo->Version;
  }

  return Status;
}

/**
  Returns the value used to fill in the HardwareInstance field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  If EFI_SUCCESS is returned, then
  the firmware device supports a method to report the HardwareInstance value.
  If the value can not be reported for the firmware device, then EFI_UNSUPPORTED
  must be returned.  EFI_DEVICE_ERROR is returned if an error occurs attempting
  to retrieve the HardwareInstance value for the firmware device.

  @param[out] HardwareInstance  The hardware instance value for the firmware
                                device.

  @retval EFI_SUCCESS       The hardware instance for the current firmware
                            device is returned in HardwareInstance.
  @retval EFI_UNSUPPORTED   The firmware device does not support a method to
                            report the hardware instance value.
  @retval EFI_DEVICE_ERROR  An error occurred attempting to retrieve the hardware
                            instance value.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetHardwareInstance (
  OUT UINT64  *HardwareInstance
  )
{
  if (HardwareInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *HardwareInstance = 0;
  return EFI_SUCCESS;
}

/**
  Returns a copy of the firmware image currently stored in the firmware device.

  @note It is recommended that all firmware devices support a method to retrieve
        a copy currently stored firmware image.  This can be used to support
        features such as recovery and rollback.

  @param[out]     Image     Pointer to a caller allocated buffer where the
                            currently stored firmware image is copied to.
  @param[in, out] ImageSize Pointer the size, in bytes, of the Image buffer.
                            On return, points to the size, in bytes, of firmware
                            image currently stored in the firmware device.

  @retval EFI_SUCCESS            Image contains a copy of the firmware image
                                 currently stored in the firmware device, and
                                 ImageSize contains the size, in bytes, of the
                                 firmware image currently stored in the
                                 firmware device.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small
                                 to hold the firmware image currently stored in
                                 the firmware device. The buffer size required
                                 is returned in ImageSize.
  @retval EFI_INVALID_PARAMETER  The Image is NULL.
  @retval EFI_INVALID_PARAMETER  The ImageSize is NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_DEVICE_ERROR       An error occurred attempting to retrieve the
                                 firmware image currently stored in the firmware
                                 device.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetImage (
  OUT    VOID   *Image,
  IN OUT UINTN  *ImageSize
  )
{
  //
  // This seems useful only if FMP device is part of the running firmware.
  //
  return EFI_UNSUPPORTED;
}

/**
  Checks if a new firmware image is valid for the firmware device.  This
  function allows firmware update operation to validate the firmware image
  before FmpDeviceSetImage() is called.

  @param[in]  Image           Points to a new firmware image.
  @param[in]  ImageSize       Size, in bytes, of a new firmware image.
  @param[out] ImageUpdatable  Indicates if a new firmware image is valid for
                              a firmware update to the firmware device.  The
                              following values from the Firmware Management
                              Protocol are supported:
                                IMAGE_UPDATABLE_VALID
                                IMAGE_UPDATABLE_INVALID
                                IMAGE_UPDATABLE_INVALID_TYPE
                                IMAGE_UPDATABLE_INVALID_OLD
                                IMAGE_UPDATABLE_VALID_WITH_VENDOR_CODE

  @retval EFI_SUCCESS            The image was successfully checked.  Additional
                                 status information is returned in
                                 ImageUpdatable.
  @retval EFI_INVALID_PARAMETER  Image is NULL.
  @retval EFI_INVALID_PARAMETER  ImageUpdatable is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceCheckImage (
  IN  CONST VOID  *Image,
  IN  UINTN       ImageSize,
  OUT UINT32      *ImageUpdatable
  )
{
  UINT32  LastAttemptStatus;

  return FmpDeviceCheckImageWithStatus (Image, ImageSize, ImageUpdatable, &LastAttemptStatus);
}

/**
  Checks if a new firmware image is valid for the firmware device.  This
  function allows firmware update operation to validate the firmware image
  before FmpDeviceSetImage() is called.

  @param[in]  Image               Points to a new firmware image.
  @param[in]  ImageSize           Size, in bytes, of a new firmware image.
  @param[out] ImageUpdatable      Indicates if a new firmware image is valid for
                                  a firmware update to the firmware device.  The
                                  following values from the Firmware Management
                                  Protocol are supported:
                                    IMAGE_UPDATABLE_VALID
                                    IMAGE_UPDATABLE_INVALID
                                    IMAGE_UPDATABLE_INVALID_TYPE
                                    IMAGE_UPDATABLE_INVALID_OLD
                                    IMAGE_UPDATABLE_VALID_WITH_VENDOR_CODE
  @param[out] LastAttemptStatus   A pointer to a UINT32 that holds the last attempt
                                  status to report back to the ESRT table in case
                                  of error.

                                  The return status code must fall in the range of
                                  LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MIN_ERROR_CODE_VALUE to
                                  LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MAX_ERROR_CODE_VALUE.

                                  If the value falls outside this range, it will be converted
                                  to LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL.

  @retval EFI_SUCCESS            The image was successfully checked.  Additional
                                 status information is returned in
                                 ImageUpdatable.
  @retval EFI_INVALID_PARAMETER  Image is NULL.
  @retval EFI_INVALID_PARAMETER  ImageUpdatable is NULL.
**/
EFI_STATUS
EFIAPI
FmpDeviceCheckImageWithStatus (
  IN  CONST VOID  *Image,
  IN  UINTN       ImageSize,
  OUT UINT32      *ImageUpdatable,
  OUT UINT32      *LastAttemptStatus
  )
{
  EFI_STATUS  Status;
  UINTN       FwSize;
  UINTN       BlockSize;
  UINTN       ManifestEntries;
  UINTN       FirmwareImageSize;
  CONST REGION_MANIFEST_ENTRY  *ManifestList;

  if ((Image == NULL) || (ImageUpdatable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
  *ImageUpdatable    = IMAGE_UPDATABLE_INVALID;

  Status = FmpDeviceGetSize (&FwSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): FmpDeviceGetSize() failed with: %r\n", __func__, Status));
    return Status;
  }

  FirmwareImageSize = ImageSize;
  ManifestList      = NULL;
  ManifestEntries   = 0;

  if (ImageSize != FwSize) {
    Status = LocateRegionManifest (Image, ImageSize, &ManifestEntries, &ManifestList, &FirmwareImageSize);
    if (EFI_ERROR (Status) || (FirmwareImageSize != FwSize) || (ManifestEntries == 0)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a(): image size (0x%x) doesn't match firmware size (0x%x) and no valid manifest found\n",
        __func__,
        ImageSize,
        FwSize
        ));
      return EFI_ABORTED;
    }
  }

  Status = SmmStoreLibGetBlockSize (&BlockSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): SmmStoreLibGetBlockSize() failed with: %r\n", __func__, Status));
    return Status;
  }

  if (FwSize % BlockSize != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): firmware size (0x%x) is not a multiple of block size (0x%x)\n",
      __func__,
      FwSize,
      BlockSize
      ));
    return EFI_ABORTED;
  }

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
  *ImageUpdatable    = IMAGE_UPDATABLE_VALID;
  return EFI_SUCCESS;
}

/**
  Updates a firmware device with a new firmware image.  This function returns
  EFI_UNSUPPORTED if the firmware image is not updatable.  If the firmware image
  is updatable, the function should perform the following minimal validations
  before proceeding to do the firmware image update.
    - Validate that the image is a supported image for this firmware device.
      Return EFI_ABORTED if the image is not supported.  Additional details
      on why the image is not a supported image may be returned in AbortReason.
    - Validate the data from VendorCode if is not NULL.  Firmware image
      validation must be performed before VendorCode data validation.
      VendorCode data is ignored or considered invalid if image validation
      fails.  Return EFI_ABORTED if the VendorCode data is invalid.

  VendorCode enables vendor to implement vendor-specific firmware image update
  policy.  Null if the caller did not specify the policy or use the default
  policy.  As an example, vendor can implement a policy to allow an option to
  force a firmware image update when the abort reason is due to the new firmware
  image version is older than the current firmware image version or bad image
  checksum.  Sensitive operations such as those wiping the entire firmware image
  and render the device to be non-functional should be encoded in the image
  itself rather than passed with the VendorCode.  AbortReason enables vendor to
  have the option to provide a more detailed description of the abort reason to
  the caller.

  @param[in]  Image             Points to the new firmware image.
  @param[in]  ImageSize         Size, in bytes, of the new firmware image.
  @param[in]  VendorCode        This enables vendor to implement vendor-specific
                                firmware image update policy.  NULL indicates
                                the caller did not specify the policy or use the
                                default policy.
  @param[in]  Progress          A function used to report the progress of
                                updating the firmware device with the new
                                firmware image.
  @param[in]  CapsuleFwVersion  The version of the new firmware image from the
                                update capsule that provided the new firmware
                                image.
  @param[out] AbortReason       A pointer to a pointer to a Null-terminated
                                Unicode string providing more details on an
                                aborted operation. The buffer is allocated by
                                this function with
                                EFI_BOOT_SERVICES.AllocatePool().  It is the
                                caller's responsibility to free this buffer with
                                EFI_BOOT_SERVICES.FreePool().

  @retval EFI_SUCCESS            The firmware device was successfully updated
                                 with the new firmware image.
  @retval EFI_ABORTED            The operation is aborted.  Additional details
                                 are provided in AbortReason.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
**/
EFI_STATUS
EFIAPI
FmpDeviceSetImage (
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode        OPTIONAL,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress           OPTIONAL,
  IN  UINT32                                         CapsuleFwVersion,
  OUT CHAR16                                         **AbortReason
  )
{
  UINT32  LastAttemptStatus;

  return FmpDeviceSetImageWithStatus (
           Image,
           ImageSize,
           VendorCode,
           Progress,
           CapsuleFwVersion,
           AbortReason,
           &LastAttemptStatus
           );
}

/**
  Advances progress of the operation by a single step, converts the steps to
  percents and invokes a callback if there is one.

  @param[in]      Callback              External callback for progress reporting
                                        or NULL if there is none.
  @param[in]      TotalSteps            Total number of flashing steps.
  @param[in, out] Step                  Current step number.
  @param[in, out] ShouldReportProgress  A flag indicating whether progress needs
                                        to be reported.
**/
STATIC
VOID
IncrementProgress (
  IN      EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Callback OPTIONAL,
  IN      UINTN                                          TotalSteps,
  IN OUT  UINTN                                          *Step,
  IN OUT  BOOLEAN                                        *ShouldReportProgress
  )
{
  EFI_STATUS  Status;
  UINTN       Progress;

  if (!*ShouldReportProgress) {
    return;
  }

  if (Callback == NULL) {
    *ShouldReportProgress = FALSE;
    return;
  }

  ++*Step;
  Progress = (*Step * 100) / TotalSteps;

  //
  // Value of 0 means "progress reporting is not supported", so avoid using it.
  //
  if (Progress == 0) {
    Progress = 1;
  }

  Status = Callback (Progress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a(): progress callback failed with: %r\n", __func__, Status));
    *ShouldReportProgress = FALSE;
  }
}

/**
  Update a flash range using Image as the source.

  This helper preserves bytes outside the requested range within any partially
  covered flash block by read-modify-writing the full block.

  @retval EFI_SUCCESS            The range was updated successfully.
  @retval EFI_DEVICE_ERROR       A read/erase/write failed.
  @retval EFI_INVALID_PARAMETER  Input parameters were invalid.
**/
STATIC
EFI_STATUS
UpdateFlashRangeFromImage (
  IN      UINTN                                          BlockSize,
  IN      CONST UINT8                                    *Image,
  IN      UINTN                                          ImageSize,
  IN      UINTN                                          RangeOffset,
  IN      UINTN                                          RangeSize,
  IN OUT  VOID                                           *BlockBuffer,
  IN      EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress OPTIONAL,
  IN      UINTN                                          TotalSteps,
  IN OUT  UINTN                                          *Step,
  IN OUT  BOOLEAN                                        *ShouldReportProgress
  )
{
  EFI_STATUS  Status;
  UINTN       Offset;
  UINTN       RangeEnd;

  if ((Image == NULL) || (BlockBuffer == NULL) || (Step == NULL) || (ShouldReportProgress == NULL) || (BlockSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RangeSize == 0) {
    return EFI_SUCCESS;
  }

  if ((RangeOffset + RangeSize) > ImageSize) {
    return EFI_INVALID_PARAMETER;
  }

  RangeEnd = RangeOffset + RangeSize;

  for (Offset = RangeOffset; Offset < RangeEnd; ) {
    UINTN  Lba;
    UINTN  BlockBase;
    UINTN  StartInBlock;
    UINTN  EndInBlock;
    UINTN  SegmentLen;
    UINTN  NumBytes;
    UINT8  *FlashBlock;

    Lba       = Offset / BlockSize;
    BlockBase = Lba * BlockSize;

    StartInBlock = Offset - BlockBase;
    EndInBlock   = MIN (BlockSize, RangeEnd - BlockBase);
    SegmentLen   = EndInBlock - StartInBlock;

    FlashBlock = (UINT8 *)BlockBuffer;

    if ((StartInBlock == 0) && (EndInBlock == BlockSize)) {
      //
      // Whole-block update: ignore read errors for the compare optimization.
      //
      NumBytes = BlockSize;
      Status   = SmmStoreLibReadAnyBlock (Lba, 0, &NumBytes, FlashBlock);
      if (!EFI_ERROR (Status) && (NumBytes == BlockSize)) {
        if (CompareMem (FlashBlock, Image + BlockBase, BlockSize) == 0) {
          IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);
          IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);
          Offset += SegmentLen;
          continue;
        }
      }

      Status = SmmStoreLibEraseAnyBlock (Lba);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);

      NumBytes = BlockSize;
      Status   = SmmStoreLibWriteAnyBlock (Lba, 0, &NumBytes, (VOID *)(Image + BlockBase));
      if (EFI_ERROR (Status) || (NumBytes != BlockSize)) {
        return EFI_DEVICE_ERROR;
      }

      IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);
      Offset += SegmentLen;
      continue;
    }

    //
    // Partial-block update: must preserve the rest of the flash block.
    //
    NumBytes = BlockSize;
    Status   = SmmStoreLibReadAnyBlock (Lba, 0, &NumBytes, FlashBlock);
    if (EFI_ERROR (Status) || (NumBytes != BlockSize)) {
      return EFI_DEVICE_ERROR;
    }

    if (CompareMem (FlashBlock + StartInBlock, Image + BlockBase + StartInBlock, SegmentLen) == 0) {
      IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);
      IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);
      Offset += SegmentLen;
      continue;
    }

    CopyMem (FlashBlock + StartInBlock, Image + BlockBase + StartInBlock, SegmentLen);

    Status = SmmStoreLibEraseAnyBlock (Lba);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);

    NumBytes = BlockSize;
    Status   = SmmStoreLibWriteAnyBlock (Lba, 0, &NumBytes, FlashBlock);
    if (EFI_ERROR (Status) || (NumBytes != BlockSize)) {
      return EFI_DEVICE_ERROR;
    }

    IncrementProgress (Progress, TotalSteps, Step, ShouldReportProgress);
    Offset += SegmentLen;
  }

  return EFI_SUCCESS;
}

/**
  This code finds variable in storage blocks (Volatile or Non-Volatile).

  @param[in]      VariableName               Name of Variable to be found.
  @param[in]      VendorGuid                 Variable vendor GUID.
  @param[out]     Attributes                 Attribute value of the variable found.
  @param[in, out] DataSize                   Size of Data found. If size is less than the
                                             data, this value contains the required size.
  @param[out]     Data                       Data pointer.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.
**/
STATIC
EFI_STATUS
EFIAPI
GetVariableHook (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data
  )
{
  DEBUG ((DEBUG_INFO, "%a(): %g:%S\n", __func__, VendorGuid, VariableName));
  return EFI_NOT_AVAILABLE_YET;
}

/**
  This code Finds the Next available variable.

  @param[in, out] VariableNameSize           Size of the variable name.
  @param[in, out] VariableName               Pointer to variable name.
  @param[in, out] VendorGuid                 Variable Vendor Guid.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.
**/
STATIC
EFI_STATUS
EFIAPI
GetNextVariableNameHook (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  DEBUG ((DEBUG_INFO, "%a(): %g:%S\n", __func__, VendorGuid, VariableName));
  return EFI_NOT_AVAILABLE_YET;
}

/**
  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param[in] VariableName                     Name of Variable to be found.
  @param[in] VendorGuid                       Variable vendor GUID.
  @param[in] Attributes                       Attribute value of the variable found
  @param[in] DataSize                         Size of Data found. If size is less than the
                                              data, this value contains the required size.
  @param[in] Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.
**/
STATIC
EFI_STATUS
EFIAPI
SetVariableHook (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  DEBUG ((
    DEBUG_INFO,
    "%a(): %g:%S, 0x%x bytes, 0x%x\n",
    __func__,
    VendorGuid,
    VariableName,
    DataSize,
    Attributes
    ));
  return EFI_NOT_AVAILABLE_YET;
}

/**
  This code returns information about the EFI variables.

  @param[in]  Attributes                     Attributes bitmask to specify the type of variables
                                             on which to return information.
  @param[out] MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                             for the EFI variables associated with the attributes specified.
  @param[out] RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                             for EFI variables associated with the attributes specified.
  @param[out] MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                             associated with the attributes specified.

  @return EFI_SUCCESS                   Query successfully.
**/
STATIC
EFI_STATUS
EFIAPI
QueryVariableInfoHook (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  DEBUG ((DEBUG_INFO, "%a(): 0x%x\n", __func__, Attributes));
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Updates a firmware device with a new firmware image.  This function returns
  EFI_UNSUPPORTED if the firmware image is not updatable.  If the firmware image
  is updatable, the function should perform the following minimal validations
  before proceeding to do the firmware image update.
    - Validate that the image is a supported image for this firmware device.
      Return EFI_ABORTED if the image is not supported.  Additional details
      on why the image is not a supported image may be returned in AbortReason.
    - Validate the data from VendorCode if is not NULL.  Firmware image
      validation must be performed before VendorCode data validation.
      VendorCode data is ignored or considered invalid if image validation
      fails.  Return EFI_ABORTED if the VendorCode data is invalid.

  VendorCode enables vendor to implement vendor-specific firmware image update
  policy.  Null if the caller did not specify the policy or use the default
  policy.  As an example, vendor can implement a policy to allow an option to
  force a firmware image update when the abort reason is due to the new firmware
  image version is older than the current firmware image version or bad image
  checksum.  Sensitive operations such as those wiping the entire firmware image
  and render the device to be non-functional should be encoded in the image
  itself rather than passed with the VendorCode.  AbortReason enables vendor to
  have the option to provide a more detailed description of the abort reason to
  the caller.

  @param[in]  Image             Points to the new firmware image.
  @param[in]  ImageSize         Size, in bytes, of the new firmware image.
  @param[in]  VendorCode        This enables vendor to implement vendor-specific
                                firmware image update policy.  NULL indicates
                                the caller did not specify the policy or use the
                                default policy.
  @param[in]  Progress          A function used to report the progress of
                                updating the firmware device with the new
                                firmware image.
  @param[in]  CapsuleFwVersion  The version of the new firmware image from the
                                update capsule that provided the new firmware
                                image.
  @param[out] AbortReason       A pointer to a pointer to a Null-terminated
                                Unicode string providing more details on an
                                aborted operation. The buffer is allocated by
                                this function with
                                EFI_BOOT_SERVICES.AllocatePool().  It is the
                                caller's responsibility to free this buffer with
                                EFI_BOOT_SERVICES.FreePool().
  @param[out] LastAttemptStatus A pointer to a UINT32 that holds the last attempt
                                status to report back to the ESRT table in case
                                of error. This value will only be checked when this
                                function returns an error.

                                The return status code must fall in the range of
                                LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MIN_ERROR_CODE_VALUE to
                                LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MAX_ERROR_CODE_VALUE.

                                If the value falls outside this range, it will be converted
                                to LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL.

  @retval EFI_SUCCESS            The firmware device was successfully updated
                                 with the new firmware image.
  @retval EFI_ABORTED            The operation is aborted.  Additional details
                                 are provided in AbortReason.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
**/
EFI_STATUS
EFIAPI
FmpDeviceSetImageWithStatus (
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode        OPTIONAL,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress           OPTIONAL,
  IN  UINT32                                         CapsuleFwVersion,
  OUT CHAR16                                         **AbortReason,
  OUT UINT32                                         *LastAttemptStatus
  )
{
  EFI_STATUS   Status;
  UINTN        BlockSize;
  UINTN        BlockCount;
  UINTN        Block;
  UINTN        EntryIndex;
  UINTN        NumBytes;
  UINTN        TotalSteps;
  UINTN        Step;
  BOOLEAN      ShouldReportProgress;
  VOID         *ReadBuffer;
  CONST UINT8  *WriteNext;
  UINTN        ManifestEntryCount;
  CONST REGION_MANIFEST_ENTRY  *ManifestEntries;
  UINTN        BaseImageSize;
  BOOLEAN      UseManifest;
  BOOLEAN      UseBiosRegion;
  UINTN        BiosOffset;
  UINTN        BiosSize;
  CONST FMAP_HEADER  *FmapHeader;
  CONST FMAP_AREA    *FmapAreas;
  UINTN              FmapOffset;
  UINTN              FmapLength;

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
  BlockCount         = 0;
  Block              = 0;
  ReadBuffer         = NULL;

  //
  // FmpDeviceCheckImageWithStatus() has already validated the image, so not
  // repeating the checks.  However, could move the checks here to be able to
  // report abort reason which can't be done in FmpDeviceCheckImageWithStatus().
  //

  Status = SmmStoreLibGetBlockSize (&BlockSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): SmmStoreLibGetBlockSize() failed with: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Discover optional manifest and FMAP. If anything looks off, fall back to
  // legacy full-flash behavior.
  //
  BaseImageSize     = ImageSize;
  ManifestEntryCount = 0;
  ManifestEntries    = NULL;
  UseManifest        = FALSE;
  UseBiosRegion      = FALSE;
  BiosOffset         = 0;
  BiosSize           = 0;
  FmapHeader         = NULL;
  FmapAreas          = NULL;
  FmapOffset         = 0;
  FmapLength         = 0;

  Status = LocateRegionManifest (Image, ImageSize, &ManifestEntryCount, &ManifestEntries, &BaseImageSize);
  if (EFI_ERROR (Status)) {
    BaseImageSize      = ImageSize;
    ManifestEntryCount = 0;
    ManifestEntries    = NULL;
  }

  Status = LocateFmapInImage (Image, BaseImageSize, &FmapOffset, &FmapLength);
  if (!EFI_ERROR (Status) && ((FmapOffset + FmapLength) <= BaseImageSize)) {
    FmapHeader = (CONST FMAP_HEADER *)((CONST UINT8 *)Image + FmapOffset);
    FmapAreas  = (CONST FMAP_AREA *)((CONST UINT8 *)FmapHeader + sizeof (FMAP_HEADER));
  }

  if ((ManifestEntryCount > 0) && (FmapHeader != NULL) && (FmapHeader->AreaCount > 0)) {
    UseManifest = TRUE;
  } else if ((ManifestEntryCount == 0) && (FmapHeader != NULL)) {
    //
    // No manifest; attempt BIOS-only update using FMAP region. On Intel
    // platforms this typically corresponds to the IFD BIOS region exposed
    // to firmware as "SI_BIOS".
    //
    CONST CHAR8  SiBiosName[16] = "SI_BIOS";

    if (!EFI_ERROR (FindFmapRegion (FmapHeader, FmapAreas, FmapHeader->AreaCount, SiBiosName, &BiosOffset, &BiosSize))) {
      if ((BiosSize != 0) && ((BiosOffset + BiosSize) <= BaseImageSize)) {
        UseBiosRegion = TRUE;
      }
    }
  }

  ReadBuffer = AllocatePool (BlockSize);
  if (ReadBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(): failed to allocate read buffer\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  if (UseManifest) {
    //
    // Validate manifest regions against the FMAP before flashing.
    //
    TotalSteps = 0;
    for (EntryIndex = 0; EntryIndex < ManifestEntryCount; ++EntryIndex) {
      UINTN  RegionOffset;
      UINTN  RegionSize;

      Status = FindFmapRegion (
                 FmapHeader,
                 FmapAreas,
                 FmapHeader->AreaCount,
                 ManifestEntries[EntryIndex].RegionName,
                 &RegionOffset,
                 &RegionSize
                 );
      if (EFI_ERROR (Status) || (RegionSize == 0) || ((RegionOffset + RegionSize) > BaseImageSize)) {
        UseManifest = FALSE;
        break;
      }

      TotalSteps += ((RegionOffset + RegionSize - 1) / BlockSize - (RegionOffset / BlockSize) + 1) * 2;
    }
  }

  ShouldReportProgress = TRUE;
  Step                 = 0;

  if (UseManifest && (TotalSteps == 0)) {
    UseManifest = FALSE;
  }

  if (UseManifest) {
    DEBUG ((DEBUG_INFO, "%a(): manifest-guided update over %u region(s)\n", __func__, (UINT32)ManifestEntryCount));

    for (EntryIndex = 0; EntryIndex < ManifestEntryCount; ++EntryIndex) {
      UINTN  RegionOffset;
      UINTN  RegionSize;

      Status = FindFmapRegion (
                 FmapHeader,
                 FmapAreas,
                 FmapHeader->AreaCount,
                 ManifestEntries[EntryIndex].RegionName,
                 &RegionOffset,
                 &RegionSize
                 );
      if (EFI_ERROR (Status) || ((RegionOffset + RegionSize) > BaseImageSize)) {
        goto IoError;
      }

      Status = UpdateFlashRangeFromImage (
                 BlockSize,
                 (CONST UINT8 *)Image,
                 BaseImageSize,
                 RegionOffset,
                 RegionSize,
                 ReadBuffer,
                 Progress,
                 TotalSteps,
                 &Step,
                 &ShouldReportProgress
                 );
      if (EFI_ERROR (Status)) {
        goto IoError;
      }
    }
  }

  if (UseBiosRegion && !UseManifest) {
    //
    // BIOS-only fallback using FMAP when no manifest is present.
    //
    DEBUG ((DEBUG_INFO, "%a(): FMAP-guided BIOS-only update\n", __func__));

    TotalSteps = ((BiosOffset + BiosSize - 1) / BlockSize - (BiosOffset / BlockSize) + 1) * 2;
    if (TotalSteps == 0) {
      goto IoError;
    }

    Status = UpdateFlashRangeFromImage (
               BlockSize,
               (CONST UINT8 *)Image,
               BaseImageSize,
               BiosOffset,
               BiosSize,
               ReadBuffer,
               Progress,
               TotalSteps,
               &Step,
               &ShouldReportProgress
               );
    if (EFI_ERROR (Status)) {
      goto IoError;
    }
  }

  if (!UseManifest && !UseBiosRegion) {
    //
    // Legacy full-flash update path.
    //
    BlockCount = BaseImageSize / BlockSize;
    DEBUG ((
      DEBUG_INFO,
      "%a(): 0x%x blocks of 0x%x bytes\n",
      __func__,
      BlockCount,
      BlockSize
      ));

    TotalSteps = BlockCount * 2; // Erase and write of each block.
    WriteNext  = Image;

    for (Block = 0; Block < BlockCount; Block++, WriteNext += BlockSize) {
      //
      // Save the flash and time by only writing a block if new contents differs
      // from the old one.
      //
      // This is an optimization, so ignore read errors (if they are indicative of
      // a serious problem, erasing or writing will fail as well).
      //
      NumBytes = BlockSize;
      Status   = SmmStoreLibReadAnyBlock (Block, 0, &NumBytes, ReadBuffer);
      if (!EFI_ERROR (Status) && (NumBytes == BlockSize)) {
        if (CompareMem (ReadBuffer, WriteNext, BlockSize) == 0) {
          // Erase step.
          IncrementProgress (Progress, TotalSteps, &Step, &ShouldReportProgress);
          // Write step.
          IncrementProgress (Progress, TotalSteps, &Step, &ShouldReportProgress);
          continue;
        }
      }

      Status = SmmStoreLibEraseAnyBlock (Block);
      if (EFI_ERROR (Status)) {
        goto IoError;
      }

      IncrementProgress (Progress, TotalSteps, &Step, &ShouldReportProgress);

      NumBytes = BlockSize;
      Status   = SmmStoreLibWriteAnyBlock (Block, 0, &NumBytes, (VOID *)WriteNext);
      if (EFI_ERROR (Status) || (NumBytes != BlockSize)) {
        goto IoError;
      }

      IncrementProgress (Progress, TotalSteps, &Step, &ShouldReportProgress);
    }
  }

  FreePool (ReadBuffer);

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;

  //
  // Updating the firmware on system flash overwrites SMMSTORE region which
  // backs up EFI variables of the running firmware.  At this point both SMI
  // handler from coreboot and variable services of EDK can be mistaken in
  // their assumptions about the location, size and contents of the region.
  // Accessing flash where SMMSTORE used to be can lead to unexpected results
  // including corruption of the new image outside of its SMMSTORE.  Switch to
  // the use of stubs for dealing with EFI variables that do nothing.
  //
  // New firmware will not report result of flashing in any way unless some
  // kind of communication mechanism is implemented for this purpose.
  //
  // If there was an error, it's unclear whether these stubs would be of any
  // help, so they are employed only on successful flashing.
  //

  gRT->GetVariable         = GetVariableHook;
  gRT->GetNextVariableName = GetNextVariableNameHook;
  gRT->SetVariable         = SetVariableHook;
  gRT->QueryVariableInfo   = QueryVariableInfoHook;

  gRT->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (
         (UINT8 *)&gRT->Hdr,
         gRT->Hdr.HeaderSize,
         &gRT->Hdr.CRC32
         );

  return EFI_SUCCESS;

IoError:
  //
  // There are several reasons why we might end up in here:
  //  - an actual issue with the flash chip which is unlikely to allow any
  //    recovery via software
  //  - a bug in SMMSTORE SMI handler of coreboot
  //  - any bug of the firmware or its configuration which has resulted in the
  //    system not being ready for a full flash update
  //
  // The last case can be caused at least by:
  //  - coreboot not lifting flash protections
  //  - Intel ME not being disabled
  //
  // This situation is atypical and can leave the flash chip in an
  // unpredictable state which can be fully functional, unbootable or something
  // in between.
  //
  // Being optimistic that the firmware is still functional, we're leaving
  // variable services available under assumption that SMMSTORE region hasn't
  // been moved withing the firmware image (most updates don't modify layout).
  // Note that the store can even be recreated on the next boot depending on
  // the damage that it has sustained.  It's also possible that we hit
  // protected range and nothing of any worth has been modified.
  //
  // If the firmware ends up unbootable, then, in general, external flashing
  // via a programmer needs to be employed to recover the device.
  //
  FreePool (ReadBuffer);
  DEBUG ((
    DEBUG_ERROR,
    "%a(): flashing has failed at block 0x%x/0x%x: %r\n",
    __func__,
    Block,
    BlockCount,
    EFI_DEVICE_ERROR
    ));
  return EFI_DEVICE_ERROR;
}

/**
  Lock the firmware device that contains a firmware image.  Once a firmware
  device is locked, any attempts to modify the firmware image contents in the
  firmware device must fail.

  @note It is recommended that all firmware devices support a lock method to
        prevent modifications to a stored firmware image.

  @note A firmware device lock mechanism is typically only cleared by a full
        system reset (not just sleep state/low power mode).

  @retval  EFI_SUCCESS      The firmware device was locked.
  @retval  EFI_UNSUPPORTED  The firmware device does not support locking
**/
EFI_STATUS
EFIAPI
FmpDeviceLock (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Constructor that performs required initialization.

  @param ImageHandle  The image handle of the process.
  @param SystemTable  The EFI System Table pointer.

  @retval EFI_SUCCESS  Initialization was successful.
**/
EFI_STATUS
EFIAPI
FmpDeviceSmmLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SmmStoreLibInitialize ();
  return EFI_SUCCESS;
}
