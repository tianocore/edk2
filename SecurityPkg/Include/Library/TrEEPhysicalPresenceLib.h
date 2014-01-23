/** @file
  Ihis library is intended to be used by BDS modules.
  This library will execute TPM2 request.

Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TREE_PHYSICAL_PRESENCE_LIB_H_
#define _TREE_PHYSICAL_PRESENCE_LIB_H_

#include <IndustryStandard/Tpm20.h>
#include <Protocol/TrEEProtocol.h>

/**
  Check and execute the pending TPM request.

  The TPM request may come from OS or BIOS. This API will display request information and wait 
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to 
  take effect.
  
  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request.  

  @param  PlatformAuth                   platform auth value. NULL means no platform auth change.
**/
VOID
EFIAPI
TrEEPhysicalPresenceLibProcessRequest (
  IN      TPM2B_AUTH                     *PlatformAuth  OPTIONAL
  );

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
  );

#endif
