/** @file
  Dhcp6 support functions implementation.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Dhcp6Impl.h"


/**
  Generate client Duid in the format of Duid-llt.

  @param[in]  Mode          The pointer to the mode of SNP.

  @retval     NULL          If it failed to generate a client Id.
  @retval     others        The pointer to the new client id.

**/
EFI_DHCP6_DUID *
Dhcp6GenerateClientId (
  IN EFI_SIMPLE_NETWORK_MODE   *Mode
  )
{
  EFI_STATUS                Status;
  EFI_DHCP6_DUID            *Duid;
  EFI_TIME                  Time;
  UINT32                    Stamp;
  EFI_GUID                  Uuid;


  //
  // Attempt to get client Id from variable to keep it constant.
  // See details in section-9 of rfc-3315.
  //
  GetVariable2 (L"ClientId", &gEfiDhcp6ServiceBindingProtocolGuid, (VOID**)&Duid, NULL);
  if (Duid != NULL) {
    return Duid;
  }

  //
  //  The format of client identifier option:
  //
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |        OPTION_CLIENTID        |          option-len           |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    .                                                               .
  //    .                              DUID                             .
  //    .                        (variable length)                      .
  //    .                                                               .
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  // If System UUID is found from SMBIOS Table, use DUID-UUID type.
  //
  if ((PcdGet8 (PcdDhcp6UidType) == Dhcp6DuidTypeUuid) && !EFI_ERROR (NetLibGetSystemGuid (&Uuid)) && !CompareGuid (&Uuid, &gZeroGuid)) {
    //
    //
    //  The format of DUID-UUID:
    //
    //    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //   |          DUID-Type (4)        |    UUID (128 bits)            |
    //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
    //   |                                                               |
    //   |                                                               |
    //   |                                -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //   |                                |
    //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

    //
    // sizeof (option-len + Duid-type + UUID-size) = 20 bytes
    //
    Duid = AllocateZeroPool (2 + 2 + sizeof (EFI_GUID));
    if (Duid == NULL) {
      return NULL;
    }

    //
    // sizeof (Duid-type + UUID-size) = 18 bytes
    //
    Duid->Length = (UINT16) (18);

    //
    // Set the Duid-type and copy UUID.
    //
    WriteUnaligned16 ((UINT16 *) (Duid->Duid), HTONS (Dhcp6DuidTypeUuid));

    CopyMem (Duid->Duid + 2, &Uuid, sizeof(EFI_GUID));

  } else {

    //
    //
    //  The format of DUID-LLT:
    //
    //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |          Duid type (1)        |    hardware type (16 bits)    |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |                        time (32 bits)                         |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    .                                                               .
    //    .             link-layer address (variable length)              .
    //    .                                                               .
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //

    //
    // Generate a time stamp of the seconds from 2000/1/1, assume 30day/month.
    //
    gRT->GetTime (&Time, NULL);
    Stamp = (UINT32)
      (
        ((((UINT32)(Time.Year - 2000) * 360 + (Time.Month - 1) * 30 + (Time.Day - 1)) * 24 + Time.Hour) * 60 + Time.Minute) *
        60 +
        Time.Second
      );

    //
    // sizeof (option-len + Duid-type + hardware-type + time) = 10 bytes
    //
    Duid = AllocateZeroPool (10 + Mode->HwAddressSize);
    if (Duid == NULL) {
      return NULL;
    }

    //
    // sizeof (Duid-type + hardware-type + time) = 8 bytes
    //
    Duid->Length = (UINT16) (Mode->HwAddressSize + 8);

    //
    // Set the Duid-type, hardware-type, time and copy the hardware address.
    //
    WriteUnaligned16 ((UINT16 *) ((UINT8 *) Duid + OFFSET_OF (EFI_DHCP6_DUID, Duid)), HTONS (Dhcp6DuidTypeLlt));
    WriteUnaligned16 ((UINT16 *) ((UINT8 *) Duid + OFFSET_OF (EFI_DHCP6_DUID, Duid) + 2), HTONS (NET_IFTYPE_ETHERNET));
    WriteUnaligned32 ((UINT32 *) ((UINT8 *) Duid + OFFSET_OF (EFI_DHCP6_DUID, Duid) + 4), HTONL (Stamp));

    CopyMem (Duid->Duid + 8, &Mode->CurrentAddress, Mode->HwAddressSize);
  }

  Status = gRT->SetVariable (
                  L"ClientId",
                  &gEfiDhcp6ServiceBindingProtocolGuid,
                  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                  Duid->Length + 2,
                  (VOID *) Duid
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Duid);
    return NULL;
  }

  return Duid;
}


/**
  Copy the Dhcp6 configure data.

  @param[in]  DstCfg        The pointer to the destination configure data.
  @param[in]  SorCfg        The pointer to the source configure data.

  @retval EFI_SUCCESS           Copy the content from SorCfg from DstCfg successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.

**/
EFI_STATUS
Dhcp6CopyConfigData (
  IN EFI_DHCP6_CONFIG_DATA      *DstCfg,
  IN EFI_DHCP6_CONFIG_DATA      *SorCfg
  )
{
  UINTN                     Index;
  UINTN                     OptionListSize;
  UINTN                     OptionSize;

  CopyMem (DstCfg, SorCfg, sizeof (EFI_DHCP6_CONFIG_DATA));

  //
  // Allocate another buffer for solicitretransmission, and copy it.
  //
  if (SorCfg->SolicitRetransmission != NULL) {

    DstCfg->SolicitRetransmission = AllocateZeroPool (sizeof (EFI_DHCP6_RETRANSMISSION));

    if (DstCfg->SolicitRetransmission == NULL) {
      //
      // Error will be handled out of this function.
      //
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (
      DstCfg->SolicitRetransmission,
      SorCfg->SolicitRetransmission,
      sizeof (EFI_DHCP6_RETRANSMISSION)
      );
  }

  if (SorCfg->OptionList != NULL && SorCfg->OptionCount != 0) {

    OptionListSize     = SorCfg->OptionCount * sizeof (EFI_DHCP6_PACKET_OPTION *);
    DstCfg->OptionList = AllocateZeroPool (OptionListSize);

    if (DstCfg->OptionList == NULL) {
      //
      // Error will be handled out of this function.
      //
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < SorCfg->OptionCount; Index++) {

      OptionSize                = NTOHS (SorCfg->OptionList[Index]->OpLen) + 4;
      DstCfg->OptionList[Index] = AllocateZeroPool (OptionSize);

      if (DstCfg->OptionList[Index] == NULL) {
        //
        // Error will be handled out of this function.
        //
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (
        DstCfg->OptionList[Index],
        SorCfg->OptionList[Index],
        OptionSize
        );
    }
  }

  return EFI_SUCCESS;
}


/**
  Clean up the configure data.

  @param[in, out]  CfgData       The pointer to the configure data.

**/
VOID
Dhcp6CleanupConfigData (
  IN OUT EFI_DHCP6_CONFIG_DATA       *CfgData
  )
{
  UINTN                          Index;

  ASSERT (CfgData != NULL);
  //
  // Clean up all fields in config data including the reference buffers, but do
  // not free the config data buffer itself.
  //
  if (CfgData->OptionList != NULL) {
    for (Index = 0; Index < CfgData->OptionCount; Index++) {
      if (CfgData->OptionList[Index] != NULL) {
        FreePool (CfgData->OptionList[Index]);
      }
    }
    FreePool (CfgData->OptionList);
  }

  if (CfgData->SolicitRetransmission != NULL) {
    FreePool (CfgData->SolicitRetransmission);
  }

  ZeroMem (CfgData, sizeof (EFI_DHCP6_CONFIG_DATA));
}


/**
  Clean up the mode data.

  @param[in, out]  ModeData      The pointer to the mode data.

**/
VOID
Dhcp6CleanupModeData (
  IN OUT EFI_DHCP6_MODE_DATA        *ModeData
  )
{
  ASSERT (ModeData != NULL);
  //
  // Clean up all fields in mode data including the reference buffers, but do
  // not free the mode data buffer itself.
  //
  if (ModeData->ClientId != NULL) {
    FreePool (ModeData->ClientId);
  }

  if (ModeData->Ia != NULL) {

    if (ModeData->Ia->ReplyPacket != NULL) {
      FreePool (ModeData->Ia->ReplyPacket);
    }
    FreePool (ModeData->Ia);
  }

  ZeroMem (ModeData, sizeof (EFI_DHCP6_MODE_DATA));
}


/**
  Calculate the expire time by the algorithm defined in rfc.

  @param[in]  Base          The base value of the time.
  @param[in]  IsFirstRt     If TRUE, it is the first time to calculate expire time.
  @param[in]  NeedSigned    If TRUE, the signed factor is needed.

  @return     Expire        The calculated result for the new expire time.

**/
UINT32
Dhcp6CalculateExpireTime (
  IN UINT32                 Base,
  IN BOOLEAN                IsFirstRt,
  IN BOOLEAN                NeedSigned
  )
{
  EFI_TIME                  Time;
  BOOLEAN                   Signed;
  UINT32                    Seed;
  UINT32                    Expire;

  //
  // Take the 10bits of microsecond in system time as a uniform distribution.
  // Take the 10th bit as a flag to determine it's signed or not.
  //
  gRT->GetTime (&Time, NULL);
  Seed   = ((Time.Nanosecond >> 10) & DHCP6_10_BIT_MASK);
  Signed = (BOOLEAN) ((((Time.Nanosecond >> 9) & 0x01) != 0) ? TRUE : FALSE);
  Signed = (BOOLEAN) (NeedSigned ? Signed : FALSE);

  //
  // Calculate expire by the following algo:
  //   1. base + base * (-0.1 ~ 0) for the first solicit
  //   2. base + base * (-0.1 ~ 0.1) for the first other messages
  //   3. 2 * base + base * (-0.1 ~ 0.1) for the subsequent all messages
  //   4. base + base * (-0.1 ~ 0) for the more than mrt timeout
  //
  // The (Seed / 0x3ff / 10) is used to a random range (0, 0.1).
  //
  if (IsFirstRt && Signed) {

    Expire = Base - (UINT32) (Base * Seed / DHCP6_10_BIT_MASK / 10);

  } else if (IsFirstRt && !Signed) {

    Expire = Base + (UINT32) (Base * Seed / DHCP6_10_BIT_MASK / 10);

  } else if (!IsFirstRt && Signed) {

    Expire = 2 * Base - (UINT32) (Base * Seed / DHCP6_10_BIT_MASK / 10);

  } else {

    Expire = 2 * Base + (UINT32) (Base * Seed / DHCP6_10_BIT_MASK / 10);
  }

  Expire = (Expire != 0) ? Expire : 1;

  return Expire;
}


/**
  Calculate the lease time by the algorithm defined in rfc.

  @param[in]  IaCb          The pointer to the Ia control block.

**/
VOID
Dhcp6CalculateLeaseTime (
  IN DHCP6_IA_CB              *IaCb
  )
{
  UINT32                      MinLt;
  UINT32                      MaxLt;
  UINTN                       Index;

  ASSERT (IaCb->Ia->IaAddressCount > 0);

  MinLt    = (UINT32) (-1);
  MaxLt    = 0;

  //
  // Calculate minlt as min of all valid life time, and maxlt as max of all
  // valid life time.
  //
  for (Index = 0; Index < IaCb->Ia->IaAddressCount; Index++) {
    MinLt  = MIN (MinLt, IaCb->Ia->IaAddress[Index].ValidLifetime);
    MaxLt  = MAX (MinLt, IaCb->Ia->IaAddress[Index].ValidLifetime);
  }

  //
  // Take 50% minlt as t1, and 80% maxlt as t2 if Dhcp6 server doesn't offer
  // such information.
  //
  IaCb->T1            = (IaCb->T1 != 0) ? IaCb->T1 : (UINT32)(MinLt * 5 / 10);
  IaCb->T2            = (IaCb->T2 != 0) ? IaCb->T2 : (UINT32)(MinLt * 8 / 10);
  IaCb->AllExpireTime = MaxLt;
  IaCb->LeaseTime     = 0;
}


/**
  Check whether the addresses are all included by the configured Ia.

  @param[in]  Ia            The pointer to the Ia.
  @param[in]  AddressCount  The number of addresses.
  @param[in]  Addresses     The pointer to the addresses buffer.

  @retval EFI_SUCCESS         The addresses are all included by the configured IA.
  @retval EFI_NOT_FOUND       The addresses are not included by the configured IA.

**/
EFI_STATUS
Dhcp6CheckAddress (
  IN EFI_DHCP6_IA             *Ia,
  IN UINT32                   AddressCount,
  IN EFI_IPv6_ADDRESS         *Addresses
  )
{
  UINTN                       Index1;
  UINTN                       Index2;
  BOOLEAN                     Found;

  //
  // Check whether the addresses are all included by the configured IA. And it
  // will return success if address count is zero, which means all addresses.
  //
  for (Index1 = 0; Index1 < AddressCount; Index1++) {

    Found = FALSE;

    for (Index2 = 0; Index2 < Ia->IaAddressCount; Index2++) {

      if (CompareMem (
            &Addresses[Index1],
            &Ia->IaAddress[Index2],
            sizeof (EFI_IPv6_ADDRESS)
            ) == 0) {

        Found = TRUE;
        break;
      }
    }

    if (!Found) {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}


/**
  Deprive the addresses from current Ia, and generate another eliminated Ia.

  @param[in]  Ia            The pointer to the Ia.
  @param[in]  AddressCount  The number of addresses.
  @param[in]  Addresses     The pointer to the addresses buffer.

  @retval     NULL          If it failed to generate the deprived Ia.
  @retval     others        The pointer to the deprived Ia.

**/
EFI_DHCP6_IA *
Dhcp6DepriveAddress (
  IN EFI_DHCP6_IA             *Ia,
  IN UINT32                   AddressCount,
  IN EFI_IPv6_ADDRESS         *Addresses
  )
{
  EFI_DHCP6_IA                *IaCopy;
  UINTN                       IaCopySize;
  UINTN                       Index1;
  UINTN                       Index2;
  BOOLEAN                     Found;

  if (AddressCount == 0) {
    //
    // It means release all Ia addresses if address count is zero.
    //
    AddressCount = Ia->IaAddressCount;
  }

  ASSERT (AddressCount != 0);

  IaCopySize = sizeof (EFI_DHCP6_IA) + (AddressCount - 1) * sizeof (EFI_DHCP6_IA_ADDRESS);
  IaCopy     = AllocateZeroPool (IaCopySize);

  if (IaCopy == NULL) {
    return NULL;
  }

  if (AddressCount == Ia->IaAddressCount) {
    //
    // If release all Ia addresses, just copy the configured Ia and then set
    // its address count as zero.
    // We may decline/release part of addresses at the beginning. So it's a
    // forwarding step to update address infor for decline/release, while the
    // other infor such as Ia state will be updated when receiving reply.
    //
    CopyMem (IaCopy, Ia, IaCopySize);
    Ia->IaAddressCount = 0;
    return IaCopy;
  }

  CopyMem (IaCopy, Ia, sizeof (EFI_DHCP6_IA));

  //
  // Move the addresses from the Ia of instance to the deprived Ia.
  //
  for (Index1 = 0; Index1 < AddressCount; Index1++) {

    Found = FALSE;

    for (Index2 = 0; Index2 < Ia->IaAddressCount; Index2++) {

      if (CompareMem (
            &Addresses[Index1],
            &Ia->IaAddress[Index2],
            sizeof (EFI_IPv6_ADDRESS)
            ) == 0) {
        //
        // Copy the deprived address to the copy of Ia
        //
        CopyMem (
          &IaCopy->IaAddress[Index1],
          &Ia->IaAddress[Index2],
          sizeof (EFI_DHCP6_IA_ADDRESS)
          );
        //
        // Delete the deprived address from the instance Ia
        //
        if (Index2 + 1 < Ia->IaAddressCount) {
          CopyMem (
            &Ia->IaAddress[Index2],
            &Ia->IaAddress[Index2 + 1],
            (Ia->IaAddressCount - Index2 - 1) * sizeof (EFI_DHCP6_IA_ADDRESS)
            );
        }
        Found = TRUE;
        break;
      }
    }
    ASSERT (Found == TRUE);
  }

  Ia->IaAddressCount    -= AddressCount;
  IaCopy->IaAddressCount = AddressCount;

  return IaCopy;
}


/**
  The dummy ext buffer free callback routine.

  @param[in]  Arg           The pointer to the parameter.

**/
VOID
EFIAPI
Dhcp6DummyExtFree (
  IN VOID                      *Arg
  )
{
}


/**
  The callback routine once message transmitted.

  @param[in]  Wrap          The pointer to the received net buffer.
  @param[in]  EndPoint      The pointer to the udp end point.
  @param[in]  IoStatus      The return status from udp io.
  @param[in]  Context       The opaque parameter to the function.

**/
VOID
EFIAPI
Dhcp6OnTransmitted (
  IN NET_BUF                   *Wrap,
  IN UDP_END_POINT             *EndPoint,
  IN EFI_STATUS                IoStatus,
  IN VOID                      *Context
  )
{
  NetbufFree (Wrap);
}


/**
  Append the option to Buf, and move Buf to the end.

  @param[in, out] Buf           The pointer to the buffer.
  @param[in]      OptType       The option type.
  @param[in]      OptLen        The length of option contents.
  @param[in]      Data          The pointer to the option content.

  @return         Buf           The position to append the next option.

**/
UINT8 *
Dhcp6AppendOption (
  IN OUT UINT8               *Buf,
  IN     UINT16              OptType,
  IN     UINT16              OptLen,
  IN     UINT8               *Data
  )
{
  //
  //  The format of Dhcp6 option:
  //
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |          option-code          |   option-len (option data)    |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                          option-data                          |
  //    |                      (option-len octets)                      |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  ASSERT (OptLen != 0);

  WriteUnaligned16 ((UINT16 *) Buf, OptType);
  Buf            += 2;
  WriteUnaligned16 ((UINT16 *) Buf, OptLen);
  Buf            += 2;
  CopyMem (Buf, Data, NTOHS (OptLen));
  Buf            += NTOHS (OptLen);

  return Buf;
}

/**
  Append the appointed IA Address option to Buf, and move Buf to the end.

  @param[in, out] Buf           The pointer to the position to append.
  @param[in]      IaAddr        The pointer to the IA Address.
  @param[in]      MessageType   Message type of DHCP6 package.

  @return         Buf           The position to append the next option.

**/
UINT8 *
Dhcp6AppendIaAddrOption (
  IN OUT UINT8                  *Buf,
  IN     EFI_DHCP6_IA_ADDRESS   *IaAddr,
  IN     UINT32                 MessageType
)
{

  //  The format of the IA Address option is:
  //
  //       0                   1                   2                   3
  //       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //      |          OPTION_IAADDR        |          option-len           |
  //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //      |                                                               |
  //      |                         IPv6 address                          |
  //      |                                                               |
  //      |                                                               |
  //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //      |                      preferred-lifetime                       |
  //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //      |                        valid-lifetime                         |
  //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //      .                                                               .
  //      .                        IAaddr-options                         .
  //      .                                                               .
  //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  //
  // Fill the value of Ia Address option type
  //
  WriteUnaligned16 ((UINT16 *) Buf, HTONS (Dhcp6OptIaAddr));
  Buf                     += 2;

  WriteUnaligned16 ((UINT16 *) Buf, HTONS (sizeof (EFI_DHCP6_IA_ADDRESS)));
  Buf                     += 2;

  CopyMem (Buf, &IaAddr->IpAddress, sizeof(EFI_IPv6_ADDRESS));
  Buf                     += sizeof(EFI_IPv6_ADDRESS);

  //
  // Fill the value of preferred-lifetime and valid-lifetime.
  // According to RFC3315 Chapter 18.1.2, the preferred-lifetime and valid-lifetime fields
  // should set to 0 when initiate a Confirm message.
  //
  if (MessageType != Dhcp6MsgConfirm) {
    WriteUnaligned32 ((UINT32 *) Buf, HTONL (IaAddr->PreferredLifetime));
  }
  Buf                     += 4;

  if (MessageType != Dhcp6MsgConfirm) {
    WriteUnaligned32 ((UINT32 *) Buf, HTONL (IaAddr->ValidLifetime));
  }
  Buf                     += 4;

  return Buf;
}


/**
  Append the appointed Ia option to Buf, and move Buf to the end.

  @param[in, out] Buf           The pointer to the position to append.
  @param[in]      Ia            The pointer to the Ia.
  @param[in]      T1            The time of T1.
  @param[in]      T2            The time of T2.
  @param[in]      MessageType   Message type of DHCP6 package.

  @return         Buf           The position to append the next Ia option.

**/
UINT8 *
Dhcp6AppendIaOption (
  IN OUT UINT8                  *Buf,
  IN     EFI_DHCP6_IA           *Ia,
  IN     UINT32                 T1,
  IN     UINT32                 T2,
  IN     UINT32                 MessageType
  )
{
  UINT8                     *AddrOpt;
  UINT16                    *Len;
  UINTN                     Index;

  //
  //  The format of IA_NA and IA_TA option:
  //
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |          OPTION_IA_NA         |          option-len           |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                        IAID (4 octets)                        |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                        T1 (only for IA_NA)                    |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                        T2 (only for IA_NA)                    |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                                                               |
  //    .                  IA_NA-options/IA_TA-options                  .
  //    .                                                               .
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  // Fill the value of Ia option type
  //
  WriteUnaligned16 ((UINT16 *) Buf, HTONS (Ia->Descriptor.Type));
  Buf                     += 2;

  //
  // Fill the len of Ia option later, keep the pointer first
  //
  Len                      = (UINT16 *) Buf;
  Buf                     += 2;

  //
  // Fill the value of iaid
  //
  WriteUnaligned32 ((UINT32 *) Buf, HTONL (Ia->Descriptor.IaId));
  Buf                     += 4;

  //
  // Fill the value of t1 and t2 if iana, keep it 0xffffffff if no specified.
  //
  if (Ia->Descriptor.Type == Dhcp6OptIana) {
    WriteUnaligned32 ((UINT32 *) Buf, HTONL ((T1 != 0) ? T1 : 0xffffffff));
    Buf                   += 4;
    WriteUnaligned32 ((UINT32 *) Buf, HTONL ((T2 != 0) ? T2 : 0xffffffff));
    Buf                   += 4;
  }

  //
  // Fill all the addresses belong to the Ia
  //
  for (Index = 0; Index < Ia->IaAddressCount; Index++) {
    AddrOpt = (UINT8 *) Ia->IaAddress + Index * sizeof (EFI_DHCP6_IA_ADDRESS);
    Buf = Dhcp6AppendIaAddrOption (Buf, (EFI_DHCP6_IA_ADDRESS *) AddrOpt, MessageType);
  }

  //
  // Fill the value of Ia option length
  //
  *Len = HTONS ((UINT16) (Buf - (UINT8 *) Len - 2));

  return Buf;
}

/**
  Append the appointed Elapsed time option to Buf, and move Buf to the end.

  @param[in, out] Buf           The pointer to the position to append.
  @param[in]      Instance      The pointer to the Dhcp6 instance.
  @param[out]     Elapsed       The pointer to the elapsed time value in
                                  the generated packet.

  @return         Buf           The position to append the next Ia option.

**/
UINT8 *
Dhcp6AppendETOption (
  IN OUT UINT8                  *Buf,
  IN     DHCP6_INSTANCE         *Instance,
  OUT    UINT16                 **Elapsed
  )
{
  //
  //  The format of elapsed time option:
  //
  //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  |      OPTION_ELAPSED_TIME      |           option-len          |
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //  |          elapsed-time         |
  //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  // Fill the value of elapsed-time option type.
  //
  WriteUnaligned16 ((UINT16 *) Buf, HTONS (Dhcp6OptElapsedTime));
  Buf                     += 2;

  //
  // Fill the len of elapsed-time option, which is fixed.
  //
  WriteUnaligned16 ((UINT16 *) Buf, HTONS(2));
  Buf                     += 2;

  //
  // Fill in elapsed time value with 0 value for now.  The actual value is
  // filled in later just before the packet is transmitted.
  //
  WriteUnaligned16 ((UINT16 *) Buf, HTONS(0));
  *Elapsed                  = (UINT16 *) Buf;
  Buf                     += 2;

  return Buf;
}

/**
  Set the elapsed time based on the given instance and the pointer to the
  elapsed time option.

  @param[in]      Elapsed       The pointer to the position to append.
  @param[in]      Instance      The pointer to the Dhcp6 instance.

**/
VOID
SetElapsedTime (
  IN     UINT16                 *Elapsed,
  IN     DHCP6_INSTANCE         *Instance
  )
{
  EFI_TIME          Time;
  UINT64            CurrentStamp;
  UINT64            ElapsedTimeValue;

  //
  // Generate a time stamp of the centiseconds from 2000/1/1, assume 30day/month.
  //
  gRT->GetTime (&Time, NULL);
  CurrentStamp = MultU64x32 (
                   ((((UINT32)(Time.Year - 2000) * 360 + (Time.Month - 1) * 30 + (Time.Day - 1)) * 24 + Time.Hour) * 60 + Time.Minute) * 60 + Time.Second,
                   100
                   ) +
                 DivU64x32(
                   Time.Nanosecond,
                   10000000
                   );

  //
  // Sentinel value of 0 means that this is the first DHCP packet that we are
  // sending and that we need to initialize the value.  First DHCP message
  // gets 0 elapsed-time.  Otherwise, calculate based on StartTime.
  //
  if (Instance->StartTime == 0) {
    ElapsedTimeValue = 0;
    Instance->StartTime = CurrentStamp;
  } else {
    ElapsedTimeValue = CurrentStamp - Instance->StartTime;

    //
    // If elapsed time cannot fit in two bytes, set it to 0xffff.
    //
    if (ElapsedTimeValue > 0xffff) {
      ElapsedTimeValue = 0xffff;
    }
  }
  WriteUnaligned16 (Elapsed, HTONS((UINT16) ElapsedTimeValue));
}


/**
  Seek the address of the first byte of the option header.

  @param[in]  Buf           The pointer to the buffer.
  @param[in]  SeekLen       The length to seek.
  @param[in]  OptType       The option type.

  @retval     NULL          If it failed to seek the option.
  @retval     others        The position to the option.

**/
UINT8 *
Dhcp6SeekOption (
  IN UINT8           *Buf,
  IN UINT32          SeekLen,
  IN UINT16          OptType
  )
{
  UINT8              *Cursor;
  UINT8              *Option;
  UINT16             DataLen;
  UINT16             OpCode;

  Option = NULL;
  Cursor = Buf;

  //
  // The format of Dhcp6 option refers to Dhcp6AppendOption().
  //
  while (Cursor < Buf + SeekLen) {
    OpCode = ReadUnaligned16 ((UINT16 *) Cursor);
    if (OpCode == HTONS (OptType)) {
      Option = Cursor;
      break;
    }
    DataLen = NTOHS (ReadUnaligned16 ((UINT16 *) (Cursor + 2)));
    Cursor += (DataLen + 4);
  }

  return Option;
}


/**
  Seek the address of the first byte of the Ia option header.

  @param[in]  Buf           The pointer to the buffer.
  @param[in]  SeekLen       The length to seek.
  @param[in]  IaDesc        The pointer to the Ia descriptor.

  @retval     NULL          If it failed to seek the Ia option.
  @retval     others        The position to the Ia option.

**/
UINT8 *
Dhcp6SeekIaOption (
  IN UINT8                    *Buf,
  IN UINT32                   SeekLen,
  IN EFI_DHCP6_IA_DESCRIPTOR  *IaDesc
  )
{
  UINT8              *Cursor;
  UINT8              *Option;
  UINT16             DataLen;
  UINT16             OpCode;
  UINT32             IaId;

  //
  // The format of IA_NA and IA_TA option refers to Dhcp6AppendIaOption().
  //
  Option = NULL;
  Cursor = Buf;

  while (Cursor < Buf + SeekLen) {
    OpCode = ReadUnaligned16 ((UINT16 *) Cursor);
    IaId   = ReadUnaligned32 ((UINT32 *) (Cursor + 4));
    if (OpCode == HTONS (IaDesc->Type) && IaId == HTONL (IaDesc->IaId)) {
      Option = Cursor;
      break;
    }
    DataLen = NTOHS (ReadUnaligned16 ((UINT16 *) (Cursor + 2)));
    Cursor += (DataLen + 4);
  }

  return Option;
}

/**
  Check whether the incoming IPv6 address in IaAddr is one of the maintained
  addresses in the IA control block.

  @param[in]  IaAddr            The pointer to the IA Address to be checked.
  @param[in]  CurrentIa         The pointer to the IA in IA control block.

  @retval     TRUE              Yes, this Address is already in IA control block.
  @retval     FALSE             No, this Address is NOT in IA control block.

**/
BOOLEAN
Dhcp6AddrIsInCurrentIa (
  IN    EFI_DHCP6_IA_ADDRESS      *IaAddr,
  IN    EFI_DHCP6_IA              *CurrentIa
  )
{
  UINT32    Index;

  ASSERT (IaAddr != NULL && CurrentIa != NULL);

  for (Index = 0; Index < CurrentIa->IaAddressCount; Index++) {
    if (EFI_IP6_EQUAL(&IaAddr->IpAddress, &CurrentIa->IaAddress[Index].IpAddress)) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Parse the address option and update the address information.

  @param[in]      CurrentIa     The pointer to the Ia Address in control block.
  @param[in]      IaInnerOpt    The pointer to the buffer.
  @param[in]      IaInnerLen    The length to parse.
  @param[out]     AddrNum       The number of addresses.
  @param[in, out] AddrBuf       The pointer to the address buffer.

**/
VOID
Dhcp6ParseAddrOption (
  IN     EFI_DHCP6_IA            *CurrentIa,
  IN     UINT8                   *IaInnerOpt,
  IN     UINT16                  IaInnerLen,
     OUT UINT32                  *AddrNum,
  IN OUT EFI_DHCP6_IA_ADDRESS    *AddrBuf
  )
{
  UINT8                       *Cursor;
  UINT16                      DataLen;
  UINT16                      OpCode;
  UINT32                      ValidLt;
  UINT32                      PreferredLt;
  EFI_DHCP6_IA_ADDRESS        *IaAddr;

  //
  //  The format of the IA Address option:
  //
  //     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |          OPTION_IAADDR        |          option-len           |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                                                               |
  //    |                         IPv6 address                          |
  //    |                                                               |
  //    |                                                               |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                      preferred-lifetime                       |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    |                        valid-lifetime                         |
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //    .                                                               .
  //    .                        IAaddr-options                         .
  //    .                                                               .
  //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //

  //
  //  Two usage model:
  //
  //    1. Pass addrbuf == null, to get the addrnum over the Ia inner options.
  //    2. Pass addrbuf != null, to resolve the addresses over the Ia inner
  //       options to the addrbuf.
  //

  Cursor   = IaInnerOpt;
  *AddrNum = 0;

  while (Cursor < IaInnerOpt + IaInnerLen) {
    //
    // Refer to RFC3315 Chapter 18.1.8, we need to update lifetimes for any addresses in the IA option
    // that the client already has recorded in the IA, and discard the Ia address option with 0 valid time.
    //
    OpCode  = ReadUnaligned16 ((UINT16 *) Cursor);
    PreferredLt = NTOHL (ReadUnaligned32 ((UINT32 *) (Cursor + 20)));
    ValidLt = NTOHL (ReadUnaligned32 ((UINT32 *) (Cursor + 24)));
    IaAddr = (EFI_DHCP6_IA_ADDRESS *) (Cursor + 4);
    if (OpCode == HTONS (Dhcp6OptIaAddr) && ValidLt >= PreferredLt &&
        (Dhcp6AddrIsInCurrentIa(IaAddr, CurrentIa) || ValidLt !=0)) {
      if (AddrBuf != NULL) {
        CopyMem (AddrBuf, IaAddr, sizeof (EFI_DHCP6_IA_ADDRESS));
        AddrBuf->PreferredLifetime = PreferredLt;
        AddrBuf->ValidLifetime     = ValidLt;
        AddrBuf = (EFI_DHCP6_IA_ADDRESS *) ((UINT8 *) AddrBuf + sizeof (EFI_DHCP6_IA_ADDRESS));
      }
      (*AddrNum)++;
    }
    DataLen = NTOHS (ReadUnaligned16 ((UINT16 *) (Cursor + 2)));
    Cursor += (DataLen + 4);
  }
}


/**
  Create a control block for the Ia according to the corresponding options.

  @param[in]  Instance              The pointer to DHCP6 Instance.
  @param[in]  IaInnerOpt            The pointer to the inner options in the Ia option.
  @param[in]  IaInnerLen            The length of all the inner options in the Ia option.
  @param[in]  T1                    T1 time in the Ia option.
  @param[in]  T2                    T2 time in the Ia option.

  @retval     EFI_NOT_FOUND         No valid IA option is found.
  @retval     EFI_SUCCESS           Create an IA control block successfully.
  @retval     EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval     EFI_DEVICE_ERROR      An unexpected error.

**/
EFI_STATUS
Dhcp6GenerateIaCb (
  IN  DHCP6_INSTANCE           *Instance,
  IN  UINT8                    *IaInnerOpt,
  IN  UINT16                   IaInnerLen,
  IN  UINT32                   T1,
  IN  UINT32                   T2
  )
{
  UINT32                       AddrNum;
  UINT32                       IaSize;
  EFI_DHCP6_IA                 *Ia;

  if (Instance->IaCb.Ia == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Calculate the number of addresses for this Ia, excluding the addresses with
  // the value 0 of valid lifetime.
  //
  Dhcp6ParseAddrOption (Instance->IaCb.Ia, IaInnerOpt, IaInnerLen, &AddrNum, NULL);

  if (AddrNum == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Allocate for new IA.
  //
  IaSize = sizeof (EFI_DHCP6_IA) + (AddrNum - 1) * sizeof (EFI_DHCP6_IA_ADDRESS);
  Ia = AllocateZeroPool (IaSize);

  if (Ia == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill up this new IA fields.
  //
  Ia->State          = Instance->IaCb.Ia->State;
  Ia->IaAddressCount = AddrNum;
  CopyMem (&Ia->Descriptor, &Instance->Config->IaDescriptor, sizeof (EFI_DHCP6_IA_DESCRIPTOR));
  Dhcp6ParseAddrOption (Instance->IaCb.Ia, IaInnerOpt, IaInnerLen, &AddrNum, Ia->IaAddress);

  //
  // Free original IA resource.
  //
  if (Instance->IaCb.Ia->ReplyPacket != NULL) {
    FreePool (Instance->IaCb.Ia->ReplyPacket);
  }
  FreePool (Instance->IaCb.Ia);


  ZeroMem (&Instance->IaCb, sizeof (DHCP6_IA_CB));

  //
  // Update IaCb to use new IA.
  //
  Instance->IaCb.Ia   = Ia;

  //

 // Fill in IaCb fields. Such as T1, T2, AllExpireTime and LeaseTime.
  //
  Instance->IaCb.T1 = T1;
  Instance->IaCb.T2 = T2;
  Dhcp6CalculateLeaseTime (&Instance->IaCb);

  return EFI_SUCCESS;
}


/**
  Cache the current IA configuration information.

  @param[in] Instance           The pointer to DHCP6 Instance.

  @retval EFI_SUCCESS           Cache the current IA successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.

**/
EFI_STATUS
Dhcp6CacheIa (
  IN DHCP6_INSTANCE           *Instance
  )
{
  UINTN                        IaSize;
  EFI_DHCP6_IA                 *Ia;

  Ia = Instance->IaCb.Ia;

  if ((Instance->CacheIa == NULL) && (Ia != NULL)) {
    //
    // Cache the current IA.
    //
    IaSize = sizeof (EFI_DHCP6_IA) + (Ia->IaAddressCount - 1) * sizeof (EFI_DHCP6_IA_ADDRESS);

    Instance->CacheIa = AllocateZeroPool (IaSize);
    if (Instance->CacheIa == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (Instance->CacheIa, Ia, IaSize);
  }
  return EFI_SUCCESS;
}

/**
  Append CacheIa to the current IA. Meanwhile, clear CacheIa.ValidLifetime to 0.

  @param[in]  Instance            The pointer to DHCP6 instance.

**/
VOID
Dhcp6AppendCacheIa (
  IN DHCP6_INSTANCE           *Instance
  )
{
  UINT8                        *Ptr;
  UINTN                        Index;
  UINTN                        IaSize;
  UINTN                        NewIaSize;
  EFI_DHCP6_IA                 *Ia;
  EFI_DHCP6_IA                 *NewIa;
  EFI_DHCP6_IA                 *CacheIa;

  Ia      = Instance->IaCb.Ia;
  CacheIa = Instance->CacheIa;

  if ((CacheIa != NULL) && (CacheIa->IaAddressCount != 0)) {
    //
    // There are old addresses existing. Merge with current addresses.
    //
    NewIaSize = sizeof (EFI_DHCP6_IA) + (Ia->IaAddressCount + CacheIa->IaAddressCount - 1) * sizeof (EFI_DHCP6_IA_ADDRESS);
    NewIa     = AllocateZeroPool (NewIaSize);
    if (NewIa == NULL) {
      return;
    }

    IaSize = sizeof (EFI_DHCP6_IA) + (Ia->IaAddressCount - 1) * sizeof (EFI_DHCP6_IA_ADDRESS);
    CopyMem (NewIa, Ia, IaSize);

    //
    // Clear old address.ValidLifetime
    //
    for (Index = 0; Index < CacheIa->IaAddressCount; Index++) {
      CacheIa->IaAddress[Index].ValidLifetime  = 0;
    }

    NewIa->IaAddressCount += CacheIa->IaAddressCount;
    Ptr   = (UINT8*)&NewIa->IaAddress[Ia->IaAddressCount];
    CopyMem (Ptr, CacheIa->IaAddress, CacheIa->IaAddressCount * sizeof (EFI_DHCP6_IA_ADDRESS));

    //
    // Migrate to the NewIa and free previous.
    //
    FreePool (Instance->CacheIa);
    FreePool (Instance->IaCb.Ia);
    Instance->CacheIa  = NULL;
    Instance->IaCb.Ia  = NewIa;
  }
}

/**
  Calculate the Dhcp6 get mapping timeout by adding additional delay to the IP6 DAD transmits count.

  @param[in]   Ip6Cfg              The pointer to Ip6 config protocol.
  @param[out]  TimeOut             The time out value in 100ns units.

  @retval   EFI_INVALID_PARAMETER  Input parameters are invalid.
  @retval   EFI_SUCCESS            Calculate the time out value successfully.
**/
EFI_STATUS
Dhcp6GetMappingTimeOut (
  IN  EFI_IP6_CONFIG_PROTOCOL       *Ip6Cfg,
  OUT UINTN                         *TimeOut
  )
{
  EFI_STATUS            Status;
  UINTN                 DataSize;
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS    DadXmits;

  if (Ip6Cfg == NULL || TimeOut == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DataSize = sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS);
  Status = Ip6Cfg->GetData (
                     Ip6Cfg,
                     Ip6ConfigDataTypeDupAddrDetectTransmits,
                     &DataSize,
                     &DadXmits
                     );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *TimeOut = TICKS_PER_SECOND * DadXmits.DupAddrDetectTransmits + DHCP6_DAD_ADDITIONAL_DELAY;

  return EFI_SUCCESS;
}
