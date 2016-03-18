/** @file
  Measured Profiling reporting for the Dp utility.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
  Gather and print ALL Profiling Records.
  
  Displays all "interesting" Profile measurements in order.
  The number of records displayed is controlled by:
     - records with a duration less than mInterestThreshold microseconds are not displayed.
     - No more than Limit records are displayed.  A Limit of zero will not limit the output.
     - If the ExcludeFlag is TRUE, records matching entries in the CumData array are not
       displayed.
  
  @pre    The mInterestThreshold global variable is set to the shortest duration to be printed.
           The mGaugeString and mUnicodeToken global arrays are used for temporary string storage.
           They must not be in use by a calling function.
  
  @param[in]    Limit         The number of records to print.  Zero is ALL.
  @param[in]    ExcludeFlag   TRUE to exclude individual Cumulative items from display.
  
**/
VOID
DumpAllProfile(
  IN UINTN      Limit,
  IN BOOLEAN    ExcludeFlag
  )
{
  EFI_STRING    StringPtr;
  EFI_STRING    StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);   
  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_PROFILE), NULL);

  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? StringPtrUnknown: StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);
  return;
}

/** 
  Gather and print Raw Profile Records.
  
  All Profile measurements with a duration greater than or equal to
  mInterestThreshold are printed without interpretation.
  
  The number of records displayed is controlled by:
     - records with a duration less than mInterestThreshold microseconds are not displayed.
     - No more than Limit records are displayed.  A Limit of zero will not limit the output.
     - If the ExcludeFlag is TRUE, records matching entries in the CumData array are not
       displayed.
  
  @pre    The mInterestThreshold global variable is set to the shortest duration to be printed.
  
  @param[in]    Limit         The number of records to print.  Zero is ALL.
  @param[in]    ExcludeFlag   TRUE to exclude individual Cumulative items from display.
  
**/
VOID
DumpRawProfile(
  IN UINTN      Limit,
  IN BOOLEAN    ExcludeFlag
  )
{
  EFI_STRING    StringPtr;
  EFI_STRING    StringPtrUnknown;

  StringPtrUnknown = HiiGetString (gHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);   
  StringPtr = HiiGetString (gHiiHandle, STRING_TOKEN (STR_DP_SECTION_RAWPROFILE), NULL);
  PrintToken( STRING_TOKEN (STR_DP_SECTION_HEADER),
              (StringPtr == NULL) ? StringPtrUnknown: StringPtr);
  FreePool (StringPtr);
  FreePool (StringPtrUnknown);
  return;
}
