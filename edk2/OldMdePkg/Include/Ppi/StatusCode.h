/** @file
  This file declares Status Code PPI.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  StatusCode.h

  @par Revision Reference:
  This PPI is defined in PEI CIS.
  Version 0.91.

**/

#ifndef __STATUS_CODE_PPI_H__
#define __STATUS_CODE_PPI_H__

#define EFI_PEI_REPORT_PROGRESS_CODE_PPI_GUID \
  { 0x229832d3, 0x7a30, 0x4b36, {0xb8, 0x27, 0xf4, 0xc, 0xb7, 0xd4, 0x54, 0x36 } }

/**
  @par Ppi Description:
  This ppi provides the sevice to report status code. There can be only one instance 
  of this service in the system.

  @param ReportStatusCode
  Service that allows PEIMs to report status codes. This function is defined in Peicis.h

**/
typedef struct {
  EFI_PEI_REPORT_STATUS_CODE  ReportStatusCode;
} EFI_PEI_PROGRESS_CODE_PPI;

extern EFI_GUID gEfiPeiStatusCodePpiGuid;

#endif
