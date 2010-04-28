/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCodeCallerId.c
    
Abstract:

  GUID used to identify id for the caller who is initiating the Status Code.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (StatusCodeCallerId)

EFI_GUID  gEfiCallerIdGuid = EFI_STANDARD_CALLER_ID_GUID;

EFI_GUID_STRING(&gEfiCallerIdGuid, "Status Code Caller Id", "Caller Id for Tiano ReportStatusCode API");
