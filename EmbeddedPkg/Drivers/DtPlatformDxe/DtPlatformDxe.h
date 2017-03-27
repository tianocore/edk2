/** @file
*
*  Copyright (c) 2017, Linaro Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __DT_PLATFORM_DXE_H__
#define __DT_PLATFORM_DXE_H__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/DtPlatformFormSet.h>

#define DT_ACPI_SELECT_DT       0x0
#define DT_ACPI_SELECT_ACPI     0x1

#define DT_ACPI_VARIABLE_NAME   L"DtAcpiPref"

typedef struct {
  UINT8         Pref;
  UINT8         Reserved[3];
} DT_ACPI_VARSTORE_DATA;

#endif
