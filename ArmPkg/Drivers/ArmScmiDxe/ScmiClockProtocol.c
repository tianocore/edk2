/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ArmScmiClockProtocol.h>
#include <Protocol/ArmScmiClock2Protocol.h>

#include "ArmScmiClockProtocolPrivate.h"
#include "ScmiPrivate.h"

/** Convert to 64 bit value from two 32 bit words.

  @param[in] Low   Lower 32 bits.
  @param[in] High  Higher 32 bits.

  @retval UINT64   64 bit value.
**/
STATIC
UINT64
ConvertTo64Bit (
  IN UINT32 Low,
  IN UINT32 High
  )
{
   return (Low | ((UINT64)High << 32));
}

/** Return version of the clock management protocol supported by SCP firmware.

  @param[in]  This     A Pointer to SCMI_CLOCK_PROTOCOL Instance.

  @param[out] Version  Version of the supported SCMI Clock management protocol.

  @retval EFI_SUCCESS       The version is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
STATIC
EFI_STATUS
ClockGetVersion (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  OUT UINT32               *Version
  )
{
  return ScmiGetProtocolVersion (SCMI_PROTOCOL_ID_CLOCK, Version);
}

/** Return total number of clock devices supported by the clock management
  protocol.

  @param[in]  This         A Pointer to SCMI_CLOCK_PROTOCOL Instance.

  @param[out] TotalClocks  Total number of clocks supported.

  @retval EFI_SUCCESS       Total number of clocks supported is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
STATIC
EFI_STATUS
ClockGetTotalClocks (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  OUT UINT32               *TotalClocks
  )
{
  EFI_STATUS  Status;
  UINT32     *ReturnValues;

  Status = ScmiGetProtocolAttributes (SCMI_PROTOCOL_ID_CLOCK, &ReturnValues);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *TotalClocks = SCMI_CLOCK_PROTOCOL_TOTAL_CLKS (ReturnValues[0]);

  return EFI_SUCCESS;
}

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
STATIC
EFI_STATUS
ClockGetClockAttributes (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  IN  UINT32               ClockId,
  OUT BOOLEAN              *Enabled,
  OUT CHAR8                *ClockAsciiName
  )
{
  EFI_STATUS          Status;

  UINT32              *MessageParams;
  CLOCK_ATTRIBUTES    *ClockAttributes;
  SCMI_COMMAND        Cmd;
  UINT32              PayloadLength;

  Status = ScmiCommandGetPayload (&MessageParams);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *MessageParams = ClockId;

  Cmd.ProtocolId = SCMI_PROTOCOL_ID_CLOCK;
  Cmd.MessageId  = SCMI_MESSAGE_ID_CLOCK_ATTRIBUTES;

  PayloadLength = sizeof (ClockId);

  Status = ScmiCommandExecute (
             &Cmd,
             &PayloadLength,
             (UINT32**)&ClockAttributes
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
   // TRUE if bit 0 of ClockAttributes->Attributes is set.
  *Enabled = CLOCK_ENABLED (ClockAttributes->Attributes);

  AsciiStrCpyS (
    ClockAsciiName,
    SCMI_MAX_STR_LEN,
    (CONST CHAR8*)ClockAttributes->ClockName
    );

  return EFI_SUCCESS;
}

/** Return list of rates supported by a given clock device.

  @param[in] This        A pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in] ClockId     Identifier for the clock device.

  @param[out] Format      SCMI_CLOCK_RATE_FORMAT_DISCRETE: Clock device
                          supports range of clock rates which are non-linear.

                          SCMI_CLOCK_RATE_FORMAT_LINEAR: Clock device supports
                          range of linear clock rates from Min to Max in steps.

  @param[out] TotalRates  Total number of rates.

  @param[in,out] RateArraySize  Size of the RateArray.

  @param[out] RateArray   List of clock rates.

  @retval EFI_SUCCESS          List of clock rates is returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval EFI_BUFFER_TOO_SMALL RateArraySize is too small for the result.
                               It has been updated to the size needed.
  @retval !(EFI_SUCCESS)       Other errors.
**/
STATIC
EFI_STATUS
ClockDescribeRates (
  IN     SCMI_CLOCK_PROTOCOL     *This,
  IN     UINT32                   ClockId,
  OUT    SCMI_CLOCK_RATE_FORMAT  *Format,
  OUT    UINT32                  *TotalRates,
  IN OUT UINT32                  *RateArraySize,
  OUT    SCMI_CLOCK_RATE         *RateArray
  )
{
  EFI_STATUS             Status;

  UINT32                 PayloadLength;
  SCMI_COMMAND           Cmd;
  UINT32                 *MessageParams;
  CLOCK_DESCRIBE_RATES   *DescribeRates;
  CLOCK_RATE_DWORD       *Rate;

  UINT32                 RequiredArraySize = 0;
  UINT32                 RateIndex = 0;
  UINT32                 RateNo;
  UINT32                 RateOffset;

  *TotalRates = 0;

  Status = ScmiCommandGetPayload (&MessageParams);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Cmd.ProtocolId = SCMI_PROTOCOL_ID_CLOCK;
  Cmd.MessageId  = SCMI_MESSAGE_ID_CLOCK_DESCRIBE_RATES;

  *MessageParams++  = ClockId;

  do {

    *MessageParams = RateIndex;

    // Set Payload length, note PayloadLength is a IN/OUT parameter.
    PayloadLength  = sizeof (ClockId) + sizeof (RateIndex);

    // Execute and wait for response on a SCMI channel.
    Status = ScmiCommandExecute (
               &Cmd,
               &PayloadLength,
               (UINT32**)&DescribeRates
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (*TotalRates == 0) {
      // In the first iteration we will get number of returned rates and number
      // of remaining rates. With this information calculate required size
      // for rate array. If provided RateArraySize is less, return an
      // error.

      *Format = RATE_FORMAT (DescribeRates->NumRatesFlags);

      *TotalRates = NUM_RATES (DescribeRates->NumRatesFlags)
                    + NUM_REMAIN_RATES (DescribeRates->NumRatesFlags);

      if (*Format == SCMI_CLOCK_RATE_FORMAT_DISCRETE) {
         RequiredArraySize = (*TotalRates) * sizeof (UINT64);
      } else {
         // We need to return triplet of 64 bit value for each rate
         RequiredArraySize = (*TotalRates) * 3 * sizeof (UINT64);
      }

      if (RequiredArraySize > (*RateArraySize)) {
        *RateArraySize = RequiredArraySize;
        return EFI_BUFFER_TOO_SMALL;
      }
    }

    RateOffset = 0;

    if (*Format == SCMI_CLOCK_RATE_FORMAT_DISCRETE) {
      for (RateNo = 0; RateNo < NUM_RATES (DescribeRates->NumRatesFlags); RateNo++) {
        Rate = &DescribeRates->Rates[RateOffset++];
        // Non-linear discrete rates.
        RateArray[RateIndex++].Rate = ConvertTo64Bit (Rate->Low, Rate->High);
      }
    } else {
      for (RateNo = 0; RateNo < NUM_RATES (DescribeRates->NumRatesFlags); RateNo++) {
        // Linear clock rates from minimum to maximum in steps
        // Minimum clock rate.
        Rate = &DescribeRates->Rates[RateOffset++];
        RateArray[RateIndex].Min = ConvertTo64Bit (Rate->Low, Rate->High);

        Rate = &DescribeRates->Rates[RateOffset++];
        // Maximum clock rate.
        RateArray[RateIndex].Max = ConvertTo64Bit (Rate->Low, Rate->High);

        Rate = &DescribeRates->Rates[RateOffset++];
        // Step.
        RateArray[RateIndex++].Step = ConvertTo64Bit (Rate->Low, Rate->High);
      }
    }
  } while (NUM_REMAIN_RATES (DescribeRates->NumRatesFlags) != 0);

  // Update RateArraySize with RequiredArraySize.
  *RateArraySize = RequiredArraySize;

  return EFI_SUCCESS;
}

/** Get clock rate.

  @param[in]  This        A Pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.

  @param[out]  Rate       Clock rate.

  @retval EFI_SUCCESS          Clock rate is returned.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
STATIC
EFI_STATUS
ClockRateGet (
  IN  SCMI_CLOCK_PROTOCOL  *This,
  IN  UINT32               ClockId,
  OUT UINT64               *Rate
  )
{
  EFI_STATUS     Status;

  UINT32            *MessageParams;
  CLOCK_RATE_DWORD  *ClockRate;
  SCMI_COMMAND      Cmd;

  UINT32         PayloadLength;

  Status = ScmiCommandGetPayload (&MessageParams);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fill arguments for clock protocol command.
  *MessageParams  = ClockId;

  Cmd.ProtocolId  = SCMI_PROTOCOL_ID_CLOCK;
  Cmd.MessageId   = SCMI_MESSAGE_ID_CLOCK_RATE_GET;

  PayloadLength = sizeof (ClockId);

  // Execute and wait for response on a SCMI channel.
  Status = ScmiCommandExecute (
             &Cmd,
             &PayloadLength,
             (UINT32**)&ClockRate
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Rate = ConvertTo64Bit (ClockRate->Low, ClockRate->High);

  return EFI_SUCCESS;
}

/** Set clock rate.

  @param[in]  This        A Pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.
  @param[in]  Rate        Clock rate.

  @retval EFI_SUCCESS          Clock rate set success.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
STATIC
EFI_STATUS
ClockRateSet (
  IN SCMI_CLOCK_PROTOCOL  *This,
  IN UINT32               ClockId,
  IN UINT64               Rate
  )
{
  EFI_STATUS                  Status;
  CLOCK_RATE_SET_ATTRIBUTES   *ClockRateSetAttributes;
  SCMI_COMMAND                Cmd;
  UINT32                      PayloadLength;

  Status = ScmiCommandGetPayload ((UINT32**)&ClockRateSetAttributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fill arguments for clock protocol command.
  ClockRateSetAttributes->ClockId    = ClockId;
  ClockRateSetAttributes->Flags      = CLOCK_SET_DEFAULT_FLAGS;
  ClockRateSetAttributes->Rate.Low   = (UINT32)Rate;
  ClockRateSetAttributes->Rate.High  = (UINT32)(Rate >> 32);

  Cmd.ProtocolId = SCMI_PROTOCOL_ID_CLOCK;
  Cmd.MessageId  = SCMI_MESSAGE_ID_CLOCK_RATE_SET;

  PayloadLength = sizeof (CLOCK_RATE_SET_ATTRIBUTES);

  // Execute and wait for response on a SCMI channel.
  Status = ScmiCommandExecute (
             &Cmd,
             &PayloadLength,
             NULL
             );

  return Status;
}

/** Enable/Disable specified clock.

  @param[in]  This        A Pointer to SCMI_CLOCK_PROTOCOL Instance.
  @param[in]  ClockId     Identifier for the clock device.
  @param[in]  Enable      TRUE to enable, FALSE to disable.

  @retval EFI_SUCCESS          Clock enable/disable successful.
  @retval EFI_DEVICE_ERROR     SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
STATIC
EFI_STATUS
ClockEnable (
  IN SCMI_CLOCK2_PROTOCOL *This,
  IN UINT32               ClockId,
  IN BOOLEAN              Enable
  )
{
  EFI_STATUS                  Status;
  CLOCK_CONFIG_SET_ATTRIBUTES *ClockConfigSetAttributes;
  SCMI_COMMAND                Cmd;
  UINT32                      PayloadLength;

  Status = ScmiCommandGetPayload ((UINT32**)&ClockConfigSetAttributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fill arguments for clock protocol command.
  ClockConfigSetAttributes->ClockId    = ClockId;
  ClockConfigSetAttributes->Attributes = Enable ? BIT0 : 0;

  Cmd.ProtocolId = SCMI_PROTOCOL_ID_CLOCK;
  Cmd.MessageId  = SCMI_MESSAGE_ID_CLOCK_CONFIG_SET;

  PayloadLength = sizeof (CLOCK_CONFIG_SET_ATTRIBUTES);

  // Execute and wait for response on a SCMI channel.
  Status = ScmiCommandExecute (
             &Cmd,
             &PayloadLength,
             NULL
             );

  return Status;
}

// Instance of the SCMI clock management protocol.
STATIC CONST SCMI_CLOCK_PROTOCOL ScmiClockProtocol = {
  ClockGetVersion,
  ClockGetTotalClocks,
  ClockGetClockAttributes,
  ClockDescribeRates,
  ClockRateGet,
  ClockRateSet
 };

// Instance of the SCMI clock management protocol.
STATIC CONST SCMI_CLOCK2_PROTOCOL ScmiClock2Protocol = {
  (SCMI_CLOCK2_GET_VERSION)ClockGetVersion,
  (SCMI_CLOCK2_GET_TOTAL_CLOCKS)ClockGetTotalClocks,
  (SCMI_CLOCK2_GET_CLOCK_ATTRIBUTES)ClockGetClockAttributes,
  (SCMI_CLOCK2_DESCRIBE_RATES)ClockDescribeRates,
  (SCMI_CLOCK2_RATE_GET)ClockRateGet,
  (SCMI_CLOCK2_RATE_SET)ClockRateSet,
  SCMI_CLOCK2_PROTOCOL_VERSION,
  ClockEnable
 };

/** Initialize clock management protocol and install protocol on a given handle.

  @param[in] Handle              Handle to install clock management protocol.

  @retval EFI_SUCCESS            Clock protocol interface installed successfully.
**/
EFI_STATUS
ScmiClockProtocolInit (
  IN EFI_HANDLE* Handle
  )
{
  return gBS->InstallMultipleProtocolInterfaces (
                Handle,
                &gArmScmiClockProtocolGuid,
                &ScmiClockProtocol,
                &gArmScmiClock2ProtocolGuid,
                &ScmiClock2Protocol,
                NULL
                );
}
