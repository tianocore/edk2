/** @file
  Header file for 'dp' command functions.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DP_H_
#define _DP_H_

#include <Uefi.h>

#include <Guid/Performance.h>
#include <Guid/ExtendedFirmwarePerformance.h>
#include <Guid/FirmwarePerformance.h>

#include <Protocol/HiiPackageList.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/SortLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/PerformanceLib.h>

extern EFI_HII_HANDLE  mDpHiiHandle;

#define DP_MAJOR_VERSION  2
#define DP_MINOR_VERSION  5

/**
  * The value assigned to DP_DEBUG controls which debug output
  * is generated.  Set it to ZERO to disable.
**/
#define DP_DEBUG  0

#define DEFAULT_THRESHOLD     1000      ///< One millisecond.
#define DEFAULT_DISPLAYCOUNT  50
#define MAXIMUM_DISPLAYCOUNT  999999    ///< Arbitrary maximum reasonable number.

#define PERF_MAXDUR  0xFFFFFFFFFFFFFFFFULL

/// Determine whether  0 <= C < L.  If L == 0, return true regardless of C.
#define WITHIN_LIMIT(C, L)  ( ((L) == 0) || ((C) < (L)) )

/// Structure for storing Timer specific information.
typedef struct {
  UINT64     StartCount;  ///< Value timer is initialized with.
  UINT64     EndCount;    ///< Value timer has just before it wraps.
  UINT32     Frequency;   ///< Timer count frequency in KHz.
  BOOLEAN    CountUp;     ///< TRUE if the counter counts up.
} TIMER_INFO;

/** Initialize one PERF_CUM_DATA structure instance for token t.
  *
  * This parameterized macro takes a single argument, t, which is expected
  * to resolve to a pointer to an ASCII string literal.  This parameter may
  * take any one of the following forms:
  *   - PERF_INIT_CUM_DATA("Token")         A string literal
  *   - PERF_INIT_CUM_DATA(pointer)         A pointer -- CHAR8 *pointer;
  *   - PERF_INIT_CUM_DATA(array)           Address of an array -- CHAR8 array[N];
**/
#define PERF_INIT_CUM_DATA(t)  { 0ULL, PERF_MAXDUR, 0ULL, (t), 0U }

typedef struct {
  UINT64    Duration;   ///< Cumulative duration for this item.
  UINT64    MinDur;     ///< Smallest duration encountered.
  UINT64    MaxDur;     ///< Largest duration encountered.
  CHAR8     *Name;      ///< ASCII name of this item.
  UINT32    Count;      ///< Total number of measurements accumulated.
} PERF_CUM_DATA;

typedef struct {
  UINT32    NumTrace;                     ///< Number of recorded TRACE performance measurements.
  UINT32    NumIncomplete;                ///< Number of measurements with no END value.
  UINT32    NumSummary;                   ///< Number of summary section measurements.
  UINT32    NumHandles;                   ///< Number of measurements with handles.
  UINT32    NumPEIMs;                     ///< Number of measurements of PEIMs.
  UINT32    NumGlobal;                    ///< Number of measurements with END value and NULL handle.
} PERF_SUMMARY_DATA;

typedef struct {
  CONST VOID     *Handle;
  CONST CHAR8    *Token;                  ///< Measured token string name.
  CONST CHAR8    *Module;                 ///< Module string name.
  UINT64         StartTimeStamp;          ///< Start time point.
  UINT64         EndTimeStamp;            ///< End time point.
  UINT32         Identifier;              ///< Identifier.
} MEASUREMENT_RECORD;

typedef struct {
  CHAR8     *Name;                        ///< Measured token string name.
  UINT64    CumulativeTime;               ///< Accumulated Elapsed Time.
  UINT64    MinTime;                      ///< Minimum Elapsed Time.
  UINT64    MaxTime;                      ///< Maximum Elapsed Time.
  UINT32    Count;                        ///< Number of measurements accumulated.
} PROFILE_RECORD;

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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE  ImageHandle
  );

#endif // _DP_H_
