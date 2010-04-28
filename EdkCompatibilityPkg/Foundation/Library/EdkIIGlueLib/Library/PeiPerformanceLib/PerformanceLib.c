/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              


Module Name:

  PerformanceLib.c
  
Abstract: 

  Support routines used by PEI_PERF_START() and PEI_PERF_END()

--*/

#include "EdkIIGluePeim.h"


#define MAX_PEI_PERF_LOG_ENTRIES 28


STATIC
VOID
InternalGetTimerValue (
  OUT UINT64    *TimerValue
  )
/*++

Routine Description:

  Set TimerValue with current tick.

Arguments:

  TimerValue  - Timer value to be set

Returns:

  EFI_SUCCESS - TimerValue is set.

--*/
{
  *TimerValue = AsmReadTsc ();
}


VOID
EFIAPI
PeiPerfMeasure (
  EFI_PEI_SERVICES              **PeiServices,
  IN UINT16                     *Token,
  IN EFI_FFS_FILE_HEADER        *FileHeader,
  IN BOOLEAN                    EntryExit,
  IN UINT64                     Value
  )
/*++

Routine Description:

  Log a timestamp count.

Arguments:

  PeiServices - Pointer to the PEI Core Services table
  
  Token       - Pointer to Token Name
  
  FileHeader  - Pointer to the file header

  EntryExit   - Indicates start or stop measurement

  Value       - The start time or the stop time

Returns:

--*/
{
  EFI_STATUS                         Status;
  EFI_HOB_GUID_TYPE                  *Hob;
  EFI_HOB_GUID_DATA_PERFORMANCE_LOG  *PerfHobData;
  PEI_PERFORMANCE_MEASURE_LOG_ENTRY  *Log;
  EFI_PEI_PPI_DESCRIPTOR             *PerfHobDescriptor;
  UINT64                             TimeCount;
  INTN                               Index;
  UINTN                              Index2;
  EFI_GUID                           *Guid;
  EFI_GUID                           *CheckGuid;

  TimeCount = 0;
  //
  // Get the END time as early as possible to make it more accurate.
  //
  if (EntryExit) {
    InternalGetTimerValue (&TimeCount);
  }

  //
  // Locate the Pei Performance Log Hob.
  //
  Status = PeiServicesLocatePpi (
                             &gEfiPeiPerformanceHobGuid, 
                             0, 
                             &PerfHobDescriptor, 
                             NULL
                             );

  //
  // If the Performance Hob was not found, build and install one.
  //
  if (EFI_ERROR(Status)) {
    Hob = BuildGuidHob (
               &gEfiPeiPerformanceHobGuid, 
               (sizeof(EFI_HOB_GUID_DATA_PERFORMANCE_LOG) +
                 ((MAX_PEI_PERF_LOG_ENTRIES-1) * 
                 sizeof(PEI_PERFORMANCE_MEASURE_LOG_ENTRY)) +
                 sizeof(EFI_PEI_PPI_DESCRIPTOR)
               )); 
    Hob = (VOID *)((UINTN)Hob - sizeof (EFI_HOB_GUID_TYPE));
    ASSERT_EFI_ERROR (Status);

    PerfHobData = (EFI_HOB_GUID_DATA_PERFORMANCE_LOG *)(Hob+1);
    PerfHobData->NumberOfEntries = 0;

    PerfHobDescriptor = (EFI_PEI_PPI_DESCRIPTOR *)((UINT8 *)(PerfHobData+1) +
                                                     (sizeof(PEI_PERFORMANCE_MEASURE_LOG_ENTRY) *
                                                       (MAX_PEI_PERF_LOG_ENTRIES-1)
                                                     )
                                                  );
    PerfHobDescriptor->Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    PerfHobDescriptor->Guid = &gEfiPeiPerformanceHobGuid;
    PerfHobDescriptor->Ppi = NULL;

    (*PeiServices)->InstallPpi (
                      PeiServices,
                      PerfHobDescriptor
                      );
  }

  PerfHobData = (EFI_HOB_GUID_DATA_PERFORMANCE_LOG *)(((UINT8 *)(PerfHobDescriptor)) -
                                                        ((sizeof(PEI_PERFORMANCE_MEASURE_LOG_ENTRY) *
                                                           (MAX_PEI_PERF_LOG_ENTRIES-1)
                                                         )
                                                         + sizeof(EFI_HOB_GUID_DATA_PERFORMANCE_LOG)
                                                      )
                                                     );

  if (PerfHobData->NumberOfEntries >= MAX_PEI_PERF_LOG_ENTRIES) {
    return;
  }

  if (!EntryExit) {
    Log = &(PerfHobData->Log[PerfHobData->NumberOfEntries]);
    ((*PeiServices)->SetMem) (Log, sizeof(PEI_PERFORMANCE_MEASURE_LOG_ENTRY), 0);

    //
    // If not NULL pointer, copy the file name
    //
    if (FileHeader != NULL) {
      Log->Name = FileHeader->Name;
    }

    //
    // Copy the description string
    //
    ((*PeiServices)->CopyMem) (
                      &(Log->DescriptionString), 
                      Token,
                      (PEI_PERF_MAX_DESC_STRING-1) * sizeof(UINT16)
                      );

    //
    // Get the start time as late as possible to make it more accurate.
    //
    InternalGetTimerValue (&TimeCount);

    //
    // Record the time stamp.
    //
    if (Value != 0) {
      Log->StartTimeCount = Value;
    } else {
      Log->StartTimeCount = TimeCount;
    }
    Log->StopTimeCount = 0;

    //
    // Increment the number of valid log entries.
    //
    PerfHobData->NumberOfEntries++;

  } else {

    for (Index = PerfHobData->NumberOfEntries-1; Index >= 0; Index--) {
      Log = NULL;
      for (Index2 = 0; Index2 < PEI_PERF_MAX_DESC_STRING; Index2++) {
        if (PerfHobData->Log[Index].DescriptionString[Index2] == 0) {
          Log = &(PerfHobData->Log[Index]);
          break;
        }
        if (PerfHobData->Log[Index].DescriptionString[Index2] !=
            Token[Index2]) {
          break;
        }
      }
      if (Log != NULL) {
        if (FileHeader != NULL) {
          Guid = &(Log->Name);
          CheckGuid = &(FileHeader->Name);
          if ((((INT32 *)Guid)[0] == ((INT32 *)CheckGuid)[0]) &&
              (((INT32 *)Guid)[1] == ((INT32 *)CheckGuid)[1]) &&
              (((INT32 *)Guid)[2] == ((INT32 *)CheckGuid)[2]) &&
              (((INT32 *)Guid)[3] == ((INT32 *)CheckGuid)[3]))  {
            if (Value != 0) {
              Log->StopTimeCount = Value;
            } else {
             Log->StopTimeCount = TimeCount;
            }
            break;
          }
        } else {
          if (Value != 0) {
            Log->StopTimeCount = Value;
          } else {
           Log->StopTimeCount = TimeCount;
          }
          break;
        }
      }
    }
            
  }

  return;
}
