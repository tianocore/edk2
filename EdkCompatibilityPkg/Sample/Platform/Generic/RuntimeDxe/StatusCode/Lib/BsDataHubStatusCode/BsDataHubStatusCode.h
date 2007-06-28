/*++

Copyright (c)  2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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
#define INITIAL_RECORD_NUM                20
#define BYTES_PER_RECORD                  EFI_STATUS_CODE_DATA_MAX_SIZE
#define BYTES_PER_BUFFER                  (BYTES_PER_RECORD * sizeof (UINT8))

#define BS_DATA_HUB_STATUS_CODE_SIGNATURE EFI_SIGNATURE_32 ('B', 'D', 'H', 'S')

typedef struct {
  UINTN           Signature;
  EFI_LIST_ENTRY  Link;
  UINT8           RecordBuffer[BYTES_PER_RECORD];
} STATUS_CODE_RECORD_LIST;

//
// Function prototypes
//
STATUS_CODE_RECORD_LIST *
AllocateRecordBuffer (
  VOID
  );
/*++

Routine Description:

  Allocate a new record list node and initialize it.
  Inserting the node into the list isn't the task of this function.

Arguments:

  None

Returns:

  A pointer to the new allocated node or NULL if non available

--*/

DATA_HUB_STATUS_CODE_DATA_RECORD *
AquireEmptyRecordBuffer (
  VOID
  );
/*++

Routine Description:

  Acquire an empty record buffer from the record list if there's free node,
  or allocate one new node and insert it to the list if the list is full and
  the function isn't run in EFI_TPL_HIGH_LEVEL.

Arguments:

  None

Returns:

  Pointer to new record buffer. NULL if none available.

--*/

EFI_STATUS
ReleaseRecordBuffer (
  IN  STATUS_CODE_RECORD_LIST  *RecordBuffer
  );
/*++

Routine Description:

  Release a buffer in the list, remove some nodes to keep the list inital length.
Arguments:

  RecordBuffer          - Buffer to release

Returns:

  EFI_SUCCESS           - If DataRecord is valid
  EFI_UNSUPPORTED       - The record list has empty

--*/

void
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