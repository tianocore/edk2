/** @file
  HII Config Access protocol implementation for PQC Transition Configuration.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PqcTransitionDxe.h"

/**
  HII Config Access Protocol ExtractConfig function.

  @param[in]  This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request           A null-terminated Unicode string in <ConfigRequest> format.
  @param[out] Progress          On return, points to a character in the Request string.
  @param[out] Results           A null-terminated Unicode string in <ConfigAltResp> format.

  @retval EFI_SUCCESS           The Results is filled with the requested values.
  @retval Others                Error occurred.
**/
EFI_STATUS
EFIAPI
PqcTransitionExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  PQC_TRANSITION_PRIVATE_DATA     *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gPqcTransitionConfigFormSetGuid, NULL)) {
    return EFI_NOT_FOUND;
  }

  PrivateData = PQC_TRANSITION_PRIVATE_DATA_FROM_THIS (This);

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof (PQC_TRANSITION_CONFIGURATION);
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               Request,
                               (UINT8 *) &PrivateData->Configuration,
                               BufferSize,
                               Results,
                               Progress
                               );

  return Status;
}

/**
  HII Config Access Protocol RouteConfig function.

  @param[in]  This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration     A null-terminated Unicode string in <ConfigResp> format.
  @param[out] Progress          A pointer to a string filled in with the offset of
                                the most recent '&' before the first failing
                                name/value pair.

  @retval EFI_SUCCESS           The Results is processed successfully.
  @retval Others                Error occurred.
**/
EFI_STATUS
EFIAPI
PqcTransitionRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  PQC_TRANSITION_PRIVATE_DATA     *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PQC_TRANSITION_PRIVATE_DATA_FROM_THIS (This);

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (PQC_TRANSITION_CONFIGURATION);
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               Configuration,
                               (UINT8 *) &PrivateData->Configuration,
                               &BufferSize,
                               Progress
                               );

  return Status;
}

/**
  HII Config Access Protocol Callback function.

  @param[in]  This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action            Specifies the type of action taken by the browser.
  @param[in]  QuestionId        A unique value which is sent to the original
                                exporting driver so that it can identify the type
                                of data to expect.
  @param[in]  Type              The type of value for the question.
  @param[in]  Value             A pointer to the data being sent to the original
                                exporting driver.
  @param[out] ActionRequest     On return, points to the action requested by the
                                callback function.

  @retval EFI_SUCCESS           The callback successfully handled the action.
  @retval Others                Error occurred.
**/
EFI_STATUS
EFIAPI
PqcTransitionCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS                   Status;
  PQC_TRANSITION_PRIVATE_DATA  *PrivateData;
  PQC_TRANSITION_MODE          NewMode;

  if (((Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN) && 
       (Action != EFI_BROWSER_ACTION_FORM_CLOSE)) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PQC_TRANSITION_PRIVATE_DATA_FROM_THIS (This);
  Status = EFI_SUCCESS;
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

  switch (Action) {
    case EFI_BROWSER_ACTION_FORM_OPEN:
      //
      // Refresh readiness status when form opens
      //
      if (QuestionId == KEY_PQC_READINESS_CHECK) {
        Status = CheckPqcReadiness (&PrivateData->ReadinessStatus);
        if (!EFI_ERROR (Status)) {
          //
          // Update configuration with latest readiness status
          //
          PrivateData->Configuration.PkHasPqcCert = PrivateData->ReadinessStatus.PkHasPqcCert ? 1 : 0;
          PrivateData->Configuration.KekHasPqcCert = PrivateData->ReadinessStatus.KekHasPqcCert ? 1 : 0;
          PrivateData->Configuration.DbHasPqcCert = PrivateData->ReadinessStatus.DbHasPqcCert ? 1 : 0;
          PrivateData->Configuration.SystemReadyForPqc = PrivateData->ReadinessStatus.SystemReadyForPqc ? 1 : 0;
        }
      }
      break;

    case EFI_BROWSER_ACTION_CHANGING:
      switch (QuestionId) {
        case KEY_PQC_TRANSITION_MODE:
          NewMode = (PQC_TRANSITION_MODE) Value->u8;
          
          //
          // Check if switching to PQC-only mode
          //
          if (NewMode == PqcTransitionModePqcOnly) {
            if (!PrivateData->ReadinessStatus.SystemReadyForPqc) {
              //
              // System is not ready for PQC-only mode
              //
              DEBUG ((DEBUG_WARN, "System is not ready for PQC-only mode transition\n"));
              
              //
              // TODO: Display warning message to user
              // For now, reject the change
              //
              Status = EFI_ACCESS_DENIED;
              break;
            }
          }
          
          //
          // Perform the mode switch
          //
          Status = SwitchPqcTransitionMode (NewMode);
          if (!EFI_ERROR (Status)) {
            PrivateData->Configuration.PqcTransitionMode = (UINT8) NewMode;
            *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
          }
          break;

        case KEY_PQC_READINESS_CHECK:
          //
          // Perform readiness check
          //
          Status = CheckPqcReadiness (&PrivateData->ReadinessStatus);
          if (!EFI_ERROR (Status)) {
            //
            // Update configuration with readiness status
            //
            PrivateData->Configuration.PkHasPqcCert = PrivateData->ReadinessStatus.PkHasPqcCert ? 1 : 0;
            PrivateData->Configuration.KekHasPqcCert = PrivateData->ReadinessStatus.KekHasPqcCert ? 1 : 0;
            PrivateData->Configuration.DbHasPqcCert = PrivateData->ReadinessStatus.DbHasPqcCert ? 1 : 0;
            PrivateData->Configuration.SystemReadyForPqc = PrivateData->ReadinessStatus.SystemReadyForPqc ? 1 : 0;
            
            *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
          }
          break;

        case KEY_PQC_FORCE_SWITCH:
          //
          // Force switch to PQC-only mode (bypass readiness check)
          //
          if (Value->b) {
            Status = SwitchPqcTransitionMode (PqcTransitionModePqcOnly);
            if (!EFI_ERROR (Status)) {
              PrivateData->Configuration.PqcTransitionMode = PQC_MODE_PQC_ONLY;
              PrivateData->Configuration.ForceSwitch = 1;
              *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
            }
          }
          break;

        case KEY_PQC_CLEANUP_TRADITIONAL:
          //
          // Clean up traditional algorithms
          //
          if (Value->b) {
            Status = CleanupTraditionalAlgorithms ();
            if (!EFI_ERROR (Status)) {
              PrivateData->Configuration.CleanupTraditional = 1;
              *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
            }
          }
          break;

        case KEY_PQC_RECOVERY_MODE:
          //
          // Enable recovery mode
          //
          PrivateData->Configuration.RecoveryMode = Value->b ? 1 : 0;
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
          break;

        default:
          break;
      }
      break;

    default:
      break;
  }

  return Status;
}