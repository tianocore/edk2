/** @file
  This driver measures Microcode Patches to TPM.

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
#include <Library/SortLib.h>
#include <Library/HobLib.h>
#include <Library/MicrocodeLib.h>
#include <Library/TpmMeasurementLib.h>


#define CPU_MICROCODE_MEASUREMENT_DESCRIPTION  "Microcode Measurement"
#define CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN  sizeof(CPU_MICROCODE_MEASUREMENT_DESCRIPTION)

#pragma pack(1)
typedef struct {
  UINT64    Address;
  UINT64    Size;
}MICROCODE_PATCH_TYPE;

typedef struct {
  UINT8     Description[CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN];
  UINT64    NumberOfMicrocodePatchesMeasured;
  UINT64    SizeOfMicrocodePatchesMeasured;
}CPU_MICROCODE_MEASUREMENT_EVENT_LOG;
#pragma pack()

STATIC BOOLEAN mMicrocodeMeasured = FALSE;

/**
  Helper Function. Microcode patches list comparison function instance for PerformQuickSort.

  @param[in] Buffer1                  The pointer to first buffer.
  @param[in] Buffer2                  The pointer to second buffer.

  @return 0                           Buffer1 equal to Buffer2.
  @return <0                          Buffer1 is less than Buffer2.
  @return >0                          Buffer1 is greater than Buffer2.
**/
INTN
EFIAPI
MicrocodePatchesListSortFunction (
  IN CONST VOID                 *Buffer1,
  IN CONST VOID                 *Buffer2
  )
{
  return ((MICROCODE_PATCH_TYPE*)Buffer2)->Address - ((MICROCODE_PATCH_TYPE*)Buffer1)->Address;
}

/**
  Measure Microcode Patches Binary Blob with EV_CPU_MICROCODE to PCR[1].

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
  UINT64                                SumOfAllPatchesSizeInMicrocodePatchHob;
  UINT32                                Index;
  MICROCODE_PATCH_TYPE                  *MicrocodePatchesList;
  UINTN                                 LastPackedMicrocodeAddress;
  UINT8                                 *MicrocodePatchesBlob;
  UINT64                                MicrocodePatchesBlobSize;


  PCRIndex  = 1;
  EventType = EV_CPU_MICROCODE;
  AsciiSPrint (
               EventLog.Description,
               CPU_MICROCODE_MEASUREMENT_EVENT_LOG_DESCRIPTION_LEN,
               CPU_MICROCODE_MEASUREMENT_DESCRIPTION
               );
  EventLog.NumberOfMicrocodePatchesMeasured = 0;
  EventLog.SizeOfMicrocodePatchesMeasured   = 0;
  EventLogSize                              = sizeof (CPU_MICROCODE_MEASUREMENT_EVENT_LOG);
  SumOfAllPatchesSizeInMicrocodePatchHob    = 0;
  LastPackedMicrocodeAddress                = 0;
  MicrocodePatchesBlob                      = NULL;
  MicrocodePatchesBlobSize                  = 0;


  if (mMicrocodeMeasured) {
    DEBUG((DEBUG_INFO, "INFO: mMicrocodeMeasured = TRUE, Skip\n"));
    return;
  }

  GuidHob = GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid);
  if (NULL == GuidHob) {
    DEBUG((DEBUG_ERROR, "ERROR: GetFirstGuidHob (&gEdkiiMicrocodePatchHobGuid) failed.\n"));
    return;
  }

  MicrocodePatchHob = GET_GUID_HOB_DATA (GuidHob);
  DEBUG ((DEBUG_INFO, "INFO: Got MicrocodePatchHob with microcode patches starting address:0x%x, microcode patches region size:0x%x, Processor Count:0x%x\n", MicrocodePatchHob->MicrocodePatchAddress, MicrocodePatchHob->MicrocodePatchRegionSize, MicrocodePatchHob->ProcessorCount));

  //
  // Extract all microcode patches to a list from MicrocodePatchHob
  //
  MicrocodePatchesList = AllocatePool (MicrocodePatchHob->ProcessorCount * sizeof (MICROCODE_PATCH_TYPE));
  if (NULL == MicrocodePatchesList) {
    DEBUG ((DEBUG_ERROR, "ERROR: AllocatePool to MicrocodePatchesList Failed!\n"));
    return;
  }
  for (Index = 0; Index < MicrocodePatchHob->ProcessorCount; Index++) {
    if (MAX_UINT64 == MicrocodePatchHob->ProcessorSpecificPatchOffset[Index]) {
      //
      // If no microcode patch was found in a slot, set the address of the microcode patch
      // in that slot to MAX_UINT64, and the size to 0, thus indicates no patch in that slot.
      //
      MicrocodePatchesList[Index].Address = MAX_UINT64;
      MicrocodePatchesList[Index].Size    = 0;

      DEBUG ((DEBUG_INFO, "INFO: Processor#%d: detected no microcode patch\n", Index));
    } else {
      MicrocodePatchesList[Index].Address     = MicrocodePatchHob->MicrocodePatchAddress + MicrocodePatchHob->ProcessorSpecificPatchOffset[Index];
      MicrocodePatchesList[Index].Size        = ((CPU_MICROCODE_HEADER*)(MicrocodePatchHob->MicrocodePatchAddress + MicrocodePatchHob->ProcessorSpecificPatchOffset[Index]))->TotalSize;
      SumOfAllPatchesSizeInMicrocodePatchHob  += MicrocodePatchesList[Index].Size;

      DEBUG ((DEBUG_INFO, "INFO: Processor#%d: Microcode patch address: 0x%x, size: 0x%x\n", Index, MicrocodePatchesList[Index].Address, MicrocodePatchesList[Index].Size));
    }
  }

  //
  // The order matters when pack all microcode patches to a binary blob. Therefore do
  // Sorting before packing.
  // NOTE: We assumed that the order of addresses of all unique microcode patch in RAM
  // is the same with the order of those in the Microcode Firmware Volume. If any future
  // updates made this assumption untenable, then please find a new solution to measure
  // microcode patches.
  //
  PerformQuickSort (
               MicrocodePatchesList,
               MicrocodePatchHob->ProcessorCount,
               sizeof (MICROCODE_PATCH_TYPE),
               MicrocodePatchesListSortFunction
               );
  for (Index = 0; Index < MicrocodePatchHob->ProcessorCount; Index++) {
    DEBUG ((DEBUG_INFO, "INFO: After sorting: Processor#%d: Microcode patch address: 0x%x, size: 0x%x\n", Index, MicrocodePatchesList[Index].Address, MicrocodePatchesList[Index].Size));
  }

  MicrocodePatchesBlob = AllocateZeroPool (SumOfAllPatchesSizeInMicrocodePatchHob);
  if (NULL == MicrocodePatchesBlob) {
    DEBUG ((DEBUG_ERROR, "ERROR - AllocateZeroPool to MicrocodePatchesBlob failed!\n"));
    FreePool (MicrocodePatchesList);
    return;
  }

  //
  // LastPackedMicrocodeAddress is used to skip duplicate microcode patch here.
  //
  for (Index = 0; Index < MicrocodePatchHob->ProcessorCount; Index++) {
    if (MicrocodePatchesList[Index].Address != LastPackedMicrocodeAddress &&
        MicrocodePatchesList[Index].Address != MAX_UINT64) {

      CopyMem (
               (VOID *)(MicrocodePatchesBlob + MicrocodePatchesBlobSize),
               (VOID *)(MicrocodePatchesList[Index].Address),
               (UINTN)(MicrocodePatchesList[Index].Size)
               );
      MicrocodePatchesBlobSize                  += MicrocodePatchesList[Index].Size;
      LastPackedMicrocodeAddress                = MicrocodePatchesList[Index].Address;
      EventLog.NumberOfMicrocodePatchesMeasured += 1;
      EventLog.SizeOfMicrocodePatchesMeasured   += MicrocodePatchesList[Index].Size;

    }
  }

  for (Index = 0; Index < MicrocodePatchesBlobSize; Index++) {
    DEBUG ((DEBUG_INFO, "%2X", MicrocodePatchesBlob[Index]));
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
    mMicrocodeMeasured = TRUE;
    gBS->CloseEvent (Event);
  } else {
    FreePool (MicrocodePatchesList);
    FreePool (MicrocodePatchesBlob);
    DEBUG ((DEBUG_ERROR, "ERROR - TpmMeasureAndLogData failed with %a!\n", Status));
  }

  return;
}

/**

  Driver to produce Microcode measurement.

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
  EFI_STATUS            Status;
  EFI_EVENT             Event;

  //
  // Measure Microcode patches
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             MeasureMicrocodePatches,
             NULL,
             &Event
             );

  return EFI_SUCCESS;
}
