/** @file
  Library that builds the DXE and MM memory protection settings HOBs based on a
  QEMU fw_cfg selector.

  The fw_cfg file "opt/org.tianocore/MemoryProtection" contains the name of the
  memory protection profile to apply. If the fw_cfg file is not present, no HOB
  is produced and the memory protection PCDs are used as a fallback.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

/**
  Build the DXE memory protection settings HOB based on a QEMU fw_cfg selector.

  The fw_cfg file "opt/org.tianocore/MemoryProtection" contains the name of the
  memory protection profile to apply (e.g. "DEBUG" selects
  DXE_MEMORY_PROTECTION_SETTINGS_DEBUG). If the fw_cfg file is not present, no
  HOB is produced and the memory protection PCDs are used as a fallback.
**/
VOID
EFIAPI
PlatformBuildMemoryProtectionSettingsHob (
  VOID
  );

/**
  Build the MM (SMM / Standalone MM) memory protection settings HOB based on a
  QEMU fw_cfg selector.

  The fw_cfg file "opt/org.tianocore/MemoryProtection" contains the name of the
  memory protection profile to apply (e.g. "DEBUG" selects
  MM_MEMORY_PROTECTION_SETTINGS_DEBUG). If the fw_cfg file is not present, no
  HOB is produced and the memory protection PCDs are used as a fallback.

  This should only be called on platforms that use traditional SMM or
  Standalone MM.
**/
VOID
EFIAPI
PlatformBuildMmMemoryProtectionSettingsHob (
  VOID
  );
