/** @file
  IPMI Command - NetFnSensorEvent NULL instance library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/IpmiCommandLib.h>

/**
  This function sends the Set Sensor Thresholds command for a sensor.

  @param[in]  SetSensorThresholdRequestData  The filled out Set Sensor Thresholds
                                             request.
  @param[out] CompletionCode                 The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSetSensorThreshold (
  IN  IPMI_SENSOR_SET_SENSOR_THRESHOLD_REQUEST_DATA  *SetSensorThresholdRequestData,
  OUT UINT8                                          *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sends the Get Sensor Thresholds command for a sensor.

  @param[in]  SensorNumber                The unique identifier of the sensor being queried.
  @param[out] GetSensorThresholdResponse  The Get Sensor Thresholds response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSensorThreshold (
  IN  UINT8                                           SensorNumber,
  OUT IPMI_SENSOR_GET_SENSOR_THRESHOLD_RESPONSE_DATA  *GetSensorThresholdResponse
  )
{
  return RETURN_UNSUPPORTED;
}
