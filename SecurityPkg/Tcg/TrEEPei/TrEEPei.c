/** @file
  Initialize TPM2 device and measure FVs before handing off control to DXE.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/FirmwareVolumeInfo2.h>
#include <Ppi/LockPhysicalPresence.h>
#include <Ppi/TpmInitialized.h>
#include <Ppi/FirmwareVolume.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>

#include <Guid/TcgEventHob.h>
#include <Guid/MeasuredFvHob.h>
#include <Guid/TpmInstance.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/HashLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Protocol/TrEEProtocol.h>
#include <Library/PerformanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>

#define PERF_ID_TREE_PEI  0x3080

typedef struct {
  EFI_GUID               *EventGuid;
  TREE_EVENT_LOG_FORMAT  LogFormat;
} TREE_EVENT_INFO_STRUCT;

TREE_EVENT_INFO_STRUCT mTreeEventInfo[] = {
  {&gTcgEventEntryHobGuid,             TREE_EVENT_LOG_FORMAT_TCG_1_2},
};

BOOLEAN                 mImageInMemory  = FALSE;
EFI_PEI_FILE_HANDLE     mFileHandle;

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

EFI_PLATFORM_FIRMWARE_BLOB *mMeasuredBaseFvInfo;
UINT32 mMeasuredBaseFvIndex = 0;

EFI_PLATFORM_FIRMWARE_BLOB *mMeasuredChildFvInfo;
UINT32 mMeasuredChildFvIndex = 0;

/**
  Measure and record the Firmware Volum Information once FvInfoPPI install.

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The FV Info is measured and recorded to TPM.
  @return Others               Fail to measure FV.

**/
EFI_STATUS
EFIAPI
FirmwareVolmeInfoPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );

/**
  Record all measured Firmware Volum Information into a Guid Hob

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The FV Info is measured and recorded to TPM.
  @return Others               Fail to measure FV.

**/
EFI_STATUS
EFIAPI
EndofPeiSignalNotifyCallBack (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR           mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiFirmwareVolumeInfoPpiGuid,
    FirmwareVolmeInfoPpiNotifyCallback 
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiFirmwareVolumeInfo2PpiGuid,
    FirmwareVolmeInfoPpiNotifyCallback 
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiEndOfPeiSignalPpiGuid,
    EndofPeiSignalNotifyCallBack
  }
};

EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI *mMeasurementExcludedFvPpi;

/**
  This function get digest from digest list.

  @param HashAlg    digest algorithm
  @param DigestList digest list
  @param Digest     digest

  @retval EFI_SUCCESS   Sha1Digest is found and returned.
  @retval EFI_NOT_FOUND Sha1Digest is not found.
**/
EFI_STATUS
Tpm2GetDigestFromDigestList (
  IN TPMI_ALG_HASH      HashAlg,
  IN TPML_DIGEST_VALUES *DigestList,
  IN VOID               *Digest
  )
{
  UINTN  Index;
  UINT16 DigestSize;

  DigestSize = GetHashSizeFromAlgo (HashAlg);
  for (Index = 0; Index < DigestList->count; Index++) {
    if (DigestList->digests[Index].hashAlg == HashAlg) {
      CopyMem (
        Digest,
        &DigestList->digests[Index].digest,
        DigestSize
        );
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Record all measured Firmware Volum Information into a Guid Hob
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
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{  
  MEASURED_HOB_DATA *MeasuredHobData;

  MeasuredHobData = NULL;

  //
  // Create a Guid hob to save all measured Fv 
  //
  MeasuredHobData = BuildGuidHob(
                      &gMeasuredFvHobGuid,
                      sizeof(UINTN) + sizeof(EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredBaseFvIndex + mMeasuredChildFvIndex)
                      );

  if (MeasuredHobData != NULL){
    //
    // Save measured FV info enty number
    //
    MeasuredHobData->Num = mMeasuredBaseFvIndex + mMeasuredChildFvIndex;

    //
    // Save measured base Fv info
    //
    CopyMem (MeasuredHobData->MeasuredFvBuf, mMeasuredBaseFvInfo, sizeof(EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredBaseFvIndex));

    //
    // Save measured child Fv info
    //
    CopyMem (&MeasuredHobData->MeasuredFvBuf[mMeasuredBaseFvIndex] , mMeasuredChildFvInfo, sizeof(EFI_PLATFORM_FIRMWARE_BLOB) * (mMeasuredChildFvIndex));
  }

  return EFI_SUCCESS;
}

/**
  Add a new entry to the Event Log.

  @param[in]     DigestList    A list of digest.
  @param[in,out] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]     NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
**/
EFI_STATUS
LogHashEvent (
  IN TPML_DIGEST_VALUES             *DigestList,
  IN OUT  TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  VOID                              *HobData;
  EFI_STATUS                        Status;
  UINTN                             Index;
  EFI_STATUS                        RetStatus;

  RetStatus = EFI_SUCCESS;
  for (Index = 0; Index < sizeof(mTreeEventInfo)/sizeof(mTreeEventInfo[0]); Index++) {
      DEBUG ((EFI_D_INFO, "  LogFormat - 0x%08x\n", mTreeEventInfo[Index].LogFormat));
      switch (mTreeEventInfo[Index].LogFormat) {
      case TREE_EVENT_LOG_FORMAT_TCG_1_2:
        Status = Tpm2GetDigestFromDigestList (TPM_ALG_SHA1, DigestList, &NewEventHdr->Digest);
        if (!EFI_ERROR (Status)) {
          HobData = BuildGuidHob (
                     &gTcgEventEntryHobGuid,
                     sizeof (*NewEventHdr) + NewEventHdr->EventSize
                     );
          if (HobData == NULL) {
            RetStatus = EFI_OUT_OF_RESOURCES;
            break;
          }

          CopyMem (HobData, NewEventHdr, sizeof (*NewEventHdr));
          HobData = (VOID *) ((UINT8*)HobData + sizeof (*NewEventHdr));
          CopyMem (HobData, NewEventData, NewEventHdr->EventSize);
        }
        break;
      }
  }

  return RetStatus;
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

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
HashLogExtendEvent (
  IN      UINT64                    Flags,
  IN      UINT8                     *HashData,
  IN      UINTN                     HashDataLen,
  IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  EFI_STATUS                        Status;
  TPML_DIGEST_VALUES                DigestList;

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    return EFI_DEVICE_ERROR;
  }

  Status = HashAndExtend (
             NewEventHdr->PCRIndex,
             HashData,
             HashDataLen,
             &DigestList
             );
  if (!EFI_ERROR (Status)) {
    if ((Flags & TREE_EXTEND_ONLY) == 0) {
      Status = LogHashEvent (&DigestList, NewEventHdr, NewEventData);
    }
  }
  
  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((EFI_D_ERROR, "HashLogExtendEvent - %r. Disable TPM.\n", Status));
    BuildGuidHob (&gTpmErrorHobGuid,0);
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
  }

  return Status;
}

/**
  Measure CRTM version.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
MeasureCRTMVersion (
  VOID
  )
{
  TCG_PCR_EVENT_HDR                 TcgEventHdr;

  //
  // Use FirmwareVersion string to represent CRTM version.
  // OEMs should get real CRTM version string and measure it.
  //

  TcgEventHdr.PCRIndex  = 0;
  TcgEventHdr.EventType = EV_S_CRTM_VERSION;
  TcgEventHdr.EventSize = (UINT32) StrSize((CHAR16*)PcdGetPtr (PcdFirmwareVersionString));

  return HashLogExtendEvent (
           0,
           (UINT8*)PcdGetPtr (PcdFirmwareVersionString),
           TcgEventHdr.EventSize,
           &TcgEventHdr,
           (UINT8*)PcdGetPtr (PcdFirmwareVersionString)
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
MeasureFvImage (
  IN EFI_PHYSICAL_ADDRESS           FvBase,
  IN UINT64                         FvLength
  )
{
  UINT32                            Index;
  EFI_STATUS                        Status;
  EFI_PLATFORM_FIRMWARE_BLOB        FvBlob;
  TCG_PCR_EVENT_HDR                 TcgEventHdr;

  //
  // Check if it is in Excluded FV list
  //
  if (mMeasurementExcludedFvPpi != NULL) {
    for (Index = 0; Index < mMeasurementExcludedFvPpi->Count; Index ++) {
      if (mMeasurementExcludedFvPpi->Fv[Index].FvBase == FvBase) {
        DEBUG ((DEBUG_INFO, "The FV which is excluded by TrEEPei starts at: 0x%x\n", FvBase));
        DEBUG ((DEBUG_INFO, "The FV which is excluded by TrEEPei has the size: 0x%x\n", FvLength));
        return EFI_SUCCESS;
      }
    }
  }

  //
  // Check whether FV is in the measured FV list.
  //
  for (Index = 0; Index < mMeasuredBaseFvIndex; Index ++) {
    if (mMeasuredBaseFvInfo[Index].BlobBase == FvBase) {
      return EFI_SUCCESS;
    }
  }
  
  //
  // Measure and record the FV to the TPM
  //
  FvBlob.BlobBase   = FvBase;
  FvBlob.BlobLength = FvLength;

  DEBUG ((DEBUG_INFO, "The FV which is measured by TrEEPei starts at: 0x%x\n", FvBlob.BlobBase));
  DEBUG ((DEBUG_INFO, "The FV which is measured by TrEEPei has the size: 0x%x\n", FvBlob.BlobLength));

  TcgEventHdr.PCRIndex = 0;
  TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB;
  TcgEventHdr.EventSize = sizeof (FvBlob);

  Status = HashLogExtendEvent (
             0,
             (UINT8*) (UINTN) FvBlob.BlobBase,
             (UINTN) FvBlob.BlobLength,
             &TcgEventHdr,
             (UINT8*) &FvBlob
             );

  //
  // Add new FV into the measured FV list.
  //
  ASSERT (mMeasuredBaseFvIndex < FixedPcdGet32 (PcdPeiCoreMaxFvSupported));
  if (mMeasuredBaseFvIndex < FixedPcdGet32 (PcdPeiCoreMaxFvSupported)) {
    mMeasuredBaseFvInfo[mMeasuredBaseFvIndex].BlobBase   = FvBase;
    mMeasuredBaseFvInfo[mMeasuredBaseFvIndex].BlobLength = FvLength;
    mMeasuredBaseFvIndex++;
  }

  return Status;
}

/**
  Measure main BIOS.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
MeasureMainBios (
  VOID
  )
{
  EFI_STATUS                        Status;
  UINT32                            FvInstances;
  EFI_PEI_FV_HANDLE                 VolumeHandle;
  EFI_FV_INFO                       VolumeInfo;
  EFI_PEI_FIRMWARE_VOLUME_PPI       *FvPpi;

  PERF_START_EX (mFileHandle, "EventRec", "TrEEPei", 0, PERF_ID_TREE_PEI);
  FvInstances    = 0;
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
               (VOID**)&FvPpi
               );
    if (!EFI_ERROR (Status)) {
      MeasureFvImage ((EFI_PHYSICAL_ADDRESS) (UINTN) VolumeInfo.FvStart, VolumeInfo.FvSize);
    }

    FvInstances++;
  }
  PERF_END_EX (mFileHandle, "EventRec", "TrEEPei", 0, PERF_ID_TREE_PEI + 1);

  return EFI_SUCCESS;
}

/**
  Measure and record the Firmware Volum Information once FvInfoPPI install.

  @param[in] PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] NotifyDescriptor  Address of the notification descriptor data structure.
  @param[in] Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS          The FV Info is measured and recorded to TPM.
  @return Others               Fail to measure FV.

**/
EFI_STATUS
EFIAPI
FirmwareVolmeInfoPpiNotifyCallback (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR      *NotifyDescriptor,
  IN VOID                           *Ppi
  )
{
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI  *Fv;
  EFI_STATUS                        Status;
  EFI_PEI_FIRMWARE_VOLUME_PPI       *FvPpi;
  UINTN                             Index;

  Fv = (EFI_PEI_FIRMWARE_VOLUME_INFO_PPI *) Ppi;

  //
  // The PEI Core can not dispatch or load files from memory mapped FVs that do not support FvPpi.
  //
  Status = PeiServicesLocatePpi (
             &Fv->FvFormat, 
             0, 
             NULL,
             (VOID**)&FvPpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }
  
  //
  // This is an FV from an FFS file, and the parent FV must have already been measured,
  // No need to measure twice, so just record the FV and return
  //
  if (Fv->ParentFvName != NULL || Fv->ParentFileName != NULL ) {
    
    ASSERT (mMeasuredChildFvIndex < FixedPcdGet32 (PcdPeiCoreMaxFvSupported));
    if (mMeasuredChildFvIndex < FixedPcdGet32 (PcdPeiCoreMaxFvSupported)) {
      //
      // Check whether FV is in the measured child FV list.
      //
      for (Index = 0; Index < mMeasuredChildFvIndex; Index++) {
        if (mMeasuredChildFvInfo[Index].BlobBase == (EFI_PHYSICAL_ADDRESS) (UINTN) Fv->FvInfo) {
          return EFI_SUCCESS;
        }
      }
      mMeasuredChildFvInfo[mMeasuredChildFvIndex].BlobBase   = (EFI_PHYSICAL_ADDRESS) (UINTN) Fv->FvInfo;
      mMeasuredChildFvInfo[mMeasuredChildFvIndex].BlobLength = Fv->FvInfoSize;
      mMeasuredChildFvIndex++;
    }
    return EFI_SUCCESS;
  }

  return MeasureFvImage ((EFI_PHYSICAL_ADDRESS) (UINTN) Fv->FvInfo, Fv->FvInfoSize);
}

/**
  Do measurement after memory is ready.

  @param[in]      PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
PeimEntryMP (
  IN      EFI_PEI_SERVICES          **PeiServices
  )
{
  EFI_STATUS                        Status;

  Status = PeiServicesLocatePpi (
               &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid, 
               0, 
               NULL,
               (VOID**)&mMeasurementExcludedFvPpi
               );
  // Do not check status, because it is optional

  mMeasuredBaseFvInfo  = (EFI_PLATFORM_FIRMWARE_BLOB *) AllocateZeroPool (sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * PcdGet32 (PcdPeiCoreMaxFvSupported));
  ASSERT (mMeasuredBaseFvInfo != NULL);
  mMeasuredChildFvInfo = (EFI_PLATFORM_FIRMWARE_BLOB *) AllocateZeroPool (sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * PcdGet32 (PcdPeiCoreMaxFvSupported));
  ASSERT (mMeasuredChildFvInfo != NULL);
  
  if (PcdGet8 (PcdTpm2ScrtmPolicy) == 1) {
    Status = MeasureCRTMVersion ();
  }

  Status = MeasureMainBios ();

  //
  // Post callbacks:
  // for the FvInfoPpi services to measure and record
  // the additional Fvs to TPM
  //
  Status = PeiServicesNotifyPpi (&mNotifyList[0]);
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
  IN       EFI_PEI_FILE_HANDLE      FileHandle,
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS                        Status;
  EFI_STATUS                        Status2;
  EFI_BOOT_MODE                     BootMode;

  if (CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid) ||
      CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)){
    DEBUG ((EFI_D_ERROR, "No TPM2 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((EFI_D_ERROR, "TPM2 error!\n"));
    return EFI_DEVICE_ERROR;
  }

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // In S3 path, skip shadow logic. no measurement is required
  //
  if (BootMode != BOOT_ON_S3_RESUME) {
    Status = (**PeiServices).RegisterForShadow(FileHandle);
    if (Status == EFI_ALREADY_STARTED) {
      mImageInMemory = TRUE;
      mFileHandle = FileHandle;
    } else if (Status == EFI_NOT_FOUND) {
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (!mImageInMemory) {
    //
    // Initialize TPM device
    //
    Status = Tpm2RequestUseTpm ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "TPM2 not detected!\n"));
      goto Done;
    }

    if (PcdGet8 (PcdTpm2InitializationPolicy) == 1) {
      if (BootMode == BOOT_ON_S3_RESUME) {
        Status = Tpm2Startup (TPM_SU_STATE);
        if (EFI_ERROR (Status) ) {
          Status = Tpm2Startup (TPM_SU_CLEAR);
        }
      } else {
        Status = Tpm2Startup (TPM_SU_CLEAR);
      }
      if (EFI_ERROR (Status) ) {
        goto Done;
      }
    }

    //
    // TpmSelfTest is optional on S3 path, skip it to save S3 time
    //
    if (BootMode != BOOT_ON_S3_RESUME) {
      if (PcdGet8 (PcdTpm2SelfTestPolicy) == 1) {
        Status = Tpm2SelfTest (NO);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
      }
    }

    //
    // Only intall TpmInitializedPpi on success
    //
    Status = PeiServicesInstallPpi (&mTpmInitializedPpiList);
    ASSERT_EFI_ERROR (Status);
  }

  if (mImageInMemory) {
    Status = PeimEntryMP ((EFI_PEI_SERVICES**)PeiServices);
    return Status;
  }

Done:
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TPM2 error! Build Hob\n"));
    BuildGuidHob (&gTpmErrorHobGuid,0);
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
      );
  }
  //
  // Always intall TpmInitializationDonePpi no matter success or fail.
  // Other driver can know TPM initialization state by TpmInitializedPpi.
  //
  Status2 = PeiServicesInstallPpi (&mTpmInitializationDonePpiList);
  ASSERT_EFI_ERROR (Status2);

  return Status;
}
