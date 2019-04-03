/** @file
Common header file shared by all source files.

This file includes package header files, library classes and protocol, PPI & GUID definitions.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_



#include <PiDxe.h>
#include <IntelQNCDxe.h>
#include <IndustryStandard/Pci22.h>

#include <Guid/MdeModuleHii.h>
#include <Protocol/Smbios.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/PlatformPolicy.h>
#include <Protocol/MpService.h>

#include <Protocol/HiiString.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>

#include <Protocol/IdeControllerInit.h>
#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/S3IoLib.h>
#include <Library/S3PciLib.h>
#include <Library/DevicePathLib.h>

#endif
