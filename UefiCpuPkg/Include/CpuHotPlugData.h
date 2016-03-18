/** @file
Definition for a structure sharing information for CPU hot plug.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_HOT_PLUG_DATA_H_
#define _CPU_HOT_PLUG_DATA_H_

#define  CPU_HOT_PLUG_DATA_REVISION_1      0x00000001

typedef struct {
  UINT32    Revision;          // Used for version identification for this structure
  UINT32    ArrayLength;       // The entries number of the following ApicId array and SmBase array
  //
  // Data required for SMBASE relocation
  //
  UINT64    *ApicId;           // Pointer to ApicId array
  UINTN     *SmBase;           // Pointer to SmBase array
  UINT32    Reserved;
  UINT32    SmrrBase;
  UINT32    SmrrSize;
} CPU_HOT_PLUG_DATA;

#endif
