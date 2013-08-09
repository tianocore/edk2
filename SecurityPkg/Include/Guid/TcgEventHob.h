/** @file
  Defines the HOB GUID used to pass a TCG_PCR_EVENT from a TPM PEIM to 
  a TPM DXE Driver. A GUIDed HOB is generated for each measurement 
  made in the PEI Phase.
    
Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCG_EVENT_HOB_H_
#define _TCG_EVENT_HOB_H_

///
/// The Global ID of a GUIDed HOB used to pass a TCG_PCR_EVENT from a TPM PEIM to a TPM DXE Driver.
///
#define EFI_TCG_EVENT_HOB_GUID \
  { \
    0x2b9ffb52, 0x1b13, 0x416f, { 0xa8, 0x7b, 0xbc, 0x93, 0xd, 0xef, 0x92, 0xa8 } \
  }

extern EFI_GUID gTcgEventEntryHobGuid;

#endif
