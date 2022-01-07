/** @file
  RISC-V SMBIOS Builder DXE module header file.

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISC_V_SMBIOS_DXE_H_
#define RISC_V_SMBIOS_DXE_H_

#include <PiDxe.h>
#include <Protocol/Smbios.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <ProcessorSpecificHobData.h>
#include <SmbiosProcessorSpecificData.h>
#endif
