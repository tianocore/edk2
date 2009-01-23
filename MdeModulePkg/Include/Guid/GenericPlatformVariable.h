/** @file
  This file defines the genenic platform guid for EFI variable.
  Common drivers without specific requirement 
  can use this guid with their variable name to specify 
  their EFI variable without defining another new guid.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __GENERIC_PLATFORM_VARIABLE_H__
#define __GENERIC_PLATFORM_VARIABLE_H__

#define EFI_GENERIC_PLATFORM_VARIABLE_GUID  \
  { 0x59d1c24f, 0x50f1, 0x401a, { 0xb1, 0x01, 0xf3, 0x3e, 0x0d, 0xae, 0xd4, 0x43 }}

extern EFI_GUID gEfiGenericPlatformVariableGuid;

#endif
