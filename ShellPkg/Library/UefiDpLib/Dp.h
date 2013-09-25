/** @file
  Common declarations for the Dp Performance Reporting Utility.

  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _EFI_APP_DP_H_
#define _EFI_APP_DP_H_

#include <Library/ShellLib.h>
#include <ShellBase.h>

#define DP_MAJOR_VERSION        2
#define DP_MINOR_VERSION        4

/**
  * The value assigned to DP_DEBUG controls which debug output
  * is generated.  Set it to ZERO to disable.
**/
#define DP_DEBUG                0

/**
  * Set to 1 once Profiling has been implemented in order to enable
  * profiling related options and report output.
**/
#define PROFILING_IMPLEMENTED   0

#define DEFAULT_THRESHOLD       1000    ///< One millisecond.
#define DEFAULT_DISPLAYCOUNT    50
#define MAXIMUM_DISPLAYCOUNT    999999  ///< Arbitrary maximum reasonable number.

#define PERF_MAXDUR             0xFFFFFFFFFFFFFFFFULL

/// Determine whether  0 <= C < L.  If L == 0, return true regardless of C.
#define WITHIN_LIMIT( C, L)   ( ((L) == 0) || ((C) < (L)) )

/// Structure for storing Timer specific information.
typedef struct {
  UINT64    StartCount;   ///< Value timer is initialized with.
  UINT64    EndCount;     ///< Value timer has just before it wraps.
  UINT32    Frequency;    ///< Timer count frequency in KHz.
  BOOLEAN   CountUp;      ///< TRUE if the counter counts up.
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
#define PERF_INIT_CUM_DATA(t)   { 0ULL, PERF_MAXDUR, 0ULL, (t), 0U }

typedef struct {
  UINT64  Duration;     ///< Cumulative duration for this item.
  UINT64  MinDur;       ///< Smallest duration encountered.
  UINT64  MaxDur;       ///< Largest duration encountered.
  CHAR8   *Name;        ///< ASCII name of this item.
  UINT32  Count;        ///< Total number of measurements accumulated.
} PERF_CUM_DATA;

typedef struct {
  UINT32                NumTrace;         ///< Number of recorded TRACE performance measurements.
  UINT32                NumProfile;       ///< Number of recorded PROFILE performance measurements.
  UINT32                NumIncomplete;    ///< Number of measurements with no END value.
  UINT32                NumSummary;       ///< Number of summary section measurements.
  UINT32                NumHandles;       ///< Number of measurements with handles.
  UINT32                NumPEIMs;         ///< Number of measurements of PEIMs.
  UINT32                NumGlobal;        ///< Number of measurements with END value and NULL handle.
} PERF_SUMMARY_DATA;

typedef struct {
  CONST VOID            *Handle;
  CONST CHAR8           *Token;           ///< Measured token string name.
  CONST CHAR8           *Module;          ///< Module string name.
  UINT64                StartTimeStamp;   ///< Start time point.
  UINT64                EndTimeStamp;     ///< End time point.
  UINT32                Identifier;       ///< Identifier.
} MEASUREMENT_RECORD;

typedef struct {
  CHAR8                 *Name;            ///< Measured token string name.
  UINT64                CumulativeTime;   ///< Accumulated Elapsed Time.
  UINT64                MinTime;          ///< Minimum Elapsed Time.
  UINT64                MaxTime;          ///< Maximum Elapsed Time.
  UINT32                Count;            ///< Number of measurements accumulated.
} PROFILE_RECORD;

#endif  // _EFI_APP_DP_H_
