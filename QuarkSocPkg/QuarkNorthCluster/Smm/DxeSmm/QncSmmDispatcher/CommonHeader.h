/** @file
Common header file shared by all source files.

This file includes package header files, library classes and protocol, PPI & GUID definitions.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_



#include <PiSmm.h>
#include <IntelQNCDxe.h>

#include <Protocol/SmmUsbDispatch2.h>
#include <Protocol/SmmPeriodicTimerDispatch2.h>
#include <Protocol/SmmIchnDispatch2.h>
#include <Protocol/SmmPowerButtonDispatch2.h>
#include <Protocol/SmmGpiDispatch2.h>
#include <Protocol/SmmSxDispatch2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmIoTrapDispatch2.h>
#include <Protocol/SmmCpu.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/DevicePathLib.h>
#include <Library/S3IoLib.h>
#include <Library/QNCAccessLib.h>

#include <Uefi/UefiBaseType.h>
#endif
