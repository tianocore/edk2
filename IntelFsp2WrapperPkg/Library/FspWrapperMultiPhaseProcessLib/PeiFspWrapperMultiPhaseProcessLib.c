/** @file
  Support FSP Wrapper MultiPhase process.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <FspEas.h>
#include <FspGlobalData.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/Variable.h>
#include <Library/PeiServicesLib.h>
#include <Library/FspWrapperPlatformMultiPhaseLib.h>

/**
  Execute 32-bit FSP API entry code.

  @param[in] Function     The 32bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 32bit code.
  @param[in] Param2       The second parameter to pass to 32bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute32BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  );

/**
  Execute 64-bit FSP API entry code.

  @param[in] Function     The 64bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 64bit code.
  @param[in] Param2       The second parameter to pass to 64bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute64BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  );

/**
  Call FspsMultiPhase API.

  @param[in] FspsMultiPhaseParams - Parameters for MultiPhase API.
  @param[in] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in] ComponentIndex       - FSP Component which executing MultiPhase initialization.

  @return EFI_UNSUPPORTED  - the requested FspsMultiPhase API is not supported.
  @return EFI_DEVICE_ERROR - the FSP header was not found.
  @return EFI status returned by FspsMultiPhase API.
**/
EFI_STATUS
EFIAPI
CallFspMultiPhaseEntry (
  IN VOID      *FspMultiPhaseParams,
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  )
{
  FSP_INFO_HEADER  *FspHeader;
  //
  // FSP_MULTI_PHASE_INIT and FSP_MULTI_PHASE_SI_INIT API functions having same prototype.
  //
  UINTN                   FspMultiPhaseApiEntry;
  UINTN                   FspMultiPhaseApiOffset;
  EFI_STATUS              Status;
  BOOLEAN                 InterruptState;
  BOOLEAN                 IsVariableServiceRequest;
  FSP_MULTI_PHASE_PARAMS  *FspMultiPhaseParamsPtr;

  FspMultiPhaseApiOffset   = 0;
  FspMultiPhaseParamsPtr   = (FSP_MULTI_PHASE_PARAMS *)FspMultiPhaseParams;
  IsVariableServiceRequest = FALSE;
  if ((FspMultiPhaseParamsPtr->MultiPhaseAction == EnumMultiPhaseGetVariableRequestInfo) ||
      (FspMultiPhaseParamsPtr->MultiPhaseAction == EnumMultiPhaseCompleteVariableRequest))
  {
    IsVariableServiceRequest = TRUE;
  }

  if (ComponentIndex == FspMultiPhaseMemInitApiIndex) {
    FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspmBaseAddress));
    if (FspHeader == NULL) {
      return EFI_DEVICE_ERROR;
    } else if (FspHeader->SpecVersion < 0x24) {
      return EFI_UNSUPPORTED;
    }

    FspMultiPhaseApiOffset = FspHeader->FspMultiPhaseMemInitEntryOffset;
  } else if (ComponentIndex == FspMultiPhaseSiInitApiIndex) {
    FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspsBaseAddress));
    if (FspHeader == NULL) {
      return EFI_DEVICE_ERROR;
    } else if (FspHeader->SpecVersion < 0x22) {
      return EFI_UNSUPPORTED;
    } else if ((FspHeader->SpecVersion < 0x24) && (IsVariableServiceRequest == TRUE)) {
      return EFI_UNSUPPORTED;
    }

    FspMultiPhaseApiOffset = FspHeader->FspMultiPhaseSiInitEntryOffset;
  }

  if (FspMultiPhaseApiOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  FspMultiPhaseApiEntry = FspHeader->ImageBase + FspMultiPhaseApiOffset;
  InterruptState        = SaveAndDisableInterrupts ();
  if ((FspHeader->ImageAttribute & BIT2) == 0) {
    // BIT2: IMAGE_ATTRIBUTE_64BIT_MODE_SUPPORT
    Status = Execute32BitCode ((UINTN)FspMultiPhaseApiEntry, (UINTN)FspMultiPhaseParams, (UINTN)NULL);
  } else {
    Status = Execute64BitCode ((UINTN)FspMultiPhaseApiEntry, (UINTN)FspMultiPhaseParams, (UINTN)NULL);
  }

  SetInterruptState (InterruptState);

  DEBUG ((DEBUG_ERROR, "CallFspMultiPhaseEntry return Status %r \n", Status));

  return Status;
}

/**
  FSP Wrapper Variable Request Handler

  @param[in, out] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in]      ComponentIndex       - FSP Component which executing MultiPhase initialization.

  @retval EFI_UNSUPPORTED   FSP Wrapper cannot support the specific variable request,
                            or FSP does not support VariableService
  @retval EFI_STATUS        Return FSP returned status

**/
EFI_STATUS
EFIAPI
FspWrapperVariableRequestHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  )
{
  EFI_STATUS                                        Status;
  FSP_MULTI_PHASE_PARAMS                            FspMultiPhaseParams;
  FSP_MULTI_PHASE_VARIABLE_REQUEST_INFO_PARAMS      *FspVariableRequestParams;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI                   *ReadOnlyVariablePpi;
  EDKII_PEI_VARIABLE_PPI                            *VariablePpi;
  BOOLEAN                                           WriteVariableSupport;
  FSP_MULTI_PHASE_COMPLETE_VARIABLE_REQUEST_PARAMS  CompleteVariableRequestParams;

  WriteVariableSupport = TRUE;
  Status               = PeiServicesLocatePpi (
                           &gEdkiiPeiVariablePpiGuid,
                           0,
                           NULL,
                           (VOID **)&VariablePpi
                           );
  if (EFI_ERROR (Status)) {
    WriteVariableSupport = FALSE;
    Status               = PeiServicesLocatePpi (
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                             (VOID **)&ReadOnlyVariablePpi
                             );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  Status = FSP_STATUS_VARIABLE_REQUEST;
  while (Status == FSP_STATUS_VARIABLE_REQUEST) {
    //
    // Get the variable request information from FSP.
    //
    FspMultiPhaseParams.MultiPhaseAction = EnumMultiPhaseGetVariableRequestInfo;
    FspMultiPhaseParams.PhaseIndex       = 0;
    Status                               = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);
    ASSERT_EFI_ERROR (Status);
    //
    // FSP should output this pointer for variable request information.
    //
    FspVariableRequestParams = (FSP_MULTI_PHASE_VARIABLE_REQUEST_INFO_PARAMS *)FspMultiPhaseParams.MultiPhaseParamPtr;
    switch (FspVariableRequestParams->VariableRequest) {
      case EnumFspVariableRequestGetVariable:
        if (WriteVariableSupport) {
          Status = VariablePpi->GetVariable (
                                  VariablePpi,
                                  FspVariableRequestParams->VariableName,
                                  FspVariableRequestParams->VariableGuid,
                                  FspVariableRequestParams->Attributes,
                                  (UINTN *)FspVariableRequestParams->DataSize,
                                  FspVariableRequestParams->Data
                                  );
        } else {
          Status = ReadOnlyVariablePpi->GetVariable (
                                          ReadOnlyVariablePpi,
                                          FspVariableRequestParams->VariableName,
                                          FspVariableRequestParams->VariableGuid,
                                          FspVariableRequestParams->Attributes,
                                          (UINTN *)FspVariableRequestParams->DataSize,
                                          FspVariableRequestParams->Data
                                          );
        }

        CompleteVariableRequestParams.VariableRequestStatus = Status;
        FspMultiPhaseParams.MultiPhaseParamPtr              = (VOID *)&CompleteVariableRequestParams;
        FspMultiPhaseParams.MultiPhaseAction                = EnumMultiPhaseCompleteVariableRequest;
        Status                                              = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);
        break;

      case EnumFspVariableRequestSetVariable:
        if (WriteVariableSupport) {
          Status = VariablePpi->SetVariable (
                                  VariablePpi,
                                  FspVariableRequestParams->VariableName,
                                  FspVariableRequestParams->VariableGuid,
                                  *FspVariableRequestParams->Attributes,
                                  (UINTN)*FspVariableRequestParams->DataSize,
                                  FspVariableRequestParams->Data
                                  );
        } else {
          Status = EFI_UNSUPPORTED;
        }

        CompleteVariableRequestParams.VariableRequestStatus = Status;
        FspMultiPhaseParams.MultiPhaseParamPtr              = (VOID *)&CompleteVariableRequestParams;
        FspMultiPhaseParams.MultiPhaseAction                = EnumMultiPhaseCompleteVariableRequest;
        Status                                              = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);
        break;

      case EnumFspVariableRequestGetNextVariableName:
        if (WriteVariableSupport) {
          Status = VariablePpi->GetNextVariableName (
                                  VariablePpi,
                                  (UINTN *)FspVariableRequestParams->VariableNameSize,
                                  FspVariableRequestParams->VariableName,
                                  FspVariableRequestParams->VariableGuid
                                  );
        } else {
          Status = ReadOnlyVariablePpi->NextVariableName (
                                          ReadOnlyVariablePpi,
                                          (UINTN *)FspVariableRequestParams->VariableNameSize,
                                          FspVariableRequestParams->VariableName,
                                          FspVariableRequestParams->VariableGuid
                                          );
        }

        CompleteVariableRequestParams.VariableRequestStatus = Status;
        FspMultiPhaseParams.MultiPhaseParamPtr              = (VOID *)&CompleteVariableRequestParams;
        FspMultiPhaseParams.MultiPhaseAction                = EnumMultiPhaseCompleteVariableRequest;
        Status                                              = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);
        break;

      case EnumFspVariableRequestQueryVariableInfo:
        if (WriteVariableSupport) {
          Status = VariablePpi->QueryVariableInfo (
                                  VariablePpi,
                                  *FspVariableRequestParams->Attributes,
                                  FspVariableRequestParams->MaximumVariableStorageSize,
                                  FspVariableRequestParams->RemainingVariableStorageSize,
                                  FspVariableRequestParams->MaximumVariableSize
                                  );
        } else {
          Status = EFI_UNSUPPORTED;
        }

        CompleteVariableRequestParams.VariableRequestStatus = Status;
        FspMultiPhaseParams.MultiPhaseParamPtr              = (VOID *)&CompleteVariableRequestParams;
        FspMultiPhaseParams.MultiPhaseAction                = EnumMultiPhaseCompleteVariableRequest;
        Status                                              = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);
        break;

      default:
        DEBUG ((DEBUG_ERROR, "Unknown VariableRequest type!\n"));
        Status = EFI_UNSUPPORTED;
        break;
    }
  }

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FspMultiPhaseApi-0x%x requested reset %r\n", ComponentIndex, Status));
    CallFspWrapperResetSystem ((UINTN)Status);
  }

  return Status;
}

/**
  FSP Wrapper MultiPhase Handler

  @param[in, out] FspHobListPtr        - Pointer to FSP HobList (valid after FSP-M completed)
  @param[in]      ComponentIndex       - FSP Component which executing MultiPhase initialization.

  @retval EFI_UNSUPPORTED   Specific MultiPhase action was not supported.
  @retval EFI_SUCCESS       MultiPhase action were completed successfully.

**/
EFI_STATUS
EFIAPI
FspWrapperMultiPhaseHandler (
  IN OUT VOID  **FspHobListPtr,
  IN UINT8     ComponentIndex
  )
{
  EFI_STATUS                                   Status;
  FSP_MULTI_PHASE_PARAMS                       FspMultiPhaseParams;
  FSP_MULTI_PHASE_GET_NUMBER_OF_PHASES_PARAMS  FspMultiPhaseGetNumber;
  UINT32                                       Index;
  UINT32                                       NumOfPhases;

  //
  // Query FSP for the number of phases supported.
  //
  FspMultiPhaseParams.MultiPhaseAction   = EnumMultiPhaseGetNumberOfPhases;
  FspMultiPhaseParams.PhaseIndex         = 0;
  FspMultiPhaseParams.MultiPhaseParamPtr = (VOID *)&FspMultiPhaseGetNumber;
  Status                                 = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);
  if (Status == EFI_UNSUPPORTED) {
    //
    // MultiPhase API was not supported
    //
    return Status;
  } else {
    ASSERT_EFI_ERROR (Status);
  }

  NumOfPhases = FspMultiPhaseGetNumber.NumberOfPhases;

  for (Index = 1; Index <= NumOfPhases; Index++) {
    DEBUG ((DEBUG_ERROR, "MultiPhase Index/NumOfPhases = %d of %d\n", Index, NumOfPhases));
    //
    // Platform actions can be added in below function for each component and phase before returning control back to FSP.
    //
    FspWrapperPlatformMultiPhaseHandler (FspHobListPtr, ComponentIndex, Index);

    FspMultiPhaseParams.MultiPhaseAction   = EnumMultiPhaseExecutePhase;
    FspMultiPhaseParams.PhaseIndex         = Index;
    FspMultiPhaseParams.MultiPhaseParamPtr = NULL;
    Status                                 = CallFspMultiPhaseEntry (&FspMultiPhaseParams, FspHobListPtr, ComponentIndex);

    if (Status == FSP_STATUS_VARIABLE_REQUEST) {
      //
      // call to Variable request handler
      //
      FspWrapperVariableRequestHandler (FspHobListPtr, ComponentIndex);
    }

    //
    // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
    //
    if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
      DEBUG ((DEBUG_INFO, "FspMultiPhaseApi-0x%x requested reset %r\n", ComponentIndex, Status));
      CallFspWrapperResetSystem ((UINTN)Status);
    }

    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
