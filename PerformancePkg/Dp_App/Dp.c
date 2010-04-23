/** @file
  * Shell application for Displaying Performance Metrics.
  *
  * The Dp application reads performance data and presents it in several
  * different formats depending upon the needs of the user.  Both
  * Trace and Measured Profiling information is processed and presented.
  *
  * Dp uses the "PerformanceLib" to read the measurement records.
  * The "TimerLib" provides information about the timer, such as frequency,
  * beginning, and ending counter values.
  * Measurement records contain identifying information (Handle, Token, Module)
  * and start and end time values.
  * Dp uses this information to group records in different ways.  It also uses
  * timer information to calculate elapsed time for each measurement.
  *
  * Copyright (c) 2009-2010, Intel Corporation. All rights reserved.<BR>
  * This program and the accompanying materials
  * are licensed and made available under the terms and conditions of the BSD License
  * which accompanies this distribution.  The full text of the license may be found at
  * http://opensource.org/licenses/bsd-license.php
  *
  * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>

#include <Guid/Performance.h>

#include <PerformanceTokens.h>
#include "Dp.h"
#include "Literals.h"
#include "DpInternal.h"

//
/// Module-Global Variables
/// @{
EFI_HII_HANDLE   gHiiHandle;
CHAR16           *mPrintTokenBuffer = NULL;
CHAR16           mGaugeString[DXE_PERFORMANCE_STRING_SIZE];
CHAR16           mUnicodeToken[PERF_TOKEN_LENGTH + 1];
UINT64           mInterestThreshold;

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

SHELL_PARAM_ITEM  DpParamList[] = {
  {STR_DP_OPTION_QH, TypeFlag},   // -?   Help
  {STR_DP_OPTION_LH, TypeFlag},   // -h   Help
  {STR_DP_OPTION_UH, TypeFlag},   // -H   Help
  {STR_DP_OPTION_LV, TypeFlag},   // -v   Verbose Mode
  {STR_DP_OPTION_UA, TypeFlag},   // -A   All, Cooked
  {STR_DP_OPTION_UR, TypeFlag},   // -R   RAW All
  {STR_DP_OPTION_LS, TypeFlag},   // -s   Summary
#if PROFILING_IMPLEMENTED
  {STR_DP_OPTION_UP, TypeFlag},   // -P   Dump Profile Data
  {STR_DP_OPTION_UT, TypeFlag},   // -T   Dump Trace Data
#endif
  {STR_DP_OPTION_LX, TypeFlag},   // -x   eXclude Cumulative Items
  {STR_DP_OPTION_LN, TypeValue},  // -n # Number of records to display for A and R
  {STR_DP_OPTION_LT, TypeValue},  // -t # Threshold of interest
  {NULL, TypeMax}
  };

/// @}

/// Display Usage and Help information.
VOID
ShowHelp( void )
{
  PrintToken (STRING_TOKEN (STR_DP_HELP_HEAD));
#if PROFILING_IMPLEMENTED
  PrintToken (STRING_TOKEN (STR_DP_HELP_FLAGS));
#else
  PrintToken (STRING_TOKEN (STR_DP_HELP_FLAGS_2));
#endif // PROFILING_IMPLEMENTED
  PrintToken (STRING_TOKEN (STR_DP_HELP_PAGINATE));
  PrintToken (STRING_TOKEN (STR_DP_HELP_VERBOSE));
  PrintToken (STRING_TOKEN (STR_DP_HELP_EXCLUDE));
  PrintToken (STRING_TOKEN (STR_DP_HELP_STAT));
  PrintToken (STRING_TOKEN (STR_DP_HELP_ALL));
  PrintToken (STRING_TOKEN (STR_DP_HELP_RAW));
#if PROFILING_IMPLEMENTED
  PrintToken (STRING_TOKEN (STR_DP_HELP_TRACE));
  PrintToken (STRING_TOKEN (STR_DP_HELP_PROFILE));
#endif // PROFILING_IMPLEMENTED
  PrintToken (STRING_TOKEN (STR_DP_HELP_THRESHOLD));
  PrintToken (STRING_TOKEN (STR_DP_HELP_COUNT));
  PrintToken (STRING_TOKEN (STR_DP_HELP_HELP));
  Print(L"\n");
}

/// Display the trailing Verbose information.
VOID
DumpStatistics( void )
{
  EFI_STRING                StringPtr;

  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_STATISTICS), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? ALit_UNKNOWN: StringPtr);

  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMTRACE),       SummaryData.NumTrace);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMINCOMPLETE),  SummaryData.NumIncomplete);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMPHASES),      SummaryData.NumSummary);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMHANDLES),     SummaryData.NumHandles, SummaryData.NumTrace - SummaryData.NumHandles);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMPEIMS),       SummaryData.NumPEIMs);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMGLOBALS),     SummaryData.NumGlobal);
#if PROFILING_IMPLEMENTED
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMPROFILE),     SummaryData.NumProfile);
#endif // PROFILING_IMPLEMENTED
}

/** Dump performance data.
  *
  * @param[in]  ImageHandle     The image handle.
  * @param[in]  SystemTable     The system table.
  *
  * @retval EFI_SUCCESS            Command completed successfully.
  * @retval EFI_INVALID_PARAMETER  Command usage error.
  * @retval value                  Unknown error.
  *
**/
EFI_STATUS
EFIAPI
InitializeDp (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  UINT64                    Freq;
  UINT64                    Ticker;

  LIST_ENTRY                *ParamPackage;
  CONST CHAR16              *CmdLineArg;
  EFI_STRING                StringPtr;
  UINTN                     Number2Display;

  EFI_STATUS                Status;
  BOOLEAN                   SummaryMode     = FALSE;
  BOOLEAN                   VerboseMode     = FALSE;
  BOOLEAN                   AllMode         = FALSE;
  BOOLEAN                   RawMode         = FALSE;
  BOOLEAN                   TraceMode       = FALSE;
  BOOLEAN                   ProfileMode     = FALSE;
  BOOLEAN                   ExcludeMode     = FALSE;


  // Get DP's entry time as soon as possible.
  // This is used as the Shell-Phase end time.
  //
  Ticker  = GetPerformanceCounter ();

  // Register our string package with HII and return the handle to it.
  //
  gHiiHandle = HiiAddPackages (&gEfiCallerIdGuid, ImageHandle, DPStrings, NULL);
  ASSERT (gHiiHandle != NULL);

/****************************************************************************
****            Process Command Line arguments                           ****
****************************************************************************/
  Status = ShellCommandLineParse (DpParamList, &ParamPackage, NULL, TRUE);

  if (EFI_ERROR(Status)) {
    PrintToken (STRING_TOKEN (STR_DP_INVALID_ARG));
    ShowHelp();
  }
  else {
    if (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_QH)  ||
        ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_LH)  ||
        ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_UH))
    {
      ShowHelp();
    }
    else {
      // Boolean Options
      VerboseMode = (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_LV));
      SummaryMode = (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_US) ||
                     ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_LS));
      AllMode     = (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_UA));
      RawMode     = (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_UR));
#if PROFILING_IMPLEMENTED
      TraceMode   = (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_UT));
      ProfileMode = (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_UP));
#endif  // PROFILING_IMPLEMENTED
      ExcludeMode = (ShellCommandLineGetFlag (ParamPackage, STR_DP_OPTION_LX));

      // Options with Values
      CmdLineArg  = ( ShellCommandLineGetValue (ParamPackage, STR_DP_OPTION_LN));
      if (CmdLineArg == NULL) {
        Number2Display = DEFAULT_DISPLAYCOUNT;
      }
      else {
        Number2Display = StrDecimalToUintn(CmdLineArg);
        if (Number2Display == 0) {
          Number2Display = MAXIMUM_DISPLAYCOUNT;
        }
      }
      CmdLineArg  = (ShellCommandLineGetValue (ParamPackage, STR_DP_OPTION_LT));
      if (CmdLineArg == NULL) {
        mInterestThreshold = DEFAULT_THRESHOLD;  // 1ms := 1,000 us
      }
      else {
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

/****************************************************************************
****            Timer specific processing                                ****
****************************************************************************/
      // Get the Performance counter characteristics:
      //          Freq = Frequency in Hz
      //    StartCount = Value loaded into the counter when it starts counting
      //      EndCount = Value counter counts to before it needs to be reset
      //
      Freq = GetPerformanceCounterProperties (&TimerInfo.StartCount, &TimerInfo.EndCount);

      // Convert the Frequency from Hz to KHz
      TimerInfo.Frequency = (UINT32)DivU64x32 (Freq, 1000);

      // Determine in which direction the performance counter counts.
      TimerInfo.CountUp = (BOOLEAN) (TimerInfo.EndCount >= TimerInfo.StartCount);

/****************************************************************************
****            Print heading                                            ****
****************************************************************************/
      // print DP's build version
      PrintToken (STRING_TOKEN (STR_DP_BUILD_REVISION), DP_MAJOR_VERSION, DP_MINOR_VERSION);

      // print performance timer characteristics
      PrintToken (STRING_TOKEN (STR_DP_KHZ), TimerInfo.Frequency);         // Print Timer frequency in KHz

      if ((VerboseMode)   &&
          (! RawMode)
         ) {
        StringPtr = HiiGetString (gHiiHandle,
                      TimerInfo.CountUp ? STRING_TOKEN (STR_DP_UP) : STRING_TOKEN (STR_DP_DOWN),
                      NULL);
        ASSERT (StringPtr != NULL);
        PrintToken (STRING_TOKEN (STR_DP_TIMER_PROPERTIES),   // Print Timer count range and direction
                    StringPtr,
                    TimerInfo.StartCount,
                    TimerInfo.EndCount
                    );
        PrintToken (STRING_TOKEN (STR_DP_VERBOSE_THRESHOLD), mInterestThreshold);
      }

/* **************************************************************************
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
      GatherStatistics();
      if (AllMode) {
        if (TraceMode) {
          DumpAllTrace( Number2Display, ExcludeMode);
        }
        if (ProfileMode) {
          DumpAllProfile( Number2Display, ExcludeMode);
        }
      }
      else if (RawMode) {
        if (TraceMode) {
          DumpRawTrace( Number2Display, ExcludeMode);
        }
        if (ProfileMode) {
          DumpRawProfile( Number2Display, ExcludeMode);
        }
      }
      else {
        //------------- Begin Cooked Mode Processing
        if (TraceMode) {
          ProcessPhases ( Ticker );
          if ( ! SummaryMode) {
            Status = ProcessHandles ( ExcludeMode);
            if ( ! EFI_ERROR( Status)) {
              ProcessPeims (     );
              ProcessGlobal (    );
              ProcessCumulative ();
            }
          }
        }
        if (ProfileMode) {
          DumpAllProfile( Number2Display, ExcludeMode);
        }
      } //------------- End of Cooked Mode Processing
      if ( VerboseMode || SummaryMode) {
        DumpStatistics();
      }
    }
  }
  (void) FreePool (mPrintTokenBuffer);
  HiiRemovePackages (gHiiHandle);
  return Status;
}
