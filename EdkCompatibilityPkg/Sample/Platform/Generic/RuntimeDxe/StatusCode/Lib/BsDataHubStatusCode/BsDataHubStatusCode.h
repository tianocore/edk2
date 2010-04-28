/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BsDataHubStatusCode.h

Abstract:

  Header for the status code data hub logging component

--*/

#ifndef _EFI_BS_DATA_HUB_STATUS_CODE_H_
#define _EFI_BS_DATA_HUB_STATUS_CODE_H_


// Statements that include other files.
//
#include "Tiano.h"
#include "EfiCommonLib.h"
#include "EfiRuntimeLib.h"
#include "EfiPrintLib.h"
#include "EfiStatusCode.h"

//
// Dependent protocols
//
#include EFI_PROTOCOL_DEPENDENCY (DataHub)

//
// Consumed protocols
//
#include EFI_ARCH_PROTOCOL_CONSUMER (StatusCode)

//
// GUID definitions
//
#include EFI_GUID_DEFINITION (StatusCode)
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

//
// Private data declarations
//
#define MAX_RECORD_NUM                    1000
#define BYTES_PER_RECORD                  EFI_STATUS_CODE_DATA_MAX_SIZE
#define BYTES_PER_BUFFER                  (BYTES_PER_RECORD * sizeof (UINT8))

#define BS_DATA_HUB_STATUS_CODE_SIGNATURE EFI_SIGNATURE_32 ('B', 'D', 'H', 'S')

typedef struct {
  UINTN           Signature;
  EFI_LIST_ENTRY  Node;
  UINT8           Data[BYTES_PER_RECORD];
} DATAHUB_STATUSCODE_RECORD;

//
// Function prototypes
//
EFI_STATUS
EFIAPI
BsDataHubInitializeStatusCode (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );
/*++

Routine Description:

  Install a data hub listener.

Arguments:

  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCCESS - Logging Hub protocol installed
  Other       - No protocol installed, unload driver.

--*/

EFI_STATUS
EFIAPI
BsDataHubReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  );
/*++

Routine Description:

  Boot service report status code listener.  This function logs the status code
  into the data hub.

Arguments:

  Same as gRT->ReportStatusCode (See Tiano Runtime Specification)

Returns:

  None

--*/

VOID
EFIAPI
LogDataHubEventHandler (
  IN  EFI_EVENT     Event,
  IN  VOID          *Context
  );
/*++

Routine Description:

  The Event handler which will be notified to log data in Data Hub.

Arguments:

  Event     -   Instance of the EFI_EVENT to signal whenever data is
                available to be logged in the system.
  Context   -   Context of the event.

Returns:

  None.

--*/
#endif
