/*++

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 TemporaryRamSupport.c

Abstract:

  This file declares Temporary RAM Support PPI.                                   
  This Ppi provides the service that migrates temporary RAM into permanent memory.

--*/

#include "Tiano.h"
#include "PeiBind.h"
#include "PeiApi.h"
#include EFI_PPI_DEFINITION (TemporaryRamSupport)

EFI_GUID  gEfiTemporaryRamSupportPpiGuid = TEMPORARY_RAM_SUPPORT_PPI_GUID;

EFI_GUID_STRING(&gEfiTemporaryRamSupportPpiGuid, "TemporaryRamSupportPpi", "TemporaryRamSupport PPI");
