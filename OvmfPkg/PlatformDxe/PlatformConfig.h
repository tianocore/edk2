/** @file

  Utility functions for serializing (persistently storing) and deserializing
  OVMF's platform configuration.

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_CONFIG_H_
#define _PLATFORM_CONFIG_H_

#include <Base.h>

//
// This structure participates in driver configuration. It does not
// (necessarily) reflect the wire format in the persistent store.
//
#pragma pack(1)
typedef struct {
  //
  // preferred graphics console resolution when booting
  //
  UINT32    HorizontalResolution;
  UINT32    VerticalResolution;
} PLATFORM_CONFIG;
#pragma pack()

//
// Please see the API documentation near the function definitions.
//
EFI_STATUS
EFIAPI
PlatformConfigSave (
  IN PLATFORM_CONFIG  *PlatformConfig
  );

EFI_STATUS
EFIAPI
PlatformConfigLoad (
  OUT PLATFORM_CONFIG  *PlatformConfig,
  OUT UINT64           *OptionalElements
  );

//
// Feature flags for OptionalElements.
//
#define PLATFORM_CONFIG_F_GRAPHICS_RESOLUTION  BIT0
#define PLATFORM_CONFIG_F_DOWNGRADE            BIT63

#endif // _PLATFORM_CONFIG_H_
