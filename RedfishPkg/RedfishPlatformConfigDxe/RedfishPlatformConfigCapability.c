/** @file
  The implementation of EDKII Redfish Platform Config Capability.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishPlatformConfigDxe.h"
#include "RedfishPlatformConfigImpl.h"

/**
  Check if the debug property is enabled or not.

  @param[in]  DebugType  Debug enablement type

  @retval TRUE, the debug property is enabled.
          FALSE, the debug property is not enabled.

**/
BOOLEAN
RedfishPlatformConfigDebugProp (
  IN UINT64  DebugType
  )
{
  UINT64  DebugProp;

  DebugProp = FixedPcdGet64 (PcdRedfishPlatformConfigDebugProperty);
  if ((DebugProp & DebugType) != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check if the Platform Configure feature is enabled or not.

  @param[in]  FeatureType  Redfish platform config feature enablement

  @retval TRUE, the feature is enabled.
          FALSE, the feature is not enabled.

**/
BOOLEAN
RedfishPlatformConfigFeatureProp (
  IN UINT64  FeatureType
  )
{
  UINT64  FeatureProp;

  FeatureProp = FixedPcdGet64 (PcdRedfishPlatformConfigFeatureProperty);
  if ((FeatureProp & FeatureType) != 0) {
    return TRUE;
  }

  return FALSE;
}
