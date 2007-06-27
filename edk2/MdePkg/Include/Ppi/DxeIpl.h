/** @file
  This file declares DXE Initial Program Load PPI.
  When the PEI core is done it calls the DXE IPL via this PPI.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  This PPI is defined in PI.
  Version 1.0.

**/

#ifndef __DXE_IPL_H__
#define __DXE_IPL_H__

#define EFI_DXE_IPL_PPI_GUID \
  { \
    0xae8ce5d, 0xe448, 0x4437, {0xa8, 0xd7, 0xeb, 0xf5, 0xf1, 0x94, 0xf7, 0x31 } \
  }

typedef struct _EFI_DXE_IPL_PPI EFI_DXE_IPL_PPI;

/**
  The architectural PPI that the PEI Foundation invokes when 
  there are no additional PEIMs to invoke.

  @param  This           Pointer to the DXE IPL PPI instance
  @param  PeiServices    Pointer to the PEI Services Table.
  @param  HobList        Pointer to the list of Hand-Off Block (HOB) entries.

  @retval EFI_SUCCESS           Upon this return code, the PEI Foundation should enter
                                some exception handling.Under normal circumstances, the DXE IPL PPI should not return.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DXE_IPL_ENTRY) (
  IN EFI_DXE_IPL_PPI              *This,
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_HOB_POINTERS         HobList
  );

/**
  @par Ppi Description:
  Final service to be invoked by the PEI Foundation.
  The DXE IPL PPI is responsible for locating and loading the DXE Foundation.
  The DXE IPL PPI may use PEI services to locate and load the DXE Foundation.

  @param Entry
  The entry point to the DXE IPL PPI.

**/
struct _EFI_DXE_IPL_PPI {
  EFI_DXE_IPL_ENTRY Entry;
};

extern EFI_GUID gEfiDxeIplPpiGuid;

#endif
