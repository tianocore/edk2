/** @file
  The mian interface of IPsec Protocol.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfigImpl.h"
#include "IpSecImpl.h"

EFI_IPSEC2_PROTOCOL  mIpSecInstance = { IpSecProcess, NULL, TRUE };

/**
  Handles IPsec packet processing for inbound and outbound IP packets.

  The EFI_IPSEC_PROCESS process routine handles each inbound or outbound packet.
  The behavior is that it can perform one of the following actions:
  bypass the packet, discard the packet, or protect the packet.

  @param[in]      This             Pointer to the EFI_IPSEC2_PROTOCOL instance.
  @param[in]      NicHandle        Instance of the network interface.
  @param[in]      IpVersion        IPV4 or IPV6.
  @param[in, out] IpHead           Pointer to the IP Header.
  @param[in, out] LastHead         The protocol of the next layer to be processed by IPsec.
  @param[in, out] OptionsBuffer    Pointer to the options buffer.
  @param[in, out] OptionsLength    Length of the options buffer.
  @param[in, out] FragmentTable    Pointer to a list of fragments.
  @param[in, out] FragmentCount    Number of fragments.
  @param[in]      TrafficDirection Traffic direction.
  @param[out]     RecycleSignal    Event for recycling of resources.

  @retval EFI_SUCCESS              The packet was bypassed and all buffers remain the same.
  @retval EFI_SUCCESS              The packet was protected.
  @retval EFI_ACCESS_DENIED        The packet was discarded.

**/
EFI_STATUS
EFIAPI
IpSecProcess (
  IN     EFI_IPSEC2_PROTOCOL             *This,
  IN     EFI_HANDLE                      NicHandle,
  IN     UINT8                           IpVersion,
  IN OUT VOID                            *IpHead,
  IN OUT UINT8                           *LastHead,
  IN OUT VOID                            **OptionsBuffer,
  IN OUT UINT32                          *OptionsLength,
  IN OUT EFI_IPSEC_FRAGMENT_DATA         **FragmentTable,
  IN OUT UINT32                          *FragmentCount,
  IN     EFI_IPSEC_TRAFFIC_DIR           TrafficDirection,
     OUT EFI_EVENT                       *RecycleSignal
  )
{
  IPSEC_PRIVATE_DATA     *Private;
  IPSEC_SPD_ENTRY        *SpdEntry;
  EFI_IPSEC_SPD_SELECTOR *SpdSelector;
  IPSEC_SAD_ENTRY        *SadEntry;
  LIST_ENTRY             *SpdList;
  LIST_ENTRY             *Entry;
  EFI_IPSEC_ACTION       Action;
  EFI_STATUS             Status;
  UINT8                  *IpPayload;
  UINT8                  OldLastHead;
  BOOLEAN                IsOutbound;

  if (OptionsBuffer == NULL || 
      OptionsLength == NULL || 
      FragmentTable == NULL || 
      FragmentCount == NULL
      ) {
    return EFI_INVALID_PARAMETER;
  }
  Private         = IPSEC_PRIVATE_DATA_FROM_IPSEC (This);
  IpPayload       = (*FragmentTable)[0].FragmentBuffer;
  IsOutbound      = (BOOLEAN) ((TrafficDirection == EfiIPsecOutBound) ? TRUE : FALSE);
  OldLastHead     = *LastHead;
  *RecycleSignal  = NULL;
  SpdList         = &mConfigData[IPsecConfigDataTypeSpd];
  
  if (!IsOutbound) {
    //
    // For inbound traffic, process the ipsec header of the packet.
    //
    Status = IpSecProtectInboundPacket (
              IpVersion,
              IpHead,
              LastHead,
              OptionsBuffer,
              OptionsLength,
              FragmentTable,
              FragmentCount,
              &SpdSelector,
              RecycleSignal
              );

    if (Status == EFI_ACCESS_DENIED || Status == EFI_OUT_OF_RESOURCES) {
      //
      // The packet is denied to access.
      //
      goto ON_EXIT;
    }

    if (Status == EFI_SUCCESS) {
      
      //
      // Check the spd entry if the packet is accessible.
      //
      if (SpdSelector == NULL) {
        Status = EFI_ACCESS_DENIED;
        goto ON_EXIT;
      }

      Status =  EFI_ACCESS_DENIED;
      NET_LIST_FOR_EACH (Entry, SpdList) {
        SpdEntry = IPSEC_SPD_ENTRY_FROM_LIST (Entry);
        if (IsSubSpdSelector (               
              (EFI_IPSEC_CONFIG_SELECTOR *) SpdSelector,
              (EFI_IPSEC_CONFIG_SELECTOR *) SpdEntry->Selector
              )) {
          Status = EFI_SUCCESS;
        }
      }      
      goto ON_EXIT;
    }       
  }

  Status  = EFI_ACCESS_DENIED;  

  NET_LIST_FOR_EACH (Entry, SpdList) {
    //
    // For outbound and non-ipsec Inbound traffic: check the spd entry.
    //
    SpdEntry = IPSEC_SPD_ENTRY_FROM_LIST (Entry);

    if (EFI_ERROR (IpSecLookupSpdEntry (
                     SpdEntry,
                     IpVersion,
                     IpHead,
                     IpPayload,
                     OldLastHead,
                     IsOutbound, 
                     &Action
                     ))) {
      //
      // If the related SPD not find
      //
      continue;
    }

    switch (Action) {

    case EfiIPsecActionProtect:

      if (IsOutbound) {
        //
        // For outbound traffic, lookup the sad entry.
        //
        Status = IpSecLookupSadEntry (
                   Private,
                   NicHandle,
                   IpVersion,
                   IpHead,
                   IpPayload,
                   OldLastHead,
                   SpdEntry,
                   &SadEntry
                   );

        if (SadEntry != NULL) {
          //
          // Process the packet by the found sad entry.
          //
          Status = IpSecProtectOutboundPacket (
                    IpVersion,
                    IpHead,
                    LastHead,
                    OptionsBuffer,
                    OptionsLength,
                    FragmentTable,
                    FragmentCount,
                    SadEntry,
                    RecycleSignal
                    );

        } else if (OldLastHead == IP6_ICMP && *IpPayload != ICMP_V6_ECHO_REQUEST) {
          //
          // TODO: if no need return not ready to upper layer, change here.
          //
          Status = EFI_SUCCESS;
        }
      } else if (OldLastHead == IP6_ICMP && *IpPayload != ICMP_V6_ECHO_REQUEST) {
        //
        // For inbound icmpv6 traffic except ping request, accept the packet
        // although no sad entry associated with protect spd entry.
        //
        Status = IpSecLookupSadEntry (
                   Private,
                   NicHandle,
                   IpVersion,
                   IpHead,
                   IpPayload,
                   OldLastHead,
                   SpdEntry,
                   &SadEntry
                   );
        if (SadEntry == NULL) {
          Status = EFI_SUCCESS;
        }
      }

      goto ON_EXIT;

    case EfiIPsecActionBypass:
      Status = EFI_SUCCESS;
      goto ON_EXIT;

    case EfiIPsecActionDiscard:
      goto ON_EXIT;   
    }
  }
   
  //
  // If don't find the related SPD entry, return the EFI_ACCESS_DENIED and discard it.
  // But it the packet is NS/NA, it should be by passed even not find the related SPD entry.
  //
  if (OldLastHead == IP6_ICMP && 
      (*IpPayload == ICMP_V6_NEIGHBOR_SOLICIT || *IpPayload == ICMP_V6_NEIGHBOR_ADVERTISE)
      ){
    Status = EFI_SUCCESS;
  }
  
ON_EXIT:
  return Status;
}

