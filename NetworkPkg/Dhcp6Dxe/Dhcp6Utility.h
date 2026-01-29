/** @file
  Dhcp6 support functions declaration.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DHCP6_UTILITY_H__
#define __EFI_DHCP6_UTILITY_H__

#define  DHCP6_10_BIT_MASK           0x3ff
#define  DHCP6_DAD_ADDITIONAL_DELAY  30000000   // 3 seconds

/**
  Generate client Duid in the format of Duid-llt.

  @param[in]  Mode          The pointer to the mode of SNP.

  @retval     NULL          if failed to generate client Id.
  @retval     Others        The pointer to the new client id.

**/
EFI_DHCP6_DUID *
Dhcp6GenerateClientId (
  IN EFI_SIMPLE_NETWORK_MODE  *Mode
  );

/**
  Copy the Dhcp6 configure data.

  @param[in]  DstCfg        The pointer to the destination configure data.
  @param[in]  SorCfg        The pointer to the source configure data.

  @retval EFI_SUCCESS           Copy the content from SorCfg from DstCfg successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.

**/
EFI_STATUS
Dhcp6CopyConfigData (
  IN EFI_DHCP6_CONFIG_DATA  *DstCfg,
  IN EFI_DHCP6_CONFIG_DATA  *SorCfg
  );

/**
  Clean up the configure data.

  @param[in, out]  CfgData       The pointer to the configure data.

**/
VOID
Dhcp6CleanupConfigData (
  IN OUT EFI_DHCP6_CONFIG_DATA  *CfgData
  );

/**
  Clean up the mode data.

  @param[in, out]  ModeData      The pointer to the mode data.

**/
VOID
Dhcp6CleanupModeData (
  IN OUT EFI_DHCP6_MODE_DATA  *ModeData
  );

/**
  Calculate the expire time by the algorithm defined in rfc.

  @param[in]  Base          The base value of the time.
  @param[in]  IsFirstRt     If TRUE, it is the first time to calculate expire time.
  @param[in]  NeedSigned    If TRUE, the signed factor is needed.

  @return     Expire        The calculated result for the new expire time.

**/
UINT32
Dhcp6CalculateExpireTime (
  IN UINT32   Base,
  IN BOOLEAN  IsFirstRt,
  IN BOOLEAN  NeedSigned
  );

/**
  Calculate the lease time by the algorithm defined in rfc.

  @param[in]  IaCb          The pointer to the Ia control block.

**/
VOID
Dhcp6CalculateLeaseTime (
  IN DHCP6_IA_CB  *IaCb
  );

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
  IN EFI_DHCP6_IA      *Ia,
  IN UINT32            AddressCount,
  IN EFI_IPv6_ADDRESS  *Addresses
  );

/**
  Deprive the addresses from current Ia, and generate another eliminated Ia.

  @param[in]  Ia            The pointer to the Ia.
  @param[in]  AddressCount  The number of addresses.
  @param[in]  Addresses     The pointer to the addresses buffer.

  @retval     NULL          If failed to generate the deprived Ia.
  @retval     others        The pointer to the deprived Ia.

**/
EFI_DHCP6_IA *
Dhcp6DepriveAddress (
  IN EFI_DHCP6_IA      *Ia,
  IN UINT32            AddressCount,
  IN EFI_IPv6_ADDRESS  *Addresses
  );

/**
  The dummy ext buffer free callback routine.

  @param[in]  Arg           The pointer to the parameter.

**/
VOID
EFIAPI
Dhcp6DummyExtFree (
  IN VOID  *Arg
  );

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
  IN NET_BUF        *Wrap,
  IN UDP_END_POINT  *EndPoint,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  );

/**
  Append the option to Buf, update the length of packet, and move Buf to the end.

  @param[in, out] Packet         A pointer to the packet, on success Packet->Length
                                 will be updated.
  @param[in, out] PacketCursor   The pointer in the packet, on success PacketCursor
                                 will be moved to the end of the option.
  @param[in]      OptType        The option type.
  @param[in]      OptLen         The length of option contents.
  @param[in]      Data           The pointer to the option content.

  @retval   EFI_INVALID_PARAMETER An argument provided to the function was invalid
  @retval   EFI_BUFFER_TOO_SMALL  The buffer is too small to append the option.
  @retval   EFI_SUCCESS           The option is appended successfully.
**/
EFI_STATUS
Dhcp6AppendOption (
  IN OUT EFI_DHCP6_PACKET  *Packet,
  IN OUT UINT8             **PacketCursor,
  IN     UINT16            OptType,
  IN     UINT16            OptLen,
  IN     UINT8             *Data
  );

/**
  Append the appointed Ia option to Buf, update the Ia option length, and move Buf
  to the end of the option.
  @param[in, out] Packet        A pointer to the packet, on success Packet->Length
                                will be updated.
  @param[in, out] PacketCursor   The pointer in the packet, on success PacketCursor
                                 will be moved to the end of the option.
  @param[in]      Ia            The pointer to the Ia.
  @param[in]      T1            The time of T1.
  @param[in]      T2            The time of T2.
  @param[in]      MessageType   Message type of DHCP6 package.

  @retval   EFI_INVALID_PARAMETER An argument provided to the function was invalid
  @retval   EFI_BUFFER_TOO_SMALL  The buffer is too small to append the option.
  @retval   EFI_SUCCESS           The option is appended successfully.
**/
EFI_STATUS
Dhcp6AppendIaOption (
  IN OUT EFI_DHCP6_PACKET  *Packet,
  IN OUT UINT8             **PacketCursor,
  IN     EFI_DHCP6_IA      *Ia,
  IN     UINT32            T1,
  IN     UINT32            T2,
  IN     UINT32            MessageType
  );

/**
  Append the appointed Elapsed time option to Buf, and move Buf to the end.

  @param[in, out] Packet        A pointer to the packet, on success Packet->Length
  @param[in, out] PacketCursor   The pointer in the packet, on success PacketCursor
                                 will be moved to the end of the option.
  @param[in]      Instance      The pointer to the Dhcp6 instance.
  @param[out]     Elapsed       The pointer to the elapsed time value in
                                  the generated packet.

  @retval   EFI_INVALID_PARAMETER An argument provided to the function was invalid
  @retval   EFI_BUFFER_TOO_SMALL  The buffer is too small to append the option.
  @retval   EFI_SUCCESS           The option is appended successfully.

**/
EFI_STATUS
Dhcp6AppendETOption (
  IN OUT EFI_DHCP6_PACKET  *Packet,
  IN OUT UINT8             **PacketCursor,
  IN     DHCP6_INSTANCE    *Instance,
  OUT    UINT16            **Elapsed
  );

/**
  Set the elapsed time based on the given instance and the pointer to the
  elapsed time option.

  @retval   EFI_INVALID_PARAMETER An argument provided to the function was invalid
  @retval   EFI_BUFFER_TOO_SMALL  The buffer is too small to append the option.
  @retval   EFI_SUCCESS           The option is appended successfully.
**/
VOID
SetElapsedTime (
  IN     UINT16          *Elapsed,
  IN     DHCP6_INSTANCE  *Instance
  );

/**
  Seek the address of the first byte of the option header.

  @param[in]  Buf           The pointer to buffer.
  @param[in]  SeekLen       The length to seek.
  @param[in]  OptType       The option type.

  @retval     NULL          If failed to seek the option.
  @retval     others        The position to the option.

**/
UINT8 *
Dhcp6SeekOption (
  IN UINT8   *Buf,
  IN UINT32  SeekLen,
  IN UINT16  OptType
  );

/**
  Seek the address of the first byte of the Ia option header.

  @param[in]  Buf           The pointer to the buffer.
  @param[in]  SeekLen       The length to seek.
  @param[in]  IaDesc        The pointer to the Ia descriptor.

  @retval     NULL          If failed to seek the Ia option.
  @retval     others        The position to the Ia option.

**/
UINT8 *
Dhcp6SeekIaOption (
  IN UINT8                    *Buf,
  IN UINT32                   SeekLen,
  IN EFI_DHCP6_IA_DESCRIPTOR  *IaDesc
  );

/**
  Parse the address option and update the address info.

  @param[in]      CurrentIa     The pointer to the Ia Address in control block.
  @param[in]      IaInnerOpt    The pointer to the buffer.
  @param[in]      IaInnerLen    The length to parse.
  @param[out]     AddrNum       The number of addresses.
  @param[in, out] AddrBuf       The pointer to the address buffer.

**/
VOID
Dhcp6ParseAddrOption (
  IN     EFI_DHCP6_IA          *CurrentIa,
  IN     UINT8                 *IaInnerOpt,
  IN     UINT16                IaInnerLen,
  OUT UINT32                   *AddrNum,
  IN OUT EFI_DHCP6_IA_ADDRESS  *AddrBuf
  );

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
  IN  DHCP6_INSTANCE  *Instance,
  IN  UINT8           *IaInnerOpt,
  IN  UINT16          IaInnerLen,
  IN  UINT32          T1,
  IN  UINT32          T2
  );

/**
  Cache the current IA configuration information.

  @param[in] Instance           The pointer to DHCP6 Instance.

  @retval EFI_SUCCESS           Cache the current IA successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.

**/
EFI_STATUS
Dhcp6CacheIa (
  IN DHCP6_INSTANCE  *Instance
  );

/**
  Append CacheIa to the current IA. Meanwhile, clear CacheIa.ValidLifetime to 0.

  @param[in]  Instance            The pointer to DHCP6 instance.

**/
VOID
Dhcp6AppendCacheIa (
  IN DHCP6_INSTANCE  *Instance
  );

/**
  Calculate the Dhcp6 get mapping timeout by adding additional delay to the IP6 DAD transmits count.

  @param[in]   Ip6Cfg              The pointer to Ip6 config protocol.
  @param[out]  TimeOut             The time out value in 100ns units.

  @retval   EFI_INVALID_PARAMETER  Input parameters are invalid.
  @retval   EFI_SUCCESS            Calculate the time out value successfully.
**/
EFI_STATUS
Dhcp6GetMappingTimeOut (
  IN  EFI_IP6_CONFIG_PROTOCOL  *Ip6Cfg,
  OUT UINTN                    *TimeOut
  );

#endif
