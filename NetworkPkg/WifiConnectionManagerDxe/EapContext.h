/** @file
  Eap configuration data structure definitions for EAP connections.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_WIFI_EAP_CONTEXT_H__
#define __EFI_WIFI_EAP_CONTEXT_H__

typedef struct {

  BOOLEAN                   IsEncrypted;
  CHAR16                    EncryptPassword[PASSWORD_STORAGE_SIZE];
  UINTN                     KeySize;
  UINT8                     KeyData[1];

} EFI_EAP_PRIVATE_KEY;

#endif
