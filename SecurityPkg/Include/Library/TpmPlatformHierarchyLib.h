/** @file
    TPM Platform Hierarchy configuration library.

    This library provides functions for customizing the TPM's Platform Hierarchy
    Authorization Value (platformAuth) and Platform Hierarchy Authorization
    Policy (platformPolicy) can be defined through this function.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPM_PLATFORM_HIERARCHY_LIB_H_
#define TPM_PLATFORM_HIERARCHY_LIB_H_

/**
   This service will perform the TPM Platform Hierarchy configuration at the SmmReadyToLock event.

**/
VOID
EFIAPI
ConfigureTpmPlatformHierarchy (
  VOID
  );

#endif
