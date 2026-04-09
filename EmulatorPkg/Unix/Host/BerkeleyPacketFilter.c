/**@file
 Berkeley Packet Filter implementation of the EMU_SNP_PROTOCOL that allows the
 emulator to get on real networks.

 Tested on Mac OS X.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Host.h"

#ifdef __APPLE__

  #include <Library/NetLib.h>

#define EMU_SNP_PRIVATE_SIGNATURE  SIGNATURE_32('E', 'M', 's', 'n')
typedef struct {
  UINTN                      Signature;

  EMU_IO_THUNK_PROTOCOL      *Thunk;
  EMU_SNP_PROTOCOL           EmuSnp;
  EFI_SIMPLE_NETWORK_MODE    *Mode;

  int                        BpfFd;
  char                       *InterfaceName;
  EFI_MAC_ADDRESS            MacAddress;
  u_int                      ReadBufferSize;
  VOID                       *ReadBuffer;

  //
  // Two walking pointers to manage the multiple packets that can be returned
  // in a single read.
  //
  VOID                       *CurrentReadPointer;
  VOID                       *EndReadPointer;

  UINT32                     ReceivedPackets;
  UINT32                     DroppedPackets;
} EMU_SNP_PRIVATE;

#define EMU_SNP_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, EMU_SNP_PRIVATE, EmuSnp, EMU_SNP_PRIVATE_SIGNATURE)

//
// Strange, but there doesn't appear to be any structure for the Ethernet header in edk2...
//

typedef struct {
  UINT8     DstAddr[NET_ETHER_ADDR_LEN];
  UINT8     SrcAddr[NET_ETHER_ADDR_LEN];
  UINT16    Type;
} ETHERNET_HEADER;

/**
  Register storage for SNP Mode.

  @param  This Protocol instance pointer.
  @param  Mode SimpleNetworkProtocol Mode structure passed into driver.

  @retval EFI_SUCCESS           The network interface was started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.

**/
EFI_STATUS
EmuSnpCreateMapping (
  IN     EMU_SNP_PROTOCOL         *This,
  IN     EFI_SIMPLE_NETWORK_MODE  *Mode
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  Private->Mode = Mode;

  //
  // Set the broadcast address.
  //
  SetMem (&Mode->BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xFF);

  CopyMem (&Mode->CurrentAddress, &Private->MacAddress, sizeof (EFI_MAC_ADDRESS));
  CopyMem (&Mode->PermanentAddress, &Private->MacAddress, sizeof (EFI_MAC_ADDRESS));

  //
  // Since the fake SNP is based on a real NIC, to avoid conflict with the host NIC
  // network stack, we use a different MAC address.
  // So just change the last byte of the MAC address for the real NIC.
  //
  Mode->CurrentAddress.Addr[NET_ETHER_ADDR_LEN - 1]++;

  return EFI_SUCCESS;
}

static struct bpf_insn  mFilterInstructionTemplate[] = {
  // Load 4 bytes from the destination MAC address.
  BPF_STMT (BPF_LD + BPF_W + BPF_ABS,  OFFSET_OF (ETHERNET_HEADER, DstAddr[0])),

  // Compare to first 4 bytes of fake MAC address.
  BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0x12345678,                 0,           3),

  // Load remaining 2 bytes from the destination MAC address.
  BPF_STMT (BPF_LD + BPF_H + BPF_ABS,  OFFSET_OF (ETHERNET_HEADER, DstAddr[4])),

  // Compare to remaining 2 bytes of fake MAC address.
  BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0x9ABC,                     5,           0),

  // Load 4 bytes from the destination MAC address.
  BPF_STMT (BPF_LD + BPF_W + BPF_ABS,  OFFSET_OF (ETHERNET_HEADER, DstAddr[0])),

  // Compare to first 4 bytes of broadcast MAC address.
  BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0xFFFFFFFF,                 0,           2),

  // Load remaining 2 bytes from the destination MAC address.
  BPF_STMT (BPF_LD + BPF_H + BPF_ABS,  OFFSET_OF (ETHERNET_HEADER, DstAddr[4])),

  // Compare to remaining 2 bytes of broadcast MAC address.
  BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0xFFFF,                     1,           0),

  // Reject packet.
  BPF_STMT (BPF_RET + BPF_K,           0),

  // Receive entire packet.
  BPF_STMT (BPF_RET + BPF_K,           -1)
};

EFI_STATUS
OpenBpfFileDescriptor (
  IN EMU_SNP_PRIVATE  *Private,
  OUT int             *Fd
  )
{
  char  BfpDeviceName[256];
  int   Index;

  //
  // Open a Berkeley Packet Filter device.  This must be done as root, so this is probably
  // the place which is most likely to fail...
  //
  for (Index = 0; TRUE; Index++ ) {
    snprintf (BfpDeviceName, sizeof (BfpDeviceName), "/dev/bpf%d", Index);

    *Fd = open (BfpDeviceName, O_RDWR, 0);
    if ( *Fd >= 0 ) {
      return EFI_SUCCESS;
    }

    if (errno == EACCES) {
      printf (
        "SNP: Permissions on '%s' are incorrect.  Fix with 'sudo chmod 666 %s'.\n",
        BfpDeviceName,
        BfpDeviceName
        );
    }

    if (errno != EBUSY) {
      break;
    }
  }

  return EFI_OUT_OF_RESOURCES;
}

/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was started.
  @retval EFI_ALREADY_STARTED   The network interface is already in the started state.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpStart (
  IN EMU_SNP_PROTOCOL  *This
  )
{
  EFI_STATUS          Status;
  EMU_SNP_PRIVATE     *Private;
  struct ifreq        BoundIf;
  struct bpf_program  BpfProgram;
  struct bpf_insn     *FilterProgram;
  u_int               Value;
  u_int               ReadBufferSize;
  UINT16              Temp16;
  UINT32              Temp32;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  switch (Private->Mode->State) {
    case EfiSimpleNetworkStopped:
      break;

    case EfiSimpleNetworkStarted:
    case EfiSimpleNetworkInitialized:
      return EFI_ALREADY_STARTED;
      break;

    default:
      return EFI_DEVICE_ERROR;
      break;
  }

  Status              = EFI_SUCCESS;
  Private->ReadBuffer = NULL;
  if (Private->BpfFd == 0) {
    Status = OpenBpfFileDescriptor (Private, &Private->BpfFd);
    if (EFI_ERROR (Status)) {
      goto DeviceErrorExit;
    }

    //
    // Get the read buffer size.
    //
    if (ioctl (Private->BpfFd, BIOCGBLEN, &ReadBufferSize) < 0) {
      goto DeviceErrorExit;
    }

    //
    // Default value from BIOCGBLEN is usually too small, so use a much larger size, if necessary.
    //
    if (ReadBufferSize < FixedPcdGet32 (PcdNetworkPacketFilterSize)) {
      ReadBufferSize = FixedPcdGet32 (PcdNetworkPacketFilterSize);
      if (ioctl (Private->BpfFd, BIOCSBLEN, &ReadBufferSize) < 0) {
        goto DeviceErrorExit;
      }
    }

    //
    // Associate our interface with this BPF file descriptor.
    //
    AsciiStrCpyS (BoundIf.ifr_name, sizeof (BoundIf.ifr_name), Private->InterfaceName);
    if (ioctl (Private->BpfFd, BIOCSETIF, &BoundIf) < 0) {
      goto DeviceErrorExit;
    }

    //
    // Enable immediate mode.
    //
    Value = 1;
    if (ioctl (Private->BpfFd, BIOCIMMEDIATE, &Value) < 0) {
      goto DeviceErrorExit;
    }

    //
    // Enable non-blocking I/O.
    //
    if (fcntl (Private->BpfFd, F_GETFL, 0) == -1) {
      goto DeviceErrorExit;
    }

    Value |= O_NONBLOCK;

    if (fcntl (Private->BpfFd, F_SETFL, Value) == -1) {
      goto DeviceErrorExit;
    }

    //
    // Disable "header complete" flag.  This means the supplied source MAC address is
    // what goes on the wire.
    //
    Value = 1;
    if (ioctl (Private->BpfFd, BIOCSHDRCMPLT, &Value) < 0) {
      goto DeviceErrorExit;
    }

    //
    // Allocate read buffer.
    //
    Private->ReadBufferSize = ReadBufferSize;
    Private->ReadBuffer     = malloc (Private->ReadBufferSize);
    if (Private->ReadBuffer == NULL) {
      goto ErrorExit;
    }

    Private->CurrentReadPointer = Private->EndReadPointer = Private->ReadBuffer;

    //
    // Install our packet filter: successful reads should only produce broadcast or unicast
    // packets directed to our fake MAC address.
    //
    FilterProgram = malloc (sizeof (mFilterInstructionTemplate));
    if ( FilterProgram == NULL ) {
      goto ErrorExit;
    }

    CopyMem (FilterProgram, &mFilterInstructionTemplate, sizeof (mFilterInstructionTemplate));

    //
    // Insert out fake MAC address into the filter.  The data has to be host endian.
    //
    CopyMem (&Temp32, &Private->Mode->CurrentAddress.Addr[0], sizeof (UINT32));
    FilterProgram[1].k = NTOHL (Temp32);
    CopyMem (&Temp16, &Private->Mode->CurrentAddress.Addr[4], sizeof (UINT16));
    FilterProgram[3].k = NTOHS (Temp16);

    BpfProgram.bf_len   = sizeof (mFilterInstructionTemplate) / sizeof (struct bpf_insn);
    BpfProgram.bf_insns = FilterProgram;

    if (ioctl (Private->BpfFd, BIOCSETF, &BpfProgram) < 0) {
      goto DeviceErrorExit;
    }

    free (FilterProgram);

    //
    // Enable promiscuous mode.
    //
    if (ioctl (Private->BpfFd, BIOCPROMISC, 0) < 0) {
      goto DeviceErrorExit;
    }

    Private->Mode->State = EfiSimpleNetworkStarted;
  }

  return Status;

DeviceErrorExit:
  Status = EFI_DEVICE_ERROR;
ErrorExit:
  if (Private->ReadBuffer != NULL) {
    free (Private->ReadBuffer);
    Private->ReadBuffer = NULL;
  }

  return Status;
}

/**
  Changes the state of a network interface from "started" to "stopped".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was stopped.
  @retval EFI_ALREADY_STARTED   The network interface is already in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpStop (
  IN EMU_SNP_PROTOCOL  *This
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  switch ( Private->Mode->State ) {
    case EfiSimpleNetworkStarted:
      break;

    case EfiSimpleNetworkStopped:
      return EFI_NOT_STARTED;
      break;

    default:
      return EFI_DEVICE_ERROR;
      break;
  }

  if (Private->BpfFd != 0) {
    close (Private->BpfFd);
    Private->BpfFd = 0;
  }

  if (Private->ReadBuffer != NULL) {
    free (Private->ReadBuffer);
    Private->CurrentReadPointer = Private->EndReadPointer = Private->ReadBuffer = NULL;
  }

  Private->Mode->State = EfiSimpleNetworkStopped;

  return EFI_SUCCESS;
}

/**
  Resets a network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation
  of additional transmit and receive buffers.

  @param  This              The protocol instance pointer.
  @param  ExtraRxBufferSize The size, in bytes, of the extra receive buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the extra
                            buffer, and the caller will not know if it is actually
                            being used.
  @param  ExtraTxBufferSize The size, in bytes, of the extra transmit buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the extra
                            buffer, and the caller will not know if it is actually
                            being used.

  @retval EFI_SUCCESS           The network interface was initialized.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory for the transmit and
                                receive buffers.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpInitialize (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINTN             ExtraRxBufferSize  OPTIONAL,
  IN UINTN             ExtraTxBufferSize  OPTIONAL
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  switch ( Private->Mode->State ) {
    case EfiSimpleNetworkStarted:
      break;

    case EfiSimpleNetworkStopped:
      return EFI_NOT_STARTED;
      break;

    default:
      return EFI_DEVICE_ERROR;
      break;
  }

  Private->Mode->MCastFilterCount     = 0;
  Private->Mode->ReceiveFilterSetting = 0;
  ZeroMem (Private->Mode->MCastFilter, sizeof (Private->Mode->MCastFilter));

  Private->Mode->State = EfiSimpleNetworkInitialized;

  return EFI_SUCCESS;
}

/**
  Resets a network adapter and re-initializes it with the parameters that were
  provided in the previous call to Initialize().

  @param  This                 The protocol instance pointer.
  @param  ExtendedVerification Indicates that the driver may perform a more
                               exhaustive verification operation of the device
                               during reset.

  @retval EFI_SUCCESS           The network interface was reset.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpReset (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           ExtendedVerification
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  switch ( Private->Mode->State ) {
    case EfiSimpleNetworkInitialized:
      break;

    case EfiSimpleNetworkStopped:
      return EFI_NOT_STARTED;
      break;

    default:
      return EFI_DEVICE_ERROR;
      break;
  }

  return EFI_SUCCESS;
}

/**
  Resets a network adapter and leaves it in a state that is safe for
  another driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was shutdown.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpShutdown (
  IN EMU_SNP_PROTOCOL  *This
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  switch ( Private->Mode->State ) {
    case EfiSimpleNetworkInitialized:
      break;

    case EfiSimpleNetworkStopped:
      return EFI_NOT_STARTED;
      break;

    default:
      return EFI_DEVICE_ERROR;
      break;
  }

  Private->Mode->State = EfiSimpleNetworkStarted;

  Private->Mode->ReceiveFilterSetting = 0;
  Private->Mode->MCastFilterCount     = 0;
  ZeroMem (Private->Mode->MCastFilter, sizeof (Private->Mode->MCastFilter));

  if (Private->BpfFd != 0) {
    close (Private->BpfFd);
    Private->BpfFd = 0;
  }

  if (Private->ReadBuffer != NULL) {
    free (Private->ReadBuffer);
    Private->CurrentReadPointer = Private->EndReadPointer = Private->ReadBuffer = NULL;
  }

  return EFI_SUCCESS;
}

/**
  Manages the multicast receive filters of a network interface.

  @param  This             The protocol instance pointer.
  @param  Enable           A bit mask of receive filters to enable on the network interface.
  @param  Disable          A bit mask of receive filters to disable on the network interface.
  @param  ResetMCastFilter Set to TRUE to reset the contents of the multicast receive
                           filters on the network interface to their default values.
  @param  McastFilterCnt   Number of multicast HW MAC addresses in the new
                           MCastFilter list. This value must be less than or equal to
                           the MCastFilterCnt field of EMU_SNP_MODE. This
                           field is optional if ResetMCastFilter is TRUE.
  @param  MCastFilter      A pointer to a list of new multicast receive filter HW MAC
                           addresses. This list will replace any existing multicast
                           HW MAC address list. This field is optional if
                           ResetMCastFilter is TRUE.

  @retval EFI_SUCCESS           The multicast receive filter list was updated.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpReceiveFilters (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINT32            Enable,
  IN UINT32            Disable,
  IN BOOLEAN           ResetMCastFilter,
  IN UINTN             MCastFilterCnt     OPTIONAL,
  IN EFI_MAC_ADDRESS   *MCastFilter OPTIONAL
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  // For now, just succeed...
  return EFI_SUCCESS;
}

/**
  Modifies or resets the current station address, if supported.

  @param  This  The protocol instance pointer.
  @param  Reset Flag used to reset the station address to the network interfaces
                permanent address.
  @param  New   The new station address to be used for the network interface.

  @retval EFI_SUCCESS           The network interfaces station address was updated.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpStationAddress (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           Reset,
  IN EFI_MAC_ADDRESS   *New OPTIONAL
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  return EFI_UNSUPPORTED;
}

/**
  Resets or collects the statistics on a network interface.

  @param  This            Protocol instance pointer.
  @param  Reset           Set to TRUE to reset the statistics for the network interface.
  @param  StatisticsSize  On input the size, in bytes, of StatisticsTable. On
                          output the size, in bytes, of the resulting table of
                          statistics.
  @param  StatisticsTable A pointer to the EFI_NETWORK_STATISTICS structure that
                          contains the statistics.

  @retval EFI_SUCCESS           The statistics were collected from the network interface.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The Statistics buffer was too small. The current buffer
                                size needed to hold the statistics is returned in
                                StatisticsSize.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpStatistics (
  IN EMU_SNP_PROTOCOL         *This,
  IN BOOLEAN                  Reset,
  IN OUT UINTN                *StatisticsSize   OPTIONAL,
  OUT EFI_NETWORK_STATISTICS  *StatisticsTable  OPTIONAL
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  return EFI_UNSUPPORTED;
}

/**
  Converts a multicast IP address to a multicast HW MAC address.

  @param  This The protocol instance pointer.
  @param  IPv6 Set to TRUE if the multicast IP address is IPv6 [RFC 2460]. Set
               to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param  IP   The multicast IP address that is to be converted to a multicast
               HW MAC address.
  @param  MAC  The multicast HW MAC address that is to be generated from IP.

  @retval EFI_SUCCESS           The multicast IP address was mapped to the multicast
                                HW MAC address.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The Statistics buffer was too small. The current buffer
                                size needed to hold the statistics is returned in
                                StatisticsSize.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpMCastIpToMac (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           IPv6,
  IN EFI_IP_ADDRESS    *IP,
  OUT EFI_MAC_ADDRESS  *MAC
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  return EFI_UNSUPPORTED;
}

/**
  Performs read and write operations on the NVRAM device attached to a
  network interface.

  @param  This       The protocol instance pointer.
  @param  ReadWrite  TRUE for read operations, FALSE for write operations.
  @param  Offset     Byte offset in the NVRAM device at which to start the read or
                     write operation. This must be a multiple of NvRamAccessSize and
                     less than NvRamSize.
  @param  BufferSize The number of bytes to read or write from the NVRAM device.
                     This must also be a multiple of NvramAccessSize.
  @param  Buffer     A pointer to the data buffer.

  @retval EFI_SUCCESS           The NVRAM access was performed.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpNvData (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           ReadWrite,
  IN UINTN             Offset,
  IN UINTN             BufferSize,
  IN OUT VOID          *Buffer
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  return EFI_UNSUPPORTED;
}

/**
  Reads the current interrupt status and recycled transmit buffer status from
  a network interface.

  @param  This            The protocol instance pointer.
  @param  InterruptStatus A pointer to the bit mask of the currently active interrupts
                          If this is NULL, the interrupt status will not be read from
                          the device. If this is not NULL, the interrupt status will
                          be read from the device. When the  interrupt status is read,
                          it will also be cleared. Clearing the transmit  interrupt
                          does not empty the recycled transmit buffer array.
  @param  TxBuf           Recycled transmit buffer address. The network interface will
                          not transmit if its internal recycled transmit buffer array
                          is full. Reading the transmit buffer does not clear the
                          transmit interrupt. If this is NULL, then the transmit buffer
                          status will not be read. If there are no transmit buffers to
                          recycle and TxBuf is not NULL, * TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpGetStatus (
  IN EMU_SNP_PROTOCOL  *This,
  OUT UINT32           *InterruptStatus OPTIONAL,
  OUT VOID             **TxBuf OPTIONAL
  )
{
  EMU_SNP_PRIVATE  *Private;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  if ( InterruptStatus != NULL ) {
    *InterruptStatus = EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
  }

  return EFI_SUCCESS;
}

/**
  Places a packet in the transmit queue of a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header to be filled in by
                     the Transmit() function. If HeaderSize is non-zero, then it
                     must be equal to This->Mode->MediaHeaderSize and the DestAddr
                     and Protocol parameters must not be NULL.
  @param  BufferSize The size, in bytes, of the entire packet (media header and
                     data) to be transmitted through the network interface.
  @param  Buffer     A pointer to the packet (media header followed by data) to be
                     transmitted. This parameter cannot be NULL. If HeaderSize is zero,
                     then the media header in Buffer must already be filled in by the
                     caller. If HeaderSize is non-zero, then the media header will be
                     filled in by the Transmit() function.
  @param  SrcAddr    The source HW MAC address. If HeaderSize is zero, then this parameter
                     is ignored. If HeaderSize is non-zero and SrcAddr is NULL, then
                     This->Mode->CurrentAddress is used for the source HW MAC address.
  @param  DestAddr   The destination HW MAC address. If HeaderSize is zero, then this
                     parameter is ignored.
  @param  Protocol   The type of header to build. If HeaderSize is zero, then this
                     parameter is ignored. See RFC 1700, section "Ether Types", for
                     examples.

  @retval EFI_SUCCESS           The packet was placed on the transmit queue.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_NOT_READY         The network interface is too busy to accept this transmit request.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpTransmit (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINTN             HeaderSize,
  IN UINTN             BufferSize,
  IN VOID              *Buffer,
  IN EFI_MAC_ADDRESS   *SrcAddr  OPTIONAL,
  IN EFI_MAC_ADDRESS   *DestAddr OPTIONAL,
  IN UINT16            *Protocol OPTIONAL
  )
{
  EMU_SNP_PRIVATE  *Private;
  ETHERNET_HEADER  *EnetHeader;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  if (Private->Mode->State < EfiSimpleNetworkStarted) {
    return EFI_NOT_STARTED;
  }

  if ( HeaderSize != 0 ) {
    if ((DestAddr == NULL) || (Protocol == NULL) || (HeaderSize != Private->Mode->MediaHeaderSize)) {
      return EFI_INVALID_PARAMETER;
    }

    if (SrcAddr == NULL) {
      SrcAddr = &Private->Mode->CurrentAddress;
    }

    EnetHeader = (ETHERNET_HEADER *)Buffer;

    CopyMem (EnetHeader->DstAddr, DestAddr, NET_ETHER_ADDR_LEN);
    CopyMem (EnetHeader->SrcAddr, SrcAddr, NET_ETHER_ADDR_LEN);

    EnetHeader->Type = HTONS (*Protocol);
  }

  if (write (Private->BpfFd, Buffer, BufferSize) < 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Receives a packet from a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header received on the network
                     interface. If this parameter is NULL, then the media header size
                     will not be returned.
  @param  BufferSize On entry, the size, in bytes, of Buffer. On exit, the size, in
                     bytes, of the packet that was received on the network interface.
  @param  Buffer     A pointer to the data buffer to receive both the media header and
                     the data.
  @param  SrcAddr    The source HW MAC address. If this parameter is NULL, the
                     HW MAC source address will not be extracted from the media
                     header.
  @param  DestAddr   The destination HW MAC address. If this parameter is NULL,
                     the HW MAC destination address will not be extracted from the
                     media header.
  @param  Protocol   The media header type. If this parameter is NULL, then the
                     protocol will not be extracted from the media header. See
                     RFC 1700 section "Ether Types" for examples.

  @retval  EFI_SUCCESS           The received data was stored in Buffer, and BufferSize has
                                 been updated to the number of bytes received.
  @retval  EFI_NOT_STARTED       The network interface has not been started.
  @retval  EFI_NOT_READY         The network interface is too busy to accept this transmit
                                 request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval  EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EmuSnpReceive (
  IN EMU_SNP_PROTOCOL  *This,
  OUT UINTN            *HeaderSize OPTIONAL,
  IN OUT UINTN         *BufferSize,
  OUT VOID             *Buffer,
  OUT EFI_MAC_ADDRESS  *SrcAddr    OPTIONAL,
  OUT EFI_MAC_ADDRESS  *DestAddr   OPTIONAL,
  OUT UINT16           *Protocol   OPTIONAL
  )
{
  EMU_SNP_PRIVATE  *Private;
  struct bpf_hdr   *BpfHeader;
  struct bpf_stat  BpfStats;
  ETHERNET_HEADER  *EnetHeader;
  ssize_t          Result;

  Private = EMU_SNP_PRIVATE_DATA_FROM_THIS (This);

  if (Private->Mode->State < EfiSimpleNetworkStarted) {
    return EFI_NOT_STARTED;
  }

  ZeroMem (&BpfStats, sizeof (BpfStats));

  if (ioctl (Private->BpfFd, BIOCGSTATS, &BpfStats) == 0) {
    Private->ReceivedPackets += BpfStats.bs_recv;
    if (BpfStats.bs_drop > Private->DroppedPackets) {
      printf (
        "SNP: STATS: RCVD = %d DROPPED = %d.  Probably need to increase BPF PcdNetworkPacketFilterSize?\n",
        BpfStats.bs_recv,
        BpfStats.bs_drop - Private->DroppedPackets
        );
      Private->DroppedPackets = BpfStats.bs_drop;
    }
  }

  //
  // Do we have any remaining packets from the previous read?
  //
  if (Private->CurrentReadPointer >= Private->EndReadPointer) {
    Result = read (Private->BpfFd, Private->ReadBuffer, Private->ReadBufferSize);
    if (Result < 0) {
      // EAGAIN means that there's no I/O outstanding against this file descriptor.
      return (errno == EAGAIN) ? EFI_NOT_READY : EFI_DEVICE_ERROR;
    }

    if (Result == 0) {
      return EFI_NOT_READY;
    }

    Private->CurrentReadPointer = Private->ReadBuffer;
    Private->EndReadPointer     = Private->CurrentReadPointer + Result;
  }

  BpfHeader  = Private->CurrentReadPointer;
  EnetHeader = Private->CurrentReadPointer + BpfHeader->bh_hdrlen;

  if (BpfHeader->bh_caplen > *BufferSize) {
    *BufferSize = BpfHeader->bh_caplen;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Buffer, EnetHeader, BpfHeader->bh_caplen);
  *BufferSize = BpfHeader->bh_caplen;

  if (HeaderSize != NULL) {
    *HeaderSize = sizeof (ETHERNET_HEADER);
  }

  if (DestAddr != NULL) {
    ZeroMem (DestAddr, sizeof (EFI_MAC_ADDRESS));
    CopyMem (DestAddr, EnetHeader->DstAddr, NET_ETHER_ADDR_LEN);
  }

  if (SrcAddr != NULL) {
    ZeroMem (SrcAddr, sizeof (EFI_MAC_ADDRESS));
    CopyMem (SrcAddr, EnetHeader->SrcAddr, NET_ETHER_ADDR_LEN);
  }

  if (Protocol != NULL) {
    *Protocol = NTOHS (EnetHeader->Type);
  }

  Private->CurrentReadPointer += BPF_WORDALIGN (BpfHeader->bh_hdrlen + BpfHeader->bh_caplen);
  return EFI_SUCCESS;
}

EMU_SNP_PROTOCOL  gEmuSnpProtocol = {
  GasketSnpCreateMapping,
  GasketSnpStart,
  GasketSnpStop,
  GasketSnpInitialize,
  GasketSnpReset,
  GasketSnpShutdown,
  GasketSnpReceiveFilters,
  GasketSnpStationAddress,
  GasketSnpStatistics,
  GasketSnpMCastIpToMac,
  GasketSnpNvData,
  GasketSnpGetStatus,
  GasketSnpTransmit,
  GasketSnpReceive
};

EFI_STATUS
GetInterfaceMacAddr (
  EMU_SNP_PRIVATE  *Private
  )
{
  EFI_STATUS          Status;
  struct ifaddrs      *IfAddrs;
  struct ifaddrs      *If;
  struct sockaddr_dl  *IfSdl;

  if (getifaddrs (&IfAddrs) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Convert the interface name to ASCII so we can find it.
  //
  Private->InterfaceName = malloc (StrSize (Private->Thunk->ConfigString));
  if (Private->InterfaceName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  UnicodeStrToAsciiStrS (
    Private->Thunk->ConfigString,
    Private->InterfaceName,
    StrSize (Private->Thunk->ConfigString)
    );

  Status = EFI_NOT_FOUND;
  If     = IfAddrs;
  while (If != NULL) {
    IfSdl = (struct sockaddr_dl *)If->ifa_addr;

    if (IfSdl->sdl_family == AF_LINK) {
      if (!AsciiStrCmp (Private->InterfaceName, If->ifa_name)) {
        CopyMem (&Private->MacAddress, LLADDR (IfSdl), NET_ETHER_ADDR_LEN);

        Status = EFI_SUCCESS;
        break;
      }
    }

    If = If->ifa_next;
  }

Exit:
  freeifaddrs (IfAddrs);
  return Status;
}

EFI_STATUS
EmuSnpThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  EMU_SNP_PRIVATE  *Private;

  if (This->Private != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (!CompareGuid (This->Protocol, &gEmuSnpProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = malloc (sizeof (EMU_SNP_PRIVATE));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = EMU_SNP_PRIVATE_SIGNATURE;
  Private->Thunk     = This;
  CopyMem (&Private->EmuSnp, &gEmuSnpProtocol, sizeof (gEmuSnpProtocol));
  GetInterfaceMacAddr (Private);

  This->Interface = &Private->EmuSnp;
  This->Private   = Private;
  return EFI_SUCCESS;
}

EFI_STATUS
EmuSnpThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  EMU_SNP_PRIVATE  *Private;

  if (!CompareGuid (This->Protocol, &gEmuSnpProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = This->Private;
  free (Private);

  return EFI_SUCCESS;
}

EMU_IO_THUNK_PROTOCOL  gSnpThunkIo = {
  &gEmuSnpProtocolGuid,
  NULL,
  NULL,
  0,
  GasketSnpThunkOpen,
  GasketSnpThunkClose,
  NULL
};

#endif
