/** @file
  This code implements the IP4Config and NicIp4Config protocols.

Copyright (c) 2006 - 2008, Intel Corporation.<BR>                                                         
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip4Config.h"

IP4_CONFIG_INSTANCE *mIp4ConfigNicList[MAX_IP4_CONFIG_IN_VARIABLE];

/**
  Return the name and MAC address for the NIC. The Name, if not NULL,
  has at least IP4_NIC_NAME_LENGTH bytes.

  @param  This                   The NIC IP4 CONFIG protocol
  @param  Name                   The buffer to return the name
  @param  NicAddr                The buffer to return the MAC addr

  @retval EFI_INVALID_PARAMETER  This is NULL
  @retval EFI_SUCCESS            The name or address of the NIC are returned.

**/
EFI_STATUS
EFIAPI
EfiNicIp4ConfigGetName (
  IN  EFI_NIC_IP4_CONFIG_PROTOCOL  *This,
  OUT  UINT16                      *Name          OPTIONAL,
  OUT  NIC_ADDR                    *NicAddr       OPTIONAL
  )
{
  IP4_CONFIG_INSTANCE       *Instance;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = IP4_CONFIG_INSTANCE_FROM_NIC_IP4CONFIG (This);

  if (Name != NULL) {
    CopyMem (Name, Instance->NicName, IP4_NIC_NAME_LENGTH);
  }

  if (NicAddr != NULL) {
    CopyMem (NicAddr, &Instance->NicAddr, sizeof (*NicAddr));
  }

  return EFI_SUCCESS;
}


/**
  Get the NIC's configure information from the IP4 configure variable.
  It will remove the invalid variable.

  @param  NicAddr                The NIC to check

  @return NULL if no configure for the NIC in the variable, or it is invalid.
          Otherwise the pointer to the NIC's IP configure parameter will be returned.

**/
NIC_IP4_CONFIG_INFO *
Ip4ConfigGetNicInfo (
  IN  NIC_ADDR              *NicAddr
  )
{
  IP4_CONFIG_VARIABLE       *Variable;
  IP4_CONFIG_VARIABLE       *NewVariable;
  NIC_IP4_CONFIG_INFO       *Config;

  //
  // Read the configuration parameter for this NicAddr from
  // the EFI variable
  //
  Variable = Ip4ConfigReadVariable ();

  if (Variable == NULL) {
    return NULL;
  }

  Config = Ip4ConfigFindNicVariable (Variable, NicAddr);

  if (Config == NULL) {
    gBS->FreePool (Variable);
    return NULL;
  }

  //
  // Validate the configuration, if the configuration is invalid,
  // remove it from the variable.
  //
  if (!Ip4ConfigIsValid (Config)) {
    NewVariable = Ip4ConfigModifyVariable (Variable, &Config->NicAddr, NULL);
    Ip4ConfigWriteVariable (NewVariable);

    if (NewVariable != NULL) {
      gBS->FreePool (NewVariable);
    };

    gBS->FreePool (Config);
    Config = NULL;
  }

  gBS->FreePool (Variable);
  return Config;
}


/**
  Get the configure parameter for this NIC.

  @param  This                   The NIC IP4 CONFIG protocol.
  @param  ConfigLen              The length of the NicConfig buffer.
  @param  NicConfig              The buffer to receive the NIC's configure
                                 parameter.

  @retval EFI_SUCCESS            The configure parameter for this NIC was 
                                 obtained successfully .
  @retval EFI_INVALID_PARAMETER  This or ConfigLen is NULL.
  @retval EFI_NOT_FOUND          There is no configure parameter for the NIC in
                                 NVRam.
  @retval EFI_BUFFER_TOO_SMALL   The ConfigLen is too small or the NicConfig is 
                                 NULL.

**/
EFI_STATUS
EFIAPI
EfiNicIp4ConfigGetInfo (
  IN  EFI_NIC_IP4_CONFIG_PROTOCOL *This,
  IN OUT  UINTN                   *ConfigLen,
  OUT NIC_IP4_CONFIG_INFO         *NicConfig
  )
{
  IP4_CONFIG_INSTANCE *Instance;
  NIC_IP4_CONFIG_INFO *Config;
  EFI_STATUS          Status;
  UINTN               Len;

  if ((This == NULL) || (ConfigLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the Nic's configuration parameter from variable
  //
  Instance  = IP4_CONFIG_INSTANCE_FROM_NIC_IP4CONFIG (This);
  Config    = Ip4ConfigGetNicInfo (&Instance->NicAddr);

  if (Config == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Copy the data to user's buffer
  //
  Len = SIZEOF_NIC_IP4_CONFIG_INFO (Config);

  if ((*ConfigLen < Len) || (NicConfig == NULL)) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    Status = EFI_SUCCESS;
    CopyMem (NicConfig, Config, Len);
    Ip4ConfigFixRouteTablePointer (&NicConfig->Ip4Info);
  }

  *ConfigLen = Len;

  gBS->FreePool (Config);
  return Status;
}


/**
  Set the IP configure parameters for this NIC. 

  If Reconfig is TRUE, the IP driver will be informed to discard current 
  auto configure parameter and restart the auto configuration process. 
  If current there is a pending auto configuration, EFI_ALREADY_STARTED is
  returned. You can only change the configure setting when either
  the configure has finished or not started yet. If NicConfig, the
  NIC's configure parameter is removed from the variable.

  @param  This                   The NIC IP4 CONFIG protocol
  @param  NicConfig              The new NIC IP4 configure parameter
  @param  Reconfig               Inform the IP4 driver to restart the auto
                                 configuration
                                 
  @retval EFI_SUCCESS            The configure parameter for this NIC was 
                                 set successfully .
  @retval EFI_INVALID_PARAMETER  This is NULL or the configure parameter is
                                 invalid.
  @retval EFI_ALREADY_STARTED    There is a pending auto configuration.
  @retval EFI_NOT_FOUND          No auto configure parameter is found

**/
EFI_STATUS
EFIAPI
EfiNicIp4ConfigSetInfo (
  IN EFI_NIC_IP4_CONFIG_PROTOCOL  *This,
  IN NIC_IP4_CONFIG_INFO          *NicConfig     OPTIONAL,
  IN BOOLEAN                      Reconfig
  )
{
  IP4_CONFIG_INSTANCE *Instance;
  IP4_CONFIG_VARIABLE *Variable;
  IP4_CONFIG_VARIABLE *NewVariable;
  EFI_STATUS          Status;

  //
  // Validate the parameters
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = IP4_CONFIG_INSTANCE_FROM_NIC_IP4CONFIG (This);

  if ((NicConfig != NULL) && (!Ip4ConfigIsValid (NicConfig) ||
      !NIC_ADDR_EQUAL (&NicConfig->NicAddr, &Instance->NicAddr))) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instance->State == IP4_CONFIG_STATE_STARTED) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Update the parameter in the configure variable
  //
  Variable = Ip4ConfigReadVariable ();

  if ((Variable == NULL) && (NicConfig == NULL)) {
    return EFI_NOT_FOUND;
  }

  NewVariable = Ip4ConfigModifyVariable (Variable, &Instance->NicAddr, NicConfig);
  Status      = Ip4ConfigWriteVariable (NewVariable);

  if (NewVariable != NULL) {
    gBS->FreePool (NewVariable);
  }

  //
  // Variable is NULL when saving the first configure parameter
  //
  if (Variable != NULL) {
    gBS->FreePool (Variable);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Signal the IP4 to run the auto configuration again
  //
  if (Reconfig && (Instance->ReconfigEvent != NULL)) {
    Status = gBS->SignalEvent (Instance->ReconfigEvent);
    NetLibDispatchDpc ();
  }

  return Status;
}

/**
  Callback function when DHCP process finished. It will save the
  retrieved IP configure parameter from DHCP to the NVRam.

  @param  Event                  The callback event
  @param  Context                Opaque context to the callback

  @return None

**/
VOID
EFIAPI
Ip4ConfigOnDhcp4Complete (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  IP4_CONFIG_INSTANCE       *Instance;
  EFI_DHCP4_MODE_DATA       Dhcp4Mode;
  EFI_IP4_IPCONFIG_DATA     *Ip4Config;
  EFI_STATUS                Status;
  BOOLEAN                   Perment;
  IP4_ADDR                  Subnet;
  IP4_ADDR                  Ip1;
  IP4_ADDR                  Ip2;

  Instance = (IP4_CONFIG_INSTANCE *) Context;
  ASSERT (Instance->Dhcp4 != NULL);

  Instance->State   = IP4_CONFIG_STATE_CONFIGURED;
  Instance->Result  = EFI_TIMEOUT;

  //
  // Get the DHCP retrieved parameters
  //
  Status = Instance->Dhcp4->GetModeData (Instance->Dhcp4, &Dhcp4Mode);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  if (Dhcp4Mode.State == Dhcp4Bound) {
    //
    // Save the new configuration retrieved by DHCP both in
    // the instance and to NVRam. So, both the IP4 driver and
    // other user can get that address.
    //
    Perment = FALSE;

    if (Instance->NicConfig != NULL) {
      ASSERT (Instance->NicConfig->Source == IP4_CONFIG_SOURCE_DHCP);
      Perment = Instance->NicConfig->Perment;
      gBS->FreePool (Instance->NicConfig);
    }

    Instance->NicConfig = AllocatePool (sizeof (NIC_IP4_CONFIG_INFO) + 2* sizeof (EFI_IP4_ROUTE_TABLE));

    if (Instance->NicConfig == NULL) {
      Instance->Result = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    Instance->NicConfig->Ip4Info.RouteTable = (EFI_IP4_ROUTE_TABLE *) (Instance->NicConfig + 1);

    CopyMem (&Instance->NicConfig->NicAddr, &Instance->NicAddr, sizeof (Instance->NicConfig->NicAddr));
    Instance->NicConfig->Source  = IP4_CONFIG_SOURCE_DHCP;
    Instance->NicConfig->Perment = Perment;

    Ip4Config                    = &Instance->NicConfig->Ip4Info;
    Ip4Config->StationAddress    = Dhcp4Mode.ClientAddress;
    Ip4Config->SubnetMask        = Dhcp4Mode.SubnetMask;

    //
    // Create a route for the connected network
    //
    Ip4Config->RouteTableSize    = 1;

    CopyMem (&Ip1, &Dhcp4Mode.ClientAddress, sizeof (IP4_ADDR));
    CopyMem (&Ip2, &Dhcp4Mode.SubnetMask, sizeof (IP4_ADDR));

    Subnet = Ip1 & Ip2;

    CopyMem (&Ip4Config->RouteTable[0].SubnetAddress, &Subnet, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Ip4Config->RouteTable[0].SubnetMask, &Dhcp4Mode.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Ip4Config->RouteTable[0].GatewayAddress, sizeof (EFI_IPv4_ADDRESS));

    //
    // Create a route if there is a default router.
    //
    if (!EFI_IP4_EQUAL (&Dhcp4Mode.RouterAddress, &mZeroIp4Addr)) {
      Ip4Config->RouteTableSize = 2;

      ZeroMem (&Ip4Config->RouteTable[1].SubnetAddress, sizeof (EFI_IPv4_ADDRESS));
      ZeroMem (&Ip4Config->RouteTable[1].SubnetMask, sizeof (EFI_IPv4_ADDRESS));
      CopyMem (&Ip4Config->RouteTable[1].GatewayAddress, &Dhcp4Mode.RouterAddress, sizeof (EFI_IPv4_ADDRESS));
    }

    Instance->Result = EFI_SUCCESS;

    //
    // ignore the return status of EfiNicIp4ConfigSetInfo. Network
    // stack can operate even that failed.
    //
    EfiNicIp4ConfigSetInfo (&Instance->NicIp4Protocol, Instance->NicConfig, FALSE);
  }

ON_EXIT:
  gBS->SignalEvent (Instance->DoneEvent);
  Ip4ConfigCleanDhcp4 (Instance);

  NetLibDispatchDpc ();

  return ;
}

/**
  Starts running the configuration policy for the EFI IPv4 Protocol driver.
  
  The Start() function is called to determine and to begin the platform 
  configuration policy by the EFI IPv4 Protocol driver. This determination may 
  be as simple as returning EFI_UNSUPPORTED if there is no EFI IPv4 Protocol 
  driver configuration policy. It may be as involved as loading some defaults 
  from nonvolatile storage, downloading dynamic data from a DHCP server, and 
  checking permissions with a site policy server.
  Starting the configuration policy is just the beginning. It may finish almost 
  instantly or it may take several minutes before it fails to retrieve configuration 
  information from one or more servers. Once the policy is started, drivers 
  should use the DoneEvent parameter to determine when the configuration policy 
  has completed. EFI_IP4_CONFIG_PROTOCOL.GetData() must then be called to 
  determine if the configuration succeeded or failed.
  Until the configuration completes successfully, EFI IPv4 Protocol driver instances 
  that are attempting to use default configurations must return EFI_NO_MAPPING.
  Once the configuration is complete, the EFI IPv4 Configuration Protocol driver 
  signals DoneEvent. The configuration may need to be updated in the future, 
  however; in this case, the EFI IPv4 Configuration Protocol driver must signal 
  ReconfigEvent, and all EFI IPv4 Protocol driver instances that are using default 
  configurations must return EFI_NO_MAPPING until the configuration policy has 
  been rerun.

  @param  This                   Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.
  @param  DoneEvent              Event that will be signaled when the EFI IPv4 
                                 Protocol driver configuration policy completes 
                                 execution. This event must be of type EVT_NOTIFY_SIGNAL.
  @param  ReconfigEvent          Event that will be signaled when the EFI IPv4 
                                 Protocol driver configuration needs to be updated. 
                                 This event must be of type EVT_NOTIFY_SIGNAL.
  
  @retval EFI_SUCCESS            The configuration policy for the EFI IPv4 Protocol 
                                 driver is now running.
  @retval EFI_INVALID_PARAMETER  One or more of the following parameters is NULL:
                                  This
                                  DoneEvent
                                  ReconfigEvent
  @retval EFI_OUT_OF_RESOURCES   Required system resources could not be allocated.
  @retval EFI_ALREADY_STARTED    The configuration policy for the EFI IPv4 Protocol 
                                 driver was already started.
  @retval EFI_DEVICE_ERROR       An unexpected system error or network error occurred.
  @retval EFI_UNSUPPORTED        This interface does not support the EFI IPv4 Protocol 
                                 driver configuration.

**/
EFI_STATUS
EFIAPI
EfiIp4ConfigStart (
  IN EFI_IP4_CONFIG_PROTOCOL  *This,
  IN EFI_EVENT                DoneEvent,
  IN EFI_EVENT                ReconfigEvent
  )
{
  IP4_CONFIG_INSTANCE       *Instance;
  EFI_DHCP4_PROTOCOL        *Dhcp4;
  EFI_DHCP4_MODE_DATA       Dhcp4Mode;
  EFI_DHCP4_PACKET_OPTION   *OptionList[1];
  IP4_CONFIG_DHCP4_OPTION   ParaList;
  EFI_STATUS                Status;
  UINT32                    Source;
  EFI_TPL                   OldTpl;

  if ((This == NULL) || (DoneEvent == NULL) || (ReconfigEvent == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = IP4_CONFIG_INSTANCE_FROM_IP4CONFIG (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Instance->State != IP4_CONFIG_STATE_IDLE) {
    Status = EFI_ALREADY_STARTED;

    goto ON_EXIT;
  }

  Instance->DoneEvent     = DoneEvent;
  Instance->ReconfigEvent = ReconfigEvent;

  Instance->NicConfig     = Ip4ConfigGetNicInfo (&Instance->NicAddr);

  if (Instance->NicConfig == NULL) {
    Source = IP4_CONFIG_SOURCE_DHCP;
  } else {
    Source = Instance->NicConfig->Source;
  }

  //
  // If the source is static, the auto configuration is done.
  // return now.
  //
  if (Source == IP4_CONFIG_SOURCE_STATIC) {
    Instance->State  = IP4_CONFIG_STATE_CONFIGURED;
    Instance->Result = EFI_SUCCESS;

    gBS->SignalEvent (Instance->DoneEvent);
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Start the dhcp process
  //
  ASSERT ((Source == IP4_CONFIG_SOURCE_DHCP) && (Instance->Dhcp4 == NULL));

  Status = NetLibCreateServiceChild (
             Instance->Controller,
             Instance->Image,
             &gEfiDhcp4ServiceBindingProtocolGuid,
             &Instance->Dhcp4Handle
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Instance->Dhcp4Handle,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **) &Instance->Dhcp4,
                  Instance->Image,
                  Instance->Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Check the current DHCP status, if the DHCP process has
  // already finished, return now.
  //
  Dhcp4  = Instance->Dhcp4;
  Status = Dhcp4->GetModeData (Dhcp4, &Dhcp4Mode);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (Dhcp4Mode.State == Dhcp4Bound) {
    Ip4ConfigOnDhcp4Complete (NULL, Instance);

    goto ON_EXIT;
  }

  //
  // Try to start the DHCP process. Use most of the current
  // DHCP configuration to avoid problems if some DHCP client
  // yields the control of this DHCP service to us.
  //
  ParaList.Head.OpCode             = DHCP_TAG_PARA_LIST;
  ParaList.Head.Length             = 2;
  ParaList.Head.Data[0]            = DHCP_TAG_NETMASK;
  ParaList.Route                   = DHCP_TAG_ROUTER;
  OptionList[0]                    = &ParaList.Head;
  Dhcp4Mode.ConfigData.OptionCount = 1;
  Dhcp4Mode.ConfigData.OptionList  = OptionList;

  Status = Dhcp4->Configure (Dhcp4, &Dhcp4Mode.ConfigData);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Start the DHCP process
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ip4ConfigOnDhcp4Complete,
                  Instance,
                  &Instance->Dhcp4Event
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Dhcp4->Start (Dhcp4, Instance->Dhcp4Event);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Instance->State  = IP4_CONFIG_STATE_STARTED;
  Instance->Result = EFI_NOT_READY;

ON_ERROR:
  if (EFI_ERROR (Status)) {
    Ip4ConfigCleanConfig (Instance);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  NetLibDispatchDpc ();

  return Status;
}


/**
  Stops running the configuration policy for the EFI IPv4 Protocol driver.
  
  The Stop() function stops the configuration policy for the EFI IPv4 Protocol driver. 
  All configuration data will be lost after calling Stop().

  @param  This                   Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.

  @retval EFI_SUCCESS            The configuration policy for the EFI IPv4 Protocol 
                                 driver has been stopped.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        The configuration policy for the EFI IPv4 Protocol 
                                 driver was not started.
  
**/
EFI_STATUS
EFIAPI
EfiIp4ConfigStop (
  IN EFI_IP4_CONFIG_PROTOCOL  *This
  )
{
  IP4_CONFIG_INSTANCE  *Instance;
  EFI_STATUS           Status;
  EFI_TPL              OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = IP4_CONFIG_INSTANCE_FROM_IP4CONFIG (This);

  Status = EFI_SUCCESS;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Instance->State == IP4_CONFIG_STATE_IDLE) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  //
  // Release all the configure parameters. Don't signal the user
  // event. The user wants to abort the configuration, this isn't
  // the configuration done or reconfiguration.
  //
  Ip4ConfigCleanConfig (Instance);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Returns the default configuration data (if any) for the EFI IPv4 Protocol driver.

  The GetData() function returns the current configuration data for the EFI IPv4 
  Protocol driver after the configuration policy has completed.
  
  @param  This                   Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.
  @param  ConfigDataSize         On input, the size of the ConfigData buffer. 
                                 On output, the count of bytes that were written 
                                 into the ConfigData buffer.
  @param  ConfigData             Pointer to the EFI IPv4 Configuration Protocol 
                                 driver configuration data structure. 
                                 Type EFI_IP4_IPCONFIG_DATA is defined in 
                                 "Related Definitions" below.

  @retval EFI_SUCCESS            The EFI IPv4 Protocol driver configuration has been returned.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        The configuration policy for the EFI IPv4 Protocol 
                                 driver is not running.
  @retval EFI_NOT_READY          EFI IPv4 Protocol driver configuration is still running.
  @retval EFI_ABORTED            EFI IPv4 Protocol driver configuration could not complete.
                                 Currently not implemented.
  @retval EFI_BUFFER_TOO_SMALL   *ConfigDataSize is smaller than the configuration 
                                 data buffer or ConfigData is NULL.

**/
EFI_STATUS
EFIAPI
EfiIp4ConfigGetData (
  IN  EFI_IP4_CONFIG_PROTOCOL *This,
  IN  OUT  UINTN              *ConfigDataSize,
  OUT EFI_IP4_IPCONFIG_DATA   *ConfigData           OPTIONAL
  )
{
  IP4_CONFIG_INSTANCE       *Instance;
  NIC_IP4_CONFIG_INFO       *NicConfig;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  UINTN                     Len;

  if ((This == NULL) || (ConfigDataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance  = IP4_CONFIG_INSTANCE_FROM_IP4CONFIG (This);

  Status = EFI_SUCCESS;
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Instance->State == IP4_CONFIG_STATE_IDLE) {
    Status = EFI_NOT_STARTED;
  } else if (Instance->State == IP4_CONFIG_STATE_STARTED) {
    Status = EFI_NOT_READY;
  }

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Copy the configure data if auto configuration succeeds.
  //
  Status = Instance->Result;

  if (Status == EFI_SUCCESS) {
    ASSERT (Instance->NicConfig != NULL);

    NicConfig = Instance->NicConfig;
    Len       = SIZEOF_IP4_CONFIG_INFO (&NicConfig->Ip4Info);

    if ((*ConfigDataSize < Len) || (ConfigData == NULL)) {
      Status = EFI_BUFFER_TOO_SMALL;
    } else {
      CopyMem (ConfigData, &NicConfig->Ip4Info, Len);
      Ip4ConfigFixRouteTablePointer (ConfigData);
    }

    *ConfigDataSize = Len;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Release all the DHCP related resources.

  @param  This                   The IP4 configure instance

  @return None

**/
VOID
Ip4ConfigCleanDhcp4 (
  IN IP4_CONFIG_INSTANCE    *This
  )
{
  if (This->Dhcp4 != NULL) {
    This->Dhcp4->Stop (This->Dhcp4);

    gBS->CloseProtocol (
          This->Dhcp4Handle,
          &gEfiDhcp4ProtocolGuid,
          This->Image,
          This->Controller
          );

    This->Dhcp4 = NULL;
  }

  if (This->Dhcp4Handle != NULL) {
    NetLibDestroyServiceChild (
      This->Controller,
      This->Image,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      This->Dhcp4Handle
      );

    This->Dhcp4Handle = NULL;
  }

  if (This->Dhcp4Event == NULL) {
    gBS->CloseEvent (This->Dhcp4Event);
    This->Dhcp4Event = NULL;
  }
}


/**
  Clean up all the configuration parameters.

  @param  Instance               The IP4 configure instance

  @return None

**/
VOID
Ip4ConfigCleanConfig (
  IN IP4_CONFIG_INSTANCE        *Instance
  )
{
  if (Instance->NicConfig != NULL) {
    gBS->FreePool (Instance->NicConfig);
    Instance->NicConfig = NULL;
  }

  Instance->State         = IP4_CONFIG_STATE_IDLE;
  Instance->DoneEvent     = NULL;
  Instance->ReconfigEvent = NULL;

  Ip4ConfigCleanDhcp4 (Instance);
}

EFI_IP4_CONFIG_PROTOCOL     mIp4ConfigProtocolTemplate = {
  EfiIp4ConfigStart,
  EfiIp4ConfigStop,
  EfiIp4ConfigGetData
};

EFI_NIC_IP4_CONFIG_PROTOCOL mNicIp4ConfigProtocolTemplate = {
  EfiNicIp4ConfigGetName,
  EfiNicIp4ConfigGetInfo,
  EfiNicIp4ConfigSetInfo
};
