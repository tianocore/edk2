/** @file
  Variable Flash Information Library

  Copyright (c) Microsoft Corporation<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Guid/VariableFlashInfo.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/VariableFlashInfoLib.h>

/**
  Get the HOB that contains variable flash information.

  @param[out] VariableFlashInfo   Pointer to a pointer to set to the variable flash information structure.

  @retval EFI_SUCCESS             Variable flash information was found successfully.
  @retval EFI_INVALID_PARAMETER   The VariableFlashInfo pointer given is NULL.
  @retval EFI_NOT_FOUND           Variable flash information could not be found.

**/
STATIC
EFI_STATUS
GetVariableFlashInfoFromHob (
  OUT VARIABLE_FLASH_INFO  **VariableFlashInfo
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  if (VariableFlashInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  GuidHob = GetFirstGuidHob (&gVariableFlashInfoHobGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  *VariableFlashInfo = GET_GUID_HOB_DATA (GuidHob);

  //
  // Assert if more than one variable flash information HOB is present.
  //
  DEBUG_CODE (
    if ((GetNextGuidHob (&gVariableFlashInfoHobGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Found two variable flash information HOBs\n"));
    ASSERT (FALSE);
  }

    );

  return EFI_SUCCESS;
}

/**
  Get the base address and size for the NV storage area used for UEFI variable storage.

  @param[out] BaseAddress    The NV storage base address.
  @param[out] Length         The NV storage length in bytes.

  @retval EFI_SUCCESS             NV storage information was found successfully.
  @retval EFI_INVALID_PARAMETER   A required pointer parameter is NULL.

**/
EFI_STATUS
EFIAPI
GetVariableFlashNvStorageInfo (
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length
  )
{
  EFI_STATUS           Status;
  VARIABLE_FLASH_INFO  *VariableFlashInfo;

  if ((BaseAddress == NULL) || (Length == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetVariableFlashInfoFromHob (&VariableFlashInfo);
  if (!EFI_ERROR (Status)) {
    *BaseAddress = VariableFlashInfo->NvVariableBaseAddress;
    *Length      = VariableFlashInfo->NvVariableLength;
  } else {
    *BaseAddress = (EFI_PHYSICAL_ADDRESS)(PcdGet64 (PcdFlashNvStorageVariableBase64) != 0 ?
                                          PcdGet64 (PcdFlashNvStorageVariableBase64) :
                                          PcdGet32 (PcdFlashNvStorageVariableBase)
                                          );
    *Length = (UINT64)PcdGet32 (PcdFlashNvStorageVariableSize);
  }

  return EFI_SUCCESS;
}

/**
  Get the base address and size for the fault tolerant write (FTW) spare
  area used for UEFI variable storage.

  @param[out] BaseAddress    The FTW spare base address.
  @param[out] Length         The FTW spare length in bytes.

  @retval EFI_SUCCESS             FTW spare information was found successfully.
  @retval EFI_INVALID_PARAMETER   A required pointer parameter is NULL.
  @retval EFI_NOT_FOUND           FTW spare information could not be found.

**/
EFI_STATUS
EFIAPI
GetVariableFlashFtwSpareInfo (
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length
  )
{
  EFI_STATUS           Status;
  VARIABLE_FLASH_INFO  *VariableFlashInfo;

  if ((BaseAddress == NULL) || (Length == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetVariableFlashInfoFromHob (&VariableFlashInfo);
  if (!EFI_ERROR (Status)) {
    *BaseAddress = VariableFlashInfo->FtwSpareBaseAddress;
    *Length      = VariableFlashInfo->FtwSpareLength;
  } else {
    *BaseAddress = (EFI_PHYSICAL_ADDRESS)(PcdGet64 (PcdFlashNvStorageFtwSpareBase64) != 0 ?
                                          PcdGet64 (PcdFlashNvStorageFtwSpareBase64) :
                                          PcdGet32 (PcdFlashNvStorageFtwSpareBase)
                                          );
    *Length = (UINT64)PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  }

  return EFI_SUCCESS;
}

/**
  Get the base address and size for the fault tolerant write (FTW) working
  area used for UEFI variable storage.

  @param[out] BaseAddress    The FTW working area base address.
  @param[out] Length         The FTW working area length in bytes.

  @retval EFI_SUCCESS             FTW working information was found successfully.
  @retval EFI_INVALID_PARAMETER   A required pointer parameter is NULL.
  @retval EFI_NOT_FOUND           FTW working information could not be found.

**/
EFI_STATUS
EFIAPI
GetVariableFlashFtwWorkingInfo (
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length
  )
{
  EFI_STATUS           Status;
  VARIABLE_FLASH_INFO  *VariableFlashInfo;

  if ((BaseAddress == NULL) || (Length == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetVariableFlashInfoFromHob (&VariableFlashInfo);
  if (!EFI_ERROR (Status)) {
    *BaseAddress = VariableFlashInfo->FtwWorkingBaseAddress;
    *Length      = VariableFlashInfo->FtwWorkingLength;
  } else {
    *BaseAddress = (EFI_PHYSICAL_ADDRESS)(PcdGet64 (PcdFlashNvStorageFtwWorkingBase64) != 0 ?
                                          PcdGet64 (PcdFlashNvStorageFtwWorkingBase64) :
                                          PcdGet32 (PcdFlashNvStorageFtwWorkingBase)
                                          );
    *Length = (UINT64)PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  }

  return EFI_SUCCESS;
}
