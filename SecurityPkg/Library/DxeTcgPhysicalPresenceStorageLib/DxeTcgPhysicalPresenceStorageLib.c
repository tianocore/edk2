/** @file
  Tcg PP storage library instance that does support any storage specific PPI.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiDxe.h>

#include <Guid/PhysicalPresenceData.h>
#include <Guid/Tcg2PhysicalPresenceData.h>
#include <Guid/TcgPhysicalPresenceStorageData.h>

#include <IndustryStandard/TcgPhysicalPresence.h>

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

#include <Library/TcgPhysicalPresenceStorageLib.h>

#include "DxeTcgPhysicalPresenceStorage.h"
#include "DxeTcg2PhysicalPresenceStorage.h"

#define CONFIRM_BUFFER_SIZE         4096

EFI_HII_HANDLE mTcgPpStorageStringPackHandle;

/**
  Get string by string id from HII Interface.

  @param[in] Id          String ID.

  @retval    CHAR16 *    String from ID.
  @retval    NULL        If error occurs.

**/
CHAR16 *
TcgPpGetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (mTcgPpStorageStringPackHandle, Id, NULL);
}

/**
  Read the specified key for user confirmation.

  @retval     TRUE        User confirmed the changes by input.
  @retval     FALSE       User discarded the changes.
**/
BOOLEAN
TcgPpStrageReadUserKey (
  VOID
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
      if ((Key.ScanCode == SCAN_F10)) {
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
  Display the confirm text and get user confirmation.

  @param[in] TpmPpCommand             The requested TPM physical presence command.

  @retval    TRUE          The user has confirmed the changes.
  @retval    FALSE         The user doesn't confirm the changes.
**/
BOOLEAN
TcgPpUserConfirm (
  IN      UINT8                     TpmPpCommand
  )
{
  CHAR16                            *ConfirmText;
  CHAR16                            *TmpStr1;
  CHAR16                            *TmpStr2;
  UINTN                             BufSize;
  UINT16                            Index;
  CHAR16                            DstStr[81];

  TmpStr2     = NULL;
  BufSize     = CONFIRM_BUFFER_SIZE;
  ConfirmText = AllocateZeroPool (BufSize);
  ASSERT (ConfirmText != NULL);

  switch (TpmPpCommand) {
  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_TRUE:
    TmpStr2 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_PP_ENABLE_BLOCK_SID));

    TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
    UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
    FreePool (TmpStr1);
    break;

  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_FALSE:
    TmpStr2 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_PP_ENABLE_BLOCK_SID));

    TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
    UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
    FreePool (TmpStr1);
    break;

  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_TRUE:
    TmpStr2 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_PP_DISABLE_BLOCK_SID));

    TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
    UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
    FreePool (TmpStr1);
    break;

  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_FALSE:
    TmpStr2 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_PP_DISABLE_BLOCK_SID));

    TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
    UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
    FreePool (TmpStr1);
    break;

  case TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID:
    TmpStr2 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_ENABLE_BLOCK_SID));

    TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
    UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
    FreePool (TmpStr1);
    break;

  case TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID:
    TmpStr2 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_DISABLE_BLOCK_SID));

    TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_HEAD_STR));
    UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
    FreePool (TmpStr1);
    break;

  default:
    break;
  }

  TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_ACCEPT_KEY));
  StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
  FreePool (TmpStr1);

  TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_NO_PPI_INFO));
  StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
  FreePool (TmpStr1);


  TmpStr1 = TcgPpGetStringById (STRING_TOKEN (TCG_STORAGE_REJECT_KEY));
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

  if (TcgPpStrageReadUserKey ()) {
    return TRUE;
  }

  return FALSE;
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
TcgPhysicalPresenceStorageLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 RequestParameter
  )
{
  //
  // Get Physical Presence command state
  //
  if (CompareGuid(PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)) {
    return TcgSubmitStorageRequest (OperationRequest, RequestParameter);
  } else {
    return Tcg2SubmitStorageRequest (OperationRequest, RequestParameter);
  }
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
TcgPhysicalPresenceStorageLibReturnOperationResponseToOsFunction (
  OUT UINT32                *MostRecentRequest,
  OUT UINT32                *Response
  )
{
  //
  // Get Physical Presence command state
  //
  if (CompareGuid(PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)) {
    return TcgReturnOperationResponseToOsFunction (MostRecentRequest, Response);
  } else {
    return Tcg2ReturnOperationResponseToOsFunction (MostRecentRequest, Response);
  }
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
TcgPhysicalPresenceStorageLibNeedUserConfirm(
  VOID
  )
{
  //
  // Get Physical Presence command state
  //
  if (CompareGuid(PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)) {
    return TcgNeedUserConfirm ();
  } else {
    return Tcg2NeedUserConfirm ();
  }
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
TcgPhysicalPresenceStorageLibProcessRequest (
  VOID
  )
{
  //
  // Get Physical Presence command state
  //
  if (CompareGuid(PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid)) {
    TcgProcessStorageRequest();
  } else {
    Tcg2ProcessStorageRequest ();
  }
}

/**
  The handler for TPM physical presence function:
  Return TPM Operation flag variable.

  @return Return Code for Return TPM Operation flag variable.
**/
UINT32
EFIAPI
TcgPhysicalPresenceStorageLibReturnStorageFlags (
  VOID
  )
{
  UINTN                                    DataSize;
  EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS  PpiFlags;
  EFI_STATUS                               Status;


  DataSize = sizeof (EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS);
  Status = gRT->GetVariable (
                  TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE,
                  &gEfiTcgPhysicalPresenceStorageGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    return TCG_BIOS_STORAGE_MANAGEMENT_FLAG_DEFAULT;
  }

  return PpiFlags.PPFlags;
}

/**

  Install Boot Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
TcgPhysicalPresenceStorageLibConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  mTcgPpStorageStringPackHandle = HiiAddPackages (&gEfiTcgPhysicalPresenceStorageGuid, gImageHandle, DxeTcgPhysicalPresenceStorageLibStrings, NULL);
  ASSERT (mTcgPpStorageStringPackHandle != NULL);

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.
  @param[in]  SystemTable       System Table

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
TcgPhysicalPresenceStorageLibDestructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  HiiRemovePackages (mTcgPpStorageStringPackHandle);

  return EFI_SUCCESS;
}
