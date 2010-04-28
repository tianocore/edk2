/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BootInRecoveryMode.c

Abstract:

  Boot Mode PPI GUID as defined in PEI EAS

--*/

#include "Tiano.h"
#include "PeiBind.h"
#include "PeiApi.h"
#include EFI_PPI_DEFINITION (BootInRecoveryMode)

EFI_GUID  gPeiBootInRecoveryModePpiGuid = PEI_BOOT_IN_RECOVERY_MODE_PEIM_PPI;

EFI_GUID_STRING(&gPeiMasterBootModePpiGuid, "BootMode", "Master Boot Mode PPI");
