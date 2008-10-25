/** @file 
  The internal header file includes the common header files, defines
  internal structure FVB_ENTRY.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>

All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FVB_H__
#define __FVB_H__


#include <PiDxe.h>

#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/FvbExtension.h>

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
