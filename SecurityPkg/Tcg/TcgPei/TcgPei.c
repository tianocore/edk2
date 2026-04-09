/** @file
  Initialize TPM device and measure FVs before handing off control to DXE.

Copyright (c) 2005 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/FirmwareVolumeInfo2.h>
#include <Ppi/LockPhysicalPresence.h>
#include <Ppi/TpmInitialized.h>
#include <Ppi/FirmwareVolume.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>
#include <Ppi/Tcg.h>

#include <Guid/TcgEventHob.h>
#include <Guid/MeasuredFvHob.h>
#include <Guid/TpmInstance.h>
#include <Guid/MigratedFvInfo.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/Tpm12CommandLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/PerformanceLib.h>

BOOLEAN  mImageInMemory = FALSE;

EFI_PEI_PPI_DESCRIPTOR  mTpmInitializedPpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializedPpiGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mTpmInitializationDonePpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

  @param[in]      This          Indicates the calling context
  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
HashLogExtendEvent (
  IN      EDKII_TCG_PPI      *This,
  IN      UINT64             Flags,
  IN      UINT8              *HashData,
  IN      UINTN              HashDataLen,
  IN      TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  );

EDKII_TCG_PPI  mEdkiiTcgPpi = {
  HashLogExtendEvent
};

EFI_PEI_PPI_DESCRIPTOR  mTcgPpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEdkiiTcgPpiGuid,
  &mEdkiiTcgPpi
};

//
// Number of firmware blobs to grow by each time we run out of room
//
#define FIRMWARE_BLOB_GROWTH_STEP  4

EFI_PLATFORM_FIRMWARE_BLOB  *mMeasuredBaseFvInfo;
UINT32                      mMeasuredMaxBaseFvIndex = 0;
UINT32                      mMeasuredBaseFvIndex    = 0;

EFI_PLATFORM_FIRMWARE_BLOB  *mMeasuredChildFvInfo;
UINT32                      mMeasuredMaxChildFvIndex = 0;
UINT32                      mMeasuredChildFvIndex    = 0;

EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI  *mMeasurementExcludedFvPpi;

/**
  Lock physical presence if needed.

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          Operation completed successfully.

**/
EFI_STATUS
EFIAPI
PhysicalPresencePpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Measure and record the Firmware Volume Information once FvInfoPPI install.

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The FV Info is measured and recorded to TPM.
  @return Others               Fail to measure FV.

**/
EFI_STATUS
EFIAPI
FirmwareVolumeInfoPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Record all measured Firmware Volume Information into a Guid Hob

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The FV Info is measured and recorded to TPM.
  @return Others               Fail to measure FV.

**/
EFI_STATUS
EFIAPI
EndofPeiSignalNotifyCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gPeiLockPhysicalPresencePpiGuid,
    PhysicalPresencePpiNotifyCallback
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiFirmwareVolumeInfoPpiGuid,
    FirmwareVolumeInfoPpiNotifyCallback
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiFirmwareVolumeInfo2PpiGuid,
    FirmwareVolumeInfoPpiNotifyCallback
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiEndOfPeiSignalPpiGuid,
    EndofPeiSignalNotifyCallBack
  }
};

/**
  Record all measured Firmware Volume Information into a Guid Hob
  Guid Hob payload layout is

     UINT32 *************************** FIRMWARE_BLOB number
     EFI_PLATFORM_FIRMWARE_BLOB******** BLOB Array

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The FV Info is measured and recorded to TPM.
  @return Others               Fail to measure FV.

**/
EFI_STATUS
EFIAPI
EndofPeiSignalNotifyCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  MEASURED_HOB_DATA  *MeasuredHobData;

  MeasuredHobData = NULL;

  PERF_CALLBACK_BEGIN (&gEfiEndOfPeiSignalPpiGuid);

  //
  // Create a Guid hob to save all measured Fv
  //
  MeasuredHobData = BuildGuidHob (
                      &gMeasuredFvHobGuid,
                      sizeof (UINTN) + sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredBaseFvIndex + mMeasuredChildFvIndex)
                      );

  if (MeasuredHobData != NULL) {
    //
    // Save measured FV info enty number
    //
    MeasuredHobData->Num = mMeasuredBaseFvIndex + mMeasuredChildFvIndex;

    //
    // Save measured base Fv info
    //
    CopyMem (MeasuredHobData->MeasuredFvBuf, mMeasuredBaseFvInfo, sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredBaseFvIndex));

    //
    // Save measured child Fv info
    //
    CopyMem (&MeasuredHobData->MeasuredFvBuf[mMeasuredBaseFvIndex], mMeasuredChildFvInfo, sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredChildFvIndex));
  }

  PERF_CALLBACK_END (&gEfiEndOfPeiSignalPpiGuid);

  return EFI_SUCCESS;
}

/**
Single function calculates SHA1 digest value for all raw data. It
combines Sha1Init(), Sha1Update() and Sha1Final().

@param[in]  Data          Raw data to be digested.
@param[in]  DataLen       Size of the raw data.
@param[out] Digest        Pointer to a buffer that stores the final digest.

@retval     EFI_SUCCESS   Always successfully calculate the final digest.
**/
EFI_STATUS
EFIAPI
TpmCommHashAll (
  IN  CONST UINT8       *Data,
  IN        UINTN       DataLen,
  OUT       TPM_DIGEST  *Digest
  )
{
  VOID   *Sha1Ctx;
  UINTN  CtxSize;

  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  ASSERT (Sha1Ctx != NULL);

  Sha1Init (Sha1Ctx);
  Sha1Update (Sha1Ctx, Data, DataLen);
  Sha1Final (Sha1Ctx, (UINT8 *)Digest);

  FreePool (Sha1Ctx);

  return EFI_SUCCESS;
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

  @param[in]      This          Indicates the calling context.
  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
HashLogExtendEvent (
  IN      EDKII_TCG_PPI      *This,
  IN      UINT64             Flags,
  IN      UINT8              *HashData,
  IN      UINTN              HashDataLen,
  IN      TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  EFI_STATUS  Status;
  VOID        *HobData;

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    return EFI_DEVICE_ERROR;
  }

  HobData = NULL;
  if (HashDataLen != 0) {
    Status = TpmCommHashAll (
               HashData,
               HashDataLen,
               &NewEventHdr->Digest
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  Status = Tpm12Extend (
             &NewEventHdr->Digest,
             NewEventHdr->PCRIndex,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  HobData = BuildGuidHob (
              &gTcgEventEntryHobGuid,
              sizeof (*NewEventHdr) + NewEventHdr->EventSize
              );
  if (HobData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyMem (HobData, NewEventHdr, sizeof (*NewEventHdr));
  HobData = (VOID *)((UINT8 *)HobData + sizeof (*NewEventHdr));
  CopyMem (HobData, NewEventData, NewEventHdr->EventSize);

Done:
  if ((Status == EFI_DEVICE_ERROR) || (Status == EFI_TIMEOUT)) {
    DEBUG ((DEBUG_ERROR, "HashLogExtendEvent - %r. Disable TPM.\n", Status));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Measure CRTM version.

  @param[in]      PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureCRTMVersion (
  IN      EFI_PEI_SERVICES  **PeiServices
  )
{
  TCG_PCR_EVENT_HDR  TcgEventHdr;

  //
  // Use FirmwareVersion string to represent CRTM version.
  // OEMs should get real CRTM version string and measure it.
  //

  TcgEventHdr.PCRIndex  = 0;
  TcgEventHdr.EventType = EV_S_CRTM_VERSION;
  TcgEventHdr.EventSize = (UINT32)StrSize ((CHAR16 *)PcdGetPtr (PcdFirmwareVersionString));

  return HashLogExtendEvent (
           &mEdkiiTcgPpi,
           0,
           (UINT8 *)PcdGetPtr (PcdFirmwareVersionString),
           TcgEventHdr.EventSize,
           &TcgEventHdr,
           (UINT8 *)PcdGetPtr (PcdFirmwareVersionString)
           );
}

/**
  Measure FV image.
  Add it into the measured FV list after the FV is measured successfully.

  @param[in]  FvBase            Base address of FV image.
  @param[in]  FvLength          Length of FV image.

  @retval EFI_SUCCESS           Fv image is measured successfully
                                or it has been already measured.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureFvImage (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  UINT32                      Index;
  EFI_STATUS                  Status;
  EFI_PLATFORM_FIRMWARE_BLOB  FvBlob;
  TCG_PCR_EVENT_HDR           TcgEventHdr;
  EFI_PHYSICAL_ADDRESS        FvOrgBase;
  EFI_PHYSICAL_ADDRESS        FvDataBase;
  EFI_PEI_HOB_POINTERS        Hob;
  EDKII_MIGRATED_FV_INFO      *MigratedFvInfo;

  //
  // Check if it is in Excluded FV list
  //
  if (mMeasurementExcludedFvPpi != NULL) {
    for (Index = 0; Index < mMeasurementExcludedFvPpi->Count; Index++) {
      if (mMeasurementExcludedFvPpi->Fv[Index].FvBase == FvBase) {
        DEBUG ((DEBUG_INFO, "The FV which is excluded by TcgPei starts at: 0x%x\n", FvBase));
        DEBUG ((DEBUG_INFO, "The FV which is excluded by TcgPei has the size: 0x%x\n", FvLength));
        return EFI_SUCCESS;
      }
    }
  }

  //
  // Check whether FV is in the measured FV list.
  //
  for (Index = 0; Index < mMeasuredBaseFvIndex; Index++) {
    if (mMeasuredBaseFvInfo[Index].BlobBase == FvBase) {
      return EFI_SUCCESS;
    }
  }

  //
  // Search the matched migration FV info
  //
  FvOrgBase  = FvBase;
  FvDataBase = FvBase;
  Hob.Raw    = GetFirstGuidHob (&gEdkiiMigratedFvInfoGuid);
  while (Hob.Raw != NULL) {
    MigratedFvInfo = GET_GUID_HOB_DATA (Hob);
    if ((MigratedFvInfo->FvNewBase == (UINT32)FvBase) && (MigratedFvInfo->FvLength == (UINT32)FvLength)) {
      //
      // Found the migrated FV info
      //
      FvOrgBase  = (EFI_PHYSICAL_ADDRESS)(UINTN)MigratedFvInfo->FvOrgBase;
      FvDataBase = (EFI_PHYSICAL_ADDRESS)(UINTN)MigratedFvInfo->FvDataBase;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gEdkiiMigratedFvInfoGuid, Hob.Raw);
  }

  //
  // Measure and record the FV to the TPM
  //
  FvBlob.BlobBase   = FvOrgBase;
  FvBlob.BlobLength = FvLength;

  DEBUG ((DEBUG_INFO, "The FV which is measured by TcgPei starts at: 0x%x\n", FvBlob.BlobBase));
  DEBUG ((DEBUG_INFO, "The FV which is measured by TcgPei has the size: 0x%x\n", FvBlob.BlobLength));

  TcgEventHdr.PCRIndex  = 0;
  TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB;
  TcgEventHdr.EventSize = sizeof (FvBlob);

  Status = HashLogExtendEvent (
             &mEdkiiTcgPpi,
             0,
             (UINT8 *)(UINTN)FvDataBase,
             (UINTN)FvBlob.BlobLength,
             &TcgEventHdr,
             (UINT8 *)&FvBlob
             );

  //
  // Add new FV into the measured FV list.
  //
  if (mMeasuredBaseFvIndex >= mMeasuredMaxBaseFvIndex) {
    mMeasuredBaseFvInfo = ReallocatePool (
                            sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * mMeasuredMaxBaseFvIndex,
                            sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredMaxBaseFvIndex + FIRMWARE_BLOB_GROWTH_STEP),
                            mMeasuredBaseFvInfo
                            );
    ASSERT (mMeasuredBaseFvInfo != NULL);
    mMeasuredMaxBaseFvIndex = mMeasuredMaxBaseFvIndex + FIRMWARE_BLOB_GROWTH_STEP;
  }

  mMeasuredBaseFvInfo[mMeasuredBaseFvIndex].BlobBase   = FvBase;
  mMeasuredBaseFvInfo[mMeasuredBaseFvIndex].BlobLength = FvLength;
  mMeasuredBaseFvIndex++;

  return Status;
}

/**
  Measure main BIOS.

  @param[in]      PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureMainBios (
  IN      EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                   Status;
  UINT32                       FvInstances;
  EFI_PEI_FV_HANDLE            VolumeHandle;
  EFI_FV_INFO                  VolumeInfo;
  EFI_PEI_FIRMWARE_VOLUME_PPI  *FvPpi;

  FvInstances = 0;
  while (TRUE) {
    //
    // Traverse all firmware volume instances of Static Core Root of Trust for Measurement
    // (S-CRTM), this firmware volume measure policy can be modified/enhanced by special
    // platform for special CRTM TPM measuring.
    //
    Status = PeiServicesFfsFindNextVolume (FvInstances, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Measure and record the firmware volume that is dispatched by PeiCore
    //
    Status = PeiServicesFfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
    ASSERT_EFI_ERROR (Status);
    //
    // Locate the corresponding FV_PPI according to founded FV's format guid
    //
    Status = PeiServicesLocatePpi (
               &VolumeInfo.FvFormat,
               0,
               NULL,
               (VOID **)&FvPpi
               );
    if (!EFI_ERROR (Status)) {
      MeasureFvImage ((EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart, VolumeInfo.FvSize);
    }

    FvInstances++;
  }

  return EFI_SUCCESS;
}

/**
  Measure and record the Firmware Volume Information once FvInfoPPI install.

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The FV Info is measured and recorded to TPM.
  @return Others               Fail to measure FV.

**/
EFI_STATUS
EFIAPI
FirmwareVolumeInfoPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI  *Fv;
  EFI_STATUS                        Status;
  EFI_PEI_FIRMWARE_VOLUME_PPI       *FvPpi;
  UINTN                             Index;

  Fv = (EFI_PEI_FIRMWARE_VOLUME_INFO_PPI *)Ppi;

  //
  // The PEI Core can not dispatch or load files from memory mapped FVs that do not support FvPpi.
  //
  Status = PeiServicesLocatePpi (
             &Fv->FvFormat,
             0,
             NULL,
             (VOID **)&FvPpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // This is an FV from an FFS file, and the parent FV must have already been measured,
  // No need to measure twice, so just record the FV and return
  //
  if ((Fv->ParentFvName != NULL) || (Fv->ParentFileName != NULL)) {
    if (mMeasuredChildFvIndex >= mMeasuredMaxChildFvIndex) {
      mMeasuredChildFvInfo = ReallocatePool (
                               sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * mMeasuredMaxChildFvIndex,
                               sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredMaxChildFvIndex + FIRMWARE_BLOB_GROWTH_STEP),
                               mMeasuredChildFvInfo
                               );
      ASSERT (mMeasuredChildFvInfo != NULL);
      mMeasuredMaxChildFvIndex = mMeasuredMaxChildFvIndex + FIRMWARE_BLOB_GROWTH_STEP;
    }

    //
    // Check whether FV is in the measured child FV list.
    //
    for (Index = 0; Index < mMeasuredChildFvIndex; Index++) {
      if (mMeasuredChildFvInfo[Index].BlobBase == (EFI_PHYSICAL_ADDRESS)(UINTN)Fv->FvInfo) {
        return EFI_SUCCESS;
      }
    }

    mMeasuredChildFvInfo[mMeasuredChildFvIndex].BlobBase   = (EFI_PHYSICAL_ADDRESS)(UINTN)Fv->FvInfo;
    mMeasuredChildFvInfo[mMeasuredChildFvIndex].BlobLength = Fv->FvInfoSize;
    mMeasuredChildFvIndex++;
    return EFI_SUCCESS;
  }

  return MeasureFvImage ((EFI_PHYSICAL_ADDRESS)(UINTN)Fv->FvInfo, Fv->FvInfoSize);
}

/**
  Set physicalPresenceLifetimeLock, physicalPresenceHWEnable and physicalPresenceCMDEnable bit by corresponding PCDs.
  And lock physical presence if needed.

  @param[in] PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param[in] NotifyDescriptor   Address of the notification descriptor data structure.
  @param[in] Ppi                Address of the PPI that was installed.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_ABORTED           physicalPresenceCMDEnable is locked.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
PhysicalPresencePpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                      Status;
  TPM_PERMANENT_FLAGS             TpmPermanentFlags;
  PEI_LOCK_PHYSICAL_PRESENCE_PPI  *LockPhysicalPresencePpi;
  TPM_PHYSICAL_PRESENCE           PhysicalPresenceValue;

  Status = Tpm12GetCapabilityFlagPermanent (&TpmPermanentFlags);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // 1. Set physicalPresenceLifetimeLock, physicalPresenceHWEnable and physicalPresenceCMDEnable bit by PCDs.
  //
  if (PcdGetBool (PcdPhysicalPresenceLifetimeLock) && !TpmPermanentFlags.physicalPresenceLifetimeLock) {
    //
    // Lock TPM LifetimeLock is required, and LifetimeLock is not locked yet.
    //
    PhysicalPresenceValue                          = TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK;
    TpmPermanentFlags.physicalPresenceLifetimeLock = TRUE;

    if (PcdGetBool (PcdPhysicalPresenceCmdEnable)) {
      PhysicalPresenceValue                      |= TPM_PHYSICAL_PRESENCE_CMD_ENABLE;
      TpmPermanentFlags.physicalPresenceCMDEnable = TRUE;
    } else {
      PhysicalPresenceValue                      |= TPM_PHYSICAL_PRESENCE_CMD_DISABLE;
      TpmPermanentFlags.physicalPresenceCMDEnable = FALSE;
    }

    if (PcdGetBool (PcdPhysicalPresenceHwEnable)) {
      PhysicalPresenceValue |= TPM_PHYSICAL_PRESENCE_HW_ENABLE;
    } else {
      PhysicalPresenceValue |= TPM_PHYSICAL_PRESENCE_HW_DISABLE;
    }

    Status = Tpm12PhysicalPresence (
               PhysicalPresenceValue
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // 2. Lock physical presence if it is required.
  //
  LockPhysicalPresencePpi = (PEI_LOCK_PHYSICAL_PRESENCE_PPI *)Ppi;
  if (!LockPhysicalPresencePpi->LockPhysicalPresence ((CONST EFI_PEI_SERVICES **)PeiServices)) {
    return EFI_SUCCESS;
  }

  if (!TpmPermanentFlags.physicalPresenceCMDEnable) {
    if (TpmPermanentFlags.physicalPresenceLifetimeLock) {
      //
      // physicalPresenceCMDEnable is locked, can't change.
      //
      return EFI_ABORTED;
    }

    //
    // Enable physical presence command
    // It is necessary in order to lock physical presence
    //
    Status = Tpm12PhysicalPresence (
               TPM_PHYSICAL_PRESENCE_CMD_ENABLE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Lock physical presence
  //
  Status = Tpm12PhysicalPresence (
             TPM_PHYSICAL_PRESENCE_LOCK
             );
  return Status;
}

/**
  Check if TPM chip is activated or not.

  @param[in]      PeiServices   Describes the list of possible PEI Services.

  @retval TRUE    TPM is activated.
  @retval FALSE   TPM is deactivated.

**/
BOOLEAN
IsTpmUsable (
  VOID
  )
{
  EFI_STATUS           Status;
  TPM_PERMANENT_FLAGS  TpmPermanentFlags;

  Status = Tpm12GetCapabilityFlagPermanent (&TpmPermanentFlags);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return (BOOLEAN)(!TpmPermanentFlags.deactivated);
}

/**
  Do measurement after memory is ready.

  @param[in]      PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
PeimEntryMP (
  IN      EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesLocatePpi (
             &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid,
             0,
             NULL,
             (VOID **)&mMeasurementExcludedFvPpi
             );
  // Do not check status, because it is optional

  Status = Tpm12RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IsTpmUsable ()) {
    if (PcdGet8 (PcdTpmScrtmPolicy) == 1) {
      Status = MeasureCRTMVersion (PeiServices);
    }

    Status = MeasureMainBios (PeiServices);
  }

  //
  // Post callbacks:
  // 1). for the FvInfoPpi services to measure and record
  // the additional Fvs to TPM
  // 2). for the OperatorPresencePpi service to determine whether to
  // lock the TPM
  //
  Status = PeiServicesNotifyPpi (&mNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // install Tcg Services
  //
  Status = PeiServicesInstallPpi (&mTcgPpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Entry point of this module.

  @param[in] FileHandle   Handle of the file being invoked.
  @param[in] PeiServices  Describes the list of possible PEI Services.

  @return Status.

**/
EFI_STATUS
EFIAPI
PeimEntryMA (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS     Status;
  EFI_STATUS     Status2;
  EFI_BOOT_MODE  BootMode;

  if (!CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)) {
    DEBUG ((DEBUG_ERROR, "No TPM12 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((DEBUG_ERROR, "TPM error!\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Initialize TPM device
  //
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // In S3 path, skip shadow logic. no measurement is required
  //
  if (BootMode != BOOT_ON_S3_RESUME) {
    Status = (**PeiServices).RegisterForShadow (FileHandle);
    if (Status == EFI_ALREADY_STARTED) {
      mImageInMemory = TRUE;
    } else if (Status == EFI_NOT_FOUND) {
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (!mImageInMemory) {
    Status = Tpm12RequestUseTpm ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "TPM not detected!\n"));
      goto Done;
    }

    if (PcdGet8 (PcdTpmInitializationPolicy) == 1) {
      if (BootMode == BOOT_ON_S3_RESUME) {
        Status = Tpm12Startup (TPM_ST_STATE);
      } else {
        Status = Tpm12Startup (TPM_ST_CLEAR);
      }

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }

    //
    // TpmSelfTest is optional on S3 path, skip it to save S3 time
    //
    if (BootMode != BOOT_ON_S3_RESUME) {
      Status = Tpm12ContinueSelfTest ();
      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }

    //
    // Only install TpmInitializedPpi on success
    //
    Status = PeiServicesInstallPpi (&mTpmInitializedPpiList);
    ASSERT_EFI_ERROR (Status);
  }

  if (mImageInMemory) {
    Status = PeimEntryMP ((EFI_PEI_SERVICES **)PeiServices);
    return Status;
  }

Done:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TPM error! Build Hob\n"));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
  }

  //
  // Always install TpmInitializationDonePpi no matter success or fail.
  // Other driver can know TPM initialization state by TpmInitializedPpi.
  //
  Status2 = PeiServicesInstallPpi (&mTpmInitializationDonePpiList);
  ASSERT_EFI_ERROR (Status2);

  return Status;
}
