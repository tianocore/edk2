/** @file
  This driver measures microcode patches to TPM.

  This driver consumes gEdkiiMicrocodePatchHobGuid, packs all unique microcode patch found in gEdkiiMicrocodePatchHobGuid to a binary blob, and measures the binary blob to TPM.

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
#include <Library/HobLib.h>
#include <Library/MicrocodeLib.h>
#include <Library/TpmMeasurementLib.h>

#define CPU_MICROCODE_MEASUREMENT_DESCRIPTION                "Microcode Measurement"
#define CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN  sizeof (CPU_MICROCODE_MEASUREMENT_DESCRIPTION)

#pragma pack(1)
typedef struct {
  UINT8    Description[CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN];
  UINTN    NumberOfMicrocodePatchesMeasured;
  UINTN    SizeOfMicrocodePatchesMeasured;
} CPU_MICROCODE_MEASUREMENT_EVENT_LOG;
#pragma pack()

/**
  Helping function.

  The function is called by QuickSort to compare the order of offsets of
  two microcode patches in RAM relative to their base address. Elements
  will be in ascending order.

  @param[in] Offset1   The pointer to the offset of first microcode patch.
  @param[in] Offset2   The pointer to the offset of second microcode patch.

  @retval 1                   The offset of first microcode patch is bigger than that of the second.
  @retval -1                  The offset of first microcode patch is smaller than that of the second.
  @retval 0                   The offset of first microcode patch equals to that of the second.
**/
INTN
EFIAPI
MicrocodePatchOffsetCompareFunction (
  IN CONST VOID  *Offset1,
  IN CONST VOID  *Offset2
  )
{
  if (*(UINT64 *)(Offset1) > *(UINT64 *)(Offset2)) {
    return 1;
  } else if (*(UINT64 *)(Offset1) < *(UINT64 *)(Offset2)) {
    return -1;
  } else {
    return 0;
  }
}

/**
  This function remove duplicate and invalid offsets in Offsets.

  This function remove duplicate and invalid offsets in Offsets. Invalid offset means MAX_UINT64 in Offsets.

  @param[in] Offsets        Microcode offset list.
  @param[in, out] Count          On call as the count of raw microcode offset list; On return as count of the clean microcode offset list.
  **/
VOID
RemoveDuplicateAndInvalidOffset (
  IN     UINT64  *Offsets,
  IN OUT UINTN   *Count
  )
{
  UINTN   Index;
  UINTN   NewCount;
  UINT64  LastOffset;
  UINT64  QuickSortBuffer;

  //
  // The order matters when packing all applied microcode patches to a single binary blob.
  // Therefore it is a must to do sorting before packing.
  // NOTE: Since microcode patches are sorted by their addresses in memory, the order of
  // addresses in memory of all the microcode patches before sorting is required to be the
  // same in every boot flow. If any future updates made this assumption untenable, then
  // there needs a new solution to measure microcode patches.
  //
  QuickSort (
    Offsets,
    *Count,
    sizeof (UINT64),
    MicrocodePatchOffsetCompareFunction,
    (VOID *)&QuickSortBuffer
    );

  NewCount   = 0;
  LastOffset = MAX_UINT64;
  for (Index = 0; Index < *Count; Index++) {
    //
    // When MAX_UINT64 element is met, all following elements are MAX_UINT64.
    //
    if (Offsets[Index] == MAX_UINT64) {
      break;
    }

    //
    // Remove duplicated offsets
    //
    if (Offsets[Index] != LastOffset) {
      LastOffset        = Offsets[Index];
      Offsets[NewCount] = Offsets[Index];
      NewCount++;
    }
  }

  *Count = NewCount;
}

/**
  Callback function.

  Called after signaling of the Ready to Boot Event. Measure microcode patches binary blob with event type EV_CPU_MICROCODE to PCR[1] in TPM.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
MeasureMicrocodePatches (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS                           Status;
  UINT32                               PCRIndex;
  UINT32                               EventType;
  CPU_MICROCODE_MEASUREMENT_EVENT_LOG  EventLog;
  UINT32                               EventLogSize;
  EFI_HOB_GUID_TYPE                    *GuidHob;
  EDKII_MICROCODE_PATCH_HOB            *MicrocodePatchHob;
  UINT64                               *Offsets;
  UINTN                                Count;
  UINTN                                Index;
  UINTN                                TotalMicrocodeSize;
  UINT8                                *MicrocodePatchesBlob;

  PCRIndex  = 1;
  EventType = EV_CPU_MICROCODE;
  AsciiStrCpyS (
    (CHAR8 *)(EventLog.Description),
    CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN,
    CPU_MICROCODE_MEASUREMENT_DESCRIPTION
    );
  EventLog.NumberOfMicrocodePatchesMeasured = 0;
  EventLog.SizeOfMicrocodePatchesMeasured   = 0;
  EventLogSize                              = sizeof (CPU_MICROCODE_MEASUREMENT_EVENT_LOG);
  Offsets                                   = NULL;
  TotalMicrocodeSize                        = 0;
  Count                                     = 0;

  GuidHob = GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid);
  if (NULL == GuidHob) {
    DEBUG ((DEBUG_ERROR, "ERROR: GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid) failed.\n"));
    return;
  }

  MicrocodePatchHob = GET_GUID_HOB_DATA (GuidHob);
  DEBUG (
    (DEBUG_INFO,
     "INFO: Got MicrocodePatchHob with microcode patches starting address:0x%x, microcode patches region size:0x%x, processor count:0x%x\n",
     MicrocodePatchHob->MicrocodePatchAddress, MicrocodePatchHob->MicrocodePatchRegionSize,
     MicrocodePatchHob->ProcessorCount)
    );

  Offsets = AllocateCopyPool (
              MicrocodePatchHob->ProcessorCount * sizeof (UINT64),
              MicrocodePatchHob->ProcessorSpecificPatchOffset
              );
  Count = MicrocodePatchHob->ProcessorCount;

  RemoveDuplicateAndInvalidOffset (Offsets, &Count);

  if (0 == Count) {
    DEBUG ((DEBUG_INFO, "INFO: No microcode patch is ever applied, skip the measurement of microcode!\n"));
    FreePool (Offsets);
    return;
  }

  for (Index = 0; Index < Count; Index++) {
    TotalMicrocodeSize +=
      GetMicrocodeLength ((CPU_MICROCODE_HEADER *)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress + Offsets[Index])));
  }

  EventLog.NumberOfMicrocodePatchesMeasured = Count;
  EventLog.SizeOfMicrocodePatchesMeasured   = TotalMicrocodeSize;

  MicrocodePatchesBlob = AllocateZeroPool (TotalMicrocodeSize);
  if (NULL == MicrocodePatchesBlob) {
    DEBUG ((DEBUG_ERROR, "ERROR: AllocateZeroPool to MicrocodePatchesBlob failed!\n"));
    FreePool (Offsets);
    return;
  }

  TotalMicrocodeSize = 0;
  for (Index = 0; Index < Count; Index++) {
    CopyMem (
      (VOID *)(MicrocodePatchesBlob + TotalMicrocodeSize),
      (VOID *)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress + Offsets[Index])),
      (UINTN)(GetMicrocodeLength (
                (CPU_MICROCODE_HEADER *)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress +
                                                 Offsets[Index]))
                ))
      );
    TotalMicrocodeSize +=
      GetMicrocodeLength ((CPU_MICROCODE_HEADER *)((UINTN)(MicrocodePatchHob->MicrocodePatchAddress + Offsets[Index])));
  }

  Status = TpmMeasureAndLogData (
             PCRIndex,                                 // PCRIndex
             EventType,                                // EventType
             &EventLog,                                // EventLog
             EventLogSize,                             // LogLen
             MicrocodePatchesBlob,                     // HashData
             TotalMicrocodeSize                        // HashDataLen
             );
  if (!EFI_ERROR (Status)) {
    gBS->CloseEvent (Event);
    DEBUG (
      (DEBUG_INFO,
       "INFO: %d Microcode patches are successfully extended to TPM! The total size measured to TPM is 0x%x\n",
       Count,
       TotalMicrocodeSize)
      );
  } else {
    DEBUG ((DEBUG_ERROR, "ERROR: TpmMeasureAndLogData failed with status %a!\n", Status));
  }

  FreePool (Offsets);
  FreePool (MicrocodePatchesBlob);
  return;
}

/**

  Driver to produce microcode measurement.

  Driver to produce microcode measurement. Which install a callback function on ready to boot event.

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @return EFI_SUCCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
MicrocodeMeasurementDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT  Event;

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
