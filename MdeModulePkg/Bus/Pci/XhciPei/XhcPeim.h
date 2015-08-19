/** @file
Private Header file for Usb Host Controller PEIM

Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _RECOVERY_XHC_H_
#define _RECOVERY_XHC_H_

#include <PiPei.h>

#include <Ppi/UsbController.h>
#include <Ppi/Usb2HostController.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>

typedef struct _PEI_XHC_DEV PEI_XHC_DEV;
typedef struct _USB_DEV_CONTEXT USB_DEV_CONTEXT;

#include "UsbHcMem.h"
#include "XhciReg.h"
#include "XhciSched.h"

#define CMD_RING_TRB_NUMBER         0x100
#define TR_RING_TRB_NUMBER          0x100
#define ERST_NUMBER                 0x01
#define EVENT_RING_TRB_NUMBER       0x200

#define XHC_1_MICROSECOND           1
#define XHC_1_MILLISECOND           (1000 * XHC_1_MICROSECOND)
#define XHC_1_SECOND                (1000 * XHC_1_MILLISECOND)

//
// XHC reset timeout experience values.
// The unit is millisecond, setting it as 1s.
//
#define XHC_RESET_TIMEOUT           (1000)

//
// Wait for root port state stable.
//
#define XHC_ROOT_PORT_STATE_STABLE  (200 * XHC_1_MILLISECOND)

//
// XHC generic timeout experience values.
// The unit is millisecond, setting it as 10s.
//
#define XHC_GENERIC_TIMEOUT         (10 * 1000)

#define XHC_LOW_32BIT(Addr64)       ((UINT32)(((UINTN)(Addr64)) & 0XFFFFFFFF))
#define XHC_HIGH_32BIT(Addr64)      ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0XFFFFFFFF))
#define XHC_BIT_IS_SET(Data, Bit)   ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define XHC_REG_BIT_IS_SET(XHC, Offset, Bit) \
          (XHC_BIT_IS_SET(XhcPeiReadOpReg ((XHC), (Offset)), (Bit)))

#define USB_DESC_TYPE_HUB              0x29
#define USB_DESC_TYPE_HUB_SUPER_SPEED  0x2a

//
// The RequestType in EFI_USB_DEVICE_REQUEST is composed of
// three fields: One bit direction, 2 bit type, and 5 bit
// target.
//
#define USB_REQUEST_TYPE(Dir, Type, Target) \
          ((UINT8)((((Dir) == EfiUsbDataIn ? 0x01 : 0) << 7) | (Type) | (Target)))

struct _USB_DEV_CONTEXT {
  //
  // Whether this entry in UsbDevContext array is used or not.
  //
  BOOLEAN                           Enabled;
  //
  // The slot id assigned to the new device through XHCI's Enable_Slot cmd.
  //
  UINT8                             SlotId;
  //
  // The route string presented an attached usb device.
  //
  USB_DEV_ROUTE                     RouteString;
  //
  // The route string of parent device if it exists. Otherwise it's zero.
  //
  USB_DEV_ROUTE                     ParentRouteString;
  //
  // The actual device address assigned by XHCI through Address_Device command.
  //
  UINT8                             XhciDevAddr;
  //
  // The requested device address from UsbBus driver through Set_Address standard usb request.
  // As XHCI spec replaces this request with Address_Device command, we have to record the
  // requested device address and establish a mapping relationship with the actual device address.
  // Then UsbBus driver just need to be aware of the requested device address to access usb device
  // through EFI_USB2_HC_PROTOCOL. Xhci driver would be responsible for translating it to actual
  // device address and access the actual device.
  //
  UINT8                             BusDevAddr;
  //
  // The pointer to the input device context.
  //
  VOID                              *InputContext;
  //
  // The pointer to the output device context.
  //
  VOID                              *OutputContext;
  //
  // The transfer queue for every endpoint.
  //
  VOID                              *EndpointTransferRing[31];
  //
  // The device descriptor which is stored to support XHCI's Evaluate_Context cmd.
  //
  EFI_USB_DEVICE_DESCRIPTOR         DevDesc;
  //
  // As a usb device may include multiple configuration descriptors, we dynamically allocate an array
  // to store them.
  // Note that every configuration descriptor stored here includes those lower level descriptors,
  // such as Interface descriptor, Endpoint descriptor, and so on.
  // These information is used to support XHCI's Config_Endpoint cmd.
  //
  EFI_USB_CONFIG_DESCRIPTOR         **ConfDesc;
};

#define USB_XHC_DEV_SIGNATURE       SIGNATURE_32 ('x', 'h', 'c', 'i')

struct _PEI_XHC_DEV {
  UINTN                             Signature;
  PEI_USB2_HOST_CONTROLLER_PPI      Usb2HostControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR            PpiDescriptor;
  UINT32                            UsbHostControllerBaseAddress;
  USBHC_MEM_POOL                    *MemPool;

  //
  // XHCI configuration data
  //
  UINT8                             CapLength;    ///< Capability Register Length
  XHC_HCSPARAMS1                    HcSParams1;   ///< Structural Parameters 1
  XHC_HCSPARAMS2                    HcSParams2;   ///< Structural Parameters 2
  XHC_HCCPARAMS                     HcCParams;    ///< Capability Parameters
  UINT32                            DBOff;        ///< Doorbell Offset
  UINT32                            RTSOff;       ///< Runtime Register Space Offset
  UINT32                            PageSize;
  UINT32                            MaxScratchpadBufs;
  UINT64                            *ScratchBuf;
  UINT64                            *ScratchEntry;
  UINT64                            *DCBAA;
  UINT32                            MaxSlotsEn;
  //
  // Cmd Transfer Ring
  //
  TRANSFER_RING                     CmdRing;
  //
  // EventRing
  //
  EVENT_RING                        EventRing;

  //
  // Store device contexts managed by XHCI device
  // The array supports up to 255 devices, entry 0 is reserved and should not be used.
  //
  USB_DEV_CONTEXT                   UsbDevContext[256];
};

#define PEI_RECOVERY_USB_XHC_DEV_FROM_THIS(a) CR (a, PEI_XHC_DEV, Usb2HostControllerPpi, USB_XHC_DEV_SIGNATURE)

/**
  Initialize the memory management pool for the host controller.

  @return Pointer to the allocated memory pool or NULL if failed.

**/
USBHC_MEM_POOL *
UsbHcInitMemPool (
  VOID
  )
;

/**
  Release the memory management pool.

  @param  Pool          The USB memory pool to free.

**/
VOID
UsbHcFreeMemPool (
  IN USBHC_MEM_POOL     *Pool
  )
;

/**
  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

  @param  Pool          The host controller's memory pool.
  @param  Size          Size of the memory to allocate.

  @return The allocated memory or NULL.

**/
VOID *
UsbHcAllocateMem (
  IN USBHC_MEM_POOL     *Pool,
  IN UINTN              Size
  )
;

/**
  Free the allocated memory back to the memory pool.

  @param  Pool          The memory pool of the host controller.
  @param  Mem           The memory to free.
  @param  Size          The size of the memory to free.

**/
VOID
UsbHcFreeMem (
  IN USBHC_MEM_POOL     *Pool,
  IN VOID               *Mem,
  IN UINTN              Size
  )
;

#endif
