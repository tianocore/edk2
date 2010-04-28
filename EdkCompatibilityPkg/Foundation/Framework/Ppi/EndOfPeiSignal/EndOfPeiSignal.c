/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EndOfPeiSignal.c

Abstract:

  This is installed prior to DXE taking over the memory map

--*/

#include "Tiano.h"
#include "PeiBind.h"
#include "PeiApi.h"
#include EFI_PPI_DEFINITION (EndOfPeiSignal)

EFI_GUID  gEndOfPeiSignalPpiGuid = PEI_END_OF_PEI_PHASE_PPI_GUID;

EFI_GUID_STRING(&gEndOfPeiSignalPpiGuid, "EndOfPeiSignal", "End of PEI Phase Signalled PPI");
