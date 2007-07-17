/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Ehci.h

Abstract:


Revision History

**/

#ifndef _EFI_EHCI_H_
#define _EFI_EHCI_H_

//
// The package level header files this module uses
//
#include <PiDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/Usb2HostController.h>
#include <Protocol/PciIo.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>


#include <IndustryStandard/Pci22.h>

typedef struct _USB2_HC_DEV  USB2_HC_DEV;

#include "UsbHcMem.h"
#include "EhciReg.h"
#include "EhciUrb.h"
#include "EhciSched.h"
#include "EhciDebug.h"

enum {
  USB2_HC_DEV_SIGNATURE     = EFI_SIGNATURE_32 ('e', 'h', 'c', 'i'),
  EHC_STALL_1_MICROSECOND   = 1,
  EHC_STALL_1_MILLISECOND   = 1000 * EHC_STALL_1_MICROSECOND,
  EHC_STALL_1_SECOND        = 1000 * EHC_STALL_1_MILLISECOND,

  EHC_SET_PORT_RESET_TIME   = 50 * EHC_STALL_1_MILLISECOND,
  EHC_CLEAR_PORT_RESET_TIME = EHC_STALL_1_MILLISECOND,
  EHC_GENERIC_TIME          = 10 * EHC_STALL_1_MILLISECOND,
  EHC_SYNC_POLL_TIME        = 20 * EHC_STALL_1_MICROSECOND,
  EHC_ASYNC_POLL_TIME       = 50 * 10000UL,                 // The unit of time is 100us

  EHC_TPL                   = TPL_NOTIFY
};

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

#define EFI_LIST_CONTAINER(Entry, Type, Field) _CR(Entry, Type, Field)


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


extern EFI_DRIVER_BINDING_PROTOCOL     gEhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL     gEhciComponentName;

#endif
