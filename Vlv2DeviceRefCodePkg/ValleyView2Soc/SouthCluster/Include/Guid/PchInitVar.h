/*++

Copyright (c) 2011  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  PchInitVar.h

Abstract:

  This file defines variable shared between PCH Init DXE driver and PCH
  Init S3 Resume PEIM.

--*/
#ifndef _PCH_INIT_VAR_H_
#define _PCH_INIT_VAR_H_

#include <Protocol/PchPlatformPolicy.h>
//
// Define the PCH Init Var GUID
//
#define PCH_INIT_VARIABLE_GUID {0xe6c2f70a, 0xb604, 0x4877,{0x85, 0xba, 0xde, 0xec, 0x89, 0xe1, 0x17, 0xeb}}
//
// Extern the GUID for PPI users.
//
extern EFI_GUID gPchInitVariableGuid;

#define PCH_INIT_VARIABLE_NAME  L"PchInit"

//
// Define the Pch Init Variable structure
//
typedef struct {
  UINT32  StorePosition;
  UINT32  ExecutePosition;
} PCH_S3_PARAMETER_HEADER;

#pragma pack(1)
typedef struct _PCH_INIT_VARIABLE {
  PCH_S3_PARAMETER_HEADER *PchS3Parameter;
} PCH_INIT_VARIABLE;
#pragma pack()

#endif
