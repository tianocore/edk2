/*++

  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
--*/

#ifndef _SEC_FTPM_POLICY_PPI_H_
#define _SEC_FTPM_POLICY_PPI_H_

#define SEC_FTPM_POLICY_PPI_GUID \
  { \
    0x4fd1ba49, 0x8f90, 0x471a, 0xa2, 0xc9, 0x17, 0x3c, 0x7a, 0x73, 0x2f, 0xd0 \
  }

extern EFI_GUID  gSeCfTPMPolicyPpiGuid;

//
// PPI definition
//
typedef struct SEC_FTPM_POLICY_PPI {
  BOOLEAN                 fTPMEnable;
} SEC_FTPM_POLICY_PPI;

#endif
