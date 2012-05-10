/** @file
  Shell application for Displaying Performance Metrics.

  The Dp application reads performance data and presents it in several
  different formats depending upon the needs of the user.  Both
  Trace and Measured Profiling information is processed and presented.

  Dp uses the "PerformanceLib" to read the measurement records.
  The "TimerLib" provides information about the timer, such as frequency,
  beginning, and ending counter values.
  Measurement records contain identifying information (Handle, Token, Module)
  and start and end time values.
  Dp uses this information to group records in different ways.  It also uses
  timer information to calculate elapsed time for each measurement.
 
  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
 
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
///@{
EFI_HII_HANDLE   gHiiHandle;
SHELL_PARAM_ITEM *DpParamList       = NULL;
CHAR16           *mPrintTokenBuffer = NULL;
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

PARAM_ITEM_LIST  ParamList[] = {
  {STRING_TOKEN (STR_DP_OPTION_QH), TypeFlag},   // -?   Help
  {STRING_TOKEN (STR_DP_OPTION_LH), TypeFlag},   // -h   Help
  {STRING_TOKEN (STR_DP_OPTION_UH), TypeFlag},   // -H   Help
  {STRING_TOKEN (STR_DP_OPTION_LV), TypeFlag},   // -v   Verbose Mode
  {STRING_TOKEN (STR_DP_OPTION_UA), TypeFlag},   // -A   All, Cooked
  {STRING_TOKEN (STR_DP_OPTION_UR), TypeFlag},   // -R   RAW All
  {STRING_TOKEN (STR_DP_OPTION_LS), TypeFlag},   // -s   Summary
#if PROFILING_IMPLEMENTED
  {STRING_TOKEN (STR_DP_OPTION_UP), TypeFlag},   // -P   Dump Profile Data
  {STRING_TOKEN (STR_DP_OPTION_UT), TypeFlag},   // -T   Dump Trace Data
#endif
  {STRING_TOKEN (STR_DP_OPTION_LX), TypeFlag},   // -x   eXclude Cumulative Items
  {STRING_TOKEN (STR_DP_OPTION_LI), TypeFlag},   // -i   Display Identifier
  {STRING_TOKEN (STR_DP_OPTION_LN), TypeValue},  // -n # Number of records to display for A and R
  {STRING_TOKEN (STR_DP_OPTION_LT), TypeValue}   // -t # Threshold of interest
  };

///@}

/**
  Transfer the param list value and get the command line parse.

**/
VOID
InitialShellParamList( void )
{
  UINT32            ListIndex;
  UINT32            ListLength;  

  //
  // Allocate one more for the end tag.
  //
  ListLength = sizeof (ParamList) / sizeof (ParamList[0]) + 1;  
  DpParamList = AllocatePool (sizeof (SHELL_PARAM_ITEM) * ListLength);
  ASSERT (DpParamList != NULL);
  
  for (ListIndex = 0; ListIndex < ListLength - 1; ListIndex ++)
  { 
    DpParamList[ListIndex].Name = HiiGetString (gHiiHandle, ParamList[ListIndex].Token, NULL);      
    DpParamList[ListIndex].Type = ParamList[ListIndex].Type;
  }
  DpParamList[ListIndex].Name = NULL;
  DpParamList[ListIndex].Type = TypeMax;
}

/**
   Display Usage and Help information.
**/
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
  PrintToken (STRING_TOKEN (STR_DP_HELP_ID));
  PrintToken (STRING_TOKEN (STR_DP_HELP_HELP));
  Print(L"\n");
}

/**
   Display the trailing Verbose information.
**/
VOID
DumpStatistics( void )
{
  EFI_STRING                StringPtr;
  EFI_STRING                StringPtrUnknown;
  StringPtr        = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_STATISTICS), NULL);
  StringPtrUnknown = HiiGetString (gHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);  
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? StringPtrUnknown : StringPtr);

  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMTRACE),       SummaryData.NumTrace);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMINCOMPLETE),  SummaryData.NumIncomplete);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMPHASES),      SummaryData.NumSummary);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMHANDLES),     SummaryData.NumHandles, SummaryData.NumTrace - SummaryData.NumHandles);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMPEIMS),       SummaryData.NumPEIMs);
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMGLOBALS),     SummaryData.NumGlobal);
#if PROFILING_IMPLEMENTED
  PrintToken( STRING_TOKEN (STR_DP_STATS_NUMPROFILE),     SummaryData.NumProfile);
#endif // PROFILING_IMPLEMENTED
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);
}

/** 
  Dump performance data.
  
  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.
  
  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_INVALID_PARAMETER  Command usage error.
  @retval value                  Unknown error.
  
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
  UINT32                    ListIndex;
  
  LIST_ENTRY                *ParamPackage;
  CONST CHAR16              *CmdLineArg;
  EFI_STRING                StringPtr;
  UINTN                     Number2Display;

  EFI_STATUS                Status;
  BOOLEAN                   SummaryMode;
  BOOLEAN                   VerboseMode;
  BOOLEAN                   AllMode;
  BOOLEAN                   RawMode;
  BOOLEAN                   TraceMode;
  BOOLEAN                   ProfileMode;
  BOOLEAN                   ExcludeMode;

  EFI_STRING                StringDpOptionQh;
  EFI_STRING                StringDpOptionLh;
  EFI_STRING                StringDpOptionUh;
  EFI_STRING                StringDpOptionLv;
  EFI_STRING                StringDpOptionUs;
  EFI_STRING                StringDpOptionLs;
  EFI_STRING                StringDpOptionUa;
  EFI_STRING                StringDpOptionUr;
  EFI_STRING                StringDpOptionUt;
  EFI_STRING                StringDpOptionUp;
  EFI_STRING                StringDpOptionLx;
  EFI_STRING                StringDpOptionLn;
  EFI_STRING                StringDpOptionLt;
  EFI_STRING                StringDpOptionLi;
  
  SummaryMode     = FALSE;
  VerboseMode     = FALSE;
  AllMode         = FALSE;
  RawMode         = FALSE;
  TraceMode       = FALSE;
  ProfileMode     = FALSE;
  ExcludeMode     = FALSE;

  StringDpOptionQh = NULL;
  StringDpOptionLh = NULL;
  StringDpOptionUh = NULL;
  StringDpOptionLv = NULL;
  StringDpOptionUs = NULL;
  StringDpOptionLs = NULL;
  StringDpOptionUa = NULL;
  StringDpOptionUr = NULL;
  StringDpOptionUt = NULL;
  StringDpOptionUp = NULL;
  StringDpOptionLx = NULL;
  StringDpOptionLn = NULL;
  StringDpOptionLt = NULL;
  StringDpOptionLi = NULL;
  StringPtr        = NULL;

  // Get DP's entry time as soon as possible.
  // This is used as the Shell-Phase end time.
  //
  Ticker  = GetPerformanceCounter ();

  // Register our string package with HII and return the handle to it.
  //
  gHiiHandle = HiiAddPackages (&gEfiCallerIdGuid, ImageHandle, DPStrings, NULL);
  ASSERT (gHiiHandle != NULL);

  // Initial the command list
  //
  InitialShellParamList ();
  
/****************************************************************************
****            Process Command Line arguments                           ****
****************************************************************************/
  Status = ShellCommandLineParse (DpParamList, &ParamPackage, NULL, TRUE);

  if (EFI_ERROR(Status)) {
    PrintToken (STRING_TOKEN (STR_DP_INVALID_ARG));
    ShowHelp();
  }
  else {
    StringDpOptionQh = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_QH), NULL);
    StringDpOptionLh = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_LH), NULL);
    StringDpOptionUh = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_UH), NULL);
    
    if (ShellCommandLineGetFlag (ParamPackage, StringDpOptionQh)  ||
        ShellCommandLineGetFlag (ParamPackage, StringDpOptionLh)  ||
        ShellCommandLineGetFlag (ParamPackage, StringDpOptionUh))
    {
      ShowHelp();
    }
    else {
      StringDpOptionLv = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_LV), NULL);
      StringDpOptionUs = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_US), NULL);
      StringDpOptionLs = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_LS), NULL);
      StringDpOptionUa = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_UA), NULL);
      StringDpOptionUr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_UR), NULL);
      StringDpOptionUt = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_UT), NULL);
      StringDpOptionUp = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_UP), NULL);
      StringDpOptionLx = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_LX), NULL);
      StringDpOptionLn = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_LN), NULL);
      StringDpOptionLt = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_LT), NULL);
      StringDpOptionLi = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_OPTION_LI), NULL);
      
      // Boolean Options
      // 
      VerboseMode = ShellCommandLineGetFlag (ParamPackage, StringDpOptionLv);
      SummaryMode = (BOOLEAN) (ShellCommandLineGetFlag (ParamPackage, StringDpOptionUs) ||
                    ShellCommandLineGetFlag (ParamPackage, StringDpOptionLs));
      AllMode     = ShellCommandLineGetFlag (ParamPackage, StringDpOptionUa);
      RawMode     = ShellCommandLineGetFlag (ParamPackage, StringDpOptionUr);
#if PROFILING_IMPLEMENTED
      TraceMode   = ShellCommandLineGetFlag (ParamPackage, StringDpOptionUt);
      ProfileMode = ShellCommandLineGetFlag (ParamPackage, StringDpOptionUp);
#endif  // PROFILING_IMPLEMENTED
      ExcludeMode = ShellCommandLineGetFlag (ParamPackage, StringDpOptionLx);
      mShowId     =  ShellCommandLineGetFlag (ParamPackage, StringDpOptionLi);

      // Options with Values
      CmdLineArg  = ShellCommandLineGetValue (ParamPackage, StringDpOptionLn);
      if (CmdLineArg == NULL) {
        Number2Display = DEFAULT_DISPLAYCOUNT;
      }
      else {
        Number2Display = StrDecimalToUintn(CmdLineArg);
        if (Number2Display == 0) {
          Number2Display = MAXIMUM_DISPLAYCOUNT;
        }
      }
      CmdLineArg  = ShellCommandLineGetValue (ParamPackage, StringDpOptionLt);
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
                      (EFI_STRING_ID) (TimerInfo.CountUp ? STRING_TOKEN (STR_DP_UP) : STRING_TOKEN (STR_DP_DOWN)),
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

  // Free the memory allocate from HiiGetString
  //
  ListIndex = 0;
  while (DpParamList[ListIndex].Name != NULL) {
    FreePool (DpParamList[ListIndex].Name);
    ListIndex ++;
  }  
  FreePool (DpParamList);

  SafeFreePool (StringDpOptionQh);
  SafeFreePool (StringDpOptionLh);
  SafeFreePool (StringDpOptionUh);
  SafeFreePool (StringDpOptionLv);
  SafeFreePool (StringDpOptionUs);
  SafeFreePool (StringDpOptionLs);
  SafeFreePool (StringDpOptionUa);
  SafeFreePool (StringDpOptionUr);
  SafeFreePool (StringDpOptionUt);
  SafeFreePool (StringDpOptionUp);
  SafeFreePool (StringDpOptionLx);
  SafeFreePool (StringDpOptionLn);
  SafeFreePool (StringDpOptionLt);
  SafeFreePool (StringDpOptionLi);
  SafeFreePool (StringPtr);
  SafeFreePool (mPrintTokenBuffer);

  HiiRemovePackages (gHiiHandle);
  return Status;
}
