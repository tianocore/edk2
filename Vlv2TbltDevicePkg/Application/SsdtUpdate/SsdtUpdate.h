/** @file
  The header file for update SSDT table to ACPI table.
  
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials 
  are licensed and made available under the terms and conditions of the BSD License 
  which accompanies this distribution.  The full text of the license may be found at 
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>

#include <Protocol/AcpiTable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#define INPUT_STRING_LEN    255
#define INPUT_STRING_SIZE   (INPUT_STRING_LEN + 1)

typedef struct {
  BOOLEAN   UpdateFromFile;
  CHAR16    FileName[INPUT_STRING_SIZE];
} FV_INPUT_DATA;

