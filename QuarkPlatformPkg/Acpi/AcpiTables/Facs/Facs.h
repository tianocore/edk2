/** @file
This file describes the contents of the ACPI Firmware ACPI Control Structure (FACS)
.  Some additional ACPI values are defined in Acpi10.h, Acpi20.h, and Acpi30.h
All changes to the FACS contents should be done in this file.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FACS_H_
#define _FACS_H_

//
// Statements that include other files
//
#include <IndustryStandard/Acpi.h>

//
// FACS Definitions
//
#define EFI_ACPI_FIRMWARE_WAKING_VECTOR 0x00000000
#define EFI_ACPI_GLOBAL_LOCK 0x00000000

#define EFI_ACPI_FIRMWARE_CONTROL_STRUCTURE_FLAGS 0x00000000
#define EFI_ACPI_X_FIRMWARE_WAKING_VECTOR 0x0000000000000000

#endif
