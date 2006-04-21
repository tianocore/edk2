/*++

Copyright (c) 2006, Intel Corporation                                                         
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

//
// Private data declarations
//
#define MAX_RECORD_NUM                    1000
#define BYTES_PER_RECORD                  EFI_STATUS_CODE_DATA_MAX_SIZE
#define EMPTY_RECORD_TAG                  0xFF

#define BS_DATA_HUB_STATUS_CODE_SIGNATURE EFI_SIGNATURE_32 ('B', 'D', 'H', 'S')

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;
  UINT8           RecordBuffer[BYTES_PER_RECORD];
} STATUS_CODE_RECORD_LIST;

//
// Function prototypes
//
STATUS_CODE_RECORD_LIST           *
GetRecordBuffer (
  VOID
  )
;

/*++

Routine Description:

  Returned buffer of length BYTES_PER_RECORD

Arguments:

  None

Returns:

  Entry in mRecordBuffer or NULL if non available

--*/
DATA_HUB_STATUS_CODE_DATA_RECORD  *
AquireEmptyRecordBuffer (
  VOID
  )
;

/*++

Routine Description:

  Allocate a mRecordBuffer entry in the form of a pointer.

Arguments:

  None

Returns:

  Pointer to new buffer. NULL if none exist.

--*/
EFI_STATUS
ReleaseRecordBuffer (
  IN  STATUS_CODE_RECORD_LIST  *RecordBuffer
  )
;

/*++

Routine Description:

  Release a mRecordBuffer entry allocated by AquireEmptyRecordBuffer ().

Arguments:

  RecordBuffer          - Data to free

Returns:

  EFI_SUCCESS           - If RecordBuffer is valid
  EFI_UNSUPPORTED       - The record list has empty

--*/
VOID
EFIAPI
LogDataHubEventHandler (
  IN  EFI_EVENT   Event,
  IN  VOID        *Context
  )
;

/*++

Routine Description:

  Event Handler that log in Status code in Data Hub.

Arguments:

  (Standard EFI Event Handler - EFI_EVENT_NOTIFY)

Returns:

  NONE

--*/
#endif
