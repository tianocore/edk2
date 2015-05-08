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
  return NULL;
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
  return EFI_SUCCESS;
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
  return 0;
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
  return;
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
  return;
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

  return FALSE;
}

