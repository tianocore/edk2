/** @file
  HddPassword PEI module which is used to unlock HDD password for S3.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HDD_PASSWORD_PEI_H_
#define _HDD_PASSWORD_PEI_H_

#include <PiPei.h>
#include <IndustryStandard/Atapi.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PciLib.h>
#include <Library/LockBoxLib.h>

#include <Ppi/AtaPassThru.h>

#include "HddPasswordCommon.h"


//
// Time out value for ATA PassThru PPI
//
#define ATA_TIMEOUT                          30000000

#endif
