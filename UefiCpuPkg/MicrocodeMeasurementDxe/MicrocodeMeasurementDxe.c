/** @file
  This driver measures microcode patches to TPM.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Guid/EventGroup.h>
#include <Guid/MicrocodePatchHob.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/HobLib.h>
#include <Library/MicrocodeLib.h>
#include <Library/TpmMeasurementLib.h>

#define CPU_MICROCODE_MEASUREMENT_DESCRIPTION  "Microcode Measurement"
#define CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN  sizeof(CPU_MICROCODE_MEASUREMENT_DESCRIPTION)

#pragma pack(1)
typedef struct {
  UINT8    Description[CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN];
  UINTN    NumberOfMicrocodePatchesMeasured;
  UINTN    SizeOfMicrocodePatchesMeasured;
} CPU_MICROCODE_MEASUREMENT_EVENT_LOG;
#pragma pack()


/**
  The function is called by QuickSort to compare the order of offsets of
  two microcode patches in RAM relative to their base address. Elements
  will be in ascending order.

  @param[in] Offset1   The pointer to the offset of first microcode patch.
  @param[in] Offset2   The pointer to the offset of second microcode patch.

  @return 1                   The offset of first microcode patch is bigger than that of the second.
  @return -1                  The offset of first microcode patch is smaller than that of the second.
  @return 0                   The offset of first microcode patch equals to that of the second.
**/
INTN
EFIAPI
MicrocodePatchOffsetCompareFunction (
  IN CONST VOID                 *Offset1,
  IN CONST VOID                 *Offset2
  )
{
  if (*(UINTN*)(Offset1) > *(UINTN*)(Offset2)) {
    return 1;
  } else if (*(UINTN*)(Offset1) < *(UINTN*)(Offset2)) {
    return -1;
  } else {
    return 0;
  }
}

/**
  This function remove duplicate and invalid offsets in PatchOffsetList.
  Invalid offset means MAX_UINTN in PatchOffsetList or MAX_UINT64 in the
  field EDKII_MICROCODE_PATCH_HOB.ProcessorSpecificPatchOffset[].

  @param[in, out] PatchOffsetList        On Call as the raw list; On Return as the clean list.
  @param[in, out] PatchOffsetListCount   On Call as the count of raw list; On Return as count
                                         of the clean list.
**/
VOID
EFIAPI
RemoveDuplicateAndInvalidOffset (
  IN OUT UINTN                 **PatchOffsetList,
  IN OUT UINTN                 *PatchOffsetListCount
  )
{
  UINT32                Index;
  UINTN                 *NewPatchOffsetList;
  UINTN                 *Walker;
  UINTN                 NewPatchOffsetListCount;
  UINTN                 LastPatchOffset;
  UINTN                 QuickSortBuffer;

  NewPatchOffsetList            = NULL;
  Walker                        = NULL;
  NewPatchOffsetListCount       = 0;
  LastPatchOffset               = MAX_UINTN;
  QuickSortBuffer               = 0;

  //
  // The order matters when packing all applied microcode patches to a single binary blob.
  // Therefore it is a must to do sorting before packing.
  // NOTE: We assumed that the order of address of every microcode patch in RAM is the same
  // with the order of those in the Microcode Firmware Volume in FLASH. If any future updates
  // made this assumption untenable, then needs a new solution to measure microcode patches.
  //
  QuickSort (
             *PatchOffsetList,
             *PatchOffsetListCount,
             sizeof (UINTN),
             MicrocodePatchOffsetCompareFunction,
             (VOID*) &QuickSortBuffer
             );
  for (Index = 0; Index < *PatchOffsetListCount; Index++) {
    if (*((*PatchOffsetList)+Index) != MAX_UINTN &&
        *((*PatchOffsetList)+Index) != LastPatchOffset) {
      NewPatchOffsetListCount += 1;
      LastPatchOffset         = *((*PatchOffsetList)+Index);
    }
  }

  LastPatchOffset    = MAX_UINTN;
  NewPatchOffsetList = AllocatePool (NewPatchOffsetListCount * sizeof (UINTN));
  Walker             = NewPatchOffsetList;
  for (Index = 0; Index < *PatchOffsetListCount; Index++) {
    if (*((*PatchOffsetList)+Index) != MAX_UINTN &&
        *((*PatchOffsetList)+Index) != LastPatchOffset) {
      *Walker          = *((*PatchOffsetList)+Index);
      LastPatchOffset  = *((*PatchOffsetList)+Index);
      Walker           += 1;
    }
  }

  FreePool (*PatchOffsetList);
  *PatchOffsetList      = NewPatchOffsetList;
  *PatchOffsetListCount = NewPatchOffsetListCount;
}

/**
  Callback function, called after signaling of the Ready to Boot Event.
  Measure microcode patches binary blob with event type EV_CPU_MICROCODE
  to PCR[1] in TPM.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
MeasureMicrocodePatches (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS                            Status;
  UINT32                                PCRIndex;
  UINT32                                EventType;
  CPU_MICROCODE_MEASUREMENT_EVENT_LOG   EventLog;
  UINT32                                EventLogSize;
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EDKII_MICROCODE_PATCH_HOB             *MicrocodePatchHob;
  UINTN                                 *PatchOffsetList;
  UINTN                                 PatchOffsetListCount;
  UINT32                                Index;
  UINTN                                 SumOfAllPatchesSizeAfterClean;
  UINT8                                 *MicrocodePatchesBlob;
  UINT64                                MicrocodePatchesBlobSize;

  PCRIndex  = 1;
  EventType = EV_CPU_MICROCODE;
  AsciiSPrint (
               (CHAR8 *)EventLog.Description,
               CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN,
               CPU_MICROCODE_MEASUREMENT_DESCRIPTION
               );
  EventLog.NumberOfMicrocodePatchesMeasured = 0;
  EventLog.SizeOfMicrocodePatchesMeasured   = 0;
  EventLogSize                              = sizeof (CPU_MICROCODE_MEASUREMENT_EVENT_LOG);
  PatchOffsetList                           = NULL;
  PatchOffsetListCount                      = 0;
  SumOfAllPatchesSizeAfterClean             = 0;
  MicrocodePatchesBlob                      = NULL;
  MicrocodePatchesBlobSize                  = 0;

  GuidHob = GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid);
  if (NULL == GuidHob) {
    DEBUG((DEBUG_ERROR, "ERROR: GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid) failed.\n"));
    return;
  }

  MicrocodePatchHob    = GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "INFO: Got MicrocodePatchHob with microcode patches starting address:0x%x, microcode patches region size:0x%x, processor count:0x%x\n", MicrocodePatchHob->MicrocodePatchAddress, MicrocodePatchHob->MicrocodePatchRegionSize, MicrocodePatchHob->ProcessorCount));

  PatchOffsetList      = AllocatePool (MicrocodePatchHob->ProcessorCount * sizeof (UINTN));
  for (Index = 0; Index < MicrocodePatchHob->ProcessorCount; Index++) {
    PatchOffsetList[Index] = (UINTN)(MicrocodePatchHob->ProcessorSpecificPatchOffset[Index]);
  }
  PatchOffsetListCount = MicrocodePatchHob->ProcessorCount;

  RemoveDuplicateAndInvalidOffset (&PatchOffsetList, &PatchOffsetListCount);

  for (Index = 0; Index < PatchOffsetListCount; Index++) {
    SumOfAllPatchesSizeAfterClean += GetMicrocodeLength ((CPU_MICROCODE_HEADER*)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress + PatchOffsetList[Index])));
  }

  EventLog.NumberOfMicrocodePatchesMeasured = PatchOffsetListCount;
  EventLog.SizeOfMicrocodePatchesMeasured   = SumOfAllPatchesSizeAfterClean;

  MicrocodePatchesBlob = AllocateZeroPool (SumOfAllPatchesSizeAfterClean);
  if (NULL == MicrocodePatchesBlob) {
    DEBUG ((DEBUG_ERROR, "ERROR: AllocateZeroPool to MicrocodePatchesBlob failed!\n"));
    FreePool (PatchOffsetList);
    return;
  }

  for (Index = 0; Index < PatchOffsetListCount; Index++) {
    CopyMem (
             (VOID *)(MicrocodePatchesBlob + MicrocodePatchesBlobSize),
             (VOID *)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress + PatchOffsetList[Index])),
             (UINTN)(GetMicrocodeLength ((CPU_MICROCODE_HEADER*)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress + PatchOffsetList[Index]))))
             );
    MicrocodePatchesBlobSize += GetMicrocodeLength ((CPU_MICROCODE_HEADER*)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress + PatchOffsetList[Index])));
  }

  if (0 == MicrocodePatchesBlobSize) {
    DEBUG ((DEBUG_INFO, "INFO: No microcode patch is ever applied, skip the measurement of microcode!\n"));
    FreePool (PatchOffsetList);
    FreePool (MicrocodePatchesBlob);
    return;
  }

  Status = TpmMeasureAndLogData (
               PCRIndex,                  // PCRIndex
               EventType,                 // EventType
               &EventLog,                 // EventLog
               EventLogSize,              // LogLen
               MicrocodePatchesBlob,      // HashData
               MicrocodePatchesBlobSize   // HashDataLen
               );
  if (!EFI_ERROR (Status)) {
    gBS->CloseEvent (Event);
    DEBUG ((DEBUG_INFO, "INFO: %d Microcode patches are successfully extended to TPM! The total size measured to TPM is 0x%x\n", PatchOffsetListCount, MicrocodePatchesBlobSize));
  } else {
    DEBUG ((DEBUG_ERROR, "ERROR: TpmMeasureAndLogData failed with status %a!\n", Status));
  }

  FreePool (PatchOffsetList);
  FreePool (MicrocodePatchesBlob);
  return;
}

/**

  Driver to produce microcode measurement.

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @return EFI_SUCCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
MicrocodeMeasurementDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_EVENT             Event;

  //
  // Measure Microcode patches
  //
  EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             MeasureMicrocodePatches,
             NULL,
             &Event
             );

  return EFI_SUCCESS;
}
