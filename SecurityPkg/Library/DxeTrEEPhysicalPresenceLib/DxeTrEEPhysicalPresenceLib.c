/** @file
  Execute pending TPM2 requests from OS or BIOS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  TrEEExecutePendingTpmRequest() will receive untrusted input and do validation.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Protocol/TrEEProtocol.h>
#include <Protocol/VariableLock.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Guid/EventGroup.h>
#include <Guid/TrEEPhysicalPresenceData.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/TrEEPpVendorLib.h>

#define CONFIRM_BUFFER_SIZE         4096

EFI_HII_HANDLE mTrEEPpStringPackHandle;

/**
  Get string by string id from HII Interface.

  @param[in] Id          String ID.

  @retval    CHAR16 *    String from ID.
  @retval    NULL        If error occurs.

**/
CHAR16 *
TrEEPhysicalPresenceGetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (mTrEEPpStringPackHandle, Id, NULL);
}

/**
  Send ClearControl and Clear command to TPM.

  @param[in]  PlatformAuth      platform auth value. NULL means no platform auth change.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
TpmCommandClear (
  IN TPM2B_AUTH                *PlatformAuth  OPTIONAL
  )
{
  EFI_STATUS                Status;
  TPMS_AUTH_COMMAND         *AuthSession;
  TPMS_AUTH_COMMAND         LocalAuthSession;

  if (PlatformAuth == NULL) {
    AuthSession = NULL;
  } else {
    AuthSession = &LocalAuthSession;
    ZeroMem (&LocalAuthSession, sizeof(LocalAuthSession));
    LocalAuthSession.sessionHandle = TPM_RS_PW;
    LocalAuthSession.hmac.size = PlatformAuth->size;
    CopyMem (LocalAuthSession.hmac.buffer, PlatformAuth->buffer, PlatformAuth->size);
  }

  DEBUG ((EFI_D_INFO, "Tpm2ClearControl ... \n"));
  Status = Tpm2ClearControl (TPM_RH_PLATFORM, AuthSession, NO);
  DEBUG ((EFI_D_INFO, "Tpm2ClearControl - %r\n", Status));
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  DEBUG ((EFI_D_INFO, "Tpm2Clear ... \n"));
  Status = Tpm2Clear (TPM_RH_PLATFORM, AuthSession);
  DEBUG ((EFI_D_INFO, "Tpm2Clear - %r\n", Status));

Done:
  ZeroMem (&LocalAuthSession.hmac, sizeof(LocalAuthSession.hmac));
  return Status;
}

/**
  Execute physical presence operation requested by the OS.

  @param[in]      PlatformAuth        platform auth value. NULL means no platform auth change.
  @param[in]      CommandCode         Physical presence operation value.
  @param[in, out] PpiFlags            The physical presence interface flags.
  
  @retval TREE_PP_OPERATION_RESPONSE_BIOS_FAILURE  Unknown physical presence operation.
  @retval TREE_PP_OPERATION_RESPONSE_BIOS_FAILURE  Error occurred during sending command to TPM or 
                                                   receiving response from TPM.
  @retval Others                                   Return code from the TPM device after command execution.
**/
UINT32
TrEEExecutePhysicalPresence (
  IN      TPM2B_AUTH                       *PlatformAuth,  OPTIONAL
  IN      UINT32                           CommandCode,
  IN OUT  EFI_TREE_PHYSICAL_PRESENCE_FLAGS *PpiFlags
  )
{
  EFI_STATUS  Status;

  switch (CommandCode) {
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_2:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_3:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_4:
      Status = TpmCommandClear (PlatformAuth);
      if (EFI_ERROR (Status)) {
        return TREE_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      } else {
        return TREE_PP_OPERATION_RESPONSE_SUCCESS;
      }

    case TREE_PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_FALSE:
      PpiFlags->PPFlags &= ~TREE_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR;
      return TREE_PP_OPERATION_RESPONSE_SUCCESS;

    case TREE_PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE:
      PpiFlags->PPFlags |= TREE_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR;
      return TREE_PP_OPERATION_RESPONSE_SUCCESS;

    default:
      if (CommandCode <= TREE_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
        return TREE_PP_OPERATION_RESPONSE_SUCCESS;
      } else {
        return TREE_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      }
  }
}


/**
  Read the specified key for user confirmation.

  @param[in]  CautionKey  If true,  F12 is used as confirm key;
                          If false, F10 is used as confirm key.

  @retval     TRUE        User confirmed the changes by input.
  @retval     FALSE       User discarded the changes.
**/
BOOLEAN
TrEEReadUserKey (
  IN     BOOLEAN                    CautionKey
  )
{
  EFI_STATUS                        Status;
  EFI_INPUT_KEY                     Key;
  UINT16                            InputKey;
      
  InputKey = 0; 
  do {
    Status = gBS->CheckEvent (gST->ConIn->WaitForKey);
    if (!EFI_ERROR (Status)) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (Key.ScanCode == SCAN_ESC) {
        InputKey = Key.ScanCode;
      }
      if ((Key.ScanCode == SCAN_F10) && !CautionKey) {
        InputKey = Key.ScanCode;
      }
      if ((Key.ScanCode == SCAN_F12) && CautionKey) {
        InputKey = Key.ScanCode;
      }
    }      
  } while (InputKey == 0);

  if (InputKey != SCAN_ESC) {
    return TRUE;
  }
  
  return FALSE;
}

/**
  The constructor function register UNI strings into imageHandle.
  
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS. 

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor successfully added string package.
  @retval Other value   The constructor can't add string package.
**/
EFI_STATUS
EFIAPI
TrEEPhysicalPresenceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mTrEEPpStringPackHandle = HiiAddPackages (&gEfiTrEEPhysicalPresenceGuid, ImageHandle, DxeTrEEPhysicalPresenceLibStrings, NULL);
  ASSERT (mTrEEPpStringPackHandle != NULL);

  return EFI_SUCCESS;
}

/**
  Display the confirm text and get user confirmation.

  @param[in] TpmPpCommand  The requested TPM physical presence command.

  @retval    TRUE          The user has confirmed the changes.
  @retval    FALSE         The user doesn't confirm the changes.
**/
BOOLEAN
TrEEUserConfirm (
  IN      UINT32                    TpmPpCommand
  )
{
  CHAR16                            *ConfirmText;
  CHAR16                            *TmpStr1;
  CHAR16                            *TmpStr2; 
  UINTN                             BufSize;
  BOOLEAN                           CautionKey;
  UINT16                            Index;
  CHAR16                            DstStr[81];
    
  TmpStr2     = NULL;
  CautionKey  = FALSE;
  BufSize     = CONFIRM_BUFFER_SIZE;
  ConfirmText = AllocateZeroPool (BufSize);
  ASSERT (ConfirmText != NULL);

  switch (TpmPpCommand) {

    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_2:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_3:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_4:
      CautionKey = TRUE;
      TmpStr2 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_CLEAR));

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n\n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);      

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case TREE_PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE:
      CautionKey = TRUE;
      TmpStr2 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_CLEAR));

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_PPI_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n\n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1); 

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_NO_PPI_INFO));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    default:
      ;
  }

  if (TmpStr2 == NULL) {
    FreePool (ConfirmText);
    return FALSE;
  }

  TmpStr1 = TrEEPhysicalPresenceGetStringById (STRING_TOKEN (TPM_REJECT_KEY));
  BufSize -= StrSize (ConfirmText);
  UnicodeSPrint (ConfirmText + StrLen (ConfirmText), BufSize, TmpStr1, TmpStr2);

  DstStr[80] = L'\0';
  for (Index = 0; Index < StrLen (ConfirmText); Index += 80) {
    StrnCpyS(DstStr, sizeof (DstStr) / sizeof (CHAR16), ConfirmText + Index, sizeof (DstStr) / sizeof (CHAR16) - 1);    
    Print (DstStr);    
  }
  
  FreePool (TmpStr1);
  FreePool (TmpStr2);
  FreePool (ConfirmText);

  if (TrEEReadUserKey (CautionKey)) {
    return TRUE;
  }

  return FALSE;  
}

/**
  Check if there is a valid physical presence command request. Also updates parameter value 
  to whether the requested physical presence command already confirmed by user
 
   @param[in]  TcgPpData                 EFI TrEE Physical Presence request data. 
   @param[in]  Flags                     The physical presence interface flags.
   @param[out] RequestConfirmed            If the physical presence operation command required user confirm from UI.
                                             True, it indicates the command doesn't require user confirm, or already confirmed 
                                                   in last boot cycle by user.
                                             False, it indicates the command need user confirm from UI.

   @retval  TRUE        Physical Presence operation command is valid.
   @retval  FALSE       Physical Presence operation command is invalid.

**/
BOOLEAN
TrEEHaveValidTpmRequest  (
  IN      EFI_TREE_PHYSICAL_PRESENCE       *TcgPpData,
  IN      EFI_TREE_PHYSICAL_PRESENCE_FLAGS Flags,
  OUT     BOOLEAN                          *RequestConfirmed
  )
{
  BOOLEAN  IsRequestValid;

  *RequestConfirmed = FALSE;

  switch (TcgPpData->PPRequest) {
    case TREE_PHYSICAL_PRESENCE_NO_ACTION:
      *RequestConfirmed = TRUE;
      return TRUE;
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_2:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_3:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_4:
      if ((Flags.PPFlags & TREE_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR) != 0) {
        *RequestConfirmed = TRUE;
      }
      break;

    case TREE_PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_FALSE:
      *RequestConfirmed = TRUE;
      break;

    case TREE_PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE:
      break;

    default:
      if (TcgPpData->PPRequest >= TREE_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
        IsRequestValid = TrEEPpVendorLibHasValidRequest (TcgPpData->PPRequest, Flags.PPFlags, RequestConfirmed);
        if (!IsRequestValid) {
          return FALSE;
        } else {
          break;
        }
      } else {
        //
        // Wrong Physical Presence command
        //
        return FALSE;
      }
  }

  if ((Flags.PPFlags & TREE_VENDOR_LIB_FLAG_RESET_TRACK) != 0) {
    //
    // It had been confirmed in last boot, it doesn't need confirm again.
    //
    *RequestConfirmed = TRUE;
  }

  //
  // Physical Presence command is correct
  //
  return TRUE;
}


/**
  Check and execute the requested physical presence command.

  Caution: This function may receive untrusted input.
  TcgPpData variable is external input, so this function will validate
  its data structure to be valid value.

  @param[in] PlatformAuth         platform auth value. NULL means no platform auth change.
  @param[in] TcgPpData            Point to the physical presence NV variable.
  @param[in] Flags                The physical presence interface flags.
**/
VOID
TrEEExecutePendingTpmRequest (
  IN      TPM2B_AUTH                       *PlatformAuth,  OPTIONAL
  IN      EFI_TREE_PHYSICAL_PRESENCE       *TcgPpData,
  IN      EFI_TREE_PHYSICAL_PRESENCE_FLAGS Flags
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  BOOLEAN                           RequestConfirmed;
  EFI_TREE_PHYSICAL_PRESENCE_FLAGS  NewFlags;
  BOOLEAN                           ResetRequired;
  UINT32                            NewPPFlags;

  if (TcgPpData->PPRequest == TREE_PHYSICAL_PRESENCE_NO_ACTION) {
    //
    // No operation request
    //
    return;
  }

  if (!TrEEHaveValidTpmRequest(TcgPpData, Flags, &RequestConfirmed)) {
    //
    // Invalid operation request.
    //
    if (TcgPpData->PPRequest <= TREE_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
      TcgPpData->PPResponse = TREE_PP_OPERATION_RESPONSE_SUCCESS;
    } else {
      TcgPpData->PPResponse = TREE_PP_OPERATION_RESPONSE_BIOS_FAILURE;
    }
    TcgPpData->LastPPRequest = TcgPpData->PPRequest;
    TcgPpData->PPRequest = TREE_PHYSICAL_PRESENCE_NO_ACTION;
    DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE);
    Status = gRT->SetVariable (
                    TREE_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTrEEPhysicalPresenceGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize,
                    TcgPpData
                    );
    return;
  }

  ResetRequired = FALSE;
  if (TcgPpData->PPRequest >= TREE_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
    NewFlags = Flags;
    NewPPFlags = NewFlags.PPFlags;
    TcgPpData->PPResponse = TrEEPpVendorLibExecutePendingRequest (PlatformAuth, TcgPpData->PPRequest, &NewPPFlags, &ResetRequired);
    NewFlags.PPFlags = (UINT8)NewPPFlags;
  } else {
    if (!RequestConfirmed) {
      //
      // Print confirm text and wait for approval. 
      //
      RequestConfirmed = TrEEUserConfirm (TcgPpData->PPRequest
                                          );
    }

    //
    // Execute requested physical presence command
    //
    TcgPpData->PPResponse = TREE_PP_OPERATION_RESPONSE_USER_ABORT;
    NewFlags = Flags;
    if (RequestConfirmed) {
      TcgPpData->PPResponse = TrEEExecutePhysicalPresence (PlatformAuth, TcgPpData->PPRequest, 
                                                           &NewFlags);
    }
  }

  //
  // Save the flags if it is updated.
  //
  if (CompareMem (&Flags, &NewFlags, sizeof(EFI_TREE_PHYSICAL_PRESENCE_FLAGS)) != 0) {
    Status   = gRT->SetVariable (
                      TREE_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                      &gEfiTrEEPhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_TREE_PHYSICAL_PRESENCE_FLAGS),
                      &NewFlags
                      ); 
  }

  //
  // Clear request
  //
  if ((NewFlags.PPFlags & TREE_VENDOR_LIB_FLAG_RESET_TRACK) == 0) {
    TcgPpData->LastPPRequest = TcgPpData->PPRequest;
    TcgPpData->PPRequest = TREE_PHYSICAL_PRESENCE_NO_ACTION;    
  }

  //
  // Save changes
  //
  DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE);
  Status = gRT->SetVariable (
                  TREE_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTrEEPhysicalPresenceGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  DataSize,
                  TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (TcgPpData->PPResponse == TREE_PP_OPERATION_RESPONSE_USER_ABORT) {
    return;
  }

  //
  // Reset system to make new TPM settings in effect
  //
  switch (TcgPpData->LastPPRequest) {
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_2:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_3:
    case TREE_PHYSICAL_PRESENCE_CLEAR_CONTROL_CLEAR_4:
      break;
    default:
      if (TcgPpData->LastPPRequest >= TREE_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
        if (ResetRequired) {
          break;
        } else {
          return ;
        }
      }
      if (TcgPpData->PPRequest != TREE_PHYSICAL_PRESENCE_NO_ACTION) {
        break;
      }
      return;
  }

  Print (L"Rebooting system to make TPM2 settings in effect\n");
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  ASSERT (FALSE);  
}

/**
  Check and execute the pending TPM request.

  The TPM request may come from OS or BIOS. This API will display request information and wait 
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to 
  take effect.
  
  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request.  

  @param[in]  PlatformAuth                   platform auth value. NULL means no platform auth change.
**/
VOID
EFIAPI
TrEEPhysicalPresenceLibProcessRequest (
  IN      TPM2B_AUTH                     *PlatformAuth  OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_TREE_PHYSICAL_PRESENCE        TcgPpData;
  EFI_TREE_PROTOCOL                 *TreeProtocol;
  EDKII_VARIABLE_LOCK_PROTOCOL      *VariableLockProtocol;
  EFI_TREE_PHYSICAL_PRESENCE_FLAGS  PpiFlags;

  Status = gBS->LocateProtocol (&gEfiTrEEProtocolGuid, NULL, (VOID **) &TreeProtocol);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Initialize physical presence flags.
  //
  DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE_FLAGS);
  Status = gRT->GetVariable (
                  TREE_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                  &gEfiTrEEPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    PpiFlags.PPFlags = 0;
    Status   = gRT->SetVariable (
                      TREE_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                      &gEfiTrEEPhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_TREE_PHYSICAL_PRESENCE_FLAGS),
                      &PpiFlags
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[TPM2] Set physical presence flag failed, Status = %r\n", Status));
      return ;
    }
  }
  DEBUG ((EFI_D_INFO, "[TPM2] PpiFlags = %x\n", PpiFlags.PPFlags));

  //
  // This flags variable controls whether physical presence is required for TPM command. 
  // It should be protected from malicious software. We set it as read-only variable here.
  //
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VariableLockProtocol);
  if (!EFI_ERROR (Status)) {
    Status = VariableLockProtocol->RequestToLock (
                                     VariableLockProtocol,
                                     TREE_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                                     &gEfiTrEEPhysicalPresenceGuid
                                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[TPM2] Error when lock variable %s, Status = %r\n", TREE_PHYSICAL_PRESENCE_FLAGS_VARIABLE, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }
  
  //
  // Initialize physical presence variable.
  //
  DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TREE_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTrEEPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    ZeroMem ((VOID*)&TcgPpData, sizeof (TcgPpData));
    DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE);
    Status   = gRT->SetVariable (
                      TREE_PHYSICAL_PRESENCE_VARIABLE,
                      &gEfiTrEEPhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      DataSize,
                      &TcgPpData
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[TPM2] Set physical presence variable failed, Status = %r\n", Status));
      return ;
    }
  }

  DEBUG ((EFI_D_INFO, "[TPM2] Flags=%x, PPRequest=%x (LastPPRequest=%x)\n", PpiFlags.PPFlags, TcgPpData.PPRequest, TcgPpData.LastPPRequest));

  //
  // Execute pending TPM request.
  //  
  TrEEExecutePendingTpmRequest (PlatformAuth, &TcgPpData, PpiFlags);
  DEBUG ((EFI_D_INFO, "[TPM2] PPResponse = %x (LastPPRequest=%x, Flags=%x)\n", TcgPpData.PPResponse, TcgPpData.LastPPRequest, PpiFlags.PPFlags));

}

/**
  Check if the pending TPM request needs user input to confirm.

  The TPM request may come from OS. This API will check if TPM request exists and need user
  input to confirmation.
  
  @retval    TRUE        TPM needs input to confirm user physical presence.
  @retval    FALSE       TPM doesn't need input to confirm user physical presence.

**/
BOOLEAN
EFIAPI
TrEEPhysicalPresenceLibNeedUserConfirm(
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_TREE_PHYSICAL_PRESENCE        TcgPpData;
  UINTN                             DataSize;
  BOOLEAN                           RequestConfirmed;
  EFI_TREE_PROTOCOL                 *TreeProtocol;
  EFI_TREE_PHYSICAL_PRESENCE_FLAGS  PpiFlags;

  Status = gBS->LocateProtocol (&gEfiTrEEProtocolGuid, NULL, (VOID **) &TreeProtocol);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Check Tpm requests
  //
  DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TREE_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTrEEPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE_FLAGS);
  Status = gRT->GetVariable (
                  TREE_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                  &gEfiTrEEPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  
  if (TcgPpData.PPRequest == TREE_PHYSICAL_PRESENCE_NO_ACTION) {
    //
    // No operation request
    //
    return FALSE;
  }

  if (!TrEEHaveValidTpmRequest(&TcgPpData, PpiFlags, &RequestConfirmed)) {
    //
    // Invalid operation request.
    //
    return FALSE;
  }

  if (!RequestConfirmed) {
    //
    // Need UI to confirm
    //
    return TRUE;
  }

  return FALSE;
}

