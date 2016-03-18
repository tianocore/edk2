/** @file
  Define Name, GUID and data format for an EFI Variable that is used to save the entry point
  of a code segment which will be loaded and executed by a standalone boot script 
  executor on S3 boot path.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BOOT_SCRIPT_EXECUTOR_VARIABLE_H_
#define _BOOT_SCRIPT_EXECUTOR_VARIABLE_H_

#define EFI_BOOT_SCRIPT_EXECUTOR_VARIABLE_GUID \
  { \
    0x3079818c, 0x46d4, 0x4a73, {0xae, 0xf3, 0xe3, 0xe4, 0x6c, 0xf1, 0xee, 0xdb} \
  }

//
// The following structure boosts performance by combining structure all ACPI related variables into one.
//
#pragma pack(1)

typedef struct {
  EFI_PHYSICAL_ADDRESS  BootScriptExecutorEntrypoint;
} BOOT_SCRIPT_EXECUTOR_VARIABLE;

#pragma pack()

#define BOOT_SCRIPT_EXECUTOR_VARIABLE_NAME  L"BootScriptExecutorVariable"

extern EFI_GUID gEfiBootScriptExecutorVariableGuid;

#define EFI_BOOT_SCRIPT_EXECUTOR_CONTEXT_GUID \
  { \
    0x79cb58c4, 0xac51, 0x442f, {0xaf, 0xd7, 0x98, 0xe4, 0x7d, 0x2e, 0x99, 0x8} \
  }

extern EFI_GUID gEfiBootScriptExecutorContextGuid;

#endif
