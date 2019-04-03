/** @file
  SetImage instance to update Microcode.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  MicrocodeWrite() and VerifyMicrocode() will receive untrusted input and do basic validation.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  Load Microcode on an Application Processor.
  The function prototype for invoking a function on an Application Processor.

  @param[in,out] Buffer  The pointer to private data buffer.
**/
VOID
EFIAPI
MicrocodeLoadAp (
  IN OUT VOID  *Buffer
  )
{
  MICROCODE_LOAD_BUFFER                *MicrocodeLoadBuffer;

  MicrocodeLoadBuffer = Buffer;
  MicrocodeLoadBuffer->Revision = LoadMicrocode (MicrocodeLoadBuffer->Address);
}

/**
  Load new Microcode on this processor

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  CpuIndex                   The index of the processor.
  @param[in]  Address                    The address of new Microcode.

  @return  Loaded Microcode signature.

**/
UINT32
LoadMicrocodeOnThis (
  IN  MICROCODE_FMP_PRIVATE_DATA  *MicrocodeFmpPrivate,
  IN  UINTN                       CpuIndex,
  IN  UINT64                      Address
  )
{
  EFI_STATUS                           Status;
  EFI_MP_SERVICES_PROTOCOL             *MpService;
  MICROCODE_LOAD_BUFFER                MicrocodeLoadBuffer;

  if (CpuIndex == MicrocodeFmpPrivate->BspIndex) {
    return LoadMicrocode (Address);
  } else {
    MpService = MicrocodeFmpPrivate->MpService;
    MicrocodeLoadBuffer.Address = Address;
    MicrocodeLoadBuffer.Revision = 0;
    Status = MpService->StartupThisAP (
                          MpService,
                          MicrocodeLoadAp,
                          CpuIndex,
                          NULL,
                          0,
                          &MicrocodeLoadBuffer,
                          NULL
                          );
    ASSERT_EFI_ERROR(Status);
    return MicrocodeLoadBuffer.Revision;
  }
}

/**
  Collect processor information.
  The function prototype for invoking a function on an Application Processor.

  @param[in,out] Buffer  The pointer to private data buffer.
**/
VOID
EFIAPI
CollectProcessorInfo (
  IN OUT VOID  *Buffer
  )
{
  PROCESSOR_INFO  *ProcessorInfo;

  ProcessorInfo = Buffer;
  ProcessorInfo->ProcessorSignature = GetCurrentProcessorSignature();
  ProcessorInfo->PlatformId = GetCurrentPlatformId();
  ProcessorInfo->MicrocodeRevision = GetCurrentMicrocodeSignature();
}

/**
  Get current Microcode information.

  The ProcessorInformation (BspIndex/ProcessorCount/ProcessorInfo)
  in MicrocodeFmpPrivate must be initialized.

  The MicrocodeInformation (DescriptorCount/ImageDescriptor/MicrocodeInfo)
  in MicrocodeFmpPrivate may not be avaiable in this function.

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
  EFI_STATUS                              Status;
  UINT32                                  AttemptStatus;
  UINTN                                   TargetCpuIndex;

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

      TargetCpuIndex = (UINTN)-1;
      Status = VerifyMicrocode(MicrocodeFmpPrivate, MicrocodeEntryPoint, TotalSize, FALSE, &AttemptStatus, NULL, &TargetCpuIndex);
      if (!EFI_ERROR(Status)) {
        IsInUse = TRUE;
        ASSERT (TargetCpuIndex < MicrocodeFmpPrivate->ProcessorCount);
        MicrocodeFmpPrivate->ProcessorInfo[TargetCpuIndex].MicrocodeIndex = Count;
      } else {
        IsInUse = FALSE;
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
  Return matched processor information.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  ProcessorSignature         The processor signature to be matched
  @param[in]  ProcessorFlags             The processor flags to be matched
  @param[in, out] TargetCpuIndex         On input, the index of target CPU which tries to match the Microcode. (UINTN)-1 means to try all.
                                         On output, the index of target CPU which matches the Microcode.

  @return matched processor information.
**/
PROCESSOR_INFO *
GetMatchedProcessor (
  IN MICROCODE_FMP_PRIVATE_DATA  *MicrocodeFmpPrivate,
  IN UINT32                      ProcessorSignature,
  IN UINT32                      ProcessorFlags,
  IN OUT UINTN                   *TargetCpuIndex
  )
{
  UINTN  Index;

  if (*TargetCpuIndex != (UINTN)-1) {
    Index = *TargetCpuIndex;
    if ((ProcessorSignature == MicrocodeFmpPrivate->ProcessorInfo[Index].ProcessorSignature) &&
        ((ProcessorFlags & (1 << MicrocodeFmpPrivate->ProcessorInfo[Index].PlatformId)) != 0)) {
      return &MicrocodeFmpPrivate->ProcessorInfo[Index];
    } else {
      return NULL;
    }
  }

  for (Index = 0; Index < MicrocodeFmpPrivate->ProcessorCount; Index++) {
    if ((ProcessorSignature == MicrocodeFmpPrivate->ProcessorInfo[Index].ProcessorSignature) &&
        ((ProcessorFlags & (1 << MicrocodeFmpPrivate->ProcessorInfo[Index].PlatformId)) != 0)) {
      *TargetCpuIndex = Index;
      return &MicrocodeFmpPrivate->ProcessorInfo[Index];
    }
  }
  return NULL;
}

/**
  Verify Microcode.

  Caution: This function may receive untrusted input.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  Image                      The Microcode image buffer.
  @param[in]  ImageSize                  The size of Microcode image buffer in bytes.
  @param[in]  TryLoad                    Try to load Microcode or not.
  @param[out] LastAttemptStatus          The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] AbortReason                A pointer to a pointer to a null-terminated string providing more
                                         details for the aborted operation. The buffer is allocated by this function
                                         with AllocatePool(), and it is the caller's responsibility to free it with a
                                         call to FreePool().
  @param[in, out] TargetCpuIndex         On input, the index of target CPU which tries to match the Microcode. (UINTN)-1 means to try all.
                                         On output, the index of target CPU which matches the Microcode.

  @retval EFI_SUCCESS               The Microcode image passes verification.
  @retval EFI_VOLUME_CORRUPTED      The Microcode image is corrupted.
  @retval EFI_INCOMPATIBLE_VERSION  The Microcode image version is incorrect.
  @retval EFI_UNSUPPORTED           The Microcode ProcessorSignature or ProcessorFlags is incorrect.
  @retval EFI_SECURITY_VIOLATION    The Microcode image fails to load.
**/
EFI_STATUS
VerifyMicrocode (
  IN  MICROCODE_FMP_PRIVATE_DATA  *MicrocodeFmpPrivate,
  IN  VOID                        *Image,
  IN  UINTN                       ImageSize,
  IN  BOOLEAN                     TryLoad,
  OUT UINT32                      *LastAttemptStatus,
  OUT CHAR16                      **AbortReason,   OPTIONAL
  IN OUT UINTN                    *TargetCpuIndex
  )
{
  UINTN                                   Index;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   TotalSize;
  UINTN                                   DataSize;
  UINT32                                  CurrentRevision;
  PROCESSOR_INFO                          *ProcessorInfo;
  UINT32                                  InCompleteCheckSum32;
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
  // Check TotalSize
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
  if ((TotalSize & (SIZE_1KB - 1)) != 0) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - TotalSize is not multiples of 1024 bytes (1 KBytes)\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidTotalSize"), L"InvalidTotalSize");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  if (TotalSize != ImageSize) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - TotalSize not equal to ImageSize\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidTotalSize"), L"InvalidTotalSize");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  //
  // Check DataSize
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
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - DataSize is not multiples of DWORDs\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidDataSize"), L"InvalidDataSize");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  //
  // Check CheckSum32
  //
  CheckSum32 = CalculateSum32((UINT32 *)MicrocodeEntryPoint, DataSize + sizeof(CPU_MICROCODE_HEADER));
  if (CheckSum32 != 0) {
    DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on CheckSum32\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
    if (AbortReason != NULL) {
      *AbortReason = AllocateCopyPool(sizeof(L"InvalidChecksum"), L"InvalidChecksum");
    }
    return EFI_VOLUME_CORRUPTED;
  }
  InCompleteCheckSum32 = CheckSum32;
  InCompleteCheckSum32 -= MicrocodeEntryPoint->ProcessorSignature.Uint32;
  InCompleteCheckSum32 -= MicrocodeEntryPoint->ProcessorFlags;
  InCompleteCheckSum32 -= MicrocodeEntryPoint->Checksum;

  //
  // Check ProcessorSignature/ProcessorFlags
  //

  ProcessorInfo = GetMatchedProcessor (MicrocodeFmpPrivate, MicrocodeEntryPoint->ProcessorSignature.Uint32, MicrocodeEntryPoint->ProcessorFlags, TargetCpuIndex);
  if (ProcessorInfo == NULL) {
    CorrectMicrocode = FALSE;
    ExtendedTableLength = TotalSize - (DataSize + sizeof(CPU_MICROCODE_HEADER));
    if (ExtendedTableLength != 0) {
      //
      // Extended Table exist, check if the CPU in support list
      //
      ExtendedTableHeader = (CPU_MICROCODE_EXTENDED_TABLE_HEADER *)((UINT8 *)(MicrocodeEntryPoint) + DataSize + sizeof(CPU_MICROCODE_HEADER));
      //
      // Calculate Extended Checksum
      //
      if ((ExtendedTableLength > sizeof(CPU_MICROCODE_EXTENDED_TABLE_HEADER)) && ((ExtendedTableLength & 0x3) == 0)) {
        CheckSum32 = CalculateSum32((UINT32 *)ExtendedTableHeader, ExtendedTableLength);
        if (CheckSum32 != 0) {
          //
          // Checksum incorrect
          //
          DEBUG((DEBUG_ERROR, "VerifyMicrocode - The checksum for extended table is incorrect\n"));
        } else {
          //
          // Checksum correct
          //
          ExtendedTableCount = ExtendedTableHeader->ExtendedSignatureCount;
          if (ExtendedTableCount > (ExtendedTableLength - sizeof(CPU_MICROCODE_EXTENDED_TABLE_HEADER)) / sizeof(CPU_MICROCODE_EXTENDED_TABLE)) {
            DEBUG((DEBUG_ERROR, "VerifyMicrocode - ExtendedTableCount %d is too big\n", ExtendedTableCount));
          } else {
            ExtendedTable = (CPU_MICROCODE_EXTENDED_TABLE *)(ExtendedTableHeader + 1);
            for (Index = 0; Index < ExtendedTableCount; Index++) {
              CheckSum32 = InCompleteCheckSum32;
              CheckSum32 += ExtendedTable->ProcessorSignature.Uint32;
              CheckSum32 += ExtendedTable->ProcessorFlag;
              CheckSum32 += ExtendedTable->Checksum;
              if (CheckSum32 != 0) {
                DEBUG((DEBUG_ERROR, "VerifyMicrocode - The checksum for ExtendedTable entry with index 0x%x is incorrect\n", Index));
              } else {
                //
                // Verify Header
                //
                ProcessorInfo = GetMatchedProcessor (MicrocodeFmpPrivate, ExtendedTable->ProcessorSignature.Uint32, ExtendedTable->ProcessorFlag, TargetCpuIndex);
                if (ProcessorInfo != NULL) {
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
        DEBUG((DEBUG_ERROR, "VerifyMicrocode - fail on Current ProcessorSignature/ProcessorFlags\n"));
      }
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INCORRECT_VERSION;
      if (AbortReason != NULL) {
        *AbortReason = AllocateCopyPool(sizeof(L"UnsupportedProcessorSignature/ProcessorFlags"), L"UnsupportedProcessorSignature/ProcessorFlags");
      }
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Check UpdateRevision
  //
  CurrentRevision = ProcessorInfo->MicrocodeRevision;
  if ((MicrocodeEntryPoint->UpdateRevision < CurrentRevision) ||
      (TryLoad && (MicrocodeEntryPoint->UpdateRevision == CurrentRevision))) {
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
    CurrentRevision = LoadMicrocodeOnThis(MicrocodeFmpPrivate, ProcessorInfo->CpuIndex, (UINTN)MicrocodeEntryPoint + sizeof(CPU_MICROCODE_HEADER));
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
  Get next FIT Microcode entrypoint.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  MicrocodeEntryPoint        Current Microcode entrypoint

  @return next FIT Microcode entrypoint.
**/
CPU_MICROCODE_HEADER *
GetNextFitMicrocode (
  IN MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate,
  IN CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint
  )
{
  UINTN                                   Index;

  for (Index = 0; Index < MicrocodeFmpPrivate->FitMicrocodeEntryCount; Index++) {
    if (MicrocodeEntryPoint == MicrocodeFmpPrivate->FitMicrocodeInfo[Index].MicrocodeEntryPoint) {
      if (Index == (UINTN) MicrocodeFmpPrivate->FitMicrocodeEntryCount - 1) {
        // it is last one
        return NULL;
      } else {
        // return next one
        return MicrocodeFmpPrivate->FitMicrocodeInfo[Index + 1].MicrocodeEntryPoint;
      }
    }
  }

  ASSERT(FALSE);
  return NULL;
}

/**
  Find empty FIT Microcode entrypoint.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  ImageSize                  The size of Microcode image buffer in bytes.
  @param[out] AvailableSize              Available size of the empty FIT Microcode entrypoint.

  @return Empty FIT Microcode entrypoint.
**/
CPU_MICROCODE_HEADER *
FindEmptyFitMicrocode (
  IN MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate,
  IN UINTN                                   ImageSize,
  OUT UINTN                                  *AvailableSize
  )
{
  UINTN                                   Index;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  CPU_MICROCODE_HEADER                    *NextMicrocodeEntryPoint;
  VOID                                    *MicrocodePatchAddress;
  UINTN                                   MicrocodePatchRegionSize;

  MicrocodePatchAddress = MicrocodeFmpPrivate->MicrocodePatchAddress;
  MicrocodePatchRegionSize = MicrocodeFmpPrivate->MicrocodePatchRegionSize;

  for (Index = 0; Index < MicrocodeFmpPrivate->FitMicrocodeEntryCount; Index++) {
    if (MicrocodeFmpPrivate->FitMicrocodeInfo[Index].Empty) {
      MicrocodeEntryPoint = MicrocodeFmpPrivate->FitMicrocodeInfo[Index].MicrocodeEntryPoint;
      NextMicrocodeEntryPoint = GetNextFitMicrocode (MicrocodeFmpPrivate, MicrocodeEntryPoint);
      if (NextMicrocodeEntryPoint != NULL) {
        *AvailableSize = (UINTN) NextMicrocodeEntryPoint - (UINTN) MicrocodeEntryPoint;
      } else {
        *AvailableSize = (UINTN) MicrocodePatchAddress + MicrocodePatchRegionSize - (UINTN) MicrocodeEntryPoint;
      }
      if (*AvailableSize >= ImageSize) {
        return MicrocodeEntryPoint;
      }
    }
  }

  return NULL;
}

/**
  Find unused FIT Microcode entrypoint.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  ImageSize                  The size of Microcode image buffer in bytes.
  @param[out] AvailableSize              Available size of the unused FIT Microcode entrypoint.

  @return Unused FIT Microcode entrypoint.
**/
CPU_MICROCODE_HEADER *
FindUnusedFitMicrocode (
  IN MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate,
  IN UINTN                                   ImageSize,
  OUT UINTN                                  *AvailableSize
  )
{
  UINTN                                   Index;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  CPU_MICROCODE_HEADER                    *NextMicrocodeEntryPoint;
  VOID                                    *MicrocodePatchAddress;
  UINTN                                   MicrocodePatchRegionSize;

  MicrocodePatchAddress = MicrocodeFmpPrivate->MicrocodePatchAddress;
  MicrocodePatchRegionSize = MicrocodeFmpPrivate->MicrocodePatchRegionSize;

  for (Index = 0; Index < MicrocodeFmpPrivate->FitMicrocodeEntryCount; Index++) {
    if (!MicrocodeFmpPrivate->FitMicrocodeInfo[Index].InUse) {
      MicrocodeEntryPoint = MicrocodeFmpPrivate->FitMicrocodeInfo[Index].MicrocodeEntryPoint;
      NextMicrocodeEntryPoint = GetNextFitMicrocode (MicrocodeFmpPrivate, MicrocodeEntryPoint);
      if (NextMicrocodeEntryPoint != NULL) {
        *AvailableSize = (UINTN) NextMicrocodeEntryPoint - (UINTN) MicrocodeEntryPoint;
      } else {
        *AvailableSize = (UINTN) MicrocodePatchAddress + MicrocodePatchRegionSize - (UINTN) MicrocodeEntryPoint;
      }
      if (*AvailableSize >= ImageSize) {
        return MicrocodeEntryPoint;
      }
    }
  }

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
  DEBUG((DEBUG_INFO, "  Length - 0x%x\n", ImageSize));

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
  Update Microcode flash region with FIT.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  TargetMicrocodeEntryPoint  Target Microcode entrypoint to be updated
  @param[in]  Image                      The Microcode image buffer.
  @param[in]  ImageSize                  The size of Microcode image buffer in bytes.
  @param[out] LastAttemptStatus          The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCCESS             The Microcode image is written.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
UpdateMicrocodeFlashRegionWithFit (
  IN  MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate,
  IN  CPU_MICROCODE_HEADER                    *TargetMicrocodeEntryPoint,
  IN  VOID                                    *Image,
  IN  UINTN                                   ImageSize,
  OUT UINT32                                  *LastAttemptStatus
  )
{
  VOID                                    *MicrocodePatchAddress;
  UINTN                                   MicrocodePatchRegionSize;
  UINTN                                   TargetTotalSize;
  EFI_STATUS                              Status;
  VOID                                    *MicrocodePatchScratchBuffer;
  UINT8                                   *ScratchBufferPtr;
  UINTN                                   ScratchBufferSize;
  UINTN                                   RestSize;
  UINTN                                   AvailableSize;
  VOID                                    *NextMicrocodeEntryPoint;
  VOID                                    *EmptyFitMicrocodeEntry;
  VOID                                    *UnusedFitMicrocodeEntry;

  DEBUG((DEBUG_INFO, "UpdateMicrocodeFlashRegionWithFit: Image - 0x%x, size - 0x%x\n", Image, ImageSize));

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
  // Target data collection
  //
  TargetTotalSize = 0;
  AvailableSize = 0;
  if (TargetMicrocodeEntryPoint != NULL) {
    if (TargetMicrocodeEntryPoint->DataSize == 0) {
      TargetTotalSize = 2048;
    } else {
      TargetTotalSize = TargetMicrocodeEntryPoint->TotalSize;
    }
    DEBUG((DEBUG_INFO, "  TargetTotalSize - 0x%x\n", TargetTotalSize));
    NextMicrocodeEntryPoint = GetNextFitMicrocode (MicrocodeFmpPrivate, TargetMicrocodeEntryPoint);
    DEBUG((DEBUG_INFO, "  NextMicrocodeEntryPoint - 0x%x\n", NextMicrocodeEntryPoint));
    if (NextMicrocodeEntryPoint != NULL) {
      ASSERT ((UINTN) NextMicrocodeEntryPoint >= ((UINTN) TargetMicrocodeEntryPoint + TargetTotalSize));
      AvailableSize = (UINTN) NextMicrocodeEntryPoint - (UINTN) TargetMicrocodeEntryPoint;
    } else {
      AvailableSize = (UINTN) MicrocodePatchAddress + MicrocodePatchRegionSize - (UINTN) TargetMicrocodeEntryPoint;
    }
    DEBUG((DEBUG_INFO, "  AvailableSize - 0x%x\n", AvailableSize));
    ASSERT (AvailableSize >= TargetTotalSize);
  }
  //
  // Total Size means the Microcode size.
  // Available Size means the Microcode size plus the pad till (1) next Microcode or (2) the end.
  //
  // (1)
  // +------+-----------+-----+------+===================+
  // | MCU1 | Microcode | PAD | MCU2 |      Empty        |
  // +------+-----------+-----+------+===================+
  //        | TotalSize |
  //        |<-AvailableSize->|
  //
  // (2)
  // +------+-----------+===================+
  // | MCU  | Microcode |      Empty        |
  // +------+-----------+===================+
  //        | TotalSize |
  //        |<-      AvailableSize        ->|
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
    // |Other | Old Image  | ...  |      Empty        |
    // +------+------------+------+===================+
    //
    // +------+---------+--+------+===================+
    // |Other |New Image|FF| ...  |      Empty        |
    // +------+---------+--+------+===================+
    //
    // 1.1. Copy new image
    CopyMem (ScratchBufferPtr, Image, ImageSize);
    ScratchBufferSize += ImageSize;
    ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    // 1.2. Pad 0xFF
    RestSize = AvailableSize - ImageSize;
    if (RestSize > 0) {
      SetMem (ScratchBufferPtr, RestSize, 0xFF);
      ScratchBufferSize += RestSize;
      ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    }
    Status = UpdateMicrocode((UINTN)TargetMicrocodeEntryPoint, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    return Status;
  }

  //
  // 2. If there is empty FIT microcode entry with enough space, use it.
  //
  EmptyFitMicrocodeEntry = FindEmptyFitMicrocode (MicrocodeFmpPrivate, ImageSize, &AvailableSize);
  if (EmptyFitMicrocodeEntry != NULL) {
    DEBUG((DEBUG_INFO, "Use empty FIT microcode entry\n"));
    // 2.1. Copy new image
    CopyMem (ScratchBufferPtr, Image, ImageSize);
    ScratchBufferSize += ImageSize;
    ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    // 2.2. Pad 0xFF
    RestSize = AvailableSize - ImageSize;
    if (RestSize > 0) {
      SetMem (ScratchBufferPtr, RestSize, 0xFF);
      ScratchBufferSize += RestSize;
      ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    }
    Status = UpdateMicrocode ((UINTN) EmptyFitMicrocodeEntry, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    if (!EFI_ERROR (Status) && (TargetMicrocodeEntryPoint != NULL)) {
      //
      // Empty old microcode.
      //
      ScratchBufferPtr = MicrocodePatchScratchBuffer;
      SetMem (ScratchBufferPtr, TargetTotalSize, 0xFF);
      ScratchBufferSize = TargetTotalSize;
      ScratchBufferPtr = (UINT8 *) MicrocodePatchScratchBuffer + ScratchBufferSize;
      UpdateMicrocode ((UINTN) TargetMicrocodeEntryPoint, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    }
    return Status;
  }

  //
  // 3. If there is unused microcode entry with enough space, use it.
  //
  UnusedFitMicrocodeEntry = FindUnusedFitMicrocode (MicrocodeFmpPrivate, ImageSize, &AvailableSize);
  if (UnusedFitMicrocodeEntry != NULL) {
    DEBUG((DEBUG_INFO, "Use unused FIT microcode entry\n"));
    // 3.1. Copy new image
    CopyMem (ScratchBufferPtr, Image, ImageSize);
    ScratchBufferSize += ImageSize;
    ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    // 3.2. Pad 0xFF
    RestSize = AvailableSize - ImageSize;
    if (RestSize > 0) {
      SetMem (ScratchBufferPtr, RestSize, 0xFF);
      ScratchBufferSize += RestSize;
      ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    }
    Status = UpdateMicrocode ((UINTN) UnusedFitMicrocodeEntry, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    if (!EFI_ERROR (Status) && (TargetMicrocodeEntryPoint != NULL)) {
      //
      // Empty old microcode.
      //
      ScratchBufferPtr = MicrocodePatchScratchBuffer;
      SetMem (ScratchBufferPtr, TargetTotalSize, 0xFF);
      ScratchBufferSize = TargetTotalSize;
      ScratchBufferPtr = (UINT8 *) MicrocodePatchScratchBuffer + ScratchBufferSize;
      UpdateMicrocode ((UINTN) TargetMicrocodeEntryPoint, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    }
    return Status;
  }

  //
  // 4. No usable FIT microcode entry.
  //
  DEBUG((DEBUG_ERROR, "No usable FIT microcode entry\n"));
  *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES;
  Status = EFI_OUT_OF_RESOURCES;

  return Status;
}

/**
  Update Microcode flash region.

  @param[in]  MicrocodeFmpPrivate        The Microcode driver private data
  @param[in]  TargetMicrocodeEntryPoint  Target Microcode entrypoint to be updated
  @param[in]  Image                      The Microcode image buffer.
  @param[in]  ImageSize                  The size of Microcode image buffer in bytes.
  @param[out] LastAttemptStatus          The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCCESS             The Microcode image is written.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
UpdateMicrocodeFlashRegion (
  IN  MICROCODE_FMP_PRIVATE_DATA              *MicrocodeFmpPrivate,
  IN  CPU_MICROCODE_HEADER                    *TargetMicrocodeEntryPoint,
  IN  VOID                                    *Image,
  IN  UINTN                                   ImageSize,
  OUT UINT32                                  *LastAttemptStatus
  )
{
  VOID                                    *MicrocodePatchAddress;
  UINTN                                   MicrocodePatchRegionSize;
  UINTN                                   TargetTotalSize;
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
  // Target data collection
  //
  TargetTotalSize = 0;
  AvailableSize = 0;
  NextMicrocodeEntryPoint = NULL;
  if (TargetMicrocodeEntryPoint != NULL) {
    if (TargetMicrocodeEntryPoint->DataSize == 0) {
      TargetTotalSize = 2048;
    } else {
      TargetTotalSize = TargetMicrocodeEntryPoint->TotalSize;
    }
    DEBUG((DEBUG_INFO, "  TargetTotalSize - 0x%x\n", TargetTotalSize));
    NextMicrocodeEntryPoint = GetNextMicrocode(MicrocodeFmpPrivate, TargetMicrocodeEntryPoint);
    DEBUG((DEBUG_INFO, "  NextMicrocodeEntryPoint - 0x%x\n", NextMicrocodeEntryPoint));
    if (NextMicrocodeEntryPoint != NULL) {
      ASSERT ((UINTN)NextMicrocodeEntryPoint >= ((UINTN)TargetMicrocodeEntryPoint + TargetTotalSize));
      AvailableSize = (UINTN)NextMicrocodeEntryPoint - (UINTN)TargetMicrocodeEntryPoint;
    } else {
      AvailableSize = (UINTN)MicrocodePatchAddress + MicrocodePatchRegionSize - (UINTN)TargetMicrocodeEntryPoint;
    }
    DEBUG((DEBUG_INFO, "  AvailableSize - 0x%x\n", AvailableSize));
    ASSERT (AvailableSize >= TargetTotalSize);
  }
  UsedRegionSize = GetCurrentMicrocodeUsedRegionSize(MicrocodeFmpPrivate);
  DEBUG((DEBUG_INFO, "  UsedRegionSize - 0x%x\n", UsedRegionSize));
  ASSERT (UsedRegionSize >= TargetTotalSize);
  if (TargetMicrocodeEntryPoint != NULL) {
    ASSERT ((UINTN)MicrocodePatchAddress + UsedRegionSize >= ((UINTN)TargetMicrocodeEntryPoint + TargetTotalSize));
  }
  //
  // Total Size means the Microcode size.
  // Available Size means the Microcode size plus the pad till (1) next Microcode or (2) the end.
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
    // |Other | Old Image  | ...  |      Empty        |
    // +------+------------+------+===================+
    //
    // +------+---------+--+------+===================+
    // |Other |New Image|FF| ...  |      Empty        |
    // +------+---------+--+------+===================+
    //
    // 1.1. Copy new image
    CopyMem (ScratchBufferPtr, Image, ImageSize);
    ScratchBufferSize += ImageSize;
    ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    // 1.2. Pad 0xFF
    RestSize = AvailableSize - ImageSize;
    if (RestSize > 0) {
      SetMem (ScratchBufferPtr, RestSize, 0xFF);
      ScratchBufferSize += RestSize;
      ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    }
    Status = UpdateMicrocode((UINTN)TargetMicrocodeEntryPoint, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
    return Status;
  }

  //
  // 2. If there is enough space to remove old one and add new one, reorg and replace old microcode.
  //
  if (MicrocodePatchRegionSize - (UsedRegionSize - TargetTotalSize) >= ImageSize) {
    if (TargetMicrocodeEntryPoint == NULL) {
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
      // |Other | Old Image  | ...  |      Empty        |
      // +------+------------+------+===================+
      //
      // +------+---------------+------+================+
      // |Other |   New Image   | ...  |      Empty     |
      // +------+---------------+------+================+
      //
      // 2.1. Copy new image
      CopyMem (ScratchBufferPtr, Image, ImageSize);
      ScratchBufferSize += ImageSize;
      ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
      // 2.2. Copy rest images after the old image.
      if (NextMicrocodeEntryPoint != 0) {
        RestSize = (UINTN)MicrocodePatchAddress + UsedRegionSize - ((UINTN)NextMicrocodeEntryPoint);
        CopyMem (ScratchBufferPtr, NextMicrocodeEntryPoint, RestSize);
        ScratchBufferSize += RestSize;
        ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
      }
      Status = UpdateMicrocode((UINTN)TargetMicrocodeEntryPoint, MicrocodePatchScratchBuffer, ScratchBufferSize, LastAttemptStatus);
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
    CopyMem (ScratchBufferPtr, Image, ImageSize);
    ScratchBufferSize += ImageSize;
    ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
    // 3.2. Copy some others to rest buffer
    for (Index = 0; Index < MicrocodeCount; Index++) {
      if (!MicrocodeInfo[Index].InUse) {
        continue;
      }
      if (MicrocodeInfo[Index].MicrocodeEntryPoint == TargetMicrocodeEntryPoint) {
        continue;
      }
      if (MicrocodeInfo[Index].TotalSize <= MicrocodePatchRegionSize - ScratchBufferSize) {
        CopyMem (ScratchBufferPtr, MicrocodeInfo[Index].MicrocodeEntryPoint, MicrocodeInfo[Index].TotalSize);
        ScratchBufferSize += MicrocodeInfo[Index].TotalSize;
        ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
      }
    }
    // 3.3. Pad 0xFF
    RestSize = MicrocodePatchRegionSize - ScratchBufferSize;
    if (RestSize > 0) {
      SetMem (ScratchBufferPtr, RestSize, 0xFF);
      ScratchBufferSize += RestSize;
      ScratchBufferPtr = (UINT8 *)MicrocodePatchScratchBuffer + ScratchBufferSize;
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
  @retval EFI_VOLUME_CORRUPTED      The Microcode image is corrupted.
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
  CPU_MICROCODE_HEADER                    *TargetMicrocodeEntryPoint;
  UINTN                                   TargetCpuIndex;
  UINTN                                   TargetMicrcodeIndex;

  //
  // MCU must be 16 bytes aligned
  //
  AlignedImage = AllocateCopyPool(ImageSize, Image);
  if (AlignedImage == NULL) {
    DEBUG((DEBUG_ERROR, "Fail to allocate aligned image\n"));
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES;
    return EFI_OUT_OF_RESOURCES;
  }

  TargetCpuIndex = (UINTN)-1;
  Status = VerifyMicrocode(MicrocodeFmpPrivate, AlignedImage, ImageSize, TRUE, LastAttemptStatus, AbortReason, &TargetCpuIndex);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Fail to verify Microcode Region\n"));
    FreePool(AlignedImage);
    return Status;
  }
  DEBUG((DEBUG_INFO, "Pass VerifyMicrocode\n"));
  *LastAttemptVersion = ((CPU_MICROCODE_HEADER *)Image)->UpdateRevision;

  DEBUG((DEBUG_INFO, "  TargetCpuIndex - 0x%x\n", TargetCpuIndex));
  ASSERT (TargetCpuIndex < MicrocodeFmpPrivate->ProcessorCount);
  TargetMicrcodeIndex = MicrocodeFmpPrivate->ProcessorInfo[TargetCpuIndex].MicrocodeIndex;
  DEBUG((DEBUG_INFO, "  TargetMicrcodeIndex - 0x%x\n", TargetMicrcodeIndex));
  if (TargetMicrcodeIndex != (UINTN)-1) {
    ASSERT (TargetMicrcodeIndex < MicrocodeFmpPrivate->DescriptorCount);
    TargetMicrocodeEntryPoint = MicrocodeFmpPrivate->MicrocodeInfo[TargetMicrcodeIndex].MicrocodeEntryPoint;
  } else {
    TargetMicrocodeEntryPoint = NULL;
  }
  DEBUG((DEBUG_INFO, "  TargetMicrocodeEntryPoint - 0x%x\n", TargetMicrocodeEntryPoint));

  if (MicrocodeFmpPrivate->FitMicrocodeInfo != NULL) {
    Status = UpdateMicrocodeFlashRegionWithFit (
               MicrocodeFmpPrivate,
               TargetMicrocodeEntryPoint,
               AlignedImage,
               ImageSize,
               LastAttemptStatus
               );
  } else {
    Status = UpdateMicrocodeFlashRegion (
               MicrocodeFmpPrivate,
               TargetMicrocodeEntryPoint,
               AlignedImage,
               ImageSize,
               LastAttemptStatus
               );
  }

  FreePool(AlignedImage);

  return Status;
}


