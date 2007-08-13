/** @file
  PI PEI master include file. This file should match the PI spec.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  Version 1.0.

**/

#ifndef __PEICIS_H__
#define __PEICIS_H__

#include <PiPei.h>

/**
  The PEI Dispatcher will invoke each PEIM one time.  During this pass, the PEI 
  Dispatcher will pass control to the PEIM at the AddressOfEntryPoint in the PE Header. 

  @param  FfsHeader        Pointer to the FFS file header.
  @param  PeiServices      Describes the list of possible PEI Services.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEIM_ENTRY_POINT)(
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );
  
#endif  