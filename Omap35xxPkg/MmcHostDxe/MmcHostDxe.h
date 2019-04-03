/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _MMC_HOST_DXE_H_
#define _MMC_HOST_DXE_H_

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OmapLib.h>
#include <Library/OmapDmaLib.h>
#include <Library/DmaLib.h>

#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/MmcHost.h>

#include <Omap3530/Omap3530.h>
#include <TPS65950.h>

#define MAX_RETRY_COUNT  (100*5)

extern EFI_BLOCK_IO_PROTOCOL gBlockIo;

#endif
