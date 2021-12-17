/** @file
  Initialize TPM2 device and measure FVs before handing off control to DXE.

Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, Microsoft Corporation.  All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/FirmwareVolumeInfo2.h>
#include <Ppi/TpmInitialized.h>
#include <Ppi/FirmwareVolume.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>
#include <Ppi/FirmwareVolumeInfoPrehashedFV.h>
#include <Ppi/Tcg.h>

#include <Guid/TcgEventHob.h>
#include <Guid/MeasuredFvHob.h>
#include <Guid/TpmInstance.h>
#include <Guid/MigratedFvInfo.h>

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
#include <Protocol/Tcg2Protocol.h>
#include <Library/PerformanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/PrintLib.h>

#define PERF_ID_TCG2_PEI  0x3080

typedef struct {
  EFI_GUID                     *EventGuid;
  EFI_TCG2_EVENT_LOG_FORMAT    LogFormat;
} TCG2_EVENT_INFO_STRUCT;

TCG2_EVENT_INFO_STRUCT  mTcg2EventInfo[] = {
  { &gTcgEventEntryHobGuid,  EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 },
  { &gTcgEvent2EntryHobGuid, EFI_TCG2_EVENT_LOG_FORMAT_TCG_2   },
};

BOOLEAN              mImageInMemory = FALSE;
EFI_PEI_FILE_HANDLE  mFileHandle;

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
  @param[in]      HashData      If BIT0 of Flags is 0, it is physical address of the
                                start of the data buffer to be hashed, extended, and logged.
                                If BIT0 of Flags is 1, it is physical address of the
                                start of the pre-hash data buffter to be extended, and logged.
                                The pre-hash data format is TPML_DIGEST_VALUES.
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

#pragma pack (1)

#define FV_HANDOFF_TABLE_DESC  "Fv(XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)"
typedef struct {
  UINT8                   BlobDescriptionSize;
  UINT8                   BlobDescription[sizeof (FV_HANDOFF_TABLE_DESC)];
  EFI_PHYSICAL_ADDRESS    BlobBase;
  UINT64                  BlobLength;
} FV_HANDOFF_TABLE_POINTERS2;

#pragma pack ()

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
  Make sure that the current PCR allocations, the TPM supported PCRs,
  PcdTcg2HashAlgorithmBitmap and the PcdTpm2HashMask are all in agreement.
**/
VOID
SyncPcrAllocationsAndPcrMask (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP  TpmHashAlgorithmBitmap;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP  BiosHashAlgorithmBitmap;
  UINT32                           TpmActivePcrBanks;
  UINT32                           NewTpmActivePcrBanks;
  UINT32                           Tpm2PcrMask;
  UINT32                           NewTpm2PcrMask;

  DEBUG ((DEBUG_ERROR, "SyncPcrAllocationsAndPcrMask!\n"));

  //
  // Determine the current TPM support and the Platform PCR mask.
  //
  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &TpmActivePcrBanks);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "Tpm2GetCapabilitySupportedAndActivePcrs - TpmHashAlgorithmBitmap: 0x%08x\n", TpmHashAlgorithmBitmap));
  DEBUG ((DEBUG_INFO, "Tpm2GetCapabilitySupportedAndActivePcrs - TpmActivePcrBanks 0x%08x\n", TpmActivePcrBanks));

  Tpm2PcrMask = PcdGet32 (PcdTpm2HashMask);
  if (Tpm2PcrMask == 0) {
    //
    // If PcdTpm2HashMask is zero, use ActivePcr setting.
    // Only when PcdTpm2HashMask is initialized to 0, will it be updated to current Active Pcrs.
    //
    PcdSet32S (PcdTpm2HashMask, TpmActivePcrBanks);
    Tpm2PcrMask = TpmActivePcrBanks;
  }

  DEBUG ((DEBUG_INFO, "Tpm2PcrMask 0x%08x\n", Tpm2PcrMask));

  //
  // The Active PCRs in the TPM need to be a strict subset of the hashing algorithms supported by BIOS.
  //
  // * Find the intersection of Pcd support and TPM active PCRs. If banks are missing from the TPM support
  // that are in the PCD, update the PCD.
  // * Find intersection of TPM Active PCRs and BIOS supported algorithms. If there are active PCR banks
  // that are not supported by the platform, update the TPM allocations and reboot.
  // Note: When the HashLibBaseCryptoRouter solution is used, the hash algorithm support from BIOS is reported
  //       by Tcg2HashAlgorithmBitmap, which is populated by HashLib instances at runtime.
  BiosHashAlgorithmBitmap = PcdGet32 (PcdTcg2HashAlgorithmBitmap);
  DEBUG ((DEBUG_INFO, "Tcg2HashAlgorithmBitmap: 0x%08x\n", BiosHashAlgorithmBitmap));

  if (((TpmActivePcrBanks & Tpm2PcrMask) != TpmActivePcrBanks) ||
      ((TpmActivePcrBanks & BiosHashAlgorithmBitmap) != TpmActivePcrBanks))
  {
    DEBUG ((DEBUG_INFO, "TpmActivePcrBanks & Tpm2PcrMask = 0x%08x\n", (TpmActivePcrBanks & Tpm2PcrMask)));
    DEBUG ((DEBUG_INFO, "TpmActivePcrBanks & BiosHashAlgorithmBitmap = 0x%08x\n", (TpmActivePcrBanks & BiosHashAlgorithmBitmap)));
    NewTpmActivePcrBanks  = TpmActivePcrBanks;
    NewTpmActivePcrBanks &= Tpm2PcrMask;
    NewTpmActivePcrBanks &= BiosHashAlgorithmBitmap;
    DEBUG ((DEBUG_INFO, "NewTpmActivePcrBanks 0x%08x\n", NewTpmActivePcrBanks));

    DEBUG ((DEBUG_INFO, "%a - Reallocating PCR banks from 0x%X to 0x%X.\n", __FUNCTION__, TpmActivePcrBanks, NewTpmActivePcrBanks));

    if (NewTpmActivePcrBanks == 0) {
      DEBUG ((DEBUG_ERROR, "%a - No viable PCRs active! Please set a less restrictive value for PcdTpm2HashMask!\n", __FUNCTION__));
      ASSERT (FALSE);
    } else {
      DEBUG ((DEBUG_ERROR, "Tpm2PcrAllocateBanks (TpmHashAlgorithmBitmap: 0x%08x, NewTpmActivePcrBanks: 0x%08x)\n", TpmHashAlgorithmBitmap, NewTpmActivePcrBanks));
      Status = Tpm2PcrAllocateBanks (NULL, (UINT32)TpmHashAlgorithmBitmap, NewTpmActivePcrBanks);
      if (EFI_ERROR (Status)) {
        //
        // We can't do much here, but we hope that this doesn't happen.
        //
        DEBUG ((DEBUG_ERROR, "%a - Failed to reallocate PCRs!\n", __FUNCTION__));
        ASSERT_EFI_ERROR (Status);
      }

      //
      // Need reset system, since we just called Tpm2PcrAllocateBanks().
      //
      ResetCold ();
    }
  }

  //
  // If there are any PCRs that claim support in the Platform mask that are
  // not supported by the TPM, update the mask.
  //
  if ((Tpm2PcrMask & TpmHashAlgorithmBitmap) != Tpm2PcrMask) {
    NewTpm2PcrMask = Tpm2PcrMask & TpmHashAlgorithmBitmap;

    DEBUG ((DEBUG_INFO, "%a - Updating PcdTpm2HashMask from 0x%X to 0x%X.\n", __FUNCTION__, Tpm2PcrMask, NewTpm2PcrMask));
    if (NewTpm2PcrMask == 0) {
      DEBUG ((DEBUG_ERROR, "%a - No viable PCRs supported! Please set a less restrictive value for PcdTpm2HashMask!\n", __FUNCTION__));
      ASSERT (FALSE);
    }

    Status = PcdSet32S (PcdTpm2HashMask, NewTpm2PcrMask);
    DEBUG ((DEBUG_ERROR, "Set PcdTpm2Hash Mask to 0x%08x\n", NewTpm2PcrMask));
    ASSERT_EFI_ERROR (Status);
  }
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
  IN TPML_DIGEST_VALUES      *DigestList,
  IN OUT  TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  VOID            *HobData;
  EFI_STATUS      Status;
  UINTN           Index;
  EFI_STATUS      RetStatus;
  UINT32          SupportedEventLogs;
  TCG_PCR_EVENT2  *TcgPcrEvent2;
  UINT8           *DigestBuffer;

  SupportedEventLogs = EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 | EFI_TCG2_EVENT_LOG_FORMAT_TCG_2;

  RetStatus = EFI_SUCCESS;
  for (Index = 0; Index < sizeof (mTcg2EventInfo)/sizeof (mTcg2EventInfo[0]); Index++) {
    if ((SupportedEventLogs & mTcg2EventInfo[Index].LogFormat) != 0) {
      DEBUG ((DEBUG_INFO, "  LogFormat - 0x%08x\n", mTcg2EventInfo[Index].LogFormat));
      switch (mTcg2EventInfo[Index].LogFormat) {
        case EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2:
          Status = GetDigestFromDigestList (TPM_ALG_SHA1, DigestList, &NewEventHdr->Digest);
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
            HobData = (VOID *)((UINT8 *)HobData + sizeof (*NewEventHdr));
            CopyMem (HobData, NewEventData, NewEventHdr->EventSize);
          }

          break;
        case EFI_TCG2_EVENT_LOG_FORMAT_TCG_2:
          //
          // Use GetDigestListSize (DigestList) in the GUID HOB DataLength calculation
          // to reserve enough buffer to hold TPML_DIGEST_VALUES compact binary.
          //
          HobData = BuildGuidHob (
                      &gTcgEvent2EntryHobGuid,
                      sizeof (TcgPcrEvent2->PCRIndex) + sizeof (TcgPcrEvent2->EventType) + GetDigestListSize (DigestList) + sizeof (TcgPcrEvent2->EventSize) + NewEventHdr->EventSize
                      );
          if (HobData == NULL) {
            RetStatus = EFI_OUT_OF_RESOURCES;
            break;
          }

          TcgPcrEvent2            = HobData;
          TcgPcrEvent2->PCRIndex  = NewEventHdr->PCRIndex;
          TcgPcrEvent2->EventType = NewEventHdr->EventType;
          DigestBuffer            = (UINT8 *)&TcgPcrEvent2->Digest;
          DigestBuffer            = CopyDigestListToBuffer (DigestBuffer, DigestList, PcdGet32 (PcdTpm2HashMask));
          CopyMem (DigestBuffer, &NewEventHdr->EventSize, sizeof (TcgPcrEvent2->EventSize));
          DigestBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);
          CopyMem (DigestBuffer, NewEventData, NewEventHdr->EventSize);
          break;
      }
    }
  }

  return RetStatus;
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

  @param[in]      This          Indicates the calling context
  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      If BIT0 of Flags is 0, it is physical address of the
                                start of the data buffer to be hashed, extended, and logged.
                                If BIT0 of Flags is 1, it is physical address of the
                                start of the pre-hash data buffter to be extended, and logged.
                                The pre-hash data format is TPML_DIGEST_VALUES.
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
  EFI_STATUS          Status;
  TPML_DIGEST_VALUES  DigestList;

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (((Flags & EDKII_TCG_PRE_HASH) != 0) || ((Flags & EDKII_TCG_PRE_HASH_LOG_ONLY) != 0)) {
    ZeroMem (&DigestList, sizeof (DigestList));
    CopyMem (&DigestList, HashData, sizeof (DigestList));
    Status = EFI_SUCCESS;
    if ((Flags & EDKII_TCG_PRE_HASH) != 0 ) {
      Status = Tpm2PcrExtend (
                 NewEventHdr->PCRIndex,
                 &DigestList
                 );
    }
  } else {
    Status = HashAndExtend (
               NewEventHdr->PCRIndex,
               HashData,
               HashDataLen,
               &DigestList
               );
  }

  if (!EFI_ERROR (Status)) {
    Status = LogHashEvent (&DigestList, NewEventHdr, NewEventData);
  }

  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((DEBUG_ERROR, "HashLogExtendEvent - %r. Disable TPM.\n", Status));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
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
  Get the FvName from the FV header.

  Causion: The FV is untrusted input.

  @param[in]  FvBase            Base address of FV image.
  @param[in]  FvLength          Length of FV image.

  @return FvName pointer
  @retval NULL   FvName is NOT found
**/
VOID *
GetFvName (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FvExtHeader;

  if (FvBase >= MAX_ADDRESS) {
    return NULL;
  }

  if (FvLength >= MAX_ADDRESS - FvBase) {
    return NULL;
  }

  if (FvLength < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    return NULL;
  }

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvBase;
  if (FvHeader->ExtHeaderOffset < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    return NULL;
  }

  if (FvHeader->ExtHeaderOffset + sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER) > FvLength) {
    return NULL;
  }

  FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(UINTN)(FvBase + FvHeader->ExtHeaderOffset);

  return &FvExtHeader->FvName;
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
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  UINT32                                                 Index;
  EFI_STATUS                                             Status;
  EFI_PLATFORM_FIRMWARE_BLOB                             FvBlob;
  FV_HANDOFF_TABLE_POINTERS2                             FvBlob2;
  VOID                                                   *EventData;
  VOID                                                   *FvName;
  TCG_PCR_EVENT_HDR                                      TcgEventHdr;
  UINT32                                                 Instance;
  UINT32                                                 Tpm2HashMask;
  TPML_DIGEST_VALUES                                     DigestList;
  UINT32                                                 DigestCount;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_PPI  *MeasurementExcludedFvPpi;
  EDKII_PEI_FIRMWARE_VOLUME_INFO_PREHASHED_FV_PPI        *PrehashedFvPpi;
  HASH_INFO                                              *PreHashInfo;
  UINT32                                                 HashAlgoMask;
  EFI_PHYSICAL_ADDRESS                                   FvOrgBase;
  EFI_PHYSICAL_ADDRESS                                   FvDataBase;
  EFI_PEI_HOB_POINTERS                                   Hob;
  EDKII_MIGRATED_FV_INFO                                 *MigratedFvInfo;

  //
  // Check Excluded FV list
  //
  Instance = 0;
  do {
    Status = PeiServicesLocatePpi (
               &gEfiPeiFirmwareVolumeInfoMeasurementExcludedPpiGuid,
               Instance,
               NULL,
               (VOID **)&MeasurementExcludedFvPpi
               );
    if (!EFI_ERROR (Status)) {
      for (Index = 0; Index < MeasurementExcludedFvPpi->Count; Index++) {
        if (  (MeasurementExcludedFvPpi->Fv[Index].FvBase == FvBase)
           && (MeasurementExcludedFvPpi->Fv[Index].FvLength == FvLength))
        {
          DEBUG ((DEBUG_INFO, "The FV which is excluded by Tcg2Pei starts at: 0x%x\n", FvBase));
          DEBUG ((DEBUG_INFO, "The FV which is excluded by Tcg2Pei has the size: 0x%x\n", FvLength));
          return EFI_SUCCESS;
        }
      }

      Instance++;
    }
  } while (!EFI_ERROR (Status));

  //
  // Check measured FV list
  //
  for (Index = 0; Index < mMeasuredBaseFvIndex; Index++) {
    if ((mMeasuredBaseFvInfo[Index].BlobBase == FvBase) && (mMeasuredBaseFvInfo[Index].BlobLength == FvLength)) {
      DEBUG ((DEBUG_INFO, "The FV which is already measured by Tcg2Pei starts at: 0x%x\n", FvBase));
      DEBUG ((DEBUG_INFO, "The FV which is already measured by Tcg2Pei has the size: 0x%x\n", FvLength));
      return EFI_SUCCESS;
    }
  }

  //
  // Check pre-hashed FV list
  //
  Instance     = 0;
  Tpm2HashMask = PcdGet32 (PcdTpm2HashMask);
  do {
    Status = PeiServicesLocatePpi (
               &gEdkiiPeiFirmwareVolumeInfoPrehashedFvPpiGuid,
               Instance,
               NULL,
               (VOID **)&PrehashedFvPpi
               );
    if (!EFI_ERROR (Status) && (PrehashedFvPpi->FvBase == FvBase) && (PrehashedFvPpi->FvLength == FvLength)) {
      ZeroMem (&DigestList, sizeof (TPML_DIGEST_VALUES));

      //
      // The FV is prehashed, check against TPM hash mask
      //
      PreHashInfo = (HASH_INFO *)(PrehashedFvPpi + 1);
      for (Index = 0, DigestCount = 0; Index < PrehashedFvPpi->Count; Index++) {
        DEBUG ((DEBUG_INFO, "Hash Algo ID in PrehashedFvPpi=0x%x\n", PreHashInfo->HashAlgoId));
        HashAlgoMask = GetHashMaskFromAlgo (PreHashInfo->HashAlgoId);
        if ((Tpm2HashMask & HashAlgoMask) != 0 ) {
          //
          // Hash is required, copy it to DigestList
          //
          WriteUnaligned16 (&(DigestList.digests[DigestCount].hashAlg), PreHashInfo->HashAlgoId);
          CopyMem (
            &DigestList.digests[DigestCount].digest,
            PreHashInfo + 1,
            PreHashInfo->HashSize
            );
          DigestCount++;
          //
          // Clean the corresponding Hash Algo mask bit
          //
          Tpm2HashMask &= ~HashAlgoMask;
        }

        PreHashInfo = (HASH_INFO *)((UINT8 *)(PreHashInfo + 1) + PreHashInfo->HashSize);
      }

      WriteUnaligned32 (&DigestList.count, DigestCount);

      break;
    }

    Instance++;
  } while (!EFI_ERROR (Status));

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
  // Init the log event for FV measurement
  //
  if (PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105) {
    FvBlob2.BlobDescriptionSize = sizeof (FvBlob2.BlobDescription);
    CopyMem (FvBlob2.BlobDescription, FV_HANDOFF_TABLE_DESC, sizeof (FvBlob2.BlobDescription));
    FvName = GetFvName (FvBase, FvLength);
    if (FvName != NULL) {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
    }

    FvBlob2.BlobBase      = FvOrgBase;
    FvBlob2.BlobLength    = FvLength;
    TcgEventHdr.PCRIndex  = 0;
    TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB2;
    TcgEventHdr.EventSize = sizeof (FvBlob2);
    EventData             = &FvBlob2;
  } else {
    FvBlob.BlobBase       = FvOrgBase;
    FvBlob.BlobLength     = FvLength;
    TcgEventHdr.PCRIndex  = 0;
    TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB;
    TcgEventHdr.EventSize = sizeof (FvBlob);
    EventData             = &FvBlob;
  }

  if (Tpm2HashMask == 0) {
    //
    // FV pre-hash algos comply with current TPM hash requirement
    // Skip hashing step in measure, only extend DigestList to PCR and log event
    //
    Status = HashLogExtendEvent (
               &mEdkiiTcgPpi,
               EDKII_TCG_PRE_HASH,
               (UINT8 *)&DigestList,        // HashData
               (UINTN)sizeof (DigestList),  // HashDataLen
               &TcgEventHdr,                // EventHdr
               EventData                    // EventData
               );
    DEBUG ((DEBUG_INFO, "The pre-hashed FV which is extended & logged by Tcg2Pei starts at: 0x%x\n", FvBase));
    DEBUG ((DEBUG_INFO, "The pre-hashed FV which is extended & logged by Tcg2Pei has the size: 0x%x\n", FvLength));
  } else {
    //
    // Hash the FV, extend digest to the TPM and log TCG event
    //
    Status = HashLogExtendEvent (
               &mEdkiiTcgPpi,
               0,
               (UINT8 *)(UINTN)FvDataBase, // HashData
               (UINTN)FvLength,            // HashDataLen
               &TcgEventHdr,               // EventHdr
               EventData                   // EventData
               );
    DEBUG ((DEBUG_INFO, "The FV which is measured by Tcg2Pei starts at: 0x%x\n", FvBase));
    DEBUG ((DEBUG_INFO, "The FV which is measured by Tcg2Pei has the size: 0x%x\n", FvLength));
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "The FV which failed to be measured starts at: 0x%x\n", FvBase));
    return Status;
  }

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

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
MeasureMainBios (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_FV_HANDLE            VolumeHandle;
  EFI_FV_INFO                  VolumeInfo;
  EFI_PEI_FIRMWARE_VOLUME_PPI  *FvPpi;

  PERF_START_EX (mFileHandle, "EventRec", "Tcg2Pei", 0, PERF_ID_TCG2_PEI);

  //
  // Only measure BFV at the very beginning. Other parts of Static Core Root of
  // Trust for Measurement(S-CRTM) will be measured later on FvInfoNotify.
  // BFV is processed without installing FV Info Ppi. Other FVs either inside BFV or
  // reported by platform will be installed with Fv Info Ppi
  // This firmware volume measure policy can be modified/enhanced by special
  // platform for special CRTM TPM measuring.
  //
  Status = PeiServicesFfsFindNextVolume (0, &VolumeHandle);
  ASSERT_EFI_ERROR (Status);

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
  ASSERT_EFI_ERROR (Status);

  Status = MeasureFvImage ((EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart, VolumeInfo.FvSize);

  PERF_END_EX (mFileHandle, "EventRec", "Tcg2Pei", 0, PERF_ID_TCG2_PEI + 1);

  return Status;
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
  Do measurement after memory is ready.

  @param[in]      PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
PeimEntryMP (
  IN      EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  //
  // install Tcg Services
  //
  Status = PeiServicesInstallPpi (&mTcgPpiList);
  ASSERT_EFI_ERROR (Status);

  if (PcdGet8 (PcdTpm2ScrtmPolicy) == 1) {
    Status = MeasureCRTMVersion ();
  }

  Status = MeasureMainBios ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
  Measure and log Separator event with error, and extend the measurement result into a specific PCR.

  @param[in] PCRIndex         PCR index.

  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The operation was unsuccessful.

**/
EFI_STATUS
MeasureSeparatorEventWithError (
  IN      TPM_PCRINDEX  PCRIndex
  )
{
  TCG_PCR_EVENT_HDR  TcgEvent;
  UINT32             EventData;

  //
  // Use EventData 0x1 to indicate there is error.
  //
  EventData          = 0x1;
  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EV_SEPARATOR;
  TcgEvent.EventSize = (UINT32)sizeof (EventData);
  return HashLogExtendEvent (&mEdkiiTcgPpi, 0, (UINT8 *)&EventData, TcgEvent.EventSize, &TcgEvent, (UINT8 *)&EventData);
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
  TPM_PCRINDEX   PcrIndex;
  BOOLEAN        S3ErrorReport;

  if (CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid) ||
      CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid))
  {
    DEBUG ((DEBUG_INFO, "No TPM2 instance required!\n"));
    return EFI_UNSUPPORTED;
  }

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((DEBUG_ERROR, "TPM2 error!\n"));
    return EFI_DEVICE_ERROR;
  }

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // In S3 path, skip shadow logic. no measurement is required
  //
  if (BootMode != BOOT_ON_S3_RESUME) {
    Status = (**PeiServices).RegisterForShadow (FileHandle);
    if (Status == EFI_ALREADY_STARTED) {
      mImageInMemory = TRUE;
      mFileHandle    = FileHandle;
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

    S3ErrorReport = FALSE;
    if (PcdGet8 (PcdTpm2InitializationPolicy) == 1) {
      if (BootMode == BOOT_ON_S3_RESUME) {
        Status = Tpm2Startup (TPM_SU_STATE);
        if (EFI_ERROR (Status)) {
          Status = Tpm2Startup (TPM_SU_CLEAR);
          if (!EFI_ERROR (Status)) {
            S3ErrorReport = TRUE;
          }
        }
      } else {
        Status = Tpm2Startup (TPM_SU_CLEAR);
      }

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }

    //
    // Update Tpm2HashMask according to PCR bank.
    //
    SyncPcrAllocationsAndPcrMask ();

    if (S3ErrorReport) {
      //
      // The system firmware that resumes from S3 MUST deal with a
      // TPM2_Startup error appropriately.
      // For example, issue a TPM2_Startup(TPM_SU_CLEAR) command and
      // configuring the device securely by taking actions like extending a
      // separator with an error digest (0x01) into PCRs 0 through 7.
      //
      for (PcrIndex = 0; PcrIndex < 8; PcrIndex++) {
        Status = MeasureSeparatorEventWithError (PcrIndex);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Separator Event with Error not Measured. Error!\n"));
        }
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

    DEBUG_CODE_BEGIN ();
    //
    // Peek into TPM PCR 00 before any BIOS measurement.
    //
    Tpm2PcrReadForActiveBank (00, NULL);
    DEBUG_CODE_END ();

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
    DEBUG ((DEBUG_ERROR, "TPM2 error! Build Hob\n"));
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
