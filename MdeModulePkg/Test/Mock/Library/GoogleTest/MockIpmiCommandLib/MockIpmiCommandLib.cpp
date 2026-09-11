/** @file MockIpmiCommandLib.cpp
  Google Test mocks for IpmiCommandLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockIpmiCommandLib.h>

MOCK_INTERFACE_DEFINITION (MockIpmiCommandLib);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetDeviceId, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSelfTestResult, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiResetWatchdogTimer, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSetWatchdogTimer, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetWatchdogTimer, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSetBmcGlobalEnables, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetBmcGlobalEnables, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiClearMessageFlags, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetMessageFlags, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetMessage, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSendMessage, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSystemUuid, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetChannelInfo, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSystemInterfaceCapability, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSolActivating, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSetSolConfigurationParameters, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSolConfigurationParameters, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetLanConfigurationParameters, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetChassisCapabilities, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetChassisStatus, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiChassisControl, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSetPowerRestorePolicy, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSetSystemBootOptions, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSystemBootOptions, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetFruInventoryAreaInfo, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiReadFruData, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiWriteFruData, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSelInfo, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSelEntry, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiAddSelEntry, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiPartialAddSelEntry, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiClearSel, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSelTime, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSetSelTime, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSdrRepositoryInfo, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSdr, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiSetSensorThreshold, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIpmiCommandLib, IpmiGetSensorThreshold, 2, EFIAPI);
