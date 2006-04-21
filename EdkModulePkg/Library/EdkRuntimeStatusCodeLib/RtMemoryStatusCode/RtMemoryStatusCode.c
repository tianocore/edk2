/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  RtMemoryStatusCode.c
   
Abstract:

  EFI lib to provide memory journal status code reporting routines.

--*/

#include <Ppi/StatusCodeMemory.h>

//
// Global variables
//
PEI_STATUS_CODE_MEMORY_PPI  mStatusCodeMemoryPpi = { 0, 0, 0, 0 };

//
// Function implementations
//
EFI_STATUS
RtMemoryReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Log a status code to a memory journal.  If no memory journal exists, 
  we will just return.

Arguments:

  Same as ReportStatusCode AP
    
Returns:

  EFI_SUCCESS   This function always returns success

--*/
{
  EFI_STATUS_CODE_ENTRY *CurrentEntry;
  UINTN                 MaxEntry;

  //
  // We don't care to log debug codes.
  //
  if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE) {
    return EFI_SUCCESS;
  }
  //
  // Update the latest entry in the journal.
  //
  MaxEntry = mStatusCodeMemoryPpi.Length / sizeof (EFI_STATUS_CODE_ENTRY);
  if (!MaxEntry) {
    //
    // If we don't have any entries, then we can return.
    // This effectively means that no memory buffer was passed forward from PEI.
    //
    return EFI_SUCCESS;
  }

  CurrentEntry = (EFI_STATUS_CODE_ENTRY *) (UINTN) (mStatusCodeMemoryPpi.Address + (mStatusCodeMemoryPpi.LastEntry * sizeof (EFI_STATUS_CODE_ENTRY)));

  mStatusCodeMemoryPpi.LastEntry = (mStatusCodeMemoryPpi.LastEntry + 1) % MaxEntry;
  if (mStatusCodeMemoryPpi.LastEntry == mStatusCodeMemoryPpi.FirstEntry) {
    mStatusCodeMemoryPpi.FirstEntry = (mStatusCodeMemoryPpi.FirstEntry + 1) % MaxEntry;
  }

  CurrentEntry->Type      = CodeType;
  CurrentEntry->Value     = Value;
  CurrentEntry->Instance  = Instance;

  return EFI_SUCCESS;
}

VOID
RtMemoryStatusCodeInitialize (
  VOID
  )
/*++

Routine Description:

  Initialization routine.
  Allocates heap space for storing Status Codes.
  Installs a PPI to point to that heap space.
  Installs a callback to switch to memory.
  Installs a callback to 

Arguments: 

  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns: 

  None

--*/
{
  EFI_HOB_GUID_TYPE           *GuidHob;
  PEI_STATUS_CODE_MEMORY_PPI  **StatusCodeMemoryPpi;

  GuidHob = GetFirstGuidHob (&gPeiStatusCodeMemoryPpiGuid);
  if (GuidHob == NULL) {
    return;
  }

  StatusCodeMemoryPpi = GET_GUID_HOB_DATA (GuidHob);

  //
  // Copy data to our structure since the HOB will go away at runtime
  //
  // BUGBUG: Virtualize for RT
  //
  mStatusCodeMemoryPpi.FirstEntry = (*StatusCodeMemoryPpi)->FirstEntry;
  mStatusCodeMemoryPpi.LastEntry  = (*StatusCodeMemoryPpi)->LastEntry;
  mStatusCodeMemoryPpi.Address    = (*StatusCodeMemoryPpi)->Address;
  mStatusCodeMemoryPpi.Length     = (*StatusCodeMemoryPpi)->Length;
}

VOID
PlaybackStatusCodes (
  IN EFI_REPORT_STATUS_CODE   ReportStatusCodeFunc
  )
/*++

Routine Description:

  Call the input ReportStatusCode function with every status code recorded in
  the journal.

Arguments: 

  ReportStatusCode    ReportStatusCode function to call.

Returns: 

  None

--*/
{
  UINTN                 MaxEntry;
  EFI_STATUS_CODE_ENTRY *CurrentEntry;
  UINTN                 Counter;

  if (ReportStatusCodeFunc == RtMemoryReportStatusCode) {
    return ;
  }
  //
  // Playback prior status codes to current listeners
  //
  MaxEntry = mStatusCodeMemoryPpi.Length / sizeof (EFI_STATUS_CODE_ENTRY);
  for (Counter = mStatusCodeMemoryPpi.FirstEntry; Counter != mStatusCodeMemoryPpi.LastEntry; Counter++) {
    //
    // Check if we have to roll back to beginning of queue buffer
    //
    if (Counter == MaxEntry) {
      Counter = 0;
    }
    //
    // Play current entry
    //
    CurrentEntry = (EFI_STATUS_CODE_ENTRY *) (UINTN) (mStatusCodeMemoryPpi.Address + (Counter * sizeof (EFI_STATUS_CODE_ENTRY)));
    ReportStatusCodeFunc (
      CurrentEntry->Type,
      CurrentEntry->Value,
      CurrentEntry->Instance,
      NULL,
      NULL
      );
  }
}
