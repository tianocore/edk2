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
#ifndef _TCG_PHYSICAL_PRESENCE_STORAGE_LIB_INTENAL_H_
#define _TCG_PHYSICAL_PRESENCE_STORAGE_LIB_INTENAL_H_
/**
  Display the confirm text and get user confirmation.

  @param[in] TpmPpCommand             The requested TPM physical presence command.

  @retval    TRUE          The user has confirmed the changes.
  @retval    FALSE         The user doesn't confirm the changes.
**/
BOOLEAN
TcgPpUserConfirm (
  IN      UINT8                     TpmPpCommand
  );


#endif

