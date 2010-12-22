/** @file
  Define Name, GUID and data format for an EFI PCD that is used to save the image base and size
  of a code segment which will be loaded and executed by a boot script thunk on S3 boot path.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BOOT_SCRIPT_THUNK_VARIABLE_H_
#define _BOOT_SCRIPT_THUNK_VARIABLE_H_

//
// The following structure boosts performance by combining structure all ACPI related variables into one.
//
#pragma pack(1)

typedef struct {
  EFI_PHYSICAL_ADDRESS  BootScriptThunkBase;
  EFI_PHYSICAL_ADDRESS  BootScriptThunkLength;
} BOOT_SCRIPT_THUNK_DATA;

#pragma pack()

#endif
