/** @file
  Define the EDKII_DEBUG_PPI that PEIMs can use to dump info to debug port.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_DEBUG_PPI_H__
#define __EDKII_DEBUG_PPI_H__

#include <Pi/PiPeiCis.h>

//
// Global ID for the EDKII_DEBUG_PPI
//
#define EDKII_DEBUG_PPI_GUID \
  { \
    0x999e699c, 0xb013, 0x475e, {0xb1, 0x7b, 0xf3, 0xa8, 0xae, 0x5c, 0x48, 0x75} \
  }

///
/// Forward declaration for the PEI_DEBUG_LIB_DEBUG_PPI EDKII_DEBUG_PPI
///
typedef struct _EDKII_DEBUG_PPI EDKII_DEBUG_PPI;

/**
  Print a debug message to debug output device if the specified error level
  is enabled.

  @param[in] ErrorLevel               The error level of the debug message.
  @param[in] Format                   Format string for the debug message to print.
  @param[in] Marker                   BASE_LIST marker for the variable argument list.

**/
typedef
VOID
(EFIAPI *EDKII_DEBUG_BPRINT)(
  IN UINTN                          ErrorLevel,
  IN CONST CHAR8                    *Format,
  IN BASE_LIST                      Marker
  );

/**
  Print an assert message containing a filename, line number, and description.
  This may be followed by a breakpoint or a dead loop.

  @param[in] FileName                 The pointer to the name of the source file that
                                      generated the assert condition.
  @param[in] LineNumber               The line number in the source file that generated
                                      the assert condition
  @param[in] Description              The pointer to the description of the assert condition.

**/
typedef
VOID
(EFIAPI *EDKII_DEBUG_ASSERT)(
  IN CONST CHAR8                    *FileName,
  IN UINTN                          LineNumber,
  IN CONST CHAR8                    *Description
  );

///
/// This PPI contains a set of services to print message to debug output device
///
struct _EDKII_DEBUG_PPI {
  EDKII_DEBUG_BPRINT    DebugBPrint;
  EDKII_DEBUG_ASSERT    DebugAssert;
};

extern EFI_GUID  gEdkiiDebugPpiGuid;

#endif
