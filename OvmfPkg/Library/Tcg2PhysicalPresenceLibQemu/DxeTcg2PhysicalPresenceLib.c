/** @file
  Execute pending TPM2 requests from OS or BIOS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tcg2ExecutePendingTpmRequest() will receive untrusted input and do validation.

Copyright (C) 2018, Red Hat, Inc.
Copyright (c) 2018, IBM Corporation. All rights reserved.<BR>
Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/Tcg2PhysicalPresenceData.h>
#include <IndustryStandard/QemuTpm.h>
#include <Protocol/Tcg2Protocol.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>

#include <Library/Tcg2PhysicalPresenceLib.h>

#define CONFIRM_BUFFER_SIZE         4096

EFI_HII_HANDLE mTcg2PpStringPackHandle;

#define TPM_PPI_FLAGS (QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ)

STATIC volatile QEMU_TPM_PPI *mPpi;


/**
  Reads QEMU PPI config from fw_cfg.

  @param[out]  The Config structure to read to.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_PROTOCOL_ERROR    Invalid fw_cfg entry size.
**/
STATIC
EFI_STATUS
QemuTpmReadConfig (
  OUT QEMU_FWCFG_TPM_CONFIG *Config
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;

  Status = QemuFwCfgFindFile ("etc/tpm/config", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize != sizeof (*Config)) {
    return EFI_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof (*Config), Config);
  return EFI_SUCCESS;
}


/**
  Initializes QEMU PPI memory region.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_PROTOCOL_ERROR    PPI address is invalid.
**/
STATIC
EFI_STATUS
QemuTpmInitPPI (
  VOID
  )
{
  EFI_STATUS                      Status;
  QEMU_FWCFG_TPM_CONFIG           Config;
  EFI_PHYSICAL_ADDRESS            PpiAddress64;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
  UINTN                           Idx;

  if (mPpi != NULL) {
    return EFI_SUCCESS;
  }

  Status = QemuTpmReadConfig (&Config);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPpi = (QEMU_TPM_PPI *)(UINTN)Config.PpiAddress;
  if (mPpi == NULL) {
    return EFI_PROTOCOL_ERROR;
  }

  DEBUG ((DEBUG_INFO, "[TPM2PP] mPpi=%p version=%d\n", mPpi, Config.TpmVersion));

  PpiAddress64 = (UINTN)mPpi;
  if ((PpiAddress64 & ~(UINT64)EFI_PAGE_MASK) !=
      ((PpiAddress64 + sizeof *mPpi - 1) & ~(UINT64)EFI_PAGE_MASK)) {
    DEBUG ((DEBUG_ERROR, "[TPM2PP] mPpi crosses a page boundary\n"));
    goto InvalidPpiAddress;
  }

  Status = gDS->GetMemorySpaceDescriptor (PpiAddress64, &Descriptor);
  if (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) {
    ASSERT_EFI_ERROR (Status);
    goto InvalidPpiAddress;
  }
  if (!EFI_ERROR (Status) &&
      (Descriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo &&
       Descriptor.GcdMemoryType != EfiGcdMemoryTypeNonExistent)) {
    DEBUG ((DEBUG_ERROR, "[TPM2PP] mPpi has an invalid memory type\n"));
    goto InvalidPpiAddress;
  }

  for (Idx = 0; Idx < ARRAY_SIZE (mPpi->Func); Idx++) {
    mPpi->Func[Idx] = 0;
  }
  if (Config.TpmVersion == QEMU_TPM_VERSION_2) {
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_NO_ACTION] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_CLEAR] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_CHANGE_EPS] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_LOG_ALL_DIGESTS] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID] = TPM_PPI_FLAGS;
    mPpi->Func[TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID] = TPM_PPI_FLAGS;
  }

  if (mPpi->In == 0) {
    mPpi->In = 1;
    mPpi->Request = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
    mPpi->LastRequest = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
    mPpi->NextStep = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
  }

  return EFI_SUCCESS;

InvalidPpiAddress:
  mPpi = NULL;
  return EFI_PROTOCOL_ERROR;
}


/**
  Get string by string id from HII Interface.

  @param[in] Id          String ID.

  @retval    CHAR16 *    String from ID.
  @retval    NULL        If error occurs.

**/
STATIC
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
    ZeroMem (&LocalAuthSession, sizeof (LocalAuthSession));
    LocalAuthSession.sessionHandle = TPM_RS_PW;
    LocalAuthSession.hmac.size = PlatformAuth->size;
    CopyMem (LocalAuthSession.hmac.buffer, PlatformAuth->buffer, PlatformAuth->size);
  }

  DEBUG ((DEBUG_INFO, "Tpm2ClearControl ... \n"));
  Status = Tpm2ClearControl (TPM_RH_PLATFORM, AuthSession, NO);
  DEBUG ((DEBUG_INFO, "Tpm2ClearControl - %r\n", Status));
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  DEBUG ((DEBUG_INFO, "Tpm2Clear ... \n"));
  Status = Tpm2Clear (TPM_RH_PLATFORM, AuthSession);
  DEBUG ((DEBUG_INFO, "Tpm2Clear - %r\n", Status));

Done:
  ZeroMem (&LocalAuthSession.hmac, sizeof (LocalAuthSession.hmac));
  return Status;
}


/**
  Change EPS.

  @param[in]  PlatformAuth      platform auth value. NULL means no platform auth change.

  @retval EFI_SUCCESS Operation completed successfully.
**/
STATIC
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
    ZeroMem (&LocalAuthSession, sizeof (LocalAuthSession));
    LocalAuthSession.sessionHandle = TPM_RS_PW;
    LocalAuthSession.hmac.size = PlatformAuth->size;
    CopyMem (LocalAuthSession.hmac.buffer, PlatformAuth->buffer, PlatformAuth->size);
  }

  Status = Tpm2ChangeEPS (TPM_RH_PLATFORM, AuthSession);
  DEBUG ((DEBUG_INFO, "Tpm2ChangeEPS - %r\n", Status));

  ZeroMem (&LocalAuthSession.hmac, sizeof(LocalAuthSession.hmac));
  return Status;
}


/**
  Execute physical presence operation requested by the OS.

  @param[in]      PlatformAuth        platform auth value. NULL means no platform auth change.
  @param[in]      CommandCode         Physical presence operation value.
  @param[in]      CommandParameter    Physical presence operation parameter.

  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE   Unknown physical presence operation.
  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE   Error occurred during sending command to TPM or
                                                   receiving response from TPM.
  @retval Others                                   Return code from the TPM device after command execution.
**/
STATIC
UINT32
Tcg2ExecutePhysicalPresence (
  IN      TPM2B_AUTH                       *PlatformAuth,  OPTIONAL
  IN      UINT32                           CommandCode,
  IN      UINT32                           CommandParameter
  )
{
  EFI_STATUS                        Status;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP   TpmHashAlgorithmBitmap;
  UINT32                            ActivePcrBanks;

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

    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
      Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &ActivePcrBanks);
      ASSERT_EFI_ERROR (Status);

      //
      // PP spec requirements:
      //    Firmware should check that all requested (set) hashing algorithms are supported with respective PCR banks.
      //    Firmware has to ensure that at least one PCR banks is active.
      // If not, an error is returned and no action is taken.
      //
      if (CommandParameter == 0 || (CommandParameter & (~TpmHashAlgorithmBitmap)) != 0) {
        DEBUG((DEBUG_ERROR, "PCR banks %x to allocate are not supported by TPM. Skip operation\n", CommandParameter));
        return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      }

      Status = Tpm2PcrAllocateBanks (PlatformAuth, TpmHashAlgorithmBitmap, CommandParameter);
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
      Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashAlgorithmBitmap, &ActivePcrBanks);
      ASSERT_EFI_ERROR (Status);
      Status = Tpm2PcrAllocateBanks (PlatformAuth, TpmHashAlgorithmBitmap, TpmHashAlgorithmBitmap);
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
STATIC
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
STATIC
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
STATIC
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

  TmpStr2     = NULL;
  CautionKey  = FALSE;
  NoPpiInfo   = FALSE;
  BufSize     = CONFIRM_BUFFER_SIZE;
  ConfirmText = AllocateZeroPool (BufSize);
  ASSERT (ConfirmText != NULL);

  mTcg2PpStringPackHandle = HiiAddPackages (&gEfiTcg2PhysicalPresenceGuid, gImageHandle, Tcg2PhysicalPresenceLibQemuStrings, NULL);
  ASSERT (mTcg2PpStringPackHandle != NULL);

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

    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
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

    case TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID:
      TmpStr2 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_ENABLE_BLOCK_SID));

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);
      break;

    case TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID:
      TmpStr2 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_DISABLE_BLOCK_SID));

      TmpStr1 = Tcg2PhysicalPresenceGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);
      break;

    default:
      ;
  }

  if (TmpStr2 == NULL) {
    FreePool (ConfirmText);
    return FALSE;
  }

  // Console for user interaction
  // We need to connect all trusted consoles for TCG PP. Here we treat all consoles in OVMF to be trusted consoles.
  EfiBootManagerConnectAllDefaultConsoles ();

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
  HiiRemovePackages (mTcg2PpStringPackHandle);

  if (Tcg2ReadUserKey (CautionKey)) {
    return TRUE;
  }

  return FALSE;
}


/**
  Check if there is a valid physical presence command request. Also updates parameter value
  to whether the requested physical presence command already confirmed by user

   @param[out] RequestConfirmed          If the physical presence operation command required user confirm from UI.
                                           True, it indicates the command doesn't require user confirm, or already confirmed
                                                 in last boot cycle by user.
                                           False, it indicates the command need user confirm from UI.

   @retval  TRUE        Physical Presence operation command is valid.
   @retval  FALSE       Physical Presence operation command is invalid.

**/
STATIC
BOOLEAN
Tcg2HaveValidTpmRequest  (
  OUT     BOOLEAN                          *RequestConfirmed
  )
{
  EFI_TCG2_PROTOCOL                 *Tcg2Protocol;
  EFI_STATUS                        Status;

  *RequestConfirmed = FALSE;

  if (mPpi->Request <= TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
    //
    // Need TCG2 protocol.
    //
    Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &Tcg2Protocol);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }
  }

  switch (mPpi->Request) {
    case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
    case TCG2_PHYSICAL_PRESENCE_LOG_ALL_DIGESTS:
      *RequestConfirmed = TRUE;
      return TRUE;

    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
    case TCG2_PHYSICAL_PRESENCE_CHANGE_EPS:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID:
    case TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID:
      break;

    default:
      //
      // Wrong Physical Presence command
      //
      return FALSE;
  }

  //
  // Physical Presence command is correct
  //
  return TRUE;
}


/**
  Check and execute the requested physical presence command.

  @param[in]      PlatformAuth      platform auth value. NULL means no platform auth change.
**/
STATIC
VOID
Tcg2ExecutePendingTpmRequest (
  IN      TPM2B_AUTH                       *PlatformAuth OPTIONAL
  )
{
  BOOLEAN                           RequestConfirmed;

  if (mPpi->Request == TCG2_PHYSICAL_PRESENCE_NO_ACTION) {
    //
    // No operation request
    //
    return;
  }

  if (!Tcg2HaveValidTpmRequest (&RequestConfirmed)) {
    //
    // Invalid operation request.
    //
    if (mPpi->Request <= TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
      mPpi->Response = TCG_PP_OPERATION_RESPONSE_SUCCESS;
    } else {
      mPpi->Response = TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
    }
    mPpi->LastRequest = mPpi->Request;
    mPpi->Request = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
    mPpi->RequestParameter = 0;
    return;
  }

  if (!RequestConfirmed) {
    //
    // Print confirm text and wait for approval.
    //
    RequestConfirmed = Tcg2UserConfirm (mPpi->Request, mPpi->RequestParameter);
  }

  //
  // Execute requested physical presence command
  //
  mPpi->Response = TCG_PP_OPERATION_RESPONSE_USER_ABORT;
  if (RequestConfirmed) {
    mPpi->Response = Tcg2ExecutePhysicalPresence (
                                                  PlatformAuth,
                                                  mPpi->Request,
                                                  mPpi->RequestParameter
                                                  );
  }

  //
  // Clear request
  //
  mPpi->LastRequest = mPpi->Request;
  mPpi->Request = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
  mPpi->RequestParameter = 0;

  if (mPpi->Response == TCG_PP_OPERATION_RESPONSE_USER_ABORT) {
    return;
  }

  //
  // Reset system to make new TPM settings in effect
  //
  switch (mPpi->LastRequest) {
  case TCG2_PHYSICAL_PRESENCE_CLEAR:
  case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
  case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
  case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
  case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
  case TCG2_PHYSICAL_PRESENCE_CHANGE_EPS:
  case TCG2_PHYSICAL_PRESENCE_LOG_ALL_DIGESTS:
    break;

  case TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID:
  case TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID:
    break;

  default:
    if (mPpi->Request != TCG2_PHYSICAL_PRESENCE_NO_ACTION) {
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
  EFI_STATUS Status;

  Status = QemuTpmInitPPI ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[TPM2PP] no PPI\n"));
    return ;
  }

  //
  // Check S4 resume
  //
  if (GetBootModeHob () == BOOT_ON_S4_RESUME) {
    DEBUG ((DEBUG_INFO, "S4 Resume, Skip TPM PP process!\n"));
    return ;
  }

  DEBUG ((DEBUG_INFO, "[TPM2PP] PPRequest=%x (PPRequestParameter=%x)\n", mPpi->Request, mPpi->RequestParameter));
  Tcg2ExecutePendingTpmRequest (PlatformAuth);
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
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "[TPM2PP] ReturnOperationResponseToOsFunction\n"));

  Status = QemuTpmInitPPI ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[TPM2PP] no PPI\n"));
    *MostRecentRequest = 0;
    *Response          = 0;
    return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_FAILURE;
  }

  *MostRecentRequest = mPpi->LastRequest;
  *Response          = mPpi->Response;

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
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "[TPM2PP] SubmitRequestToPreOSFunction, Request = %x, %x\n", OperationRequest, RequestParameter));

  Status = QemuTpmInitPPI ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[TPM2PP] no PPI\n"));
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  mPpi->Request = OperationRequest;
  mPpi->RequestParameter = RequestParameter;

  return TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS;
}
