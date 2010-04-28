/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PeiPerformanceHob.h
    
Abstract:
  The PEI performance HOB definition.

--*/

#ifndef _PEI_PERFORMANCE_HOB_GUID_H_
#define _PEI_PERFORMANCE_HOB_GUID_H_


#define EFI_PEI_PERFORMANCE_HOB_GUID  \
{0x10f432de, 0xdeec, 0x4631, {0x80, 0xcd, 0x47, 0xf6, 0x5d, 0x8f, 0x80, 0xbb}}

#define PEI_PERF_MAX_DESC_STRING 8

typedef struct {
  UINT64          StartTimeCount;
  UINT64          StopTimeCount;
  EFI_GUID        Name;
  UINT16          DescriptionString[PEI_PERF_MAX_DESC_STRING];
} PEI_PERFORMANCE_MEASURE_LOG_ENTRY;

typedef struct {
  UINT32                             NumberOfEntries;
  UINT32                             Reserved;
  PEI_PERFORMANCE_MEASURE_LOG_ENTRY  Log[1];
} EFI_HOB_GUID_DATA_PERFORMANCE_LOG;

extern EFI_GUID gEfiPeiPerformanceHobGuid;

#endif
