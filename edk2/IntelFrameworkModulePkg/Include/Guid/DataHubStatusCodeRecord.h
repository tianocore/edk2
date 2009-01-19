/** @file
  GUID used to identify Data Hub records that originate from the Tiano 
  ReportStatusCode API.
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#ifndef __STATUS_CODE_H__
#define __STATUS_CODE_H__

#define EFI_DATA_HUB_STATUS_CODE_RECORD_GUID \
  { \
    0xd083e94c, 0x6560, 0x42e4, {0xb6, 0xd4, 0x2d, 0xf7, 0x5a, 0xdf, 0x6a, 0x2a } \
  }

typedef struct {
  EFI_STATUS_CODE_TYPE  CodeType;
  EFI_STATUS_CODE_VALUE Value;
  UINT32                Instance;
  EFI_GUID              CallerId;
  EFI_STATUS_CODE_DATA  Data;
} DATA_HUB_STATUS_CODE_DATA_RECORD;

extern EFI_GUID gEfiDataHubStatusCodeRecordGuid;

#endif // __STATUS_CODE_H__

