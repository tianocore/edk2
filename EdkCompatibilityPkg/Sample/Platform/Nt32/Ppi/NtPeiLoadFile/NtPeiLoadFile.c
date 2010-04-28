/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 NtPeiLoadFile.c

Abstract:

  Abstraction for the NT Load Image PPI GUID as defined in Tiano

--*/

#include "Tiano.h"
#include "PeiBind.h"
#include "PeiApi.h"
#include EFI_PPI_DEFINITION (NtPeiLoadFile)

EFI_GUID  gNtPeiLoadFileGuid = NT_PEI_LOAD_FILE_GUID;

EFI_GUID_STRING(&gNtPeiLoadFileGuid, "NtPeiLoadFile", "NT PEI LOAD FILE PPI");
