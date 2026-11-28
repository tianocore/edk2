/** @file
  IPMI Command - NetFnSensorEvent.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/IpmiCommandLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>
#include <IndustryStandard/Ipmi.h>

/**
  This function sends the Set Sensor Thresholds command for a sensor.

  @param[in]  SetSensorThresholdRequestData  The filled out Set Sensor Thresholds
                                             request.
  @param[out] CompletionCode                 The command completion code.

  @retval EFI_SUCCESS            The command was sent successfully.
  @retval EFI_INVALID_PARAMETER  SetSensorThresholdRequestData or CompletionCode
                                 is NULL.
  @retval Others                 See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetSensorThreshold (
  IN  IPMI_SENSOR_SET_SENSOR_THRESHOLD_REQUEST_DATA  *SetSensorThresholdRequestData,
  OUT UINT8                                          *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      ResponseDataSize;

  if ((SetSensorThresholdRequestData == NULL) || (CompletionCode == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Request or completion code is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  ResponseDataSize = sizeof (*CompletionCode);

  Status = IpmiSubmitCommand (
             IPMI_NETFN_SENSOR_EVENT,
             IPMI_SENSOR_SET_SENSOR_THRESHOLDS,
             (UINT8 *)SetSensorThresholdRequestData,
             sizeof (*SetSensorThresholdRequestData),
             CompletionCode,
             &ResponseDataSize
             );
  return Status;
}

/**
  This function sends the Get Sensor Thresholds command for a sensor.

  @param[in]  SensorNumber                The unique identifier of the sensor being queried.
  @param[out] GetSensorThresholdResponse  The Get Sensor Thresholds response, including
                                          the completion code.

  @retval EFI_SUCCESS            The command was sent successfully.
  @retval EFI_INVALID_PARAMETER  GetSensorThresholdResponse is NULL.
  @retval Others                 See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSensorThreshold (
  IN  UINT8                                           SensorNumber,
  OUT IPMI_SENSOR_GET_SENSOR_THRESHOLD_RESPONSE_DATA  *GetSensorThresholdResponse
  )
{
  EFI_STATUS  Status;
  UINT32      ResponseDataSize;

  if (GetSensorThresholdResponse == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Response is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  ResponseDataSize = sizeof (*GetSensorThresholdResponse);

  ZeroMem (GetSensorThresholdResponse, sizeof (*GetSensorThresholdResponse));

  Status = IpmiSubmitCommand (
             IPMI_NETFN_SENSOR_EVENT,
             IPMI_SENSOR_GET_SENSOR_THRESHOLDS,
             &SensorNumber,
             sizeof (SensorNumber),
             (UINT8 *)GetSensorThresholdResponse,
             &ResponseDataSize
             );
  return Status;
}
