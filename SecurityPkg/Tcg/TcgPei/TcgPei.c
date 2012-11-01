/** @file
  Initialize TPM device and measure FVs before handing off control to DXE.

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/LockPhysicalPresence.h>
#include <Ppi/TpmInitialized.h>
#include <Ppi/FirmwareVolume.h>
#include <Guid/TcgEventHob.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/TpmCommLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesTablePointerLib.h>

#include "TpmComm.h"

BOOLEAN                 mImageInMemory  = FALSE;

EFI_PEI_PPI_DESCRIPTOR  mTpmInitializedPpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializedPpiGuid,
  NULL
};

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
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );

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

EFI_PEI_NOTIFY_DESCRIPTOR           mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gPeiLockPhysicalPresencePpiGuid,
    PhysicalPresencePpiNotifyCallback
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiFirmwareVolumeInfoPpiGuid,
    FirmwareVolmeInfoPpiNotifyCallback 
  }
};

CHAR8 mSCrtmVersion[] = "{D20BC7C6-A1A5-415c-AE85-38290AB6BE04}";

EFI_PLATFORM_FIRMWARE_BLOB mMeasuredFvInfo[FixedPcdGet32 (PcdPeiCoreMaxFvSupported)];
UINT32 mMeasuredFvIndex = 0;

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

  @param[in]      PeiServices   Describes the list of possible PEI Services.
  @param[in]      HashData      Physical address of the start of the data buffer 
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
  @param[in]      TpmHandle     TPM handle.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in]      NewEventData  Pointer to the new event data.  

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
HashLogExtendEvent (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      UINT8                     *HashData,
  IN      UINTN                     HashDataLen,
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  EFI_STATUS                        Status;
  VOID                              *HobData;

  HobData = NULL;
  if (HashDataLen != 0) {
    Status = TpmCommHashAll (
               HashData,
               HashDataLen,
               &NewEventHdr->Digest
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = TpmCommExtend (
             PeiServices,
             TpmHandle,
             &NewEventHdr->Digest,
             NewEventHdr->PCRIndex,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  HobData = BuildGuidHob (
             &gTcgEventEntryHobGuid,
             sizeof (*NewEventHdr) + NewEventHdr->EventSize
             );
  if (HobData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (HobData, NewEventHdr, sizeof (*NewEventHdr));
  HobData = (VOID *) ((UINT8*)HobData + sizeof (*NewEventHdr));
  CopyMem (HobData, NewEventData, NewEventHdr->EventSize);
  return EFI_SUCCESS;
}

/**
  Measure CRTM version.

  @param[in]      PeiServices   Describes the list of possible PEI Services.
  @param[in]      TpmHandle     TPM handle.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureCRTMVersion (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle
  )
{
  TCG_PCR_EVENT_HDR                 TcgEventHdr;

  //
  // Here, only a static GUID is measured instead of real CRTM version.
  // OEMs should get real CRTM version string and measure it.
  //

  TcgEventHdr.PCRIndex  = 0;
  TcgEventHdr.EventType = EV_S_CRTM_VERSION;
  TcgEventHdr.EventSize = sizeof (mSCrtmVersion);
  return HashLogExtendEvent (
           PeiServices,
           (UINT8*)&mSCrtmVersion,
           TcgEventHdr.EventSize,
           TpmHandle,
           &TcgEventHdr,
           (UINT8*)&mSCrtmVersion
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
  IN EFI_PHYSICAL_ADDRESS           FvBase,
  IN UINT64                         FvLength
  )
{
  UINT32                            Index;
  EFI_STATUS                        Status;
  EFI_PLATFORM_FIRMWARE_BLOB        FvBlob;
  TCG_PCR_EVENT_HDR                 TcgEventHdr;
  TIS_TPM_HANDLE                    TpmHandle;

  TpmHandle = (TIS_TPM_HANDLE) (UINTN) TPM_BASE_ADDRESS;

  //
  // Check whether FV is in the measured FV list.
  //
  for (Index = 0; Index < mMeasuredFvIndex; Index ++) {
    if (mMeasuredFvInfo[Index].BlobBase == FvBase) {
      return EFI_SUCCESS;
    }
  }
  
  //
  // Measure and record the FV to the TPM
  //
  FvBlob.BlobBase   = FvBase;
  FvBlob.BlobLength = FvLength;

  DEBUG ((DEBUG_INFO, "The FV which is measured by TcgPei starts at: 0x%x\n", FvBlob.BlobBase));
  DEBUG ((DEBUG_INFO, "The FV which is measured by TcgPei has the size: 0x%x\n", FvBlob.BlobLength));

  TcgEventHdr.PCRIndex = 0;
  TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB;
  TcgEventHdr.EventSize = sizeof (FvBlob);

  Status = HashLogExtendEvent (
             (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
             (UINT8*) (UINTN) FvBlob.BlobBase,
             (UINTN) FvBlob.BlobLength,
             TpmHandle,
             &TcgEventHdr,
             (UINT8*) &FvBlob
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Add new FV into the measured FV list.
  //
  ASSERT (mMeasuredFvIndex < FixedPcdGet32 (PcdPeiCoreMaxFvSupported));
  if (mMeasuredFvIndex < FixedPcdGet32 (PcdPeiCoreMaxFvSupported)) {
    mMeasuredFvInfo[mMeasuredFvIndex].BlobBase   = FvBase;
    mMeasuredFvInfo[mMeasuredFvIndex++].BlobLength = FvLength;
  }

  return Status;
}

/**
  Measure main BIOS.

  @param[in]      PeiServices   Describes the list of possible PEI Services.
  @param[in]      TpmHandle     TPM handle.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureMainBios (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle
  )
{
  EFI_STATUS                        Status;
  UINT32                            FvInstances;
  EFI_PEI_FV_HANDLE                 VolumeHandle;
  EFI_FV_INFO                       VolumeInfo;
  EFI_PEI_FIRMWARE_VOLUME_PPI       *FvPpi;
  
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
  // No need to measure twice, so just returns
  //
  if (Fv->ParentFvName != NULL || Fv->ParentFileName != NULL ) {
    return EFI_SUCCESS;
  }

  return MeasureFvImage ((EFI_PHYSICAL_ADDRESS) (UINTN) Fv->FvInfo, Fv->FvInfoSize);
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
  IN EFI_PEI_SERVICES               **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR      *NotifyDescriptor,
  IN VOID                           *Ppi
  )
{
  EFI_STATUS                        Status;
  PEI_LOCK_PHYSICAL_PRESENCE_PPI    *LockPhysicalPresencePpi;
  BOOLEAN                           LifetimeLock;
  BOOLEAN                           CmdEnable;
  TIS_TPM_HANDLE                    TpmHandle;
  TPM_PHYSICAL_PRESENCE             PhysicalPresenceValue;

  TpmHandle        = (TIS_TPM_HANDLE) (UINTN) TPM_BASE_ADDRESS;

  Status = TpmCommGetCapability (PeiServices, TpmHandle, NULL, &LifetimeLock, &CmdEnable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // 1. Set physicalPresenceLifetimeLock, physicalPresenceHWEnable and physicalPresenceCMDEnable bit by PCDs.
  //
  if (PcdGetBool (PcdPhysicalPresenceLifetimeLock) && !LifetimeLock) {
    //
    // Lock TPM LifetimeLock is required, and LifetimeLock is not locked yet. 
    //
    PhysicalPresenceValue = TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK;

    if (PcdGetBool (PcdPhysicalPresenceCmdEnable)) {
      PhysicalPresenceValue |= TPM_PHYSICAL_PRESENCE_CMD_ENABLE;
      CmdEnable = TRUE;
    } else {
      PhysicalPresenceValue |= TPM_PHYSICAL_PRESENCE_CMD_DISABLE;
      CmdEnable = FALSE;
    }

    if (PcdGetBool (PcdPhysicalPresenceHwEnable)) {
      PhysicalPresenceValue |= TPM_PHYSICAL_PRESENCE_HW_ENABLE;
    } else {
      PhysicalPresenceValue |= TPM_PHYSICAL_PRESENCE_HW_DISABLE;
    }      
     
    Status = TpmCommPhysicalPresence (
               PeiServices,
               TpmHandle,
               PhysicalPresenceValue
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  
  //
  // 2. Lock physical presence if it is required.
  //
  LockPhysicalPresencePpi = (PEI_LOCK_PHYSICAL_PRESENCE_PPI *) Ppi;
  if (!LockPhysicalPresencePpi->LockPhysicalPresence ((CONST EFI_PEI_SERVICES**) PeiServices)) {
    return EFI_SUCCESS;
  }

  if (!CmdEnable) {
    if (LifetimeLock) {
      //
      // physicalPresenceCMDEnable is locked, can't change.
      //
      return EFI_ABORTED;
    }

    //
    // Enable physical presence command
    // It is necessary in order to lock physical presence
    //
    Status = TpmCommPhysicalPresence (
               PeiServices,
               TpmHandle,
               TPM_PHYSICAL_PRESENCE_CMD_ENABLE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Lock physical presence
  // 
  Status = TpmCommPhysicalPresence (
              PeiServices,
              TpmHandle,
              TPM_PHYSICAL_PRESENCE_LOCK
              );
  return Status;
}

/**
  Check if TPM chip is activeated or not.

  @param[in]      PeiServices   Describes the list of possible PEI Services.
  @param[in]      TpmHandle     TPM handle.

  @retval TRUE    TPM is activated.
  @retval FALSE   TPM is deactivated.

**/
BOOLEAN
EFIAPI
IsTpmUsable (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle
  )
{
  EFI_STATUS                        Status;
  BOOLEAN                           Deactivated;

  Status = TpmCommGetCapability (PeiServices, TpmHandle, &Deactivated, NULL, NULL);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  return (BOOLEAN)(!Deactivated); 
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
  IN      EFI_PEI_SERVICES          **PeiServices
  )
{
  EFI_STATUS                        Status;
  TIS_TPM_HANDLE                    TpmHandle;

  TpmHandle = (TIS_TPM_HANDLE)(UINTN)TPM_BASE_ADDRESS;
  Status = TisPcRequestUseTpm ((TIS_PC_REGISTERS_PTR)TpmHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IsTpmUsable (PeiServices, TpmHandle)) {
    Status = MeasureCRTMVersion (PeiServices, TpmHandle);
    ASSERT_EFI_ERROR (Status);

    Status = MeasureMainBios (PeiServices, TpmHandle);
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
  EFI_BOOT_MODE                     BootMode;
  TIS_TPM_HANDLE                    TpmHandle;

  if (PcdGetBool (PcdHideTpmSupport) && PcdGetBool (PcdHideTpm)) {
    return EFI_UNSUPPORTED;
  }

  Status = (**PeiServices).RegisterForShadow(FileHandle);
  if (Status == EFI_ALREADY_STARTED) {
    mImageInMemory = TRUE;
  } else if (Status == EFI_NOT_FOUND) {
    ASSERT_EFI_ERROR (Status);
  }

  if (!mImageInMemory) {
    //
    // Initialize TPM device
    //
    Status = PeiServicesGetBootMode (&BootMode);
    ASSERT_EFI_ERROR (Status);

    TpmHandle = (TIS_TPM_HANDLE)(UINTN)TPM_BASE_ADDRESS;
    Status = TisPcRequestUseTpm ((TIS_PC_REGISTERS_PTR)TpmHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "TPM not detected!\n"));
      return Status;
    }

    Status = TpmCommStartup ((EFI_PEI_SERVICES**)PeiServices, TpmHandle, BootMode);
    if (EFI_ERROR (Status) ) {
      return Status;
    }

    //
    // TpmSelfTest is optional on S3 path, skip it to save S3 time
    //
    if (BootMode != BOOT_ON_S3_RESUME) {
      Status = TpmCommContinueSelfTest ((EFI_PEI_SERVICES**)PeiServices, TpmHandle);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    Status = PeiServicesInstallPpi (&mTpmInitializedPpiList);
    ASSERT_EFI_ERROR (Status);
  }

  if (mImageInMemory) {
    Status = PeimEntryMP ((EFI_PEI_SERVICES**)PeiServices);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}
