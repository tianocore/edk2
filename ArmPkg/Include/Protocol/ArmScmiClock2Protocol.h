/** @file

  Copyright (c) 2017-2021, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#ifndef ARM_SCMI_CLOCK2_PROTOCOL_H_
#define ARM_SCMI_CLOCK2_PROTOCOL_H_

#include <Protocol/ArmScmi.h>
#include <Protocol/ArmScmiClockProtocol.h>

#define ARM_SCMI_CLOCK2_PROTOCOL_GUID  {\
  0xb8d8caf2, 0x9e94, 0x462c, { 0xa8, 0x34, 0x6c, 0x99, 0xfc, 0x05, 0xef, 0xcf } \
  }

extern EFI_GUID  gArmScmiClock2ProtocolGuid;

#define SCMI_CLOCK2_PROTOCOL_VERSION  1

typedef struct _SCMI_CLOCK2_PROTOCOL SCMI_CLOCK2_PROTOCOL;

// Protocol Interface functions.

/** Return version of the clock management protocol supported by SCP firmware.

  @param[in]  This     A Pointer to SCMI_CLOCK2_PROTOCOL Instance.

  @param[out] Version  Version of the supported SCMI Clock management protocol.

  @retval EFI_SUCCESS       The version is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK2_GET_VERSION)(
  IN  SCMI_CLOCK2_PROTOCOL  *This,
  OUT UINT32                *Version
  );

/** Return total number of clock devices supported by the clock management
   protocol.

  @param[in]  This         A Pointer to SCMI_CLOCK2_PROTOCOL Instance.

  @param[out] TotalClocks  Total number of clocks supported.

  @retval EFI_SUCCESS       Total number of clocks supported is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK2_GET_TOTAL_CLOCKS)(
  IN  SCMI_CLOCK2_PROTOCOL  *This,
  OUT UINT32                *TotalClocks
  );

/** Return attributes of a clock device.

  @param[in]  This        A Pointer to SCMI_CLOCK2_PROTOCOL Instance.
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
(EFIAPI *SCMI_CLOCK2_GET_CLOCK_ATTRIBUTES)(
  IN  SCMI_CLOCK2_PROTOCOL  *This,
  IN  UINT32                ClockId,
  OUT BOOLEAN               *Enabled,
  OUT CHAR8                 *ClockAsciiName
  );

/** Return list of rates supported by a given clock device.

  @param[in] This        A pointer to SCMI_CLOCK2_PROTOCOL Instance.
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
(EFIAPI *SCMI_CLOCK2_DESCRIBE_RATES)(
  IN     SCMI_CLOCK2_PROTOCOL     *This,
  IN     UINT32                   ClockId,
  OUT    SCMI_CLOCK_RATE_FORMAT   *Format,
  OUT    UINT32                   *TotalRates,
  IN OUT UINT32                   *RateArraySize,
  OUT    SCMI_CLOCK_RATE          *RateArray
  );

/** Get clock rate.

  @param[in]  This        A Pointer to SCMI_CLOCK2_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.

  @param[out]  Rate       Clock rate.

  @retval EFI_SUCCESS          Clock rate is returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK2_RATE_GET)(
  IN  SCMI_CLOCK2_PROTOCOL  *This,
  IN  UINT32                ClockId,
  OUT UINT64                *Rate
  );

/** Set clock rate.

  @param[in]  This        A Pointer to SCMI_CLOCK2_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.
  @param[in]  Rate        Clock rate.

  @retval EFI_SUCCESS          Clock rate set success.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK2_RATE_SET)(
  IN SCMI_CLOCK2_PROTOCOL   *This,
  IN UINT32                 ClockId,
  IN UINT64                 Rate
  );

/** Enable/Disable specified clock.
    Function is only available under gArmScmiClock2ProtocolGuid

  @param[in]  This        A Pointer to SCMI_CLOCK2_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.
  @param[in]  Enable      TRUE to enable, FALSE to disable.

  @retval EFI_SUCCESS          Clock enable/disable successful.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_CLOCK2_ENABLE)(
  IN SCMI_CLOCK2_PROTOCOL   *This,
  IN UINT32                 ClockId,
  IN BOOLEAN                Enable
  );

typedef struct _SCMI_CLOCK2_PROTOCOL {
  SCMI_CLOCK2_GET_VERSION             GetVersion;
  SCMI_CLOCK2_GET_TOTAL_CLOCKS        GetTotalClocks;
  SCMI_CLOCK2_GET_CLOCK_ATTRIBUTES    GetClockAttributes;
  SCMI_CLOCK2_DESCRIBE_RATES          DescribeRates;
  SCMI_CLOCK2_RATE_GET                RateGet;
  SCMI_CLOCK2_RATE_SET                RateSet;

  // Extension to original ClockProtocol, added here so SCMI_CLOCK2_PROTOCOL
  // can be cast to SCMI_CLOCK_PROTOCOL
  UINTN                               Version; // For future expandability
  SCMI_CLOCK2_ENABLE                  Enable;
} SCMI_CLOCK2_PROTOCOL;

#endif /* ARM_SCMI_CLOCK2_PROTOCOL_H_ */
