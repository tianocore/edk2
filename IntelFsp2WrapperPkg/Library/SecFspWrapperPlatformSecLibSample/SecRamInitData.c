/** @file
  Sample to provide TempRamInitParams data.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/PcdLib.h>
#include <FspEas.h>

typedef struct {
  UINT32                      MicrocodeRegionBase;
  UINT32                      MicrocodeRegionSize;
  UINT32                      CodeRegionBase;
  UINT32                      CodeRegionSize;
} FSPT_CORE_UPD;

typedef struct {
  FSP_UPD_HEADER    FspUpdHeader;
  FSPT_CORE_UPD     FsptCoreUpd;
} FSPT_UPD_CORE_DATA;

GLOBAL_REMOVE_IF_UNREFERENCED CONST FSPT_UPD_CORE_DATA FsptUpdDataPtr = {
  {
    0x4450555F54505346,
    0x00,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    ((UINT32)FixedPcdGet64 (PcdCpuMicrocodePatchAddress) + FixedPcdGet32 (PcdFlashMicrocodeOffset)),
    ((UINT32)FixedPcdGet64 (PcdCpuMicrocodePatchRegionSize) - FixedPcdGet32 (PcdFlashMicrocodeOffset)),
    FixedPcdGet32 (PcdFlashCodeCacheAddress),
    FixedPcdGet32 (PcdFlashCodeCacheSize),
  }
};

