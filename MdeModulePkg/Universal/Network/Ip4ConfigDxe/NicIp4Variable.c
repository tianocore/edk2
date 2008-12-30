/** @file
  Routines used to operate the Ip4 configure variable

Copyright (c) 2006 - 2008, Intel Corporation.<BR>                                                         
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "NicIp4Variable.h"


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
        (Station == 0) || !Ip4IsUnicast (Station, Netmask)) {
      return FALSE;
    }

    //
    // Validate that the next hops are on the connected network
    // or that is a direct route (Gateway == 0).
    //
    for (Index = 0; Index < IpConfig->RouteTableSize; Index++) {
      Gateway = EFI_NTOHL (IpConfig->RouteTable[Index].GatewayAddress);

      if ((Gateway != 0) && (!IP4_NET_EQUAL (Station, Gateway, Netmask) ||
          !Ip4IsUnicast (Gateway, Netmask))) {
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

  None

  @return The IP4 configure read if it is there and is valid, otherwise NULL

**/
IP4_CONFIG_VARIABLE *
Ip4ConfigReadVariable (
  VOID
  )
{
  IP4_CONFIG_VARIABLE       *Variable;
  EFI_STATUS                Status;
  UINTN                     Size;
  UINT16                    CheckSum;

  //
  // Get the size of variable, then allocate a buffer to read the variable.
  //
  Size     = 0;
  Variable = NULL;
  Status   = gRT->GetVariable (
                    EFI_NIC_IP4_CONFIG_VARIABLE,
                    &gEfiNicIp4ConfigVariableGuid,
                    NULL,
                    &Size,
                    NULL
                    );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  if (Size < sizeof (IP4_CONFIG_VARIABLE)) {
    goto REMOVE_VARIABLE;
  }

  Variable = AllocatePool (Size);

  if (Variable == NULL) {
    return NULL;
  }

  Status = gRT->GetVariable (
                  EFI_NIC_IP4_CONFIG_VARIABLE,
                  &gEfiNicIp4ConfigVariableGuid,
                  NULL,
                  &Size,
                  Variable
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Verify the checksum, variable size and count
  //
  CheckSum = (UINT16) (~NetblockChecksum ((UINT8 *) Variable, (UINT32)Size));

  if ((CheckSum != 0) || (Size != Variable->Len)) {
    goto REMOVE_VARIABLE;
  }

  if ((Variable->Count < 1) || (Variable->Count > MAX_IP4_CONFIG_IN_VARIABLE)) {
    goto REMOVE_VARIABLE;
  }

  return Variable;

REMOVE_VARIABLE:
  Ip4ConfigWriteVariable (NULL);

ON_ERROR:
  if (Variable != NULL) {
    gBS->FreePool (Variable);
  }

  return NULL;
}


/**
  Write the IP4 configure variable to the NVRAM. If Config
  is NULL, remove the variable.

  @param  Config       The IP4 configure data to write

  @retval EFI_SUCCESS  The variable is written to the NVRam
  @retval Others       Failed to write the variable.

**/
EFI_STATUS
Ip4ConfigWriteVariable (
  IN IP4_CONFIG_VARIABLE    *Config        OPTIONAL
  )
{
  EFI_STATUS                Status;

  Status = gRT->SetVariable (
                  EFI_NIC_IP4_CONFIG_VARIABLE,
                  &gEfiNicIp4ConfigVariableGuid,
                  IP4_CONFIG_VARIABLE_ATTRIBUTES,
                  (Config == NULL) ? 0 : Config->Len,
                  Config
                  );

  return Status;
}


/**
  Locate the IP4 configure parameters from the variable.If a
  configuration is found, copy it to a newly allocated block
  of memory to avoid the alignment problem. Caller should
  release the memory after use.

  @param  Variable     The IP4 configure variable to search in
  @param  NicAddr      The interface address to check

  @return The point to the NIC's IP4 configure info if it is found
          in the IP4 variable, otherwise NULL.

**/
NIC_IP4_CONFIG_INFO *
Ip4ConfigFindNicVariable (
  IN IP4_CONFIG_VARIABLE    *Variable,
  IN NIC_ADDR               *NicAddr
  )
{
  NIC_IP4_CONFIG_INFO       Temp;
  NIC_IP4_CONFIG_INFO       *Config;
  UINT32                    Index;
  UINT8                     *Cur;
  UINT32                    Len;

  Cur = (UINT8*)&Variable->ConfigInfo;

  for (Index = 0; Index < Variable->Count; Index++) {
    //
    // Copy the data to Temp to avoid the alignment problems
    //
    CopyMem (&Temp, Cur, sizeof (NIC_IP4_CONFIG_INFO));
    Len = SIZEOF_NIC_IP4_CONFIG_INFO (&Temp);

    //
    // Found the matching configuration parameters, allocate
    // a block of memory then copy it out.
    //
    if (NIC_ADDR_EQUAL (&Temp.NicAddr, NicAddr)) {
      Config = AllocatePool (Len);

      if (Config == NULL) {
        return NULL;
      }

      CopyMem (Config, Cur, Len);
      Ip4ConfigFixRouteTablePointer (&Config->Ip4Info);
      return Config;
    }

    Cur += Len;
  }

  return NULL;
}


/**
  Modify the configuration parameter for the NIC in the variable.
  If Config is NULL, old configuration will be remove from the new
  variable. Otherwise, append it or replace the old one.

  @param  Variable     The IP4 variable to change
  @param  NicAddr      The interface to search
  @param  Config       The new configuration parameter (NULL to remove the old)

  @return The new IP4_CONFIG_VARIABLE variable if the new variable has at
          least one NIC configure and no EFI_OUT_OF_RESOURCES failure.
          Return NULL either because failed to locate memory for new variable
          or the only NIC configure is removed from the Variable.

**/
IP4_CONFIG_VARIABLE *
Ip4ConfigModifyVariable (
  IN IP4_CONFIG_VARIABLE    *Variable,    OPTIONAL
  IN NIC_ADDR               *NicAddr,
  IN NIC_IP4_CONFIG_INFO    *Config       OPTIONAL
  )
{
  NIC_IP4_CONFIG_INFO       Temp;
  NIC_IP4_CONFIG_INFO       *Old;
  IP4_CONFIG_VARIABLE       *NewVar;
  UINT32                    Len;
  UINT32                    TotalLen;
  UINT32                    Count;
  UINT8                     *Next;
  UINT8                     *Cur;
  UINT32                    Index;

  ASSERT ((Variable != NULL) || (Config != NULL));

  //
  // Compute the total length
  //
  if (Variable != NULL) {
    //
    // Variable != NULL, then Config can be NULL or not.  and so is
    // the Old. If old configure exists, it is removed from the
    // Variable. New configure is append to the variable.
    //
    //
    Count     = Variable->Count;
    Cur       = (UINT8 *)&Variable->ConfigInfo;
    TotalLen  = Variable->Len;

    Old       = Ip4ConfigFindNicVariable (Variable, NicAddr);

    if (Old != NULL) {
      TotalLen -= SIZEOF_NIC_IP4_CONFIG_INFO (Old);
      gBS->FreePool (Old);
    }

    if (Config != NULL) {
      TotalLen += SIZEOF_NIC_IP4_CONFIG_INFO (Config);
    }

    //
    // Return NULL if the only NIC_IP4_CONFIG_INFO is being removed.
    //
    if (TotalLen < sizeof (IP4_CONFIG_VARIABLE)) {
      return NULL;
    }

  } else {
    //
    // Variable == NULL and Config != NULL, Create a new variable with
    // this NIC configure.
    //
    Count     = 0;
    Cur       = NULL;
    TotalLen  = sizeof (IP4_CONFIG_VARIABLE) - sizeof (NIC_IP4_CONFIG_INFO)
                + SIZEOF_NIC_IP4_CONFIG_INFO (Config);
  }

  ASSERT (TotalLen >= sizeof (IP4_CONFIG_VARIABLE));

  NewVar = AllocateZeroPool (TotalLen);

  if (NewVar == NULL) {
    return NULL;
  }

  NewVar->Len = TotalLen;

  //
  // Copy the other configure parameters from the old variable
  //
  Next = (UINT8 *)&NewVar->ConfigInfo;

  for (Index = 0; Index < Count; Index++) {
    CopyMem (&Temp, Cur, sizeof (NIC_IP4_CONFIG_INFO));
    Len = SIZEOF_NIC_IP4_CONFIG_INFO (&Temp);

    if (!NIC_ADDR_EQUAL (&Temp.NicAddr, NicAddr)) {
      CopyMem (Next, Cur, Len);
      Next += Len;
      NewVar->Count++;
    }

    Cur += Len;
  }

  //
  // Append the new configure if it isn't NULL.
  //
  Len = 0;

  if (Config != NULL) {
    Len = SIZEOF_NIC_IP4_CONFIG_INFO (Config);

    CopyMem (Next, Config, Len);
    NewVar->Count++;
  }

  ASSERT (Next + Len == (UINT8 *) NewVar + TotalLen);

  NewVar->CheckSum = (UINT16) (~NetblockChecksum ((UINT8 *) NewVar, TotalLen));
  return NewVar;
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

