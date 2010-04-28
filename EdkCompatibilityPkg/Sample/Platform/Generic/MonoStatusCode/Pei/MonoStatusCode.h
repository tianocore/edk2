/*++

Copyright (c) 2004 - 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
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
#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include "EfiCommonLib.h"

//
// Driver Produced DXE Protocol Prototypes
//
#include EFI_PPI_PRODUCER (StatusCode)

//
// Driver Consumed DXE Protocol Prototypes
//
#include EFI_ARCH_PROTOCOL_CONSUMER (StatusCode)

//
// Driver GUID includes
//
#include EFI_GUID_DEFINITION (StatusCode)
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (GlobalVariable)

extern EFI_GUID mStatusCodeRuntimeGuid;

//
// Platform specific function Declarations.  These must be implemented in a
// subdirectory named PlatformName in a file named PlatformStatusCode.c.
// See D845GRG\PlatformStatusCode.c for an example of a simple status code
// implementation.
// See Nt32\PlatformStatusCode.c for an example of a status code implementation
// that relocates itself into memory.
//
//
// This is the driver entry point and must be defined.
//
EFI_STATUS
EFIAPI
InstallMonoStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );

//
// This is the platform function to initialize the listeners desired by the
// platform.
//
VOID
PlatformInitializeStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );

//
// This is the platform function that calls all of the listeners desired by the
// platform.
//
EFI_STATUS
EFIAPI
PlatformReportStatusCode (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
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
  IN EFI_PEI_SERVICES          **PeiServices
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
  IN EFI_PEI_SERVICES       **PeiServices
  );

#endif
