/** @file
  * Trace reporting for the Dp utility.
  *
  * Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  * This program and the accompanying materials
  * are licensed and made available under the terms and conditions of the BSD License
  * which accompanies this distribution.  The full text of the license may be found at
  * http://opensource.org/licenses/bsd-license.php
  *
  * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>

#include <Guid/Performance.h>

#include "Dp.h"
#include "Literals.h"
#include "DpInternal.h"

/** 
  Collect verbose statistics about the logged performance measurements.
  
  General Summary information for all Trace measurements is gathered and
  stored within the SummaryData structure.  This information is both
  used internally by subsequent reporting functions, and displayed
  at the end of verbose reports.
  
  @pre  The SummaryData and CumData structures must be initialized
        prior to calling this function.
  
  @post The SummaryData and CumData structures contain statistics for the
        current performance logs.
**/
VOID
GatherStatistics(
  VOID
)
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    Duration;
  UINTN                     LogEntryKey;
  INTN                      TIndex;

  LogEntryKey = 0;
  while ((LogEntryKey = GetPerformanceMeasurement (
                        LogEntryKey,
                        &Measurement.Handle,
                        &Measurement.Token,
                        &Measurement.Module,
                        &Measurement.StartTimeStamp,
                        &Measurement.EndTimeStamp)) != 0)
  {
    ++SummaryData.NumTrace;           // Count the number of TRACE Measurement records
    if (Measurement.EndTimeStamp == 0) {
      ++SummaryData.NumIncomplete;    // Count the incomplete records
      continue;
    }

    if (Measurement.Handle != NULL) {
      ++SummaryData.NumHandles;       // Count the number of measurements with non-NULL handles
    }

    if (IsPhase( &Measurement)) {
      ++SummaryData.NumSummary;       // Count the number of major phases
    }
    else {  // !IsPhase(...
      if(Measurement.Handle == NULL) {
        ++SummaryData.NumGlobal;
      }
    }

    if (AsciiStrnCmp (Measurement.Token, ALit_PEIM, PERF_TOKEN_LENGTH) == 0) {
      ++SummaryData.NumPEIMs;         // Count PEIM measurements
    }

    Duration = GetDuration (&Measurement);
    TIndex = GetCumulativeItem (&Measurement);
    if (TIndex >= 0) {
      CumData[TIndex].Duration += Duration;
      CumData[TIndex].Count++;
      if ( Duration < CumData[TIndex].MinDur ) {
        CumData[TIndex].MinDur = Duration;
      }
      if ( Duration > CumData[TIndex].MaxDur ) {
        CumData[TIndex].MaxDur = Duration;
      }
    }
  }
}

/** 
  Gather and print ALL Trace Records.
  
  Displays all "interesting" Trace measurements in order.<BR>
  The number of records displayed is controlled by:
     - records with a duration less than mInterestThreshold microseconds are not displayed.
     - No more than Limit records are displayed.  A Limit of zero will not limit the output.
     - If the ExcludeFlag is TRUE, records matching entries in the CumData array are not
       displayed.
  
  @pre    The mInterestThreshold global variable is set to the shortest duration to be printed.
           The mGaugeString and mUnicodeToken global arrays are used for temporary string storage.
           They must not be in use by a calling function.
  
  @param[in]    Limit       The number of records to print.  Zero is ALL.
  @param[in]    ExcludeFlag TRUE to exclude individual Cumulative items from display.
  
**/
VOID
DumpAllTrace(
  IN UINTN             Limit,
  IN BOOLEAN           ExcludeFlag
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  const CHAR16              *IncFlag;
  UINTN                     LogEntryKey;
  UINTN                     Count;
  UINTN                     Index;
  UINTN                     TIndex;

  EFI_HANDLE                *HandleBuffer;
  UINTN                     Size;
  EFI_HANDLE                TempHandle;
  EFI_STATUS                Status;

  IncFlag = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_ALL), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (IncFlag == NULL) ? ALit_UNKNOWN: IncFlag);

  // Get Handle information
  //
  Size = 0;
  HandleBuffer = &TempHandle;
  Status  = gBS->LocateHandle (AllHandles, NULL, NULL, &Size, &TempHandle);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HandleBuffer = AllocatePool (Size);
    ASSERT (HandleBuffer != NULL);
    if (HandleBuffer == NULL) {
      return;
    }
    Status  = gBS->LocateHandle (AllHandles, NULL, NULL, &Size, HandleBuffer);
  }
  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_DP_HANDLES_ERROR), Status);
  }
  else {
    // We have successfully populated the HandleBuffer
    // Display ALL Measurement Records
    //    Up to Limit lines displayed
    //    Display only records with Elapsed times >= mInterestThreshold
    //    Display driver names in Module field for records with Handles.
    //
    PrintToken (STRING_TOKEN (STR_DP_ALL_HEADR) );
    PrintToken (STRING_TOKEN (STR_DP_DASHES) );

    LogEntryKey = 0;
    Count = 0;
    Index = 0;
    while ( WITHIN_LIMIT(Count, Limit) &&
            ((LogEntryKey = GetPerformanceMeasurement (
                            LogEntryKey,
                            &Measurement.Handle,
                            &Measurement.Token,
                            &Measurement.Module,
                            &Measurement.StartTimeStamp,
                            &Measurement.EndTimeStamp)) != 0)
          )
    {
      ++Index;    // Count every record.  First record is 1.
      ElapsedTime = 0;
      if (Measurement.EndTimeStamp != 0) {
        Duration = GetDuration (&Measurement);
        ElapsedTime = DurationInMicroSeconds ( Duration );
        IncFlag = STR_DP_COMPLETE;
      }
      else {
        IncFlag = STR_DP_INCOMPLETE;  // Mark incomplete records
      }
      if ((ElapsedTime < mInterestThreshold)                 ||
          ((ExcludeFlag) && (GetCumulativeItem(&Measurement) >= 0))
         ) {      // Ignore "uninteresting" or excluded records
        continue;
      }
      if (Measurement.EndTimeStamp == 0) {
        ElapsedTime = Measurement.StartTimeStamp;
      }
      ++Count;    // Count the number of records printed

      // If Handle is non-zero, see if we can determine a name for the driver
      AsciiStrToUnicodeStr (Measurement.Module, mGaugeString); // Use Module by default
      AsciiStrToUnicodeStr (Measurement.Token, mUnicodeToken);
      if (Measurement.Handle != NULL) {
        // See if the Handle is in the HandleBuffer
        for (TIndex = 0; TIndex < (Size / sizeof(HandleBuffer[0])); TIndex++) {
          if (Measurement.Handle == HandleBuffer[TIndex]) {
            GetNameFromHandle (HandleBuffer[TIndex]);
            break;
          }
        }
      }
      // Ensure that the argument strings are not too long.
      mGaugeString[31] = 0;
      mUnicodeToken[18] = 0;

      PrintToken( STRING_TOKEN (STR_DP_ALL_STATS),
        Index,      // 1 based, Which measurement record is being printed
        IncFlag,
        Measurement.Handle,
        mGaugeString,
        mUnicodeToken,
        ElapsedTime
      );
    }
  }
  if (HandleBuffer != &TempHandle) {
    FreePool (HandleBuffer);
  }
}

/** 
  Gather and print Raw Trace Records.
  
  All Trace measurements with a duration greater than or equal to
  mInterestThreshold are printed without interpretation.
  
  The number of records displayed is controlled by:
     - records with a duration less than mInterestThreshold microseconds are not displayed.
     - No more than Limit records are displayed.  A Limit of zero will not limit the output.
     - If the ExcludeFlag is TRUE, records matching entries in the CumData array are not
       displayed.
  
  @pre    The mInterestThreshold global variable is set to the shortest duration to be printed.
  
  @param[in]    Limit       The number of records to print.  Zero is ALL.
  @param[in]    ExcludeFlag TRUE to exclude individual Cumulative items from display.
  
**/
VOID
DumpRawTrace(
  IN UINTN          Limit,
  IN BOOLEAN        ExcludeFlag
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  UINTN                     LogEntryKey;
  UINTN                     Count;
  UINTN                     Index;

  EFI_STRING    StringPtr;

  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_RAWTRACE), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? ALit_UNKNOWN: StringPtr);

  PrintToken (STRING_TOKEN (STR_DP_RAW_HEADR) );
  PrintToken (STRING_TOKEN (STR_DP_RAW_DASHES) );

  LogEntryKey = 0;
  Count = 0;
  Index = 0;
  while ( WITHIN_LIMIT(Count, Limit) &&
          ((LogEntryKey = GetPerformanceMeasurement (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp)) != 0)
        )
  {
    ++Index;    // Count every record.  First record is 1.
    ElapsedTime = 0;
    if (Measurement.EndTimeStamp != 0) {
      Duration = GetDuration (&Measurement);
      ElapsedTime = DurationInMicroSeconds ( Duration );
    }
    if ((ElapsedTime < mInterestThreshold)                 ||
        ((ExcludeFlag) && (GetCumulativeItem(&Measurement) >= 0))
        ) { // Ignore "uninteresting" or Excluded records
      continue;
    }
    ++Count;    // Count the number of records printed
    PrintToken (STRING_TOKEN (STR_DP_RAW_VARS),
      Index,      // 1 based, Which measurement record is being printed
      Measurement.Handle,
      Measurement.StartTimeStamp,
      Measurement.EndTimeStamp,
      Measurement.Token,
      Measurement.Module
    );
  }
}

/** 
  Gather and print Major Phase metrics.
  
  @param[in]    Ticker      The timer value for the END of Shell phase
  
**/
VOID
ProcessPhases(
  UINT64            Ticker
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    BdsTimeoutValue;
  UINT64                    SecTime;
  UINT64                    PeiTime;
  UINT64                    DxeTime;
  UINT64                    BdsTime;
  UINT64                    ShellTime;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  UINT64                    Total;
  EFI_STRING                StringPtr;
  UINTN                     LogEntryKey;

  BdsTimeoutValue = 0;
  SecTime         = 0;
  PeiTime         = 0;
  DxeTime         = 0;
  BdsTime         = 0;
  ShellTime       = 0;   
  //
  // Get Execution Phase Statistics
  //
  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_PHASES), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? ALit_UNKNOWN: StringPtr);

  LogEntryKey = 0;
  while ((LogEntryKey = GetPerformanceMeasurement (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp)) != 0)
  {
    if (AsciiStrnCmp (Measurement.Token, ALit_SHELL, PERF_TOKEN_LENGTH) == 0) {
      Measurement.EndTimeStamp = Ticker;
    }
    if (Measurement.EndTimeStamp == 0) { // Skip "incomplete" records
      continue;
    }
    Duration = GetDuration (&Measurement);
    if (   Measurement.Handle != NULL
        && (AsciiStrnCmp (Measurement.Token, ALit_BdsTO, PERF_TOKEN_LENGTH) == 0)
       )
    {
      BdsTimeoutValue = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_SEC, PERF_TOKEN_LENGTH) == 0) {
      SecTime     = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_PEI, PERF_TOKEN_LENGTH) == 0) {
      PeiTime     = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_DXE, PERF_TOKEN_LENGTH) == 0) {
      DxeTime      = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_BDS, PERF_TOKEN_LENGTH) == 0) {
      BdsTime      = Duration;
    } else if (AsciiStrnCmp (Measurement.Token, ALit_SHELL, PERF_TOKEN_LENGTH) == 0) {
      ShellTime    = Duration;
    }
  }

  Total = 0;

  // print SEC phase duration time
  //
  if (SecTime > 0) {
    ElapsedTime = DurationInMicroSeconds ( SecTime );     // Calculate elapsed time in microseconds
    Total += DivU64x32 (ElapsedTime, 1000);   // Accumulate time in milliseconds
    PrintToken (STRING_TOKEN (STR_DP_SEC_PHASE), ElapsedTime);
  }

  // print PEI phase duration time
  //
  if (PeiTime > 0) {
    ElapsedTime = DivU64x32 (
                    PeiTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    PrintToken (STRING_TOKEN (STR_DP_PHASE_DURATION), ALit_PEI, ElapsedTime);
  }

  // print DXE phase duration time
  //
  if (DxeTime > 0) {
    ElapsedTime = DivU64x32 (
                    DxeTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    PrintToken (STRING_TOKEN (STR_DP_PHASE_DURATION), ALit_DXE, ElapsedTime);
  }

  // print BDS phase duration time
  //
  if (BdsTime > 0) {
    ElapsedTime = DivU64x32 (
                    BdsTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    PrintToken (STRING_TOKEN (STR_DP_PHASE_DURATION), ALit_BDS, ElapsedTime);
  }

  if (BdsTimeoutValue > 0) {
    ElapsedTime = DivU64x32 (
                    BdsTimeoutValue,
                    (UINT32)TimerInfo.Frequency
                    );
    PrintToken (STRING_TOKEN (STR_DP_PHASE_BDSTO), ALit_BdsTO, ElapsedTime);
  }

  // print SHELL phase duration time
  //
  if (ShellTime > 0) {
    ElapsedTime = DivU64x32 (
                    ShellTime,
                    (UINT32)TimerInfo.Frequency
                    );
    Total += ElapsedTime;
    PrintToken (STRING_TOKEN (STR_DP_PHASE_DURATION), ALit_SHELL, ElapsedTime);
  }

  PrintToken (STRING_TOKEN (STR_DP_TOTAL_DURATION), Total);
}

/** 
  Gather and print Handle data.
  
  @param[in]    ExcludeFlag   TRUE to exclude individual Cumulative items from display.
  
  @return       Status from a call to gBS->LocateHandle().
**/
EFI_STATUS
ProcessHandles(
  IN BOOLEAN      ExcludeFlag
  )
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    ElapsedTime;
  UINT64                    Duration;
  EFI_HANDLE                *HandleBuffer;
  EFI_STRING                StringPtr;
  UINTN                     Index;
  UINTN                     LogEntryKey;
  UINTN                     Count;
  UINTN                     Size;
  EFI_HANDLE                TempHandle;
  EFI_STATUS                Status;

  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_DRIVERS), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? ALit_UNKNOWN: StringPtr);

  Size = 0;
  HandleBuffer = &TempHandle;
  Status  = gBS->LocateHandle (AllHandles, NULL, NULL, &Size, &TempHandle);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HandleBuffer = AllocatePool (Size);
    ASSERT (HandleBuffer != NULL);
    if (HandleBuffer == NULL) {
      return Status;
    }
    Status  = gBS->LocateHandle (AllHandles, NULL, NULL, &Size, HandleBuffer);
  }
  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_DP_HANDLES_ERROR), Status);
  }
  else {
#if DP_DEBUG == 2
    Print (L"There are %,d Handles defined.\n", (Size / sizeof(HandleBuffer[0])));
#endif

    PrintToken (STRING_TOKEN (STR_DP_HANDLE_GUID) );
    PrintToken (STRING_TOKEN (STR_DP_DASHES) );

    LogEntryKey = 0;
    Count   = 0;
    while ((LogEntryKey = GetPerformanceMeasurement (
                            LogEntryKey,
                            &Measurement.Handle,
                            &Measurement.Token,
                            &Measurement.Module,
                            &Measurement.StartTimeStamp,
                            &Measurement.EndTimeStamp)) != 0)
    {
      Count++;
      Duration = GetDuration (&Measurement);
      ElapsedTime = DurationInMicroSeconds ( Duration );
      if ((ElapsedTime < mInterestThreshold)                 ||
          (Measurement.EndTimeStamp == 0)                    ||
          (Measurement.Handle == NULL)                       ||
          ((ExcludeFlag) && (GetCumulativeItem(&Measurement) >= 0))
         ) { // Ignore "uninteresting" or excluded records
        continue;
      }
      mGaugeString[0] = 0;    // Empty driver name by default
      AsciiStrToUnicodeStr (Measurement.Token, mUnicodeToken);
      // See if the Handle is in the HandleBuffer
      for (Index = 0; Index < (Size / sizeof(HandleBuffer[0])); Index++) {
        if (Measurement.Handle == HandleBuffer[Index]) {
          GetNameFromHandle (HandleBuffer[Index]); // Name is put into mGaugeString
          break;
        }
      }
      // Ensure that the argument strings are not too long.
      mGaugeString[31] = 0;
      mUnicodeToken[18] = 0;
      if (mGaugeString[0] != 0) {
        // Display the record if it has a valid handle.
        PrintToken (
          STRING_TOKEN (STR_DP_HANDLE_VARS),
          Count,      // 1 based, Which measurement record is being printed
          Index + 1,  // 1 based, Which handle is being printed
          mGaugeString,
          mUnicodeToken,
          ElapsedTime
        );
      }
    }
  }
  if (HandleBuffer != &TempHandle) {
    FreePool (HandleBuffer);
  }
  return Status;
}

/** 
  Gather and print PEIM data.
  
  Only prints complete PEIM records
  
**/
VOID
ProcessPeims(
  VOID
)
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    Duration;
  UINT64                    ElapsedTime;
  EFI_STRING                StringPtr;
  UINTN                     LogEntryKey;
  UINTN                     TIndex;


  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_PEIMS), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? ALit_UNKNOWN: StringPtr);

  PrintToken (STRING_TOKEN (STR_DP_PEIM_SECTION));
  PrintToken (STRING_TOKEN (STR_DP_DASHES));
  TIndex  = 0;
  LogEntryKey = 0;
  while ((LogEntryKey = GetPerformanceMeasurement (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp)) != 0)
  {
    TIndex++;
    if ((Measurement.EndTimeStamp == 0) ||
        (AsciiStrnCmp (Measurement.Token, ALit_PEIM, PERF_TOKEN_LENGTH) != 0)
       ) {
      continue;
    }

    Duration = GetDuration (&Measurement);
    ElapsedTime = DurationInMicroSeconds ( Duration );  // Calculate elapsed time in microseconds
    if (ElapsedTime >= mInterestThreshold) {
      GetNameFromHandle ((EFI_HANDLE) Measurement.Handle); // Name placed in mGaugeString
      PrintToken (STRING_TOKEN (STR_DP_PEIM_STAT2),
            TIndex,   // 1 based, Which measurement record is being printed
            Measurement.Handle,
            mGaugeString,
            ElapsedTime
      );
    }
  }
}

/** 
  Gather and print global data.
  
  Strips out incomplete or "Execution Phase" records
  Only prints records where Handle is NULL
  Increment TIndex for every record, even skipped ones, so that we have an
  indication of every measurement record taken.
  
**/
VOID
ProcessGlobal(
  VOID
)
{
  MEASUREMENT_RECORD        Measurement;
  UINT64                    Duration;
  UINT64                    ElapsedTime;
  EFI_STRING                StringPtr;
  UINTN                     LogEntryKey;
  UINTN                     Index;        // Index, or number, of the measurement record being processed

  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_GENERAL), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? ALit_UNKNOWN: StringPtr);

  PrintToken (STRING_TOKEN (STR_DP_GLOBAL_SECTION));
  PrintToken (STRING_TOKEN (STR_DP_DASHES));

  Index = 1;
  LogEntryKey = 0;

  while ((LogEntryKey = GetPerformanceMeasurement (
                          LogEntryKey,
                          &Measurement.Handle,
                          &Measurement.Token,
                          &Measurement.Module,
                          &Measurement.StartTimeStamp,
                          &Measurement.EndTimeStamp)) != 0)
  {
    AsciiStrToUnicodeStr (Measurement.Module, mGaugeString);
    AsciiStrToUnicodeStr (Measurement.Token, mUnicodeToken);
    if ( ! ( IsPhase( &Measurement)  ||
        (Measurement.Handle != NULL)      ||
        (Measurement.EndTimeStamp == 0)
        ))
    {
      Duration = GetDuration (&Measurement);
      ElapsedTime = DurationInMicroSeconds ( Duration );
      if (ElapsedTime >= mInterestThreshold) {
        PrintToken (
          STRING_TOKEN (STR_DP_FOUR_VARS_2),
          Index,
          mGaugeString,
          mUnicodeToken,
          ElapsedTime
          );
      }
    }
    Index++;
  }
}

/** 
  Gather and print cumulative data.
  
  Traverse the measurement records and:<BR>
  For each record with a Token listed in the CumData array:<BR>
     - Update the instance count and the total, minimum, and maximum durations.
  Finally, print the gathered cumulative statistics.
  
**/
VOID
ProcessCumulative(
  VOID
)
{
  UINT64                    Avgval;         // the computed average duration
  EFI_STRING                StringPtr;
  UINTN                     TIndex;


  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_CUMULATIVE), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? ALit_UNKNOWN: StringPtr);

  PrintToken (STRING_TOKEN (STR_DP_CUMULATIVE_SECT_1));
  PrintToken (STRING_TOKEN (STR_DP_CUMULATIVE_SECT_2));
  PrintToken (STRING_TOKEN (STR_DP_DASHES));

  for ( TIndex = 0; TIndex < NumCum; ++TIndex) {
    Avgval = DivU64x32 (CumData[TIndex].Duration, CumData[TIndex].Count);
    PrintToken (STRING_TOKEN (STR_DP_CUMULATIVE_STATS),
                CumData[TIndex].Name,
                CumData[TIndex].Count,
                DurationInMicroSeconds(CumData[TIndex].Duration),
                DurationInMicroSeconds(Avgval),
                DurationInMicroSeconds(CumData[TIndex].MinDur),
                DurationInMicroSeconds(CumData[TIndex].MaxDur)
               );
  }
}
