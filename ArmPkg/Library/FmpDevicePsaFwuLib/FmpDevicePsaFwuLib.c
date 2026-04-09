/**

  Copyright (c) 2024, Arm Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Platform Security Firmware Update for the A-profile Specification 1.0
    (https://developer.arm.com/documentation/den0118/latest)

  @par Glossary:
    - FW    - Firmware
    - FWU   - Firmware Update
    - PSA   - Platform Security update for the A-profile specification
    - FMP   - Firmware Management Protocol Device.

**/
#include <LastAttemptStatus.h>

#include <IndustryStandard/PsaMmFwUpdate.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/FmpDeviceLib.h>

#include <Guid/SystemResourceTable.h>

#include "PsaFwuLib.h"

#define IS_FWU_FUNC_SUPPORTED(Id)  ((BOOLEAN)((mSupportFunction & (1ULL << Id)) != 0))

STATIC UINT64                      mSupportFunction = 0;
STATIC UINT32                      mFwuFlags        = 0;
STATIC UINT32                      mVendorFlags     = 0;
STATIC UINT64                      mMaxPayloadSize;
STATIC EFI_EVENT                   mEfiVirtualAddressChangeEvent;
STATIC EFI_EVENT                   mEfiReadyToBootEvent;
STATIC PSA_MM_FWU_IMAGE_DIRECTORY  *mImageDirectory = NULL;
STATIC PSA_MM_FWU_IMG_INFO_ENTRY   *mImageEntry     = NULL;
STATIC UINTN                       mImageDirectorySize;
STATIC BOOLEAN                     mOnRuntime      = FALSE;
STATIC BOOLEAN                     mUpdateDisabled = FALSE;

/**
 * Convert Image Directory's address on VirtualAddress Change Event.
 *
 * @param [in]   Event      Registered VirtualAddress Change Event.
 * @param [in]   Context    Additional Data.
 *
 */
STATIC
VOID
EFIAPI
FmpPsaVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mOnRuntime = TRUE;
  PsaFwuVirtualAddressChangeEvent (Event, Context);
  gRT->ConvertPointer (0x00, (VOID **)&mImageDirectory);
  gRT->ConvertPointer (0x00, (VOID **)&mImageEntry);
}

/**
 * Check if firmware update is supported.
 *
 * @retval EFI_SUCCESS            Firmware update is supported.
 * @retval EFI_UNSUPPORTED        Firmware update is not supported.
 */
STATIC
EFI_STATUS
EFIAPI
CheckFwuSupport (
  IN VOID
  )
{
  EFI_STATUS                Status;
  UINT16                    *FunctionPresence;
  PSA_MM_FWU_DISCOVER_RESP  *FwuInfo;
  UINT16                    Idx;

  ASSERT ((sizeof (mSupportFunction) * 8) >= PSA_MM_FWU_COMMAND_MAX_ID);

  Status = FwuDiscovery (&FwuInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwuInfo->NumFunc >= PSA_MM_FWU_COMMAND_MAX_ID) {
    return EFI_UNSUPPORTED;
  }

  if (FwuInfo->ServiceStatus != SERVICE_STATUS_OPERATIVE) {
    DEBUG ((DEBUG_ERROR, "Fwu service isn't operative!\n"));
    return EFI_UNSUPPORTED;
  }

  FunctionPresence = (UINT16 *)GET_FWU_DATA_BUFFER (FwuInfo);

  for (Idx = 0; Idx < FwuInfo->NumFunc; Idx++) {
    mSupportFunction |= 1ULL << FunctionPresence[Idx];
  }

  if ((!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_BEGIN_STAGING)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_END_STAGING)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_CANCEL_STAGING)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_OPEN)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_WRITE_STREAM)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_READ_STREAM)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_COMMIT)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_ACCEPT_IMAGE)) ||
      (!IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_SELECT_PREVIOUS)))
  {
    DEBUG ((DEBUG_ERROR, "Error: Required FWU function(s) not supported.\n"));
    DEBUG ((DEBUG_ERROR, "\tFWU_BEGIN_STATING: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_BEGIN_STAGING) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_END_STATING: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_END_STAGING) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_CANCEL_STATING: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_CANCEL_STAGING) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_OPEN: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_OPEN) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_WRITE_STREAM: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_WRITE_STREAM) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_READ_STREAM: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_READ_STREAM) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_COMMIT: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_COMMIT) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_ACCEPT_IMAGE: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_ACCEPT_IMAGE) ? "Supported" : "Unsupported"));
    DEBUG ((DEBUG_ERROR, "\tFWU_SELECT_PREVIOUS: %s\n", IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_SELECT_PREVIOUS) ? "Supported" : "Unsupported"));
    return EFI_UNSUPPORTED;
  }

  mFwuFlags       = FwuInfo->Flags;
  mVendorFlags    = FwuInfo->VendorSpecificFlags;
  mMaxPayloadSize = FwuInfo->MaxPayloadSize;

  DEBUG ((DEBUG_INFO, "FmpPsaFwuLib: ServiceStatus: %d\n", FwuInfo->ServiceStatus));
  DEBUG ((DEBUG_INFO, "FmpPsaFwuLib: VersionMajor: %d\n", FwuInfo->VersionMajor));
  DEBUG ((DEBUG_INFO, "FmpPsaFwuLib: VersionMinor: %d\n", FwuInfo->VersionMinor));
  DEBUG ((DEBUG_INFO, "FmpPsaFwuLib: mFwuFlags: %d\n", FwuInfo->Flags));
  DEBUG ((DEBUG_INFO, "FmpPsaFwuLib: mVendorFlags: %d\n", FwuInfo->VendorSpecificFlags));
  DEBUG ((DEBUG_INFO, "FmpPsaFwuLib: mMaxPayloadSize: %d\n", FwuInfo->MaxPayloadSize));

  return EFI_SUCCESS;
}

/**
 * Get firmware update image directory.
 *
 * @retval EFI_SUCCESS
 * @retval Others         Fail to get Image Directory Information.
 */
STATIC
EFI_STATUS
EFIAPI
GetFwuImageDirectory (
  IN VOID
  )
{
  EFI_STATUS                 Status;
  EFI_STATUS                 CloseStatus;
  UINT32                     Handle;
  UINT32                     ReadBytes;
  UINT32                     TotalBytes;
  UINT32                     Progress;
  UINT32                     TotalWork;
  VOID                       *Buffer;
  PSA_MM_FWU_IMG_INFO_ENTRY  *ImgEntry;
  UINTN                      Idx;

  Status = FwuOpen (&gPsaFwuImageDirectoryGuid, FwuOpStreamRead, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "FmpPsaFwuLib: FwuOpen failed for Image Directory: %d\n",
      Status
      ));
    return Status;
  }

  if (mImageDirectory == NULL) {
    // Get the size of image directory.
    Status = FwuReadStream (Handle, NULL, 0, &ReadBytes, &TotalBytes);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "FmpPsaFwuLib: Failed to get image directory size: %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    CloseStatus = FwuCommit (Handle, 1, 0, &Progress, &TotalWork);
    ASSERT (CloseStatus == EFI_SUCCESS);

    mImageDirectorySize = TotalBytes;

    /*
     * mImageDirectory should be allocated from the RuntimePool
     * when runtime capsule updates are supported.
     */
    mImageDirectory = AllocateRuntimePool (mImageDirectorySize);
    if (mImageDirectory == NULL) {
      DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: Failed to allocate memory for image directory!\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    // Reopen handle to read image directory.
    Status = FwuOpen (&gPsaFwuImageDirectoryGuid, FwuOpStreamRead, &Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "FmpPsaFwuLib: FwuOpen failed for Image Directory: %r\n",
        Status
        ));
      return Status;
    }
  }

  /**
   * Read Image directory first or refresh the Image directory contents.
   */
  TotalBytes = 0;
  ReadBytes  = MIN (mImageDirectorySize, mMaxPayloadSize);
  Buffer     = (VOID *)mImageDirectory;

  do {
    Status = FwuReadStream (
               Handle,
               Buffer + TotalBytes,
               ReadBytes,
               &ReadBytes,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: Failed to read image directory! %r\n", Status));
      goto ErrorHandler;
    }

    TotalBytes += ReadBytes;
    ReadBytes   = MIN ((mImageDirectorySize - TotalBytes), mMaxPayloadSize);
  } while (TotalBytes < mImageDirectorySize);

  /**
   * Find Image Entry related to this FmpDevicePkg Driver.
   */
  for (Idx = 0; Idx < mImageDirectory->NumImages; Idx++) {
    ImgEntry = GET_FWU_IMG_INFO_ENTRY (mImageDirectory, Idx);
    if (CompareGuid (&ImgEntry->ImgTypeGuid, &gEfiCallerIdGuid)) {
      mImageEntry = ImgEntry;
      break;
    }
  }

ErrorHandler:
  CloseStatus = FwuCommit (Handle, 1, 0, &Progress, &TotalWork);
  ASSERT (CloseStatus == EFI_SUCCESS);

  return Status;
}

/**
 * Accept image at ReadyToBoot phase.
 *
 * @param [in]   Event      Registered ready to boot event..
 * @param [in]   Context    Additional Data.
 *
 */
STATIC
VOID
EFIAPI
FmpPsaReadyToBootEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = FwuAcceptImage (&gEfiCallerIdGuid);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: Failed to accept the image: %g\n", &gEfiCallerIdGuid));
  }

  /**
   * Refresh Image Directory.
   */
  Status = GetFwuImageDirectory ();
  if (EFI_ERROR (Status) ||
      (mImageEntry->Accepted == FWU_IMAGE_UNACCEPTED))
  {
    DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: Image not renewed in Image Directory after calling accept - %g\n", &gEfiCallerIdGuid));
  }
}

/**
  Used to pass the FMP install function to this lib.  This allows the library to
  have control of the handle that the FMP instance is installed on.  This allows
  the library to use DriverBinding protocol model to locate its device(s) in the
  system.

  @param[in] Func  Function pointer to FMP install function.

  @retval EFI_SUCCESS       Library has saved function pointer and will call
                            function pointer on each DriverBinding Start.
  @retval EFI_UNSUPPORTED   Library doesn't use driver binding and only supports
                            a single instance.
  @retval other error       Error occurred.  Don't install FMP

**/
EFI_STATUS
EFIAPI
RegisterFmpInstaller (
  IN FMP_DEVICE_LIB_REGISTER_FMP_INSTALLER  Func
  )
{
  //
  // This is a system firmware update that does not use Driver Binding Protocol
  //
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
  IN FMP_DEVICE_LIB_REGISTER_FMP_UNINSTALLER  FmpUninstaller
  )
{
  //
  // This is a system firmware update that does not use Driver Binding Protocol
  //
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
  //
  // This is a system firmware update that does not use Driver Binding Protocol
  //
  return EFI_UNSUPPORTED;
}

/**
  Returns the size, in bytes, of the firmware image currently stored in the
  firmware device.  This function is used to by the GetImage() and
  GetImageInfo() services of the Firmware Management Protocol.  If the image
  size cannot be determined from the firmware device, then 0 must be returned.

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
  return EFI_UNSUPPORTED;
}

/**
  Used to return a library supplied guid that will be the ImageTypeId guid of
  the FMP descriptor.  This is optional but can be used if at runtime the guid
  needs to be determined.

  @param[out] Guid  Double Guid Ptr that will be updated to point to guid.
                    This should be from static memory and will not be freed.

  @return EFI_UNSUPPORTED  Library instance doesn't need dynamic guid.
  @return Error            Any error will cause the wrapper to use the GUID
                           defined by PCD.
  @return EFI_SUCCESS      Guid ptr should be updated to point to static memory
                           which contains a valid guid.

**/
EFI_STATUS
EFIAPI
FmpDeviceGetImageTypeIdGuidPtr (
  OUT EFI_GUID  **Guid
  )
{
  /**
   * Use FmpDxe gEfiCallerId.
   */
  *Guid = &gEfiCallerIdGuid;
  return EFI_SUCCESS;
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
  @retval EFI_INVALID_PARAMETER  One of the parameters - Settings or Supported - is NULL.

**/
EFI_STATUS
EFIAPI
FmpDeviceGetAttributes (
  IN OUT UINT64  *Supported,
  IN OUT UINT64  *Setting
  )
{
  if ((Supported == NULL) || (Setting == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  /**
   * IMAGE_ATTRIBUTE_IMAGE_UPDATABLE:
   *    - Support firmware image update.
   *
   * IMAGE_ATTRIBUTE_RESET_REQUIRED:
   *    - Reset required to take effect new firmware image.
   *
   * IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED:
   *    - Authentication required while GetImage(), SetImage() and CheckImage().
   *
   * IMAGE_ATTRIBUTE_IN_USE:
   *    - This image can be used (to distinguish from duplicated images).
   *    - If this flag is unset, ESRT entry isn't generated.
   *      See "UEFI Spec 23.4.4 Mapping Firmware Management Protocol Descriptors to ESRT Entries"
   *
   * IMAGE_ATTRIBUTE_UEFI_IMAGE:
   *    - Firmware binary saved with EFI compatible format image.
   *    - Arm uses FIP format. so it doesn't set this flag.
   */
  *Supported = (IMAGE_ATTRIBUTE_IMAGE_UPDATABLE         |
                IMAGE_ATTRIBUTE_RESET_REQUIRED          |
                IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED |
                IMAGE_ATTRIBUTE_IN_USE
                );
  *Setting = (IMAGE_ATTRIBUTE_IMAGE_UPDATABLE         |
              IMAGE_ATTRIBUTE_RESET_REQUIRED          |
              IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED |
              IMAGE_ATTRIBUTE_IN_USE
              );
  return EFI_SUCCESS;
}

/**
  Gets the current Lowest Supported Version.

  This is a protection mechanism so that a previous version with known issue is
  not applied.  ONLY implement this if your running firmware has a method to
  return this at runtime.  If EFI_UNSUPPORTED is returned, then the Lowest
  Supported Version is stored in a UEFI Variable.

  @param[out] Version  On return this value represents the current Lowest
                       Supported Version (in same format as GetVersion).

  @retval EFI_SUCCESS       The Lowest Supported Version was correctly retrieved
  @retval EFI_UNSUPPORTED   Device firmware doesn't support reporting LSV
  @retval EFI_DEVICE_ERROR  Error occurred when trying to get the LSV
**/
EFI_STATUS
EFIAPI
FmpDeviceGetLowestSupportedVersion (
  IN OUT UINT32  *LowestSupportedVersion
  )
{
  //
  // Retrieve the lowest support version from a PCD
  // NOTE: This method of using a PCD can only be used for the system firmware
  //       FMP instance that is updated every time the system firmware is
  //       updated.  If system firmware updates support partial updates that
  //       would not include the system firmware FMP instance, then a PCD can
  //       not be used and the value must come from the currently running system
  //       firmware image.
  //
  *LowestSupportedVersion = PcdGet32 (PcdSystemFirmwareFmpLowestSupportedVersion);
  return EFI_SUCCESS;
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
  if (VersionString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the version string from a PCD
  // NOTE: This method of using a PCD can only be used for the system firmware
  //       FMP instance that is updated every time the system firmware is
  //       updated.  If system firmware updates support partial updates that
  //       would not include the system firmware FMP instance, then a PCD can
  //       not be used and the value must come from the currently running system
  //       firmware image.
  //
  *VersionString = (CHAR16 *)AllocateCopyPool (
                               PcdGetSize (PcdSystemFirmwareFmpVersionString),
                               PcdGetPtr (PcdSystemFirmwareFmpVersionString)
                               );
  if (*VersionString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Gets the current running version.

  ONLY implement this if your running firmware has a method to return this at
  runtime.

  @param[out] Version  On return this value represents the current running
                       version.

  @retval EFI_SUCCESS       The version was correctly retrieved.
  @retval EFI_UNSUPPORTED   Device firmware doesn't support reporting current
                            version.
  @retval EFI_DEVICE_ERROR  Error occurred when trying to get the version.
**/
EFI_STATUS
EFIAPI
FmpDeviceGetVersion (
  IN OUT UINT32  *Version
  )
{
  if (Version == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the version string from a PCD
  // NOTE: This method of using a PCD can only be used for the system firmware
  //       FMP instance that is updated every time the system firmware is
  //       updated.  If system firmware updates support partial updates that
  //       would not include the system firmware FMP instance, then a PCD can
  //       not be used and the value must come from the currently running system
  //       firmware image.
  //
  *Version = PcdGet32 (PcdSystemFirmwareFmpVersion);

  return EFI_SUCCESS;
}

/**
  Returns the value used to fill in the HardwareInstance field of the
  EFI_FIRMWARE_IMAGE_DESCRIPTOR structure that is returned by the GetImageInfo()
  service of the Firmware Management Protocol.  If EFI_SUCCESS is returned, then
  the firmware device supports a method to report the HardwareInstance value.
  If the value cannot be reported for the firmware device, then EFI_UNSUPPORTED
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
  return EFI_UNSUPPORTED;
}

/**
  Retrieves a copy of the current firmware image of the device.

  This function allows a copy of the current firmware image to be created and
  saved.  The saved copy could later been used, for example, in firmware image
  recovery or rollback.

  @param[in,out] Image        Points to the buffer where the current image
                              is copied to.
  @param[in,out] ImageSize    On entry, points to the size of the buffer pointed
                              to by Image, in bytes.
                              On return, points to the length of the image,
                              in bytes.

  @retval EFI_SUCCESS            The image was successfully read from the device.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small
                                 to hold the image. The current buffer size
                                 needed to hold the image is returned in
                                 ImageSize.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_NOT_FOUND          The current image is not copied to the buffer.
  @retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpDeviceGetImage (
  IN OUT VOID   *Image,
  IN OUT UINTN  *ImageSize
  )
{
  /**
   * FIP image includes not only UEFI image but also other software components
   * and theirs configuration.
   * They shouldn't be exposed by calling GetTheImage().
   */
  return EFI_UNSUPPORTED;
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
  @retval EFI_INVALID_PARAMETER  LastAttemptStatus was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpDeviceSetImageWithStatus (
  IN  CONST VOID *Image,
  IN  UINTN ImageSize,
  IN  CONST VOID *VendorCode, OPTIONAL
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress, OPTIONAL
  IN  UINT32                                         CapsuleFwVersion,
  OUT CHAR16                                         **AbortReason,
  OUT UINT32                                         *LastAttemptStatus
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  CancelStatus;
  UINT32      Updatable;
  UINT32      WriteBytes;
  UINT32      TotalBytes;
  UINT32      Handle;
  UINT32      CommitProgress;
  UINT32      CommitTotalWorks;

  Updatable          = 0;
  *LastAttemptStatus = LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MIN_ERROR_CODE_VALUE;

  Status = FmpDeviceCheckImageWithStatus (Image, ImageSize, &Updatable, LastAttemptStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: FmpDeviceSetImageWithStatus - Check Image failed with %r.\n", Status));
    return Status;
  }

  if (Updatable != IMAGE_UPDATABLE_VALID) {
    DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: FmpDeviceSetImageWithStatus - Check Image returned that the Image was not valid for update.  Updatable value = 0x%X.\n", Updatable));
    return EFI_ABORTED;
  }

  if ((mFwuFlags & FWU_DISCOVER_FLAGS_PARTIAL_UPDATE_SUPPORT) ==
      FWU_DISCOVER_FLAGS_PARTIAL_UPDATE_SUPPORT)
  {
    Status = FwuBeginStaging (&gEfiCallerIdGuid, 1, 0);
  } else {
    Status = FwuBeginStaging (NULL, 0, 0);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "FmpPsaFwuLib: FmpDeviceSetImageWithStatus - Failed to enter Staging: %r.\n",
      Status
      ));
    return Status;
  }

  if (Progress != NULL) {
    Status = Progress (1);
    if (EFI_ERROR (Status)) {
      goto CancelUpdate;
    }
  }

  Status = FwuOpen (&gEfiCallerIdGuid, FwuOpStreamWrite, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "FmpPsaFwuLib: FmpDeviceSetImageWithStatus - Failed to open(%g), Status=%r.\n",
      &gEfiCallerIdGuid,
      Status
      ));
    goto CancelUpdate;
  }

  if (Progress != NULL) {
    Status = Progress (5);
    if (EFI_ERROR (Status)) {
      goto CancelUpdate;
    }
  }

  TotalBytes = 0;
  WriteBytes = MIN (ImageSize, mMaxPayloadSize);

  do {
    Status = FwuWriteStream (Handle, Image + TotalBytes, WriteBytes);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "FmpPsaFwuLib: FmpDeviceSetImageWithStatus - Failed to write: %r.\n",
        Status
        ));
      goto CancelUpdate;
    }

    TotalBytes += WriteBytes;
    WriteBytes  = MIN ((ImageSize - TotalBytes), mMaxPayloadSize);

    /**
     * Set the progress percentage between 5% to 55%
     */
    if (Progress != NULL) {
      Status = Progress ((TotalBytes / (2 * ImageSize)) + 5);
      if (EFI_ERROR (Status)) {
        goto CancelUpdate;
      }
    }
  } while (TotalBytes < ImageSize);

  if (Progress != NULL) {
    Status = Progress (60);
    if (EFI_ERROR (Status)) {
      goto CancelUpdate;
    }
  }

  /**
   * NOTE:
   *     Next boot, This image accepted in Entry point.
   */
  Status = FwuCommit (Handle, 1, 0, &CommitProgress, &CommitTotalWorks);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "FmpPsaFwuLib: FmpDeviceSetImageWithStatus - Failed to commit: %r.\n",
      Status
      ));
    goto CancelUpdate;
  }

  if (Progress != NULL) {
    Status = Progress (90);
    if (EFI_ERROR (Status)) {
      goto CancelUpdate;
    }
  }

  Status = FwuEndStaging ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "FmpPsaFwuLib: FmpDeviceSetImageWithStatus - Failed to enter Staging: %r.\n",
      Status
      ));
    goto CancelUpdate;
  }

  if (Progress != NULL) {
    Progress (100);
  }

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;

  /*
   * Reload CorrectBoot value from ImageDirectory.
   */
  Status = GetFwuImageDirectory ();

  ASSERT (Status == EFI_SUCCESS);
  ASSERT (!(mImageDirectory->CorrectBoot));

  /*
   * After FwuEndStaging(), the active index is updated.
   * In other words, the UpdateAgent's state no longer matches
   * the correct boot context (current index != active index).
   * Therefore, disable firmware updates so that prevent update
   * of images in the "current index".
   */
  mUpdateDisabled = TRUE;

  return EFI_SUCCESS;

CancelUpdate:
  if (IS_FWU_FUNC_SUPPORTED (PSA_MM_FWU_COMMAND_CANCEL_STAGING)) {
    CancelStatus = FwuCancelStaging ();
    ASSERT (CancelStatus == EFI_SUCCESS);
  }

  return Status;
}

/**
  Updates the firmware image of the device.

  This function updates the hardware with the new firmware image.  This function
  returns EFI_UNSUPPORTED if the firmware image is not updatable.  If the
  firmware image is updatable, the function should perform the following minimal
  validations before proceeding to do the firmware image update.
    - Validate the image is a supported image for this device.  The function
      returns EFI_ABORTED if the image is unsupported.  The function can
      optionally provide more detailed information on why the image is not a
      supported image.
    - Validate the data from VendorCode if not null.  Image validation must be
      performed before VendorCode data validation.  VendorCode data is ignored
      or considered invalid if image validation failed.  The function returns
      EFI_ABORTED if the data is invalid.

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

  @param[in]  Image             Points to the new image.
  @param[in]  ImageSize         Size of the new image in bytes.
  @param[in]  VendorCode        This enables vendor to implement vendor-specific
                                firmware image update policy. Null indicates the
                                caller did not specify the policy or use the
                                default policy.
  @param[in]  Progress          A function used by the driver to report the
                                progress of the firmware update.
  @param[in]  CapsuleFwVersion  FMP Payload Header version of the image.
  @param[out] AbortReason       A pointer to a pointer to a null-terminated
                                string providing more details for the aborted
                                operation. The buffer is allocated by this
                                function with AllocatePool(), and it is the
                                caller's responsibility to free it with a call
                                to FreePool().

  @retval EFI_SUCCESS            The device was successfully updated with the
                                 new image.
  @retval EFI_ABORTED            The operation is aborted.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpDeviceSetImage (
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
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
                                  of error. This value will only be checked when this
                                  function returns an error.

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
  @retval EFI_INVALID_PARAMETER  LastAttemptStatus is NULL.

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
  if ((Image == NULL) ||
      (ImageSize == 0) ||
      (ImageUpdatable == NULL) ||
      (LastAttemptStatus == NULL))
  {
    DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: CheckImageWithStatus - Invalid parameters.\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((ImageSize > mImageEntry->ImgMaxSize) || (mUpdateDisabled)) {
    *ImageUpdatable    = IMAGE_UPDATABLE_INVALID;
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_DEVICE_LIBRARY_MIN_ERROR_CODE_VALUE;
    DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: CheckImageWithStatus - Invalid Status.\n"));
    return EFI_INVALID_PARAMETER;
  }

  *ImageUpdatable    = IMAGE_UPDATABLE_VALID;
  *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;

  return EFI_SUCCESS;
}

/**
  Checks if the firmware image is valid for the device.

  This function allows firmware update application to validate the firmware image without
  invoking the SetImage() first.

  @param[in]  Image              Points to the new image.
  @param[in]  ImageSize          Size of the new image in bytes.
  @param[out] ImageUpdatable     Indicates if the new image is valid for update. It also provides,
                                 if available, additional information if the image is invalid.

  @retval EFI_SUCCESS            The image was successfully checked.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.

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

  return FmpDeviceCheckImageWithStatus (
           Image,
           ImageSize,
           ImageUpdatable,
           &LastAttemptStatus
           );
}

/**
  Device firmware should trigger lock mechanism so that device fw cannot be
  updated or tampered with. This lock mechanism is generally only cleared by a
  full system reset (not just sleep state/low power mode)

  @retval EFI_SUCCESS           The device was successfully locked.
  @retval EFI_UNSUPPORTED       The hardware device/firmware doesn't support locking

**/
EFI_STATUS
EFIAPI
FmpDeviceLock (
  VOID
  )
{
  /**
   * Because Arm doesn't update firmware via capsule coalescing,
   * If the deviced locked, FmpDevicePkg cannot update firmware properly
   * which assumes firmware is updated via capsule coalescing.
   * Also, firmware update device is already isolated by StandaloneMm.
   * Therefore, It's enough to return EFI_UNSUPPORTED.
   */
  return EFI_UNSUPPORTED;
}

/**
  Platform Flash Access Lib Constructor.
**/
EFI_STATUS
EFIAPI
FmpDevicePsaFwuLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FmpPsaVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mEfiVirtualAddressChangeEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PsaFwuLibInit ();
  if (EFI_ERROR (Status)) {
    goto ErrorHandler;
  }

  Status = CheckFwuSupport ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "FmpPsaFwuLib: There's not supported function. Disable FWU...\n"
      ));
    mUpdateDisabled = TRUE;
    Status          = EFI_SUCCESS;

    goto ErrorHandler;
  }

  Status = GetFwuImageDirectory ();
  if (EFI_ERROR (Status)) {
    goto ErrorHandler;
  }

  if (mImageEntry == NULL) {
    DEBUG ((
      DEBUG_WARN,
      "FmpPsaFwuLib: %g Image Type isn't present on Image Directory. Disable FWU on this image type.\n",
      &gEfiCallerIdGuid
      ));
    mUpdateDisabled = TRUE;
    Status          = EFI_SUCCESS;

    goto ErrorHandler;
  }

  if (!mImageDirectory->CorrectBoot) {
    DEBUG ((
      DEBUG_WARN,
      "FmpPsaFwuLib: Incorrect Boot!\n"
      ));

    mUpdateDisabled = TRUE;

    Status = FwuSelectPrevious ();
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "FmpPsaFwuLib: Failed to select previous... %r\n",
        Status
        ));
    }

    /*
     * The firmware boots using the previous index, not the active index.
     * fwu_select_previous() corrects this by setting the active index to the previous one.
     *
     * Even if fwu_select_previous() fails,
     * the system can still proceed because it has already booted from the previous image.
     * In this case, the firmware update feature is disabled.
     */
    Status = EFI_SUCCESS;

    goto ErrorHandler;
  }

  if (mImageEntry->Accepted == FWU_IMAGE_UNACCEPTED) {
    /*
     * If the firmware is able to boot to this stage and
     * load the FMP device driver, we can consider that the Boot is successful.
     * Therefore, accept the image in the ReadyToBoot phase.
     */
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    FmpPsaReadyToBootEvent,
                    NULL,
                    &gEfiEventReadyToBootGuid,
                    &mEfiReadyToBootEvent
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpPsaFwuLib: Failed to accept the image: %g\n", &gEfiCallerIdGuid));
      goto ErrorHandler;
    }
  }

  return EFI_SUCCESS;

ErrorHandler:
  if (mImageDirectory != NULL) {
    FreePool (mImageDirectory);
    mImageEntry     = NULL;
    mImageDirectory = NULL;
  }

  PsaFwuLibExit ();

  gBS->CloseEvent (mEfiVirtualAddressChangeEvent);

  return Status;
}
