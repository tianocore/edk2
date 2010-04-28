/*++

  Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
  
Module Name:
  
  Stall.c

Abstract:

  Stall PPI

--*/

#include "Tiano.h"
#include "Pei.h"
#include EFI_PPI_DEFINITION (Stall)

EFI_GUID  gPeiStallPpiGuid = PEI_STALL_PPI_GUID;

EFI_GUID_STRING(&gPeiStallPpiGuid, "Stall", "Stall PPI");
