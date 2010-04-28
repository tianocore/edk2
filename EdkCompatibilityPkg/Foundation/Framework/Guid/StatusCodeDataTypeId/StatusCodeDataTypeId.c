/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCodeDataTypeId.c
    
Abstract:

  GUID used to identify id for the caller who is initiating the Status Code.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

//
// Taken out from StatusCode.C created by PRC
//
EFI_GUID  gEfiStatusCodeDataTypeStringGuid            = EFI_STATUS_CODE_DATA_TYPE_STRING_GUID;
EFI_GUID  gEfiStatusCodeDataTypeDebugGuid             = EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID;
EFI_GUID  gEfiStatusCodeDataTypeAssertGuid            = EFI_STATUS_CODE_DATA_TYPE_ASSERT_GUID;
EFI_GUID  gEfiStatusCodeDataTypeExceptionHandlerGuid  = EFI_STATUS_CODE_DATA_TYPE_EXCEPTION_HANDLER_GUID;
EFI_GUID  gEfiStatusCodeDataTypeErrorGuid             = EFI_STATUS_CODE_DATA_TYPE_ERROR_GUID;
EFI_GUID  gEfiStatusCodeDataTypeProgressCodeGuid      = EFI_STATUS_CODE_DATA_TYPE_PROGRESS_CODE_GUID;

EFI_GUID  gEfiStatusCodeSpecificDataGuid              = EFI_STATUS_CODE_SPECIFIC_DATA_GUID;

EFI_GUID_STRING(&gEfiStatusCodeDataTypeStringGuid, "Status Code", "Data Hub record Data type String");
EFI_GUID_STRING(&gEfiStatusCodeSpecificDataGuid, "Status Code", "Data Hub record Data type specific ");
EFI_GUID_STRING(&gEfiStatusCodeDataTypeDebugGuid, "Status Code", "Data Hub record data type Debug");
EFI_GUID_STRING(&gEfiStatusCodeDataTypeAssertGuid, "Status Code", "Data Hub record data type Assert");
EFI_GUID_STRING(&gEfiStatusCodeDataTypeErrorGuid, "Status Code", "Data Hub record data type Error");
EFI_GUID_STRING(&gEfiStatusCodeDataTypeProgressCodeGuid, "Status Code", "Data Hub record data type Progress Code");
EFI_GUID_STRING
  (&gEfiStatusCodeDataTypeExceptionHandlerGuid, "Status Code", "Data Hub record Data type Exception handler");

EFI_GUID_STRING
  (&gEfiStatusCodeSpecificDataGuid, "Status Code Specific Data", "Specific Data for Tiano ReportStatusCode API");
