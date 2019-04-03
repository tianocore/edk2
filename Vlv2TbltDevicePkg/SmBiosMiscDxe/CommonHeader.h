/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_



#include <FrameworkDxe.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>

#include <Guid/DataHubRecords.h>
#include <Guid/MdeModuleHii.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <PchRegs.h>
#include <Library/PchPlatformLib.h>
#include <Library/PrintLib.h>

#endif
