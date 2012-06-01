/** @file
  Routines used to operate the Ip4 configure variable.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip4Config.h"
#include "NicIp4Variable.h"

BOOLEAN  mIp4ConfigVariableReclaimed = FALSE;

/**
  Check whether the configure parameter is valid.

  @param  NicConfig    The configure parameter to check

  @return TRUE if the parameter is valid for the interface, otherwise FALSE.

**/
BOOLEAN
Ip4ConfigIsValid (
  IN NIC_IP4_CONFIG_INFO    *NicConfig
  )
{
  EFI_IP4_IPCONFIG_DATA     *IpConfig;
  IP4_ADDR                  Station;
  IP4_ADDR                  Netmask;
  IP4_ADDR                  Gateway;
  UINT32                    Index;

  IpConfig = &NicConfig->Ip4Info;

  if (NicConfig->Source == IP4_CONFIG_SOURCE_STATIC) {
    //
    // Validate that the addresses are unicast and mask
    // is properly formated
    //
    Station = EFI_NTOHL (IpConfig->StationAddress);
    Netmask = EFI_NTOHL (IpConfig->SubnetMask);

    if ((Netmask == 0) || !IP4_IS_VALID_NETMASK (Netmask) ||
        (Station == 0) || !NetIp4IsUnicast (Station, Netmask)) {
      return FALSE;
    }

    //
    // Validate that the next hops are on the connected network
    // or that is a direct route (Gateway == 0).
    //
    for (Index = 0; Index < IpConfig->RouteTableSize; Index++) {
      Gateway = EFI_NTOHL (IpConfig->RouteTable[Index].GatewayAddress);

      if ((Gateway != 0) && (!IP4_NET_EQUAL (Station, Gateway, Netmask) ||
          !NetIp4IsUnicast (Gateway, Netmask))) {
        return FALSE;
      }
    }

    return TRUE;
  }

  //
  // return false if it is an unkown configure source. Valid
  // sources are static and dhcp.
  //
  return (BOOLEAN) (NicConfig->Source == IP4_CONFIG_SOURCE_DHCP);
}



/**
  Read the ip4 configure variable from the EFI variable.

  @param  Instance     The IP4 CONFIG instance.

  @return The IP4 configure read if it is there and is valid, otherwise NULL.

**/
NIC_IP4_CONFIG_INFO *
Ip4ConfigReadVariable (
  IN  IP4_CONFIG_INSTANCE   *Instance
  )
{
  NIC_IP4_CONFIG_INFO *NicConfig;

  GetVariable2 (Instance->MacString, &gEfiNicIp4ConfigVariableGuid, (VOID**)&NicConfig, NULL);
  if (NicConfig != NULL) {
    Ip4ConfigFixRouteTablePointer (&NicConfig->Ip4Info);
  }

  return NicConfig;
}

/**
  Write the IP4 configure variable to the NVRAM. If Config
  is NULL, remove the variable.

  @param  Instance     The IP4 CONFIG instance.
  @param  NicConfig    The IP4 configure data to write.

  @retval EFI_SUCCESS  The variable is written to the NVRam.
  @retval Others       Failed to write the variable.

**/
EFI_STATUS
Ip4ConfigWriteVariable (
  IN IP4_CONFIG_INSTANCE    *Instance,
  IN NIC_IP4_CONFIG_INFO    *NicConfig OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (
                  Instance->MacString,
                  &gEfiNicIp4ConfigVariableGuid,
                  IP4_CONFIG_VARIABLE_ATTRIBUTES,
                  (NicConfig == NULL) ? 0 : SIZEOF_NIC_IP4_CONFIG_INFO (NicConfig),
                  NicConfig
                  );

  return Status;
}

/**
  Check whether a NIC exist in the platform given its MAC address.

  @param  NicAddr      The MAC address for the NIC to be checked.

  @retval TRUE         The NIC exist in the platform.
  @retval FALSE        The NIC doesn't exist in the platform.

**/
BOOLEAN
Ip4ConfigIsNicExist (
  IN NIC_ADDR               *NicAddr
  )
{
  EFI_STATUS      Status;
  EFI_HANDLE      *HandleBuffer;
  UINTN           NumberOfHandles;
  UINTN           Index;
  BOOLEAN         Found;
  UINTN           AddrSize;
  EFI_MAC_ADDRESS MacAddr;

  //
  // Locate Service Binding handles.
  //
  Status = gBS->LocateHandleBuffer (
                 ByProtocol,
                 &gEfiManagedNetworkServiceBindingProtocolGuid,
                 NULL,
                 &NumberOfHandles,
                 &HandleBuffer
                 );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Found = FALSE;
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get MAC address.
    //
    AddrSize = 0;
    Status = NetLibGetMacAddress (HandleBuffer[Index], &MacAddr, &AddrSize);
    if (EFI_ERROR (Status)) {
      Found = FALSE;
      goto Exit;
    }

    if ((NicAddr->Len == AddrSize) && (CompareMem (NicAddr->MacAddr.Addr, MacAddr.Addr, AddrSize) == 0)) {
      Found = TRUE;
      goto Exit;
    }
  }

Exit:
  FreePool (HandleBuffer);
  return Found;
}

/**
  Reclaim Ip4Config Variables for NIC which has been removed from the platform.

**/
VOID
Ip4ConfigReclaimVariable (
  VOID
  )
{
  EFI_STATUS           Status;
  UINTN                VariableNameSize;
  CHAR16               *VariableName;
  CHAR16               *CurrentVariableName;
  EFI_GUID             VendorGuid;
  UINTN                VariableNameBufferSize;
  NIC_IP4_CONFIG_INFO  *NicConfig;

  //
  // Check whether we need perform reclaim.
  //
  if (mIp4ConfigVariableReclaimed) {
    return;
  }
  mIp4ConfigVariableReclaimed = TRUE;

  //
  // Get all Ip4Config Variable.
  //
  VariableNameSize = sizeof (CHAR16);
  VariableName = AllocateZeroPool (VariableNameSize);
  VariableNameBufferSize = VariableNameSize;

  while (TRUE) {
    Status = gRT->GetNextVariableName (
                    &VariableNameSize,
                    VariableName,
                    &VendorGuid
                    );

Check:
    if (Status == EFI_BUFFER_TOO_SMALL) {
      VariableName = ReallocatePool (VariableNameBufferSize, VariableNameSize, VariableName);
      VariableNameBufferSize = VariableNameSize;
      //
      // Try again using the new buffer.
      //
      Status = gRT->GetNextVariableName (
                      &VariableNameSize,
                      VariableName,
                      &VendorGuid
                      );
    }

    if (EFI_ERROR (Status)) {
      //
      // No more variable available, finish search.
      //
      break;
    }

    //
    // Check variable GUID.
    //
    if (!CompareGuid (&VendorGuid, &gEfiNicIp4ConfigVariableGuid)) {
      continue;
    }

    GetVariable2 (VariableName, &gEfiNicIp4ConfigVariableGuid, (VOID**)&NicConfig, NULL);
    if (NicConfig == NULL) {
      break;
    }

    if (!Ip4ConfigIsNicExist (&NicConfig->NicAddr)) {
      //
      // No NIC found for this Ip4Config variable, remove it.
      // Since we are in loop of GetNextVariableName(), we need move on to next
      // Variable first and then delete current Variable.
      //
      CurrentVariableName = AllocateCopyPool (VariableNameSize, VariableName);
      Status = gRT->GetNextVariableName (
                      &VariableNameSize,
                      VariableName,
                      &VendorGuid
                      );

      gRT->SetVariable (
             CurrentVariableName,
             &gEfiNicIp4ConfigVariableGuid,
             IP4_CONFIG_VARIABLE_ATTRIBUTES,
             0,
             NULL
             );
      FreePool (CurrentVariableName);

      //
      // We already get next variable, go to check it.
      //
      goto Check;
    }
  }

  FreePool (VariableName);
}

/**
  Fix the RouteTable pointer in an EFI_IP4_IPCONFIG_DATA structure.

  The pointer is set to be immediately follow the ConfigData if there're entries
  in the RouteTable. Otherwise it is set to NULL.

  @param  ConfigData     The IP4 IP configure data.

**/
VOID
Ip4ConfigFixRouteTablePointer (
  IN OUT EFI_IP4_IPCONFIG_DATA  *ConfigData
  )
{
  //
  // The memory used for route table entries must immediately follow
  // the ConfigData and be not packed.
  //
  if (ConfigData->RouteTableSize > 0) {
    ConfigData->RouteTable = (EFI_IP4_ROUTE_TABLE *) (ConfigData + 1);
  } else {
    ConfigData->RouteTable = NULL;
  }
}

