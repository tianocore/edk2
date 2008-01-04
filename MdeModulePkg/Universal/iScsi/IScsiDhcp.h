/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiDhcp.h

Abstract:

--*/

#ifndef _ISCSI_DHCP_H_
#define _ISCSI_DHCP_H_

#include <Protocol/Dhcp4.h>

#define DHCP4_TAG_PARA_LIST             55
#define DHCP4_TAG_NETMASK               1
#define DHCP4_TAG_ROUTER                3
#define DHCP4_TAG_DNS                   6
#define DHCP4_TAG_SERVER_ID             54
#define DHCP4_TAG_ROOT_PATH             17
#define ISCSI_ROOT_PATH_ID              "iscsi:"
#define ISCSI_ROOT_PATH_FIELD_DELIMITER ':'

enum {
  RP_FIELD_IDX_SERVERNAME = 0,
  RP_FIELD_IDX_PROTOCOL,
  RP_FIELD_IDX_PORT,
  RP_FIELD_IDX_LUN,
  RP_FIELD_IDX_TARGETNAME,
  RP_FIELD_IDX_MAX
};

typedef struct _ISCSI_ROOT_PATH_FIELD {
  CHAR8 *Str;
  UINT8 Len;
} ISCSI_ROOT_PATH_FIELD;

EFI_STATUS
IScsiDoDhcp (
  IN EFI_HANDLE                 Image,
  IN EFI_HANDLE                 Controller,
  IN ISCSI_SESSION_CONFIG_DATA  *ConfigData
  );

#endif
