/** @file
  The implementation for ifcommand shell command.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "UefiShellNetwork1CommandsLib.h"

#define NIC_ITEM_CONFIG_SIZE   (sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * MAX_IP4_CONFIG_IN_VARIABLE)
#define EFI_IP4_TO_U32(EfiIpAddr)   (*(IP4_ADDR*)((EfiIpAddr).Addr))

BOOLEAN                                 mIp4ConfigExist    = FALSE;
STATIC EFI_HII_CONFIG_ROUTING_PROTOCOL  *mHiiConfigRouting = NULL;

STATIC CONST UINTN SecondsToNanoSeconds = 10000000;
STATIC CONST CHAR16 DhcpString[5]       = L"DHCP";
STATIC CONST CHAR16 StaticString[7]     = L"STATIC";
STATIC CONST CHAR16 PermanentString[10] = L"PERMANENT";

typedef struct {
  LIST_ENTRY                  Link;
  EFI_HANDLE                  Handle;
  NIC_ADDR                    NicAddress;
  CHAR16                      Name[IP4_NIC_NAME_LENGTH];
  BOOLEAN                     MediaPresentSupported;
  BOOLEAN                     MediaPresent;
  EFI_IP4_CONFIG_PROTOCOL     *Ip4Config;
  NIC_IP4_CONFIG_INFO         *ConfigInfo;
} NIC_INFO;

typedef struct {
  EFI_IP_ADDRESS              DestIp;
  EFI_MAC_ADDRESS             DestMac;
  EFI_IP_ADDRESS              LocalIp;
  EFI_MAC_ADDRESS             LocalMac;
  UINT8                       MacLen;
  EFI_EVENT                   OnResolved;
  BOOLEAN                     Duplicate;
} ARP_REQUEST;

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-c",     TypeValue},
  {L"-l",     TypeValue},
  {L"-s",     TypeMaxValue},
  {NULL,      TypeMax}
  };

STATIC LIST_ENTRY                  NicInfoList;
STATIC BOOLEAN                     ArpResolved;
STATIC BOOLEAN                     mTimeout;

/**
  Count the space delimited items in a string.

  @param[in] String     A pointer to the string to count.

  @return The number of space-delimited items.
  @retval 0xFF an error occured.
**/
UINT8
EFIAPI
CountSubItems (
  IN CONST CHAR16 *String
  )
{
  CONST CHAR16  *Walker;
  UINT8         Count;

  if (String == NULL || *String == CHAR_NULL) {
    return (0xFF);
  }

  for (Walker = String, Count = 0 ; Walker != NULL && *Walker != CHAR_NULL ; Walker = (StrStr(Walker, L" ")==NULL?NULL:StrStr(Walker, L" ")+1), Count++);
  return (Count);
}

/**
  Find the NIC_INFO by the specified nic name.

  @param[in] Name     The pointer to the string containing the NIC name.
  
  @return The pointer to the NIC_INFO if there is a NIC_INFO named by Name.
  @retval NULL  No NIC_INFO was found for Name.
**/
NIC_INFO*
EFIAPI
IfconfigFindNicByName (
  IN CONST CHAR16           *Name
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *NextEntry;
  NIC_INFO                  *Info;
  CHAR16                    *TempString;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &NicInfoList) {
    Info = BASE_CR (Entry, NIC_INFO, Link);
    TempString = (CHAR16*)Info->Name;

    if (StringNoCaseCompare (&Name, &TempString) == 0) {
      return Info;
    }
  }

  return NULL;
}

/**
  Tests whether a child handle is a child device of the controller.

  @param[in] ControllerHandle   A handle for a (parent) controller to test.
  @param[in] ChildHandle        A child handle to test.
  @param[in] ProtocolGuid       Supplies the protocol that the child controller
                                opens on its parent controller.

  @retval EFI_SUCCESS         ChildHandle is a child of the ControllerHandle.
  @retval EFI_UNSUPPORTED     ChildHandle is not a child of the ControllerHandle.
**/
EFI_STATUS
EFIAPI
TestChildHandle (
  IN CONST EFI_HANDLE       ControllerHandle,
  IN CONST EFI_HANDLE       ChildHandle,
  IN CONST EFI_GUID         *ProtocolGuid
  )
{
  EFI_STATUS                            Status;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *OpenInfoBuffer;
  UINTN                                 EntryCount;
  UINTN                                 Index;

  ASSERT (ProtocolGuid != NULL);

  //
  // Retrieve the list of agents that are consuming the specific protocol
  // on ControllerHandle.
  //
  Status = gBS->OpenProtocolInformation (
                 ControllerHandle,
                 (EFI_GUID *) ProtocolGuid,
                 &OpenInfoBuffer,
                 &EntryCount
                 );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Inspect if ChildHandle is one of the agents.
  //
  Status = EFI_UNSUPPORTED;
  for (Index = 0; Index < EntryCount; Index++) {
    if ((OpenInfoBuffer[Index].ControllerHandle == ChildHandle) &&
        (OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      Status = EFI_SUCCESS;
      break;
    }
  }

  FreePool (OpenInfoBuffer);
  return Status;
}

/**
  Get the child handle of the NIC handle.

  @param[in] Controller     Routing information: GUID.
  @param[out] ChildHandle   Returned child handle.

  @retval EFI_SUCCESS         Successfully to get child handle.
**/
EFI_STATUS 
GetChildHandle (
  IN EFI_HANDLE         Controller,
  OUT EFI_HANDLE        *ChildHandle
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 *Handles;
  UINTN                      HandleCount;
  UINTN                      Index;
  EFI_DEVICE_PATH_PROTOCOL   *ChildDeviceDevicePath;
  VENDOR_DEVICE_PATH         *VendorDeviceNode;

  //
  // Locate all EFI Hii Config Access protocols
  //
  Status = gBS->LocateHandleBuffer (
                 ByProtocol,
                 &gEfiHiiConfigAccessProtocolGuid,
                 NULL,
                 &HandleCount,
                 &Handles
                 );
  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < HandleCount; Index++) {
  
    Status = TestChildHandle (Controller, Handles[Index], &gEfiManagedNetworkServiceBindingProtocolGuid);
    if (!EFI_ERROR (Status)) {
      //
      // Get device path on the child handle
      //
      Status = gBS->HandleProtocol (
                     Handles[Index],
                     &gEfiDevicePathProtocolGuid,
                     (VOID **) &ChildDeviceDevicePath
                     );
      
      if (!EFI_ERROR (Status)) {
        while (!IsDevicePathEnd (ChildDeviceDevicePath)) {
          ChildDeviceDevicePath = NextDevicePathNode (ChildDeviceDevicePath);
          //
          // Parse one instance
          //
          if (ChildDeviceDevicePath->Type == HARDWARE_DEVICE_PATH && 
              ChildDeviceDevicePath->SubType == HW_VENDOR_DP) {
            VendorDeviceNode = (VENDOR_DEVICE_PATH *) ChildDeviceDevicePath;
            if (CompareMem (&VendorDeviceNode->Guid, &gEfiNicIp4ConfigVariableGuid, sizeof (EFI_GUID)) == 0) {
              //
              // Found item matched gEfiNicIp4ConfigVariableGuid
              //
              *ChildHandle = Handles[Index];
              FreePool (Handles);
              return EFI_SUCCESS;
            }
          }
        }
      }      
    }
  }

  FreePool (Handles);
  return Status;  
}

/**
  Append OFFSET/WIDTH/VALUE items at the beginning of string.

  @param[in, out]  String      The pointer to the string to append onto.
  @param[in]       MaxLen      The max number of UNICODE char in String
                               including the terminate NULL char.
  @param[in]       Offset      Offset value.
  @param[in]       Width       Width value.
  @param[in]       Block       Point to data buffer.

  @return The count of unicode character that were appended.
**/
UINTN
EFIAPI
AppendOffsetWidthValue (
  IN OUT CHAR16               *String,
  IN UINTN                    MaxLen,
  IN UINTN                    Offset,
  IN UINTN                    Width,
  IN CONST UINT8              *Block
  )

{
  CHAR16                      *OriString;

  OriString = String;

  StrnCpyS (String, MaxLen, L"&OFFSET=", 9);
  String += StrLen (L"&OFFSET=");
  String += UnicodeSPrint (String, 20, L"%x", Offset);

  StrnCpyS (String, MaxLen, L"&WIDTH=", 8);
  String += StrLen (L"&WIDTH=");
  String += UnicodeSPrint (String, 20, L"%x", Width);

  if (Block != NULL) {
    StrnCpyS (String, MaxLen, L"&VALUE=", 8);
    String += StrLen (L"&VALUE=");
    while ((Width--) != 0) {
      String += UnicodeSPrint (String, 20, L"%x", Block[Width]);
    }
  }
  
  return String - OriString;
}

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param ConfigString  String to be converted
**/
CHAR16* 
EFIAPI
HiiToLower (
  IN CHAR16   *ConfigString
  )
{
  CHAR16      *String;
  BOOLEAN     Lower;

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; String != NULL && *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && *String >= L'A' && *String <= L'F') {
      *String = (CHAR16) (*String - L'A' + L'a');
    }
  }

  return (ConfigString);
}


/**
  Construct <ConfigHdr> using routing information GUID/NAME/PATH.

  @param[in] Guid         Routing information: GUID.
  @param[in] Name         Routing information: NAME.
  @param[in] DriverHandle Driver handle which contains the routing information: PATH.

  @retval NULL            An error occured.
  @return                 The pointer to configHdr string.
**/
CHAR16 *
EFIAPI
ConstructConfigHdr (
  IN CONST EFI_GUID          *Guid,
  IN CONST CHAR16            *Name,
  IN EFI_HANDLE              DriverHandle
  )
{
  EFI_STATUS                 Status;
  CHAR16                     *ConfigHdr;
  UINTN                      ConfigHdrBufferSize;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  CHAR16                     *String;
  UINTN                      Index;
  UINT8                      *Buffer;
  UINTN                      DevicePathLength;
  UINTN                      NameLength;

  //
  // Get the device path from handle installed EFI HII Config Access protocol
  //
  Status = gBS->HandleProtocol (
                 DriverHandle,
                 &gEfiDevicePathProtocolGuid,
                 (VOID **) &DevicePath
                 );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DevicePathLength = GetDevicePathSize (DevicePath);
  NameLength = StrLen (Name);
  ConfigHdrBufferSize = (5 + sizeof (EFI_GUID) * 2 + 6 + NameLength * 4 + 6 + DevicePathLength * 2 + 1) * sizeof (CHAR16);
  ConfigHdr = AllocateZeroPool (ConfigHdrBufferSize);
  if (ConfigHdr == NULL) {
    return NULL;
  } 

  String = ConfigHdr;
  StrnCpyS (String, ConfigHdrBufferSize/sizeof(CHAR16), L"GUID=", 6);
  String += StrLen (L"GUID=");

  //
  // Append Guid converted to <HexCh>32
  //
  for (Index = 0, Buffer = (UINT8 *)Guid; Index < sizeof (EFI_GUID); Index++) {
    String += UnicodeSPrint (String, 6, L"%02x", *Buffer++);
  }

  //
  // Append L"&NAME="
  //
  StrnCpyS (String, ConfigHdrBufferSize/sizeof(CHAR16), L"&NAME=", 7);
  String += StrLen (L"&NAME=");
  for (Index = 0; Index < NameLength ; Index++) {
    String += UnicodeSPrint (String, 10, L"00%x", Name[Index]);
  }
  
  //
  // Append L"&PATH="
  //
  StrnCpyS (String, ConfigHdrBufferSize/sizeof(CHAR16), L"&PATH=", 7);
  String += StrLen (L"&PATH=");
  for (Index = 0, Buffer = (UINT8 *) DevicePath; Index < DevicePathLength; Index++) {
    String += UnicodeSPrint (String, 6, L"%02x", *Buffer++);
  }

  return (HiiToLower(ConfigHdr));
}

/**
  Get network physical device NIC information.

  @param[in] Handle         The network physical device handle.
  @param[out] NicAddr       NIC information.

  @retval EFI_SUCCESS         Get NIC information successfully.
**/                  
EFI_STATUS
EFIAPI
IfConfigGetNicMacInfo (
  IN  EFI_HANDLE                    Handle,
  OUT NIC_ADDR                      *NicAddr
  )    
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    MnpHandle;
  EFI_SIMPLE_NETWORK_MODE       SnpMode;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  MnpHandle = NULL;
  Mnp       = NULL;

  Status = NetLibCreateServiceChild (
             Handle,
             gImageHandle, 
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &MnpHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  MnpHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &Mnp
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Mnp->GetModeData (Mnp, NULL, &SnpMode);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    goto ON_ERROR;
  }
 
  NicAddr->Type    = (UINT16) SnpMode.IfType;
  NicAddr->Len     = (UINT8) SnpMode.HwAddressSize;
  CopyMem (&NicAddr->MacAddr, &SnpMode.CurrentAddress, NicAddr->Len);

ON_ERROR:

  NetLibDestroyServiceChild (
    Handle,
    gImageHandle, 
    &gEfiManagedNetworkServiceBindingProtocolGuid,
    MnpHandle
    );

  return Status;

}

/**
  Get network physical device NIC information.

  @param[in] Handle         The network physical device handle.
  @param[out] MediaPresentSupported
                            Upon successful return, TRUE is media present 
                            is supported.  FALSE otherwise.
  @param[out] MediaPresent  Upon successful return, TRUE is media present 
                            is enabled.  FALSE otherwise.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
EFIAPI
IfConfigGetNicMediaStatus (
  IN  EFI_HANDLE                    Handle,
  OUT BOOLEAN                       *MediaPresentSupported,
  OUT BOOLEAN                       *MediaPresent
  )    
                  
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    MnpHandle;
  EFI_SIMPLE_NETWORK_MODE       SnpMode;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  MnpHandle = NULL;
  Mnp       = NULL;

  Status = NetLibCreateServiceChild (
             Handle,
             gImageHandle, 
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &MnpHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  MnpHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &Mnp
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Mnp->GetModeData (Mnp, NULL, &SnpMode);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    goto ON_ERROR;
  }
 
  *MediaPresentSupported = SnpMode.MediaPresentSupported;
  *MediaPresent = SnpMode.MediaPresent;

ON_ERROR:

  NetLibDestroyServiceChild (
    Handle,
    gImageHandle, 
    &gEfiManagedNetworkServiceBindingProtocolGuid,
    MnpHandle
    );

  return Status;

}

/**
  Get all Nic's information through HII service.

  @retval EFI_SUCCESS         All the nic information is collected.
**/
EFI_STATUS
EFIAPI
IfconfigGetAllNicInfoByHii (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *Handles;
  UINTN                         HandleCount;
  CHAR16                        *ConfigResp;
  UINTN                         ConfigRespBufferSize;
  CHAR16                        *ConfigHdr;
  UINTN                         Index;
  CHAR16                        *AccessProgress;
  CHAR16                        *AccessResults;
  UINTN                         BufferSize;
  NIC_INFO                      *NicInfo;
  NIC_IP4_CONFIG_INFO           *NicConfigRequest;
  NIC_IP4_CONFIG_INFO           *NicConfig;
  CHAR16                        *String;
  UINTN                         Length;
  UINTN                         Offset;
  EFI_HANDLE                    ChildHandle;

  AccessResults    = NULL;
  ConfigHdr        = NULL;
  ConfigResp       = NULL;
  NicConfigRequest = NULL;
  NicInfo          = NULL;

  InitializeListHead (&NicInfoList);

  //
  // Check if HII Config Routing protocol available.
  //
  Status = gBS->LocateProtocol (
                &gEfiHiiConfigRoutingProtocolGuid,
                NULL,
                (VOID**)&mHiiConfigRouting
                );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Locate all network device handles
  //
  Status = gBS->LocateHandleBuffer (
                 ByProtocol,
                 &gEfiManagedNetworkServiceBindingProtocolGuid,
                 NULL,
                 &HandleCount,
                 &Handles
                 );
  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = GetChildHandle (Handles[Index], &ChildHandle);
    if (EFI_ERROR (Status)) {
      //
      // If failed to get Child handle, try NIC controller handle for back-compatibility.
      //
      ChildHandle = Handles[Index];
    }
    //
    // Construct configuration request string header
    //
    ConfigHdr = ConstructConfigHdr (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, ChildHandle);
    if (ConfigHdr != NULL) {
      Length = StrLen (ConfigHdr);
    } else {
      Length = 0;
    }
    ConfigRespBufferSize = (Length + NIC_ITEM_CONFIG_SIZE * 2 + 100) * sizeof (CHAR16);
    ConfigResp = AllocateZeroPool (ConfigRespBufferSize);
    if (ConfigResp == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }
    if (ConfigHdr != NULL) {
      StrnCpyS (ConfigResp, ConfigRespBufferSize/sizeof(CHAR16), ConfigHdr, Length + NIC_ITEM_CONFIG_SIZE * 2 + 100 - 1);
    }
 
    //
    // Append OFFSET/WIDTH pair
    //
    String = ConfigResp + Length;
    Offset = 0;
    AppendOffsetWidthValue (String, 
                            ConfigRespBufferSize/sizeof(CHAR16) - Length, 
                            Offset, 
                            NIC_ITEM_CONFIG_SIZE, 
                            NULL
                            );

    NicInfo = AllocateZeroPool (sizeof (NIC_INFO));
    if (NicInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }
    NicInfo->Handle       = Handles[Index];

    //
    // Get network physical devcie MAC information
    //
    IfConfigGetNicMacInfo (Handles[Index], &NicInfo->NicAddress);
    if (NicInfo->NicAddress.Type == NET_IFTYPE_ETHERNET) {
      UnicodeSPrint (NicInfo->Name, IP4_NIC_NAME_LENGTH, L"eth%d", Index);
    } else {
      UnicodeSPrint (NicInfo->Name, IP4_NIC_NAME_LENGTH, L"unk%d", Index);
    }

    //
    // Get media status
    //
    IfConfigGetNicMediaStatus (Handles[Index], &NicInfo->MediaPresentSupported, &NicInfo->MediaPresent);

    NicConfigRequest = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
    if (NicConfigRequest == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    //
    // Get network parameters by HII service
    //
    Status = mHiiConfigRouting->ExtractConfig (
                                  mHiiConfigRouting,
                                  ConfigResp,
                                  &AccessProgress,
                                  &AccessResults
                                  );
    if (!EFI_ERROR (Status)) {
      BufferSize = NIC_ITEM_CONFIG_SIZE;
      Status = mHiiConfigRouting->ConfigToBlock (
                                    mHiiConfigRouting,
                                    AccessResults,
                                    (UINT8 *) NicConfigRequest,
                                    &BufferSize,
                                    &AccessProgress
                                    );
      if (!EFI_ERROR (Status)) {
        BufferSize = sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * NicConfigRequest->Ip4Info.RouteTableSize;
        NicConfig = AllocateZeroPool (BufferSize);
        if (NicConfig == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto ON_ERROR;
        }
        CopyMem (NicConfig, NicConfigRequest, BufferSize);

        //
        // If succeeds to get NIC configuration, fix up routetable pointer.
        //
        NicConfig->Ip4Info.RouteTable = (EFI_IP4_ROUTE_TABLE *) (&NicConfig->Ip4Info + 1);
        NicInfo->ConfigInfo   = NicConfig;

      } else {
        NicInfo->ConfigInfo   = NULL;
      }

      FreePool (AccessResults);

    } else {
      NicInfo->ConfigInfo   = NULL;
    }

    //
    // Add the Nic's info to the global NicInfoList.
    //
    InsertTailList (&NicInfoList, &NicInfo->Link);

    FreePool (NicConfigRequest);
    FreePool (ConfigResp);
    FreePool (ConfigHdr);
  }

  FreePool (Handles);

  return EFI_SUCCESS;
 
ON_ERROR:
  if (AccessResults != NULL) {
    FreePool (AccessResults);
  }
  if (NicConfigRequest != NULL) {
    FreePool (NicConfigRequest);
  }
  if (NicInfo != NULL) {
    FreePool (NicInfo);
  }
  if (ConfigResp != NULL) {
    FreePool (ConfigResp);
  }
  if (ConfigHdr != NULL) {
    FreePool (ConfigHdr);
  }

  FreePool (Handles);

  return Status;
}

/**
  Set the address for the specified nic by HII service.

  @param[in] NicInfo    A pointer to the NIC_INFO of the Nic to be configured.
  @param[in] Config     The command line arguments for the set operation.

  @retval EFI_SUCCESS         The address set operation is done.
**/
SHELL_STATUS
EFIAPI
IfconfigSetNicAddrByHii (
  IN CONST NIC_INFO                 *NicInfo,
  IN CONST NIC_IP4_CONFIG_INFO      *Config
  )
{
  EFI_STATUS                    Status;
  SHELL_STATUS                  ShellStatus;
  NIC_IP4_CONFIG_INFO           *NicConfig;
  CHAR16                        *ConfigResp;
  UINTN                         ConfigRespBufferSize;
  CHAR16                        *ConfigHdr;
  CHAR16                        *AccessProgress;
  CHAR16                        *AccessResults;
  CHAR16                        *String;
  UINTN                         Length;
  UINTN                         Offset;
  EFI_HANDLE                    ChildHandle;

  AccessResults  = NULL;
  ConfigHdr      = NULL;
  ConfigResp     = NULL;
  NicConfig      = NULL;
  ShellStatus    = SHELL_SUCCESS;

  Status = GetChildHandle (NicInfo->Handle, &ChildHandle);
  if (EFI_ERROR (Status)) {
    //
    // If failed to get Child handle, try NIC controller handle for back-compatibility
    //
    ChildHandle = NicInfo->Handle;
  }
  //
  // Construct config request string header
  //
  ConfigHdr = ConstructConfigHdr (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, ChildHandle);
  if (ConfigHdr != NULL) {
    Length = StrLen (ConfigHdr);
  } else {
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  ConfigRespBufferSize = (Length + NIC_ITEM_CONFIG_SIZE * 2 + 100) * sizeof (CHAR16);
  ConfigResp = AllocateZeroPool (ConfigRespBufferSize);
  if (ConfigResp == NULL) {
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  if (ConfigHdr != NULL) {
    StrnCpyS (ConfigResp, ConfigRespBufferSize/sizeof(CHAR16), ConfigHdr, Length + NIC_ITEM_CONFIG_SIZE * 2 + 100 - 1);
  }

  NicConfig = AllocateZeroPool (NIC_ITEM_CONFIG_SIZE);
  if (NicConfig == NULL) {
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  if (Config != NULL) {
    CopyMem (NicConfig, Config, sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * Config->Ip4Info.RouteTableSize);
  }

  //
  // Append OFFSET/WIDTH pair
  //
  String = ConfigResp + Length;
  Offset = 0;
  AppendOffsetWidthValue (String, 
                          ConfigRespBufferSize/sizeof(CHAR16) - Length,
                          Offset, 
                          NIC_ITEM_CONFIG_SIZE, 
                          NULL
                          );

  //
  // Call HII helper function to generate configuration string
  //
  Status = mHiiConfigRouting->BlockToConfig (
                                mHiiConfigRouting,
                                ConfigResp,
                                (UINT8 *) NicConfig,
                                NIC_ITEM_CONFIG_SIZE,
                                &AccessResults,
                                &AccessProgress
                                );
  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_NOT_FOUND;
    goto ON_EXIT;
  }

  //
  // Set IP setting by HII servie
  //
  Status = mHiiConfigRouting->RouteConfig (
                                mHiiConfigRouting,
                                AccessResults,
                                &AccessProgress
                                );
  if (EFI_ERROR(Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
  }

ON_EXIT:
  SHELL_FREE_NON_NULL(AccessResults);
  SHELL_FREE_NON_NULL(NicConfig);
  SHELL_FREE_NON_NULL(ConfigResp);
  SHELL_FREE_NON_NULL(ConfigHdr);

  return ShellStatus;
}

/**
  The callback function for the Arp address resolved event.

  @param[in] Event    The event this function is registered to.
  @param[in] Context  The context registered to the event.
**/
VOID
EFIAPI
IfconfigOnArpResolved (
  IN EFI_EVENT                  Event,
  IN VOID                       *Context
  )
{
  ARP_REQUEST                   *Request;
  UINT8                         Index;

  Request = (ARP_REQUEST *) Context;
  ASSERT (Request != NULL);

  Request->Duplicate = FALSE;
  
  if (0 == CompareMem (&Request->LocalMac, &Request->DestMac, Request->MacLen)) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL,
      STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
      gShellNetwork1HiiHandle, 
      L"Already Configured",
      (UINTN)Request->DestIp.v4.Addr[0],
      (UINTN)Request->DestIp.v4.Addr[1],
      (UINTN)Request->DestIp.v4.Addr[2],
      (UINTN)Request->DestIp.v4.Addr[3]
      );
    ArpResolved = TRUE;
    return;
  }
  
  for (Index = 0; Index < Request->MacLen; Index++) {
    if (Request->DestMac.Addr[Index] != 0) {
      Request->Duplicate = TRUE;
    }
  }

  if (Request->Duplicate) {
    ShellPrintHiiEx(
    -1,
    -1,
    NULL,
    STRING_TOKEN(STR_IFCONFIG_CONF_IP_ADDR), 
    gShellNetwork1HiiHandle, 
    (UINTN)Request->DestMac.Addr[0], 
    (UINTN)Request->DestMac.Addr[1], 
    (UINTN)Request->DestMac.Addr[2],
    (UINTN)Request->DestMac.Addr[3], 
    (UINTN)Request->DestMac.Addr[4], 
    (UINTN)Request->DestMac.Addr[5]
    );    
  }

  ArpResolved = TRUE;
  return ;
}

/**
  Check whether the address to be configured conflicts with other hosts.

  @param[in] NicInfo    The pointer to the NIC_INFO of the Nic to be configured.
  @param[in] IpAddr     The IPv4 address to be configured to the Nic.

  @return TRUE      Some other host already uses the IpAddr.
  @return FALSE     The address is unused.
**/
BOOLEAN
EFIAPI
IfconfigIsIpDuplicate (
  IN  NIC_INFO                  *NicInfo,
  IN  IP4_ADDR                  IpAddr
  )
{
  EFI_ARP_PROTOCOL              *Arp;
  EFI_ARP_CONFIG_DATA           ArpCfgData;
  EFI_HANDLE                    ArpHandle;
  ARP_REQUEST                   Request;
  EFI_STATUS                    Status;

  Arp           = NULL;
  ArpHandle     = NULL;
  ZeroMem (&Request, sizeof (ARP_REQUEST));

  Status = NetLibCreateServiceChild (
             NicInfo->Handle,
             gImageHandle, 
             &gEfiArpServiceBindingProtocolGuid,
             &ArpHandle
             );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = gBS->OpenProtocol (
                 ArpHandle,
                 &gEfiArpProtocolGuid,
                 (VOID**)&Arp,
                 gImageHandle,
                 ArpHandle,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Set up the Arp requests
  //
  EFI_IP4_TO_U32 (Request.DestIp.v4)  = IpAddr;
  EFI_IP4_TO_U32 (Request.LocalIp.v4) = 0xffffffff;
  Request.LocalMac                    = NicInfo->NicAddress.MacAddr;
  Request.MacLen                      = NicInfo->NicAddress.Len;
  
  Status = gBS->CreateEvent (
                 EVT_NOTIFY_SIGNAL,
                 TPL_CALLBACK,
                 IfconfigOnArpResolved,
                 (VOID *) &Request,
                 &Request.OnResolved
                 );
  
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  
  ArpCfgData.SwAddressType    = 0x0800;
  ArpCfgData.SwAddressLength  = 4;
  ArpCfgData.StationAddress   = &Request.LocalIp;
  ArpCfgData.EntryTimeOut     = 0;
  ArpCfgData.RetryCount       = 3;
  ArpCfgData.RetryTimeOut     = 0;
  
  Status = Arp->Configure (Arp, &ArpCfgData);
  
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = Arp->Request (
                  Arp,
                  &Request.DestIp,
                  Request.OnResolved,
                  &Request.DestMac
                  );
  
  if (EFI_ERROR (Status) && (Status != EFI_NOT_READY)) {
    goto ON_EXIT;
  }

  while (!ArpResolved) {
    
  }

ON_EXIT:
  if (Request.OnResolved != NULL) {
    gBS->CloseEvent (Request.OnResolved);
  }

  NetLibDestroyServiceChild (
    NicInfo->Handle, 
    gImageHandle, 
    &gEfiArpServiceBindingProtocolGuid, 
    ArpHandle
    );

  return Request.Duplicate;
}

/**
  The callback function for the timer event used to get map.

  @param[in] Event    The event this function is registered to.
  @param[in] Context  The context registered to the event.
**/
VOID
EFIAPI
TimeoutToGetMap (
  IN EFI_EVENT      Event,
  IN VOID           *Context
  )
{
  mTimeout = TRUE;
  return ;
}

/**
  Create an IP child, use it to start the auto configuration, then destroy it.

  @param[in] NicInfo    The pointer to the NIC_INFO of the Nic to be configured.

  @retval EFI_SUCCESS         The configuration is done.
**/
EFI_STATUS
EFIAPI
IfconfigStartIp4(
  IN NIC_INFO                   *NicInfo
  )
{
  EFI_IP4_PROTOCOL              *Ip4;
  EFI_HANDLE                    Ip4Handle;
  EFI_HANDLE                    TimerToGetMap;
  EFI_IP4_CONFIG_DATA           Ip4ConfigData;
  EFI_IP4_MODE_DATA             Ip4Mode;
  EFI_STATUS                    Status;

  //
  // Get the Ip4ServiceBinding Protocol
  //
  Ip4Handle     = NULL;
  Ip4           = NULL;
  TimerToGetMap = NULL;

  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_START_SET_ADDR), gShellNetwork1HiiHandle);

  Status = NetLibCreateServiceChild (
             NicInfo->Handle,
             gImageHandle,
             &gEfiIp4ServiceBindingProtocolGuid,
             &Ip4Handle
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                 Ip4Handle,
                 &gEfiIp4ProtocolGuid,
                 (VOID **) &Ip4,
                 NicInfo->Handle,
                 gImageHandle,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Ip4ConfigData.DefaultProtocol          = EFI_IP_PROTO_ICMP;
  Ip4ConfigData.AcceptAnyProtocol        = FALSE;
  Ip4ConfigData.AcceptIcmpErrors         = FALSE;
  Ip4ConfigData.AcceptBroadcast          = FALSE;
  Ip4ConfigData.AcceptPromiscuous        = FALSE;
  Ip4ConfigData.UseDefaultAddress        = TRUE;
  ZeroMem (&Ip4ConfigData.StationAddress, sizeof (EFI_IPv4_ADDRESS));
  ZeroMem (&Ip4ConfigData.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  Ip4ConfigData.TypeOfService            = 0;
  Ip4ConfigData.TimeToLive               = 1;
  Ip4ConfigData.DoNotFragment            = FALSE;
  Ip4ConfigData.RawData                  = FALSE;
  Ip4ConfigData.ReceiveTimeout           = 0;
  Ip4ConfigData.TransmitTimeout          = 0;

  Status = Ip4->Configure (Ip4, &Ip4ConfigData);

  if (Status == EFI_NO_MAPPING) {
    mTimeout = FALSE;
    Status  = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL | EVT_TIMER,
                    TPL_CALLBACK,
                    TimeoutToGetMap,
                    NULL,
                    &TimerToGetMap
                    );
    
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    
    Status = gBS->SetTimer (
                   TimerToGetMap,
                   TimerRelative,
                   MultU64x32 (SecondsToNanoSeconds, 5)
                   );
    
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_WAIT_SET_DONE), gShellNetwork1HiiHandle);
    
    while (!mTimeout) {
      Ip4->Poll (Ip4);
  
      if (!EFI_ERROR (Ip4->GetModeData (Ip4, &Ip4Mode, NULL, NULL)) && 
          Ip4Mode.IsConfigured) {       
        break;
      }
    }    
  }

  Status = Ip4->GetModeData (Ip4, &Ip4Mode, NULL, NULL);

  if ((Status == EFI_SUCCESS) && Ip4Mode.IsConfigured) {
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL,
      STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
      gShellNetwork1HiiHandle, 
      L"Default",
      (UINTN)Ip4Mode.ConfigData.StationAddress.Addr[0],
      (UINTN)Ip4Mode.ConfigData.StationAddress.Addr[1],
      (UINTN)Ip4Mode.ConfigData.StationAddress.Addr[2],
      (UINTN)Ip4Mode.ConfigData.StationAddress.Addr[3]
      );
  }
  
ON_EXIT: 

  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_GET_DEF_ADDR_FAIL), gShellNetwork1HiiHandle);
  }

  if (TimerToGetMap != NULL) {
    gBS->SetTimer (TimerToGetMap, TimerCancel, 0);
    gBS->CloseEvent (TimerToGetMap);
  }

  NetLibDestroyServiceChild (
    NicInfo->Handle,
    gImageHandle,
    &gEfiIp4ServiceBindingProtocolGuid,
    Ip4Handle
    );
  
  return Status;
}

/**
  Set the address for the nic specified by the params.

  @param[in] Argc       The count of the passed in Params.
  @param[in] Params     The command line arguments for the set operation.

  @retval EFI_SUCCESS   The address set operation is done.
  @return               Some error occurs.
**/
SHELL_STATUS
EFIAPI
IfconfigSetNicAddr (
  IN UINTN                      Argc,
  IN CONST CHAR16               *Params
  )
{
  NIC_IP4_CONFIG_INFO           *Config;
  NIC_IP4_CONFIG_INFO           *OldConfig;
  EFI_IP_ADDRESS                Ip;
  EFI_IP_ADDRESS                Mask;
  EFI_IP_ADDRESS                Gateway;
  NIC_INFO                      *Info;
  BOOLEAN                       Permanent;
  SHELL_STATUS                  ShellStatus;
  CONST CHAR16                  *Walker;
  CHAR16                        *Temp;
  CONST CHAR16                  *DhcpTemp;
  CONST CHAR16                  *StaticTemp;
  CONST CHAR16                  *PermTemp;
  UINT32                        NetworkBytes1;
  UINT32                        NetworkBytes2;
  EFI_STATUS                    Status;

  Walker  = Params;
  Temp    = NULL;
  Temp    = StrnCatGrow(&Temp, NULL, Walker, StrStr(Walker, L" ")-Walker);
  Info    = IfconfigFindNicByName (Temp);

  if (Info == NULL) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INTERFACE_NOT_FOUND), gShellNetwork1HiiHandle, Temp);
    return SHELL_NOT_FOUND;
  }

  Walker  += StrLen(Temp) + 1;
  FreePool(Temp);
  Temp = NULL;
  Temp    = StrnCatGrow(&Temp, NULL, Walker, StrStr(Walker, L" ")==NULL?0:StrStr(Walker, L" ")-Walker);

  Config = AllocateZeroPool (sizeof (NIC_IP4_CONFIG_INFO) + 2 * sizeof (EFI_IP4_ROUTE_TABLE));
  if (Config == NULL) {
    return SHELL_OUT_OF_RESOURCES;
  }

  Config->Ip4Info.RouteTable = (EFI_IP4_ROUTE_TABLE *) (Config + 1);

  OldConfig = Info->ConfigInfo;
  Permanent   = FALSE;
  ShellStatus = SHELL_INVALID_PARAMETER;

  DhcpTemp = DhcpString;
  StaticTemp = StaticString;
  
  if (StringNoCaseCompare(&Temp, &DhcpTemp) == 0) {
    //
    // Validate the parameter for DHCP, two valid forms: eth0 DHCP and eth0 DHCP permanent
    //
    if ((Argc != 2) && (Argc!= 3)) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ifconfig", Temp);  
      ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
      goto ON_EXIT;
    }

    if (Argc == 3) {
      Walker  += StrLen(Temp) + 1;
      FreePool(Temp);
      Temp    = NULL;
      Temp    = StrnCatGrow(&Temp, NULL, Walker, 0);

      PermTemp = PermanentString;
      if (StringNoCaseCompare(&Temp, &PermTemp) != 0) {
        ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_GEN_PROBLEM_OP2), gShellNetwork1HiiHandle, L"ifconfig", Temp, PermanentString, L"Nothing");  
        ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
        goto ON_EXIT;
      }

      Permanent = TRUE;
    }

    if ((OldConfig != NULL) && (OldConfig->Source == IP4_CONFIG_SOURCE_DHCP) &&
        (OldConfig->Permanent == Permanent)) {

      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INTERFACE_CONFIGURED), gShellNetwork1HiiHandle, Info->Name);
      ShellStatus = SHELL_ALREADY_STARTED;
      goto ON_EXIT;
    }

    Config->Source = IP4_CONFIG_SOURCE_DHCP;
  } else if (StringNoCaseCompare(&Temp, &StaticTemp) == 0) {
    //
    // validate the parameter, two forms: eth0 static IP NETMASK GATEWAY and
    // eth0 static IP NETMASK GATEWAY permanent 
    //
    if ((Argc != 5) && (Argc != 6)) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ifconfig", Temp);  
      ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
      goto ON_EXIT;
    }

    Walker  += StrLen(Temp) + 1;
    FreePool(Temp);
    Temp    = NULL;
    Temp    = StrnCatGrow(&Temp, NULL, Walker, StrStr(Walker, L" ")-Walker);

    if (EFI_ERROR (NetLibStrToIp4 (Temp, &Ip.v4))) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_IP_STR), gShellNetwork1HiiHandle, Temp);
      ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
      goto ON_EXIT;
    }

    Walker  += StrLen(Temp) + 1;
    FreePool(Temp);
    Temp    = NULL;
    Temp    = StrnCatGrow(&Temp, NULL, Walker, StrStr(Walker, L" ")-Walker);
    if (EFI_ERROR (NetLibStrToIp4 (Temp, &Mask.v4))) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_IP_STR), gShellNetwork1HiiHandle, Temp);
      ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
      goto ON_EXIT;
    }

    Walker  += StrLen(Temp) + 1;
    FreePool(Temp);
    Temp    = NULL;
    if (Argc == 6) {
      Temp    = StrnCatGrow(&Temp, NULL, Walker, StrStr(Walker, L" ")-Walker);
    } else {
      Temp    = StrnCatGrow(&Temp, NULL, Walker, 0);
    }
    if (EFI_ERROR (NetLibStrToIp4 (Temp, &Gateway.v4))) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_IP_STR), gShellNetwork1HiiHandle, Temp);
      ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
      goto ON_EXIT;
    }

    if (Argc == 6) {
      Walker  += StrLen(Temp) + 1;
      FreePool(Temp);
      Temp    = NULL;
      Temp    = StrnCatGrow(&Temp, NULL, Walker, 0);

      PermTemp = PermanentString;
      if (StringNoCaseCompare(&Temp, &PermTemp) != 0) {
        ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_GEN_PROBLEM_OP2), gShellNetwork1HiiHandle, L"ifconfig", Temp, PermanentString, L"Nothing");  
        ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
        goto ON_EXIT;
      }

      Permanent = TRUE;
    }

    NetworkBytes1 = NTOHL (Ip.Addr[0]);
    NetworkBytes2 = NTOHL (Mask.Addr[0]);
    if ((Ip.Addr[0] == 0) || (Mask.Addr[0] == 0) ||
        !NetIp4IsUnicast (NetworkBytes1, NetworkBytes2)) {

      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_ADDR_PAIR), gShellNetwork1HiiHandle);
      ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
      goto ON_EXIT;
    }

    NetworkBytes1 = NTOHL (Gateway.Addr[0]);
    if (!IP4_NET_EQUAL (Ip.Addr[0], Gateway.Addr[0], Mask.Addr[0]) ||
        !NetIp4IsUnicast (NetworkBytes1, NetworkBytes2)) {
        
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_GATEWAY), gShellNetwork1HiiHandle);
      ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
      goto ON_EXIT;
    }

    //
    // Set the configuration up, two route table entries are added:
    // one for the direct connected network, and another for the 
    // default gateway. Remember, some structure members are cleared
    // by AllocateZeroPool
    //
    Config->Source = IP4_CONFIG_SOURCE_STATIC;
    Config->Ip4Info.RouteTableSize = 2;

    CopyMem (&Config->Ip4Info.StationAddress, &Ip.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Config->Ip4Info.SubnetMask, &Mask.v4, sizeof (EFI_IPv4_ADDRESS));

    Ip.Addr[0] = Ip.Addr[0] & Mask.Addr[0];

    CopyMem (&Config->Ip4Info.RouteTable[0].SubnetAddress, &Ip.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Config->Ip4Info.RouteTable[0].SubnetMask, &Mask.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Config->Ip4Info.RouteTable[1].GatewayAddress, &Gateway.v4, sizeof (EFI_IPv4_ADDRESS));
  } else {
    // neither static or DHCP.  error.
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_GEN_TOO_FEW), gShellNetwork1HiiHandle, L"ifconfig");  
    ASSERT(ShellStatus == SHELL_INVALID_PARAMETER);
    goto ON_EXIT;
  }

  CopyMem (&Config->NicAddr, &Info->NicAddress, sizeof (NIC_ADDR));
  Config->Permanent = Permanent;

  //
  // Use HII service to set NIC address
  //
  ShellStatus = IfconfigSetNicAddrByHii (Info, Config);
  if (ShellStatus != SHELL_SUCCESS) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_SET_FAIL), gShellNetwork1HiiHandle, ShellStatus^MAX_BIT);
    goto ON_EXIT;
  } 

  Status = IfconfigStartIp4 (Info);
  if (EFI_ERROR(Status)) {
    ShellStatus = SHELL_ACCESS_DENIED;
  }

  if (ShellStatus != SHELL_SUCCESS) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_IP_CHILD_FAIL), gShellNetwork1HiiHandle, ShellStatus^MAX_BIT);
  }
  
ON_EXIT:
  SHELL_FREE_NON_NULL(Config);
  
  return ShellStatus;
}

/**
  Show the address information for the nic specified.

  @param[in] Name   A pointer to the string containg the nic's name, if NULL, 
                    all nics' information is shown.
**/
VOID
EFIAPI
IfconfigShowNicInfo (
  IN CONST CHAR16           *Name
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *NextEntry;
  NIC_INFO                  *NicInfo;
  UINT32                    Index;
  EFI_IP4_IPCONFIG_DATA     *Ip4Config;
  EFI_IPv4_ADDRESS          Gateway;
  CONST CHAR16              *TempString;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &NicInfoList) {
    NicInfo = BASE_CR (Entry, NIC_INFO, Link);

    TempString = (CHAR16*)NicInfo->Name;
    if ((Name != NULL) && (StringNoCaseCompare (&Name, &TempString) != 0)) {
      continue;
    }

    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_NIC_NAME), gShellNetwork1HiiHandle, NicInfo->Name);

    ShellPrintHiiEx(
    -1,
    -1,
    NULL,
    STRING_TOKEN(STR_IFCONFIG_SHOW_MAC_ADDR), 
    gShellNetwork1HiiHandle, 
    (UINTN)NicInfo->NicAddress.MacAddr.Addr[0], 
    (UINTN)NicInfo->NicAddress.MacAddr.Addr[1], 
    (UINTN)NicInfo->NicAddress.MacAddr.Addr[2],
    (UINTN)NicInfo->NicAddress.MacAddr.Addr[3], 
    (UINTN)NicInfo->NicAddress.MacAddr.Addr[4], 
    (UINTN)NicInfo->NicAddress.MacAddr.Addr[5]
    );    

    Print (L"  Media State: %s\n", NicInfo->MediaPresent ? L"Media present" : L"Media disconnected");

    if (NicInfo->ConfigInfo == NULL) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_NIC_NOT_CONFIGURED), gShellNetwork1HiiHandle);
      continue;
    } 

    if (NicInfo->ConfigInfo->Source == IP4_CONFIG_SOURCE_DHCP) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_CONFIG_SOURCE), gShellNetwork1HiiHandle, L"DHCP");
    } else if (NicInfo->ConfigInfo->Source == IP4_CONFIG_SOURCE_STATIC) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_CONFIG_SOURCE), gShellNetwork1HiiHandle, L"STATIC");
    } else {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_CONFIG_SOURCE), gShellNetwork1HiiHandle, L"Unknown");
    }

    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_IFCONFIG_PERMANENT_STATUS),
      gShellNetwork1HiiHandle,
      (NicInfo->ConfigInfo->Permanent? L"TRUE":L"FALSE")
      );

    Ip4Config = &NicInfo->ConfigInfo->Ip4Info;

    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL,
      STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
      gShellNetwork1HiiHandle, 
      L"IP address",
      (UINTN)Ip4Config->StationAddress.Addr[0],
      (UINTN)Ip4Config->StationAddress.Addr[1],
      (UINTN)Ip4Config->StationAddress.Addr[2],
      (UINTN)Ip4Config->StationAddress.Addr[3]
      );
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL,
      STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
      gShellNetwork1HiiHandle, 
      L"Mask",
      (UINTN)Ip4Config->SubnetMask.Addr[0],
      (UINTN)Ip4Config->SubnetMask.Addr[1],
      (UINTN)Ip4Config->SubnetMask.Addr[2],
      (UINTN)Ip4Config->SubnetMask.Addr[3]
      );

    ZeroMem (&Gateway, sizeof (EFI_IPv4_ADDRESS));
    
    for (Index = 0; Index < Ip4Config->RouteTableSize; Index++) {
      if ((CompareMem (&Ip4Config->RouteTable[Index].SubnetAddress, &mZeroIp4Addr, sizeof (EFI_IPv4_ADDRESS)) == 0) &&
          (CompareMem (&Ip4Config->RouteTable[Index].SubnetMask   , &mZeroIp4Addr, sizeof (EFI_IPv4_ADDRESS)) == 0) ){
        CopyMem (&Gateway, &Ip4Config->RouteTable[Index].GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
      }
    }
   
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL,
      STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
      gShellNetwork1HiiHandle, 
      L"Gateway",
      (UINTN)Gateway.Addr[0],
      (UINTN)Gateway.Addr[1],
      (UINTN)Gateway.Addr[2],
      (UINTN)Gateway.Addr[3]
      );

    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_ROUTES_SIZE), gShellNetwork1HiiHandle, Ip4Config->RouteTableSize);

    for (Index = 0; Index < Ip4Config->RouteTableSize; Index++) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_ROUTES_ENTRY_INDEX), gShellNetwork1HiiHandle, Index);

      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL,
        STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
        gShellNetwork1HiiHandle, 
        L"Subnet",
        (UINTN)Ip4Config->RouteTable[Index].SubnetAddress.Addr[0],
        (UINTN)Ip4Config->RouteTable[Index].SubnetAddress.Addr[1],
        (UINTN)Ip4Config->RouteTable[Index].SubnetAddress.Addr[2],
        (UINTN)Ip4Config->RouteTable[Index].SubnetAddress.Addr[3]
        );

      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL,
        STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
        gShellNetwork1HiiHandle, 
        L"Netmask",
        (UINTN)Ip4Config->RouteTable[Index].SubnetMask.Addr[0],
        (UINTN)Ip4Config->RouteTable[Index].SubnetMask.Addr[1],
        (UINTN)Ip4Config->RouteTable[Index].SubnetMask.Addr[2],
        (UINTN)Ip4Config->RouteTable[Index].SubnetMask.Addr[3]
        );

      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL,
        STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR), 
        gShellNetwork1HiiHandle, 
        L"Gateway",
        (UINTN)Ip4Config->RouteTable[Index].GatewayAddress.Addr[0],
        (UINTN)Ip4Config->RouteTable[Index].GatewayAddress.Addr[1],
        (UINTN)Ip4Config->RouteTable[Index].GatewayAddress.Addr[2],
        (UINTN)Ip4Config->RouteTable[Index].GatewayAddress.Addr[3]
        );
    }
  }

  return ;
}

/**
  Clear address configuration for the nic specified.

  @param[in] Name     A pointer to the string containg the nic's name, 
                      if NULL, all nics address configurations are cleared.

  @retval EFI_SUCCESS The address configuration is cleared.
  @return             Some error occurs.
**/
EFI_STATUS
EFIAPI
IfconfigClearNicAddr (
  IN CONST CHAR16                     *Name
  )
{
  LIST_ENTRY                    *Entry;
  LIST_ENTRY                    *NextEntry;
  NIC_INFO                      *Info;
  EFI_STATUS                    Status;
  
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &NicInfoList) {
    Info = BASE_CR (Entry, NIC_INFO, Link);

    if ((Name != NULL) && (StrCmp (Name, Info->Name) != 0)) {
      continue;
    }

//    if (Info->NicIp4Config == NULL) { 
      Status = IfconfigSetNicAddrByHii (Info, NULL);
//    } else {
//      Status = Info->NicIp4Config->SetInfo (Info->NicIp4Config, NULL, TRUE);
//    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
  
}

/**
  Function for 'ifconfig' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunIfconfig (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  BOOLEAN             ListOperation;
  BOOLEAN             ClearOperation;
  BOOLEAN             SetOperation;
  CONST CHAR16        *Item;
  LIST_ENTRY          *Entry;
  NIC_INFO            *Info;

  InitializeListHead (&NicInfoList);
  Status = EFI_INVALID_PARAMETER;
  ShellStatus = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellNetwork1HiiHandle, L"ifconfig", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }

    goto Done;
  }

  ClearOperation = ShellCommandLineGetFlag(Package, L"-c");
  ListOperation  = ShellCommandLineGetFlag(Package, L"-l");
  SetOperation   = ShellCommandLineGetFlag(Package, L"-s");

  if ((ClearOperation && ListOperation)
    ||(SetOperation   && ListOperation)
    ||(ClearOperation && SetOperation)
    ) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CON), gShellNetwork1HiiHandle, L"ifconfig");  
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto Done;
  } else if (!ClearOperation && !ListOperation && !SetOperation) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellNetwork1HiiHandle, L"ifconfig");  
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto Done;
  }
    
    
  Status = IfconfigGetAllNicInfoByHii ();
  if (EFI_ERROR (Status)) {
    if (mIp4ConfigExist) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_GET_NIC_FAIL), gShellNetwork1HiiHandle, Status);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROTOCOL_NF), gShellNetwork1HiiHandle, L"ifconfig", L"gEfiIp4ConfigProtocolGuid", &gEfiIp4ConfigProtocolGuid);  
    }

    return SHELL_NOT_FOUND;
  }

  if (ListOperation) {
    Item = ShellCommandLineGetValue (Package, L"-l");

    if (Item != NULL && CountSubItems(Item) > 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellNetwork1HiiHandle, L"ifconfig", Item, L"-l");  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } 
    
    //
    // Show the configuration.
    //
    IfconfigShowNicInfo (Item);
  } else if (SetOperation) {
    Item = ShellCommandLineGetValue (Package, L"-s");

    //
    // The correct command line arguments for setting address are:
    // IfConfig -s eth0 DHCP [permanent]
    // IfConfig -s eth0 static ip netmask gateway [permanent]
    //
    if (Item == NULL || (CountSubItems(Item) < 2) || (CountSubItems(Item) > 6) || (CountSubItems(Item) == 4)) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_GEN_NO_VALUE), gShellNetwork1HiiHandle, L"ifconfig", L"-s");  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    ShellStatus = IfconfigSetNicAddr (CountSubItems(Item), Item);
  } else if (ClearOperation) {
    Item = ShellCommandLineGetValue (Package, L"-c");

    if (Item != NULL && CountSubItems(Item) > 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellNetwork1HiiHandle, L"ifconfig", Item, L"-c");  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    IfconfigClearNicAddr (Item);
  } else {
    ASSERT(FALSE);
  }

Done:
  while (!IsListEmpty (&NicInfoList)) {
    Entry = NicInfoList.ForwardLink;
    Info  = BASE_CR (Entry, NIC_INFO, Link);

    RemoveEntryList (Entry);

    if (Info->ConfigInfo != NULL) {
      FreePool (Info->ConfigInfo);
    }

    FreePool (Info);
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList(Package);
  }

  return (ShellStatus);
}
