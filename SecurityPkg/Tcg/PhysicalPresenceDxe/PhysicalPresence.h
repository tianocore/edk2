/** @file
  The header file for TPM physical presence driver.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PHYSICAL_PRESENCE_H__
#define __PHYSICAL_PRESENCE_H__

#include <PiDxe.h>

#include <Protocol/TcgService.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Guid/EventGroup.h>
#include <Guid/PhysicalPresenceData.h>

#define TPM_PP_USER_ABORT           ((TPM_RESULT)(-0x10))
#define TPM_PP_BIOS_FAILURE         ((TPM_RESULT)(-0x0f))

#define CONFIRM_BUFFER_SIZE                    4096

#endif
