/** @file
  PEI Firmware Interface Table (FIT) Library instance.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent.

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FirmwareInterfaceTableLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>

#include <Guid/Fit.h>
#include <IndustryStandard/Acm.h>
#include <IndustryStandard/FirmwareInterfaceTable.h>

UINT8 FitHobFitEntryTypeExclusions[] = {
  FIT_TYPE_01_MICROCODE,
  FIT_TYPE_07_BIOS_STARTUP_MODULE
};

UINT8 FitHobFitObjectTypeExclusions[] = {
  FIT_TYPE_00_HEADER,
  FIT_TYPE_01_MICROCODE,
  FIT_TYPE_07_BIOS_STARTUP_MODULE,
  FIT_TYPE_10_CSE_SECURE_BOOT
};

/**
  Determine whether an entry of a given FIT record type should be used.

  @param[in]  FitEntryType          A FIT record type.

  @retval     TRUE                  The FIT entry is not in the exclusion list and should be used.
  @retval     FALSE                 The FIT entry is in the exclusion list and should not be used.
**/
BOOLEAN
EFIAPI
UseFitTypeEntry (
  UINT8 FitEntryType
  )
{
  UINT32 Index;

  for (Index = 0; Index < ARRAY_SIZE (FitHobFitEntryTypeExclusions); Index++) {
    if (FitHobFitEntryTypeExclusions[Index] == FitEntryType) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Determine whether an object in a given FIT record type should be used.

  @param[in]  FitObjectType         A FIT record type.

  @retval     TRUE                  The FIT object is not in the exclusion list and should be used.
  @retval     FALSE                 The FIT object is in the exclusion list and should not be used.
**/
BOOLEAN
EFIAPI
UseFitObjectEntry (
  UINT8 FitObjectType
  )
{
  UINT32 Index;

  for (Index = 0; Index < ARRAY_SIZE (FitHobFitObjectTypeExclusions); Index++) {
    if (FitHobFitObjectTypeExclusions[Index] == FitObjectType) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Finds the base and size of ACM FW.

  The residing memory of the ACM binary should be valid while the FIT containing its address is valid. For example,
  a FIT that can be found in post-memory should not point to an ACM FW in CPU cache given cache is invalidated
  when permanent memory is enabled.

  @param[out] AcmBase               A pointer to a pointer to the ACM base.
  @param[out] AcmSize               The size of the ACM FW in bytes.

  @retval     EFI_SUCCESS           The ACM FW base address was determined successfully.
  @retval     EFI_INVALID_PARAMETER An NULL pointer was given.
  @retval     EFI_NOT_FOUND         The ACM FW base address could not be determined.
**/
EFI_STATUS
EFIAPI
GetAcmBaseAndSize (
  OUT VOID  **AcmBase,
  OUT UINTN *AcmSize
  )
{
  EFI_STATUS          Status;
  EFI_STATUS          FindFvStatus;
  UINTN               FvInstance;
  EFI_PEI_FV_HANDLE   FvHandle;
  EFI_PEI_FILE_HANDLE FileHandle;
  ACM_HEADER          *AcmHeader;

  if (AcmBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (FvInstance = 0, FindFvStatus = EFI_SUCCESS; !EFI_ERROR (FindFvStatus); FvInstance++) {
    FindFvStatus = PeiServicesFfsFindNextVolume (FvInstance, &FvHandle);
    if (!EFI_ERROR (FindFvStatus)) {
      ASSERT (((EFI_FIRMWARE_VOLUME_HEADER *) FvHandle)->Signature == EFI_FVH_SIGNATURE);
      FileHandle = NULL;
      Status = PeiServicesFfsFindFileByName (&gStartupAcmPeiFileGuid, FvHandle, &FileHandle);
      if (!EFI_ERROR (Status)) {
        if (IS_FFS_FILE2 (FileHandle)) {
          AcmHeader = (ACM_HEADER *) ((UINT8 *) FileHandle + sizeof (EFI_FFS_FILE_HEADER2));
        } else {
          AcmHeader = (ACM_HEADER *) ((UINT8 *) FileHandle + sizeof (EFI_FFS_FILE_HEADER));
        }
        if (!EFI_ERROR (Status) && AcmHeader->ModuleType == ACM_MODULE_TYPE_CHIPSET_ACM) {
          DEBUG ((DEBUG_INFO, "ACM discovered at memory location 0x%x.\n", (UINTN) AcmHeader));
          DEBUG ((DEBUG_INFO, "  ACM Chipset ID: 0x%x\n", AcmHeader->ChipsetId));
          DEBUG ((DEBUG_INFO, "  ACM Date: 0x%x\n", AcmHeader->Date));
          DEBUG ((DEBUG_INFO, "  ACM Size: 0x%x\n", AcmHeader->Size));
          *AcmBase = (VOID *) AcmHeader;
          *AcmSize = AcmHeader->Size;

          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Calculates the number of FIT entries and total size for the FIT entries and associated objects.

  @param[in]  Fit                   A pointer to the Fit table to be used in the calculation.
  @param[in]  NemBase               The NEM base address.
  @param[in]  SourceFitEntries      The number of entries to process in SourceFit.
  @param[out] FitEntries            The number of FIT entries found not in the exclusion list.
  @param[out] FitSize               The total size needed for the FIT entries and FIT objects not in the
                                    exclusion lists.

  @retval     EFI_SUCCESS           The FIT entry and object calculation was performed successfully.
  @retval     EFI_INVALID_PARAMETER A given parameter is invalid.
**/
EFI_STATUS
EFIAPI
CalculateFitEntriesAndSize (
  IN  CONST FIRMWARE_INTERFACE_TABLE_ENTRY  *Fit,
  IN  UINTN                                 NemBase,
  IN  UINT8                                 FitSourceEntries,
  OUT UINT8                                 *FitEntries,
  OUT UINT16                                *FitSize
  )
{
  UINT32  Index;

  if (Fit == NULL || NemBase == 0 || FitSourceEntries == 0 || FitEntries == NULL || FitSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (*FitEntries = 0, *FitSize = 0, Index = 0; Index < FitSourceEntries; Index++) {
    if (UseFitTypeEntry (Fit[Index].Type)) {
      DEBUG ((DEBUG_INFO, "[%d] - Found FIT entry of type %d.\n", Index, Fit[Index].Type));

      //
      // Do not include ACM binary in the HOB buffer allocation as it is too large and will cause the buffer to exceed
      // the maximum HOB size. ACM will be pointed to in its discovered location within the BIOS image from this FIT.
      //
      if (
        UseFitObjectEntry (Fit[Index].Type) && Fit[Index].Address >= (UINT64) NemBase &&
        Fit[Index].Type != FIT_TYPE_02_STARTUP_ACM
        ) {
        DEBUG ((
          DEBUG_INFO,
          "  Found FIT object. Size 0x%x. Address 0x%x.\n",
          (UINTN) *(UINT32 *) (&Fit[Index].Size[0]) & 0xFFFFFF,
          Fit[Index].Address
          ));
        *FitSize += *(UINT16 *) (&Fit[Index].Size[0]) & 0xFFFFFF;
        *FitSize += FIT_ALIGNMENT;
      }
      (*FitEntries)++;
    } else {
      DEBUG ((DEBUG_INFO, "[%d] - Found but ignoring FIT entry of type %d.\n", Index, Fit[Index].Type));
    }
  }
  *FitSize += ((*FitEntries + 1) * sizeof (FIRMWARE_INTERFACE_TABLE_ENTRY));
  *FitSize = ALIGN_VALUE (*FitSize, FIT_ALIGNMENT);

  return EFI_SUCCESS;
}

/**
  Copies FIT entries and objects from a source FIT to a destination FIT.

  @param[in]  SourceFit             The source FIT buffer.
  @param[out] DesinationFit         The destination FIT buffer.
  @param[in]  SourceFitEntries      The number of entries to process in SourceFit.
  @param[in]  DestinationFitEntries The total number of entries in DestinationFit.
  @param[in]  NemBase               The NEM base address.

  @retval     EFI_SUCCESS           The FIT entries and objects were copied successfully.
  @retval     EFI_INVALID_PARAMETER A given parameter is invalid.
**/
EFI_STATUS
EFIAPI
CopyFitEntriesAndObjects (
  IN CONST FIRMWARE_INTERFACE_TABLE_ENTRY *SourceFit,
  OUT      FIRMWARE_INTERFACE_TABLE_ENTRY *DestinationFit,
  IN       UINT8                          SourceFitEntries,
  IN       UINT8                          DestinationFitEntries,
  IN       UINTN                          NemBase
  )
{
  EFI_STATUS  Status;
  UINT32      DestinationIndex;
  UINT32      Index;
  UINTN       FitObjectSize;
  VOID        *DestinationFitObjectPtr;
  VOID        *SourceFitObjectPtr;

  if (SourceFit == NULL || DestinationFit == NULL || SourceFitEntries == 0 || NemBase == 0) {
    return EFI_INVALID_PARAMETER;
  }

  DestinationFitObjectPtr = (VOID *) &(DestinationFit[DestinationFitEntries + 1]);
  DestinationFitObjectPtr = ALIGN_POINTER (DestinationFitObjectPtr, FIT_ALIGNMENT);

  for (DestinationIndex = 0, Index = 0; Index < SourceFitEntries; Index++) {
    if (UseFitTypeEntry (SourceFit[Index].Type)) {
      DEBUG ((
        DEBUG_INFO,
        "Copying FIT entry of type %d from 0x%x to 0x%x.\n",
        SourceFit[Index].Type,
        (UINTN) &SourceFit[Index],
        (UINTN) &DestinationFit[DestinationIndex]
        ));
      CopyMem (
        (VOID *) &DestinationFit[DestinationIndex],
        (VOID *) &SourceFit[Index],
        sizeof (FIRMWARE_INTERFACE_TABLE_ENTRY)
        );

      if (UseFitObjectEntry (SourceFit[Index].Type)) {
        if (SourceFit[Index].Type == FIT_TYPE_02_STARTUP_ACM) {
          Status = GetAcmBaseAndSize (&SourceFitObjectPtr, &FitObjectSize);
          if (EFI_ERROR (Status)) {
            DestinationFit[DestinationIndex].Address = 0;
            DestinationFit[DestinationIndex].Size[0] = 0;
          } else {
            DestinationFit[DestinationIndex].Address = (UINT64) SourceFitObjectPtr;
            DestinationFit[DestinationIndex].Size[0] = (UINT8) (FitObjectSize / 16);
          }
        } else if (SourceFit[Index].Address >= (UINT64) NemBase) {
          FitObjectSize = *(UINTN *) (&SourceFit[Index].Size[0]) & 0xFFFFFF;
          SourceFitObjectPtr = (VOID *) (UINTN) SourceFit[Index].Address;
          DEBUG ((
            DEBUG_INFO,
            "Copying FIT object for entry of type %d of size 0x%x from 0x%x to 0x%x.\n",
            SourceFit[Index].Type,
            FitObjectSize,
            (UINTN) SourceFitObjectPtr,
            (UINTN) DestinationFitObjectPtr
            ));
          CopyMem (DestinationFitObjectPtr, SourceFitObjectPtr, FitObjectSize);
          DestinationFit[DestinationIndex].Address = (UINT64) DestinationFitObjectPtr;
          DestinationFitObjectPtr = (VOID *) ((UINT8 *) DestinationFitObjectPtr + FitObjectSize);
          DestinationFitObjectPtr = ALIGN_POINTER (DestinationFitObjectPtr, FIT_ALIGNMENT);
        }
      }
      DestinationIndex++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Copies FIT data from a pre-existing FIT location to a FIT HOB.

  Future FIT API requests will return information from the FIT HOB after this API is invoked. The FIT entries
  may be trimmed to BIOS-relevant entries to reduce memory consumption. This allows the total HOB size to be less
  than the maximum HOB size and reduces memory usage if this API is used in pre-memory.

  @param[in]  FitSourceBase         The base address of a pre-existing FIT.

  @retval     EFI_SUCCESS           The FIT HOB was produced successfully.
  @retval     EFI_INVALID_PARAMETER The parameter given is invalid.
  @retval     EFI_NOT_FOUND         The source FIT table could not be found.
**/
EFI_STATUS
EFIAPI
ProduceFitHob (
  IN UINTN FitSourceBase
  )
{
  EFI_STATUS                      Status;
  UINT8                           FitSourceEntries;
  UINT8                           FitDestinationEntries;
  UINT16                          FitDestinationSize;
  FIRMWARE_INTERFACE_TABLE_ENTRY  *FitSourceEntry;
  FIRMWARE_INTERFACE_TABLE_ENTRY  *BackupFit;
  FIT_HOB_STRUCTURE               *FitHobStructureHob;

  BackupFit = NULL;

  if (FitSourceBase == 0) {
    return EFI_INVALID_PARAMETER;
  }

  FitSourceEntry = (FIRMWARE_INTERFACE_TABLE_ENTRY *) FitSourceBase;
  if (FitSourceEntry[0].Address != *(UINT64 *) "_FIT_   ") {
    return EFI_NOT_FOUND;
  } else if (FitSourceEntry[0].Type != FIT_TYPE_00_HEADER) {
    return EFI_NOT_FOUND;
  }

  FitSourceEntries = *(UINT8 *) (&FitSourceEntry[0].Size[0]) & 0xFFFFFF;
  ASSERT (FitSourceEntries > 0);

  Status =  CalculateFitEntriesAndSize (
              FitSourceEntry,
              (UINTN) PcdGet64 (PcdNemBaseAddress),
              FitSourceEntries,
              &FitDestinationEntries,
              &FitDestinationSize
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Source FIT entries = 0x%x.\n", FitSourceEntries));
  DEBUG ((DEBUG_INFO, "Destination FIT entries = 0x%x.\n", FitDestinationEntries));
  DEBUG ((DEBUG_INFO, "Destination FIT size = 0x%x\n", FitDestinationSize));

  //
  // There can only be a single FIT HOB. If a FIT HOB already exists reuse the HOB buffer.
  // It may be recreated to refresh the FIT pointers such as needed during the permanent memory transition.
  //
  FitHobStructureHob = GetFirstGuidHob (&gFitStructureHobGuid);
  if (FitHobStructureHob == NULL) {
    Status =  PeiServicesCreateHob (
                EFI_HOB_TYPE_GUID_EXTENSION,
                ALIGN_VALUE ((UINT16) sizeof (FIT_HOB_STRUCTURE) + FitDestinationSize, FIT_ALIGNMENT),
                (VOID **) &FitHobStructureHob
                );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ZeroMem (GET_GUID_HOB_DATA (FitHobStructureHob), GET_GUID_HOB_DATA_SIZE (FitHobStructureHob));
    CopyGuid (&(FitHobStructureHob->Header.Name), &gFitStructureHobGuid);
    FitHobStructureHob->Revision = 1;
    FitHobStructureHob->Fit = (FIRMWARE_INTERFACE_TABLE_ENTRY *) ALIGN_POINTER ((FitHobStructureHob + 1), FIT_ALIGNMENT);
  } else {
    //
    // Copy a pre-existing FIT HOB to a temporary location so it is not mutated during the copy.
    //
    BackupFit = AllocatePool (GET_GUID_HOB_DATA_SIZE (FitHobStructureHob));
    if (BackupFit == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (
      (VOID *) BackupFit,
      (VOID *) FitHobStructureHob->Fit,
      GET_GUID_HOB_DATA_SIZE (FitHobStructureHob) - OFFSET_OF (FIT_HOB_STRUCTURE, Fit)
      );
    ASSERT (BackupFit[0].Address == *(UINT64 *) "_FIT_   ");
    ASSERT (BackupFit[0].Type == FIT_TYPE_00_HEADER);
    FitSourceEntry = BackupFit;
  }
  ASSERT (FitDestinationSize <= GET_GUID_HOB_DATA_SIZE (FitHobStructureHob));

  Status =  CopyFitEntriesAndObjects (
              FitSourceEntry,
              FitHobStructureHob->Fit,
              FitSourceEntries,
              FitDestinationEntries,
              (UINTN) PcdGet64 (PcdNemBaseAddress)
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

 FitHobStructureHob->Fit[0].Size[0] = FitDestinationEntries;
  if (FitHobStructureHob->Fit[0].C_V == 1) {
    FitHobStructureHob->Fit[0].Chksum = CalculateCheckSum8 (
                                      (UINT8 *) &FitHobStructureHob->Fit[0],
                                      sizeof (FIRMWARE_INTERFACE_TABLE_ENTRY) * FitDestinationEntries
                                      );
  }

  if (BackupFit != NULL) {
    FreePool (BackupFit);
  }

  return EFI_SUCCESS;
}

/**
  Checks if an existing FIT HOB exists before producing a new FIT HOB.

  @param[out]   FitHobStructure             A pointer to a pointer to the FIT_HOB_STRUCTURE. The pointer will be
                                            udpdated to point to a FIT_HOB_STRUCTURE instance.

  @retval       EFI_SUCCESS                 The FIT HOB structure was located and returned successfully.
  @retval       EFI_NOT_FOUND               The gFitStructureHobGuid was not found.
  @retval       EFI_DEVICE_ERROR            The GUID in the NEM map structure is invalid.
**/
EFI_STATUS
EFIAPI
CheckAndProduceFitHob (
  OUT FIT_HOB_STRUCTURE **FitHobStructure
  )
{
  EFI_STATUS          Status;
  FIT_HOB_STRUCTURE   *FitHobStructurePtr;

  FitHobStructurePtr  = GetFirstGuidHob (&gFitStructureHobGuid);
  if (FitHobStructurePtr == NULL) {
    Status = ProduceFitHob ((UINTN) PcdGet64 (PcdFitPointerAddress));
    FitHobStructurePtr = GetFirstGuidHob (&gFitStructureHobGuid);

    if (EFI_ERROR (Status) || FitHobStructurePtr == NULL) {
      DEBUG ((DEBUG_ERROR, "NemMapLib Error: FIT structure HOB does not exist!\n"));
      ASSERT (!EFI_ERROR (Status) && FitHobStructurePtr != NULL);
      return Status;
    }
  }

  if (!CompareGuid (&(FitHobStructurePtr->Header.Name), &gFitStructureHobGuid)) {
    DEBUG ((DEBUG_ERROR, "FIT HOB signature is invalid!\n"));
    return EFI_DEVICE_ERROR;
  }

  *FitHobStructure = FitHobStructurePtr;

  return EFI_SUCCESS;
}

/**
  Returns a pointer to the FIT HOB if it exists.

  @param[out]   FitHobStructure             A pointer to a pointer to the FIT_HOB_STRUCTURE. The pointer will be
                                            udpdated to point to a FIT_HOB_STRUCTURE instance if the FIT HOB
                                            can be located.

  @retval       EFI_SUCCESS                 The FIT HOB structure was located and returned successfully.
  @retval       EFI_NOT_FOUND               The gFitStructureHobGuid was not found.
**/
EFI_STATUS
EFIAPI
GetFitHob (
  OUT FIT_HOB_STRUCTURE **FitHobStructure
  )
{
  FIT_HOB_STRUCTURE   *FitHobStructurePtr;

  FitHobStructurePtr = GetFirstGuidHob (&gFitStructureHobGuid);
  if (FitHobStructurePtr != NULL) {
    *FitHobStructure = FitHobStructurePtr;
  } else {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Determines the Firmware Interface Table (FIT) base address.

  @param[out]   FitBase           The FIT base address.

  @retval       EFI_SUCCESS       The operation completed successfully.
  @retval       EFI_NOT_FOUND     The gNemMapStructureHobGuid was not found
  @retval       EFI_NOT_FOUND     The FIT table could not be found (or is no longer available)
  @retval       EFI_DEVICE_ERROR  The GUID in the NEM map structure is invalid.
**/
EFI_STATUS
EFIAPI
GetFitBase (
  OUT UINTN    *FitBase
  )
{
  FIRMWARE_INTERFACE_TABLE_ENTRY  *FitEntry;
  UINTN                           FitBaseAddress;

  FitBaseAddress = *(UINTN *) (UINTN) PcdGet64 (PcdFitPointerAddress);

  FitEntry = (FIRMWARE_INTERFACE_TABLE_ENTRY *) FitBaseAddress;
  if (FitEntry[0].Address != *(UINT64 *) "_FIT_   ") {
    return EFI_NOT_FOUND;
  } else {
    *FitBase = FitBaseAddress;
  }

  return EFI_SUCCESS;
}
