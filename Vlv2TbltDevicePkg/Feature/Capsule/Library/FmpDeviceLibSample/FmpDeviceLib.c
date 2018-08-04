/**

Copyright (c) 2016, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/


#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FmpDeviceLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Used to pass the FMP install function to this lib.
  This allows the library to have control of the handle
  that the FMP instance is installed on.  This allows the library
  to use DriverBinding protocol model to locate its device(s) in the
  system.

  @param[in] 				Function pointer to FMP install function.

  @retval EFI_SUCCESS     	Library has saved function pointer and will call function pointer on each DriverBinding Start.
  @retval EFI_UNSUPPORTED 	Library doesn't use driver binding and only supports a single instance.
  @retval other error     	Error occurred.  Don't install FMP

**/
EFI_STATUS
EFIAPI
RegisterFmpInstaller(
IN FMP_DEVICE_LIB_REGISTER_FMP_INSTALLER Func
)
{
	// Because this is a sample lib with very simple fake device we don't use
	// the driverbinding protocol to locate our device.
	//
	return EFI_UNSUPPORTED;
}


/**
Used to get the size of the image in bytes.
NOTE - Do not return zero as that will identify the device as
not updatable.

@retval UINTN that represents the size of the firmware.

**/
EFI_STATUS
EFIAPI
FmpDeviceGetSize (
  IN UINTN  *Size
  )
{
  if (Size == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Size = 0x1000;
  return EFI_SUCCESS;
}

/**
Used to return a library supplied guid that will be the ImageTypeId guid of the FMP descriptor.
This is optional but can be used if at runtime the guid needs to be determined.

@param  Guid:  Double Guid Ptr that will be updated to point to guid.  This should be from static memory
and will not be freed.
@return EFI_UNSUPPORTED: if you library instance doesn't need dynamic guid return this.
@return Error: Any error will cause the wrapper to use the GUID defined by PCD
@return EFI_SUCCESS:  Guid ptr should be updated to point to static memeory which contains a valid guid
**/
EFI_STATUS
EFIAPI
FmpDeviceGetImageTypeIdGuidPtr(
  OUT EFI_GUID** Guid)
{
  //this instance doesn't need dynamic guid detection.
  return EFI_UNSUPPORTED;
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
  IN OUT UINT64  *Supported,
  IN OUT UINT64  *Setting
  )
{
  if (Supported == NULL || Setting == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Supported = (IMAGE_ATTRIBUTE_IMAGE_UPDATABLE | IMAGE_ATTRIBUTE_IN_USE);
  *Setting   = (IMAGE_ATTRIBUTE_IMAGE_UPDATABLE | IMAGE_ATTRIBUTE_IN_USE);
  return EFI_SUCCESS;
}

/**
Gets the current Lowest Supported Version.
This is a protection mechanism so that a previous version with known issue is not
applied.

ONLY implement this if your running firmware has a method to return this at runtime.

@param[out] Version           On return this value represents the
current Lowest Supported Version (in same format as GetVersion).

@retval EFI_SUCCESS           The Lowest Supported Version was correctly retrieved
@retval EFI_UNSUPPORTED       Device firmware doesn't support reporting LSV
@retval EFI_DEVICE_ERROR      Error occurred when trying to get the LSV
**/
EFI_STATUS
EFIAPI
FmpDeviceGetLowestSupportedVersion (
  IN OUT UINT32* LowestSupportedVersion
  )
{
  return EFI_UNSUPPORTED;
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
  *VersionString = NULL;
  return EFI_UNSUPPORTED;
}

/**
Gets the current running version.
ONLY implement this if your running firmware has a method to return this at runtime.

@param[out] Version           On return this value represents the current running version

@retval EFI_SUCCESS           The version was correctly retrieved
@retval EFI_UNSUPPORTED       Device firmware doesn't support reporting current version
@retval EFI_DEVICE_ERROR      Error occurred when trying to get the version
**/
EFI_STATUS
EFIAPI
FmpDeviceGetVersion(
IN OUT UINT32* Version
)
{
  return EFI_UNSUPPORTED;
}


/**
Retrieves a copy of the current firmware image of the device.

This function allows a copy of the current firmware image to be created and saved.
The saved copy could later been used, for example, in firmware image recovery or rollback.

@param[out] Image              Points to the buffer where the current image is copied to.
@param[out] ImageSize          On entry, points to the size of the buffer pointed to by Image, in bytes.
On return, points to the length of the image, in bytes.

@retval EFI_SUCCESS            The device was successfully updated with the new image.
@retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to hold the
image. The current buffer size needed to hold the image is returned
in ImageSize.
@retval EFI_INVALID_PARAMETER  The Image was NULL.
@retval EFI_NOT_FOUND          The current image is not copied to the buffer.
@retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpDeviceGetImage(
IN  OUT  VOID                         *Image,
IN  OUT  UINTN                        *ImageSize
)
/*++

Routine Description:

    This is a function used to read the current firmware from the device into memory.
    This is an optional function and can return EFI_UNSUPPORTED.  This is useful for
    test and diagnostics.

Arguments:
    Image               -- Buffer to place the image into.
    ImageSize           -- Size of the Image buffer.

Return Value:

    EFI_STATUS code.
    If not possible or not practical return EFI_UNSUPPORTED.

--*/
{
  return EFI_UNSUPPORTED;
}//GetImage()


/**
Updates the firmware image of the device.

This function updates the hardware with the new firmware image.
This function returns EFI_UNSUPPORTED if the firmware image is not updatable.
If the firmware image is updatable, the function should perform the following minimal validations
before proceeding to do the firmware image update.
- Validate the image is a supported image for this device.  The function returns EFI_ABORTED if
the image is unsupported.  The function can optionally provide more detailed information on
why the image is not a supported image.
- Validate the data from VendorCode if not null.  Image validation must be performed before
VendorCode data validation.  VendorCode data is ignored or considered invalid if image
validation failed.  The function returns EFI_ABORTED if the data is invalid.

VendorCode enables vendor to implement vendor-specific firmware image update policy.  Null if
the caller did not specify the policy or use the default policy.  As an example, vendor can implement
a policy to allow an option to force a firmware image update when the abort reason is due to the new
firmware image version is older than the current firmware image version or bad image checksum.
Sensitive operations such as those wiping the entire firmware image and render the device to be
non-functional should be encoded in the image itself rather than passed with the VendorCode.
AbortReason enables vendor to have the option to provide a more detailed description of the abort
reason to the caller.

@param[in]  Image              Points to the new image.
@param[in]  ImageSize          Size of the new image in bytes.
@param[in]  VendorCode         This enables vendor to implement vendor-specific firmware image update policy.
Null indicates the caller did not specify the policy or use the default policy.
@param[in]  Progress           A function used by the driver to report the progress of the firmware update.
@param[in]  CapsuleFwVersion   FMP Payload Header version of the image
@param[out] AbortReason        A pointer to a pointer to a null-terminated string providing more
details for the aborted operation. The buffer is allocated by this function
with AllocatePool(), and it is the caller's responsibility to free it with a
call to FreePool().

@retval EFI_SUCCESS            The device was successfully updated with the new image.
@retval EFI_ABORTED            The operation is aborted.
@retval EFI_INVALID_PARAMETER  The Image was NULL.
@retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpDeviceSetImage (
IN  CONST VOID                                       *Image,
IN  UINTN                                            ImageSize,
IN  CONST VOID                                       *VendorCode,
IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS    Progress,
IN  UINT32                                           CapsuleFwVersion,
OUT CHAR16                                           **AbortReason
)
{
    EFI_STATUS Status                         = EFI_SUCCESS;
    UINT32 Updateable                         = 0;

    Status = FmpDeviceCheckImage(Image, ImageSize, &Updateable);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "SetImage - Check Image failed with %r.\n", Status));
        goto cleanup;
    }

    if (Updateable != IMAGE_UPDATABLE_VALID)
    {
        DEBUG((DEBUG_ERROR, "SetImage - Check Image returned that the Image was not valid for update.  Updatable value = 0x%X.\n", Updateable));
        Status = EFI_ABORTED;
        goto cleanup;
    }

    if (Progress == NULL)
    {
        DEBUG((DEBUG_ERROR, "SetImage - Invalid progress callback\n"));
        Status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }

    Status = Progress(15);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "SetImage - Progress Callback failed with Status %r.\n", Status));
    }

    {
      UINTN  p;

      for (p = 20; p < 100; p++) {
        gBS->Stall (100000);  //us  = 0.1 seconds
        Progress (p);
      }
    }

    //TODO: add support for VendorCode, and AbortReason
cleanup:
    return Status;
}// SetImage()



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
FmpDeviceCheckImage(
IN  CONST VOID                        *Image,
IN  UINTN                             ImageSize,
OUT UINT32                            *ImageUpdateable
)
{
    EFI_STATUS status = EFI_SUCCESS;

    if (ImageUpdateable == NULL)
    {
        DEBUG((DEBUG_ERROR, "CheckImage - ImageUpdateable Pointer Parameter is NULL.\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    //Set to valid and then if any tests fail it will update this flag.
    //
    *ImageUpdateable = IMAGE_UPDATABLE_VALID;

    if (Image == NULL)
    {
        DEBUG((DEBUG_ERROR, "CheckImage - Image Pointer Parameter is NULL.\n"));
        *ImageUpdateable = IMAGE_UPDATABLE_INVALID; //not sure if this is needed
        return EFI_INVALID_PARAMETER;
    }

cleanup:
    return status;
}// CheckImage()

/**
Device frimware should trigger lock mechanism so that device fw can not be updated or tampered with.
This lock mechanism is generally only cleared by a full system reset (not just sleep state/low power mode)

@retval EFI_SUCCESS           The device was successfully locked.
@retval EFI_UNSUPPORTED       The hardware device/firmware doesn't support locking

**/
EFI_STATUS
EFIAPI
FmpDeviceLock(
)
{
  return EFI_SUCCESS;
}
