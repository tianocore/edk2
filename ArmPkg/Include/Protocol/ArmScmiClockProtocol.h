/** @file

  Copyright (c) 2017-2021, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#ifndef ARM_SCMI_CLOCK_PROTOCOL_H_
#define ARM_SCMI_CLOCK_PROTOCOL_H_

#include <Protocol/ArmScmi.h>

#define ARM_SCMI_CLOCK_PROTOCOL_GUID { \
  0x91ce67a8, 0xe0aa, 0x4012, {0xb9, 0x9f, 0xb6, 0xfc, 0xf3, 0x4, 0x8e, 0xaa} \
  }

extern EFI_GUID gArmScmiClockProtocolGuid;

// Message Type for clock management protocol.
typedef enum {
  ScmiMessageIdClockAttributes    = 0x3,
  ScmiMessageIdClockDescribeRates = 0x4,
  ScmiMessageIdClockRateSet       = 0x5,
  ScmiMessageIdClockRateGet       = 0x6,
  ScmiMessageIdClockConfigSet     = 0x7
} SCMI_MESSAGE_ID_CLOCK;

typedef enum {
  ScmiClockRateFormatDiscrete, // Non-linear range.
  ScmiClockRateFormatLinear    // Linear range.
} SCMI_CLOCK_RATE_FORMAT;

// Clock management protocol version.
#define SCMI_CLOCK_PROTOCOL_VERSION 0x10000

#define SCMI_CLOCK_PROTOCOL_PENDING_ASYNC_RATES_MASK      0xFFU
#define SCMI_CLOCK_PROTOCOL_PENDING_ASYNC_RATES_SHIFT     16
#define SCMI_CLOCK_PROTOCOL_NUM_CLOCKS_MASK               0xFFFFU

/** Total number of pending asynchronous clock rates changes
  supported by the SCP, Attr Bits[23:16]
*/
#define SCMI_CLOCK_PROTOCOL_MAX_ASYNC_CLK_RATES(Attr) (                       \
                  (Attr >> SCMI_CLOCK_PROTOCOL_PENDING_ASYNC_RATES_SHIFT) &&  \
                   SCMI_CLOCK_PROTOCOL_PENDING_ASYNC_RATES_MASK)

// Total of clock devices supported by the SCP, Attr Bits[15:0]
#define SCMI_CLOCK_PROTOCOL_TOTAL_CLKS(Attr) (Attr & SCMI_CLOCK_PROTOCOL_NUM_CLOCKS_MASK)

#pragma pack(1)

/* Depending on the format (linear/non-linear) supported by a clock device
   either Rate or Min/Max/Step triplet is valid.
*/
typedef struct {
  UINT64 Min;
  UINT64 Max;
  UINT64 Step;
} SCMI_CLOCK_RATE_CONTINUOUS;

typedef struct {
  UINT64 Rate;
} SCMI_CLOCK_RATE_DISCRETE;

typedef union {
  SCMI_CLOCK_RATE_CONTINUOUS ContinuousRate;
  SCMI_CLOCK_RATE_DISCRETE DiscreteRate;
} SCMI_CLOCK_RATE;

#pragma pack()

typedef struct _SCMI_CLOCK_PROTOCOL SCMI_CLOCK_PROTOCOL;

// Protocol Interface functions.

/** Return version of the clock management protocol supported by SCP firmware.

  @param[in]  This     A Pointer to SCMI_CLOCK_PROTOCOL Instance.

  @param[out] Version  Version of the supported SCMI Clock management protocol.

  @retval EFI_SUCCESS       The version is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK_GET_VERSION) (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  OUT UINT32               *Version
  );

/** Return total number of clock devices supported by the clock management
   protocol.

  @param[in]  This         A Pointer to SCMI_CLOCK_PROTOCOL Instance.

  @param[out] TotalClocks  Total number of clocks supported.

  @retval EFI_SUCCESS       Total number of clocks supported is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK_GET_TOTAL_CLOCKS) (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  OUT UINT32               *TotalClocks
  );

/** Return attributes of a clock device.

  @param[in]  This        A Pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.

  @param[out] Enabled         If TRUE, the clock device is enabled.
  @param[out] ClockAsciiName  A NULL terminated ASCII string with the clock
                              name, of up to 16 bytes.

  @retval EFI_SUCCESS          Clock device attributes are returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK_GET_CLOCK_ATTRIBUTES) (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  IN  UINT32               ClockId,
  OUT BOOLEAN              *Enabled,
  OUT CHAR8                *ClockAsciiName
  );

/** Return list of rates supported by a given clock device.

  @param[in] This        A pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in] ClockId     Identifier for the clock device.

  @param[out] Format      ScmiClockRateFormatDiscrete: Clock device
                          supports range of clock rates which are non-linear.

                          ScmiClockRateFormatLinear: Clock device supports
                          range of linear clock rates from Min to Max in steps.

  @param[out] TotalRates  Total number of rates.

  @param[in,out] RateArraySize  Size of the RateArray.

  @param[out] RateArray   List of clock rates.

  @retval EFI_SUCCESS          List of clock rates are returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval EFI_BUFFER_TOO_SMALL RateArraySize is too small for the result.
                               It has been updated to the size needed.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK_DESCRIBE_RATES) (
  IN     SCMI_CLOCK_PROTOCOL     *This,
  IN     UINT32                   ClockId,
  OUT    SCMI_CLOCK_RATE_FORMAT  *Format,
  OUT    UINT32                  *TotalRates,
  IN OUT UINT32                  *RateArraySize,
  OUT    SCMI_CLOCK_RATE         *RateArray
  );

/** Get clock rate.

  @param[in]  This        A Pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.

  @param[out]  Rate       Clock rate.

  @retval EFI_SUCCESS          Clock rate is returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK_RATE_GET) (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  IN  UINT32               ClockId,
  OUT UINT64               *Rate
  );

/** Set clock rate.

  @param[in]  This        A Pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.
  @param[in]  Rate        Clock rate.

  @retval EFI_SUCCESS          Clock rate set success.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK_RATE_SET) (
  IN SCMI_CLOCK_PROTOCOL  *This,
  IN UINT32               ClockId,
  IN UINT64               Rate
  );

typedef struct _SCMI_CLOCK_PROTOCOL {
  SCMI_CLOCK_GET_VERSION GetVersion;
  SCMI_CLOCK_GET_TOTAL_CLOCKS GetTotalClocks;
  SCMI_CLOCK_GET_CLOCK_ATTRIBUTES GetClockAttributes;
  SCMI_CLOCK_DESCRIBE_RATES DescribeRates;
  SCMI_CLOCK_RATE_GET RateGet;
  SCMI_CLOCK_RATE_SET RateSet;
} SCMI_CLOCK_PROTOCOL;

#endif /* ARM_SCMI_CLOCK_PROTOCOL_H_ */

