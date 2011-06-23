/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  SerialStatusCode.h

Abstract:

Revision History:

**/

#ifndef _DXELDR_SERIAL_STATUS_CODE_H_
#define _DXELDR_SERIAL_STATUS_CODE_H_

//
// Statements that include other files
//
#include "DxeIpl.h"

//
// GUID consumed
//


VOID
InstallSerialStatusCode (
  IN EFI_REPORT_STATUS_CODE    *ReportStatusCode
  );

#endif 
