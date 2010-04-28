/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCode.c
    
Abstract:

  GUIDs used to identify Data Hub records that originate from the Tiano 
  ReportStatusCode API.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (StatusCode)

EFI_GUID  gEfiStatusCodeGuid = EFI_STATUS_CODE_GUID;

EFI_GUID_STRING(&gEfiStatusCodeGuid, "Status Code", "Data Hub record for Tiano ReportStatusCode API");
