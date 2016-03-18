/** @file
  GUID used as HII Package list GUID in UsbCredentialProviderDxe driver.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __USB_CREDENTIAL_PROVIDER_HII_H__
#define __USB_CREDENTIAL_PROVIDER_HII_H__

//
// Used for save password credential and form browser
// And used as provider identifier
//
#define USB_CREDENTIAL_PROVIDER_GUID \
  { \
    0xd0849ed1, 0xa88c, 0x4ba6, { 0xb1, 0xd6, 0xab, 0x50, 0xe2, 0x80, 0xb7, 0xa9 }\
  }

extern EFI_GUID gUsbCredentialProviderGuid;

#endif
