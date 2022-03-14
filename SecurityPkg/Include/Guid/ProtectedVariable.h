/** @file
  The GUID definitions specific for protected variable services.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PROTECTED_VARIABLE_H__
#define PROTECTED_VARIABLE_H__

#define EDKII_PROTECTED_VARIABLE_GLOBAL_GUID \
  { 0x8ebf379a, 0xf18e, 0x4728, { 0xa4, 0x10, 0x0, 0xcf, 0x9a, 0x65, 0xbe, 0x91 } }

#define EDKII_METADATA_HMAC_VARIABLE_GUID \
  { 0xb54cda50, 0xec54, 0x4b20, { 0x85, 0xb4, 0x57, 0xbf, 0x52, 0x98, 0x68, 0x3d } }

extern EFI_GUID  gEdkiiProtectedVariableGlobalGuid;
extern EFI_GUID  gEdkiiMetaDataHmacVariableGuid;
extern EFI_GUID  gEdkiiProtectedVariableContextGuid;

#endif // __PROTECTED_VARIABLE_H__
