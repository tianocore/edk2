/** @file
  SetImage instance to update Microcode.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  MicrocodeWrite() and VerifyMicrocode() will receive untrusted input and do basic validation.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MicrocodeUpdate.h"


/**
  Verify Microcode.

  Caution: This function may receive untrusted input.

  @param[in]  Image              The Microcode image buffer.
  @param[in]  ImageSize          The size of Microcode image buffer in bytes.
  @param[in]  TryLoad            Try to load Microcode or not.
  @param[out] LastAttemptStatus  The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] AbortReason        A pointer to a pointer to a null-terminated string providing more
                                 details for the aborted operation. The buffer is allocated by this function
                                 with AllocatePool(), and it is the caller's responsibility to free it with a
                                 call to FreePool().

  @retval EFI_SUCCESS               The Microcode image passes verification.
  @retval EFI_VOLUME_CORRUPTED      The Microcode image is corrupt.
  @retval EFI_INCOMPATIBLE_VERSION  The Microcode image version is incorrect.
  @retval EFI_UNSUPPORTED           The Microcode ProcessorSignature or ProcessorFlags is incorrect.
  @retval EFI_SECURITY_VIOLATION    The Microcode image fails to load.
**/
EFI_STATUS
VerifyMicrocode (
  IN VOID    *Image,
  IN UINTN   ImageSize,
  IN BOOLEAN TryLoad,
  OUT UINT32 *LastAttemptStatus,
  OUT CHAR16 **AbortReason
  );

/**
  Get Microcode Region.

  @param[out] MicrocodePatchAddress      The address of Microcode
  @param[out] MicrocodePatchRegionSize   The region size of Microcode

  @retval TRUE   The Microcode region is returned.
  @retval FALSE  No Microcode region.
**/
BOOLEAN
GetMicrocodeRegion (
  OUT VOID     **MicrocodePatchAddress,
  OUT UINTN    *MicrocodePatchRegionSize
  )
{
  *MicrocodePatchAddress = (VOID *)(UINTN)PcdGet64(PcdCpuMicrocodePatchAddress);
  *MicrocodePatchRegionSize = (UINTN)PcdGet64(PcdCpuMicrocodePatchRegionSize);

  if ((*MicrocodePatchAddress == NULL) || (*MicrocodePatchRegionSize == 0)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Get Microcode update signature of currently loaded Microcode update.

  @return  Microcode signature.

**/
UINT32
GetCurrentMicrocodeSignature (
  VOID
  )
{
  UINT64 Signature;

  AsmWriteMsr64(MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid(CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  Signature = AsmReadMsr64(MSR_IA32_BIOS_SIGN_ID);
  return (UINT32)RShiftU64(Signature, 32);
}

/**
  Get current processor signature.

  @return current processor signature.
**/
UINT32
GetCurrentProcessorSignature (
  VOID
  )
{
  UINT32                                  RegEax;
  AsmCpuid(CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);
  return RegEax;
}

/**
  Get current platform ID.

  @return current platform ID.
**/
UINT8
GetCurrentPlatformId (
  VOID
  )
{
  UINT8                                   PlatformId;

  PlatformId = (UINT8)AsmMsrBitFieldRead64(MSR_IA32_PLATFORM_ID, 50, 52);
  return PlatformId;
}

/**
  Load new Microcode.

  @param[in] Address  The address of new Microcode.

  @return  Loaded Microcode signature.

**/
UINT32
LoadMicrocode (
  IN UINT64  Address
  )
{
  AsmWriteMsr64(MSR_IA32_BIOS_UPDT_TRIG, Address);
  return GetCurrentMicrocodeSignature();
}

/**
  If the Microcode is used by current processor.

  @param[in]  MicrocodeEntryPoint  The Microcode buffer

  @retval TRUE  The Microcode is used by current processor.
  @retval FALSE The Microcode is NOT used by current processor.
**/
BOOLEAN
IsMicrocodeInUse (
  IN CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint
  )
{
  UINT32      AttemptStatus;
  UINTN       TotalSize;
  EFI_STATUS  Status;

  if (MicrocodeEntryPoint->HeaderVersion == 0x1 && MicrocodeEntryPoint->LoaderRevision == 0x1) {
    //
    // It is the microcode header. It is not the padding data between microcode patches
    // becasue the padding data should not include 0x00000001 and it should be the repeated
    // byte format (like 0xXYXYXYXY....).
    //
    if (MicrocodeEntryPoint->DataSize == 0) {
      TotalSize = 2048;
    } else {
      TotalSize = MicrocodeEntryPoint->TotalSize;
    }
    Status = VerifyMicrocode(MicrocodeEntryPoint, TotalSize, FALSE, &AttemptStatus, NULL);
    if (!EFI_ERROR(Status)) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Get current Microcode information.

  NOTE: The DescriptorCount/ImageDescriptor/MicrocodeInfo in MicrocodeFmpPrivate
  are not avaiable in this function.

  @param[in]   MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]   DescriptorCount            The count of Microcode ImageDescriptor allocated.
  @param[out]  ImageDescriptor            Microcode ImageDescriptor
  @param[out]  MicrocodeInfo              Microcode information

  @return Microcode count
**/
UINTN
GetMicrocodeInfo (
  IN  MICROCODE_FMP_PRIVATE_DATA     *MicrocodeFmpPrivate,
  IN  UINTN                          DescriptorCount,  OPTIONAL
  OUT EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageDescriptor, OPTIONAL
  OUT MICROCODE_INFO                 *MicrocodeInfo    OPTIONAL
  )
{
  VOID                                    *MicrocodePatchAddress;
  UINTN                                   MicrocodePatchRegionSize;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   MicrocodeEnd;
  UINTN                                   TotalSize;
  UINTN                                   Count;
  UINT64                                  ImageAttributes;
  BOOLEAN                                 IsInUse;

  MicrocodePatchAddress = MicrocodeFmpPrivate->MicrocodePatchAddress;
  MicrocodePatchRegionSize = MicrocodeFmpPrivate->MicrocodePatchRegionSize;

  DEBUG((DEBUG_INFO, "Microcode Region - 0x%x - 0x%x\n", MicrocodePatchAddress, MicrocodePatchRegionSize));

  Count = 0;

  MicrocodeEnd = (UINTN)MicrocodePatchAddress + MicrocodePatchRegionSize;
  MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (UINTN) MicrocodePatchAddress;
  do {
    if (MicrocodeEntryPoint->HeaderVersion == 0x1 && MicrocodeEntryPoint->LoaderRevision == 0x1) {
      //
      // It is the microcode header. It is not the padding data between microcode patches
      // becasue the padding data should not include 0x00000001 and it should be the repeated
      // byte format (like 0xXYXYXYXY....).
      //
      if (MicrocodeEntryPoint->DataSize == 0) {
        TotalSize = 2048;
      } else {
        TotalSize = MicrocodeEntryPoint->TotalSize;
      }

      IsInUse = IsMicrocodeInUse (MicrocodeEntryPoint);

      if (ImageDescriptor != NULL && DescriptorCount > Count) {
        ImageDescriptor[Count].ImageIndex = (UINT8)(Count + 1);
        CopyGuid (&ImageDescriptor[Count].ImageTypeId, &gMicrocodeFmpImageTypeIdGuid);
        ImageDescriptor[Count].ImageId = LShiftU64(MicrocodeEntryPoint->ProcessorFlags, 32) + MicrocodeEntryPoint->ProcessorSignature.Uint32;
        ImageDescriptor[Count].ImageIdName = NULL;
        ImageDescriptor[Count].Version = MicrocodeEntryPoint->UpdateRevision;
        ImageDescriptor[Count].VersionName = NULL;
        ImageDescriptor[Count].Size = TotalSize;
        ImageAttributes = IMAGE_ATTRIBUTE_IMAGE_UPDATABLE | IMAGE_ATTRIBUTE_RESET_REQUIRED;
        if (IsInUse) {
          ImageAttributes |= IMAGE_ATTRIBUTE_IN_USE;
        }
        ImageDescriptor[Count].AttributesSupported = ImageAttributes | IMAGE_ATTRIBUTE_IN_USE;
        ImageDescriptor[Count].AttributesSetting = ImageAttributes;
        ImageDescriptor[Count].Compatibilities = 0;
        ImageDescriptor[Count].LowestSupportedImageVersion = MicrocodeEntryPoint->UpdateRevision; // do not support rollback
        ImageDescriptor[Count].LastAttemptVersion = 0;
        ImageDescriptor[Count].LastAttemptStatus = 0;
        ImageDescriptor[Count].HardwareInstance = 0;
      }
      if (MicrocodeInfo != NULL && DescriptorCount > Count) {
        MicrocodeInfo[Count].MicrocodeEntryPoint = MicrocodeEntryPoint;
        MicrocodeInfo[Count].TotalSize = TotalSize;
        MicrocodeInfo[Count].InUse = IsInUse;
      }
    } else {
      //
      // It is the padding data between the microcode patches for microcode patches alignment.
      // Because the microcode patch is the multiple of 1-KByte, the padding data should not
      // exist if the microcode patch alignment value is not larger than 1-KByte. So, the microcode
      // alignment value should be larger than 1-KByte. We could skip SIZE_1KB padding data to
      // find the next possible microcode patch header.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    Count++;
    ASSERT(Count < 0xFF);

    //
    // Get the next patch.
    //
    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + TotalSize);
  } while (((UINTN) MicrocodeEntryPoint < MicrocodeEnd));

  return Count;
}

/**
  Verify Microcode.

  Caution: This function may receive untrusted input.

  @param[in]  Image              The Microcode image buffer.
  @param[in]  ImageSize          The size of Microcode image buffer in bytes.
  @param[in]  TryLoad            Try to load Microcode or not.
  @param[out] LastAttemptStatus  The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] AbortReason        A pointer to a pointer to a null-terminated string providing more
                                 details for the aborted operation. The buffer is allocated by this function
                                 with AllocatePool(), and it is the caller's responsibility to free it with a
                                 call to FreePool().

  @retval EFI_SUCCESS               The Microcode image passes verification.
  @retval EFI_VOLUME_CORRUPTED      The Microcode image is corrupt.
  @retval EFI_INCOMPATIBLE_VERSION  The Microcode image version is incorrect.
  @retval EFI_UNSUPPORTED           The Microcode ProcessorSignature or ProcessorFlags is incorrect.
  @retval EFI_SECURITY_VIOLATION    The Microcode image fails to load.
**/
EFI_STATUS
VerifyMicrocode (
  IN VOID    *Image,
  IN UINTN   ImageSize,
  IN BOOLEAN TryLoad,
  OUT UINT32 *LastAttemptStatus,
  OUT CHAR16 **AbortReason
  )
{
  UINTN                                   Index;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   TotalSize;
  UINTN                                   DataSize;
  UINT32                                  CurrentRevision;
  UINT32                                  CurrentProcessorSignature;
  UINT8                                   CurrentPlatformId;
  UINT32                                  CheckSum32;
  UINTN                                   ExtendedTableLength;
  UINT32                                  ExtendedTableCount;
  CPU_MICROCODE_EXTENDED_TABLE            *ExtendedTable;
  CPU_MICROCODE_EXTENDED_TABLE_HEADER     *ExtendedTableHeader;
  BOOLEAN                                 CorrectMicrocode;

  //
  // Check HeaderVersion
  //
  MicrocodeEntryPoint = Image;
  if (MicrocodeEntryPoint->HeaderVersion != 0x1) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on HeaderVersion\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidHeaderVersion"), L"InvalidHeaderVersion");
    }
    return EFI_INCOMPATIBLE_VERSION;
  }
  //
  // Check LoaderRevision
  //
  if (MicrocodeEntryPoint->LoaderRevision != 0x1) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on LoaderRevision\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidLoaderVersion"), L"InvalidLoaderVersion");
    }
    return EFI_INCOMPATIBLE_VERSION;
  }
  //
  // Check Size
  //
  if (MicrocodeEntryPoint->DataSize == 0) {
    TotalSize = 2048;
  } else {
    TotalSize = MicrocodeEntryPoint->TotalSize;
  }
  if (TotalSize <= sizeof(CPU_MICROCODE_HEADER)) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - TotalSize too small\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidTotalSize"), L"InvalidTotalSize");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  if (TotalSize != ImageSize) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on TotalSize\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidTotalSize"), L"InvalidTotalSize");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  //
  // Check CheckSum32
  //
  if (MicrocodeEntryPoint->DataSize == 0) {
    DataSize = 2048 - sizeof(CPU_MICROCODE_HEADER);
  } else {
    DataSize = MicrocodeEntryPoint->DataSize;
  }
  if (DataSize > TotalSize - sizeof(CPU_MICROCODE_HEADER)) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - DataSize too big\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidDataSize"), L"InvalidDataSize");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  if ((DataSize & 0x3) != 0) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - DataSize not aligned\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidDataSize"), L"InvalidDataSize");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  CheckSum32 = CalculateSum32((UINT32 *)MicrocodeEntryPoint, DataSize + sizeof(CPU_MICROCODE_HEADER));
  if (CheckSum32 != 0) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on CheckSum32\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidChecksum"), L"InvalidChecksum");
    }
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Check ProcessorSignature/ProcessorFlags
  //
  CorrectMicrocode = FALSE;
  CurrentProcessorSignature = GetCurrentProcessorSignature();
  CurrentPlatformId = GetCurrentPlatformId();
  if ((MicrocodeEntryPoint->ProcessorSignature.Uint32 != CurrentProcessorSignature) ||
      ((MicrocodeEntryPoint->ProcessorFlags & (1 << CurrentPlatformId)) == 0)) {
    ExtendedTableLength = TotalSize - (DataSize + sizeof(CPU_MICROCODE_HEADER));
    if (ExtendedTableLength != 0) {
      //
      // Extended Table exist, check if the CPU in support list
      //
      ExtendedTableHeader = (CPU_MICROCODE_EXTENDED_TABLE_HEADER *)((UINT8 *)(MicrocodeEntryPoint) + DataSize + sizeof(CPU_MICROCODE_HEADER));
      //
      // Calculate Extended Checksum
      //
      if ((ExtendedTableLength > sizeof(CPU_MICROCODE_EXTENDED_TABLE_HEADER)) && ((ExtendedTableLength & 0x3) != 0)) {
        CheckSum32 = CalculateSum32((UINT32 *)ExtendedTableHeader, ExtendedTableLength);
        if (CheckSum32 == 0) {
          //
          // Checksum correct
          //
          ExtendedTableCount = ExtendedTableHeader->ExtendedSignatureCount;
          if (ExtendedTableCount <= (ExtendedTableLength - sizeof(CPU_MICROCODE_EXTENDED_TABLE_HEADER)) / sizeof(CPU_MICROCODE_EXTENDED_TABLE)) {
            ExtendedTable = (CPU_MICROCODE_EXTENDED_TABLE *)(ExtendedTableHeader + 1);
            for (Index = 0; Index < ExtendedTableCount; Index++) {
              CheckSum32 = CalculateSum32((UINT32 *)ExtendedTable, sizeof(CPU_MICROCODE_EXTENDED_TABLE));
              if (CheckSum32 == 0) {
                //
                // Verify Header
                //
                if ((ExtendedTable->ProcessorSignature.Uint32 == CurrentProcessorSignature) &&
                    (ExtendedTable->ProcessorFlag & (1 << CurrentPlatformId))) {
                  //
                  // Find one
                  //
                  CorrectMicrocode = TRUE;
                  break;
                }
              }
              ExtendedTable++;
            }
          }
        }
      }
    }
    if (!CorrectMicrocode) {
      if (TryLoad) {
        DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on CurrentProcessorSignature/ProcessorFlags\n"));
      }
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INCORRECT_VERSION;
      if (AbortReason != NULL) {
        if (MicrocodeEntryPoint->ProcessorSignature.Uint32 != CurrentProcessorSignature) {
          *AbortReason = AllocateCopyPool(sizeof(L"UnsupportedProcessSignature"), L"UnsupportedProcessSignature");
        } else {
          *AbortReason = AllocateCopyPool(sizeof(L"UnsupportedProcessorFlags"), L"UnsupportedProcessorFlags");
        }
      }
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Check UpdateRevision
  //
  CurrentRevision = GetCurrentMicrocodeSignature();
  if (MicrocodeEntryPoint->UpdateRevision < CurrentRevision) {
    if (TryLoad) {
      DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on UpdateRevision\n"));
    }
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INCORRECT_VERSION;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"IncorrectRevision"), L"IncorrectRevision");
    }
    return EFI_INCOMPATIBLE_VERSION;
  }

  //
  // try load MCU
  //
  if (TryLoad) {
    CurrentRevision = LoadMicrocode((UINTN)MicrocodeEntryPoint + sizeof(CPU_MICROCODE_HEADER));
    if (MicrocodeEntryPoint->UpdateRevision != CurrentRevision) {
      DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on LoadMicrocode\n"));
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR;
      if (AbortReason != NULL) {
        *AbortReason = AllocateCopyPool(sizeof(L"InvalidData"), L"InvalidData");
      }
      return EFI_SECURITY_VIOLATION;
    }
  }

  return EFI_SUCCESS;
}

/**
  Get current Microcode in used.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data

  @return current Microcode in used.
**/
VOID *
GetCurrentMicrocodeInUse (
  IN MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate
  )
{
  UINTN                                   Index;

  for (Index = 0; Index < MicrocodeFmpPrivate->DescriptorCount; Index++) {
    if (!MicrocodeFmpPrivate->MicrocodeInfo[Index].InUse) {
      continue;
    }
    if (IsMicrocodeInUse (MicrocodeFmpPrivate->MicrocodeInfo[Index].MicrocodeEntryPoint)) {
      return MicrocodeFmpPrivate->MicrocodeInfo[Index].MicrocodeEntryPoint;
    }
  }
  return NULL;
}

/**
  Get next Microcode entrypoint.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  MicrocodeEntryPoint        Current Microcode entrypoint

  @return next Microcode entrypoint.
**/
CPU_MICROCODE_HEADER *
GetNextMicrocode (
  IN MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate,
  IN CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint
  )
{
  UINTN                                   Index;

  for (Index = 0; Index < MicrocodeFmpPrivate->DescriptorCount; Index++) {
    if (MicrocodeEntryPoint == MicrocodeFmpPrivate->MicrocodeInfo[Index].MicrocodeEntryPoint) {
      if (Index == (UINTN)MicrocodeFmpPrivate->DescriptorCount - 1) {
        // it is last one
        return NULL;
      } else {
        // return next one
        return MicrocodeFmpPrivate->MicrocodeInfo[Index + 1].MicrocodeEntryPoint;
      }
    }
  }

  ASSERT(FALSE);
  return NULL;
}

/**
  Get current Microcode used region size.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data

  @return current Microcode used region size.
**/
UINTN
GetCurrentMicrocodeUsedRegionSize (
  IN MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate
  )
{
  if (MicrocodeFmpPrivate->DescriptorCount == 0) {
    return 0;
  }

  return (UINTN)MicrocodeFmpPrivate->MicrocodeInfo[MicrocodeFmpPrivate->DescriptorCount - 1].MicrocodeEntryPoint
         + (UINTN)MicrocodeFmpPrivate->MicrocodeInfo[MicrocodeFmpPrivate->DescriptorCount - 1].TotalSize
         - (UINTN)MicrocodeFmpPrivate->MicrocodePatchAddress;
}

/**
  Update Microcode.

  @param[in]   Address            The flash address of Microcode.
  @param[in]   Image              The Microcode image buffer.
  @param[in]   ImageSize          The size of Microcode image buffer in bytes.
  @param[out]  LastAttemptStatus  The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCCESS           The Microcode image is updated.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
**/
EFI_STATUS
UpdateMicrocode (
  IN UINT64   Address,
  IN VOID     *Image,
  IN UINTN    ImageSize,
  OUT UINT32  *LastAttemptStatus
  )
{
  EFI_STATUS  Status;

  DEBUG((DEBUG_INFO, "PlatformUpdate:"));
  DEBUG((DEBUG_INFO, "  Address - 0x%lx,", Address));
  DEBUG((DEBUG_INFO, "  Legnth - 0x%x\n", ImageSize));

  Status = MicrocodeFlashWrite (
             Address,
             Image,
             ImageSize
             );
  if (!EFI_ERROR(Status)) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
  } else {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
  }
  return Status;
}

/**
  Update Microcode flash region.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  CurrentMicrocodeEntryPoint Current Microcode entrypoint
  @param[in]  Image                      The Microcode image buffer.
  @param[in]  ImageSize                  The size of Microcode image buffer in bytes.
  @param[out] LastAttemptStatus          The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCCESS             The Microcode image is written.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
UpdateMicrocodeFlashRegion (
  IN  MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate,
  IN  CPU_MICROCODE_HEADER                    *CurrentMicrocodeEntryPoint,
  IN  VOID                                    *Image,
  IN  UINTN                                   ImageSize,
  OUT UINT32                                  *LastAttemptStatus
  )
{
  VOID                                    *MicrocodePatchAddress;
  UINTN                                   MicrocodePatchRegionSize;
  UINTN                                   CurrentTotalSize;
  UINTN                                   UsedRegionSize;
  EFI_STATUS                              Status;
  VOID                                    *MicrocodePatchScratchBuffer;
  UINT8                                   *ScratchBufferPtr;
  UINTN                                   ScratchBufferSize;
  UINTN                                   RestSize;
  UINTN                                   AvailableSize;
  VOID                                    *NextMicrocodeEntryPoint;
  MICROCODE_INFO                          *MicrocodeInfo;
  UINTN                                   MicrocodeCount;
  UINTN                                   Index;

  DEBUG((DEBUG_INFO, "UpdateMicrocodeFlashRegion: Image - 0x%x, size - 0x%x\n", Image, ImageSize));

  MicrocodePatchAddress = MicrocodeFmpPrivate->MicrocodePatchAddress;
  MicrocodePatchRegionSize = MicrocodeFmpPrivate->MicrocodePatchRegionSize;

  MicrocodePatchScratchBuffer = AllocateZeroPool (MicrocodePatchRegionSize);
  if (MicrocodePatchScratchBuffer == NULL) {
    DEBUG((DEBUG_ERROR, "Fail to allocate Microcode Scratch buffer\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES;
    return EFI_OUT_OF_RESOURCES;
  }
  ScratchBufferPtr = MicrocodePatchScratchBuffer;
  ScratchBufferSize = 0;

  //
  // Current data collection
  //
  CurrentTotalSize = 0;
  AvailableSize = 0;
  NextMicrocodeEntryPoint = NULL;
  if (CurrentMicrocodeEntryPoint != NULL) {
    if (CurrentMicrocodeEntryPoint->DataSize == 0) {
      CurrentTotalSize = 2048;
    } else {
      CurrentTotalSize = CurrentMicrocodeEntryPoint->TotalSize;
    }
    DEBUG((DEBUG_INFO, "  CurrentTotalSize - 0x%x\n", CurrentTotalSize));
    NextMicrocodeEntryPoint = GetNextMicrocode(MicrocodeFmpPrivate, CurrentMicrocodeEntryPoint);
    DEBUG((DEBUG_INFO, "  NextMicrocodeEntryPoint - 0x%x\n", NextMicrocodeEntryPoint));
    if (NextMicrocodeEntryPoint != NULL) {
      ASSERT ((UINTN)NextMicrocodeEntryPoint >= ((UINTN)CurrentMicrocodeEntryPoint + CurrentTotalSize));
      AvailableSize = (UINTN)NextMicrocodeEntryPoint - (UINTN)CurrentMicrocodeEntryPoint;
    } else {
      AvailableSize = (UINTN)MicrocodePatchAddress + MicrocodePatchRegionSize - (UINTN)CurrentMicrocodeEntryPoint;
    }
    DEBUG((DEBUG_INFO, "  AvailableSize - 0x%x\n", AvailableSize));
  }
  ASSERT (AvailableSize >= CurrentTotalSize);
  UsedRegionSize = GetCurrentMicrocodeUsedRegionSize(MicrocodeFmpPrivate);
  DEBUG((DEBUG_INFO, "  UsedRegionSize - 0x%x\n", UsedRegionSize));
  ASSERT (UsedRegionSize >= CurrentTotalSize);
  if (CurrentMicrocodeEntryPoint != NULL) {
    ASSERT ((UINTN)MicrocodePatchAddress + UsedRegionSize >= ((UINTN)CurrentMicrocodeEntryPoint + CurrentTotalSize));
  }
  //
  // Total Size means the Microcode data size.
  // Available Size means the Microcode data size plus the pad till next (1) Microcode or (2) the end.
  //
  // (1)
  // +------+-----------+-----+------+===================+
  // | MCU1 | Microcode | PAD | MCU2 |      Empty        |
  // +------+-----------+-----+------+===================+
  //        | TotalSize |
  //        |<-AvailableSize->|
  // |<-        UsedRegionSize     ->|
  //
  // (2)
  // +------+-----------+===================+
  // | MCU  | Microcode |      Empty        |
  // +------+-----------+===================+
  //        | TotalSize |
  //        |<-      AvailableSize        ->|
  // |<-UsedRegionSize->|
  //

  //
  // Update based on policy
  //

  //
  // 1. If there is enough space to update old one in situ, replace old microcode in situ.
  //
  if (AvailableSize >= ImageSize) {
    DEBUG((DEBUG_INFO, "Replace old microcode in situ\n"));
    //
    // +------+------------+------+===================+
    // |Other1| Old Image  |Other2|      Empty        |
    // +------+------------+------+===================+
    //
    // +------+---------+--+------+===================+
    // |Other1|New Image|FF|Other2|      Empty        |
    // +------+---------+--+------+===================+
    //
    // 1.1. Copy new image
    CopyMem (ScratchBufferPtr, Image, ImageSize);
    ScratchBufferSize += ImageSize;
    ScratchBufferPtr = (UINT8 *)ScratchBufferPtr + ScratchBufferSize;
    // 1.2. Pad 0xFF
    RestSize = AvailableSize - ImageSize;
    if (RestSize > 0) {
      SetMem (ScratchBufferPtr, RestSize, 0xFF);
      ScratchBufferSize += RestSize;
      ScratchBufferPtr = (UINT8 *)ScratchBufferPtr + ScratchBufferSize;
    }
    Status = UpdateMicrocode((UINTN)CurrentMicrocodeEntryPoint, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    return Status;
  }

  //
  // 2. If there is enough space to remove old one and add new one, reorg and replace old microcode.
  //
  if (MicrocodePatchRegionSize - (UsedRegionSize - CurrentTotalSize) >= ImageSize) {
    if (CurrentMicrocodeEntryPoint == NULL) {
      DEBUG((DEBUG_INFO, "Append new microcode\n"));
      //
      // +------+------------+------+===================+
      // |Other1|   Other    |Other2|      Empty        |
      // +------+------------+------+===================+
      //
      // +------+------------+------+-----------+=======+
      // |Other1|   Other    |Other2| New Image | Empty |
      // +------+------------+------+-----------+=======+
      //
      Status = UpdateMicrocode((UINTN)MicrocodePatchAddress + UsedRegionSize, Image, ImageSize, LastAttemptStatus);
    } else {
      DEBUG((DEBUG_INFO, "Reorg and replace old microcode\n"));
      //
      // +------+------------+------+===================+
      // |Other1| Old Image  |Other2|      Empty        |
      // +------+------------+------+===================+
      //
      // +------+---------------+------+================+
      // |Other1|   New Image   |Other2|      Empty     |
      // +------+---------------+------+================+
      //
      // 2.1. Copy new image
      CopyMem (MicrocodePatchScratchBuffer, Image, ImageSize);
      ScratchBufferSize += ImageSize;
      ScratchBufferPtr = (UINT8 *)ScratchBufferPtr + ScratchBufferSize;
      // 2.2. Copy rest images after the old image.
      if (NextMicrocodeEntryPoint != 0) {
        RestSize = (UINTN)MicrocodePatchAddress + UsedRegionSize - ((UINTN)NextMicrocodeEntryPoint);
        CopyMem (ScratchBufferPtr, (UINT8 *)CurrentMicrocodeEntryPoint + CurrentTotalSize, RestSize);
        ScratchBufferSize += RestSize;
        ScratchBufferPtr = (UINT8 *)ScratchBufferPtr + ScratchBufferSize;
      }
      Status = UpdateMicrocode((UINTN)CurrentMicrocodeEntryPoint, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    }
    return Status;
  }

  //
  // 3. The new image can be put in MCU region, but not all others can be put.
  //    So all the unused MCU is removed.
  //
  if (MicrocodePatchRegionSize >= ImageSize) {
    //
    // +------+------------+------+===================+
    // |Other1| Old Image  |Other2|      Empty        |
    // +------+------------+------+===================+
    //
    // +-------------------------------------+--------+
    // |        New Image                    | Other  |
    // +-------------------------------------+--------+
    //
    DEBUG((DEBUG_INFO, "Add new microcode from beginning\n"));

    MicrocodeCount = MicrocodeFmpPrivate->DescriptorCount;
    MicrocodeInfo = MicrocodeFmpPrivate->MicrocodeInfo;

    // 3.1. Copy new image
    CopyMem (MicrocodePatchScratchBuffer, Image, ImageSize);
    ScratchBufferSize += ImageSize;
    ScratchBufferPtr = (UINT8 *)ScratchBufferPtr + ScratchBufferSize;
    // 3.2. Copy some others to rest buffer
    for (Index = 0; Index < MicrocodeCount; Index++) {
      if (!MicrocodeInfo[Index].InUse) {
        continue;
      }
      if (MicrocodeInfo[Index].MicrocodeEntryPoint == CurrentMicrocodeEntryPoint) {
        continue;
      }
      if (MicrocodeInfo[Index].TotalSize <= MicrocodePatchRegionSize - ScratchBufferSize) {
        CopyMem (ScratchBufferPtr, MicrocodeInfo[Index].MicrocodeEntryPoint, MicrocodeInfo[Index].TotalSize);
        ScratchBufferSize += MicrocodeInfo[Index].TotalSize;
        ScratchBufferPtr = (UINT8 *)ScratchBufferPtr + ScratchBufferSize;
      }
    }
    // 3.3. Pad 0xFF
    RestSize = MicrocodePatchRegionSize - ScratchBufferSize;
    if (RestSize > 0) {
      SetMem (ScratchBufferPtr, RestSize, 0xFF);
      ScratchBufferSize += RestSize;
      ScratchBufferPtr = (UINT8 *)ScratchBufferPtr + ScratchBufferSize;
    }
    Status = UpdateMicrocode((UINTN)MicrocodePatchAddress, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    return Status;
  }

  //
  // 4. The new image size is bigger than the whole MCU region.
  //
  DEBUG((DEBUG_ERROR, "Microcode too big\n"));
  *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES;
  Status = EFI_OUT_OF_RESOURCES;

  return Status;
}

/**
  Write Microcode.

  Caution: This function may receive untrusted input.

  @param[in]   MicrocodeFmpPrivate The Microcode driver private data
  @param[in]   Image               The Microcode image buffer.
  @param[in]   ImageSize           The size of Microcode image buffer in bytes.
  @param[out]  LastAttemptVersion  The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]  LastAttemptStatus   The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]  AbortReason         A pointer to a pointer to a null-terminated string providing more
                                   details for the aborted operation. The buffer is allocated by this function
                                   with AllocatePool(), and it is the caller's responsibility to free it with a
                                   call to FreePool().

  @retval EFI_SUCCESS               The Microcode image is written.
  @retval EFI_VOLUME_CORRUPTED      The Microcode image is corrupt.
  @retval EFI_INCOMPATIBLE_VERSION  The Microcode image version is incorrect.
  @retval EFI_SECURITY_VIOLATION    The Microcode image fails to load.
  @retval EFI_WRITE_PROTECTED       The flash device is read only.
**/
EFI_STATUS
MicrocodeWrite (
  IN  MICROCODE_FMP_PRIVATE_DATA   *MicrocodeFmpPrivate,
  IN  VOID                         *Image,
  IN  UINTN                        ImageSize,
  OUT UINT32                       *LastAttemptVersion,
  OUT UINT32                       *LastAttemptStatus,
  OUT CHAR16                       **AbortReason
  )
{
  EFI_STATUS                              Status;
  VOID                                    *AlignedImage;
  CPU_MICROCODE_HEADER                    *CurrentMicrocodeEntryPoint;

  //
  // We must get Current MicrocodeEntrypoint *before* VerifyMicrocode,
  // because the MicrocodeSignature might be updated in VerifyMicrocode.
  //
  CurrentMicrocodeEntryPoint = GetCurrentMicrocodeInUse(MicrocodeFmpPrivate);
  DEBUG((DEBUG_INFO, "  CurrentMicrocodeEntryPoint - 0x%x\n", CurrentMicrocodeEntryPoint));

  //
  // MCU must be 16 bytes aligned
  //
  AlignedImage = AllocateCopyPool(ImageSize, Image);
  if (AlignedImage == NULL) {
    DEBUG((DEBUG_ERROR, "Fail to allocate aligned image\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES;
    return EFI_OUT_OF_RESOURCES;
  }

  *LastAttemptVersion = ((CPU_MICROCODE_HEADER *)Image)->UpdateRevision;
  Status = VerifyMicrocode(AlignedImage, ImageSize, TRUE, LastAttemptStatus, AbortReason);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Fail to verify Microcode Region\n"));
    FreePool(AlignedImage);
    return Status;
  }
  DEBUG((DEBUG_INFO, "Pass VerifyMicrocode\n"));

  Status = UpdateMicrocodeFlashRegion(
             MicrocodeFmpPrivate,
             CurrentMicrocodeEntryPoint,
             AlignedImage,
             ImageSize,
             LastAttemptStatus
             );

  FreePool(AlignedImage);

  return Status;
}


