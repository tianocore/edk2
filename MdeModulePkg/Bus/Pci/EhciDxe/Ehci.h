/** @file

  Provides some data struct used by EHCI controller driver.

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_EHCI_H_
#define _EFI_EHCI_H_


#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/PciIo.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Pci22.h>

typedef struct _USB2_HC_DEV  USB2_HC_DEV;

#include "UsbHcMem.h"
#include "EhciReg.h"
#include "EhciUrb.h"
#include "EhciSched.h"
#include "EhciDebug.h"
#include "ComponentName.h"

typedef enum {
  EHC_1_MICROSECOND            = 1,
  EHC_1_MILLISECOND            = 1000 * EHC_1_MICROSECOND,
  EHC_1_SECOND                 = 1000 * EHC_1_MILLISECOND,

  //
  // EHCI register operation timeout, set by experience
  //
  EHC_RESET_TIMEOUT            = 1 * EHC_1_SECOND,
  EHC_GENERIC_TIMEOUT          = 10 * EHC_1_MILLISECOND,

  //
  // Wait for roothub port power stable, refers to Spec[EHCI1.0-2.3.9]
  //
  EHC_ROOT_PORT_RECOVERY_STALL = 20 * EHC_1_MILLISECOND,

  //
  // Sync and Async transfer polling interval, set by experience,
  // and the unit of Async is 100us, means 50ms as interval.
  //
  EHC_SYNC_POLL_INTERVAL       = 20 * EHC_1_MICROSECOND,
  EHC_ASYNC_POLL_INTERVAL      = 50 * 10000U
} EHC_TIMEOUT_EXPERIENCE_VALUE;


  //
  // EHC raises TPL to TPL_NOTIFY to serialize all its operations
  // to protect shared data structures.
  //
#define  EHC_TPL                TPL_NOTIFY

#define  USB2_HC_DEV_SIGNATURE  SIGNATURE_32 ('e', 'h', 'c', 'i')

//
//Iterate through the doule linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
//Iterate through the doule linked list. This is delete-safe.
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

#define EHC_FROM_THIS(a)   CR(a, USB2_HC_DEV, Usb2Hc, USB2_HC_DEV_SIGNATURE)

struct _USB2_HC_DEV {
  UINTN                     Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;

  EFI_PCI_IO_PROTOCOL       *PciIo;
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
  // Asynchronous(bulk and control) transfer schedule data:
  // ReclaimHead is used as the head of the asynchronous transfer
  // list. It acts as the reclamation header.
  //
  EHC_QH                   *ReclaimHead;

  //
  // Peroidic (interrupt) transfer schedule data:
  //
  VOID                      *PeriodFrame;     // Mapped as common buffer
  VOID                      *PeriodFrameHost;
  VOID                      *PeriodFrameMap;

  EHC_QH                    *PeriodOne;
  LIST_ENTRY                AsyncIntTransfers;

  //
  // EHCI configuration data
  //
  UINT32                    HcStructParams; // Cache of HC structure parameter, EHC_HCSPARAMS_OFFSET
  UINT32                    HcCapParams;    // Cache of HC capability parameter, HCCPARAMS
  UINT32                    CapLen;         // Capability length
  UINT32                    High32bitAddr;

  //
  // Misc
  //
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
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
EhcDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );

#endif

