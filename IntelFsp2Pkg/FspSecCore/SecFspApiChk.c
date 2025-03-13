/** @file

  Copyright (c) 2016 - 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecFsp.h"

/**
  This function check the FSP API calling condition.

  It updates the FSP API index and UPD data pointer in FSP global data
  when FspSiliconInit and FspSmmInit are called.

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
  FSP_INFO_HEADER  *FspInfoHeader;

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
      // Set "API Index" and "UPD Data Pointer" in FSP Global Data for FspSiliconInit and FspSmmInit
      // For FspMemoryInit: The global data is not valid when code runs here.
      //                    The 2 fields will be updated by SecCore after the global data is allocated in stack.
      //
      SetFspApiCallingIndex (ApiIdx);

      if (ApiParam == NULL) {
        FspInfoHeader = (FSP_INFO_HEADER *)AsmGetFspInfoHeader ();
        ApiParam      = (VOID *)(UINTN)(FspInfoHeader->ImageBase + FspInfoHeader->CfgRegionOffset);
      }

      if (ApiIdx == FspSiliconInitApiIndex) {
        SetFspSiliconInitUpdDataPointer (ApiParam);
      } else if (ApiIdx == FspSmmInitApiIndex) {
        SetFspSmmInitUpdDataPointer (ApiParam);
      }
    }
  }

  return Status;
}
