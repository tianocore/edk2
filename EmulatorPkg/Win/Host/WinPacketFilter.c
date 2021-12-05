/**@file
 Windows Packet Filter implementation of the EMU_SNP_PROTOCOL that allows the
 emulator to get on real networks.

Copyright (c) 2004 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
(C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "WinHost.h"

#define NETWORK_LIBRARY_NAME_U          L"SnpNt32Io.dll"
#define NETWORK_LIBRARY_INITIALIZE      "SnpInitialize"
#define NETWORK_LIBRARY_FINALIZE        "SnpFinalize"
#define NETWORK_LIBRARY_SET_RCV_FILTER  "SnpSetReceiveFilter"
#define NETWORK_LIBRARY_RECEIVE         "SnpReceive"
#define NETWORK_LIBRARY_TRANSMIT        "SnpTransmit"

#pragma pack(1)
typedef struct _NT_NET_INTERFACE_INFO {
  UINT32             InterfaceIndex;
  EFI_MAC_ADDRESS    MacAddr;
} NT_NET_INTERFACE_INFO;
#pragma pack()

#define NET_ETHER_HEADER_SIZE       14
#define MAX_INTERFACE_INFO_NUMBER   16
#define SNP_MAX_TX_BUFFER_NUM       65536
#define SNP_TX_BUFFER_INCREASEMENT  32
#define DEFAULT_SELECTED_NIC_INDEX  0

//
//  Functions in Net Library
//
typedef
INT32
(*NT_NET_INITIALIZE) (
  IN OUT  UINT32                 *InterfaceCount,
  IN OUT  NT_NET_INTERFACE_INFO  *InterfaceInfoBuffer
  );

typedef
INT32
(*NT_NET_FINALIZE) (
  VOID
  );

typedef
INT32
(*NT_NET_SET_RECEIVE_FILTER) (
  IN  UINT32           Index,
  IN  UINT32           EnableFilter,
  IN  UINT32           MCastFilterCnt,
  IN  EFI_MAC_ADDRESS  *MCastFilter
  );

typedef
INT32
(*NT_NET_RECEIVE) (
  IN      UINT32  Index,
  IN OUT  UINT32  *BufferSize,
  OUT     VOID    *Buffer
  );

typedef
INT32
(*NT_NET_TRANSMIT) (
  IN  UINT32           Index,
  IN  UINT32           HeaderSize,
  IN  UINT32           BufferSize,
  IN  VOID             *Buffer,
  IN  EFI_MAC_ADDRESS  *SrcAddr,
  IN  EFI_MAC_ADDRESS  *DestAddr,
  IN  UINT16           *Protocol
  );

typedef struct _NT_NET_UTILITY_TABLE {
  NT_NET_INITIALIZE            Initialize;
  NT_NET_FINALIZE              Finalize;
  NT_NET_SET_RECEIVE_FILTER    SetReceiveFilter;
  NT_NET_RECEIVE               Receive;
  NT_NET_TRANSMIT              Transmit;
} NT_NET_UTILITY_TABLE;

//
//  Instance data for each fake SNP instance
//
#define WIN_NT_INSTANCE_SIGNATURE  SIGNATURE_32 ('N', 'T', 'I', 'S')

typedef struct  {
  UINT32                     Signature;

  //
  // Array of the recycled transmit buffer address.
  //
  UINT64                     *RecycledTxBuf;

  //
  // Current number of recycled buffer pointers in RecycledTxBuf.
  //
  UINT32                     RecycledTxBufCount;

  //
  // The maximum number of recycled buffer pointers in RecycledTxBuf.
  //
  UINT32                     MaxRecycledTxBuf;
  EFI_SIMPLE_NETWORK_MODE    Mode;
  NT_NET_INTERFACE_INFO      InterfaceInfo;
} WIN_NT_INSTANCE_DATA;

//
//  Instance data for each SNP private instance
//
#define WIN_NT_SIMPLE_NETWORK_PRIVATE_SIGNATURE  SIGNATURE_32 ('N', 'T', 's', 'n')

typedef struct {
  UINTN                      Signature;
  EMU_IO_THUNK_PROTOCOL      *Thunk;
  EMU_SNP_PROTOCOL           EmuSnp;
  EFI_SIMPLE_NETWORK_MODE    *Mode;
  HMODULE                    NetworkLibraryHandle;
  NT_NET_UTILITY_TABLE       NtNetUtilityTable;
  WIN_NT_INSTANCE_DATA       Instance;
} WIN_NT_SNP_PRIVATE;

#define WIN_NT_SNP_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, WIN_NT_SNP_PRIVATE, EmuSnp, WIN_NT_SIMPLE_NETWORK_PRIVATE_SIGNATURE)

/**
  Register storage for SNP Mode.

  @param  This Protocol instance pointer.
  @param  Mode SimpleNetworkProtocol Mode structure passed into driver.

  @retval EFI_SUCCESS           The network interface was started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.

**/
EFI_STATUS
WinNtSnpCreateMapping (
  IN     EMU_SNP_PROTOCOL         *This,
  IN     EFI_SIMPLE_NETWORK_MODE  *Mode
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

  Private->Mode = Mode;

  //
  // Set the broadcast address.
  //
  CopyMem (&Mode->BroadcastAddress, &Private->Instance.Mode.BroadcastAddress, sizeof (EFI_MAC_ADDRESS));
  //
  // Set the MAC address.
  //
  CopyMem (&Mode->CurrentAddress, &Private->Instance.Mode.CurrentAddress, sizeof (EFI_MAC_ADDRESS));
  CopyMem (&Mode->PermanentAddress, &Private->Instance.Mode.PermanentAddress, sizeof (EFI_MAC_ADDRESS));

  return EFI_SUCCESS;
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
WinNtSnpStart (
  IN EMU_SNP_PROTOCOL  *This
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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

  Private->Mode->State = EfiSimpleNetworkStarted;

  return EFI_SUCCESS;
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
WinNtSnpStop (
  IN EMU_SNP_PROTOCOL  *This
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpInitialize (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINTN             ExtraRxBufferSize  OPTIONAL,
  IN UINTN             ExtraTxBufferSize  OPTIONAL
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpReset (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           ExtendedVerification
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpShutdown (
  IN EMU_SNP_PROTOCOL  *This
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpReceiveFilters (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINT32            Enable,
  IN UINT32            Disable,
  IN BOOLEAN           ResetMCastFilter,
  IN UINTN             MCastFilterCnt     OPTIONAL,
  IN EFI_MAC_ADDRESS   *MCastFilter OPTIONAL
  )
{
  WIN_NT_SNP_PRIVATE  *Private;
  INT32               ReturnValue;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

  ReturnValue = Private->NtNetUtilityTable.SetReceiveFilter (
                                             Private->Instance.InterfaceInfo.InterfaceIndex,
                                             Enable,
                                             (UINT32)MCastFilterCnt,
                                             MCastFilter
                                             );

  if (ReturnValue <= 0) {
    return EFI_DEVICE_ERROR;
  }

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
WinNtSnpStationAddress (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           Reset,
  IN EFI_MAC_ADDRESS   *New OPTIONAL
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpStatistics (
  IN EMU_SNP_PROTOCOL         *This,
  IN BOOLEAN                  Reset,
  IN OUT UINTN                *StatisticsSize   OPTIONAL,
  OUT EFI_NETWORK_STATISTICS  *StatisticsTable  OPTIONAL
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpMCastIpToMac (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           IPv6,
  IN EFI_IP_ADDRESS    *IP,
  OUT EFI_MAC_ADDRESS  *MAC
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpNvData (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           ReadWrite,
  IN UINTN             Offset,
  IN UINTN             BufferSize,
  IN OUT VOID          *Buffer
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

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
WinNtSnpGetStatus (
  IN EMU_SNP_PROTOCOL  *This,
  OUT UINT32           *InterruptStatus OPTIONAL,
  OUT VOID             **TxBuf OPTIONAL
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

  if (TxBuf != NULL) {
    if (Private->Instance.RecycledTxBufCount != 0) {
      Private->Instance.RecycledTxBufCount--;
      *((UINT8 **)TxBuf) = (UINT8 *)(UINTN)Private->Instance.RecycledTxBuf[Private->Instance.RecycledTxBufCount];
    } else {
      *((UINT8 **)TxBuf) = NULL;
    }
  }

  if (InterruptStatus != NULL) {
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
WinNtSnpTransmit (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINTN             HeaderSize,
  IN UINTN             BufferSize,
  IN VOID              *Buffer,
  IN EFI_MAC_ADDRESS   *SrcAddr  OPTIONAL,
  IN EFI_MAC_ADDRESS   *DestAddr OPTIONAL,
  IN UINT16            *Protocol OPTIONAL
  )
{
  WIN_NT_SNP_PRIVATE  *Private;
  INT32               ReturnValue;
  UINT64              *Tmp;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

  if ((HeaderSize != 0) && (SrcAddr == NULL)) {
    SrcAddr = &Private->Instance.Mode.CurrentAddress;
  }

  ReturnValue = Private->NtNetUtilityTable.Transmit (
                                             Private->Instance.InterfaceInfo.InterfaceIndex,
                                             (UINT32)HeaderSize,
                                             (UINT32)BufferSize,
                                             Buffer,
                                             SrcAddr,
                                             DestAddr,
                                             Protocol
                                             );

  if (ReturnValue < 0) {
    return EFI_DEVICE_ERROR;
  } else {
    if ((Private->Instance.MaxRecycledTxBuf + SNP_TX_BUFFER_INCREASEMENT) >= SNP_MAX_TX_BUFFER_NUM) {
      return EFI_NOT_READY;
    }

    if (Private->Instance.RecycledTxBufCount < Private->Instance.MaxRecycledTxBuf) {
      Private->Instance.RecycledTxBuf[Private->Instance.RecycledTxBufCount] = (UINT64)Buffer;
      Private->Instance.RecycledTxBufCount++;
    } else {
      Tmp = malloc (sizeof (UINT64) * (Private->Instance.MaxRecycledTxBuf + SNP_TX_BUFFER_INCREASEMENT));
      if (Tmp == NULL) {
        return EFI_DEVICE_ERROR;
      }

      CopyMem (Tmp, Private->Instance.RecycledTxBuf, sizeof (UINT64) * Private->Instance.RecycledTxBufCount);
      free (Private->Instance.RecycledTxBuf);
      Private->Instance.RecycledTxBuf     =  Tmp;
      Private->Instance.MaxRecycledTxBuf += SNP_TX_BUFFER_INCREASEMENT;
    }
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
WinNtSnpReceive (
  IN EMU_SNP_PROTOCOL  *This,
  OUT UINTN            *HeaderSize OPTIONAL,
  IN OUT UINTN         *BufferSize,
  OUT VOID             *Buffer,
  OUT EFI_MAC_ADDRESS  *SrcAddr    OPTIONAL,
  OUT EFI_MAC_ADDRESS  *DestAddr   OPTIONAL,
  OUT UINT16           *Protocol   OPTIONAL
  )
{
  WIN_NT_SNP_PRIVATE  *Private;
  INT32               ReturnValue;
  UINTN               BufSize;

  Private = WIN_NT_SNP_PRIVATE_DATA_FROM_THIS (This);

  BufSize = *BufferSize;

  ASSERT (Private->NtNetUtilityTable.Receive != NULL);

  ReturnValue = Private->NtNetUtilityTable.Receive (
                                             Private->Instance.InterfaceInfo.InterfaceIndex,
                                             BufferSize,
                                             Buffer
                                             );

  if (ReturnValue < 0) {
    if (ReturnValue == -100) {
      return EFI_BUFFER_TOO_SMALL;
    }

    return EFI_DEVICE_ERROR;
  } else if (ReturnValue == 0) {
    return EFI_NOT_READY;
  }

  if (HeaderSize != NULL) {
    *HeaderSize = 14;
  }

  if (SrcAddr != NULL) {
    ZeroMem (SrcAddr, sizeof (EFI_MAC_ADDRESS));
    CopyMem (SrcAddr, ((UINT8 *)Buffer) + 6, 6);
  }

  if (DestAddr != NULL) {
    ZeroMem (DestAddr, sizeof (EFI_MAC_ADDRESS));
    CopyMem (DestAddr, ((UINT8 *)Buffer), 6);
  }

  if (Protocol != NULL) {
    *Protocol = NTOHS (*((UINT16 *)(((UINT8 *)Buffer) + 12)));
  }

  return (*BufferSize <= BufSize) ? EFI_SUCCESS : EFI_BUFFER_TOO_SMALL;
}

/**
  Initialize the snpnt32 driver instance.

  @param  Instance              Pointer to the instance context data.
  @param  NetInfo               Pointer to the interface info.

  @retval EFI_SUCCESS           The driver instance is initialized.
  @retval other                 Initialization errors.

**/
EFI_STATUS
WinNtInitializeInstanceData (
  IN OUT WIN_NT_INSTANCE_DATA   *Instance,
  IN     NT_NET_INTERFACE_INFO  *NetInfo
  )
{
  if ((Instance == NULL) || (NetInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Instance, sizeof (WIN_NT_INSTANCE_DATA));

  Instance->Signature                  = WIN_NT_INSTANCE_SIGNATURE;
  Instance->RecycledTxBufCount         = 0;
  Instance->MaxRecycledTxBuf           = 32;
  Instance->Mode.State                 = EfiSimpleNetworkInitialized;
  Instance->Mode.HwAddressSize         = NET_ETHER_ADDR_LEN;
  Instance->Mode.MediaHeaderSize       = NET_ETHER_HEADER_SIZE;
  Instance->Mode.MaxPacketSize         = 1500;
  Instance->Mode.MaxMCastFilterCount   = MAX_MCAST_FILTER_CNT;
  Instance->Mode.IfType                = NET_IFTYPE_ETHERNET;
  Instance->Mode.MediaPresentSupported = TRUE;
  Instance->Mode.MediaPresent          = TRUE;

  //
  // Allocate the RecycledTxBuf.
  //
  Instance->RecycledTxBuf = malloc (sizeof (UINT64) * Instance->MaxRecycledTxBuf);
  if (Instance->RecycledTxBuf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  //  Set the interface information.
  //
  CopyMem (&Instance->InterfaceInfo, NetInfo, sizeof (Instance->InterfaceInfo));

  //
  //  Set broadcast address
  //
  SetMem (&Instance->Mode.BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xFF);

  //
  //  Copy Current/PermanentAddress MAC address
  //
  CopyMem (&Instance->Mode.CurrentAddress, &Instance->InterfaceInfo.MacAddr, sizeof (Instance->Mode.CurrentAddress));
  CopyMem (&Instance->Mode.PermanentAddress, &Instance->InterfaceInfo.MacAddr, sizeof (Instance->Mode.PermanentAddress));

  //
  //  Since the fake SNP is based on a real NIC, to avoid conflict with the host
  //  NIC network stack, we use a different MAC address.
  //  So just change the last byte of the MAC address for the real NIC.
  //
  Instance->Mode.CurrentAddress.Addr[NET_ETHER_ADDR_LEN - 1]++;

  return EFI_SUCCESS;
}

/**
  Initialize the net utility data.

  @param  This                  Pointer to the private data.
  @param  ActiveInstance        The active network interface.

  @retval EFI_SUCCESS           The global data is initialized.
  @retval EFI_NOT_FOUND         The required DLL is not found.
  @retval EFI_DEVICE_ERROR      Error initialize network utility library.
  @retval other                 Other errors.

**/
EFI_STATUS
WintNtInitializeNetUtilityData (
  IN OUT WIN_NT_SNP_PRIVATE  *Private,
  IN UINT8                   ActiveInstance
  )
{
  EFI_STATUS             Status;
  CHAR16                 *DllFileNameU;
  INT32                  ReturnValue;
  BOOLEAN                NetUtilityLibInitDone;
  NT_NET_INTERFACE_INFO  NetInterfaceInfoBuffer[MAX_INTERFACE_INFO_NUMBER];
  UINT32                 InterfaceCount;
  UINT8                  ActiveInterfaceIndex;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NetUtilityLibInitDone = FALSE;
  InterfaceCount        = MAX_INTERFACE_INFO_NUMBER;
  DllFileNameU          = NETWORK_LIBRARY_NAME_U;

  //
  //  Load network utility library
  //
  Private->NetworkLibraryHandle = LoadLibraryEx (DllFileNameU, NULL, 0);
  if (NULL == Private->NetworkLibraryHandle) {
    return EFI_NOT_FOUND;
  }

  Private->NtNetUtilityTable.Initialize = (NT_NET_INITIALIZE)GetProcAddress (Private->NetworkLibraryHandle, NETWORK_LIBRARY_INITIALIZE);
  if (NULL == Private->NtNetUtilityTable.Initialize) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  Private->NtNetUtilityTable.Finalize = (NT_NET_FINALIZE)GetProcAddress (Private->NetworkLibraryHandle, NETWORK_LIBRARY_FINALIZE);
  if (NULL == Private->NtNetUtilityTable.Finalize) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  Private->NtNetUtilityTable.SetReceiveFilter = (NT_NET_SET_RECEIVE_FILTER)GetProcAddress (Private->NetworkLibraryHandle, NETWORK_LIBRARY_SET_RCV_FILTER);
  if (NULL == Private->NtNetUtilityTable.SetReceiveFilter) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  Private->NtNetUtilityTable.Receive = (NT_NET_RECEIVE)GetProcAddress (Private->NetworkLibraryHandle, NETWORK_LIBRARY_RECEIVE);
  if (NULL == Private->NtNetUtilityTable.Receive) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  Private->NtNetUtilityTable.Transmit = (NT_NET_TRANSMIT)GetProcAddress (Private->NetworkLibraryHandle, NETWORK_LIBRARY_TRANSMIT);
  if (NULL == Private->NtNetUtilityTable.Transmit) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  //
  //  Initialize the network utility library
  //  And enumerate the interfaces in emulator host
  //
  ReturnValue = Private->NtNetUtilityTable.Initialize (&InterfaceCount, &NetInterfaceInfoBuffer[0]);
  if (ReturnValue <= 0) {
    Status = EFI_DEVICE_ERROR;
    goto ErrorReturn;
  }

  NetUtilityLibInitDone = TRUE;

  if (InterfaceCount == 0) {
    Status = EFI_NOT_FOUND;
    goto ErrorReturn;
  }

  DEBUG ((DEBUG_INFO, "%a, total %d interface(s) found\n", __FUNCTION__, InterfaceCount));
  //
  // Active interface index is set to first interface if given instance does
  // not exist.
  //
  ActiveInterfaceIndex = (ActiveInstance >= InterfaceCount ? DEFAULT_SELECTED_NIC_INDEX : ActiveInstance);

  //
  // Initialize instance
  //
  Status = WinNtInitializeInstanceData (&Private->Instance, &NetInterfaceInfoBuffer[ActiveInterfaceIndex]);
  if (EFI_ERROR (Status)) {
    goto ErrorReturn;
  }

  return EFI_SUCCESS;

ErrorReturn:

  if (Private->Instance.RecycledTxBuf != NULL) {
    free (Private->Instance.RecycledTxBuf);
  }

  if (NetUtilityLibInitDone) {
    if (Private->NtNetUtilityTable.Finalize != NULL) {
      Private->NtNetUtilityTable.Finalize ();
      Private->NtNetUtilityTable.Finalize = NULL;
    }
  }

  return Status;
}

/**
  Release the net utility data.

  @param  This                  Pointer to the private data.

  @retval EFI_SUCCESS           The global data is released.
  @retval other                 Other errors.

**/
EFI_STATUS
WintNtReleaseNetUtilityData (
  IN OUT WIN_NT_SNP_PRIVATE  *Private
  )
{
  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->Instance.RecycledTxBuf != NULL) {
    free (Private->Instance.RecycledTxBuf);
  }

  if (Private->NtNetUtilityTable.Finalize != NULL) {
    Private->NtNetUtilityTable.Finalize ();
  }

  FreeLibrary (Private->NetworkLibraryHandle);

  return EFI_SUCCESS;
}

EMU_SNP_PROTOCOL  mWinNtSnpProtocol = {
  WinNtSnpCreateMapping,
  WinNtSnpStart,
  WinNtSnpStop,
  WinNtSnpInitialize,
  WinNtSnpReset,
  WinNtSnpShutdown,
  WinNtSnpReceiveFilters,
  WinNtSnpStationAddress,
  WinNtSnpStatistics,
  WinNtSnpMCastIpToMac,
  WinNtSnpNvData,
  WinNtSnpGetStatus,
  WinNtSnpTransmit,
  WinNtSnpReceive
};

/**
  Open SNP thunk protocol.

  @param  This                  Pointer to the thunk protocol instance.

  @retval EFI_SUCCESS           SNP thunk protocol is opened successfully.
  @retval EFI_UNSUPPORTED       This is not SNP thunk protocol.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
  @retval other                 Other errors.

**/
EFI_STATUS
WinNtSnpThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  WIN_NT_SNP_PRIVATE  *Private;
  UINT8               HostInterfaceIndex;

  HostInterfaceIndex = 0;

  if (This->Private != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (!CompareGuid (This->Protocol, &gEmuSnpProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = malloc (sizeof (WIN_NT_SNP_PRIVATE));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = WIN_NT_SIMPLE_NETWORK_PRIVATE_SIGNATURE;
  Private->Thunk     = This;
  CopyMem (&Private->EmuSnp, &mWinNtSnpProtocol, sizeof (mWinNtSnpProtocol));

  This->Interface = &Private->EmuSnp;
  This->Private   = Private;

  if ((This->ConfigString != NULL) && (This->ConfigString[0] != '\0')) {
    HostInterfaceIndex = (UINT8)StrDecimalToUintn (This->ConfigString);
  }

  return WintNtInitializeNetUtilityData (Private, HostInterfaceIndex);
}

/**
  Close SNP thunk protocol.

  @param  This                  Pointer to the thunk protocol instance.

  @retval EFI_SUCCESS           SNP thunk protocol is closed successfully.
  @retval EFI_UNSUPPORTED       This is not SNP thunk protocol.
  @retval other                 Other errors.

**/
EFI_STATUS
WinNtSnpThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  )
{
  WIN_NT_SNP_PRIVATE  *Private;

  if (!CompareGuid (This->Protocol, &gEmuSnpProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = This->Private;
  WintNtReleaseNetUtilityData (Private);
  free (Private);

  return EFI_SUCCESS;
}

EMU_IO_THUNK_PROTOCOL  mWinNtSnpThunkIo = {
  &gEmuSnpProtocolGuid,
  NULL,
  NULL,
  0,
  WinNtSnpThunkOpen,
  WinNtSnpThunkClose,
  NULL
};
