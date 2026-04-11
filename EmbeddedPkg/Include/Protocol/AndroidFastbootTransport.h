/** @file

  Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  Transport protocol over which Android Fastboot transactions can be made.
  Fastboot is designed for USB, but this protocol is intended as an abstraction
   so that it can be implemented over any transport mechanism.
*/

#ifndef __ANDROID_FASTBOOT_TRANSPORT_H__
#define __ANDROID_FASTBOOT_TRANSPORT_H__

extern EFI_GUID  gAndroidFastbootTransportProtocolGuid;

/*
  Set up the transport system for use by Fastboot.
  e.g. For USB this probably means making the device enumerable. For TCP,
       preparing to accept incoming connections.

  It is _not_ the responsibility of this protocol's implementer to unite the
  data phase into a single buffer - that is handled by the Fastboot UEFI
  application. As the Fastboot protocol spec says: "Short packets are always
   acceptable and zero-length packets are ignored."
  However the commands and responses must be in a single packet, and the order
  of the packets must of course be maintained.

  If there is a fatal error in the receive channel, ReceiveEvent will be
  signalled, and a subsequent call to Receive() will return an error. This
  allows data transported prior to the error to be received.

  @param[in] ReceiveEvent  Event to be Signalled when a packet has been received
                           and is ready to be retrieved via Receive().

  @retval EFI_SUCCESS       Initialised successfully.
  @retval EFI_DEVICE_ERROR  Error in initialising hardware
  @retval (other)           Error return from LocateProtocol functions.
*/
typedef
EFI_STATUS
(*FASTBOOT_TRANSPORT_START) (
  IN EFI_EVENT  ReceiveEvent
  );

/*
  Function to be called when all Fastboot transactions are finished, to
  de-initialise the transport system.
  e.g. A USB OTG system might want to get out of peripheral mode so it can be
       a USB host.

  Note that this function will be called after an error is reported by Send or
  Receive

  @retval EFI_SUCCESS       De-initialised successfully.
  @retval EFI_DEVICE_ERROR  Error de-initialising hardware.
*/
typedef
EFI_STATUS
(*FASTBOOT_TRANSPORT_STOP) (
  VOID
  );

/*
  Send data. This function can be used both for command responses like "OKAY"
  and for the data phase (the protocol doesn't describe any situation when the
   latter might be necessary, but does allow it)

  Transmission need not finish before the function returns.
  If there is an error in transmission from which the transport system cannot
  recover, FatalErrorEvent will be signalled. Otherwise, it is assumed that all
  data was delivered successfully.

  @param[in] BufferSize       Size in bytes of data to send.
  @param[in] Buffer           Data to send.
  @param[in] FatalErrorEvent  Event to signal if there was an error in
                              transmission from which the transport system
                              cannot recover.

  @retval EFI_SUCCESS       The data was sent or queued for send.
  @retval EFI_DEVICE_ERROR  There was an error preparing to send the data.
 */
typedef
EFI_STATUS
(*FASTBOOT_TRANSPORT_SEND) (
  IN        UINTN      BufferSize,
  IN  CONST VOID       *Buffer,
  IN        EFI_EVENT  *FatalErrorEvent
  );

/*
  When the event has been Signalled to say data is available from the host,
  this function is used to get data. In order to handle the case where several
  packets are received before ReceiveEvent's notify function is called, packets
  received are queued, and each call to this function returns the next packet in
  the queue. It should therefore be called in a loop, the exit condition being a
  return of EFI_NOT_READY.

  @param[out]  Buffer       Pointer to received data. Callee allocated - the
                            caller must free it with FreePool.
  @param[out]  BufferSize   The size of received data in bytes

  @retval EFI_NOT_READY     There is no data available
  @retval EFI_DEVICE_ERROR  There was a fatal error in the receive channel.
                            e.g. for USB the cable was unplugged or for TCP the
                            connection was closed by the remote host..
*/
typedef
EFI_STATUS
(*FASTBOOT_TRANSPORT_RECEIVE) (
  OUT UINTN  *BufferSize,
  OUT VOID   **Buffer
  );

typedef struct _FASTBOOT_TRANSPORT_PROTOCOL {
  FASTBOOT_TRANSPORT_START      Start;
  FASTBOOT_TRANSPORT_STOP       Stop;
  FASTBOOT_TRANSPORT_SEND       Send;
  FASTBOOT_TRANSPORT_RECEIVE    Receive;
} FASTBOOT_TRANSPORT_PROTOCOL;

#endif
