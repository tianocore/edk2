/** @file
  The implementation of common functions shared by IP6 driver.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ip6Impl.h"

/**
  Build a array of EFI_IP6_ADDRESS_INFO to be returned to the caller. The number
  of EFI_IP6_ADDRESS_INFO is also returned. If AddressList is NULL,
  only the address count is returned.

  @param[in]  IpSb              The IP6 service binding instance.
  @param[out] AddressCount      The number of returned addresses.
  @param[out] AddressList       The pointer to the array of EFI_IP6_ADDRESS_INFO.
                                This is an optional parameter.


  @retval EFI_SUCCESS           The address array successfully built.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the address info.
  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.

**/
EFI_STATUS
Ip6BuildEfiAddressList (
  IN IP6_SERVICE            *IpSb,
  OUT UINT32                *AddressCount,
  OUT EFI_IP6_ADDRESS_INFO  **AddressList OPTIONAL
  )
{
  UINT32                Count;
  LIST_ENTRY            *Entry;
  EFI_IP6_ADDRESS_INFO  *EfiAddrInfo;
  IP6_ADDRESS_INFO      *AddrInfo;

  if (AddressCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IpSb->LinkLocalOk) {
    Count = 1 + IpSb->DefaultInterface->AddressCount;
  } else {
    Count = 0;
  }

  *AddressCount = Count;

  if ((AddressList == NULL) || (Count == 0)) {
    return EFI_SUCCESS;
  }

  if (*AddressList == NULL) {
    *AddressList = AllocatePool (sizeof (EFI_IP6_ADDRESS_INFO) * Count);
    if (*AddressList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  EfiAddrInfo = *AddressList;

  IP6_COPY_ADDRESS (&EfiAddrInfo->Address, &IpSb->LinkLocalAddr);
  EfiAddrInfo->PrefixLength = IP6_LINK_LOCAL_PREFIX_LENGTH;

  EfiAddrInfo++;
  Count = 1;

  NET_LIST_FOR_EACH (Entry, &IpSb->DefaultInterface->AddressList) {
    AddrInfo = NET_LIST_USER_STRUCT_S (Entry, IP6_ADDRESS_INFO, Link, IP6_ADDR_INFO_SIGNATURE);

    IP6_COPY_ADDRESS (&EfiAddrInfo->Address, &AddrInfo->Address);
    EfiAddrInfo->PrefixLength = AddrInfo->PrefixLength;

    EfiAddrInfo++;
    Count++;
  }

  ASSERT (Count == *AddressCount);

  return EFI_SUCCESS;
}

/**
  Generate the multicast addresses identify the group of all IPv6 nodes or IPv6
  routers defined in RFC4291.

  All Nodes Addresses: FF01::1, FF02::1.
  All Router Addresses: FF01::2, FF02::2, FF05::2.

  @param[in]  Router            If TRUE, generate all routers addresses,
                                else generate all node addresses.
  @param[in]  Scope             interface-local(1), link-local(2), or site-local(5)
  @param[out] Ip6Addr           The generated multicast address.

  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval EFI_SUCCESS           The address is generated.

**/
EFI_STATUS
Ip6SetToAllNodeMulticast (
  IN  BOOLEAN          Router,
  IN  UINT8            Scope,
  OUT EFI_IPv6_ADDRESS *Ip6Addr
  )
{
  if (Ip6Addr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Router && Scope == IP6_SITE_LOCAL_SCOPE) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Ip6Addr, sizeof (EFI_IPv6_ADDRESS));
  Ip6Addr->Addr[0] = 0xFF;
  Ip6Addr->Addr[1] = Scope;

  if (!Router) {
    Ip6Addr->Addr[15] = 0x1;
  } else {
    Ip6Addr->Addr[15] = 0x2;
  }

  return EFI_SUCCESS;
}

/**
  This function converts MAC address to 64 bits interface ID according to RFC4291
  and returns the interface ID. Currently only 48-bit MAC address is supported by
  this function.

  @param[in, out]  IpSb      The IP6 service binding instance.

  @retval          NULL      The operation fails.
  @return                    Pointer to the generated interface ID.

**/
UINT8 *
Ip6CreateInterfaceID (
  IN OUT IP6_SERVICE         *IpSb
  )
{
  UINT8                      InterfaceId[8];
  UINT8                      Byte;
  EFI_MAC_ADDRESS            *MacAddr;
  UINT32                     AddrLen;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  AddrLen = IpSb->SnpMode.HwAddressSize;

  //
  // Currently only IEEE 802 48-bit MACs are supported to create link local address.
  //
  if (AddrLen != IP6_MAC_LEN || IpSb->InterfaceIdLen != IP6_IF_ID_LEN) {
    return NULL;
  }

  MacAddr = &IpSb->SnpMode.CurrentAddress;

  //
  // Convert MAC address to 64 bits interface ID according to Appendix A of RFC4291:
  // 1. Insert 0xFFFE to the middle
  // 2. Invert the universal/local bit - bit 6 in network order
  //
  CopyMem (InterfaceId, MacAddr, 3);
  InterfaceId[3] = 0xFF;
  InterfaceId[4] = 0xFE;
  CopyMem (&InterfaceId[5], &MacAddr->Addr[3], 3);

  Byte = (UINT8) (InterfaceId[0] & IP6_U_BIT);
  if (Byte == IP6_U_BIT) {
    InterfaceId[0] &= ~IP6_U_BIT;
  } else {
    InterfaceId[0] |= IP6_U_BIT;
  }

  //
  // Return the interface ID.
  //
  return AllocateCopyPool (IpSb->InterfaceIdLen, InterfaceId);
}

/**
  This function creates link-local address from interface identifier. The
  interface identifier is normally created from MAC address. It might be manually
  configured by administrator if the link-local address created from MAC address
  is a duplicate address.

  @param[in, out]  IpSb      The IP6 service binding instance.

  @retval          NULL      If the operation fails.
  @return                    The generated Link Local address, in network order.

**/
EFI_IPv6_ADDRESS *
Ip6CreateLinkLocalAddr (
  IN OUT IP6_SERVICE           *IpSb
  )
{
  EFI_IPv6_ADDRESS             *Ip6Addr;
  EFI_IP6_CONFIG_PROTOCOL      *Ip6Config;
  UINTN                        DataSize;
  EFI_IP6_CONFIG_INTERFACE_ID  InterfaceId;
  EFI_STATUS                   Status;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  if (IpSb->InterfaceId != NULL) {
    FreePool (IpSb->InterfaceId);
  }

  //
  // Get the interface id if it is manully configured.
  //
  Ip6Config = &IpSb->Ip6ConfigInstance.Ip6Config;
  DataSize  = sizeof (EFI_IP6_CONFIG_INTERFACE_ID);
  ZeroMem (&InterfaceId, DataSize);

  Status = Ip6Config->GetData (
                        Ip6Config,
                        Ip6ConfigDataTypeAltInterfaceId,
                        &DataSize,
                        &InterfaceId
                        );
  if (Status == EFI_NOT_FOUND) {
    //
    // Since the interface id is not configured, generate the interface id from
    // MAC address.
    //
    IpSb->InterfaceId = Ip6CreateInterfaceID (IpSb);
    if (IpSb->InterfaceId == NULL) {
      return NULL;
    }

    CopyMem (&InterfaceId, IpSb->InterfaceId, IpSb->InterfaceIdLen);
    //
    // Record the interface id.
    //
    Status = Ip6Config->SetData (
                          Ip6Config,
                          Ip6ConfigDataTypeAltInterfaceId,
                          DataSize,
                          &InterfaceId
                          );
    if (EFI_ERROR (Status)) {
      FreePool (IpSb->InterfaceId);
      IpSb->InterfaceId = NULL;
      return NULL;
    }
  } else if (!EFI_ERROR (Status)) {
    IpSb->InterfaceId = AllocateCopyPool (DataSize, &InterfaceId);
    if (IpSb->InterfaceId == NULL) {
      return NULL;
    }
  } else {
    return NULL;
  }

  //
  // Append FE80::/64 to the left of IPv6 address then return.
  //
  Ip6Addr = AllocateZeroPool (sizeof (EFI_IPv6_ADDRESS));
  if (Ip6Addr == NULL) {
    FreePool (IpSb->InterfaceId);
    IpSb->InterfaceId = NULL;
    return NULL;
  }

  CopyMem (&Ip6Addr->Addr[8], IpSb->InterfaceId, IpSb->InterfaceIdLen);
  Ip6Addr->Addr[1] = 0x80;
  Ip6Addr->Addr[0] = 0xFE;

  return Ip6Addr;
}

/**
  Compute the solicited-node multicast address for an unicast or anycast address,
  by taking the low-order 24 bits of this address, and appending those bits to
  the prefix FF02:0:0:0:0:1:FF00::/104.

  @param[in]  Ip6Addr               The unicast or anycast address, in network order.
  @param[out] MulticastAddr         The generated solicited-node multicast address,
                                    in network order.

**/
VOID
Ip6CreateSNMulticastAddr (
  IN EFI_IPv6_ADDRESS  *Ip6Addr,
  OUT EFI_IPv6_ADDRESS *MulticastAddr
  )
{
  ASSERT (Ip6Addr != NULL && MulticastAddr != NULL);

  ZeroMem (MulticastAddr, sizeof (EFI_IPv6_ADDRESS));

  MulticastAddr->Addr[0]  = 0xFF;
  MulticastAddr->Addr[1]  = 0x02;
  MulticastAddr->Addr[11] = 0x1;
  MulticastAddr->Addr[12] = 0xFF;

  CopyMem (&MulticastAddr->Addr[13], &Ip6Addr->Addr[13], 3);
}

/**
  Insert a node IP6_ADDRESS_INFO to an IP6 interface.

  @param[in, out]  IpIf             Points to an IP6 interface.
  @param[in]       AddrInfo         Points to IP6_ADDRESS_INFO

**/
VOID
Ip6AddAddr (
  IN OUT IP6_INTERFACE *IpIf,
  IN IP6_ADDRESS_INFO  *AddrInfo
  )
{
  InsertHeadList (&IpIf->AddressList, &AddrInfo->Link);
  IpIf->AddressCount++;
}

/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.
  
  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
Ip6DestroyChildEntryByAddr (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  IP6_PROTOCOL                  *Instance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  EFI_IPv6_ADDRESS              *Address;
  
  Instance = NET_LIST_USER_STRUCT_S (Entry, IP6_PROTOCOL, Link, IP6_PROTOCOL_SIGNATURE);
  ServiceBinding = ((IP6_DESTROY_CHILD_BY_ADDR_CALLBACK_CONTEXT*) Context)->ServiceBinding;
  Address = ((IP6_DESTROY_CHILD_BY_ADDR_CALLBACK_CONTEXT*) Context)->Address;

  if ((Instance->State == IP6_STATE_CONFIGED) && EFI_IP6_EQUAL (&Instance->ConfigData.StationAddress, Address)) {
    return ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
  }
  
  return EFI_SUCCESS;
}

/**
  Destroy the IP instance if its StationAddress is removed. It is the help function
  for Ip6RemoveAddr().

  @param[in, out]  IpSb             Points to an IP6 service binding instance.
  @param[in]       Address          The to be removed address

**/
VOID
Ip6DestroyInstanceByAddress (
  IN OUT IP6_SERVICE   *IpSb,
  IN EFI_IPv6_ADDRESS  *Address
  )
{
  LIST_ENTRY                    *List;
  IP6_DESTROY_CHILD_BY_ADDR_CALLBACK_CONTEXT  Context;

  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  List = &IpSb->Children;
  Context.ServiceBinding = &IpSb->ServiceBinding;
  Context.Address = Address;
  NetDestroyLinkList (
    List,
    Ip6DestroyChildEntryByAddr,
    &Context,
    NULL
    );
}

/**
  Remove the IPv6 address from the address list node points to IP6_ADDRESS_INFO.

  This function removes the matching IPv6 addresses from the address list and
  adjusts the address count of the address list. If IpSb is not NULL, this function
  calls Ip6LeaveGroup to see whether it should call Mnp->Groups() to remove the
  its solicited-node multicast MAC address from the filter list and sends out
  a Multicast Listener Done. If Prefix is NULL, all address in the address list
  will be removed. If Prefix is not NULL, the address that matching the Prefix
  with PrefixLength in the address list will be removed.

  @param[in]       IpSb             NULL or points to IP6 service binding instance.
  @param[in, out]  AddressList      Address list array.
  @param[in, out]  AddressCount     The count of addresses in address list array.
  @param[in]       Prefix           NULL or an IPv6 address prefix.
  @param[in]       PrefixLength     The length of Prefix.

  @retval    EFI_SUCCESS            The operation completed successfully.
  @retval    EFI_NOT_FOUND          The address matching the Prefix with PrefixLength
                                    cannot be found in the address list.
  @retval    EFI_INVALID_PARAMETER  Any input parameter is invalid.

**/
EFI_STATUS
Ip6RemoveAddr (
  IN IP6_SERVICE       *IpSb          OPTIONAL,
  IN OUT LIST_ENTRY    *AddressList,
  IN OUT UINT32        *AddressCount,
  IN EFI_IPv6_ADDRESS  *Prefix        OPTIONAL,
  IN UINT8             PrefixLength
  )
{
  EFI_STATUS           Status;
  LIST_ENTRY           *Entry;
  LIST_ENTRY           *Next;
  IP6_ADDRESS_INFO     *AddrInfo;
  EFI_IPv6_ADDRESS     SnMCastAddr;

  if (IsListEmpty (AddressList) || *AddressCount < 1 || PrefixLength > IP6_PREFIX_NUM) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, AddressList) {
    AddrInfo = NET_LIST_USER_STRUCT_S (Entry, IP6_ADDRESS_INFO, Link, IP6_ADDR_INFO_SIGNATURE);

    if (Prefix == NULL ||
        (PrefixLength == 128 && EFI_IP6_EQUAL (Prefix, &AddrInfo->Address)) ||
        (PrefixLength == AddrInfo->PrefixLength && NetIp6IsNetEqual (Prefix, &AddrInfo->Address, PrefixLength))
        ) {
      if (IpSb != NULL) {
        NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);
        Ip6CreateSNMulticastAddr (&AddrInfo->Address, &SnMCastAddr);
        Ip6LeaveGroup (IpSb, &SnMCastAddr);

        //
        // Destroy any instance who is using the dying address as the source address.
        //
        Ip6DestroyInstanceByAddress (IpSb, &AddrInfo->Address);
      }

      RemoveEntryList (Entry);
      FreePool (AddrInfo);
      (*AddressCount)--;

      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

/**
  Check whether the incoming Ipv6 address is a solicited-node multicast address.

  @param[in]  Ip6               Ip6 address, in network order.

  @retval TRUE                  Yes, solicited-node multicast address
  @retval FALSE                 No

**/
BOOLEAN
Ip6IsSNMulticastAddr (
  IN EFI_IPv6_ADDRESS *Ip6
  )
{
  EFI_IPv6_ADDRESS    Sn;
  BOOLEAN             Flag;

  Ip6CreateSNMulticastAddr (Ip6, &Sn);
  Flag = FALSE;

  if (CompareMem (Sn.Addr, Ip6->Addr, 13) == 0) {
    Flag = TRUE;
  }

  return Flag;
}

/**
  Check whether the incoming IPv6 address is one of the maintained addresses in
  the IP6 service binding instance.

  @param[in]  IpSb              Points to a IP6 service binding instance.
  @param[in]  Address           The IP6 address to be checked.
  @param[out] Interface         If not NULL, output the IP6 interface which
                                maintains the Address.
  @param[out] AddressInfo       If not NULL, output the IP6 address information
                                of the Address.

  @retval TRUE                  Yes, it is one of the maintained address.
  @retval FALSE                 No, it is not one of the maintained address.

**/
BOOLEAN
Ip6IsOneOfSetAddress (
  IN  IP6_SERVICE           *IpSb,
  IN  EFI_IPv6_ADDRESS      *Address,
  OUT IP6_INTERFACE         **Interface   OPTIONAL,
  OUT IP6_ADDRESS_INFO      **AddressInfo OPTIONAL
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Entry2;
  IP6_INTERFACE             *IpIf;
  IP6_ADDRESS_INFO          *TmpAddressInfo;

  //
  // Check link-local address first
  //
  if (IpSb->LinkLocalOk && EFI_IP6_EQUAL (&IpSb->LinkLocalAddr, Address)) {
    if (Interface != NULL) {
      *Interface = IpSb->DefaultInterface;
    }

    if (AddressInfo != NULL) {
      *AddressInfo = NULL;
    }

    return TRUE;
  }

  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    IpIf = NET_LIST_USER_STRUCT_S (Entry, IP6_INTERFACE, Link, IP6_INTERFACE_SIGNATURE);

    NET_LIST_FOR_EACH (Entry2, &IpIf->AddressList) {
      TmpAddressInfo = NET_LIST_USER_STRUCT_S (Entry2, IP6_ADDRESS_INFO, Link, IP6_ADDR_INFO_SIGNATURE);

      if (EFI_IP6_EQUAL (&TmpAddressInfo->Address, Address)) {
        if (Interface != NULL) {
          *Interface = IpIf;
        }

        if (AddressInfo != NULL) {
          *AddressInfo = TmpAddressInfo;
        }

        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Check whether the incoming MAC address is valid.

  @param[in]  IpSb              Points to a IP6 service binding instance.
  @param[in]  LinkAddress       The MAC address.

  @retval TRUE                  Yes, it is valid.
  @retval FALSE                 No, it is not valid.

**/
BOOLEAN
Ip6IsValidLinkAddress (
  IN  IP6_SERVICE      *IpSb,
  IN  EFI_MAC_ADDRESS  *LinkAddress
  )
{
  UINT32               Index;

  //
  // TODO: might be updated later to be more acceptable.
  //
  for (Index = IpSb->SnpMode.HwAddressSize; Index < sizeof (EFI_MAC_ADDRESS); Index++) {
    if (LinkAddress->Addr[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Copy the PrefixLength bits from Src to Dest.

  @param[out] Dest              A pointer to the buffer to copy to.
  @param[in]  Src               A pointer to the buffer to copy from.
  @param[in]  PrefixLength      The number of bits to copy.

**/
VOID
Ip6CopyAddressByPrefix (
  OUT EFI_IPv6_ADDRESS *Dest,
  IN  EFI_IPv6_ADDRESS *Src,
  IN  UINT8            PrefixLength
  )
{
  UINT8 Byte;
  UINT8 Bit;
  UINT8 Mask;

  ASSERT (Dest != NULL && Src != NULL);
  ASSERT (PrefixLength < IP6_PREFIX_NUM);

  Byte = (UINT8) (PrefixLength / 8);
  Bit  = (UINT8) (PrefixLength % 8);

  ZeroMem (Dest, sizeof (EFI_IPv6_ADDRESS));

  CopyMem (Dest, Src, Byte);

  if (Bit > 0) {
    Mask = (UINT8) (0xFF << (8 - Bit));
    ASSERT (Byte < 16);
    Dest->Addr[Byte] = (UINT8) (Src->Addr[Byte] & Mask);
  }
}

/**
  Get the MAC address for a multicast IP address. Call
  Mnp's McastIpToMac to find the MAC address instead of
  hard-coding the NIC to be Ethernet.

  @param[in]  Mnp                   The Mnp instance to get the MAC address.
  @param[in]  Multicast             The multicast IP address to translate.
  @param[out] Mac                   The buffer to hold the translated address.

  @retval EFI_SUCCESS               The multicast IP successfully
                                    translated to a multicast MAC address.
  @retval Other                     The address is not converted because an error occurred.

**/
EFI_STATUS
Ip6GetMulticastMac (
  IN  EFI_MANAGED_NETWORK_PROTOCOL *Mnp,
  IN  EFI_IPv6_ADDRESS             *Multicast,
  OUT EFI_MAC_ADDRESS              *Mac
  )
{
  EFI_IP_ADDRESS        EfiIp;

  IP6_COPY_ADDRESS (&EfiIp.v6, Multicast);

  return Mnp->McastIpToMac (Mnp, TRUE, &EfiIp, Mac);
}

/**
  Convert the multibyte field in IP header's byter order.
  In spite of its name, it can also be used to convert from
  host to network byte order.

  @param[in, out]  Head                  The IP head to convert.

  @return Point to the converted IP head.

**/
EFI_IP6_HEADER *
Ip6NtohHead (
  IN OUT EFI_IP6_HEADER *Head
  )
{
  Head->FlowLabelL    = NTOHS (Head->FlowLabelL);
  Head->PayloadLength = NTOHS (Head->PayloadLength);

  return Head;
}

