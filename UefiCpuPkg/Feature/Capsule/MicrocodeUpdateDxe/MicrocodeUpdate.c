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
  Get Microcode Region.

  @param[out] MicrocodePatchAddress      The address of Microcode
  @param[out] MicrocodePatchRegionSize   The region size of Microcode

  @retval TRUE   The Microcode region is returned.
  @retval FALSE  No Microcode region.
**/
BOOLEAN
GetMicrocodeRegion (
  OUT UINT64   *MicrocodePatchAddress,
  OUT UINT64   *MicrocodePatchRegionSize
  )
{
  *MicrocodePatchAddress = PcdGet64(PcdCpuMicrocodePatchAddress);
  *MicrocodePatchRegionSize = PcdGet64(PcdCpuMicrocodePatchRegionSize);

  if ((*MicrocodePatchAddress == 0) || (*MicrocodePatchRegionSize == 0)) {
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
  Get current Microcode information.

  @param[out]  ImageDescriptor  Microcode ImageDescriptor
  @param[in]   DescriptorCount  The count of Microcode ImageDescriptor allocated.

  @return Microcode count
**/
UINTN
GetMicrocodeInfo (
  OUT EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageDescriptor, OPTIONAL
  IN  UINTN                          DescriptorCount   OPTIONAL
  )
{
  BOOLEAN                                 Result;
  UINT64                                  MicrocodePatchAddress;
  UINT64                                  MicrocodePatchRegionSize;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   MicrocodeEnd;
  UINTN                                   TotalSize;
  UINTN                                   Count;
  UINT64                                  ImageAttributes;
  UINT32                                  CurrentRevision;

  Result = GetMicrocodeRegion(&MicrocodePatchAddress, &MicrocodePatchRegionSize);
  if (!Result) {
    DEBUG((DEBUG_ERROR, "Fail to get Microcode Region\n"));
    return 0;
  }
  DEBUG((DEBUG_INFO, "Microcode Region - 0x%lx - 0x%lx\n", MicrocodePatchAddress, MicrocodePatchRegionSize));

  Count = 0;
  CurrentRevision = GetCurrentMicrocodeSignature();

  MicrocodeEnd = (UINTN)(MicrocodePatchAddress + MicrocodePatchRegionSize);
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

      if (ImageDescriptor != NULL && DescriptorCount > Count) {
        ImageDescriptor[Count].ImageIndex = (UINT8)(Count + 1);
        CopyGuid (&ImageDescriptor[Count].ImageTypeId, &gMicrocodeFmpImageTypeIdGuid);
        ImageDescriptor[Count].ImageId = LShiftU64(MicrocodeEntryPoint->ProcessorFlags, 32) + MicrocodeEntryPoint->ProcessorSignature.Uint32;
        ImageDescriptor[Count].ImageIdName = NULL;
        ImageDescriptor[Count].Version = MicrocodeEntryPoint->UpdateRevision;
        ImageDescriptor[Count].VersionName = NULL;
        ImageDescriptor[Count].Size = TotalSize;
        ImageAttributes = IMAGE_ATTRIBUTE_IMAGE_UPDATABLE | IMAGE_ATTRIBUTE_RESET_REQUIRED;
        if (CurrentRevision == MicrocodeEntryPoint->UpdateRevision) {
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
  Read Microcode.

  @param[in]       ImageIndex The index of Microcode image.
  @param[in, out]  Image      The Microcode image buffer.
  @param[in, out]  ImageSize  The size of Microcode image buffer in bytes.

  @retval EFI_SUCCESS    The Microcode image is read.
  @retval EFI_NOT_FOUND  The Microcode image is not found.
**/
EFI_STATUS
MicrocodeRead (
  IN UINTN      ImageIndex,
  IN OUT VOID   *Image,
  IN OUT UINTN  *ImageSize
  )
{
  BOOLEAN                                 Result;
  UINT64                                  MicrocodePatchAddress;
  UINT64                                  MicrocodePatchRegionSize;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   MicrocodeEnd;
  UINTN                                   TotalSize;
  UINTN                                   Count;

  Result = GetMicrocodeRegion(&MicrocodePatchAddress, &MicrocodePatchRegionSize);
  if (!Result) {
    DEBUG((DEBUG_ERROR, "Fail to get Microcode Region\n"));
    return EFI_NOT_FOUND;
  }
  DEBUG((DEBUG_INFO, "Microcode Region - 0x%lx - 0x%lx\n", MicrocodePatchAddress, MicrocodePatchRegionSize));

  Count = 0;

  MicrocodeEnd = (UINTN)(MicrocodePatchAddress + MicrocodePatchRegionSize);
  MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(UINTN)MicrocodePatchAddress;
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

    } else {
      //
      // It is the padding data between the microcode patches for microcode patches alignment.
      // Because the microcode patch is the multiple of 1-KByte, the padding data should not
      // exist if the microcode patch alignment value is not larger than 1-KByte. So, the microcode
      // alignment value should be larger than 1-KByte. We could skip SIZE_1KB padding data to
      // find the next possible microcode patch header.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(((UINTN)MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    Count++;
    ASSERT(Count < 0xFF);

    //
    // Get the next patch.
    //
    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(((UINTN)MicrocodeEntryPoint) + TotalSize);
  } while (((UINTN)MicrocodeEntryPoint < MicrocodeEnd));

  return EFI_NOT_FOUND;
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
      DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on CurrentProcessorSignature/ProcessorFlags\n"));
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
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on UpdateRevision\n"));
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

  @return current Microcode in used.
**/
VOID *
GetCurrentMicrocodeInUse (
  VOID
  )
{
  BOOLEAN                                 Result;
  EFI_STATUS                              Status;
  UINT64                                  MicrocodePatchAddress;
  UINT64                                  MicrocodePatchRegionSize;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   MicrocodeEnd;
  UINTN                                   TotalSize;
  UINTN                                   Count;
  UINT32                                  AttemptStatus;

  Result = GetMicrocodeRegion(&MicrocodePatchAddress, &MicrocodePatchRegionSize);
  if (!Result) {
    DEBUG((DEBUG_ERROR, "Fail to get Microcode Region\n"));
    return NULL;
  }
  DEBUG((DEBUG_INFO, "Microcode Region - 0x%lx - 0x%lx\n", MicrocodePatchAddress, MicrocodePatchRegionSize));

  Count = 0;

  MicrocodeEnd = (UINTN)(MicrocodePatchAddress + MicrocodePatchRegionSize);
  MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(UINTN)MicrocodePatchAddress;
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
      Status = VerifyMicrocode(MicrocodeEntryPoint, TotalSize, FALSE, &AttemptStatus, NULL);
      if (!EFI_ERROR(Status)) {
        return MicrocodeEntryPoint;
      }

    } else {
      //
      // It is the padding data between the microcode patches for microcode patches alignment.
      // Because the microcode patch is the multiple of 1-KByte, the padding data should not
      // exist if the microcode patch alignment value is not larger than 1-KByte. So, the microcode
      // alignment value should be larger than 1-KByte. We could skip SIZE_1KB padding data to
      // find the next possible microcode patch header.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(((UINTN)MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    Count++;
    ASSERT(Count < 0xFF);

    //
    // Get the next patch.
    //
    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(((UINTN)MicrocodeEntryPoint) + TotalSize);
  } while (((UINTN)MicrocodeEntryPoint < MicrocodeEnd));

  return NULL;
}

/**
  Get current Microcode used region size.

  @return current Microcode used region size.
**/
UINTN
GetCurrentMicrocodeUsedRegionSize (
  VOID
  )
{
  BOOLEAN                                 Result;
  UINT64                                  MicrocodePatchAddress;
  UINT64                                  MicrocodePatchRegionSize;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   MicrocodeEnd;
  UINTN                                   TotalSize;
  UINTN                                   Count;
  UINTN                                   MicrocodeUsedEnd;

  Result = GetMicrocodeRegion(&MicrocodePatchAddress, &MicrocodePatchRegionSize);
  if (!Result) {
    DEBUG((DEBUG_ERROR, "Fail to get Microcode Region\n"));
    return 0;
  }
  DEBUG((DEBUG_INFO, "Microcode Region - 0x%lx - 0x%lx\n", MicrocodePatchAddress, MicrocodePatchRegionSize));

  MicrocodeUsedEnd = (UINTN)MicrocodePatchAddress;
  Count = 0;

  MicrocodeEnd = (UINTN)(MicrocodePatchAddress + MicrocodePatchRegionSize);
  MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(UINTN)MicrocodePatchAddress;
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

    } else {
      //
      // It is the padding data between the microcode patches for microcode patches alignment.
      // Because the microcode patch is the multiple of 1-KByte, the padding data should not
      // exist if the microcode patch alignment value is not larger than 1-KByte. So, the microcode
      // alignment value should be larger than 1-KByte. We could skip SIZE_1KB padding data to
      // find the next possible microcode patch header.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(((UINTN)MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    Count++;
    ASSERT(Count < 0xFF);
    MicrocodeUsedEnd = (UINTN)MicrocodeEntryPoint;

    //
    // Get the next patch.
    //
    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(((UINTN)MicrocodeEntryPoint) + TotalSize);
  } while (((UINTN)MicrocodeEntryPoint < MicrocodeEnd));

  return MicrocodeUsedEnd - (UINTN)MicrocodePatchAddress;
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
  Write Microcode.

  Caution: This function may receive untrusted input.

  @param[in]   ImageIndex         The index of Microcode image.
  @param[in]   Image              The Microcode image buffer.
  @param[in]   ImageSize          The size of Microcode image buffer in bytes.
  @param[out]  LastAttemptVersion The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]  LastAttemptStatus  The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]  AbortReason        A pointer to a pointer to a null-terminated string providing more
                             details for the aborted operation. The buffer is allocated by this function
                             with AllocatePool(), and it is the caller's responsibility to free it with a
                             call to FreePool().

  @retval EFI_SUCCESS               The Microcode image is written.
  @retval EFI_VOLUME_CORRUPTED      The Microcode image is corrupt.
  @retval EFI_INCOMPATIBLE_VERSION  The Microcode image version is incorrect.
  @retval EFI_SECURITY_VIOLATION    The Microcode image fails to load.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
**/
EFI_STATUS
MicrocodeWrite (
  IN  UINTN   ImageIndex,
  IN  VOID    *Image,
  IN  UINTN   ImageSize,
  OUT UINT32  *LastAttemptVersion,
  OUT UINT32  *LastAttemptStatus,
  OUT CHAR16  **AbortReason
  )
{
  BOOLEAN                                 Result;
  EFI_STATUS                              Status;
  UINT64                                  MicrocodePatchAddress;
  UINT64                                  MicrocodePatchRegionSize;
  CPU_MICROCODE_HEADER                    *CurrentMicrocodeEntryPoint;
  UINTN                                   CurrentTotalSize;
  UINTN                                   UsedRegionSize;
  VOID                                    *AlignedImage;

  Result = GetMicrocodeRegion(&MicrocodePatchAddress, &MicrocodePatchRegionSize);
  if (!Result) {
    DEBUG((DEBUG_ERROR, "Fail to get Microcode Region\n"));
    return EFI_NOT_FOUND;
  }

  CurrentTotalSize = 0;
  CurrentMicrocodeEntryPoint = GetCurrentMicrocodeInUse();
  if (CurrentMicrocodeEntryPoint != NULL) {
    if (CurrentMicrocodeEntryPoint->DataSize == 0) {
      CurrentTotalSize = 2048;
    } else {
      CurrentTotalSize = CurrentMicrocodeEntryPoint->TotalSize;
    }
  }

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

  if (CurrentTotalSize < ImageSize) {
    UsedRegionSize = GetCurrentMicrocodeUsedRegionSize();
    if (MicrocodePatchRegionSize - UsedRegionSize >= ImageSize) {
      //
      // Append
      //
      DEBUG((DEBUG_INFO, "Append new microcode\n"));
      Status = UpdateMicrocode(MicrocodePatchAddress + UsedRegionSize, AlignedImage, ImageSize, LastAttemptStatus);
    } else if (MicrocodePatchRegionSize >= ImageSize) {
      //
      // Ignor all others and just add this one from beginning.
      //
      DEBUG((DEBUG_INFO, "Add new microcode from beginning\n"));
      Status = UpdateMicrocode(MicrocodePatchAddress, AlignedImage, ImageSize, LastAttemptStatus);
    } else {
      DEBUG((DEBUG_ERROR, "Microcode too big\n"));
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES;
      Status = EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // Replace
    //
    DEBUG((DEBUG_INFO, "Replace old microcode\n"));
    Status = UpdateMicrocode((UINTN)CurrentMicrocodeEntryPoint, AlignedImage, ImageSize, LastAttemptStatus);
  }

  FreePool(AlignedImage);

  return Status;
}


