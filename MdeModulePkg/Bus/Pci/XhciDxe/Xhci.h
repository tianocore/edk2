/** @file

  Provides some data structure definitions used by the XHCI host controller driver.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_XHCI_H_
#define _EFI_XHCI_H_

#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/PciIo.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Pci.h>

typedef struct _USB_XHCI_DEV    USB_XHCI_DEV;
typedef struct _USB_DEV_CONTEXT USB_DEV_CONTEXT;

#include "XhciReg.h"
#include "XhciSched.h"
#include "ComponentName.h"

//
// XHC timeout experience values
//
#define XHC_1_MICROSECOND            1
#define XHC_1_MILLISECOND            (1000 * XHC_1_MICROSECOND)
#define XHC_1_SECOND                 (1000 * XHC_1_MILLISECOND)

//
// XHCI register operation timeout, set by experience
//
#define XHC_RESET_TIMEOUT            (1 * XHC_1_SECOND)
#define XHC_GENERIC_TIMEOUT          (10 * XHC_1_MILLISECOND)

//
// Wait for roothub port power stable, refers to Spec[XHCI1.0-2.3.9]
//
#define XHC_ROOT_PORT_RECOVERY_STALL (20 * XHC_1_MILLISECOND)

//
// Sync and Async transfer polling interval, set by experience,
// and the unit of Async is 100us, means 50ms as interval.
//
#define XHC_SYNC_POLL_INTERVAL       (20 * XHC_1_MILLISECOND)
#define XHC_ASYNC_POLL_INTERVAL      (50 * 10000U)

//
// XHC raises TPL to TPL_NOTIFY to serialize all its operations
// to protect shared data structures.
//
#define XHC_TPL                      TPL_NOTIFY

#define CMD_RING_TRB_NUMBER          0x40
#define TR_RING_TRB_NUMBER           0x40
#define ERST_NUMBER                  0x01
#define EVENT_RING_TRB_NUMBER        0x80

#define CMD_INTER                    0
#define CTRL_INTER                   1
#define BULK_INTER                   2
#define INT_INTER                    3
#define INT_INTER_ASYNC              4

//
// Iterate through the doule linked list. This is delete-safe.
// Don't touch NextEntry
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead) \
  for (Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)

#define EFI_LIST_CONTAINER(Entry, Type, Field) BASE_CR(Entry, Type, Field)

#define XHC_LOW_32BIT(Addr64)        ((UINT32)(((UINTN)(Addr64)) & 0xFFFFFFFF))
#define XHC_HIGH_32BIT(Addr64)       ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0xFFFFFFFF))
#define XHC_BIT_IS_SET(Data, Bit)    ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define XHC_REG_BIT_IS_SET(Xhc, Offset, Bit) \
          (XHC_BIT_IS_SET(XhcReadOpReg ((Xhc), (Offset)), (Bit)))

#define XHCI_IS_DATAIN(EndpointAddr) XHC_BIT_IS_SET((EndpointAddr), 0x80)

#define USB_XHCI_DEV_SIGNATURE       SIGNATURE_32 ('x', 'h', 'c', 'i')
#define XHC_FROM_THIS(a)             CR(a, USB_XHCI_DEV, Usb2Hc, USB_XHCI_DEV_SIGNATURE)

#define USB_DESC_TYPE_HUB              0x29
#define USB_DESC_TYPE_HUB_SUPER_SPEED  0x2a

//
// Xhci Data and Ctrl Structures
//
#pragma pack(1)
typedef struct {
  UINT8                     ProgInterface;
  UINT8                     SubClassCode;
  UINT8                     BaseCode;
} USB_CLASSC;

typedef struct {
  UINT8                     Length;
  UINT8                     DescType;
  UINT8                     NumPorts;
  UINT16                    HubCharacter;
  UINT8                     PwrOn2PwrGood;
  UINT8                     HubContrCurrent;
  UINT8                     Filler[16];
} EFI_USB_HUB_DESCRIPTOR;
#pragma pack()

struct _USB_XHCI_DEV {
  UINT32                    Signature;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT64                    OriginalPciAttributes;

  EFI_USB2_HC_PROTOCOL      Usb2Hc;

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  //
  // ExitBootServicesEvent is used to set OS semaphore and
  // stop the XHC DMA operation after exit boot service.
  //
  EFI_EVENT                 ExitBootServiceEvent;
  EFI_EVENT                 PollTimer;
  LIST_ENTRY                AsyncIntTransfers;

  UINT8                     CapLength;    ///< Capability Register Length
  XHC_HCSPARAMS1            HcSParams1;   ///< Structural Parameters 1
  XHC_HCSPARAMS2            HcSParams2;   ///< Structural Parameters 2
  XHC_HCCPARAMS             HcCParams;    ///< Capability Parameters
  UINT32                    DBOff;        ///< Doorbell Offset
  UINT32                    RTSOff;       ///< Runtime Register Space Offset
  UINT16                    MaxInterrupt;
  UINT32                    PageSize;
  UINT64                    *ScratchBuf;
  UINT32                    MaxScratchpadBufs;
  UINT32                    ExtCapRegBase;
  UINT32                    UsbLegSupOffset;
  UINT64                    *DCBAA;
  UINT32                    MaxSlotsEn;
  //
  // Cmd Transfer Ring
  //
  TRANSFER_RING             CmdRing;
  //
  // CmdEventRing
  //
  EVENT_RING                CmdEventRing;
  //
  // ControlTREventRing
  //
  EVENT_RING                CtrlTrEventRing;
  //
  // BulkTREventRing
  //
  EVENT_RING                BulkTrEventRing;
  //
  // IntTREventRing
  //
  EVENT_RING                IntTrEventRing;
  //
  // AsyncIntTREventRing
  //
  EVENT_RING                AsynIntTrEventRing;
  //
  // Misc
  //
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;

};

struct _USB_DEV_CONTEXT {
  //
  // Whether this entry in UsbDevContext array is used or not.
  //
  BOOLEAN                   Enabled;
  //
  // The slot id assigned to the new device through XHCI's Enable_Slot cmd.
  //
  UINT8                     SlotId;
  //
  // The route string presented an attached usb device.
  //
  USB_DEV_ROUTE             RouteString;
  //
  // The route string of parent device if it exists. Otherwise it's zero.
  //
  USB_DEV_ROUTE             ParentRouteString;
  //
  // The actual device address assigned by XHCI through Address_Device command.
  //
  UINT8                     XhciDevAddr;
  //
  // The requested device address from UsbBus driver through Set_Address standard usb request.
  // As XHCI spec replaces this request with Address_Device command, we have to record the
  // requested device address and establish a mapping relationship with the actual device address.
  // Then UsbBus driver just need to be aware of the requested device address to access usb device
  // through EFI_USB2_HC_PROTOCOL. Xhci driver would be responsible for translating it to actual
  // device address and access the actual device.
  //
  UINT8                     BusDevAddr;
  //
  // The pointer to the input device context.
  //
  VOID                      *InputContext;
  //
  // The pointer to the output device context.
  //
  VOID                      *OutputDevContxt;
  //
  // The transfer queue for every endpoint.
  //
  VOID                      *EndpointTransferRing[31];
  //
  // The device descriptor which is stored to support XHCI's Evaluate_Context cmd.
  //
  EFI_USB_DEVICE_DESCRIPTOR DevDesc;
  //
  // As a usb device may include multiple configuration descriptors, we dynamically allocate an array
  // to store them.
  // Note that every configuration descriptor stored here includes those lower level descriptors,
  // such as Interface descriptor, Endpoint descriptor, and so on.
  // These information is used to support XHCI's Config_Endpoint cmd.
  //
  EFI_USB_CONFIG_DESCRIPTOR **ConfDesc;
};

extern EFI_DRIVER_BINDING_PROTOCOL      gXhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL      gXhciComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL     gXhciComponentName2;
extern USB_DEV_CONTEXT                  UsbDevContext[];

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has Usb2HcProtocol installed will
  be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
XhcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

/**
  Starting the Usb XHCI Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          supports this device.
  @return EFI_UNSUPPORTED      do not support this device.
  @return EFI_DEVICE_ERROR     cannot be started due to device Error.
  @return EFI_OUT_OF_RESOURCES cannot allocate resources.

**/
EFI_STATUS
EFIAPI
XhcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS          Success.
  @return EFI_DEVICE_ERROR     Fail.

**/
EFI_STATUS
EFIAPI
XhcDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );

/**
  Sets a feature for the specified root hub port.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Root hub port to set.
  @param  PortFeature           Feature to set.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcSetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  );

/**
  Clears a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port whose feature is
                                requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared
                                for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  );

#endif
