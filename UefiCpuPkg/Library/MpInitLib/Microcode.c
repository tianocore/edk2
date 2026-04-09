/** @file
  Implementation of loading microcode on processors.

  Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"

/**
  Detect whether specified processor can find matching microcode patch and load it.

  @param[in]  CpuMpData        The pointer to CPU MP Data structure.
  @param[in]  ProcessorNumber  The handle number of the processor. The range is
                               from 0 to the total number of logical processors
                               minus 1.
**/
VOID
MicrocodeDetect (
  IN CPU_MP_DATA  *CpuMpData,
  IN UINTN        ProcessorNumber
  )
{
  CPU_MICROCODE_HEADER        *Microcode;
  UINTN                       MicrocodeEnd;
  CPU_AP_DATA                 *BspData;
  UINT32                      LatestRevision;
  CPU_MICROCODE_HEADER        *LatestMicrocode;
  UINT32                      ThreadId;
  EDKII_PEI_MICROCODE_CPU_ID  MicrocodeCpuId;

  if (CpuMpData->MicrocodePatchRegionSize == 0) {
    //
    // There is no microcode patches
    //
    return;
  }

  GetProcessorLocationByApicId (GetInitialApicId (), NULL, NULL, &ThreadId);
  if (ThreadId != 0) {
    //
    // Skip loading microcode if it is not the first thread in one core.
    //
    return;
  }

  GetProcessorMicrocodeCpuId (&MicrocodeCpuId);

  if (ProcessorNumber != (UINTN)CpuMpData->BspNumber) {
    //
    // Direct use microcode of BSP if AP is the same as BSP.
    // Assume BSP calls this routine() before AP.
    //
    BspData = &(CpuMpData->CpuData[CpuMpData->BspNumber]);
    if ((BspData->ProcessorSignature == MicrocodeCpuId.ProcessorSignature) &&
        (BspData->PlatformId == MicrocodeCpuId.PlatformId) &&
        (BspData->MicrocodeEntryAddr != 0))
    {
      LatestMicrocode = (CPU_MICROCODE_HEADER *)(UINTN)BspData->MicrocodeEntryAddr;
      LatestRevision  = LatestMicrocode->UpdateRevision;
      goto LoadMicrocode;
    }
  }

  //
  // BSP or AP which is different from BSP runs here
  // Use 0 as the starting revision to search for microcode because MicrocodePatchInfo HOB needs
  // the latest microcode location even it's loaded to the processor.
  //
  LatestRevision  = 0;
  LatestMicrocode = NULL;
  Microcode       = (CPU_MICROCODE_HEADER *)(UINTN)CpuMpData->MicrocodePatchAddress;
  MicrocodeEnd    = (UINTN)Microcode + (UINTN)CpuMpData->MicrocodePatchRegionSize;

  do {
    if (!IsValidMicrocode (Microcode, MicrocodeEnd - (UINTN)Microcode, LatestRevision, &MicrocodeCpuId, 1, TRUE)) {
      //
      // It is the padding data between the microcode patches for microcode patches alignment.
      // Because the microcode patch is the multiple of 1-KByte, the padding data should not
      // exist if the microcode patch alignment value is not larger than 1-KByte. So, the microcode
      // alignment value should be larger than 1-KByte. We could skip SIZE_1KB padding data to
      // find the next possible microcode patch header.
      //
      Microcode = (CPU_MICROCODE_HEADER *)((UINTN)Microcode + SIZE_1KB);
      continue;
    }

    LatestMicrocode = Microcode;
    LatestRevision  = LatestMicrocode->UpdateRevision;

    Microcode = (CPU_MICROCODE_HEADER *)(((UINTN)Microcode) + GetMicrocodeLength (Microcode));
  } while ((UINTN)Microcode < MicrocodeEnd);

LoadMicrocode:
  if (LatestRevision != 0) {
    //
    // Save the detected microcode patch entry address (including the microcode
    // patch header) for each processor even it's the same as the loaded one.
    // It will be used when building the microcode patch cache HOB.
    //
    CpuMpData->CpuData[ProcessorNumber].MicrocodeEntryAddr = (UINTN)LatestMicrocode;
  }

  if (LatestRevision > GetProcessorMicrocodeSignature ()) {
    //
    // BIOS only authenticate updates that contain a numerically larger revision
    // than the currently loaded revision, where Current Signature < New Update
    // Revision. A processor with no loaded update is considered to have a
    // revision equal to zero.
    //
    LoadMicrocode (LatestMicrocode);
  }

  //
  // It's possible that the microcode fails to load. Just capture the CPU microcode revision after loading.
  //
  CpuMpData->CpuData[ProcessorNumber].MicrocodeRevision = GetProcessorMicrocodeSignature ();
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
  IN OUT CPU_MP_DATA           *CpuMpData,
  IN     MICROCODE_PATCH_INFO  *Patches,
  IN     UINTN                 PatchCount,
  IN     UINTN                 TotalLoadSize
  )
{
  UINTN  Index;
  VOID   *MicrocodePatchInRam;
  UINT8  *Walker;

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
      (VOID *)Patches[Index].Address,
      Patches[Index].Size
      );
    Walker += Patches[Index].Size;
  }

  //
  // Update the microcode patch related fields in CpuMpData
  //
  CpuMpData->MicrocodePatchAddress    = (UINTN)MicrocodePatchInRam;
  CpuMpData->MicrocodePatchRegionSize = TotalLoadSize;

  DEBUG ((
    DEBUG_INFO,
    "%a: Required microcode patches have been loaded at 0x%lx, with size 0x%lx.\n",
    __func__,
    CpuMpData->MicrocodePatchAddress,
    CpuMpData->MicrocodePatchRegionSize
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
  IN OUT CPU_MP_DATA  *CpuMpData
  )
{
  UINTN                       Index;
  CPU_MICROCODE_HEADER        *MicrocodeEntryPoint;
  UINTN                       MicrocodeEnd;
  UINTN                       TotalSize;
  MICROCODE_PATCH_INFO        *PatchInfoBuffer;
  UINTN                       MaxPatchNumber;
  UINTN                       PatchCount;
  UINTN                       TotalLoadSize;
  EDKII_PEI_MICROCODE_CPU_ID  *MicrocodeCpuIds;
  BOOLEAN                     Valid;

  //
  // Initialize the microcode patch related fields in CpuMpData as the values
  // specified by the PCD pair. If the microcode patches are loaded into memory,
  // these fields will be updated.
  //
  CpuMpData->MicrocodePatchAddress    = PcdGet64 (PcdCpuMicrocodePatchAddress);
  CpuMpData->MicrocodePatchRegionSize = PcdGet64 (PcdCpuMicrocodePatchRegionSize);

  MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(UINTN)CpuMpData->MicrocodePatchAddress;
  MicrocodeEnd        = (UINTN)MicrocodeEntryPoint +
                        (UINTN)CpuMpData->MicrocodePatchRegionSize;
  if ((MicrocodeEntryPoint == NULL) || ((UINTN)MicrocodeEntryPoint == MicrocodeEnd)) {
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

  MicrocodeCpuIds = AllocatePages (
                      EFI_SIZE_TO_PAGES (CpuMpData->CpuCount * sizeof (EDKII_PEI_MICROCODE_CPU_ID))
                      );
  if (MicrocodeCpuIds == NULL) {
    FreePool (PatchInfoBuffer);
    return;
  }

  for (Index = 0; Index < CpuMpData->CpuCount; Index++) {
    MicrocodeCpuIds[Index].PlatformId         = CpuMpData->CpuData[Index].PlatformId;
    MicrocodeCpuIds[Index].ProcessorSignature = CpuMpData->CpuData[Index].ProcessorSignature;
  }

  //
  // Process the header of each microcode patch within the region.
  // The purpose is to decide which microcode patch(es) will be loaded into memory.
  // Microcode checksum is not verified because it's slow when performing on flash.
  //
  do {
    Valid = IsValidMicrocode (
              MicrocodeEntryPoint,
              MicrocodeEnd - (UINTN)MicrocodeEntryPoint,
              0,
              MicrocodeCpuIds,
              CpuMpData->CpuCount,
              FALSE
              );
    if (!Valid) {
      //
      // Padding data between the microcode patches, skip 1KB to check next entry.
      //
      MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)(((UINTN)MicrocodeEntryPoint) + SIZE_1KB);
      continue;
    }

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

    TotalSize = GetMicrocodeLength (MicrocodeEntryPoint);

    //
    // Store the information of this microcode patch
    //
    PatchInfoBuffer[PatchCount - 1].Address = (UINTN)MicrocodeEntryPoint;
    PatchInfoBuffer[PatchCount - 1].Size    = TotalSize;
    TotalLoadSize                          += TotalSize;

    //
    // Process the next microcode patch
    //
    MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *)((UINTN)MicrocodeEntryPoint + TotalSize);
  } while ((UINTN)MicrocodeEntryPoint < MicrocodeEnd);

  if (PatchCount != 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a: 0x%x microcode patches will be loaded into memory, with size 0x%x.\n",
      __func__,
      PatchCount,
      TotalLoadSize
      ));

    ShadowMicrocodePatchWorker (CpuMpData, PatchInfoBuffer, PatchCount, TotalLoadSize);
  }

OnExit:
  if (PatchInfoBuffer != NULL) {
    FreePool (PatchInfoBuffer);
  }

  FreePages (MicrocodeCpuIds, EFI_SIZE_TO_PAGES (CpuMpData->CpuCount * sizeof (EDKII_PEI_MICROCODE_CPU_ID)));
}

/**
  Shadow the required microcode patches data into memory.

  @param[in, out]  CpuMpData    The pointer to CPU MP Data structure.
**/
VOID
ShadowMicrocodeUpdatePatch (
  IN OUT CPU_MP_DATA  *CpuMpData
  )
{
  EFI_STATUS  Status;

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
  UINT64  *Address,
  UINT64  *RegionSize
  )
{
  EFI_HOB_GUID_TYPE          *GuidHob;
  EDKII_MICROCODE_PATCH_HOB  *MicrocodePathHob;

  GuidHob = GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_INFO, "%a: Microcode patch cache HOB is not found.\n", __func__));
    return FALSE;
  }

  MicrocodePathHob = GET_GUID_HOB_DATA (GuidHob);

  *Address    = MicrocodePathHob->MicrocodePatchAddress;
  *RegionSize = MicrocodePathHob->MicrocodePatchRegionSize;

  DEBUG ((
    DEBUG_INFO,
    "%a: MicrocodeBase = 0x%lx, MicrocodeSize = 0x%lx\n",
    __func__,
    *Address,
    *RegionSize
    ));

  return TRUE;
}
