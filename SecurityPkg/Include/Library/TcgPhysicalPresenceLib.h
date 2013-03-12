/** @file
  Ihis library is intended to be used by BDS modules.
  This library will lock TPM after executing TPM request.

Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCG_PHYSICAL_PRESENCE_LIB_H_
#define _TCG_PHYSICAL_PRESENCE_LIB_H_

/**
  Check and execute the pending TPM request and Lock TPM.

  The TPM request may come from OS or BIOS. This API will display request information and wait 
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to 
  take effect. At last, it will lock TPM to prevent TPM state change by malware.
  
  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request. This API should also 
  be invoked as early as possible as TPM is locked in this function.
  
**/
VOID
EFIAPI
TcgPhysicalPresenceLibProcessRequest (
  VOID
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
TcgPhysicalPresenceLibNeedUserConfirm(
  VOID
  );

#endif
