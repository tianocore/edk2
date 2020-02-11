/** @file
  Implementation of loading microcode on processors.

  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"

/**
  Get microcode update signature of currently loaded microcode update.

  @return  Microcode signature.
**/
UINT32
GetCurrentMicrocodeSignature (
  VOID
  )
{
  MSR_IA32_BIOS_SIGN_ID_REGISTER   BiosSignIdMsr;

  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  BiosSignIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID);
  return BiosSignIdMsr.Bits.MicrocodeUpdateSignature;
}

/**
  Detect whether specified processor can find matching microcode patch and load it.

  Microcode Payload as the following format:
  +----------------------------------------+------------------+
  |          CPU_MICROCODE_HEADER          |                  |
  +----------------------------------------+  CheckSum Part1  |
  |            Microcode Binary            |                  |
  +----------------------------------------+------------------+
  |  CPU_MICROCODE_EXTENDED_TABLE_HEADER   |                  |
  +----------------------------------------+  CheckSum Part2  |
  |      CPU_MICROCODE_EXTENDED_TABLE      |                  |
  |                   ...                  |                  |
  +----------------------------------------+------------------+

  There may by multiple CPU_MICROCODE_EXTENDED_TABLE in this format.
  The count of CPU_MICROCODE_EXTENDED_TABLE is indicated by ExtendedSignatureCount
  of CPU_MICROCODE_EXTENDED_TABLE_HEADER structure.

  When we are trying to verify the CheckSum32 with extended table.
  We should use the fields of exnteded table to replace the corresponding
  fields in CPU_MICROCODE_HEADER structure, and recalculate the
  CheckSum32 with CPU_MICROCODE_HEADER + Microcode Binary. We named
  it as CheckSum Part3.

  The CheckSum Part2 is used to verify the CPU_MICROCODE_EXTENDED_TABLE_HEADER
  and CPU_MICROCODE_EXTENDED_TABLE parts. We should make sure CheckSum Part2
  is correct before we are going to verify each CPU_MICROCODE_EXTENDED_TABLE.

  Only ProcessorSignature, ProcessorFlag and CheckSum are different between
  CheckSum Part1 and CheckSum Part3. To avoid multiple computing CheckSum Part3.
  Save an in-complete CheckSum32 from CheckSum Part1 for common parts.
  When we are going to calculate CheckSum32, just should use the corresponding part
  of the ProcessorSignature, ProcessorFlag and CheckSum with in-complete CheckSum32.

  Notes: CheckSum32 is not a strong verification.
         It does not guarantee that the data has not been modified.
         CPU has its own mechanism to verify Microcode Binary part.

  @param[in]  CpuMpData        The pointer to CPU MP Data structure.
  @param[in]  ProcessorNumber  The handle number of the processor. The range is
                               from 0 to the total number of logical processors
                               minus 1.
**/
VOID
MicrocodeDetect (
  IN CPU_MP_DATA             *CpuMpData,
  IN UINTN                   ProcessorNumber
  )
{
  UINT32                                  ExtendedTableLength;
  UINT32                                  ExtendedTableCount;
  CPU_MICROCODE_EXTENDED_TABLE            *ExtendedTable;
  CPU_MICROCODE_EXTENDED_TABLE_HEADER     *ExtendedTableHeader;
  CPU_MICROCODE_HEADER                    *MicrocodeEntryPoint;
  UINTN                                   MicrocodeEnd;
  UINTN                                   Index;
  UINT8                                   PlatformId;
  CPUID_VERSION_INFO_EAX                  Eax;
  CPU_AP_DATA                             *CpuData;
  UINT32                                  CurrentRevision;
  UINT32                                  LatestRevision;
  UINTN                                   TotalSize;
  UINT32                                  CheckSum32;
  UINT32                                  InCompleteCheckSum32;
  BOOLEAN                                 CorrectMicrocode;
  VOID                                    *MicrocodeData;
  MSR_IA32_PLATFORM_ID_REGISTER           PlatformIdMsr;
  UINT32                                  ThreadId;
  BOOLEAN                                 IsBspCallIn;

  if (CpuMpData->MicrocodePatchRegionSize == 0) {
    //
    // There is no microcode patches
    //
    return;
  }

  CurrentRevision = GetCurrentMicrocodeSignature ();
  IsBspCallIn     = (ProcessorNumber == (UINTN)CpuMpData->BspNumber) ? TRUE : FALSE;

  GetProcessorLocationByApicId (GetInitialApicId (), NULL, NULL, &ThreadId);
  if (ThreadId != 0) {
    //
    // Skip loading microcode if it is not the first thread in one core.
    //
    return;
  }

  ExtendedTableLength = 0;
  //
  // Here data of CPUID leafs have not been collected into context buffer, so
  // GetProcessorCpuid() cannot be used here to retrieve CPUID data.
  //
  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, NULL, NULL, NULL);

  //
  // The index of platform information resides in bits 50:52 of MSR IA32_PLATFORM_ID
  //
  PlatformIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_PLATFORM_ID);
  PlatformId = (UINT8) PlatformIdMsr.Bits.PlatformId;


  //
  // Check whether AP has same processor with BSP.
  // If yes, direct use microcode info saved by BSP.
  //
  if (!IsBspCallIn) {
    //
    // Get the CPU data for BSP
    //
    CpuData = &(CpuMpData->CpuData[CpuMpData->BspNumber]);
    if ((CpuData->ProcessorSignature == Eax.Uint32) &&
        (CpuData->PlatformId == PlatformId) &&
        (CpuData->MicrocodeEntryAddr != 0)) {
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(UINTN) CpuData->MicrocodeEntryAddr;
      MicrocodeData       = (VOID *) (MicrocodeEntryPoint + 1);
      LatestRevision      = MicrocodeEntryPoint->UpdateRevision;
      goto Done;
    }
  }

  LatestRevision = 0;
  MicrocodeData  = NULL;
  MicrocodeEnd = (UINTN) (CpuMpData->MicrocodePatchAddress + CpuMpData->MicrocodePatchRegionSize);
  MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (UINTN) CpuMpData->MicrocodePatchAddress;

  do {
    //
    // Check if the microcode is for the Cpu and the version is newer
    // and the update can be processed on the platform
    //
    CorrectMicrocode = FALSE;

    if (MicrocodeEntryPoint->DataSize == 0) {
      TotalSize = sizeof (CPU_MICROCODE_HEADER) + 2000;
    } else {
      TotalSize = sizeof (CPU_MICROCODE_HEADER) + MicrocodeEntryPoint->DataSize;
    }

    ///
    /// 0x0       MicrocodeBegin  MicrocodeEntry  MicrocodeEnd      0xffffffff
    /// |--------------|---------------|---------------|---------------|
    ///                                 valid TotalSize
    /// TotalSize is only valid between 0 and (MicrocodeEnd - MicrocodeEntry).
    /// And it should be aligned with 4 bytes.
    /// If the TotalSize is invalid, skip 1KB to check next entry.
    ///
    if ( (UINTN)MicrocodeEntryPoint > (MAX_ADDRESS - TotalSize) ||
         ((UINTN)MicrocodeEntryPoint + TotalSize) > MicrocodeEnd ||
         (TotalSize & 0x3) != 0
       ) {
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    //
    // Save an in-complete CheckSum32 from CheckSum Part1 for common parts.
    //
    InCompleteCheckSum32 = CalculateSum32 (
                             (UINT32 *) MicrocodeEntryPoint,
                             TotalSize
                             );
    InCompleteCheckSum32 -= MicrocodeEntryPoint->ProcessorSignature.Uint32;
    InCompleteCheckSum32 -= MicrocodeEntryPoint->ProcessorFlags;
    InCompleteCheckSum32 -= MicrocodeEntryPoint->Checksum;

    if (MicrocodeEntryPoint->HeaderVersion == 0x1) {
      //
      // It is the microcode header. It is not the padding data between microcode patches
      // because the padding data should not include 0x00000001 and it should be the repeated
      // byte format (like 0xXYXYXYXY....).
      //
      if (MicrocodeEntryPoint->ProcessorSignature.Uint32 == Eax.Uint32 &&
          MicrocodeEntryPoint->UpdateRevision > LatestRevision &&
          (MicrocodeEntryPoint->ProcessorFlags & (1 << PlatformId))
          ) {
        //
        // Calculate CheckSum Part1.
        //
        CheckSum32 = InCompleteCheckSum32;
        CheckSum32 += MicrocodeEntryPoint->ProcessorSignature.Uint32;
        CheckSum32 += MicrocodeEntryPoint->ProcessorFlags;
        CheckSum32 += MicrocodeEntryPoint->Checksum;
        if (CheckSum32 == 0) {
          CorrectMicrocode = TRUE;
        }
      } else if ((MicrocodeEntryPoint->DataSize != 0) &&
                 (MicrocodeEntryPoint->UpdateRevision > LatestRevision)) {
        ExtendedTableLength = MicrocodeEntryPoint->TotalSize - (MicrocodeEntryPoint->DataSize +
                                sizeof (CPU_MICROCODE_HEADER));
        if (ExtendedTableLength != 0) {
          //
          // Extended Table exist, check if the CPU in support list
          //
          ExtendedTableHeader = (CPU_MICROCODE_EXTENDED_TABLE_HEADER *) ((UINT8 *) (MicrocodeEntryPoint)
                                  + MicrocodeEntryPoint->DataSize + sizeof (CPU_MICROCODE_HEADER));
          //
          // Calculate Extended Checksum
          //
          if ((ExtendedTableLength % 4) == 0) {
            //
            // Calculate CheckSum Part2.
            //
            CheckSum32 = CalculateSum32 ((UINT32 *) ExtendedTableHeader, ExtendedTableLength);
            if (CheckSum32 == 0) {
              //
              // Checksum correct
              //
              ExtendedTableCount = ExtendedTableHeader->ExtendedSignatureCount;
              ExtendedTable      = (CPU_MICROCODE_EXTENDED_TABLE *) (ExtendedTableHeader + 1);
              for (Index = 0; Index < ExtendedTableCount; Index ++) {
                //
                // Calculate CheckSum Part3.
                //
                CheckSum32 = InCompleteCheckSum32;
                CheckSum32 += ExtendedTable->ProcessorSignature.Uint32;
                CheckSum32 += ExtendedTable->ProcessorFlag;
                CheckSum32 += ExtendedTable->Checksum;
                if (CheckSum32 == 0) {
                  //
                  // Verify Header
                  //
                  if ((ExtendedTable->ProcessorSignature.Uint32 == Eax.Uint32) &&
                      (ExtendedTable->ProcessorFlag & (1 << PlatformId)) ) {
                    //
                    // Find one
                    //
                    CorrectMicrocode = TRUE;
                    break;
                  }
                }
                ExtendedTable ++;
              }
            }
          }
        }
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
    //
    // Get the next patch.
    //
    if (MicrocodeEntryPoint->DataSize == 0) {
      TotalSize = 2048;
    } else {
      TotalSize = MicrocodeEntryPoint->TotalSize;
    }

    if (CorrectMicrocode) {
      LatestRevision = MicrocodeEntryPoint->UpdateRevision;
      MicrocodeData = (VOID *) ((UINTN) MicrocodeEntryPoint + sizeof (CPU_MICROCODE_HEADER));
    }

    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + TotalSize);
  } while (((UINTN) MicrocodeEntryPoint < MicrocodeEnd));

Done:
  if (LatestRevision != 0) {
    //
    // Save the detected microcode patch entry address (including the
    // microcode patch header) for each processor.
    // It will be used when building the microcode patch cache HOB.
    //
    CpuMpData->CpuData[ProcessorNumber].MicrocodeEntryAddr =
      (UINTN) MicrocodeData -  sizeof (CPU_MICROCODE_HEADER);
  }

  if (LatestRevision > CurrentRevision) {
    //
    // BIOS only authenticate updates that contain a numerically larger revision
    // than the currently loaded revision, where Current Signature < New Update
    // Revision. A processor with no loaded update is considered to have a
    // revision equal to zero.
    //
    ASSERT (MicrocodeData != NULL);
    AsmWriteMsr64 (
        MSR_IA32_BIOS_UPDT_TRIG,
        (UINT64) (UINTN) MicrocodeData
        );
    //
    // Get and check new microcode signature
    //
    CurrentRevision = GetCurrentMicrocodeSignature ();
    if (CurrentRevision != LatestRevision) {
      AcquireSpinLock(&CpuMpData->MpLock);
      DEBUG ((EFI_D_ERROR, "Updated microcode signature [0x%08x] does not match \
                loaded microcode signature [0x%08x]\n", CurrentRevision, LatestRevision));
      ReleaseSpinLock(&CpuMpData->MpLock);
    }
  }
}

/**
  Determine if a microcode patch matchs the specific processor signature and flag.

  @param[in]  CpuMpData             The pointer to CPU MP Data structure.
  @param[in]  ProcessorSignature    The processor signature field value
                                    supported by a microcode patch.
  @param[in]  ProcessorFlags        The prcessor flags field value supported by
                                    a microcode patch.

  @retval TRUE     The specified microcode patch will be loaded.
  @retval FALSE    The specified microcode patch will not be loaded.
**/
BOOLEAN
IsProcessorMatchedMicrocodePatch (
  IN CPU_MP_DATA                 *CpuMpData,
  IN UINT32                      ProcessorSignature,
  IN UINT32                      ProcessorFlags
  )
{
  UINTN          Index;
  CPU_AP_DATA    *CpuData;

  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    CpuData = &CpuMpData->CpuData[Index];
    if ((ProcessorSignature == CpuData->ProcessorSignature) &&
        (ProcessorFlags & (1 << CpuData->PlatformId)) != 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check the 'ProcessorSignature' and 'ProcessorFlags' of the microcode
  patch header with the CPUID and PlatformID of the processors within
  system to decide if it will be copied into memory.

  @param[in]  CpuMpData             The pointer to CPU MP Data structure.
  @param[in]  MicrocodeEntryPoint   The pointer to the microcode patch header.

  @retval TRUE     The specified microcode patch need to be loaded.
  @retval FALSE    The specified microcode patch dosen't need to be loaded.
**/
BOOLEAN
IsMicrocodePatchNeedLoad (
  IN CPU_MP_DATA                 *CpuMpData,
  CPU_MICROCODE_HEADER           *MicrocodeEntryPoint
  )
{
  BOOLEAN                                NeedLoad;
  UINTN                                  DataSize;
  UINTN                                  TotalSize;
  CPU_MICROCODE_EXTENDED_TABLE_HEADER    *ExtendedTableHeader;
  UINT32                                 ExtendedTableCount;
  CPU_MICROCODE_EXTENDED_TABLE           *ExtendedTable;
  UINTN                                  Index;

  //
  // Check the 'ProcessorSignature' and 'ProcessorFlags' in microcode patch header.
  //
  NeedLoad = IsProcessorMatchedMicrocodePatch (
               CpuMpData,
               MicrocodeEntryPoint->ProcessorSignature.Uint32,
               MicrocodeEntryPoint->ProcessorFlags
               );

  //
  // If the Extended Signature Table exists, check if the processor is in the
  // support list
  //
  DataSize  = MicrocodeEntryPoint->DataSize;
  TotalSize = (DataSize == 0) ? 2048 : MicrocodeEntryPoint->TotalSize;
  if ((!NeedLoad) && (DataSize != 0) &&
      (TotalSize - DataSize > sizeof (CPU_MICROCODE_HEADER) +
                              sizeof (CPU_MICROCODE_EXTENDED_TABLE_HEADER))) {
    ExtendedTableHeader = (CPU_MICROCODE_EXTENDED_TABLE_HEADER *) ((UINT8 *) (MicrocodeEntryPoint)
                            + DataSize + sizeof (CPU_MICROCODE_HEADER));
    ExtendedTableCount  = ExtendedTableHeader->ExtendedSignatureCount;
    ExtendedTable       = (CPU_MICROCODE_EXTENDED_TABLE *) (ExtendedTableHeader + 1);

    for (Index = 0; Index < ExtendedTableCount; Index ++) {
      //
      // Check the 'ProcessorSignature' and 'ProcessorFlag' of the Extended
      // Signature Table entry with the CPUID and PlatformID of the processors
      // within system to decide if it will be copied into memory
      //
      NeedLoad = IsProcessorMatchedMicrocodePatch (
                   CpuMpData,
                   ExtendedTable->ProcessorSignature.Uint32,
                   ExtendedTable->ProcessorFlag
                   );
      if (NeedLoad) {
        break;
      }
      ExtendedTable ++;
    }
  }

  return NeedLoad;
}


/**
  Actual worker function that shadows the required microcode patches into memory.

  @param[in, out]  CpuMpData        The pointer to CPU MP Data structure.
  @param[in]       Patches          The pointer to an array of information on
                                    the microcode patches that will be loaded
                                    into memory.
  @param[in]       PatchCount       The number of microcode patches that will
                                    be loaded into memory.
  @param[in]       TotalLoadSize    The total size of all the microcode patches
                                    to be loaded.
**/
VOID
ShadowMicrocodePatchWorker (
  IN OUT CPU_MP_DATA             *CpuMpData,
  IN     MICROCODE_PATCH_INFO    *Patches,
  IN     UINTN                   PatchCount,
  IN     UINTN                   TotalLoadSize
  )
{
  UINTN    Index;
  VOID     *MicrocodePatchInRam;
  UINT8    *Walker;

  ASSERT ((Patches != NULL) && (PatchCount != 0));

  MicrocodePatchInRam = AllocatePages (EFI_SIZE_TO_PAGES (TotalLoadSize));
  if (MicrocodePatchInRam == NULL) {
    return;
  }

  //
  // Load all the required microcode patches into memory
  //
  for (Walker = MicrocodePatchInRam, Index = 0; Index < PatchCount; Index++) {
    CopyMem (
      Walker,
      (VOID *) Patches[Index].Address,
      Patches[Index].Size
      );
    Walker += Patches[Index].Size;
  }

  //
  // Update the microcode patch related fields in CpuMpData
  //
  CpuMpData->MicrocodePatchAddress    = (UINTN) MicrocodePatchInRam;
  CpuMpData->MicrocodePatchRegionSize = TotalLoadSize;

  DEBUG ((
    DEBUG_INFO,
    "%a: Required microcode patches have been loaded at 0x%lx, with size 0x%lx.\n",
    __FUNCTION__, CpuMpData->MicrocodePatchAddress, CpuMpData->MicrocodePatchRegionSize
    ));

  return;
}

/**
  Shadow the required microcode patches data into memory according to PCD
  PcdCpuMicrocodePatchAddress and PcdCpuMicrocodePatchRegionSize.

  @param[in, out]  CpuMpData    The pointer to CPU MP Data structure.
**/
VOID
ShadowMicrocodePatchByPcd (
  IN OUT CPU_MP_DATA             *CpuMpData
  )
{
  CPU_MICROCODE_HEADER                   *MicrocodeEntryPoint;
  UINTN                                  MicrocodeEnd;
  UINTN                                  DataSize;
  UINTN                                  TotalSize;
  MICROCODE_PATCH_INFO                   *PatchInfoBuffer;
  UINTN                                  MaxPatchNumber;
  UINTN                                  PatchCount;
  UINTN                                  TotalLoadSize;

  //
  // Initialize the microcode patch related fields in CpuMpData as the values
  // specified by the PCD pair. If the microcode patches are loaded into memory,
  // these fields will be updated.
  //
  CpuMpData->MicrocodePatchAddress    = PcdGet64 (PcdCpuMicrocodePatchAddress);
  CpuMpData->MicrocodePatchRegionSize = PcdGet64 (PcdCpuMicrocodePatchRegionSize);

  MicrocodeEntryPoint    = (CPU_MICROCODE_HEADER *) (UINTN) CpuMpData->MicrocodePatchAddress;
  MicrocodeEnd           = (UINTN) MicrocodeEntryPoint +
                           (UINTN) CpuMpData->MicrocodePatchRegionSize;
  if ((MicrocodeEntryPoint == NULL) || ((UINTN) MicrocodeEntryPoint == MicrocodeEnd)) {
    //
    // There is no microcode patches
    //
    return;
  }

  PatchCount      = 0;
  MaxPatchNumber  = DEFAULT_MAX_MICROCODE_PATCH_NUM;
  TotalLoadSize   = 0;
  PatchInfoBuffer = AllocatePool (MaxPatchNumber * sizeof (MICROCODE_PATCH_INFO));
  if (PatchInfoBuffer == NULL) {
    return;
  }

  //
  // Process the header of each microcode patch within the region.
  // The purpose is to decide which microcode patch(es) will be loaded into memory.
  //
  do {
    if (MicrocodeEntryPoint->HeaderVersion != 0x1) {
      //
      // Padding data between the microcode patches, skip 1KB to check next entry.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    DataSize  = MicrocodeEntryPoint->DataSize;
    TotalSize = (DataSize == 0) ? 2048 : MicrocodeEntryPoint->TotalSize;
    if ( (UINTN)MicrocodeEntryPoint > (MAX_ADDRESS - TotalSize) ||
         ((UINTN)MicrocodeEntryPoint + TotalSize) > MicrocodeEnd ||
         (DataSize & 0x3) != 0 ||
         (TotalSize & (SIZE_1KB - 1)) != 0 ||
         TotalSize < DataSize
       ) {
      //
      // Not a valid microcode header, skip 1KB to check next entry.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

    if (IsMicrocodePatchNeedLoad (CpuMpData, MicrocodeEntryPoint)) {
      PatchCount++;
      if (PatchCount > MaxPatchNumber) {
        //
        // Current 'PatchInfoBuffer' cannot hold the information, double the size
        // and allocate a new buffer.
        //
        if (MaxPatchNumber > MAX_UINTN / 2 / sizeof (MICROCODE_PATCH_INFO)) {
          //
          // Overflow check for MaxPatchNumber
          //
          goto OnExit;
        }

        PatchInfoBuffer = ReallocatePool (
                            MaxPatchNumber * sizeof (MICROCODE_PATCH_INFO),
                            2 * MaxPatchNumber * sizeof (MICROCODE_PATCH_INFO),
                            PatchInfoBuffer
                            );
        if (PatchInfoBuffer == NULL) {
          goto OnExit;
        }
        MaxPatchNumber = MaxPatchNumber * 2;
      }

      //
      // Store the information of this microcode patch
      //
      PatchInfoBuffer[PatchCount - 1].Address = (UINTN) MicrocodeEntryPoint;
      PatchInfoBuffer[PatchCount - 1].Size    = TotalSize;
      TotalLoadSize += TotalSize;
    }

    //
    // Process the next microcode patch
    //
    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) (((UINTN) MicrocodeEntryPoint) + TotalSize);
  } while (((UINTN) MicrocodeEntryPoint < MicrocodeEnd));

  if (PatchCount != 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a: 0x%x microcode patches will be loaded into memory, with size 0x%x.\n",
      __FUNCTION__, PatchCount, TotalLoadSize
      ));

    ShadowMicrocodePatchWorker (CpuMpData, PatchInfoBuffer, PatchCount, TotalLoadSize);
  }

OnExit:
  if (PatchInfoBuffer != NULL) {
    FreePool (PatchInfoBuffer);
  }
  return;
}

/**
  Shadow the required microcode patches data into memory.

  @param[in, out]  CpuMpData    The pointer to CPU MP Data structure.
**/
VOID
ShadowMicrocodeUpdatePatch (
  IN OUT CPU_MP_DATA             *CpuMpData
  )
{
  EFI_STATUS     Status;

  Status = PlatformShadowMicrocode (CpuMpData);
  if (EFI_ERROR (Status)) {
    ShadowMicrocodePatchByPcd (CpuMpData);
  }
}

/**
  Get the cached microcode patch base address and size from the microcode patch
  information cache HOB.

  @param[out] Address       Base address of the microcode patches data.
                            It will be updated if the microcode patch
                            information cache HOB is found.
  @param[out] RegionSize    Size of the microcode patches data.
                            It will be updated if the microcode patch
                            information cache HOB is found.

  @retval  TRUE     The microcode patch information cache HOB is found.
  @retval  FALSE    The microcode patch information cache HOB is not found.

**/
BOOLEAN
GetMicrocodePatchInfoFromHob (
  UINT64                         *Address,
  UINT64                         *RegionSize
  )
{
  EFI_HOB_GUID_TYPE            *GuidHob;
  EDKII_MICROCODE_PATCH_HOB    *MicrocodePathHob;

  GuidHob = GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid);
  if (GuidHob == NULL) {
    DEBUG((DEBUG_INFO, "%a: Microcode patch cache HOB is not found.\n", __FUNCTION__));
    return FALSE;
  }

  MicrocodePathHob = GET_GUID_HOB_DATA (GuidHob);

  *Address    = MicrocodePathHob->MicrocodePatchAddress;
  *RegionSize = MicrocodePathHob->MicrocodePatchRegionSize;

  DEBUG((
    DEBUG_INFO, "%a: MicrocodeBase = 0x%lx, MicrocodeSize = 0x%lx\n",
    __FUNCTION__, *Address, *RegionSize
    ));

  return TRUE;
}
