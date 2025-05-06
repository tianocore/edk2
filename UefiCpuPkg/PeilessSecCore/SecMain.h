/** @file
  Master header file for SecCore.

  Copyright (c) 2008 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025 Ventana Micro Systems Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SEC_MAIN_H_
#define _SEC_MAIN_H_

#include <PiPei.h>
#include <Ppi/SecPlatformInformation.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/PlatformSecLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/HobLib.h>

/**
  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param SizeOfRam           Size of the temporary memory available for use.
  @param TempRamBase         Base address of temporary ram
  @param BootFirmwareVolume  Base address of the Boot Firmware Volume.
**/
VOID
NORETURN
EFIAPI
SecStartup (
  IN UINT32  SizeOfRam,
  IN UINT32  TempRamBase,
  IN VOID    *BootFirmwareVolume
  );

#endif /* _SEC_MAIN_H_ */
