/** @file
  Event group triggered when Memory Protection Compatibility Mode is activated.
  Platforms can hook into this event to perform follow up actions.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#define COMPATIBILITY_MODE_ACTIVATED_EVENT_GUID \
   { 0x209BA820, 0xCE32, 0x49E1, { 0x95, 0xAE, 0x39, 0x43, 0x3D, 0xEF, 0xD5, 0xEE } }

extern EFI_GUID  gCompatibilityModeActivatedEventGuid;
