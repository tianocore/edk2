/** @file
  EBL commands for Network Devices

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ebl.h"

EFI_STATUS
ParseIp (
  IN  CHAR8           *String,
  OUT EFI_IP_ADDRESS  *Address
  )
{
  Address->v4.Addr[0] = (UINT8)AsciiStrDecimalToUintn (String);
  String = AsciiStrStr(String, ".") + 1;
  Address->v4.Addr[1] = (UINT8)AsciiStrDecimalToUintn (String);
  String = AsciiStrStr(String, ".") + 1;
  Address->v4.Addr[2] = (UINT8)AsciiStrDecimalToUintn (String);
  String = AsciiStrStr(String, ".") + 1;
  Address->v4.Addr[3] = (UINT8)AsciiStrDecimalToUintn (String);

  return EFI_SUCCESS;
}

EFI_STATUS
EblIpCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS        Status = EFI_INVALID_PARAMETER;
  EFI_MAC_ADDRESS   Mac;
  EFI_IP_ADDRESS    Ip;

  if (Argc == 1) {
    // Get current IP/MAC

    // Get current MAC address
    Status = EblGetCurrentMacAddress (&Mac);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    AsciiPrint ("MAC Address:  %02x:%02x:%02x:%02x:%02x:%02x\n", Mac.Addr[0],  Mac.Addr[1],  Mac.Addr[2],  Mac.Addr[3],  Mac.Addr[4],  Mac.Addr[5]);

    // Get current IP address
    Status = EblGetCurrentIpAddress (&Ip);
    if (EFI_ERROR(Status)) {
      AsciiPrint("IP Address is not configured.\n");
      Status = EFI_SUCCESS;
      goto Exit;
    }

    AsciiPrint("IP Address:   %d.%d.%d.%d\n", Ip.v4.Addr[0], Ip.v4.Addr[1],Ip.v4.Addr[2], Ip.v4.Addr[3]);

  } else if ((Argv[1][0] == 'r') && (Argc == 2)) {
    // Get new address via dhcp
    Status = EblPerformDHCP (TRUE);
  } else if ((Argv[1][0] == 's') && (Argc == 3)) {
    // Set static IP
    Status = ParseIp (Argv[2], &Ip);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = EblSetStationIp (&Ip, NULL);
  }

Exit:
  return Status;
}

GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdNetworkTemplate[] =
{
  {
    "ip",
    " ; print current ip address\n\r   [r]; request DHCP address\n\r   [s] ipaddr; set static IP address",
    NULL,
    EblIpCmd
  }
};


/**
  Initialize the commands in this in this file
**/
VOID
EblInitializeNetworkCmd (
  VOID
  )
{
  EblAddCommands (mCmdNetworkTemplate, sizeof (mCmdNetworkTemplate)/sizeof (EBL_COMMAND_TABLE));
}

