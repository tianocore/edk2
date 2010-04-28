/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCode.h
    
Abstract:

  Status Code PPI as defined in Tiano

--*/

#ifndef _PEI_STATUS_CODE_PPI_H
#define _PEI_STATUS_CODE_PPI_H

#define PEI_STATUS_CODE_PPI_GUID \
  { \
    0x229832d3, 0x7a30, 0x4b36, {0xb8, 0x27, 0xf4, 0xc, 0xb7, 0xd4, 0x54, 0x36} \
  }

EFI_FORWARD_DECLARATION (PEI_STATUS_CODE_PPI);

typedef
EFI_STATUS
(EFIAPI *PEI_REPORT_STATUS_CODE) (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN EFI_STATUS_CODE_TYPE            CodeType,
  IN EFI_STATUS_CODE_VALUE           Value,
  IN UINT32                          Instance,
  IN EFI_GUID                        * CallerId,
  IN EFI_STATUS_CODE_DATA            * Data OPTIONAL
  );

struct _PEI_STATUS_CODE_PPI {
  PEI_REPORT_STATUS_CODE  ReportStatusCode;
};

extern EFI_GUID gPeiStatusCodePpiGuid;

#endif
