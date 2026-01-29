/** @file
  Header file contains code for USB Ethernet Control Model
  driver definitions

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef USB_CDC_ECM_H_
#define USB_CDC_ECM_H_

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

typedef struct {
  UINTN                          Signature;
  EDKII_USB_ETHERNET_PROTOCOL    UsbEth;
  EFI_HANDLE                     UsbCdcDataHandle;
  EFI_USB_IO_PROTOCOL            *UsbIo;
  EFI_USB_CONFIG_DESCRIPTOR      *Config;
  UINT8                          NumOfInterface;
  UINT8                          BulkInEndpoint;
  UINT8                          BulkOutEndpoint;
  UINT8                          InterruptEndpoint;
  EFI_MAC_ADDRESS                MacAddress;
} USB_ETHERNET_DRIVER;

#define USB_ECM_DRIVER_VERSION         1
#define USB_ETHERNET_BULK_TIMEOUT      1
#define USB_ETHERNET_TRANSFER_TIMEOUT  200

#define USB_ETHERNET_SIGNATURE  SIGNATURE_32('u', 'e', 't', 'h')
#define USB_ETHERNET_DEV_FROM_THIS(a)  CR (a, USB_ETHERNET_DRIVER, UsbEth, USB_ETHERNET_SIGNATURE)

typedef struct {
  UINT16    Src;
  UINT16    Dst;
} BIT_MAP;

extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbEcmComponentName2;

EFI_STATUS
EFIAPI
UsbEcmDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbEcmDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbEcmDriverStop (
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
  IN OUT  USB_ETHERNET_DRIVER  *UsbEthDriver
  );

EFI_STATUS
EFIAPI
UsbEthEcmReceive (
  IN     PXE_CDB                      *Cdb,
  IN     EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN OUT VOID                         *Packet,
  IN OUT UINTN                        *PacketLength
  );

EFI_STATUS
EFIAPI
UsbEthEcmTransmit (
  IN      PXE_CDB                      *Cdb,
  IN      EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN      VOID                         *Packet,
  IN OUT  UINTN                        *PacketLength
  );

EFI_STATUS
EFIAPI
UsbEthEcmInterrupt (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN BOOLEAN                      IsNewTransfer,
  IN UINTN                        PollingInterval,
  IN EFI_USB_DEVICE_REQUEST       *Request
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
UsbEthEcmBulkSize (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT UINTN                        *BulkSize
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
GetUsbEthFunDescriptor (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  OUT USB_ETHERNET_FUN_DESCRIPTOR  *UsbEthFunDescriptor
  );

EFI_STATUS
EFIAPI
SetUsbEthMcastFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN VOID                         *McastAddr
  );

EFI_STATUS
EFIAPI
SetUsbEthPowerFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value,
  IN UINT16                       Length,
  IN VOID                         *PatternFilter
  );

EFI_STATUS
EFIAPI
GetUsbEthPowerFilter (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN  UINT16                       Value,
  OUT BOOLEAN                      *PatternActive
  );

EFI_STATUS
EFIAPI
SetUsbEthPacketFilter (
  IN EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN UINT16                       Value
  );

EFI_STATUS
EFIAPI
GetUsbEthStatistic (
  IN  EDKII_USB_ETHERNET_PROTOCOL  *This,
  IN  UINT16                       FeatureSelector,
  OUT VOID                         *Statistic
  );

#endif
