/** @file
  PlatformId internal header for PlatformPei

  Copyright (c) 2024, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLATFORM_PEI_PLATFORMID_H__
#define __PLATFORM_PEI_PLATFORMID_H__

/**
 * Reads opt/org.tianocode/sp800155evt/%d from 0 to the first positive integer
 * where the file does not exist and registers each file's contents in an
 * EFI_HOB_GUID_TYPE with name gTcg800155PlatformIdEventHobGuid. These HOBs
 * are used by a later driver to write to the event log as unmeasured events.
 * These events inform the event log analyzer of firmware provenance and
 * reference integrity manifests.
**/
VOID
PlatformIdInitialization (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  );

#endif // __PLATFORM_PEI_PLATFORMID_H__
