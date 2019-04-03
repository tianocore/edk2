/** @file
  The header file of Coreboot Support PEIM.

Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __PEI_COREBOOT_SUPPORT_H__
#define __PEI_COREBOOT_SUPPORT_H__

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/CbParseLib.h>
#include <Library/MtrrLib.h>
#include <Library/IoLib.h>
#include <Library/CbPlatformSupportLib.h>

#include <Guid/SmramMemoryReserve.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/FrameBufferInfoGuid.h>
#include <Guid/SystemTableInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>

#include <Ppi/MasterBootMode.h>
#include "Coreboot.h"

typedef struct {
  UINT32  UsableLowMemTop;
  UINT32  SystemLowMemTop;
} CB_MEM_INFO;

#endif
