/** @file
  Eap configuration data structure definitions for EAP connections.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
