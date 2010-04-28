/*++

Copyright (c) 2004 - 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Pei.h

Abstract:

  Framework PEI master include file.

  This is the main include file for Framework PEI components. There should be
  no defines or macros added to this file, other than the EFI version 
  information already in this file.

  Don't add include files to the list for convenience, only add things
  that are architectural. Don't add Protocols or GUID include files here

--*/

#ifndef _PEI_H_
#define _PEI_H_

//
// PEI Specification Revision information
//
#include "TianoCommon.h"

#include "PeiBind.h"
#include "PeiApi.h"
#include "EfiDebug.h"
#include "PeiDebug.h"

//
// Enable code sharing with DXE by removing ASSERT and DEBUG
//
// #define ASSERT(a)
// #define DEBUG (a)
//

#ifdef EFI_PEI_REPORT_STATUS_CODE_ON
#define PEI_REPORT_STATUS_CODE_CODE(Code) Code
#define PEI_REPORT_STATUS_CODE(PeiServices, CodeType, Value, Instance, CallerId, Data) \
          (*PeiServices)->PeiReportStatusCode (PeiServices, CodeType, Value, Instance, CallerId, Data)
#else
#define PEI_REPORT_STATUS_CODE_CODE(Code)
#define PEI_REPORT_STATUS_CODE(PeiServices, CodeType, Value, Instance, CallerId, Data)
#endif

#endif
