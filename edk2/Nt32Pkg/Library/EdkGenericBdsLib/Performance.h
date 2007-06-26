/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Performance.h

Abstract:

  This file included the performance relete function header and 
  definition.

--*/

#ifndef _PERF_H_
#define _PERF_H_

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#define PERF_TOKEN_LENGTH       28
#define PERF_PEI_ENTRY_MAX_NUM  50

typedef struct {
  CHAR8   Token[PERF_TOKEN_LENGTH];
  UINT32  Duration;
} PERF_DATA;

typedef struct {
  UINT64        BootToOs;
  UINT64        S3Resume;
  UINT32        S3EntryNum;
  PERF_DATA     S3Entry[PERF_PEI_ENTRY_MAX_NUM];
  UINT64        CpuFreq;
  UINT64        BDSRaw;
  UINT32        Count;
  UINT32        Signiture;
} PERF_HEADER;

VOID
WriteBootToOsPerformanceData (
  VOID
  );

VOID
ClearDebugRegisters (
  VOID
  );

#endif
