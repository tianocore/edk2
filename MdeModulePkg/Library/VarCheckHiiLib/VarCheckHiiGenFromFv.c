/** @file
  Var Check Hii generation from FV.

Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VarCheckHiiGen.h"

// {d0bc7cb4-6a47-495f-aa11-710746da06a2}
#define EFI_VFR_ATTRACT_GUID \
{ 0xd0bc7cb4, 0x6a47, 0x495f, { 0xaa, 0x11, 0x71, 0x7, 0x46, 0xda, 0x6, 0xa2 } }

EFI_GUID  gVfrArrayAttractGuid = EFI_VFR_ATTRACT_GUID;

#define ALL_FF_GUID \
{ 0xFFFFFFFF, 0xFFFF, 0xFFFF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } }

EFI_GUID  mAllFfGuid = ALL_FF_GUID;

#define VAR_CHECK_VFR_DRIVER_INFO_SIGNATURE  SIGNATURE_32 ('V', 'D', 'R', 'I')

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Link;
  EFI_GUID      *DriverGuid;
} VAR_CHECK_VFR_DRIVER_INFO;

LIST_ENTRY  mVfrDriverList = INITIALIZE_LIST_HEAD_VARIABLE (mVfrDriverList);

#define VAR_CHECK_VFR_DRIVER_INFO_FROM_LINK(a)  CR (a, VAR_CHECK_VFR_DRIVER_INFO, Link, VAR_CHECK_VFR_DRIVER_INFO_SIGNATURE)

#define MAX_MATCH_GUID_NUM  100

/**
  Get the address by Guid.

  Parse the FFS and find the GUID address.
  There may be multiple Guids matching the searched Guid.

  @param Ffs                Pointer to the FFS.
  @param Guid               Guid to find.
  @param Length             The length of FFS.
  @param Offset             Pointer to pointer to the offset.
  @param NumOfMatchingGuid  The number of matching Guid.

  @retval EFI_SUCCESS       One or multiple Guids matching the searched Guid.
  @retval EFI_NOT_FOUND     No Guid matching the searched Guid.

**/
EFI_STATUS
GetAddressByGuid (
  IN  VOID      *Ffs,
  IN  EFI_GUID  *Guid,
  IN  UINTN     Length,
  OUT UINTN     **Offset,
  OUT UINT8     *NumOfMatchingGuid
  )
{
  UINTN    LoopControl;
  BOOLEAN  Found;

  if ((Ffs == NULL) || (Guid == NULL) || (Length == 0)) {
    return EFI_NOT_FOUND;
  }

  if (NumOfMatchingGuid != NULL) {
    *NumOfMatchingGuid = 0;
  }

  Found = FALSE;
  for (LoopControl = 0; LoopControl < Length; LoopControl++) {
    if (CompareGuid (Guid, (EFI_GUID *)((UINT8 *)Ffs + LoopControl))) {
      Found = TRUE;
      //
      // If NumOfMatchGuid or Offset are NULL, means user only want
      // to check whether current FFS includes this Guid or not.
      //
      if ((NumOfMatchingGuid != NULL) && (Offset != NULL)) {
        if (*NumOfMatchingGuid == 0) {
          *Offset = InternalVarCheckAllocateZeroPool (sizeof (UINTN) * MAX_MATCH_GUID_NUM);
          ASSERT (*Offset != NULL);
        }

        *(*Offset + *NumOfMatchingGuid) = LoopControl + sizeof (EFI_GUID);
        (*NumOfMatchingGuid)++;
      } else {
        break;
      }
    }
  }

  return (Found ? EFI_SUCCESS : EFI_NOT_FOUND);
}

/**
  Search the VfrBin Base address.

  According to the known GUID gVfrArrayAttractGuid to get the base address from FFS.

  @param Ffs                    Pointer to the FFS.
  @param EfiAddr                Pointer to the EFI in FFS
  @param Length                 The length of FFS.
  @param Offset                 Pointer to pointer to the Addr (Offset).
  @param NumOfMatchingOffset    The number of Addr (Offset).

  @retval EFI_SUCCESS           Get the address successfully.
  @retval EFI_NOT_FOUND         No VfrBin found.

**/
EFI_STATUS
SearchVfrBinInFfs (
  IN  VOID   *Ffs,
  IN  VOID   *EfiAddr,
  IN  UINTN  Length,
  OUT UINTN  **Offset,
  OUT UINT8  *NumOfMatchingOffset
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  UINTN       VirOffValue;

  if ((Ffs == NULL) || (Offset == NULL)) {
    return EFI_NOT_FOUND;
  }

  Status = GetAddressByGuid (
             Ffs,
             &gVfrArrayAttractGuid,
             Length,
             Offset,
             NumOfMatchingOffset
             );
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  for (Index = 0; Index < *NumOfMatchingOffset; Index++) {
    //
    // Got the virOffset after the GUID
    //
    VirOffValue = *(UINTN *)((UINTN)Ffs + *(*Offset + Index));
    //
    // Transfer the offset to the VA address. One modules may own multiple VfrBin address.
    //
    *(*Offset + Index) = (UINTN)EfiAddr + VirOffValue;
  }

  return Status;
}

/**
  Parse FFS.

  @param[in] Fv2            Pointer to Fv2 protocol.
  @param[in] DriverGuid     Pointer to driver GUID.

  @return Found the driver in the FV or not.

**/
BOOLEAN
ParseFfs (
  IN EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv2,
  IN EFI_GUID                       *DriverGuid
  )
{
  EFI_STATUS              Status;
  EFI_FV_FILETYPE         FoundType;
  EFI_FV_FILE_ATTRIBUTES  FileAttributes;
  UINT32                  AuthenticationStatus;
  UINTN                   Size;
  VOID                    *Buffer;
  UINTN                   SectionSize;
  VOID                    *SectionBuffer;
  UINTN                   VfrBinIndex;
  UINT8                   NumberofMatchingVfrBin;
  UINTN                   *VfrBinBaseAddress;

  Status = Fv2->ReadFile (
                  Fv2,
                  DriverGuid,
                  NULL,
                  &Size,
                  &FoundType,
                  &FileAttributes,
                  &AuthenticationStatus
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Buffer = NULL;
  Status = Fv2->ReadSection (
                  Fv2,
                  DriverGuid,
                  EFI_SECTION_RAW,
                  0, // Instance
                  &Buffer,
                  &Size,
                  &AuthenticationStatus
                  );
  if (!EFI_ERROR (Status)) {
    Status = SearchVfrBinInFfs (Buffer, 0, Size, &VfrBinBaseAddress, &NumberofMatchingVfrBin);
    if (!EFI_ERROR (Status)) {
      SectionBuffer = NULL;
      Status        = Fv2->ReadSection (
                             Fv2,
                             DriverGuid,
                             EFI_SECTION_PE32,
                             0, // Instance
                             &SectionBuffer,
                             &SectionSize,
                             &AuthenticationStatus
                             );
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "FfsNameGuid - %g\n", DriverGuid));
        DEBUG ((DEBUG_INFO, "NumberofMatchingVfrBin - 0x%02x\n", NumberofMatchingVfrBin));

        for (VfrBinIndex = 0; VfrBinIndex < NumberofMatchingVfrBin; VfrBinIndex++) {
 #ifdef DUMP_HII_DATA
          DEBUG_CODE (
            DumpHiiPackage ((UINT8 *)(UINTN)SectionBuffer + VfrBinBaseAddress[VfrBinIndex] + sizeof (UINT32));
            );
 #endif
          VarCheckParseHiiPackage ((UINT8 *)(UINTN)SectionBuffer + VfrBinBaseAddress[VfrBinIndex] + sizeof (UINT32), TRUE);
        }

        FreePool (SectionBuffer);
      }

      InternalVarCheckFreePool (VfrBinBaseAddress);
    }

    FreePool (Buffer);
  }

  return TRUE;
}

/**
  Parse FVs.

  @param[in] ScanAll    Scan all modules in all FVs or not.

**/
VOID
ParseFv (
  IN BOOLEAN  ScanAll
  )
{
  EFI_STATUS                     Status;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          HandleCount;
  UINTN                          Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv2;
  VOID                           *Key;
  EFI_FV_FILETYPE                FileType;
  EFI_GUID                       NameGuid;
  EFI_FV_FILE_ATTRIBUTES         FileAttributes;
  UINTN                          Size;
  UINTN                          FfsIndex;
  VAR_CHECK_VFR_DRIVER_INFO      *VfrDriverInfo;
  LIST_ENTRY                     *VfrDriverLink;

  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiFirmwareVolume2ProtocolGuid,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Search all FVs
  //
  for (Index = 0; Index < HandleCount; Index++) {
    DEBUG ((DEBUG_INFO, "FvIndex - %x\n", Index));
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&Fv2
                    );
    ASSERT_EFI_ERROR (Status);

    DEBUG_CODE_BEGIN ();
    EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *Fvb2;
    EFI_PHYSICAL_ADDRESS                 FvAddress;
    UINT64                               FvSize;

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolumeBlock2ProtocolGuid,
                    (VOID **)&Fvb2
                    );
    ASSERT_EFI_ERROR (Status);
    Status = Fvb2->GetPhysicalAddress (Fvb2, &FvAddress);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "FvAddress - 0x%08x\n", FvAddress));
      FvSize = ((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvAddress)->FvLength;
      DEBUG ((DEBUG_INFO, "FvSize    - 0x%08x\n", FvSize));
    }

    DEBUG_CODE_END ();

    if (ScanAll) {
      //
      // Need to parse all modules in all FVs.
      //
      Key = InternalVarCheckAllocateZeroPool (Fv2->KeySize);
      ASSERT (Key != NULL);

      for (FfsIndex = 0; ; FfsIndex++) {
        FileType = EFI_FV_FILETYPE_ALL;
        Status   = Fv2->GetNextFile (
                          Fv2,
                          Key,
                          &FileType,
                          &NameGuid,
                          &FileAttributes,
                          &Size
                          );
        if (EFI_ERROR (Status)) {
          break;
        }

        ParseFfs (Fv2, &NameGuid);
      }

      InternalVarCheckFreePool (Key);
    } else {
      //
      // Only parse drivers in the VFR drivers list.
      //
      VfrDriverLink = mVfrDriverList.ForwardLink;
      while (VfrDriverLink != &mVfrDriverList) {
        VfrDriverInfo = VAR_CHECK_VFR_DRIVER_INFO_FROM_LINK (VfrDriverLink);
        VfrDriverLink = VfrDriverLink->ForwardLink;
        if (ParseFfs (Fv2, VfrDriverInfo->DriverGuid)) {
          //
          // Found the driver in the FV.
          //
          RemoveEntryList (&VfrDriverInfo->Link);
          InternalVarCheckFreePool (VfrDriverInfo);
        }
      }
    }
  }

  FreePool (HandleBuffer);
}

/**
  Create Vfr Driver List.

  @param[in] DriverGuidArray    Driver Guid Array

**/
VOID
CreateVfrDriverList (
  IN EFI_GUID  *DriverGuidArray
  )
{
  UINTN                      Index;
  VAR_CHECK_VFR_DRIVER_INFO  *VfrDriverInfo;

  for (Index = 0; !IsZeroGuid (&DriverGuidArray[Index]); Index++) {
    DEBUG ((DEBUG_INFO, "CreateVfrDriverList: %g\n", &DriverGuidArray[Index]));
    VfrDriverInfo = InternalVarCheckAllocateZeroPool (sizeof (*VfrDriverInfo));
    ASSERT (VfrDriverInfo != NULL);
    VfrDriverInfo->Signature  = VAR_CHECK_VFR_DRIVER_INFO_SIGNATURE;
    VfrDriverInfo->DriverGuid = &DriverGuidArray[Index];
    InsertTailList (&mVfrDriverList, &VfrDriverInfo->Link);
  }
}

/**
  Destroy Vfr Driver List.

**/
VOID
DestroyVfrDriverList (
  VOID
  )
{
  VAR_CHECK_VFR_DRIVER_INFO  *VfrDriverInfo;
  LIST_ENTRY                 *VfrDriverLink;

  while (mVfrDriverList.ForwardLink != &mVfrDriverList) {
    VfrDriverLink = mVfrDriverList.ForwardLink;
    VfrDriverInfo = VAR_CHECK_VFR_DRIVER_INFO_FROM_LINK (VfrDriverLink);
    RemoveEntryList (&VfrDriverInfo->Link);
    InternalVarCheckFreePool (VfrDriverInfo);
  }
}

/**
  Generate from FV.

**/
VOID
VarCheckHiiGenFromFv (
  VOID
  )
{
  EFI_GUID  *DriverGuidArray;
  BOOLEAN   ScanAll;

  DEBUG ((DEBUG_INFO, "VarCheckHiiGenDxeFromFv\n"));

  //
  // Get vfr driver guid array from PCD.
  //
  DriverGuidArray = (EFI_GUID *)PcdGetPtr (PcdVarCheckVfrDriverGuidArray);

  if (IsZeroGuid (&DriverGuidArray[0])) {
    //
    // No VFR driver will be parsed from FVs.
    //
    return;
  }

  if (CompareGuid (&DriverGuidArray[0], &mAllFfGuid)) {
    ScanAll = TRUE;
  } else {
    ScanAll = FALSE;
    CreateVfrDriverList (DriverGuidArray);
  }

  ParseFv (ScanAll);

  if (!ScanAll) {
    DestroyVfrDriverList ();
  }
}
