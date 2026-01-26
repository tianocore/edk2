/** @file
  This module implements functions to be used by MPXY client

  Copyright (c) 2024, Ventana Micro Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SBI_MPXY_H_
#define SBI_MPXY_H_

enum {
  MpxyChanAttrProtId,
  MpxyChanAttrProtVersion,
  MpxyChanAttrMsgDataMaxLen,
  MpxyChanAttrMsgSendTimeout,
  MpxyChanAttrMsgCompletionTimeout,
  MpxyChanAttrChannelCapability,
  MpxyChanAttrSseEventId,
  MpxyChanAttrMsiControl,
  MpxyChanAttrMsiAddrLow,
  MpxyChanAttrMsiAddrHigh,
  MpxyChanAttrMsiData,
  MpxyChanAttrEventStateControl,
  MpxyChanAttrMax
};

#define MPXY_MSG_PROTO_ATTR_START  0x80000000
#define MPXY_MSG_PROTO_ATTR_END    0xffffffff

/**
  Get the list of channels available on MPXY.

  @param[in] StartIndex - Index to start reading from. Initially it will be zero,
             after subsequent reads, it will be the last index read + 1.
  @param[out] List of the channels available.
  @param[out] Number of channels remaining.
  @param[out] Number of channels retruned in this read.

  @retval EFI_SUCCESS   If list of channels was obtained successfully.
**/
EFI_STATUS
EFIAPI
SbiMpxyGetChannelList (
  IN  UINTN  StartIndex,
  OUT UINTN  *ChannelList,
  OUT UINTN  *Remaining,
  OUT UINTN  *Returned
  );

/**
  Read the attributes (both base and protocol specific) of a channel

  @param[in] Channel for which attributes are to be read
  @param[in] The base attribute ID
  @param[in] Number of attributes to be read
  @param[out] Attributes read from channel from base attributes Id

  @retval EFI_SUCCESS If the attributes were read successfully
**/

EFI_STATUS
EFIAPI
SbiMpxyReadChannelAttrs (
  IN UINTN    ChannelId,
  IN UINT32   BaseAttrId,
  IN UINT32   NrAttrs,
  OUT UINT32  *Attrs
  );

/**
  Open specified MPXY channel for communication. It will allocate the shared
  memory or resize the previous one if required.

  @param[in] ChannelId  The channel to be initialized
  @retval EFI_SUCCESS   If the allocation or resize of shared memory was
                        successfully done.
**/
EFI_STATUS
EFIAPI
SbiMpxyChannelOpen (
  IN UINTN  ChannelId
  );

/**
  Close the specified MPXY channel.

  @param[in] ChannelId  The channel to be uninitialized
  @retval EFI_SUCCESS   If the allocation or resize of shared memory was
                        successfully done.
**/
EFI_STATUS
EFIAPI
SbiMpxyChannelClose (
  IN UINTN  ChannelId
  );

/**
  Send a message with response over Mpxy.

  @param[in] ChannelId       The Channel on which message would be sent
  @param[in] MessageId       Message protocol specific message identification
  @param[in] MessageDataLen  Length of the message to be sent
  @param[in] Message         Pointer to buffer containing message
  @param[in] Response        Pointer to buffer to which response should be written
  @param[in] ResponseLen     Pointer where the size of response should be written

  @retval EFI_SUCCESS    The shared memory was disabled
  @retval Other          Some error occured during the operation
**/
EFI_STATUS
EFIAPI
SbiMpxySendMessage (
  IN UINTN   ChannelId,
  IN UINTN   MessageId,
  IN VOID    *Message,
  IN UINTN   MessageDataLen,
  OUT VOID   *Response,
  OUT UINTN  *ResponseLen
  );

#endif
