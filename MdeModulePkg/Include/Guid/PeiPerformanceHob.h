/** @file
  This file defines performance guid for the performance entry in the HOB list, 
  and define the PEI Performance HOB data structures.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PEI_PERFORMANCE_HOB_H__
#define __PEI_PERFORMANCE_HOB_H__

//
// This is the GUID of PEI performance HOB
//
#define PEI_PERFORMANCE_HOB_GUID \
  { 0xec4df5af, 0x4395, 0x4cc9, { 0x94, 0xde, 0x77, 0x50, 0x6d, 0x12, 0xc7, 0xb8 } }

//
// PEI_PERFORMANCE_STRING_SIZE must be a multiple of 8.
//
#define PEI_PERFORMANCE_STRING_SIZE     8
#define PEI_PERFORMANCE_STRING_LENGTH   (PEI_PERFORMANCE_STRING_SIZE - 1)

typedef struct {
  EFI_PHYSICAL_ADDRESS  Handle;
  CHAR8                 Token[PEI_PERFORMANCE_STRING_SIZE];
  CHAR8                 Module[PEI_PERFORMANCE_STRING_SIZE];
  UINT64                StartTimeStamp;
  UINT64                EndTimeStamp;
} PEI_PERFORMANCE_LOG_ENTRY;

//
// The header must be aligned at 8 bytes.
// 
typedef struct {
  UINT32                             NumberOfEntries;
  UINT32                             Reserved;
} PEI_PERFORMANCE_LOG_HEADER;


extern EFI_GUID gPeiPerformanceHobGuid;

#endif
