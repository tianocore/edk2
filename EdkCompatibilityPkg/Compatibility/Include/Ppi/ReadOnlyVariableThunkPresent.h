/** @file
  PPI published by the ReadOnlyVariable to ReadOnlyVariable2 or ReadOnlyVariable2 to ReadOnlyVariable
  thunking PEIM to mark that such module has already been excuted.

  This PPI is referenced in Edk2/EdkCompatibility/Compatibility/ReadOnlyVariable2ToReadOnlyVariable 
  and Edk2/EdkCompatibility/Compatibility/ReadOnlyVariable2ToReadOnlyVariable.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_READONLY_VARIABLE_THUNK_PRESENT_H_
#define _PEI_READONLY_VARIABLE_THUNK_PRESENT_H_

#define PEI_READONLY_VARIABLE_THUNK_PRESENT_GUID  \
  { 0xe84e9e0b, 0xb5dd, 0x4d08, { 0x9e, 0x46, 0x82, 0x1f, 0xf, 0x14, 0xe9, 0x1b } }

extern EFI_GUID                     gPeiReadonlyVariableThunkPresentPpiGuid;

#endif

