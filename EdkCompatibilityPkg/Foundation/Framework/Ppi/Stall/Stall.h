/*++

  Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
  
Module Name:

  Stall.h
    
Abstract:

  Stall PPI 

--*/

#ifndef _PEI_STALL_PPI_H_
#define _PEI_STALL_PPI_H_

#define PEI_STALL_PPI_GUID \
  { \
    0x1f4c6f90, 0xb06b, 0x48d8, {0xa2, 0x01, 0xba, 0xe5, 0xf1, 0xcd, 0x7d, 0x56} \
  }

EFI_FORWARD_DECLARATION (PEI_STALL_PPI);

typedef
EFI_STATUS
(EFIAPI *PEI_STALL) (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN PEI_STALL_PPI              * This,
  IN UINTN                      Microseconds
  );

struct _PEI_STALL_PPI {
  UINTN     Resolution;
  PEI_STALL Stall;
};

extern EFI_GUID gPeiStallPpiGuid;

#endif
