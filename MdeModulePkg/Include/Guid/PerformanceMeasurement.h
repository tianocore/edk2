/** @file

Performance measurement protocol, allows logging performance data.

Copyright (c) 2017, Microsoft Corporation<BR>
Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PERFORMANCE_MEASUREMENT_H_
#define _PERFORMANCE_MEASUREMENT_H_

//
// GUID for Performance measurement Protocol
//
#define PERFORMANCE_MEASUREMENT_PROTOCOL_GUID \
  { 0xc85d06be, 0x5f75, 0x48ce, {0xa8, 0x0f, 0x12, 0x36, 0xba, 0x3b, 0x87, 0xb1 } }

#define SMM_PERFORMANCE_MEASUREMENT_PROTOCOL_GUID \
  { 0xd56b6d73, 0x1a7b, 0x4015, {0x9b, 0xb4, 0x7b, 0x07, 0x17, 0x29, 0xed, 0x24 } }

typedef struct _EDKII_PERFORMANCE_MEASUREMENT_PROTOCOL EDKII_PERFORMANCE_MEASUREMENT_PROTOCOL;

typedef enum {
  PerfStartEntry,                        // used in StartPerformanceMeasurement()/StartPerformanceMeasurementEx()
                                         // (map to PERF_START/PERF_START_EX)
  PerfEndEntry,                          // used in EndPerformanceMeasurement()/EndPerformanceMeasurementEx()
                                         // (map to PERF_END/PERF_END_EX)
  PerfEntry                              // used in LogPerformanceMeasurement()
                                         // (map to other Perf macros except above 4 macros)
} PERF_MEASUREMENT_ATTRIBUTE;

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
typedef
EFI_STATUS
(EFIAPI *CREATE_PERFORMANCE_MEASUREMENT)(
  IN CONST VOID                        *CallerIdentifier  OPTIONAL,
  IN CONST VOID                        *Guid      OPTIONAL,
  IN CONST CHAR8                       *String    OPTIONAL,
  IN       UINT64                      TimeStamp  OPTIONAL,
  IN       UINT64                      Address    OPTIONAL,
  IN       UINT32                      Identifier,
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute
  );

struct _EDKII_PERFORMANCE_MEASUREMENT_PROTOCOL {
  CREATE_PERFORMANCE_MEASUREMENT    CreatePerformanceMeasurement;
};

extern EFI_GUID  gEdkiiPerformanceMeasurementProtocolGuid;
extern EFI_GUID  gEdkiiSmmPerformanceMeasurementProtocolGuid;

#endif // _PERFORMANCE_MEASUREMENT_H_
