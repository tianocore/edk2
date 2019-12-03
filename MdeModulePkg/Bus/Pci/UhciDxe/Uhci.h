/** @file

  The definition for UHCI driver model and HC protocol routines.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_UHCI_H_
#define _EFI_UHCI_H_


#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbHostController.h>
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

typedef struct _USB_HC_DEV  USB_HC_DEV;

#include "UsbHcMem.h"
#include "UhciQueue.h"
#include "UhciReg.h"
#include "UhciSched.h"
#include "UhciDebug.h"
#include "ComponentName.h"

//
// UHC timeout experience values
//

#define UHC_1_MICROSECOND             1
#define UHC_1_MILLISECOND             (1000 * UHC_1_MICROSECOND)
#define UHC_1_SECOND                  (1000 * UHC_1_MILLISECOND)

//
// UHCI register operation timeout, set by experience
//
#define UHC_GENERIC_TIMEOUT           UHC_1_SECOND

//
// Wait for force global resume(FGR) complete, refers to
// specification[UHCI11-2.1.1]
//
#define UHC_FORCE_GLOBAL_RESUME_STALL (20 * UHC_1_MILLISECOND)

//
// Wait for roothub port reset and recovery, reset stall
// is set by experience, and recovery stall refers to
// specification[UHCI11-2.1.1]
//
#define UHC_ROOT_PORT_RESET_STALL     (50 * UHC_1_MILLISECOND)
#define UHC_ROOT_PORT_RECOVERY_STALL  (10 * UHC_1_MILLISECOND)

//
// Sync and Async transfer polling interval, set by experience,
// and the unit of Async is 100us.
//
#define UHC_SYNC_POLL_INTERVAL        (1 * UHC_1_MILLISECOND)
#define UHC_ASYNC_POLL_INTERVAL       EFI_TIMER_PERIOD_MILLISECONDS(1)

//
// UHC raises TPL to TPL_NOTIFY to serialize all its operations
// to protect shared data structures.
//
#define  UHCI_TPL                     TPL_NOTIFY

#define  USB_HC_DEV_SIGNATURE         SIGNATURE_32 ('u', 'h', 'c', 'i')

#pragma pack(1)
typedef struct {
  UINT8               ProgInterface;
  UINT8               SubClassCode;
  UINT8               BaseCode;
} USB_CLASSC;
#pragma pack()

#define UHC_FROM_USB2_HC_PROTO(This)  CR(This, USB_HC_DEV, Usb2Hc, USB_HC_DEV_SIGNATURE)

//
// USB_HC_DEV support the UHCI hardware controller. It schedules
// the asynchronous interrupt transfer with the same method as
// EHCI: a reversed tree structure. For synchronous interrupt,
// control and bulk transfer, it uses three static queue head to
// schedule them. SyncIntQh is for interrupt transfer. LsCtrlQh is
// for LOW speed control transfer, and FsCtrlBulkQh is for FULL
// speed control or bulk transfer. This is because FULL speed contrl
// or bulk transfer can reclaim the unused bandwidth. Some USB
// device requires this bandwidth reclamation capability.
//
struct _USB_HC_DEV {
  UINT32                    Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT64                    OriginalPciAttributes;

  //
  // Schedule data structures
  //
  UINT32                    *FrameBase; // the buffer pointed by this pointer is used to store pci bus address of the QH descriptor.
  UINT32                    *FrameBaseHostAddr; // the buffer pointed by this pointer is used to store host memory address of the QH descriptor.
  UHCI_QH_SW                *SyncIntQh;
  UHCI_QH_SW                *CtrlQh;
  UHCI_QH_SW                *BulkQh;

  //
  // Structures to maintain asynchronus interrupt transfers.
  // When asynchronous interrutp transfer is unlinked from
  // the frame list, the hardware may still hold a pointer
  // to it. To synchronize with hardware, its resoureces are
  // released in two steps using Recycle and RecycleWait.
  // Check the asynchronous interrupt management routines.
  //
  LIST_ENTRY                AsyncIntList;
  EFI_EVENT                 AsyncIntMonitor;
  UHCI_ASYNC_REQUEST        *Recycle;
  UHCI_ASYNC_REQUEST        *RecycleWait;


  UINTN                     RootPorts;
  USBHC_MEM_POOL            *MemPool;
  EFI_UNICODE_STRING_TABLE  *CtrlNameTable;
  VOID                      *FrameMapping;

  //
  // ExitBootServicesEvent is used to stop the EHC DMA operation
  // after exit boot service.
  //
  EFI_EVENT                 ExitBootServiceEvent;
};

extern EFI_DRIVER_BINDING_PROTOCOL   gUhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUhciComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUhciComponentName2;

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has UsbHcProtocol installed will be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
UhciDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

/**
  Starting the Usb UHCI Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @retval EFI_SUCCESS          This driver supports this device.
  @retval EFI_UNSUPPORTED      This driver does not support this device.
  @retval EFI_DEVICE_ERROR     This driver cannot be started due to device Error.
                               EFI_OUT_OF_RESOURCES- Failed due to resource shortage.

**/
EFI_STATUS
EFIAPI
UhciDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
EFIAPI
UhciDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  );

#endif
