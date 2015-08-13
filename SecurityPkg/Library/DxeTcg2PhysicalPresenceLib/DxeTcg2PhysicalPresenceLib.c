/** @file
  Execute pending TPM2 requests from OS or BIOS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tpm2ExecutePendingTpmRequest() will receive untrusted input and do validation.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Protocol/Tcg2Protocol.h>
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
#include <Library/HobLib.h>
#include <Guid/EventGroup.h>
#include <Guid/Tcg2PhysicalPresenceData.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/Tcg2PpVendorLib.h>

#define CONFIRM_BUFFER_SIZE         4096

EFI_HII_HANDLE mTcg2PpStringPackHandle;

/**
  Get string by string id from HII Interface.

  @param[in] Id          String ID.

  @retval    CHAR16 *    String from ID.
  @retval    NULL        If error occurs.

**/
CHAR16 *
Tcg2PhysicalPresenceGetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (mTcg2PpStringPackHandle, Id, NULL);
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
Tpm2CommandClear (
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
  Alloc PCR data.

  @param[in]  PlatformAuth      platform auth value. NULL means no platform auth change.
  @param[in]  SupportedPCRBanks Supported PCR banks
  @param[in]  PCRBanks          PCR banks
  
  @retval EFI_SUCCESS Operation completed successfully.
**/
EFI_STATUS
Tpm2CommandAllocPcr (
  IN TPM2B_AUTH                *PlatformAuth,  OPTIONAL
  IN UINT32                    SupportedPCRBanks,
  IN UINT32                    PCRBanks
  )
{
  EFI_STATUS                Status;
  TPMS_AUTH_COMMAND         *AuthSession;
  TPMS_AUTH_COMMAND         LocalAuthSession;
  TPML_PCR_SELECTION        PcrAllocation;
  TPMI_YES_NO               AllocationSuccess;
  UINT32                    MaxPCR;
  UINT32                    SizeNeeded;
  UINT32                    SizeAvailable;

  if (PlatformAuth == NULL) {
    AuthSession = NULL;
  } else {
    AuthSession = &LocalAuthSession;
    ZeroMem (&LocalAuthSession, sizeof(LocalAuthSession));
    LocalAuthSession.sessionHandle = TPM_RS_PW;
    LocalAuthSession.hmac.size = PlatformAuth->size;
    CopyMem (LocalAuthSession.hmac.buffer, PlatformAuth->buffer, PlatformAuth->size);
  }

  //
  // Fill input
  //
  ZeroMem (&PcrAllocation, sizeof(PcrAllocation));
  if ((EFI_TCG2_BOOT_HASH_ALG_SHA1 & SupportedPCRBanks) != 0) {
    PcrAllocation.pcrSelections[PcrAllocation.count].hash = TPM_ALG_SHA1;
    PcrAllocation.pcrSelections[PcrAllocation.count].sizeofSelect = PCR_SELECT_MAX;
    if ((EFI_TCG2_BOOT_HASH_ALG_SHA1 & PCRBanks) != 0) {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0xFF;
    } else {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0x00;
    }
    PcrAllocation.count++;
  }
  if ((EFI_TCG2_BOOT_HASH_ALG_SHA256 & SupportedPCRBanks) != 0) {
    PcrAllocation.pcrSelections[PcrAllocation.count].hash = TPM_ALG_SHA256;
    PcrAllocation.pcrSelections[PcrAllocation.count].sizeofSelect = PCR_SELECT_MAX;
    if ((EFI_TCG2_BOOT_HASH_ALG_SHA256 & PCRBanks) != 0) {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0xFF;
    } else {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0x00;
    }
    PcrAllocation.count++;
  }
  if ((EFI_TCG2_BOOT_HASH_ALG_SHA384 & SupportedPCRBanks) != 0) {
    PcrAllocation.pcrSelections[PcrAllocation.count].hash = TPM_ALG_SHA384;
    PcrAllocation.pcrSelections[PcrAllocation.count].sizeofSelect = PCR_SELECT_MAX;
    if ((EFI_TCG2_BOOT_HASH_ALG_SHA384 & PCRBanks) != 0) {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0xFF;
    } else {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0x00;
    }
    PcrAllocation.count++;
  }
  if ((EFI_TCG2_BOOT_HASH_ALG_SHA512 & SupportedPCRBanks) != 0) {
    PcrAllocation.pcrSelections[PcrAllocation.count].hash = TPM_ALG_SHA512;
    PcrAllocation.pcrSelections[PcrAllocation.count].sizeofSelect = PCR_SELECT_MAX;
    if ((EFI_TCG2_BOOT_HASH_ALG_SHA512 & PCRBanks) != 0) {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0xFF;
    } else {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0x00;
    }
    PcrAllocation.count++;
  }
  if ((EFI_TCG2_BOOT_HASH_ALG_SM3_256 & SupportedPCRBanks) != 0) {
    PcrAllocation.pcrSelections[PcrAllocation.count].hash = TPM_ALG_SM3_256;
    PcrAllocation.pcrSelections[PcrAllocation.count].sizeofSelect = PCR_SELECT_MAX;
    if ((EFI_TCG2_BOOT_HASH_ALG_SM3_256 & PCRBanks) != 0) {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0xFF;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0xFF;
    } else {
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[0] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[1] = 0x00;
      PcrAllocation.pcrSelections[PcrAllocation.count].pcrSelect[2] = 0x00;
    }
    PcrAllocation.count++;
  }
  Status = Tpm2PcrAllocate (
             TPM_RH_PLATFORM,
             AuthSession,
             &PcrAllocation,
             &AllocationSuccess,
             &MaxPCR,
             &SizeNeeded,
             &SizeAvailable
             );
  DEBUG ((EFI_D_INFO, "Tpm2PcrAllocate - %r\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "AllocationSuccess - %02x\n", AllocationSuccess));
  DEBUG ((EFI_D_INFO, "MaxPCR            - %08x\n", MaxPCR));
  DEBUG ((EFI_D_INFO, "SizeNeeded        - %08x\n", SizeNeeded));
  DEBUG ((EFI_D_INFO, "SizeAvailable     - %08x\n", SizeAvailable));

  return EFI_SUCCESS;
}

/**
  Change EPS.

  @param[in]  PlatformAuth      platform auth value. NULL means no platform auth change.
  
  @retval EFI_SUCCESS Operation completed successfully.
**/
EFI_STATUS
Tpm2CommandChangeEps (
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

  Status = Tpm2ChangeEPS (TPM_RH_PLATFORM, AuthSession);
  DEBUG ((EFI_D_INFO, "Tpm2ChangeEPS - %r\n", Status));
  return Status;
}

/**
  Execute physical presence operation requested by the OS.

  @param[in]      PlatformAuth        platform auth value. NULL means no platform auth change.
  @param[in]      CommandCode         Physical presence operation value.
  @param[in]      CommandParameter    Physical presence operation parameter.
  @param[in, out] PpiFlags            The physical presence interface flags.
  
  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE   Unknown physical presence operation.
  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE   Error occurred during sending command to TPM or 
                                                   receiving response from TPM.
  @retval Others                                   Return code from the TPM device after command execution.
**/
UINT32
Tcg2ExecutePhysicalPresence (
  IN      TPM2B_AUTH                       *PlatformAuth,  OPTIONAL
  IN      UINT32                           CommandCode,
  IN      UINT32                           CommandParameter,
  IN OUT  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS *PpiFlags
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_PROTOCOL                 *Tcg2Protocol;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY  ProtocolCapability;

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &Tcg2Protocol);
  ASSERT_EFI_ERROR (Status);

  ProtocolCapability.Size = sizeof(ProtocolCapability);
  Status = Tcg2Protocol->GetCapability (
                           Tcg2Protocol,
                           &ProtocolCapability
                           );
  ASSERT_EFI_ERROR (Status);

  switch (CommandCode) {
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      Status = Tpm2CommandClear (PlatformAuth);
      if (EFI_ERROR (Status)) {
        return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      } else {
        return TCG_PP_OPERATION_RESPONSE_SUCCESS;
      }

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_CLEAR_TRUE:
      PpiFlags->PPFlags |= TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CLEAR;
      return TCG_PP_OPERATION_RESPONSE_SUCCESS;

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_CLEAR_FALSE:
      PpiFlags->PPFlags &= ~TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CLEAR;
      return TCG_PP_OPERATION_RESPONSE_SUCCESS;

    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
      Status = Tpm2CommandAllocPcr (PlatformAuth, ProtocolCapability.HashAlgorithmBitmap, CommandParameter);
      if (EFI_ERROR (Status)) {
        return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      } else {
        return TCG_PP_OPERATION_RESPONSE_SUCCESS;
      }

    case TCG2_PHYSICAL_PRESENCE_CHANGE_EPS:
      Status = Tpm2CommandChangeEps (PlatformAuth);
      if (EFI_ERROR (Status)) {
        return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      } else {
        return TCG_PP_OPERATION_RESPONSE_SUCCESS;
      }

    case TCG2_PHYSICAL_PRESENCE_LOG_ALL_DIGESTS:
      Status = Tpm2CommandAllocPcr (PlatformAuth, ProtocolCapability.HashAlgorithmBitmap, ProtocolCapability.HashAlgorithmBitmap);
      if (EFI_ERROR (Status)) {
        return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      } else {
        return TCG_PP_OPERATION_RESPONSE_SUCCESS;
      }

    default:
      if (CommandCode <= TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
        return TCG_PP_OPERATION_RESPONSE_SUCCESS;
      } else {
        return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
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
Tcg2ReadUserKey (
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
  Fill Buffer With BootHashAlg.

  @param[in] Buffer               Buffer to be filled.
  @param[in] BufferSize           Size of buffer.
  @param[in] BootHashAlg          BootHashAlg.

**/
VOID
Tcg2FillBufferWithBootHashAlg (
  IN UINT16  *Buffer,
  IN UINTN   BufferSize,
  IN UINT32  BootHashAlg
  )
{
  Buffer[0] = 0;
  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA1) != 0) {
    if (Buffer[0] != 0) {
      StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L", ", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
    }
    StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA1", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
  }
  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA256) != 0) {
    if (Buffer[0] != 0) {
      StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L", ", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
    }
    StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA256", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
  }
  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA384) != 0) {
    if (Buffer[0] != 0) {
      StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L", ", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
    }
    StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA384", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
  }
  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SHA512) != 0) {
    if (Buffer[0] != 0) {
      StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L", ", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
    }
    StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L"SHA512", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
  }
  if ((BootHashAlg & EFI_TCG2_BOOT_HASH_ALG_SM3_256) != 0) {
    if (Buffer[0] != 0) {
      StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L", ", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
    }
    StrnCatS (Buffer, BufferSize / sizeof (CHAR16), L"SM3_256", (BufferSize / sizeof (CHAR16)) - StrLen (Buffer) - 1);
  }
}

/**
  Display the confirm text and get user confirmation.

  @param[in] TpmPpCommand             The requested TPM physical presence command.
  @param[in] TpmPpCommandParameter    The requested TPM physical presence command parameter.

  @retval    TRUE          The user has confirmed the changes.
  @retval    FALSE         The user doesn't confirm the changes.
**/
BOOLEAN
Tcg2UserConfirm (
  IN      UINT32                    TpmPpCommand,
  IN      UINT32                    TpmPpCommandParameter
  )
{
  CHAR16                            *ConfirmText;
  CHAR16                            *TmpStr1;
  CHAR16                            *TmpStr2; 
  UINTN                             BufSize;
  BOOLEAN                           CautionKey;
  BOOLEAN                           NoPpiInfo;
  UINT16                            Index;
  CHAR16                            DstStr[81];
  CHAR16                            TempBuffer[1024];
  CHAR16                            TempBuffer2[1024];
  EFI_TCG2_PROTOCOL                 *Tcg2Protocol;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY  ProtocolCapability;
  UINT32                            CurrentPCRBanks;
  EFI_STATUS                        Status;

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &Tcg2Protocol);
  ASSERT_EFI_ERROR (Status);

  ProtocolCapability.Size = sizeof(ProtocolCapability);
  Status = Tcg2Protocol->GetCapability (
                           Tcg2Protocol,
                           &ProtocolCapability
                           );
  ASSERT_EFI_ERROR (Status);

  Status = Tcg2Protocol->GetActivePcrBanks (
                           Tcg2Protocol,
                           &CurrentPCRBanks
                           );
  ASSERT_EFI_ERROR (Status);
  
  TmpStr2     = NULL;
  CautionKey  = FALSE;
  NoPpiInfo   = FALSE;
  BufSize     = CONFIRM_BUFFER_SIZE;
  ConfirmText = AllocateZeroPool (BufSize);
  ASSERT (ConfirmText != NULL);

  switch (TpmPpCommand) {

    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      CautionKey = TRUE;
      TmpStr2 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CLEAR));

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n\n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);      

      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_CLEAR_FALSE:
      CautionKey = TRUE;
      NoPpiInfo  = TRUE;
      TmpStr2 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CLEAR));

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_PPI_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n\n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1); 

      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
      CautionKey = TRUE;
      TmpStr2 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_SET_PCR_BANKS));

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_SET_PCR_BANKS_1));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);      

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_SET_PCR_BANKS_2));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);      

      Tcg2FillBufferWithBootHashAlg (TempBuffer, sizeof(TempBuffer), TpmPpCommandParameter);
      Tcg2FillBufferWithBootHashAlg (TempBuffer2, sizeof(TempBuffer2), CurrentPCRBanks);

      TmpStr1 = AllocateZeroPool (BufSize);
      ASSERT (TmpStr1 != NULL);
      UnicodeSPrint (TmpStr1, BufSize, L"Current PCRBanks is 0x%x. (%s)\nNew PCRBanks is 0x%x. (%s)\n", CurrentPCRBanks, TempBuffer2, TpmPpCommandParameter, TempBuffer);

      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);      

      break;

    case TCG2_PHYSICAL_PRESENCE_CHANGE_EPS:
      CautionKey = TRUE;
      TmpStr2 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CHANGE_EPS));

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CHANGE_EPS_1));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);      
      
      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CHANGE_EPS_2));
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

  if (TpmPpCommand < TCG2_PHYSICAL_PRESENCE_STORAGE_MANAGEMENT_BEGIN) {
    if (CautionKey) {
      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
    } else {
      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
    }
    StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
    FreePool (TmpStr1);

    if (NoPpiInfo) {
      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NO_PPI_INFO));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
    }

    TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TPM_REJECT_KEY));
  } else {
    if (CautionKey) {
      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_CAUTION_KEY));
    } else {
      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_ACCEPT_KEY));
    }
    StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
    FreePool (TmpStr1);

    if (NoPpiInfo) {
      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_NO_PPI_INFO));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
    }

    TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_REJECT_KEY));
  }
  BufSize -= StrSize (ConfirmText);
  UnicodeSPrint (ConfirmText + StrLen (ConfirmText), BufSize, TmpStr1, TmpStr2);

  DstStr[80] = L'\0';
  for (Index = 0; Index < StrLen (ConfirmText); Index += 80) {
    StrnCpyS (DstStr, sizeof (DstStr) / sizeof (CHAR16), ConfirmText + Index, sizeof (DstStr) / sizeof (CHAR16) - 1);    
    Print (DstStr);    
  }
  
  FreePool (TmpStr1);
  FreePool (TmpStr2);
  FreePool (ConfirmText);

  if (Tcg2ReadUserKey (CautionKey)) {
    return TRUE;
  }

  return FALSE;  
}

/**
  Check if there is a valid physical presence command request. Also updates parameter value 
  to whether the requested physical presence command already confirmed by user
 
   @param[in]  TcgPpData                 EFI Tcg2 Physical Presence request data. 
   @param[in]  Flags                     The physical presence interface flags.
   @param[out] RequestConfirmed            If the physical presence operation command required user confirm from UI.
                                             True, it indicates the command doesn't require user confirm, or already confirmed 
                                                   in last boot cycle by user.
                                             False, it indicates the command need user confirm from UI.

   @retval  TRUE        Physical Presence operation command is valid.
   @retval  FALSE       Physical Presence operation command is invalid.

**/
BOOLEAN
Tcg2HaveValidTpmRequest  (
  IN      EFI_TCG2_PHYSICAL_PRESENCE       *TcgPpData,
  IN      EFI_TCG2_PHYSICAL_PRESENCE_FLAGS Flags,
  OUT     BOOLEAN                          *RequestConfirmed
  )
{
  BOOLEAN  IsRequestValid;

  *RequestConfirmed = FALSE;

  switch (TcgPpData->PPRequest) {
    case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
      *RequestConfirmed = TRUE;
      return TRUE;

    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      if ((Flags.PPFlags & TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CLEAR) == 0) {
        *RequestConfirmed = TRUE;
      }
      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_CLEAR_TRUE:
      *RequestConfirmed = TRUE;
      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_CLEAR_FALSE:
      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
      if ((Flags.PPFlags & TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CHANGE_PCRS) == 0) {
        *RequestConfirmed = TRUE;
      }
      break;

    case TCG2_PHYSICAL_PRESENCE_CHANGE_EPS:
      if ((Flags.PPFlags & TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CHANGE_EPS) == 0) {
        *RequestConfirmed = TRUE;
      }
      break;
      
    case TCG2_PHYSICAL_PRESENCE_LOG_ALL_DIGESTS:
      *RequestConfirmed = TRUE;
      break;

    default:
      if (TcgPpData->PPRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
        IsRequestValid = Tcg2PpVendorLibHasValidRequest (TcgPpData->PPRequest, Flags.PPFlags, RequestConfirmed);
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

  if ((Flags.PPFlags & TCG2_LIB_PP_FLAG_RESET_TRACK) != 0) {
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
Tcg2ExecutePendingTpmRequest (
  IN      TPM2B_AUTH                       *PlatformAuth,  OPTIONAL
  IN      EFI_TCG2_PHYSICAL_PRESENCE       *TcgPpData,
  IN      EFI_TCG2_PHYSICAL_PRESENCE_FLAGS Flags
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  BOOLEAN                           RequestConfirmed;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS  NewFlags;
  BOOLEAN                           ResetRequired;
  UINT32                            NewPPFlags;

  if (TcgPpData->PPRequest == TCG2_PHYSICAL_PRESENCE_NO_ACTION) {
    //
    // No operation request
    //
    return;
  }

  if (!Tcg2HaveValidTpmRequest(TcgPpData, Flags, &RequestConfirmed)) {
    //
    // Invalid operation request.
    //
    if (TcgPpData->PPRequest <= TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
      TcgPpData->PPResponse = TCG_PP_OPERATION_RESPONSE_SUCCESS;
    } else {
      TcgPpData->PPResponse = TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
    }
    TcgPpData->LastPPRequest = TcgPpData->PPRequest;
    TcgPpData->PPRequest = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
    TcgPpData->PPRequestParameter = 0;

    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status = gRT->SetVariable (
                    TCG2_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize,
                    TcgPpData
                    );
    return;
  }

  ResetRequired = FALSE;
  if (TcgPpData->PPRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
    NewFlags = Flags;
    NewPPFlags = NewFlags.PPFlags;
    TcgPpData->PPResponse = Tcg2PpVendorLibExecutePendingRequest (PlatformAuth, TcgPpData->PPRequest, &NewPPFlags, &ResetRequired);
    NewFlags.PPFlags = NewPPFlags;
  } else {
    if (!RequestConfirmed) {
      //
      // Print confirm text and wait for approval. 
      //
      RequestConfirmed = Tcg2UserConfirm (TcgPpData->PPRequest, TcgPpData->PPRequestParameter);
    }

    //
    // Execute requested physical presence command
    //
    TcgPpData->PPResponse = TCG_PP_OPERATION_RESPONSE_USER_ABORT;
    NewFlags = Flags;
    if (RequestConfirmed) {
      TcgPpData->PPResponse = Tcg2ExecutePhysicalPresence (
                                PlatformAuth,
                                TcgPpData->PPRequest, 
                                TcgPpData->PPRequestParameter, 
                                &NewFlags
                                );
    }
  }

  //
  // Save the flags if it is updated.
  //
  if (CompareMem (&Flags, &NewFlags, sizeof(EFI_TCG2_PHYSICAL_PRESENCE_FLAGS)) != 0) {
    Status   = gRT->SetVariable (
                      TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                      &gEfiTcg2PhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_TCG2_PHYSICAL_PRESENCE_FLAGS),
                      &NewFlags
                      ); 
  }

  //
  // Clear request
  //
  if ((NewFlags.PPFlags & TCG2_LIB_PP_FLAG_RESET_TRACK) == 0) {
    TcgPpData->LastPPRequest = TcgPpData->PPRequest;
    TcgPpData->PPRequest = TCG2_PHYSICAL_PRESENCE_NO_ACTION;    
    TcgPpData->PPRequestParameter = 0;
  }

  //
  // Save changes
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->SetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  DataSize,
                  TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (TcgPpData->PPResponse == TCG_PP_OPERATION_RESPONSE_USER_ABORT) {
    return;
  }

  //
  // Reset system to make new TPM settings in effect
  //
  switch (TcgPpData->LastPPRequest) {
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
    case TCG2_PHYSICAL_PRESENCE_CHANGE_EPS:
    case TCG2_PHYSICAL_PRESENCE_LOG_ALL_DIGESTS:
      break;

    default:
      if (TcgPpData->LastPPRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
        if (ResetRequired) {
          break;
        } else {
          return ;
        }
      }
      if (TcgPpData->PPRequest != TCG2_PHYSICAL_PRESENCE_NO_ACTION) {
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
Tcg2PhysicalPresenceLibProcessRequest (
  IN      TPM2B_AUTH                     *PlatformAuth  OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE        TcgPpData;
  EFI_TCG2_PROTOCOL                 *Tcg2Protocol;
  EDKII_VARIABLE_LOCK_PROTOCOL      *VariableLockProtocol;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS  PpiFlags;

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &Tcg2Protocol);
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Check S4 resume
  //
  if (GetBootModeHob () == BOOT_ON_S4_RESUME) {
    DEBUG ((EFI_D_INFO, "S4 Resume, Skip TPM PP process!\n"));
    return ;
  }

  mTcg2PpStringPackHandle = HiiAddPackages (&gEfiTcg2PhysicalPresenceGuid, gImageHandle, DxeTcg2PhysicalPresenceLibStrings, NULL);
  ASSERT (mTcg2PpStringPackHandle != NULL);

  //
  // Initialize physical presence flags.
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE_FLAGS);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    PpiFlags.PPFlags = TCG2_BIOS_TPM_MANAGEMENT_FLAG_DEFAULT;
    Status   = gRT->SetVariable (
                      TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                      &gEfiTcg2PhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_TCG2_PHYSICAL_PRESENCE_FLAGS),
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
                                     TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                                     &gEfiTcg2PhysicalPresenceGuid
                                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[TPM2] Error when lock variable %s, Status = %r\n", TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }
  
  //
  // Initialize physical presence variable.
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    ZeroMem ((VOID*)&TcgPpData, sizeof (TcgPpData));
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status   = gRT->SetVariable (
                      TCG2_PHYSICAL_PRESENCE_VARIABLE,
                      &gEfiTcg2PhysicalPresenceGuid,
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
  Tcg2ExecutePendingTpmRequest (PlatformAuth, &TcgPpData, PpiFlags);
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
Tcg2PhysicalPresenceLibNeedUserConfirm(
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_PHYSICAL_PRESENCE        TcgPpData;
  UINTN                             DataSize;
  BOOLEAN                           RequestConfirmed;
  EFI_TCG2_PROTOCOL                 *Tcg2Protocol;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS  PpiFlags;

  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &Tcg2Protocol);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Check S4 resume
  //
  if (GetBootModeHob () == BOOT_ON_S4_RESUME) {
    DEBUG ((EFI_D_INFO, "S4 Resume, Skip TPM PP process!\n"));
    return FALSE;
  }

  //
  // Check Tpm requests
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE_FLAGS);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  
  if (TcgPpData.PPRequest == TCG2_PHYSICAL_PRESENCE_NO_ACTION) {
    //
    // No operation request
    //
    return FALSE;
  }

  if (!Tcg2HaveValidTpmRequest(&TcgPpData, PpiFlags, &RequestConfirmed)) {
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


/**
  The handler for TPM physical presence function:
  Return TPM Operation Response to OS Environment.

  @param[out]     MostRecentRequest Most recent operation request.
  @param[out]     Response          Response to the most recent operation request.

  @return Return Code for Return TPM Operation Response to OS Environment.
**/
UINT32
EFIAPI
Tcg2PhysicalPresenceLibReturnOperationResponseToOsFunction (
  OUT UINT32                *MostRecentRequest,
  OUT UINT32                *Response
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE        PpData;
  
  DEBUG ((EFI_D_INFO, "[TPM2] ReturnOperationResponseToOsFunction\n"));

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpData
                  );
  if (EFI_ERROR (Status)) {
    *MostRecentRequest = 0;
    *Response          = 0;
    DEBUG ((EFI_D_ERROR, "[TPM2] Get PP variable failure! Status = %r\n", Status));
    return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_FAILURE;
  }
  
  *MostRecentRequest = PpData.LastPPRequest;
  *Response          = PpData.PPResponse;

  return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_SUCCESS;
}

/**
  The handler for TPM physical presence function:
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

  Caution: This function may receive untrusted input.
  
  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      RequestParameter TPM physical presence operation request parameter.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
          Submit TPM Operation Request to Pre-OS Environment 2.
**/
UINT32
EFIAPI
Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 RequestParameter
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE        PpData;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS  Flags;
  
  DEBUG ((EFI_D_INFO, "[TPM2] SubmitRequestToPreOSFunction, Request = %x, %x\n", OperationRequest, RequestParameter));
  
  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[TPM2] Get PP variable failure! Status = %r\n", Status));
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  if ((OperationRequest > TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) &&
      (OperationRequest < TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) ) {
    //
    // This command requires UI to prompt user for Auth data.
    //
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED;
  }

  if (PpData.PPRequest != OperationRequest) {
    PpData.PPRequest = (UINT8)OperationRequest;
    PpData.PPRequestParameter = RequestParameter;
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status = gRT->SetVariable (
                    TCG2_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize,
                    &PpData
                    );
  }

  if (EFI_ERROR (Status)) { 
    DEBUG ((EFI_D_ERROR, "[TPM2] Set PP variable failure! Status = %r\n", Status));
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  if (OperationRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE_FLAGS);
    Status = gRT->GetVariable (
                    TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    NULL,
                    &DataSize,
                    &Flags
                    );
    if (EFI_ERROR (Status)) {
      Flags.PPFlags = TCG2_BIOS_TPM_MANAGEMENT_FLAG_DEFAULT;
    }
    return Tcg2PpVendorLibSubmitRequestToPreOSFunction (OperationRequest, Flags.PPFlags, RequestParameter);
  }

  return TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS;
}
