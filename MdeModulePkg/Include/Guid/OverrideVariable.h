/** @file
  This file defines the platform driver override variable name and variable guid.
  The variable(s) contain the override mappings from Controller Device Path to a set of Driver Device Paths.
  
  VariableLayout {
  //
  // NotEnd indicate whether the variable is the last one, and has no subsequent variable need to load.
  // Each variable has MaximumVariableSize limitation, so multiple variables are required to store
  // large mapping infos.
  // The variable(s) name rule is PlatDriOver, PlatDriOver1, PlatDriOver2, ....
  //
  UINT32                         NotEnd;               //Zero is the last one.
  //
  // The entry which contains the mapping that Controller Device Path to a set of Driver Device Paths
  // There are often multi mapping entries in a variable.
  //
  UINT32                         SIGNATURE;            //SIGNATURE_32('p','d','o','i')
  UINT32                         DriverNum;
  EFI_DEVICE_PATH_PROTOCOL       ControllerDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  ......
  UINT32                         NotEnd;                //Zero is the last one.
  UINT32                         SIGNATURE;
  UINT32                         DriverNum;
  EFI_DEVICE_PATH_PROTOCOL       ControllerDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  ......
  }

Copyright (c) 2008 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_OVERRIDE_VARIABLE_GUID_H__
#define __EFI_OVERRIDE_VARIABLE_GUID_H__

///
/// This guid is used for a platform driver override variable
///
#define EFI_OVERRIDE_VARIABLE_GUID  \
  { 0x8e3d4ad5, 0xf762, 0x438a, { 0xa1, 0xc1, 0x5b, 0x9f, 0xe6, 0x8c, 0x6b, 0x15 }}

#define EFI_PLATFORM_OVERRIDE_VARIABLE_NAME L"PlatDriOver"

extern EFI_GUID gEfiOverrideVariableGuid;

#endif // #ifndef __EFI_OVERRIDE_VARIABLE_GUID_H__
