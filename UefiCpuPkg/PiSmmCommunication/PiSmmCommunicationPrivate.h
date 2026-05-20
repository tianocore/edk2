/** @file
PiSmmCommunication private data structure

Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#pragma pack(push, 1)

#define SMM_COMMUNICATION_SIGNATURE  SIGNATURE_32 ('S','M','M','C')

typedef struct {
  UINT32                  Signature;
  UINT32                  SwSmiNumber;
  EFI_PHYSICAL_ADDRESS    BufferPtrAddress;
} EFI_SMM_COMMUNICATION_CONTEXT;

#pragma pack(pop)
