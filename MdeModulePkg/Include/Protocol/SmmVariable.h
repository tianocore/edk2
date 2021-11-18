/** @file
  EFI SMM Variable Protocol is related to EDK II-specific implementation of variables
  and intended for use as a means to store data in the EFI SMM environment.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_VARIABLE_H__
#define __SMM_VARIABLE_H__

#define EFI_SMM_VARIABLE_PROTOCOL_GUID \
  { \
    0xed32d533, 0x99e6, 0x4209, { 0x9c, 0xc0, 0x2d, 0x72, 0xcd, 0xd9, 0x98, 0xa7 } \
  }

typedef struct _EFI_SMM_VARIABLE_PROTOCOL EFI_SMM_VARIABLE_PROTOCOL;

///
/// EFI SMM Variable Protocol is intended for use as a means
/// to store data in the EFI SMM environment.
///
struct _EFI_SMM_VARIABLE_PROTOCOL {
  EFI_GET_VARIABLE              SmmGetVariable;
  EFI_GET_NEXT_VARIABLE_NAME    SmmGetNextVariableName;
  EFI_SET_VARIABLE              SmmSetVariable;
  EFI_QUERY_VARIABLE_INFO       SmmQueryVariableInfo;
};

extern EFI_GUID  gEfiSmmVariableProtocolGuid;

#endif
