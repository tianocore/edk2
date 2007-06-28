/**
  The internal header file for EdkFvbServiceLib.

Copyright (c) 2006 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

**/

#ifndef __FVB_H__
#define __FVB_H__

//
// The package level header files this module uses
//
#include <PiDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/FvbExtension.h>
//
// The Library classes this module consumes
//
#include <Library/FvbServiceLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define MAX_FVB_COUNT 16

typedef struct {
  EFI_HANDLE                          Handle;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_EXTENSION_PROTOCOL          *FvbExtension;
  BOOLEAN                             IsRuntimeAccess;
} FVB_ENTRY;

#endif
