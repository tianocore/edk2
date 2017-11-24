/** @file
  Shell command for Displaying Performance Metrics.

  The Dp command reads performance data and presents it in several
  different formats depending upon the needs of the user.  Both
  Trace and Measured Profiling information is processed and presented.

  Dp uses the "PerformanceLib" to read the measurement records.
  The "TimerLib" provides information about the timer, such as frequency,
  beginning, and ending counter values.
  Measurement records contain identifying information (Handle, Token, Module)
  and start and end time values.
  Dp uses this information to group records in different ways.  It also uses
  timer information to calculate elapsed time for each measurement.
 
  Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.
  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
 
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "PerformanceTokens.h"
#include "Dp.h"
#include "Literals.h"
#include "DpInternal.h"

EFI_HANDLE   mDpHiiHandle;

//
/// Module-Global Variables
///@{
CHAR16           mGaugeString[DP_GAUGE_STRING_LENGTH + 1];
CHAR16           mUnicodeToken[DXE_PERFORMANCE_STRING_SIZE];
UINT64           mInterestThreshold;
BOOLEAN          mShowId = FALSE;

PERF_SUMMARY_DATA SummaryData = { 0 };    ///< Create the SummaryData structure and init. to ZERO.

/// Timer Specific Information.
TIMER_INFO TimerInfo;

/// Items for which to gather cumulative statistics.
PERF_CUM_DATA CumData[] = {
  PERF_INIT_CUM_DATA (LOAD_IMAGE_TOK),
  PERF_INIT_CUM_DATA (START_IMAGE_TOK),
  PERF_INIT_CUM_DATA (DRIVERBINDING_START_TOK),
  PERF_INIT_CUM_DATA (DRIVERBINDING_SUPPORT_TOK)
};

/// Number of items for which we are gathering cumulative statistics.
UINT32 const      NumCum = sizeof(CumData) / sizeof(PERF_CUM_DATA);

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeFlag},   // -v   Verbose Mode
  {L"-A", TypeFlag},   // -A   All, Cooked
  {L"-R", TypeFlag},   // -R   RAW All
  {L"-s", TypeFlag},   // -s   Summary
#if PROFILING_IMPLEMENTED
  {L"-P", TypeFlag},   // -P   Dump Profile Data
  {L"-T", TypeFlag},   // -T   Dump Trace Data
#endif // PROFILING_IMPLEMENTED
  {L"-x", TypeFlag},   // -x   eXclude Cumulative Items
  {L"-i", TypeFlag},   // -i   Display Identifier
  {L"-c", TypeValue},  // -c   Display cumulative data.
  {L"-n", TypeValue},  // -n # Number of records to display for A and R
  {L"-t", TypeValue},  // -t # Threshold of interest
  {NULL, TypeMax}
  };

///@}

/**
   Display the trailing Verbose information.
**/
VOID
DumpStatistics( void )
{
  EFI_STRING                StringPtr;
  EFI_STRING                StringPtrUnknown;
  StringPtr        = HiiGetString (mDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_STATISTICS), NULL);
  StringPtrUnknown = HiiGetString (mDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), mDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown : StringPtr);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMTRACE), mDpHiiHandle,      SummaryData.NumTrace);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMINCOMPLETE), mDpHiiHandle, SummaryData.NumIncomplete);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMPHASES), mDpHiiHandle,     SummaryData.NumSummary);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMHANDLES), mDpHiiHandle,    SummaryData.NumHandles, SummaryData.NumTrace - SummaryData.NumHandles);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMPEIMS), mDpHiiHandle,      SummaryData.NumPEIMs);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMGLOBALS), mDpHiiHandle,    SummaryData.NumGlobal);
#if PROFILING_IMPLEMENTED
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMPROFILE), mDpHiiHandle,    SummaryData.NumProfile);
#endif // PROFILING_IMPLEMENTED
  SHELL_FREE_NON_NULL (StringPtr);
  SHELL_FREE_NON_NULL (StringPtrUnknown);
}

/**
  Initialize the cumulative data.

**/
VOID
InitCumulativeData (
  VOID
  )
{
  UINTN                             Index;

  for (Index = 0; Index < NumCum; ++Index) {
    CumData[Index].Count = 0;
    CumData[Index].MinDur = PERF_MAXDUR;
    CumData[Index].MaxDur = 0;
    CumData[Index].Duration = 0;
  }
}

/**
  Dump performance data.
  
  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.
  
  @retval SHELL_SUCCESS            Command completed successfully.
  @retval SHELL_INVALID_PARAMETER  Command usage error.
  @retval SHELL_ABORTED            The user aborts the operation.
  @retval value                    Unknown error.
**/
SHELL_STATUS
RunDp (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  LIST_ENTRY                *ParamPackage;
  CONST CHAR16              *CmdLineArg;
  EFI_STATUS                Status;

  PERFORMANCE_PROPERTY      *PerformanceProperty;
  UINTN                     Number2Display;

  EFI_STRING                StringPtr;
  BOOLEAN                   SummaryMode;
  BOOLEAN                   VerboseMode;
  BOOLEAN                   AllMode;
  BOOLEAN                   RawMode;
  BOOLEAN                   TraceMode;
  BOOLEAN                   ProfileMode;
  BOOLEAN                   ExcludeMode;
  BOOLEAN                   CumulativeMode;
  CONST CHAR16              *CustomCumulativeToken;
  PERF_CUM_DATA             *CustomCumulativeData;
  UINTN                     NameSize;
  SHELL_STATUS              ShellStatus;

  StringPtr   = NULL;
  SummaryMode = FALSE;
  VerboseMode = FALSE;
  AllMode     = FALSE;
  RawMode     = FALSE;
  TraceMode   = FALSE;
  ProfileMode = FALSE;
  ExcludeMode = FALSE;
  CumulativeMode = FALSE;
  CustomCumulativeData = NULL;
  ShellStatus = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // Process Command Line arguments
  //
  Status = ShellCommandLineParse (ParamList, &ParamPackage, NULL, TRUE);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_INVALID_ARG), mDpHiiHandle);
    return SHELL_INVALID_PARAMETER;
  }

  //
  // Boolean options
  //
  VerboseMode = ShellCommandLineGetFlag (ParamPackage, L"-v");
  SummaryMode = (BOOLEAN) (ShellCommandLineGetFlag (ParamPackage, L"-S") || ShellCommandLineGetFlag (ParamPackage, L"-s"));
  AllMode     = ShellCommandLineGetFlag (ParamPackage, L"-A");
  RawMode     = ShellCommandLineGetFlag (ParamPackage, L"-R");
#if PROFILING_IMPLEMENTED
  TraceMode   = ShellCommandLineGetFlag (ParamPackage, L"-T");
  ProfileMode = ShellCommandLineGetFlag (ParamPackage, L"-P");
#endif  // PROFILING_IMPLEMENTED
  ExcludeMode = ShellCommandLineGetFlag (ParamPackage, L"-x");
  mShowId     = ShellCommandLineGetFlag (ParamPackage, L"-i");
  CumulativeMode = ShellCommandLineGetFlag (ParamPackage, L"-c");

  // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-n");
  if (CmdLineArg == NULL) {
    Number2Display = DEFAULT_DISPLAYCOUNT;
  } else {
    Number2Display = StrDecimalToUintn(CmdLineArg);
    if (Number2Display == 0) {
      Number2Display = MAXIMUM_DISPLAYCOUNT;
    }
  }

  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-t");
  if (CmdLineArg == NULL) {
    mInterestThreshold = DEFAULT_THRESHOLD;  // 1ms := 1,000 us
  } else {
    mInterestThreshold = StrDecimalToUint64(CmdLineArg);
  }

  // Handle Flag combinations and default behaviors
  // If both TraceMode and ProfileMode are FALSE, set them both to TRUE
  if ((! TraceMode) && (! ProfileMode)) {
    TraceMode   = TRUE;
#if PROFILING_IMPLEMENTED
    ProfileMode = TRUE;
#endif  // PROFILING_IMPLEMENTED
  }

  //
  // Initialize the pre-defined cumulative data.
  //
  InitCumulativeData ();

  //
  // Init the custom cumulative data.
  //
  CustomCumulativeToken = ShellCommandLineGetValue (ParamPackage, L"-c");
  if (CustomCumulativeToken != NULL) {
    CustomCumulativeData = AllocateZeroPool (sizeof (PERF_CUM_DATA));
    if (CustomCumulativeData == NULL) {
      return SHELL_OUT_OF_RESOURCES;
    }
    CustomCumulativeData->MinDur = PERF_MAXDUR;
    CustomCumulativeData->MaxDur = 0;
    CustomCumulativeData->Count  = 0;
    CustomCumulativeData->Duration = 0;
    NameSize = StrLen (CustomCumulativeToken) + 1;
    CustomCumulativeData->Name   = AllocateZeroPool (NameSize);
    if (CustomCumulativeData->Name == NULL) {
      FreePool (CustomCumulativeData);
      return SHELL_OUT_OF_RESOURCES;
    }
    UnicodeStrToAsciiStrS (CustomCumulativeToken, CustomCumulativeData->Name, NameSize);
  }

  //
  // Timer specific processing
  //
  // Get the Performance counter characteristics:
  //          Freq = Frequency in Hz
  //    StartCount = Value loaded into the counter when it starts counting
  //      EndCount = Value counter counts to before it needs to be reset
  //
  Status = EfiGetSystemConfigurationTable (&gPerformanceProtocolGuid, (VOID **) &PerformanceProperty);
  if (EFI_ERROR (Status) || (PerformanceProperty == NULL)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PERF_PROPERTY_NOT_FOUND), mDpHiiHandle);
    goto Done;
  }

  TimerInfo.Frequency  = (UINT32)DivU64x32 (PerformanceProperty->Frequency, 1000);
  TimerInfo.StartCount = PerformanceProperty->TimerStartValue;
  TimerInfo.EndCount   = PerformanceProperty->TimerEndValue;

  // Determine in which direction the performance counter counts.
  TimerInfo.CountUp = (BOOLEAN) (TimerInfo.EndCount >= TimerInfo.StartCount);

  //
  // Print header
  //
  // print DP's build version
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_BUILD_REVISION), mDpHiiHandle, DP_MAJOR_VERSION, DP_MINOR_VERSION);

  // print performance timer characteristics
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_KHZ), mDpHiiHandle, TimerInfo.Frequency);

  if (VerboseMode && !RawMode) {
    StringPtr = HiiGetString (mDpHiiHandle,
                  (EFI_STRING_ID) (TimerInfo.CountUp ? STRING_TOKEN (STR_DP_UP) : STRING_TOKEN (STR_DP_DOWN)), NULL);
    ASSERT (StringPtr != NULL);
    // Print Timer count range and direction
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_TIMER_PROPERTIES), mDpHiiHandle,
                StringPtr,
                TimerInfo.StartCount,
                TimerInfo.EndCount
                );
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_VERBOSE_THRESHOLD), mDpHiiHandle, mInterestThreshold);
  }

/****************************************************************************
****            Print Sections based on command line options
****
****  Option modes have the following priority:
****    v Verbose     --  Valid in combination with any other options
****    t Threshold   --  Modifies All, Raw, and Cooked output
****                      Default is 0 for All and Raw mode
****                      Default is DEFAULT_THRESHOLD for "Cooked" mode
****    n Number2Display  Used by All and Raw mode.  Otherwise ignored.
****    A All         --  R and S options are ignored
****    R Raw         --  S option is ignored
****    s Summary     --  Modifies "Cooked" output only
****    Cooked (Default)
****
****  The All, Raw, and Cooked modes are modified by the Trace and Profile
****  options.
****    !T && !P  := (0) Default, Both are displayed
****     T && !P  := (1) Only Trace records are displayed
****    !T &&  P  := (2) Only Profile records are displayed
****     T &&  P  := (3) Same as Default, both are displayed
****************************************************************************/
  GatherStatistics (CustomCumulativeData);
  if (CumulativeMode) {                       
    ProcessCumulative (CustomCumulativeData);
  } else if (AllMode) {
    if (TraceMode) {
      Status = DumpAllTrace( Number2Display, ExcludeMode);
      if (Status == EFI_ABORTED) {
        ShellStatus = SHELL_ABORTED;
        goto Done;
      }
    }
    if (ProfileMode) {
      DumpAllProfile( Number2Display, ExcludeMode);
    }
  } else if (RawMode) {
    if (TraceMode) {
      Status = DumpRawTrace( Number2Display, ExcludeMode);
      if (Status == EFI_ABORTED) {
        ShellStatus = SHELL_ABORTED;
        goto Done;
      }
    }
    if (ProfileMode) {
      DumpRawProfile( Number2Display, ExcludeMode);
    }
  } else {
    //------------- Begin Cooked Mode Processing
    if (TraceMode) {
      ProcessPhases ();
      if ( ! SummaryMode) {
        Status = ProcessHandles ( ExcludeMode);
        if (Status == EFI_ABORTED) {
          ShellStatus = SHELL_ABORTED;
          goto Done;
        }

        Status = ProcessPeims ();
        if (Status == EFI_ABORTED) {
          ShellStatus = SHELL_ABORTED;
          goto Done;
        }

        Status = ProcessGlobal ();
        if (Status == EFI_ABORTED) {
          ShellStatus = SHELL_ABORTED;
          goto Done;
        }

        ProcessCumulative (NULL);
      }
    }
    if (ProfileMode) {
      DumpAllProfile( Number2Display, ExcludeMode);
    }
  } //------------- End of Cooked Mode Processing
  if ( VerboseMode || SummaryMode) {
    DumpStatistics();
  }

Done:
  if (ParamPackage != NULL) {
    ShellCommandLineFreeVarList (ParamPackage);
  }
  SHELL_FREE_NON_NULL (StringPtr);
  if (CustomCumulativeData != NULL) {
    SHELL_FREE_NON_NULL (CustomCumulativeData->Name);
  }
  SHELL_FREE_NON_NULL (CustomCumulativeData);

  return ShellStatus;
}


/**
  Retrive HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HANDLE
InitializeHiiPackage (
  EFI_HANDLE                  ImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  EFI_HANDLE                  HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  return HiiHandle;
}
