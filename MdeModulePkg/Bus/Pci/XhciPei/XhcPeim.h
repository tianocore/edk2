/** @file
Private Header file for Usb Host Controller PEIM

Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RECOVERY_XHC_H_
#define _RECOVERY_XHC_H_

#include <PiPei.h>

#include <Ppi/UsbController.h>
#include <Ppi/Usb2HostController.h>
#include <Ppi/IoMmu.h>
#include <Ppi/EndOfPeiPhase.h>

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
// TRSTRCY delay requirement in usb 2.0 spec chapter 7.1.7.5.
// The unit is microsecond, setting it as 10ms.
//
#define XHC_RESET_RECOVERY_DELAY     (10 * 1000)

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
  // EndOfPei callback is used to stop the XHC DMA operation
  // after exit PEI phase.
  //
  EFI_PEI_NOTIFY_DESCRIPTOR         EndOfPeiNotifyList;

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
  VOID                              *ScratchMap;
  UINT64                            *ScratchEntry;
  UINTN                             *ScratchEntryMap;
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
#define PEI_RECOVERY_USB_XHC_DEV_FROM_THIS_NOTIFY(a) CR (a, PEI_XHC_DEV, EndOfPeiNotifyList, USB_XHC_DEV_SIGNATURE)

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


/**
  Initialize IOMMU.
**/
VOID
IoMmuInit (
  VOID
  );

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
IoMmuMap (
  IN  EDKII_IOMMU_OPERATION Operation,
  IN  VOID                  *HostAddress,
  IN  OUT UINTN             *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
**/
EFI_STATUS
IoMmuUnmap (
  IN VOID                  *Mapping
  );

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateBuffer (
  IN UINTN                  Pages,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
IoMmuFreeBuffer (
  IN UINTN                  Pages,
  IN VOID                   *HostAddress,
  IN VOID                   *Mapping
  );

/**
  Allocates aligned pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  Pages                 The number of pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateAlignedBuffer (
  IN UINTN                  Pages,
  IN UINTN                  Alignment,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

#endif
