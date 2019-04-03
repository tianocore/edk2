/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_


#include <Base.h>

#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

#define R_PCH_ACPI_SMI_EN       0x30
#define B_PCH_ACPI_APMC_EN      0x00000020
#define B_PCH_ACPI_EOS          0x00000002
#define B_PCH_ACPI_GBL_SMI_EN   0x00000001
#define R_PCH_ACPI_SMI_STS      0x34
#define B_PCH_ACPI_APM_STS      0x00000020

#endif
