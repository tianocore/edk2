/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#ifndef ARM_SCMI_PERFORMANCE_PROTOCOL_H_
#define ARM_SCMI_PERFORMANCE_PROTOCOL_H_

#include <Protocol/ArmScmi.h>

#define PERFORMANCE_PROTOCOL_VERSION  0x10000

#define ARM_SCMI_PERFORMANCE_PROTOCOL_GUID  { \
  0x9b8ba84, 0x3dd3, 0x49a6, {0xa0, 0x5a, 0x31, 0x34, 0xa5, 0xf0, 0x7b, 0xad} \
  }

extern EFI_GUID gArmScmiPerformanceProtocolGuid;

typedef struct _SCMI_PERFORMANCE_PROTOCOL SCMI_PERFORMANCE_PROTOCOL;

#pragma pack(1)

#define POWER_IN_MW_SHIFT       16
#define POWER_IN_MW_MASK        0x1
#define NUM_PERF_DOMAINS_MASK   0xFFFF

// Total number of performance domains, Attr Bits [15:0]
#define SCMI_PERF_TOTAL_DOMAINS(Attr)  (Attr & NUM_PERF_DOMAINS_MASK)

// A flag to express power values in mW or platform specific way, Attr Bit [16]
#define SCMI_PERF_POWER_IN_MW(Attr)  ((Attr >> POWER_IN_MW_SHIFT) &    \
                                      POWER_IN_MW_MASK)

// Performance protocol attributes return values.
typedef struct {
  UINT32 Attributes;
  UINT64 StatisticsAddress;
  UINT32 StatisticsLen;
} SCMI_PERFORMANCE_PROTOCOL_ATTRIBUTES;

#define SCMI_PERF_SUPPORT_LVL_CHANGE_NOTIFY(Attr) ((Attr >> 28) & 0x1)
#define SCMI_PERF_SUPPORT_LIM_CHANGE_NOTIFY(Attr) ((Attr >> 29) & 0x1)
#define SCMI_PERF_SUPPORT_SET_LVL(Attr) ((Attr >> 30) & 0x1)
#define SCMI_PERF_SUPPORT_SET_LIM(Attr) ((Attr >> 31) & 0x1)
#define SCMI_PERF_RATE_LIMIT(RateLimit) (RateLimit & 0xFFF)

// Performance protocol domain attributes.
typedef struct {
  UINT32 Attributes;
  UINT32 RateLimit;
  UINT32 SustainedFreq;
  UINT32 SustainedPerfLevel;
  UINT8  Name[SCMI_MAX_STR_LEN];
} SCMI_PERFORMANCE_DOMAIN_ATTRIBUTES;

// Worst case latency in microseconds, Bits[15:0]
#define PERF_LATENCY_MASK                          0xFFFF
#define SCMI_PERFORMANCE_PROTOCOL_LATENCY(Latency) (Latency & PERF_LATENCY_MASK)

// Performance protocol performance level.
typedef  struct {
  UINT32 Level;
  UINT32 PowerCost;
  UINT32 Latency;
} SCMI_PERFORMANCE_LEVEL;

// Performance protocol performance limit.
typedef struct {
  UINT32 RangeMax;
  UINT32 RangeMin;
} SCMI_PERFORMANCE_LIMITS;

#pragma pack()

/** Return version of the performance management protocol supported by SCP.
   firmware.

  @param[in]  This      A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.

  @param[out] Version   Version of the supported SCMI performance management
                        protocol.

  @retval EFI_SUCCESS       The version is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_GET_VERSION) (
  IN  SCMI_PERFORMANCE_PROTOCOL  *This,
  OUT UINT32                     *Version
  );

/** Return protocol attributes of the performance management protocol.

  @param[in] This         A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.

  @param[out] Attributes  Protocol attributes.

  @retval EFI_SUCCESS       Protocol attributes are returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_GET_ATTRIBUTES) (
  IN  SCMI_PERFORMANCE_PROTOCOL              *This,
  OUT SCMI_PERFORMANCE_PROTOCOL_ATTRIBUTES   *Attributes

  );

/** Return performance domain attributes.

  @param[in]  This        A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.
  @param[in]  DomainId    Identifier for the performance domain.

  @param[out] Attributes  Performance domain attributes.

  @retval EFI_SUCCESS       Domain attributes are returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_GET_DOMAIN_ATTRIBUTES) (
  IN  SCMI_PERFORMANCE_PROTOCOL           *This,
  IN  UINT32                               DomainId,
  OUT SCMI_PERFORMANCE_DOMAIN_ATTRIBUTES  *DomainAttributes
  );

/** Return list of performance domain levels of a given domain.

  @param[in] This        A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.
  @param[in] DomainId    Identifier for the performance domain.

  @param[out] NumLevels   Total number of levels a domain can support.

  @param[in,out]  LevelArraySize Size of the performance level array.

  @param[out] LevelArray   Array of the performance levels.

  @retval EFI_SUCCESS          Domain levels are returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval EFI_BUFFER_TOO_SMALL LevelArraySize is too small for the result.
                               It has been updated to the size needed.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_DESCRIBE_LEVELS) (
  IN     SCMI_PERFORMANCE_PROTOCOL  *This,
  IN     UINT32                     DomainId,
  OUT    UINT32                     *NumLevels,
  IN OUT UINT32                     *LevelArraySize,
  OUT    SCMI_PERFORMANCE_LEVEL     *LevelArray
  );

/** Set performance limits of a domain.

  @param[in] This        A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.
  @param[in] DomainId    Identifier for the performance domain.
  @param[in] Limit       Performance limit to set.

  @retval EFI_SUCCESS          Performance limits set successfully.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_LIMITS_SET) (
  IN SCMI_PERFORMANCE_PROTOCOL *This,
  IN UINT32                    DomainId,
  IN SCMI_PERFORMANCE_LIMITS   *Limits
  );

/** Get performance limits of a domain.

  @param[in]  This        A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.
  @param[in]  DomainId    Identifier for the performance domain.

  @param[out] Limit       Performance Limits of the domain.

  @retval EFI_SUCCESS          Performance limits are returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_LIMITS_GET) (
  SCMI_PERFORMANCE_PROTOCOL *This,
  UINT32                    DomainId,
  SCMI_PERFORMANCE_LIMITS   *Limits
  );

/** Set performance level of a domain.

  @param[in]  This        A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.
  @param[in]  DomainId    Identifier for the performance domain.
  @param[in]  Level       Performance level of the domain.

  @retval EFI_SUCCESS          Performance level set successfully.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_LEVEL_SET) (
  IN SCMI_PERFORMANCE_PROTOCOL *This,
  IN UINT32                    DomainId,
  IN UINT32                    Level
  );

/** Get performance level of a domain.

  @param[in]  This        A Pointer to SCMI_PERFORMANCE_PROTOCOL Instance.
  @param[in]  DomainId    Identifier for the performance domain.

  @param[out] Level       Performance level of the domain.

  @retval EFI_SUCCESS          Performance level got successfully.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PERFORMANCE_LEVEL_GET) (
  IN  SCMI_PERFORMANCE_PROTOCOL *This,
  IN  UINT32                    DomainId,
  OUT UINT32                    *Level
  );

typedef struct _SCMI_PERFORMANCE_PROTOCOL {
  SCMI_PERFORMANCE_GET_VERSION GetVersion;
  SCMI_PERFORMANCE_GET_ATTRIBUTES GetProtocolAttributes;
  SCMI_PERFORMANCE_GET_DOMAIN_ATTRIBUTES GetDomainAttributes;
  SCMI_PERFORMANCE_DESCRIBE_LEVELS DescribeLevels;
  SCMI_PERFORMANCE_LIMITS_SET LimitsSet;
  SCMI_PERFORMANCE_LIMITS_GET LimitsGet;
  SCMI_PERFORMANCE_LEVEL_SET LevelSet;
  SCMI_PERFORMANCE_LEVEL_GET LevelGet;
} SCMI_PERFORMANCE_PROTOCOL;

typedef enum {
  SCMI_MESSAGE_ID_PERFORMANCE_DOMAIN_ATTRIBUTES = 0x3,
  SCMI_MESSAGE_ID_PERFORMANCE_DESCRIBE_LEVELS   = 0x4,
  SCMI_MESSAGE_ID_PERFORMANCE_LIMITS_SET        = 0x5,
  SCMI_MESSAGE_ID_PERFORMANCE_LIMITS_GET        = 0x6,
  SCMI_MESSAGE_ID_PERFORMANCE_LEVEL_SET         = 0x7,
  SCMI_MESSAGE_ID_PERFORMANCE_LEVEL_GET         = 0x8,
} SCMI_MESSAGE_ID_PERFORMANCE;

#endif /* ARM_SCMI_PERFORMANCE_PROTOCOL_H_ */

