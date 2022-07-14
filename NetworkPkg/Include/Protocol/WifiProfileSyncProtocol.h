/** @file
  Intel One Click Recovery WiFi Profile Sync Profile Protocol.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef WIFI_PROFILE_SYNC_PROTOCOL_H_
#define WIFI_PROFILE_SYNC_PROTOCOL_H_

#include <WifiConnectionManagerDxe/WifiConnectionMgrConfig.h>

//
//  WiFi Profile Sync Protocol GUID variable.
//
extern EFI_GUID  gEfiWiFiProfileSyncProtocolGuid;

/**
  Used by the WiFi connection manager to get the WiFi profile that ASF shared
  and was stored in WiFi profile protocol. Aligns the ASF WiFi profile data to
  the WCM profile structure.

  @param[in, out]  WcmProfile       WiFi Connection Manager profile structure
  @param[in, out]  MacAddress       MAC address from AMT saved to NiC MAC address

  @retval EFI_SUCCESS               Profiles returned
  @retval EFI_UNSUPPORTED           Profile protocol sharing not supported or enabled
  @retval EFI_NOT_FOUND             No profiles returned
  @retval Others                    Error Occurred
**/
typedef
EFI_STATUS
(EFIAPI *WIFI_PROFILE_GET)(
  IN OUT  WIFI_MGR_NETWORK_PROFILE  *Profile,
  IN OUT  EFI_80211_MAC_ADDRESS     MacAddress
  );

/**
  Sets the WiFi connection status recieved by the WiFiConnectionManager.
  Input as EFI_80211_CONNECT_NETWORK_RESULT_CODE then converted and stored
  as EFI_STATUS type.

  @param[in] ConnectionStatus     WiFi connection attempt results
**/
typedef
VOID
(EFIAPI *WIFI_SET_CONNECT_STATE)(
  IN EFI_80211_CONNECT_NETWORK_RESULT_CODE  ConnectionStatus
  );

/**
  Retrieves the WiFi connection status when in either KVM OR OCR WLAN recovery.

  @retval EFI_SUCCESS               WiFi connection completed succesfully
  @retval Others                    Error Occurred
**/
typedef
EFI_STATUS
(EFIAPI *WIFI_GET_CONNECT_STATE)(
  VOID
  );

//
//  WiFi Profile Sync Protocol structure.
//
typedef struct {
  UINT32                    Revision;
  WIFI_SET_CONNECT_STATE    WifiProfileSyncSetConnectState;
  WIFI_GET_CONNECT_STATE    WifiProfileSyncGetConnectState;
  WIFI_PROFILE_GET          WifiProfileSyncGetProfile;
} EFI_WIFI_PROFILE_SYNC_PROTOCOL;

/**
  Intel WiFi Profile Protocol revision number.

  Revision 1:   Initial version
**/
#define  EFI_WIFI_PROFILE_SYNC_PROTOCOL_REVISION  1

#endif // WIFI_PROFILE_SYNC_PROTOCOL_H_
