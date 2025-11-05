/** @file
  Header file for for USB Rndis driver

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef USB_RNDIS_H_
#define USB_RNDIS_H_

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiUsbLib.h>
#include <Protocol/UsbIo.h>
#include <Protocol/UsbEthernetProtocol.h>

typedef struct _REMOTE_NDIS_MSG_HEADER REMOTE_NDIS_MSG_HEADER;

typedef struct {
  UINT32                         Signature;
  EDKII_USB_ETHERNET_PROTOCOL    UsbEth;
  EFI_HANDLE                     UsbCdcDataHandle;
  EFI_HANDLE                     UsbRndisHandle;
  EFI_USB_IO_PROTOCOL            *UsbIo;
  EFI_USB_IO_PROTOCOL            *UsbIoCdcData;
  EFI_USB_CONFIG_DESCRIPTOR      *Config;
  UINT8                          NumOfInterface;
  UINT8                          BulkInEndpoint;
  UINT8                          BulkOutEndpoint;
  UINT8                          InterrupEndpoint;
  EFI_MAC_ADDRESS                MacAddress;
  UINT32                         RequestId;
  UINT32                         Medium;
  UINT32                         MaxPacketsPerTransfer;
  UINT32                         MaxTransferSize;
  UINT32                         PacketAlignmentFactor;
  LIST_ENTRY                     ReceivePacketList;
} USB_RNDIS_DEVICE;

#define USB_RNDIS_DRIVER_VERSION       1
#define USB_TX_ETHERNET_BULK_TIMEOUT   3000
#define USB_RX_ETHERNET_BULK_TIMEOUT   3
#define USB_ETHERNET_TRANSFER_TIMEOUT  200

#define LAN_BULKIN_CMD_CONTROL      1
#define MAXIMUM_STOPBULKIN_CNT      300        // Indicating maximum counts for waiting bulk in command
#define MINIMUM_STOPBULKIN_CNT      3          // Indicating minimum counts for waiting bulk in command
#define BULKIN_CMD_POLLING_CNT      300        // Indicating the waiting counts for send bulk in command when system pending
#define RNDIS_RESERVED_BYTE_LENGTH  8

#define USB_RNDIS_SIGNATURE  SIGNATURE_32('r', 'n', 'd', 's')
#define USB_RNDIS_DEVICE_FROM_THIS(a)  CR (a, USB_RNDIS_DEVICE, UsbEth, USB_RNDIS_SIGNATURE)

extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbRndisComponentName2;

typedef struct {
  UINT16    Src;
  UINT16    Dst;
} BIT_MAP;

EFI_STATUS
EFIAPI
UsbRndisDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbRndisDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbRndisDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

EFI_STATUS
LoadAllDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  OUT EFI_USB_CONFIG_DESCRIPTOR  **ConfigDesc
  );

BOOLEAN
NextDescriptor (
  IN EFI_USB_CONFIG_DESCRIPTOR  *Desc,
  IN OUT UINTN                  *Offset
  );

EFI_STATUS
GetFunctionalDescriptor (
  IN  EFI_USB_CONFIG_DESCRIPTOR  *Config,
  IN  UINT8                      FunDescriptorType,
  OUT VOID                       *DataBuffer
  );

VOID
GetEndpoint (
  IN      EFI_USB_IO_PROTOCOL  *UsbIo,
  IN OUT  USB_RNDIS_DEVICE     *UsbRndisDevice
  );

EFI_STATUS
EFIAPI
UsbRndisInterrupt (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN BOOLEAN                      IsNewTransfer,
  IN UINTN                        PollingInterval,
  IN EFI_USB_DEVICE_REQUEST       *Requst
  );

EFI_STATUS
EFIAPI
InterruptCallback (
  IN  VOID    *Data,
  IN  UINTN   DataLength,
  IN  VOID    *Context,
  IN  UINT32  Status
  );

EFI_STATUS
EFIAPI
GetUsbEthMacAddress (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT EFI_MAC_ADDRESS              *MacAddress
  );

EFI_STATUS
EFIAPI
UsbEthBulkSize (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT UINTN                        *BulkSize
  );

EFI_STATUS
EFIAPI
RndisDummyReturn (
  IN  PXE_CDB   *Cdb,
  IN  NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiStart (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiStop (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiGetInitInfo (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiGetConfigInfo (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiInitialize (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiTransmit (
  IN      PXE_CDB                      *Cdb,
  IN      EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN      VOID                         *BulkOutData,
  IN OUT  UINTN                        *DataLength
  );

EFI_STATUS
EFIAPI
RndisUndiReceive (
  IN     PXE_CDB                      *Cdb,
  IN     EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN OUT VOID                         *BulkInData,
  IN OUT UINTN                        *DataLength
  );

EFI_STATUS
EFIAPI
RndisUndiReset (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiShutdown (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiReceiveFilter (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
RndisUndiGetStatus (
  IN PXE_CDB   *Cdb,
  IN NIC_DATA  *Nic
  );

EFI_STATUS
EFIAPI
GetUsbHeaderFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_HEADER_FUN_DESCRIPTOR    *UsbHeaderFunDescriptor
  );

EFI_STATUS
EFIAPI
GetUsbUnionFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_UNION_FUN_DESCRIPTOR     *UsbUnionFunDescriptor
  );

EFI_STATUS
EFIAPI
GetUsbRndisFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_ETHERNET_FUN_DESCRIPTOR  *UsbEthFunDescriptor
  );

EFI_STATUS
EFIAPI
SetUsbRndisMcastFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN VOID                         *McastAddr
  );

EFI_STATUS
EFIAPI
SetUsbRndisPowerFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN UINT16                       Length,
  IN VOID                         *PatternFilter
  );

EFI_STATUS
EFIAPI
GetUsbRndisPowerFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN BOOLEAN                      *PatternActive
  );

EFI_STATUS
EFIAPI
SetUsbRndisPacketFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value
  );

EFI_STATUS
EFIAPI
GetRndisStatistic (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN VOID                         *Statistic
  );

EFI_STATUS
RndisControlMsg (
  IN  USB_RNDIS_DEVICE        *UsbRndisDevice,
  IN  REMOTE_NDIS_MSG_HEADER  *RndisMsg,
  OUT REMOTE_NDIS_MSG_HEADER  *RndisMsgResponse
  );

EFI_STATUS
RndisTransmitDataMsg (
  IN  USB_RNDIS_DEVICE        *UsbRndisDevice,
  IN  REMOTE_NDIS_MSG_HEADER  *RndisMsg,
  UINTN                       *TransferLength
  );

EFI_STATUS
RndisReceiveDataMsg (
  IN  USB_RNDIS_DEVICE        *UsbRndisDevice,
  IN  REMOTE_NDIS_MSG_HEADER  *RndisMsg,
  UINTN                       *TransferLength
  );

VOID
PrintRndisMsg (
  IN  REMOTE_NDIS_MSG_HEADER  *RndisMsg
  );

#define RNDIS_MAJOR_VERSION      0x00000001
#define RNDIS_MINOR_VERSION      0x00000000
#define RNDIS_MAX_TRANSFER_SIZE  0x4000

#define RNDIS_PACKET_MSG           0x00000001
#define RNDIS_INITIALIZE_MSG       0x00000002
#define RNDIS_INITIALIZE_CMPLT     0x80000002
#define RNDIS_HLT_MSG              0x00000003
#define RNDIS_QUERY_MSG            0x00000004
#define RNDIS_QUERY_CMPLT          0x80000004
#define RNDIS_SET_MSG              0x00000005
#define RNDIS_SET_CMPLT            0x80000005
#define RNDIS_RESET_MSG            0x00000006
#define RNDIS_RESET_CMPLT          0x80000006
#define RNDIS_INDICATE_STATUS_MSG  0x00000007
#define RNDIS_KEEPALIVE_MSG        0x00000008
#define RNDIS_KEEPALIVE_CMPLT      0x80000008

#define RNDIS_STATUS_SUCCESS           0x00000000
#define RNDIS_STATUS_FAILURE           0xC0000001
#define RNDIS_STATUS_INVALID_DATA      0xC0010015
#define RNDIS_STATUS_NOT_SUPPORTED     0xC00000BB
#define RNDIS_STATUS_MEDIA_CONNECT     0x4001000B
#define RNDIS_STATUS_MEDIA_DISCONNECT  0x4001000C

#define RNDIS_CONTROL_TIMEOUT    10000            // 10sec
#define RNDIS_KEEPALIVE_TIMEOUT  5000             // 5sec

#define SEND_ENCAPSULATED_COMMAND  0x00000000
#define GET_ENCAPSULATED_RESPONSE  0x00000001

//
// General Objects
//
// Taken from NTDDNDIS.H
#define OID_GEN_SUPPORTED_LIST         0x00010101
#define OID_GEN_HARDWARE_STATUS        0x00010102
#define OID_GEN_MEDIA_SUPPORTED        0x00010103
#define OID_GEN_MEDIA_IN_USE           0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD      0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE     0x00010106
#define OID_GEN_LINK_SPEED             0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE  0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE   0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE    0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE     0x0001010B
#define OID_GEN_VENDOR_ID              0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION     0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER  0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD      0x0001010F
#define OID_GEN_DRIVER_VERSION         0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE     0x00010111
#define OID_GEN_PROTOCOL_OPTIONS       0x00010112
#define OID_GEN_MAC_OPTIONS            0x00010113
#define OID_GEN_MEDIA_CONNECT_STATUS   0x00010114
#define OID_GEN_MAXIMUM_SEND_PACKETS   0x00010115
#define OID_GEN_VENDOR_DRIVER_VERSION  0x00010116

#define OID_GEN_XMIT_OK        0x00020101
#define OID_GEN_RCV_OK         0x00020102
#define OID_GEN_XMIT_ERROR     0x00020103
#define OID_GEN_RCV_ERROR      0x00020104
#define OID_GEN_RCV_NO_BUFFER  0x00020105

#define OID_GEN_DIRECTED_BYTES_XMIT    0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT   0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT   0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT  0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT   0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT  0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV     0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV    0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV    0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV   0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV    0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV   0x0002020C
#define OID_GEN_RCV_CRC_ERROR          0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH  0x0002020E

#define OID_802_3_CURRENT_ADDRESS  0x01010102
//
// Ndis Packet Filter Bits (OID_GEN_CURRENT_PACKET_FILTER).
//
#define NDIS_PACKET_TYPE_DIRECTED        0x0001
#define NDIS_PACKET_TYPE_MULTICAST       0x0002
#define NDIS_PACKET_TYPE_ALL_MULTICAST   0x0004
#define NDIS_PACKET_TYPE_BROADCAST       0x0008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING  0x0010
#define NDIS_PACKET_TYPE_PROMISCUOUS     0x0020
#define NDIS_PACKET_TYPE_SMT             0x0040
#define NDIS_PACKET_TYPE_ALL_LOCAL       0x0080
#define NDIS_PACKET_TYPE_MAC_FRAME       0x8000
#define NDIS_PACKET_TYPE_FUNCTIONAL      0x4000
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL  0x2000
#define NDIS_PACKET_TYPE_GROUP           0x1000

#pragma pack(1)

typedef struct _REMOTE_NDIS_MSG_HEADER {
  UINT32    MessageType;
  UINT32    MessageLength;
} REMOTE_NDIS_MSG_HEADER;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
  UINT32    MajorVersion;
  UINT32    MinorVersion;
  UINT32    MaxTransferSize;
} REMOTE_NDIS_INITIALIZE_MSG;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
} REMOTE_NDIS_HALT_MSG;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
  UINT32    Oid;
  UINT32    InformationBufferLength;
  UINT32    InformationBufferOffset;
  UINT32    Reserved;
} REMOTE_NDIS_QUERY_MSG;

typedef struct {
  REMOTE_NDIS_QUERY_MSG    QueryMsg;
  UINT8                    Addr[6];
} REMOTE_NDIS_QUERY_MAC_MSG;

typedef struct {
  REMOTE_NDIS_QUERY_MSG    QueryMsg;
  UINT32                   MaxTotalSize;
} REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_MSG;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
  UINT32    Oid;
  UINT32    InformationBufferLength;
  UINT32    InformationBufferOffset;
  UINT32    Reserved;
} REMOTE_NDIS_SET_MSG;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    Reserved;
} REMOTE_NDIS_RESET_MSG;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    Status;
  UINT32    StatusBufferLength;
  UINT32    StatusBufferOffset;
} REMOTE_NDIS_INDICATE_STATUS_MSG;

typedef struct {
  UINT32    DiagStatus;
  UINT32    ErrorOffset;
} RNDIS_DIAGNOSTIC_INFO;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
} REMOTE_NDIS_KEEPALIVE_MSG;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
  UINT32    Status;
  UINT32    MajorVersion;
  UINT32    MinorVersion;
  UINT32    DeviceFlags;
  UINT32    Medium;
  UINT32    MaxPacketsPerTransfer;
  UINT32    MaxTransferSize;
  UINT32    PacketAlignmentFactor;
  UINT64    Reserved;
} REMOTE_NDIS_INITIALIZE_CMPLT;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
  UINT32    Status;
  UINT32    InformationBufferLength;
  UINT32    InformationBufferOffset;
} REMOTE_NDIS_QUERY_CMPLT;

typedef struct {
  REMOTE_NDIS_QUERY_CMPLT    QueryCmplt;
  UINT8                      Addr[6];
} REMOTE_NDIS_QUERY_MAC_CMPLT;

typedef struct {
  REMOTE_NDIS_QUERY_CMPLT    QueryCmplt;
  UINT32                     MaxTotalSize;
} REMOTE_NDIS_QUERY_MAX_TOTAL_SIZE_CMPLT;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
  UINT32    Status;
} REMOTE_NDIS_SET_CMPLT;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    Status;
  UINT32    AddressingReset;
} REMOTE_NDIS_RESET_CMPLT;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    RequestID;
  UINT32    Status;
} REMOTE_NDIS_KEEPALIVE_CMPLT;

typedef struct {
  UINT32    MessageType;
  UINT32    MessageLength;
  UINT32    DataOffset;
  UINT32    DataLength;
  UINT32    OutOfBandDataOffset;
  UINT32    OutOfBandDataLength;
  UINT32    NumOutOfBandDataElements;
  UINT32    PerPacketInfoOffset;
  UINT32    PerPacketInfoLength;
  UINT32    Reserved1;
  UINT32    Reserved2;
} REMOTE_NDIS_PACKET_MSG;

typedef struct {
  UINT32    Size;
  UINT32    Type;
  UINT32    ClassInformationOffset;
} OUT_OF_BAND_DATA_RECORD;

typedef struct {
  UINT32    Size;
  UINT32    Type;
  UINT32    ClassInformationOffset;
} PER_PACKET_INFO_DATA_RECORD;

typedef struct {
  LIST_ENTRY    PacketList;
  UINT8         *OrgBuffer;
  UINTN         RemainingLength;
  UINT8         *PacketStartBuffer;    // Variable size data to follow
} PACKET_LIST;

#pragma pack()

#endif
