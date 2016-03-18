/** @file
PiSmmCommunication private data structure

Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_COMMUNICATION_PRIVATE_H_
#define _SMM_COMMUNICATION_PRIVATE_H_

#pragma pack(push, 1)

#define SMM_COMMUNICATION_SIGNATURE SIGNATURE_32 ('S','M','M','C')

typedef struct {
  UINT32                   Signature;
  UINT32                   SwSmiNumber;
  EFI_PHYSICAL_ADDRESS     BufferPtrAddress;
} EFI_SMM_COMMUNICATION_CONTEXT;

#pragma pack(pop)

#endif
