/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCode.c

Abstract:

  Pei Core Status Code Support

Revision History

**/

#include <PeiMain.h>

EFI_STATUS
EFIAPI
PeiReportStatusCode (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE           CodeType,
  IN EFI_STATUS_CODE_VALUE          Value, 
  IN UINT32                         Instance,
  IN CONST EFI_GUID                 *CallerId,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
/*++

Routine Description:

  Core version of the Status Code reporter

Arguments:

  PeiServices - The PEI core services table.
  
  CodeType    - Type of Status Code.
  
  Value       - Value to output for Status Code.
  
  Instance    - Instance Number of this status code.
  
  CallerId    - ID of the caller of this status code.
  
  Data        - Optional data associated with this status code.

Returns:

  Status  - EFI_SUCCESS             if status code is successfully reported
          - EFI_NOT_AVAILABLE_YET   if StatusCodePpi has not been installed

--*/
{
  EFI_STATUS                Status;
  EFI_PEI_PROGRESS_CODE_PPI *StatusCodePpi;
  

  //
  //Locate StatusCode Ppi.
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiStatusCodePpiGuid,         
             0,                         
             NULL,                      
             (VOID **)&StatusCodePpi                  
             );

  if (!EFI_ERROR (Status)) {
    Status = StatusCodePpi->ReportStatusCode (
                            PeiServices,
                            CodeType,
                            Value,
                            Instance,
                            CallerId,
                            Data
                            );
 
   return Status;   
  } 
  
  
  return  EFI_NOT_AVAILABLE_YET; 
}



