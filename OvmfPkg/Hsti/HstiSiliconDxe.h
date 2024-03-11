/** @file
  This file contains the required header files for the HSTI Silicon DXE driver

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HSTI_SILICON_DXE_H_
#define _HSTI_SILICON_DXE_H_

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/Hsti.h>
#include <Protocol/AdapterInformation.h>
#include <Library/HstiLib.h>
#include <HstiFeatureBit.h>

#pragma pack(1)
typedef struct {
  UINT32  Version;
  UINT32  Role;
  CHAR16  ImplementationID[256];
  UINT32  SecurityFeaturesSize;
  UINT8   SecurityFeaturesRequired[HSTI_SECURITY_FEATURE_SIZE];
  UINT8   SecurityFeaturesImplemented[HSTI_SECURITY_FEATURE_SIZE];
  UINT8   SecurityFeaturesVerified[HSTI_SECURITY_FEATURE_SIZE];
  CHAR16  End;
} ADAPTER_INFO_PLATFORM_SECURITY_STRUCT;
#pragma pack()

extern UINT8  mFeatureImplemented[HSTI_SECURITY_FEATURE_SIZE];
extern EFI_GUID gHstiProtocolGuid;
extern EFI_GUID gHstiPublishCompleteProtocolGuid;

#endif
