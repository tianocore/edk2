/** @file
  EDKII System Capsule library.

  EDKII System Capsule library instance.

  CapsuleAuthenticateSystemFirmware(), ExtractAuthenticatedImage() will receive
  untrusted input and do basic validation.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/SystemResourceTable.h>
#include <Guid/FirmwareContentsSigned.h>
#include <Guid/WinCertificate.h>
#include <Guid/EdkiiSystemFmpCapsule.h>
#include <Guid/WinCertificate.h>
#include <Guid/ImageAuthentication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/EdkiiSystemCapsuleLib.h>
#include <Library/FmpAuthenticationLib.h>

#include <Protocol/FirmwareManagement.h>

EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR  *mImageFmpInfo;
UINTN                                   mImageFmpInfoSize;
EFI_GUID                                mEdkiiSystemFirmwareFileGuid;

/**
  Check if a block of buffer is erased.

  @param[in] ErasePolarity  Erase polarity attribute of the firmware volume
  @param[in] InBuffer       The buffer to be checked
  @param[in] BufferSize     Size of the buffer in bytes

  @retval    TRUE           The block of buffer is erased
  @retval    FALSE          The block of buffer is not erased
**/
BOOLEAN
IsBufferErased (
  IN UINT8  ErasePolarity,
  IN VOID   *InBuffer,
  IN UINTN  BufferSize
  )
{
  UINTN  Count;
  UINT8  EraseByte;
  UINT8  *Buffer;

  if (ErasePolarity == 1) {
    EraseByte = 0xFF;
  } else {
    EraseByte = 0;
  }

  Buffer = InBuffer;
  for (Count = 0; Count < BufferSize; Count++) {
    if (Buffer[Count] != EraseByte) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Get Section buffer pointer by SectionType and SectionInstance.

  @param[in]   SectionBuffer     The buffer of section
  @param[in]   SectionBufferSize The size of SectionBuffer in bytes
  @param[in]   SectionType       The SectionType of Section to be found
  @param[in]   SectionInstance   The Instance of Section to be found
  @param[out]  OutSectionBuffer  The section found, including SECTION_HEADER
  @param[out]  OutSectionSize    The size of section found, including SECTION_HEADER

  @retval TRUE  The FFS buffer is found.
  @retval FALSE The FFS buffer is not found.
**/
BOOLEAN
GetSectionByType (
  IN VOID              *SectionBuffer,
  IN UINT32            SectionBufferSize,
  IN EFI_SECTION_TYPE  SectionType,
  IN UINTN             SectionInstance,
  OUT VOID             **OutSectionBuffer,
  OUT UINTN            *OutSectionSize
  )
{
  EFI_COMMON_SECTION_HEADER  *SectionHeader;
  UINTN                      SectionSize;
  UINTN                      Instance;

  DEBUG ((DEBUG_INFO, "GetSectionByType - Buffer: 0x%08x - 0x%08x\n", SectionBuffer, SectionBufferSize));

  //
  // Find Section
  //
  SectionHeader = SectionBuffer;

  Instance = 0;
  while ((UINTN)SectionHeader < (UINTN)SectionBuffer + SectionBufferSize) {
    DEBUG ((DEBUG_INFO, "GetSectionByType - Section: 0x%08x\n", SectionHeader));
    if (IS_SECTION2 (SectionHeader)) {
      SectionSize = SECTION2_SIZE (SectionHeader);
    } else {
      SectionSize = SECTION_SIZE (SectionHeader);
    }

    if (SectionHeader->Type == SectionType) {
      if (Instance == SectionInstance) {
        *OutSectionBuffer = (UINT8 *)SectionHeader;
        *OutSectionSize   = SectionSize;
        DEBUG ((DEBUG_INFO, "GetSectionByType - 0x%x - 0x%x\n", *OutSectionBuffer, *OutSectionSize));
        return TRUE;
      } else {
        DEBUG ((DEBUG_INFO, "GetSectionByType - find section instance %x\n", Instance));
        Instance++;
      }
    } else {
      //
      // Skip other section type
      //
      DEBUG ((DEBUG_INFO, "GetSectionByType - other section type 0x%x\n", SectionHeader->Type));
    }

    //
    // Next Section
    //
    SectionHeader = (EFI_COMMON_SECTION_HEADER *)((UINTN)SectionHeader + ALIGN_VALUE (SectionSize, 4));
  }

  return FALSE;
}

/**
  Get FFS buffer pointer by FileName GUID and FileType.

  @param[in]   FdStart          The System Firmware FD image
  @param[in]   FdSize           The size of System Firmware FD image
  @param[in]   FileName         The FileName GUID of FFS to be found
  @param[in]   Type             The FileType of FFS to be found
  @param[out]  OutFfsBuffer     The FFS buffer found, including FFS_FILE_HEADER
  @param[out]  OutFfsBufferSize The size of FFS buffer found, including FFS_FILE_HEADER

  @retval TRUE  The FFS buffer is found.
  @retval FALSE The FFS buffer is not found.
**/
BOOLEAN
GetFfsByName (
  IN VOID             *FdStart,
  IN UINTN            FdSize,
  IN EFI_GUID         *FileName,
  IN EFI_FV_FILETYPE  Type,
  OUT VOID            **OutFfsBuffer,
  OUT UINTN           *OutFfsBufferSize
  )
{
  UINTN                           FvSize;
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FvExtHeader;
  EFI_FFS_FILE_HEADER             *FfsHeader;
  UINT32                          FfsSize;
  UINTN                           TestLength;
  BOOLEAN                         FvFound;

  DEBUG ((DEBUG_INFO, "GetFfsByName - FV: 0x%08x - 0x%08x\n", (UINTN)FdStart, (UINTN)FdSize));

  FvFound  = FALSE;
  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FdStart;
  while ((UINTN)FvHeader < (UINTN)FdStart + FdSize - 1) {
    FvSize = (UINTN)FdStart + FdSize - (UINTN)FvHeader;

    if (FvHeader->Signature != EFI_FVH_SIGNATURE) {
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)FvHeader + SIZE_4KB);
      continue;
    }

    DEBUG ((DEBUG_INFO, "checking FV....0x%08x - 0x%x\n", FvHeader, FvHeader->FvLength));
    FvFound = TRUE;
    if (FvHeader->FvLength > FvSize) {
      DEBUG ((DEBUG_ERROR, "GetFfsByName - FvSize: 0x%08x, MaxSize - 0x%08x\n", (UINTN)FvHeader->FvLength, (UINTN)FvSize));
      return FALSE;
    }

    FvSize = (UINTN)FvHeader->FvLength;

    //
    // Find FFS
    //
    if (FvHeader->ExtHeaderOffset != 0) {
      FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)((UINT8 *)FvHeader + FvHeader->ExtHeaderOffset);
      FfsHeader   = (EFI_FFS_FILE_HEADER *)((UINT8 *)FvExtHeader + FvExtHeader->ExtHeaderSize);
    } else {
      FfsHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FvHeader + FvHeader->HeaderLength);
    }

    FfsHeader = (EFI_FFS_FILE_HEADER *)((UINTN)FvHeader + ALIGN_VALUE ((UINTN)FfsHeader - (UINTN)FvHeader, 8));

    while ((UINTN)FfsHeader < (UINTN)FvHeader + FvSize - 1) {
      DEBUG ((DEBUG_INFO, "GetFfsByName - FFS: 0x%08x\n", FfsHeader));
      TestLength = (UINTN)((UINTN)FvHeader + FvSize - (UINTN)FfsHeader);
      if (TestLength > sizeof (EFI_FFS_FILE_HEADER)) {
        TestLength = sizeof (EFI_FFS_FILE_HEADER);
      }

      if (IsBufferErased (1, FfsHeader, TestLength)) {
        break;
      }

      if (IS_FFS_FILE2 (FfsHeader)) {
        FfsSize = FFS_FILE2_SIZE (FfsHeader);
      } else {
        FfsSize = FFS_FILE_SIZE (FfsHeader);
      }

      if (CompareGuid (FileName, &FfsHeader->Name) &&
          ((Type == EFI_FV_FILETYPE_ALL) || (FfsHeader->Type == Type)))
      {
        *OutFfsBuffer     = FfsHeader;
        *OutFfsBufferSize = FfsSize;
        return TRUE;
      } else {
        //
        // Any other type is not allowed
        //
        DEBUG ((DEBUG_INFO, "GetFfsByName - other FFS type 0x%x, name %g\n", FfsHeader->Type, &FfsHeader->Name));
      }

      //
      // Next File
      //
      FfsHeader = (EFI_FFS_FILE_HEADER *)((UINTN)FfsHeader + ALIGN_VALUE (FfsSize, 8));
    }

    //
    // Next FV
    //
    FvHeader = (VOID *)(UINTN)((UINTN)FvHeader + FvHeader->FvLength);
  }

  if (!FvFound) {
    DEBUG ((DEBUG_ERROR, "GetFfsByName - NO FV Found\n"));
  }

  return FALSE;
}

/**
  Extract the driver FV from an authenticated image.

  @param[in]  AuthenticatedImage      The authenticated capsule image.
  @param[in]  AuthenticatedImageSize  The size of the authenticated capsule image in bytes.
  @param[out] DriverFvImage           The driver FV image.
  @param[out] DriverFvImageSize       The size of the driver FV image in bytes.

  @retval TRUE  The driver Fv is extracted.
  @retval FALSE The driver Fv is not extracted.
**/
BOOLEAN
EFIAPI
ExtractDriverFvImage (
  IN VOID    *AuthenticatedImage,
  IN UINTN   AuthenticatedImageSize,
  OUT VOID   **DriverFvImage,
  OUT UINTN  *DriverFvImageSize
  )
{
  BOOLEAN  Result;
  UINT32   FileHeaderSize;

  *DriverFvImage     = NULL;
  *DriverFvImageSize = 0;

  Result = GetFfsByName (AuthenticatedImage, AuthenticatedImageSize, &gEdkiiSystemFmpCapsuleDriverFvFileGuid, EFI_FV_FILETYPE_RAW, DriverFvImage, DriverFvImageSize);
  if (!Result) {
    return FALSE;
  }

  if (IS_FFS_FILE2 (*DriverFvImage)) {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER);
  }

  *DriverFvImage     = (UINT8 *)*DriverFvImage + FileHeaderSize;
  *DriverFvImageSize = *DriverFvImageSize - FileHeaderSize;

  return Result;
}

/**
  Extract the config image from an authenticated image.

  @param[in]  AuthenticatedImage      The authenticated capsule image.
  @param[in]  AuthenticatedImageSize  The size of the authenticated capsule image in bytes.
  @param[out] ConfigImage             The config image.
  @param[out] ConfigImageSize         The size of the config image in bytes.

  @retval TRUE  The config image is extracted.
  @retval FALSE The config image is not extracted.
**/
BOOLEAN
EFIAPI
ExtractConfigImage (
  IN VOID    *AuthenticatedImage,
  IN UINTN   AuthenticatedImageSize,
  OUT VOID   **ConfigImage,
  OUT UINTN  *ConfigImageSize
  )
{
  BOOLEAN  Result;
  UINT32   FileHeaderSize;

  *ConfigImage     = NULL;
  *ConfigImageSize = 0;

  Result = GetFfsByName (AuthenticatedImage, AuthenticatedImageSize, &gEdkiiSystemFmpCapsuleConfigFileGuid, EFI_FV_FILETYPE_RAW, ConfigImage, ConfigImageSize);
  if (!Result) {
    return FALSE;
  }

  if (IS_FFS_FILE2 (*ConfigImage)) {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER);
  }

  *ConfigImage     = (UINT8 *)*ConfigImage + FileHeaderSize;
  *ConfigImageSize = *ConfigImageSize - FileHeaderSize;

  return Result;
}

/**
  Extract the authenticated image from an FMP capsule image.

  Caution: This function may receive untrusted input.

  @param[in]  Image                   The FMP capsule image, including EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               The size of FMP capsule image in bytes.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] AuthenticatedImage      The authenticated capsule image, excluding EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[out] AuthenticatedImageSize  The size of the authenticated capsule image in bytes.

  @retval TRUE  The authenticated image is extracted.
  @retval FALSE The authenticated image is not extracted.
**/
BOOLEAN
EFIAPI
ExtractAuthenticatedImage (
  IN VOID     *Image,
  IN UINTN    ImageSize,
  OUT UINT32  *LastAttemptStatus,
  OUT VOID    **AuthenticatedImage,
  OUT UINTN   *AuthenticatedImageSize
  )
{
  EFI_FIRMWARE_IMAGE_AUTHENTICATION  *ImageAuth;
  EFI_STATUS                         Status;
  GUID                               *CertType;
  VOID                               *PublicKeyData;
  UINTN                              PublicKeyDataLength;

  DEBUG ((DEBUG_INFO, "ExtractAuthenticatedImage - Image: 0x%08x - 0x%08x\n", (UINTN)Image, (UINTN)ImageSize));

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
  if ((Image == NULL) || (ImageSize == 0)) {
    return FALSE;
  }

  ImageAuth = (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)Image;
  if (ImageSize < sizeof (EFI_FIRMWARE_IMAGE_AUTHENTICATION)) {
    DEBUG ((DEBUG_ERROR, "ExtractAuthenticatedImage - ImageSize too small\n"));
    return FALSE;
  }

  if (ImageAuth->AuthInfo.Hdr.dwLength <= OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)) {
    DEBUG ((DEBUG_ERROR, "ExtractAuthenticatedImage - dwLength too small\n"));
    return FALSE;
  }

  if ((UINTN)ImageAuth->AuthInfo.Hdr.dwLength > MAX_UINTN - sizeof (UINT64)) {
    DEBUG ((DEBUG_ERROR, "ExtractAuthenticatedImage - dwLength too big\n"));
    return FALSE;
  }

  if (ImageSize <= sizeof (ImageAuth->MonotonicCount) + ImageAuth->AuthInfo.Hdr.dwLength) {
    DEBUG ((DEBUG_ERROR, "ExtractAuthenticatedImage - ImageSize too small\n"));
    return FALSE;
  }

  if (ImageAuth->AuthInfo.Hdr.wRevision != 0x0200) {
    DEBUG ((DEBUG_ERROR, "ExtractAuthenticatedImage - wRevision: 0x%02x, expect - 0x%02x\n", (UINTN)ImageAuth->AuthInfo.Hdr.wRevision, (UINTN)0x0200));
    return FALSE;
  }

  if (ImageAuth->AuthInfo.Hdr.wCertificateType != WIN_CERT_TYPE_EFI_GUID) {
    DEBUG ((DEBUG_ERROR, "ExtractAuthenticatedImage - wCertificateType: 0x%02x, expect - 0x%02x\n", (UINTN)ImageAuth->AuthInfo.Hdr.wCertificateType, (UINTN)WIN_CERT_TYPE_EFI_GUID));
    return FALSE;
  }

  CertType = &ImageAuth->AuthInfo.CertType;
  DEBUG ((DEBUG_INFO, "ExtractAuthenticatedImage - CertType: %g\n", CertType));

  if (CompareGuid (&gEfiCertPkcs7Guid, CertType)) {
    PublicKeyData       = PcdGetPtr (PcdPkcs7CertBuffer);
    PublicKeyDataLength = PcdGetSize (PcdPkcs7CertBuffer);
  } else if (CompareGuid (&gEfiCertTypeRsa2048Sha256Guid, CertType)) {
    PublicKeyData       = PcdGetPtr (PcdRsa2048Sha256PublicKeyBuffer);
    PublicKeyDataLength = PcdGetSize (PcdRsa2048Sha256PublicKeyBuffer);
  } else {
    return FALSE;
  }

  ASSERT (PublicKeyData != NULL);
  ASSERT (PublicKeyDataLength != 0);

  Status = AuthenticateFmpImage (
             ImageAuth,
             ImageSize,
             PublicKeyData,
             PublicKeyDataLength
             );
  switch (Status) {
    case RETURN_SUCCESS:
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
      break;
    case RETURN_SECURITY_VIOLATION:
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR;
      break;
    case RETURN_INVALID_PARAMETER:
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
      break;
    case RETURN_UNSUPPORTED:
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
      break;
    case RETURN_OUT_OF_RESOURCES:
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES;
      break;
    default:
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
      break;
  }

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (AuthenticatedImage != NULL) {
    *AuthenticatedImage = (UINT8 *)ImageAuth + ImageAuth->AuthInfo.Hdr.dwLength + sizeof (ImageAuth->MonotonicCount);
  }

  if (AuthenticatedImageSize != NULL) {
    *AuthenticatedImageSize = ImageSize - ImageAuth->AuthInfo.Hdr.dwLength - sizeof (ImageAuth->MonotonicCount);
  }

  return TRUE;
}

/**
  Extract ImageFmpInfo from system firmware.

  @param[in]  SystemFirmwareImage     The System Firmware image.
  @param[in]  SystemFirmwareImageSize The size of the System Firmware image in bytes.
  @param[out] ImageFmpInfo            The ImageFmpInfo.
  @param[out] ImageFmpInfoSize        The size of the ImageFmpInfo in bytes.

  @retval TRUE  The ImageFmpInfo is extracted.
  @retval FALSE The ImageFmpInfo is not extracted.
**/
BOOLEAN
EFIAPI
ExtractSystemFirmwareImageFmpInfo (
  IN VOID                                     *SystemFirmwareImage,
  IN UINTN                                    SystemFirmwareImageSize,
  OUT EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR  **ImageFmpInfo,
  OUT UINTN                                   *ImageFmpInfoSize
  )
{
  BOOLEAN  Result;
  UINT32   SectionHeaderSize;
  UINT32   FileHeaderSize;

  *ImageFmpInfo     = NULL;
  *ImageFmpInfoSize = 0;

  Result = GetFfsByName (SystemFirmwareImage, SystemFirmwareImageSize, &gEdkiiSystemFirmwareImageDescriptorFileGuid, EFI_FV_FILETYPE_ALL, (VOID **)ImageFmpInfo, ImageFmpInfoSize);
  if (!Result) {
    return FALSE;
  }

  if (IS_FFS_FILE2 (*ImageFmpInfo)) {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER);
  }

  *ImageFmpInfo     = (VOID *)((UINT8 *)*ImageFmpInfo + FileHeaderSize);
  *ImageFmpInfoSize = *ImageFmpInfoSize - FileHeaderSize;

  Result = GetSectionByType (*ImageFmpInfo, (UINT32)*ImageFmpInfoSize, EFI_SECTION_RAW, 0, (VOID **)ImageFmpInfo, ImageFmpInfoSize);
  if (!Result) {
    return FALSE;
  }

  if (IS_SECTION2 (*ImageFmpInfo)) {
    SectionHeaderSize = sizeof (EFI_RAW_SECTION2);
  } else {
    SectionHeaderSize = sizeof (EFI_RAW_SECTION);
  }

  *ImageFmpInfo     = (VOID *)((UINT8 *)*ImageFmpInfo + SectionHeaderSize);
  *ImageFmpInfoSize = *ImageFmpInfoSize - SectionHeaderSize;

  return TRUE;
}

/**
  Extract the System Firmware image from an authenticated image.

  @param[in]  AuthenticatedImage      The authenticated capsule image.
  @param[in]  AuthenticatedImageSize  The size of the authenticated capsule image in bytes.
  @param[out] SystemFirmwareImage     The System Firmware image.
  @param[out] SystemFirmwareImageSize The size of the System Firmware image in bytes.

  @retval TRUE  The System Firmware image is extracted.
  @retval FALSE The System Firmware image is not extracted.
**/
BOOLEAN
EFIAPI
ExtractSystemFirmwareImage (
  IN VOID    *AuthenticatedImage,
  IN UINTN   AuthenticatedImageSize,
  OUT VOID   **SystemFirmwareImage,
  OUT UINTN  *SystemFirmwareImageSize
  )
{
  BOOLEAN  Result;
  UINT32   FileHeaderSize;

  *SystemFirmwareImage     = NULL;
  *SystemFirmwareImageSize = 0;

  Result = GetFfsByName (AuthenticatedImage, AuthenticatedImageSize, &mEdkiiSystemFirmwareFileGuid, EFI_FV_FILETYPE_RAW, SystemFirmwareImage, SystemFirmwareImageSize);
  if (!Result) {
    // no nested FV, just return all data.
    *SystemFirmwareImage     = AuthenticatedImage;
    *SystemFirmwareImageSize = AuthenticatedImageSize;

    return TRUE;
  }

  if (IS_FFS_FILE2 (*SystemFirmwareImage)) {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    FileHeaderSize = sizeof (EFI_FFS_FILE_HEADER);
  }

  *SystemFirmwareImage     = (UINT8 *)*SystemFirmwareImage + FileHeaderSize;
  *SystemFirmwareImageSize = *SystemFirmwareImageSize - FileHeaderSize;

  return Result;
}

/**
  Authenticated system firmware FMP capsule image.

  Caution: This function may receive untrusted input.

  @param[in]  Image                   The FMP capsule image, including EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               The size of FMP capsule image in bytes.
  @param[in]  ForceVersionMatch       TRUE: The version of capsule must be as same as the version of current image.
                                      FALSE: The version of capsule must be as same as greater than the lowest
                                             supported version of current image.
  @param[out] LastAttemptVersion      The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] AuthenticatedImage      The authenticated capsule image, excluding EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[out] AuthenticatedImageSize  The size of the authenticated capsule image in bytes.

  @retval TRUE  Authentication passes and the authenticated image is extracted.
  @retval FALSE Authentication fails and the authenticated image is not extracted.
**/
EFI_STATUS
EFIAPI
CapsuleAuthenticateSystemFirmware (
  IN VOID     *Image,
  IN UINTN    ImageSize,
  IN BOOLEAN  ForceVersionMatch,
  OUT UINT32  *LastAttemptVersion,
  OUT UINT32  *LastAttemptStatus,
  OUT VOID    **AuthenticatedImage,
  OUT UINTN   *AuthenticatedImageSize
  )
{
  BOOLEAN                                 Result;
  EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR  *ImageFmpInfo;
  UINTN                                   ImageFmpInfoSize;
  EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR  *CurrentImageFmpInfo;
  UINTN                                   CurrentImageFmpInfoSize;
  VOID                                    *SystemFirmwareImage;
  UINTN                                   SystemFirmwareImageSize;

  *LastAttemptVersion = 0;

  //
  // NOTE: This function need run in an isolated environment.
  // Do not touch FMP protocol and its private structure.
  //
  if (mImageFmpInfo == NULL) {
    DEBUG ((DEBUG_INFO, "ImageFmpInfo is not set\n"));
    return EFI_SECURITY_VIOLATION;
  }

  Result = ExtractAuthenticatedImage ((VOID *)Image, ImageSize, LastAttemptStatus, AuthenticatedImage, AuthenticatedImageSize);
  if (!Result) {
    DEBUG ((DEBUG_INFO, "ExtractAuthenticatedImage - fail\n"));
    return EFI_SECURITY_VIOLATION;
  }

  DEBUG ((DEBUG_INFO, "AuthenticatedImage - 0x%x - 0x%x\n", *AuthenticatedImage, *AuthenticatedImageSize));

  Result = ExtractSystemFirmwareImage (*AuthenticatedImage, *AuthenticatedImageSize, &SystemFirmwareImage, &SystemFirmwareImageSize);
  if (!Result) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    DEBUG ((DEBUG_INFO, "ExtractSystemFirmwareImage - fail\n"));
    return EFI_SECURITY_VIOLATION;
  }

  DEBUG ((DEBUG_INFO, "SystemFirmwareImage - 0x%x - 0x%x\n", SystemFirmwareImage, SystemFirmwareImageSize));

  Result = ExtractSystemFirmwareImageFmpInfo (SystemFirmwareImage, SystemFirmwareImageSize, &ImageFmpInfo, &ImageFmpInfoSize);
  if (!Result) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    DEBUG ((DEBUG_INFO, "ExtractSystemFirmwareImageFmpInfo - fail\n"));
    return EFI_SECURITY_VIOLATION;
  }

  *LastAttemptVersion = ImageFmpInfo->Version;
  DEBUG ((DEBUG_INFO, "ImageFmpInfo - 0x%x - 0x%x\n", ImageFmpInfo, ImageFmpInfoSize));
  DEBUG ((DEBUG_INFO, "NewImage Version - 0x%x\n", ImageFmpInfo->Version));
  DEBUG ((DEBUG_INFO, "NewImage LowestSupportedImageVersion - 0x%x\n", ImageFmpInfo->LowestSupportedImageVersion));

  CurrentImageFmpInfo     = mImageFmpInfo;
  CurrentImageFmpInfoSize = mImageFmpInfoSize;

  DEBUG ((DEBUG_INFO, "ImageFmpInfo - 0x%x - 0x%x\n", CurrentImageFmpInfo, CurrentImageFmpInfoSize));
  DEBUG ((DEBUG_INFO, "Current Version - 0x%x\n", CurrentImageFmpInfo->Version));
  DEBUG ((DEBUG_INFO, "Current LowestSupportedImageVersion - 0x%x\n", CurrentImageFmpInfo->LowestSupportedImageVersion));

  if (ForceVersionMatch) {
    if (CurrentImageFmpInfo->Version != ImageFmpInfo->Version) {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INCORRECT_VERSION;
      DEBUG ((DEBUG_INFO, "ForceVersionMatch check - fail\n"));
      return EFI_SECURITY_VIOLATION;
    }
  } else {
    if (ImageFmpInfo->Version < CurrentImageFmpInfo->LowestSupportedImageVersion) {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INCORRECT_VERSION;
      DEBUG ((DEBUG_INFO, "LowestSupportedImageVersion check - fail\n"));
      return EFI_SECURITY_VIOLATION;
    }
  }

  *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
  return EFI_SUCCESS;
}

/**
  PcdCallBack gets the real set PCD value

  @param[in]      CallBackGuid    The PCD token GUID being set.
  @param[in]      CallBackToken   The PCD token number being set.
  @param[in, out] TokenData       A pointer to the token data being set.
  @param[in]      TokenDataSize   The size, in bytes, of the data being set.

**/
VOID
EFIAPI
EdkiiSystemCapsuleLibPcdCallBack (
  IN        CONST GUID  *CallBackGuid  OPTIONAL,
  IN        UINTN       CallBackToken,
  IN  OUT   VOID        *TokenData,
  IN        UINTN       TokenDataSize
  )
{
  if (CompareGuid (CallBackGuid, &gEfiSignedCapsulePkgTokenSpaceGuid) &&
      (CallBackToken == PcdToken (PcdEdkiiSystemFirmwareImageDescriptor)))
  {
    mImageFmpInfoSize = TokenDataSize;
    mImageFmpInfo     = AllocateCopyPool (mImageFmpInfoSize, TokenData);
    ASSERT (mImageFmpInfo != NULL);
    //
    // Cancel Callback after get the real set value
    //
    LibPcdCancelCallback (
      &gEfiSignedCapsulePkgTokenSpaceGuid,
      PcdToken (PcdEdkiiSystemFirmwareImageDescriptor),
      EdkiiSystemCapsuleLibPcdCallBack
      );
  }

  if (CompareGuid (CallBackGuid, &gEfiSignedCapsulePkgTokenSpaceGuid) &&
      (CallBackToken == PcdToken (PcdEdkiiSystemFirmwareFileGuid)))
  {
    CopyGuid (&mEdkiiSystemFirmwareFileGuid, TokenData);
    //
    // Cancel Callback after get the real set value
    //
    LibPcdCancelCallback (
      &gEfiSignedCapsulePkgTokenSpaceGuid,
      PcdToken (PcdEdkiiSystemFirmwareFileGuid),
      EdkiiSystemCapsuleLibPcdCallBack
      );
  }
}

/**
  The constructor function.

  @retval EFI_SUCCESS   The constructor successfully .
**/
EFI_STATUS
EFIAPI
EdkiiSystemCapsuleLibConstructor (
  VOID
  )
{
  mImageFmpInfoSize = PcdGetSize (PcdEdkiiSystemFirmwareImageDescriptor);
  mImageFmpInfo     = PcdGetPtr (PcdEdkiiSystemFirmwareImageDescriptor);
  //
  // Verify Firmware Image Descriptor first
  //
  if ((mImageFmpInfoSize < sizeof (EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR)) ||
      (mImageFmpInfo->Signature != EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR_SIGNATURE))
  {
    //
    // SystemFirmwareImageDescriptor is not set.
    // Register PCD set callback to hook PCD value set.
    //
    mImageFmpInfo     = NULL;
    mImageFmpInfoSize = 0;
    LibPcdCallbackOnSet (
      &gEfiSignedCapsulePkgTokenSpaceGuid,
      PcdToken (PcdEdkiiSystemFirmwareImageDescriptor),
      EdkiiSystemCapsuleLibPcdCallBack
      );
  } else {
    mImageFmpInfo = AllocateCopyPool (mImageFmpInfoSize, mImageFmpInfo);
    ASSERT (mImageFmpInfo != NULL);
  }

  CopyGuid (&mEdkiiSystemFirmwareFileGuid, PcdGetPtr (PcdEdkiiSystemFirmwareFileGuid));
  //
  // Verify GUID value first
  //
  if (CompareGuid (&mEdkiiSystemFirmwareFileGuid, &gZeroGuid)) {
    LibPcdCallbackOnSet (
      &gEfiSignedCapsulePkgTokenSpaceGuid,
      PcdToken (PcdEdkiiSystemFirmwareFileGuid),
      EdkiiSystemCapsuleLibPcdCallBack
      );
  }

  return EFI_SUCCESS;
}
