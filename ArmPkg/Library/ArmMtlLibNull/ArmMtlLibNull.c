/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#include <Library/ArmMtlLib.h>
#include <Library/DebugLib.h>

/** Wait until channel is free.

  @param[in] Channel                Pointer to a channel.
  @param[in] TimeOutInMicroSeconds  Timeout in micro seconds.

  @retval EFI_UNSUPPORTED           Interface not implemented.
**/
EFI_STATUS
MtlWaitUntilChannelFree (
  IN MTL_CHANNEL  *Channel,
  IN UINTN        TimeOutInMicroSeconds
  )
{
  return EFI_UNSUPPORTED;
}

/** Return the address of the message payload.

  @param[in] Channel   Pointer to a channel.

  @retval UINT32*      Pointer to the payload.
**/
UINT32 *
MtlGetChannelPayload (
  IN  MTL_CHANNEL  *Channel
  )
{
  ASSERT (FALSE);
  return NULL;
}

/** Return pointer to a channel for the requested channel type.

  @param[in] ChannelType        ChannelType, Low or High priority channel.
                                MTL_CHANNEL_TYPE_LOW or
                                MTL_CHANNEL_TYPE_HIGH

  @param[out] Channel           Holds pointer to the channel.

  @retval EFI_UNSUPPORTED       Requested channel type not supported or
                                interface not implemented.
**/
EFI_STATUS
MtlGetChannel (
  IN  MTL_CHANNEL_TYPE  ChannelType,
  OUT MTL_CHANNEL       **Channel
  )
{
  return EFI_UNSUPPORTED;
}

/** Mark the channel busy and ring the doorbell.

  @param[in] Channel               Pointer to a channel.
  @param[in] MessageHeader         Message header.

  @param[out] PayloadLength        Message length.

  @retval EFI_UNSUPPORTED          Interface not implemented.
**/
EFI_STATUS
MtlSendMessage (
  IN  MTL_CHANNEL  *Channel,
  IN  UINT32       MessageHeader,
  OUT UINT32       PayloadLength
  )
{
  return EFI_UNSUPPORTED;
}

/** Wait for a response on a channel.

  If channel is free after sending message, it implies SCP responded
  with a response on the channel.

  @param[in] Channel               Pointer to a channel.

  @retval EFI_UNSUPPORTED          Interface not implemented.
**/
EFI_STATUS
MtlReceiveMessage (
  IN  MTL_CHANNEL  *Channel,
  OUT UINT32       *MessageHeader,
  OUT UINT32       *PayloadLength
  )
{
  return EFI_UNSUPPORTED;
}
