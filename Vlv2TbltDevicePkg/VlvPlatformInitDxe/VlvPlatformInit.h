
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  VlvPlatformInit.h

Abstract:

  Header file for SA Initialization Driver.

--*/

#ifndef _VLV_PLATFORM_INIT_DXE_H_
#define _VLV_PLATFORM_INIT_DXE_H_
#include "PiDxe.h"

#include <Protocol/VlvPlatformPolicy.h>

#include "IgdOpRegion.h"

#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include "Library/DebugLib.h"
#include "Library/S3IoLib.h"
#include "Library/S3PciLib.h"
#include "Library/IoLib.h"
#include "Library/PciLib.h"
#include "Library/S3BootScriptLib.h"

//
// GT RELATED EQUATES
//
#define GTT_MEM_ALIGN        22
#define GTTMMADR_SIZE_4MB    0x400000

#define IGD_BUS             0x00
#define IGD_DEV                  0x02
#define IGD_FUN_0                0x00

#define IGD_R_VID                0x00
#define IGD_R_CMD                0x04
#define IGD_R_GTTMMADR           0x10

#define IGD_R_BGSM               0x70
#define LockBit                  BIT0

#define IGD_VID             0x8086
#define IGD_DID             0xA001
#define IGD_MGGC_OFFSET     0x0050      //GMCH Graphics Control Register 0x50
#define IGD_BSM_OFFSET      0x005C      //Base of Stolen Memory
#define IGD_SWSCI_OFFSET    0x00E0      //Software SCI 0xE0 2
#define IGD_ASLE_OFFSET     0x00E4      //System Display Event Register 0xE4 4
#define IGD_ASLS_OFFSET     0x00FC      // ASL Storage

#endif

