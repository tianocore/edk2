/** @file
  GUID used as HII FormSet and HII Package list GUID in PwdCredentialProviderDxe driver.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PWD_CREDENTIAL_PROVIDER_HII_H__
#define __PWD_CREDENTIAL_PROVIDER_HII_H__

//
// Used for save password credential and form browser.
// Also used as provider identifier.
//
#define PWD_CREDENTIAL_PROVIDER_GUID \
  { \
    0x78b9ec8b, 0xc000, 0x46c5, { 0xac, 0x93, 0x24, 0xa0, 0xc1, 0xbb, 0x0, 0xce } \
  }

extern EFI_GUID gPwdCredentialProviderGuid;

#endif
