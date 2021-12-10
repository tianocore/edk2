/** @file
  Master header files for SmmCorePerformanceLib instance.

  This header file holds the prototypes of the SMM Performance and PerformanceEx Protocol published by this
  library instance at its constructor.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_
#define _SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_

#include <Guid/Performance.h>
#include <Guid/PerformanceMeasurement.h>
#include <Guid/ExtendedFirmwarePerformance.h>
#include <Guid/FirmwarePerformance.h>
#include <Guid/ZeroGuid.h>
#include <Guid/EventGroup.h>

#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PerformanceLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/SmmMemLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include <Protocol/SmmBase2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>

//
// Interface declarations for SMM PerformanceMeasurement Protocol.
//

/**
  Create performance record with event description and a timestamp.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID.
  @param Guid              - Pointer to a GUID.
  @param String            - Pointer to a string describing the measurement.
  @param TimeStamp         - 64-bit time stamp.
  @param Address           - Pointer to a location in memory relevant to the measurement.
  @param Identifier        - Performance identifier describing the type of measurement.
  @param Attribute         - The attribute of the measurement. According to attribute can create a start
                             record for PERF_START/PERF_START_EX, or a end record for PERF_END/PERF_END_EX,
                             or a general record for other Perf macros.

  @retval EFI_SUCCESS           - Successfully created performance record.
  @retval EFI_OUT_OF_RESOURCES  - Ran out of space to store the records.
  @retval EFI_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                  pointer or invalid PerfId.
**/
EFI_STATUS
EFIAPI
CreatePerformanceMeasurement (
  IN CONST VOID                        *CallerIdentifier  OPTIONAL,
  IN CONST VOID                        *Guid      OPTIONAL,
  IN CONST CHAR8                       *String    OPTIONAL,
  IN       UINT64                      TimeStamp  OPTIONAL,
  IN       UINT64                      Address    OPTIONAL,
  IN       UINT32                      Identifier,
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute
  );

#endif
