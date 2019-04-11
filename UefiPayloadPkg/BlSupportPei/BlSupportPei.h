/** @file
  The header file of bootloader support PEIM.

Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __PEI_BOOTLOADER_SUPPORT_H__
#define __PEI_BOOTLOADER_SUPPORT_H__

#include <PiPei.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/BlParseLib.h>
#include <Library/MtrrLib.h>
#include <Library/IoLib.h>
#include <Library/PlatformSupportLib.h>
#include <IndustryStandard/Acpi.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemTableInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/GraphicsInfoHob.h>
#include <Ppi/MasterBootMode.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>

typedef struct {
  UINT32  UsableLowMemTop;
  UINT32  SystemLowMemTop;
} PAYLOAD_MEM_INFO;

#endif
