/** @file
  This driver checks whether there is pending TPM request. If yes, 
  it will display TPM request information and ask for user confirmation.
  The TPM request will be cleared after it is processed.  
  
Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PhysicalPresence.h"

EFI_HII_HANDLE mPpStringPackHandle;

/**
  Get TPM physical presence permanent flags.

  @param[out] LifetimeLock  Returns physicalPresenceLifetimeLock permanent flag.  
  @param[out] CmdEnable     Returns physicalPresenceCMDEnable permanent flag.
  
  @retval EFI_SUCCESS       Flags were returns successfully.
  @retval other             Failed to locate EFI TCG Protocol.

**/
EFI_STATUS
GetTpmCapability (
  OUT  BOOLEAN                      *LifetimeLock,
  OUT  BOOLEAN                      *CmdEnable
  )
{
  EFI_STATUS                        Status;
  EFI_TCG_PROTOCOL                  *TcgProtocol;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_RSP_COMMAND_HDR               *TpmRsp;
  UINT32                            *SendBufPtr;
  UINT8                             SendBuffer[sizeof (*TpmRqu) + sizeof (UINT32) * 3];
  TPM_PERMANENT_FLAGS               *TpmPermanentFlags;
  UINT8                             RecvBuffer[40];
  
  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **)&TcgProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Fill request header
  //
  TpmRsp = (TPM_RSP_COMMAND_HDR*)RecvBuffer;
  TpmRqu = (TPM_RQU_COMMAND_HDR*)SendBuffer;
  
  TpmRqu->tag       = H2NS (TPM_TAG_RQU_COMMAND);
  TpmRqu->paramSize = H2NL (sizeof (SendBuffer));
  TpmRqu->ordinal   = H2NL (TPM_ORD_GetCapability);

  //
  // Set request parameter
  //
  SendBufPtr      = (UINT32*)(TpmRqu + 1);
  WriteUnaligned32 (SendBufPtr++, H2NL (TPM_CAP_FLAG));
  WriteUnaligned32 (SendBufPtr++, H2NL (sizeof (TPM_CAP_FLAG_PERMANENT)));
  WriteUnaligned32 (SendBufPtr, H2NL (TPM_CAP_FLAG_PERMANENT));  
  
  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          sizeof (SendBuffer),
                          (UINT8*)TpmRqu,
                          sizeof (RecvBuffer),
                          (UINT8*)&RecvBuffer
                          );
  ASSERT_EFI_ERROR (Status);
  ASSERT (TpmRsp->tag == H2NS (TPM_TAG_RSP_COMMAND));
  ASSERT (TpmRsp->returnCode == 0);
  
  TpmPermanentFlags = (TPM_PERMANENT_FLAGS *)&RecvBuffer[sizeof (TPM_RSP_COMMAND_HDR) + sizeof (UINT32)];
  
  if (LifetimeLock != NULL) {
    *LifetimeLock = TpmPermanentFlags->physicalPresenceLifetimeLock;
  }

  if (CmdEnable != NULL) {
    *CmdEnable = TpmPermanentFlags->physicalPresenceCMDEnable;
  }

  return Status;
}

/**
  Issue TSC_PhysicalPresence command to TPM.

  @param[in] PhysicalPresence     The state to set the TPM's Physical Presence flags.  
  
  @retval EFI_SUCCESS             TPM executed the command successfully.
  @retval EFI_SECURITY_VIOLATION  TPM returned error when executing the command.
  @retval other                   Failed to locate EFI TCG Protocol.

**/
EFI_STATUS
TpmPhysicalPresence (
  IN      TPM_PHYSICAL_PRESENCE     PhysicalPresence
  )
{
  EFI_STATUS                        Status;
  EFI_TCG_PROTOCOL                  *TcgProtocol;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_PHYSICAL_PRESENCE             *TpmPp;
  TPM_RSP_COMMAND_HDR               TpmRsp;
  UINT8                             Buffer[sizeof (*TpmRqu) + sizeof (*TpmPp)];

  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **)&TcgProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TpmRqu = (TPM_RQU_COMMAND_HDR*)Buffer;
  TpmPp = (TPM_PHYSICAL_PRESENCE*)(TpmRqu + 1);

  TpmRqu->tag = H2NS (TPM_TAG_RQU_COMMAND);
  TpmRqu->paramSize = H2NL (sizeof (Buffer));
  TpmRqu->ordinal = H2NL (TSC_ORD_PhysicalPresence);
  WriteUnaligned16 (TpmPp, (TPM_PHYSICAL_PRESENCE) H2NS (PhysicalPresence));  

  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          sizeof (Buffer),
                          (UINT8*)TpmRqu,
                          sizeof (TpmRsp),
                          (UINT8*)&TpmRsp
                          );
  ASSERT_EFI_ERROR (Status);
  ASSERT (TpmRsp.tag == H2NS (TPM_TAG_RSP_COMMAND));
  if (TpmRsp.returnCode != 0) {
    //
    // If it fails, some requirements may be needed for this command.
    //
    return EFI_SECURITY_VIOLATION;
  }
  return Status;
}

/**
  Issue a TPM command for which no additional output data will be returned.

  @param[in] TcgProtocol              EFI TCG Protocol instance.  
  @param[in] Ordinal                  TPM command code.  
  @param[in] AdditionalParameterSize  Additional parameter size.  
  @param[in] AdditionalParameters     Pointer to the Additional paramaters.  
  
  @retval TPM_PP_BIOS_FAILURE         Error occurred during sending command to TPM or 
                                      receiving response from TPM.
  @retval Others                      Return code from the TPM device after command execution.

**/
TPM_RESULT
TpmCommandNoReturnData (
  IN      EFI_TCG_PROTOCOL          *TcgProtocol,
  IN      TPM_COMMAND_CODE          Ordinal,
  IN      UINTN                     AdditionalParameterSize,
  IN      VOID                      *AdditionalParameters
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_RSP_COMMAND_HDR               TpmRsp;
  UINT32                            Size;

  TpmRqu = (TPM_RQU_COMMAND_HDR*)AllocatePool (
                                   sizeof (*TpmRqu) + AdditionalParameterSize
                                   );
  if (TpmRqu == NULL) {
    return TPM_PP_BIOS_FAILURE;
  }

  TpmRqu->tag       = H2NS (TPM_TAG_RQU_COMMAND);
  Size              = (UINT32)(sizeof (*TpmRqu) + AdditionalParameterSize);
  TpmRqu->paramSize = H2NL (Size);
  TpmRqu->ordinal   = H2NL (Ordinal);
  gBS->CopyMem (TpmRqu + 1, AdditionalParameters, AdditionalParameterSize);

  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          Size,
                          (UINT8*)TpmRqu,
                          (UINT32)sizeof (TpmRsp),
                          (UINT8*)&TpmRsp
                          );
  FreePool (TpmRqu);
  if (EFI_ERROR (Status) || (TpmRsp.tag != H2NS (TPM_TAG_RSP_COMMAND))) {
    return TPM_PP_BIOS_FAILURE;
  }
  return H2NL (TpmRsp.returnCode);
}

/**
  Execute physical presence operation requested by the OS.

  @param[in]      TcgProtocol         EFI TCG Protocol instance.  
  @param[in]      CommandCode         Physical presence operation value.  
  @param[in, out] PpiFlags            The physical presence interface flags. 
  
  @retval TPM_PP_BIOS_FAILURE         Unknown physical presence operation.
  @retval TPM_PP_BIOS_FAILURE         Error occurred during sending command to TPM or 
                                      receiving response from TPM.
  @retval Others                      Return code from the TPM device after command execution.

**/
TPM_RESULT
ExecutePhysicalPresence (
  IN      EFI_TCG_PROTOCOL          *TcgProtocol,
  IN      UINT8                     CommandCode,
  IN OUT  UINT8                     *PpiFlags
  )
{
  BOOLEAN                           BoolVal;
  TPM_RESULT                        TpmResponse;
  UINT32                            InData[5];

  switch (CommandCode) {
    case ENABLE:
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalEnable,
               0,
               NULL
               );

    case DISABLE:
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalDisable,
               0,
               NULL
               );

    case ACTIVATE:
      BoolVal = FALSE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalSetDeactivated,
               sizeof (BoolVal),
               &BoolVal
               );

    case DEACTIVATE:
      BoolVal = TRUE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalSetDeactivated,
               sizeof (BoolVal),
               &BoolVal
               );

    case CLEAR:
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_ForceClear,
               0,
               NULL
               );

    case ENABLE_ACTIVATE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, ENABLE, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, ACTIVATE, PpiFlags);
      }
      return TpmResponse;

    case DEACTIVATE_DISABLE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, DEACTIVATE, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, DISABLE, PpiFlags);
      }
      return TpmResponse;

    case SET_OWNER_INSTALL_TRUE:
      BoolVal = TRUE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_SetOwnerInstall,
               sizeof (BoolVal),
               &BoolVal
               );

    case SET_OWNER_INSTALL_FALSE:
      BoolVal = FALSE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_SetOwnerInstall,
               sizeof (BoolVal),
               &BoolVal
               );

    case ENABLE_ACTIVATE_OWNER_TRUE:
      //
      // ENABLE_ACTIVATE + SET_OWNER_INSTALL_TRUE
      // SET_OWNER_INSTALL_TRUE will be executed atfer reboot
      //
      if ((*PpiFlags & FLAG_RESET_TRACK) == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, ENABLE_ACTIVATE, PpiFlags);
        *PpiFlags |= FLAG_RESET_TRACK;
      } else {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, SET_OWNER_INSTALL_TRUE, PpiFlags);
        *PpiFlags &= ~FLAG_RESET_TRACK;
      }
      return TpmResponse;

    case DEACTIVATE_DISABLE_OWNER_FALSE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, SET_OWNER_INSTALL_FALSE, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, DEACTIVATE_DISABLE, PpiFlags);
      }
      return TpmResponse;

    case DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
      InData[0] = H2NL (TPM_SET_STCLEAR_DATA);            // CapabilityArea
      InData[1] = H2NL (sizeof(UINT32));                  // SubCapSize
      InData[2] = H2NL (TPM_SD_DEFERREDPHYSICALPRESENCE); // SubCap
      InData[3] = H2NL (sizeof(UINT32));                  // SetValueSize
      InData[4] = H2NL (1);                               // UnownedFieldUpgrade; bit0
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_SetCapability,
               sizeof (UINT32) * 5,
               InData
               );

    case SET_OPERATOR_AUTH:
      //
      // TPM_SetOperatorAuth
      // This command requires UI to prompt user for Auth data
      // Here it is NOT implemented
      //
      return TPM_PP_BIOS_FAILURE;

    case CLEAR_ENABLE_ACTIVATE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, CLEAR, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, ENABLE_ACTIVATE, PpiFlags);
      }
      return TpmResponse;

    case SET_NO_PPI_PROVISION_FALSE:
      *PpiFlags &= ~FLAG_NO_PPI_PROVISION;
      return 0;

    case SET_NO_PPI_PROVISION_TRUE:
      *PpiFlags |= FLAG_NO_PPI_PROVISION;
      return 0;

    case SET_NO_PPI_CLEAR_FALSE:
      *PpiFlags &= ~FLAG_NO_PPI_CLEAR;
      return 0;

    case SET_NO_PPI_CLEAR_TRUE:
      *PpiFlags |= FLAG_NO_PPI_CLEAR;
      return 0;

    case SET_NO_PPI_MAINTENANCE_FALSE:
      *PpiFlags &= ~FLAG_NO_PPI_MAINTENANCE;
      return 0;

    case SET_NO_PPI_MAINTENANCE_TRUE:
      *PpiFlags |= FLAG_NO_PPI_MAINTENANCE;
      return 0;
  
    case ENABLE_ACTIVATE_CLEAR:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, ENABLE_ACTIVATE, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, CLEAR, PpiFlags);
      }
      return TpmResponse;

    case ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:
      //
      // ENABLE_ACTIVATE + CLEAR_ENABLE_ACTIVATE
      // CLEAR_ENABLE_ACTIVATE will be executed atfer reboot.
      //
      if ((*PpiFlags & FLAG_RESET_TRACK) == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, ENABLE_ACTIVATE, PpiFlags);
        *PpiFlags |= FLAG_RESET_TRACK;
      } else {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, CLEAR_ENABLE_ACTIVATE, PpiFlags);
        *PpiFlags &= ~FLAG_RESET_TRACK;
      } 
      return TpmResponse;

    default:
      ;
  }
  return TPM_PP_BIOS_FAILURE;
}


/**
  Read the specified key for user confirmation.

  @param[in]  CautionKey  If true,  F12 is used as confirm key;
                          If false, F10 is used as confirm key.

  @retval     TRUE        User confirmed the changes by input.
  @retval     FALSE       User discarded the changes.

**/
BOOLEAN
ReadUserKey (
  IN     BOOLEAN                    CautionKey
  )
{
  EFI_STATUS                        Status;
  EFI_INPUT_KEY                     Key;
  UINT16                            InputKey;
  EFI_TPL                           OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL); 
  gBS->RestoreTPL (TPL_APPLICATION);
      
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

  gBS->RaiseTPL (OldTpl); 

  if (InputKey != SCAN_ESC) {
    return TRUE;
  }
  
  return FALSE;
}

/**
  Display the confirm text and get user confirmation.

  @param[in] TpmPpCommand  The requested TPM physical presence command.

  @retval  TRUE            The user has confirmed the changes.
  @retval  FALSE           The user doesn't confirm the changes.
**/
BOOLEAN
UserConfirm (
  IN      UINT8                     TpmPpCommand
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

  mPpStringPackHandle = HiiAddPackages (
                          &gEfiPhysicalPresenceGuid,
                          NULL,
                          PhysicalPresenceDxeStrings,
                          NULL
                          );
  ASSERT (mPpStringPackHandle != NULL);

  switch (TpmPpCommand) {
    case ENABLE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ENABLE), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);      
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case DISABLE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_DISABLE), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;
      
    case ACTIVATE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACTIVATE), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case DEACTIVATE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_DEACTIVATE), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1); 
      break;

    case CLEAR:
      CautionKey = TRUE;
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CLEAR), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_CLEAR), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      StrnCat (ConfirmText, L" \n\n", (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);      

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CAUTION_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case ENABLE_ACTIVATE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ENABLE_ACTIVATE), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NOTE_ON), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case DEACTIVATE_DISABLE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_DEACTIVATE_DISABLE), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);      
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NOTE_OFF), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case SET_OWNER_INSTALL_TRUE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ALLOW_TAKE_OWNERSHIP), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);      
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case SET_OWNER_INSTALL_FALSE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_DISALLOW_TAKE_OWNERSHIP), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);      
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case ENABLE_ACTIVATE_OWNER_TRUE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_TURN_ON), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NOTE_ON), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case DEACTIVATE_DISABLE_OWNER_FALSE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_TURN_OFF), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);      
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NOTE_OFF), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
      CautionKey = TRUE;
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_UNOWNED_FIELD_UPGRADE), NULL);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_UPGRADE_HEAD_STR), NULL);      
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);
      
      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_MAINTAIN), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CAUTION_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case SET_OPERATOR_AUTH:
      //
      // TPM_SetOperatorAuth
      // This command requires UI to prompt user for Auth data
      // Here it is NOT implemented
      //
      break;

    case CLEAR_ENABLE_ACTIVATE:
      CautionKey = TRUE;
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CLEAR_TURN_ON), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NOTE_ON), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_CLEAR), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_CLEAR_CONT), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CAUTION_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case SET_NO_PPI_PROVISION_TRUE:
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NO_PPI_PROVISION), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_PPI_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ACCEPT_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NO_PPI_INFO), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case SET_NO_PPI_CLEAR_TRUE:
      CautionKey = TRUE;
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CLEAR), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_PPI_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NOTE_CLEAR), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_CLEAR), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      StrnCat (ConfirmText, L" \n\n", (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1); 

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CAUTION_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NO_PPI_INFO), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case SET_NO_PPI_MAINTENANCE_TRUE:
      CautionKey = TRUE;
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NO_PPI_MAINTAIN), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_PPI_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_MAINTAIN), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CAUTION_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NO_PPI_INFO), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case ENABLE_ACTIVATE_CLEAR:
      CautionKey = TRUE;
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ENABLE_ACTIVATE_CLEAR), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_CLEAR), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      StrnCat (ConfirmText, L" \n\n", (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CAUTION_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:
      CautionKey = TRUE;
      TmpStr2 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE), NULL);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_HEAD_STR), NULL);
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_NOTE_ON), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_CLEAR), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_WARNING_CLEAR_CONT), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_CAUTION_KEY), NULL);
      StrnCat (ConfirmText, TmpStr1, (BufSize / sizeof (CHAR16 *)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    default:
      ;
  }

  if (TmpStr2 == NULL) {
    FreePool (ConfirmText);
    return FALSE;
  }

  TmpStr1 = HiiGetString (mPpStringPackHandle, STRING_TOKEN (TPM_REJECT_KEY), NULL);
  BufSize -= StrSize (ConfirmText);
  UnicodeSPrint (ConfirmText + StrLen (ConfirmText), BufSize, TmpStr1, TmpStr2);

  DstStr[80] = L'\0';
  for (Index = 0; Index < StrLen (ConfirmText); Index += 80) {
    StrnCpy(DstStr, ConfirmText + Index, 80);    
    Print (DstStr);    
  }
  
  FreePool (TmpStr1);
  FreePool (TmpStr2);
  FreePool (ConfirmText);

  if (ReadUserKey (CautionKey)) {
    return TRUE;
  }

  return FALSE;  
}

/**
  Check and execute the requested physical presence command.
  
  @param[in, out] TcgPpData  Point to the physical presence NV variable.

**/
VOID
ExecutePendingTpmRequest (
  IN OUT  EFI_PHYSICAL_PRESENCE     *TcgPpData
  )
{
  EFI_STATUS                        Status;
  EFI_TCG_PROTOCOL                  *TcgProtocol;
  UINTN                             DataSize;
  UINT8                             Flags;
  BOOLEAN                           RequestConfirmed;

  Flags            = TcgPpData->Flags;
  RequestConfirmed = FALSE;  
  switch (TcgPpData->PPRequest) {
    case NO_ACTION:
      return;
    case ENABLE:
    case DISABLE:
    case ACTIVATE:
    case DEACTIVATE:
    case ENABLE_ACTIVATE:
    case DEACTIVATE_DISABLE:
    case SET_OWNER_INSTALL_TRUE:
    case SET_OWNER_INSTALL_FALSE:
    case ENABLE_ACTIVATE_OWNER_TRUE:
    case DEACTIVATE_DISABLE_OWNER_FALSE:
    case SET_OPERATOR_AUTH:
      if ((Flags & FLAG_NO_PPI_PROVISION) != 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case CLEAR:
    case ENABLE_ACTIVATE_CLEAR:
      if ((Flags & FLAG_NO_PPI_CLEAR) != 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
      if ((Flags & FLAG_NO_PPI_MAINTENANCE) != 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case CLEAR_ENABLE_ACTIVATE:
    case ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:
      if ((Flags & FLAG_NO_PPI_CLEAR) != 0 && (Flags & FLAG_NO_PPI_PROVISION) != 0) {
        RequestConfirmed = TRUE;
      }
      break;  

    case SET_NO_PPI_PROVISION_FALSE:
    case SET_NO_PPI_CLEAR_FALSE:
    case SET_NO_PPI_MAINTENANCE_FALSE:
      RequestConfirmed = TRUE;
      break;
  }

  if ((Flags & FLAG_RESET_TRACK) != 0) {
    //
    // It had been confirmed in last boot, it doesn't need confirm again.
    //
    RequestConfirmed = TRUE;
  }

  if (!RequestConfirmed) {
    //
    // Print confirm text and wait for approval. 
    //
    RequestConfirmed = UserConfirm (TcgPpData->PPRequest);
  }

  //
  // Execute requested physical presence command.
  //
  TcgPpData->PPResponse = TPM_PP_USER_ABORT;
  if (RequestConfirmed) {
    Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID**) &TcgProtocol);
    ASSERT_EFI_ERROR (Status);
    TcgPpData->PPResponse = ExecutePhysicalPresence (TcgProtocol, TcgPpData->PPRequest, &TcgPpData->Flags);
  }

  //
  // Clear request
  //
  if ((TcgPpData->Flags & FLAG_RESET_TRACK) == 0) {
    TcgPpData->LastPPRequest = TcgPpData->PPRequest;
    TcgPpData->PPRequest = 0;    
  }

  //
  // Save changes
  //
  DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
  Status = gRT->SetVariable (
                  PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiPhysicalPresenceGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  DataSize,
                  TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (TcgPpData->PPResponse == TPM_PP_USER_ABORT) {
    return;
  }

  //
  // Reset system to make new TPM settings in effect
  //
  switch (TcgPpData->LastPPRequest) {
    case ACTIVATE:
    case DEACTIVATE:
    case CLEAR:
    case ENABLE_ACTIVATE:
    case DEACTIVATE_DISABLE:
    case ENABLE_ACTIVATE_OWNER_TRUE:
    case DEACTIVATE_DISABLE_OWNER_FALSE:
    case DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
    case CLEAR_ENABLE_ACTIVATE:
    case ENABLE_ACTIVATE_CLEAR:
    case ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:      
      break;
    default:
      if (TcgPpData->PPRequest != 0) {
        break;
      }
      return;
  }

  Print (L"Rebooting system to make TPM settings in effect\n");
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  ASSERT (FALSE);  
}

/**
  Check and execute the physical presence command requested and
  Lock physical presence.

  @param[in]  Event        Event whose notification function is being invoked
  @param[in]  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
OnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                        Status;
  BOOLEAN                           LifetimeLock;
  BOOLEAN                           CmdEnable;
  UINTN                             DataSize;
  EFI_PHYSICAL_PRESENCE             TcgPpData;
  
  //
  // Check pending request, if not exist, just return.
  //
  DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  ASSERT_EFI_ERROR (Status);
  DEBUG ((EFI_D_INFO, "[TPM] Flags=%x, PPRequest=%x\n", TcgPpData.Flags, TcgPpData.PPRequest));
 
  Status = GetTpmCapability (&LifetimeLock, &CmdEnable);
  if (EFI_ERROR (Status)) {
    return ;
  }

  if (!CmdEnable) {
    if (LifetimeLock) {
      //
      // physicalPresenceCMDEnable is locked, can't execute physical presence command.
      //
      return ;
    }
    Status = TpmPhysicalPresence (TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
    if (EFI_ERROR (Status)) {
      return ;
    }
  }

  //
  // Set operator physical presence flags
  //
  TpmPhysicalPresence (TPM_PHYSICAL_PRESENCE_PRESENT);
  
  //
  // Execute pending TPM request.
  //  
  ExecutePendingTpmRequest (&TcgPpData);
  DEBUG ((EFI_D_INFO, "[TPM] PPResponse = %x\n", TcgPpData.PPResponse));

  //
  // Lock physical presence.
  //
  TpmPhysicalPresence (TPM_PHYSICAL_PRESENCE_NOTPRESENT | TPM_PHYSICAL_PRESENCE_LOCK);
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DriverEntry (
  IN      EFI_HANDLE                ImageHandle,
  IN      EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_EVENT                         Event;
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_PHYSICAL_PRESENCE             TcgPpData;
  
  //
  // Initialize physical presence variable exists.
  //
  DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      ZeroMem ((VOID*)&TcgPpData, sizeof (TcgPpData));
      TcgPpData.Flags |= FLAG_NO_PPI_PROVISION;
      DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
      Status   = gRT->SetVariable (
                        PHYSICAL_PRESENCE_VARIABLE,
                        &gEfiPhysicalPresenceGuid,
                        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                        DataSize,
                        &TcgPpData
                        );
    }
    ASSERT_EFI_ERROR (Status);
  }

  //
  // TPL Level of physical presence should be larger 
  // than one of TcgDxe driver (TPL_CALLBACK)
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             OnReadyToBoot,
             NULL,
             &Event
             );
  return Status;
}

