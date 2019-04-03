/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#ifndef ARM_MTL_LIB_H_
#define ARM_MTL_LIB_H_

#include <Uefi/UefiBaseType.h>

// Ideally we don't need packed struct. However we can't rely on compilers.
#pragma pack(1)

typedef struct {
  UINT32 Reserved1;
  UINT32 ChannelStatus;
  UINT64 Reserved2;
  UINT32 Flags;
  UINT32 Length;
  UINT32 MessageHeader;

  // NOTE: Since EDK2 does not allow flexible array member [] we declare
  // here array of 1 element length. However below is used as a variable
  // length array.
  UINT32 Payload[1];    // size less object gives offset to payload.
} MTL_MAILBOX;

#pragma pack()

// Channel Type, Low-priority, and High-priority
typedef enum {
  MTL_CHANNEL_TYPE_LOW = 0,
  MTL_CHANNEL_TYPE_HIGH = 1
} MTL_CHANNEL_TYPE;

typedef struct {
  UINT64 PhysicalAddress;
  UINT32 ModifyMask;
  UINT32 PreserveMask;
} MTL_DOORBELL;

typedef struct {
  MTL_CHANNEL_TYPE ChannelType;
  MTL_MAILBOX      * CONST MailBox;
  MTL_DOORBELL     DoorBell;
} MTL_CHANNEL;

/** Wait until channel is free.

  @param[in] Channel                Pointer to a channel.
  @param[in] TimeOutInMicroSeconds  Time out in micro seconds.

  @retval EFI_SUCCESS               Channel is free.
  @retval EFI_TIMEOUT               Time out error.
**/
EFI_STATUS
MtlWaitUntilChannelFree (
  IN MTL_CHANNEL  *Channel,
  IN UINTN        TimeOutInMicroSeconds
  );

/** Return the address of the message payload.

  @param[in] Channel   Pointer to a channel.

  @retval UINT32*      Pointer to the payload.
**/
UINT32*
MtlGetChannelPayload (
  IN MTL_CHANNEL  *Channel
  );

/** Return pointer to a channel for the requested channel type.

  @param[in] ChannelType        ChannelType, Low or High priority channel.
                                MTL_CHANNEL_TYPE_LOW or
                                MTL_CHANNEL_TYPE_HIGH

  @param[out] Channel           Holds pointer to the channel.

  @retval EFI_SUCCESS           Pointer to channel is returned.
  @retval EFI_UNSUPPORTED       Requested channel type not supported.
**/
EFI_STATUS
MtlGetChannel (
  IN  MTL_CHANNEL_TYPE  ChannelType,
  OUT MTL_CHANNEL       **Channel
  );

/** Mark the channel busy and ring the doorbell.

  @param[in] Channel               Pointer to a channel.
  @param[in] MessageHeader         Message header.

  @param[out] PayloadLength        Message length.

  @retval EFI_SUCCESS              Message sent successfully.
  @retval EFI_DEVICE_ERROR         Channel is busy.
**/
EFI_STATUS
MtlSendMessage (
  IN  MTL_CHANNEL  *Channel,
  IN  UINT32       MessageHeader,
  OUT UINT32       PayloadLength
  );

/** Wait for a response on a channel.

  If channel is free after sending message, it implies SCP responded
  with a response on the channel.

  @param[in] Channel               Pointer to a channel.

  @retval EFI_SUCCESS              Message received successfully.
  @retval EFI_TIMEOUT              Time out error.
**/
EFI_STATUS
MtlReceiveMessage (
  IN  MTL_CHANNEL  *Channel,
  OUT UINT32       *MessageHeader,
  OUT UINT32       *PayloadLength
  );

#endif  /* ARM_MTL_LIB_H_ */

