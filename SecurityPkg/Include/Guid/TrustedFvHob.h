/** @file
  Defines the HOB GUID used to pass all PEI trusted FV info to 
  DXE Driver.
    
Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TRUSTED_FV_HOB_H_
#define _TRUSTED_FV_HOB_H_

///
/// The Global ID of a GUIDed HOB used to pass all PEI trusted FV info to DXE Driver.
///
#define EFI_TRUSTED_FV_HOB_GUID \
  { \
    0xb2360b42, 0x7173, 0x420a, { 0x86, 0x96, 0x46, 0xca, 0x6b, 0xab, 0x10, 0x60 } \
  }

extern EFI_GUID gTrustedFvHobGuid;

#endif
