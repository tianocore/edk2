/** @file

  Provides some data struct used by EHCI controller driver.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_EHCI_H_
#define _EFI_EHCI_H_


#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/PciIo.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Pci.h>

typedef struct _USB2_HC_DEV  USB2_HC_DEV;

#include "UsbHcMem.h"
#include "EhciReg.h"
#include "EhciUrb.h"
#include "EhciSched.h"
#include "EhciDebug.h"
#include "ComponentName.h"

//
// EHC timeout experience values
//

#define EHC_1_MICROSECOND            1
#define EHC_1_MILLISECOND            (1000 * EHC_1_MICROSECOND)
#define EHC_1_SECOND                 (1000 * EHC_1_MILLISECOND)

//
// EHCI register operation timeout, set by experience
//
#define EHC_RESET_TIMEOUT            (1 * EHC_1_SECOND)
#define EHC_GENERIC_TIMEOUT          (10 * EHC_1_MILLISECOND)

//
// Wait for roothub port power stable, refers to Spec[EHCI1.0-2.3.9]
//
#define EHC_ROOT_PORT_RECOVERY_STALL (20 * EHC_1_MILLISECOND)

//
// Sync and Async transfer polling interval, set by experience,
// and the unit of Async is 100us, means 1ms as interval.
//
#define EHC_SYNC_POLL_INTERVAL       (1 * EHC_1_MILLISECOND)
#define EHC_ASYNC_POLL_INTERVAL      EFI_TIMER_PERIOD_MILLISECONDS(1)

//
// EHCI debug port control status register bit definition
//
#define USB_DEBUG_PORT_IN_USE        BIT10
#define USB_DEBUG_PORT_ENABLE        BIT28
#define USB_DEBUG_PORT_OWNER         BIT30
#define USB_DEBUG_PORT_IN_USE_MASK   (USB_DEBUG_PORT_IN_USE | \
                                      USB_DEBUG_PORT_OWNER)

//
// EHC raises TPL to TPL_NOTIFY to serialize all its operations
// to protect shared data structures.
//
#define  EHC_TPL                     TPL_NOTIFY

//
//Iterate through the double linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
//Iterate through the double linked list. This is delete-safe.
//Don't touch NextEntry
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead)            \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)

#define EFI_LIST_CONTAINER(Entry, Type, Field) BASE_CR(Entry, Type, Field)


#define EHC_LOW_32BIT(Addr64)     ((UINT32)(((UINTN)(Addr64)) & 0XFFFFFFFF))
#define EHC_HIGH_32BIT(Addr64)    ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0XFFFFFFFF))
#define EHC_BIT_IS_SET(Data, Bit) ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define EHC_REG_BIT_IS_SET(Ehc, Offset, Bit) \
          (EHC_BIT_IS_SET(EhcReadOpReg ((Ehc), (Offset)), (Bit)))

#define USB2_HC_DEV_SIGNATURE  SIGNATURE_32 ('e', 'h', 'c', 'i')
#define EHC_FROM_THIS(a)       CR(a, USB2_HC_DEV, Usb2Hc, USB2_HC_DEV_SIGNATURE)

struct _USB2_HC_DEV {
  UINTN                     Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;

  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT64                    OriginalPciAttributes;
  USBHC_MEM_POOL            *MemPool;

  //
  // Schedule data shared between asynchronous and periodic
  // transfers:
  // ShortReadStop, as its name indicates, is used to terminate
  // the short read except the control transfer. EHCI follows
  // the alternative next QTD point when a short read happens.
  // For control transfer, even the short read happens, try the
  // status stage.
  //
  EHC_QTD                  *ShortReadStop;
  EFI_EVENT                 PollTimer;

  //
  // ExitBootServicesEvent is used to stop the EHC DMA operation
  // after exit boot service.
  //
  EFI_EVENT                 ExitBootServiceEvent;

  //
  // Asynchronous(bulk and control) transfer schedule data:
  // ReclaimHead is used as the head of the asynchronous transfer
  // list. It acts as the reclamation header.
  //
  EHC_QH                   *ReclaimHead;

  //
  // Periodic (interrupt) transfer schedule data:
  //
  VOID                      *PeriodFrame;     // the buffer pointed by this pointer is used to store pci bus address of the QH descriptor.
  VOID                      *PeriodFrameHost; // the buffer pointed by this pointer is used to store host memory address of the QH descriptor.
  VOID                      *PeriodFrameMap;

  EHC_QH                    *PeriodOne;
  LIST_ENTRY                AsyncIntTransfers;

  //
  // EHCI configuration data
  //
  UINT32                    HcStructParams; // Cache of HC structure parameter, EHC_HCSPARAMS_OFFSET
  UINT32                    HcCapParams;    // Cache of HC capability parameter, HCCPARAMS
  UINT32                    CapLen;         // Capability length

  //
  // Misc
  //
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;

  //
  // EHCI debug port info
  //
  UINT16                    DebugPortOffset; // The offset of debug port mmio register
  UINT8                     DebugPortBarNum; // The bar number of debug port mmio register
  UINT8                     DebugPortNum;    // The port number of usb debug port

  BOOLEAN                   Support64BitDma; // Whether 64 bit DMA may be used with this device
};


extern EFI_DRIVER_BINDING_PROTOCOL      gEhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL      gEhciComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL     gEhciComponentName2;

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
EhcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

/**
  Starting the Usb EHCI Driver.

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
EhcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
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
EhcDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );

#endif

