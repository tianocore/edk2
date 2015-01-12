/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  MonoStatusCode.h

Abstract:

  Monolithic single PEIM to provide the status code functionality.
  The PEIM is a blend of libraries that correspond to the different status code
  listeners that a platform installs.

--*/

#ifndef _MONO_STATUS_CODE_H_
#define _MONO_STATUS_CODE_H_

//
// Statements that include other files.
//
#include "PiPei.h"

#include "Pi/PiBootMode.h"

#include "Ppi/StatusCode.h"
#include "Ppi/MemoryDiscovered.h"
#include "Ppi/FvLoadFile.h"

#include "Library/HobLib.h"
#include "Library/DebugLib.h"
#include "Library/IoLib.h"
#include "Library/SerialPortLib.h"
#include "Protocol/StatusCode.h"


#ifndef _STATUS_CODE_ENABLER_H_
#define _STATUS_CODE_ENABLER_H_

#ifdef EFI_DEBUG

#define EFI_STATUS_CODE_ENABLER_HOB_GUID \
  { \
    0x5ffc6cf3, 0x71ad, 0x46f5, 0xbd, 0x8b, 0x7e, 0x8f, 0xfe, 0x19, 0x7, 0xd7 \
  }

extern EFI_GUID gEfiSerialStatusCodeEnablerHobGuid;

typedef struct _EFI_STATUS_CODE_INFO {
  BOOLEAN    StatusCodeDisable;
} EFI_STATUS_CODE_INFO;

#endif
#endif



//
// Platform specific function Declarations.  These must be implemented in a
// subdirectory named PlatformName in a file named PlatformStatusCode.c.
//

//
// This is the platform function to initialize the listeners desired by the
// platform.
//
VOID
PlatformInitializeStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  );

//
// This is the platform function that calls all of the listeners desired by the
// platform.
//
EFI_STATUS
EFIAPI
PlatformReportStatusCode (
  IN CONST EFI_PEI_SERVICES       **PeiServices,
  IN EFI_STATUS_CODE_TYPE         CodeType,
  IN EFI_STATUS_CODE_VALUE        Value,
  IN UINT32                       Instance,
  IN CONST EFI_GUID               * CallerId,
  IN CONST EFI_STATUS_CODE_DATA   * Data OPTIONAL
  );

//
// Platform independent function Declarations
//
//
// Initialize the status code listeners and publish the status code PPI.
//
VOID
EFIAPI
InitializeMonoStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN const EFI_PEI_SERVICES    **PeiServices
  );

//
// Convert a DXE status code call into a PEI status code call.
//
EFI_STATUS
EFIAPI
TranslateDxeStatusCodeToPeiStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  );

//
// Publish a HOB that contains the listener to be used by DXE.
//
EFI_STATUS
EFIAPI
InitializeDxeReportStatusCode (
  IN const EFI_PEI_SERVICES       **PeiServices
  );

#endif
