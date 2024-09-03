/** @file

  Copyright (c) 2016 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecFsp.h"

/**
  This function check the FSP API calling condition.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspApiCallingCheck (
  IN UINT8  ApiIdx,
  IN VOID   *ApiParam
  )
{
  EFI_STATUS       Status;
  FSP_GLOBAL_DATA  *FspData;

  Status  = EFI_SUCCESS;
  FspData = GetFspGlobalDataPointer ();

  if (ApiIdx == NotifyPhaseApiIndex) {
    //
    // NotifyPhase check
    //
    if ((FspData == NULL) || ((UINT32)(UINTN)FspData == MAX_UINT32)) {
      Status = EFI_UNSUPPORTED;
    } else {
      if (FspData->Signature != FSP_GLOBAL_DATA_SIGNATURE) {
        Status = EFI_UNSUPPORTED;
      }
    }
  } else if (ApiIdx == FspMemoryInitApiIndex) {
    //
    // FspMemoryInit check
    //
    if ((UINT32)(UINTN)FspData != MAX_UINT32) {
      Status = EFI_UNSUPPORTED;
    } else if (ApiParam == NULL) {
      Status = EFI_SUCCESS;
    } else if (EFI_ERROR (FspUpdSignatureCheck (ApiIdx, ApiParam))) {
      Status = EFI_INVALID_PARAMETER;
    }
  } else if (ApiIdx == TempRamExitApiIndex) {
    //
    // TempRamExit check
    //
    if ((FspData == NULL) || ((UINT32)(UINTN)FspData == MAX_UINT32)) {
      Status = EFI_UNSUPPORTED;
    } else {
      if (FspData->Signature != FSP_GLOBAL_DATA_SIGNATURE) {
        Status = EFI_UNSUPPORTED;
      }
    }
  } else if ((ApiIdx == FspSiliconInitApiIndex) || (ApiIdx == FspMultiPhaseSiInitApiIndex)) {
    //
    // FspSiliconInit check
    //
    if ((FspData == NULL) || ((UINT32)(UINTN)FspData == MAX_UINT32)) {
      Status = EFI_UNSUPPORTED;
    } else {
      if (FspData->Signature != FSP_GLOBAL_DATA_SIGNATURE) {
        Status = EFI_UNSUPPORTED;
      } else if (ApiIdx == FspSiliconInitApiIndex) {
        if (ApiParam == NULL) {
          Status = EFI_SUCCESS;
        } else if (EFI_ERROR (FspUpdSignatureCheck (FspSiliconInitApiIndex, ApiParam))) {
          Status = EFI_INVALID_PARAMETER;
        }

        //
        // Reset MultiPhase NumberOfPhases to zero
        //
        FspData->NumberOfPhases = 0;
      }
    }
  } else if (ApiIdx == FspMultiPhaseMemInitApiIndex) {
    if ((FspData == NULL) || ((UINT32)(UINTN)FspData == MAX_UINT32)) {
      Status = EFI_UNSUPPORTED;
    }
  } else if (ApiIdx == FspSmmInitApiIndex) {
    //
    // FspSmmInitApiIndex check
    //
    if ((FspData == NULL) || ((UINT32)(UINTN)FspData == MAX_UINT32)) {
      Status = EFI_UNSUPPORTED;
    } else {
      if (FspData->Signature != FSP_GLOBAL_DATA_SIGNATURE) {
        Status = EFI_UNSUPPORTED;
      } else if (ApiParam == NULL) {
        Status = EFI_SUCCESS;
      } else if (EFI_ERROR (FspUpdSignatureCheck (FspSmmInitApiIndex, ApiParam))) {
        Status = EFI_INVALID_PARAMETER;
      }
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (!EFI_ERROR (Status)) {
    if ((ApiIdx != FspMemoryInitApiIndex)) {
      //
      // For FspMemoryInit, the global data is not valid yet
      // The API index will be updated by SecCore after the global data
      // is initialized
      //
      SetFspApiCallingIndex (ApiIdx);
    }
  }

  return Status;
}
