/** @file
Private Header file for Usb Host Controller PEIM

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RECOVERY_EHC_H_
#define _RECOVERY_EHC_H_

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

typedef struct _PEI_USB2_HC_DEV PEI_USB2_HC_DEV;

#define EFI_LIST_ENTRY LIST_ENTRY

#include "UsbHcMem.h"
#include "EhciReg.h"
#include "EhciUrb.h"
#include "EhciSched.h"

#define EFI_USB_SPEED_FULL 0x0000
#define EFI_USB_SPEED_LOW  0x0001
#define EFI_USB_SPEED_HIGH 0x0002

#define PAGESIZE           4096

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
// Sync transfer polling interval, set by experience.
//
#define EHC_SYNC_POLL_INTERVAL       (6 * EHC_1_MILLISECOND)

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

struct _PEI_USB2_HC_DEV {
  UINTN                               Signature;
  PEI_USB2_HOST_CONTROLLER_PPI        Usb2HostControllerPpi;
  EDKII_IOMMU_PPI                     *IoMmu;
  EFI_PEI_PPI_DESCRIPTOR              PpiDescriptor;
  //
  // EndOfPei callback is used to stop the EHC DMA operation
  // after exit PEI phase.
  //
  EFI_PEI_NOTIFY_DESCRIPTOR           EndOfPeiNotifyList;
  UINT32                              UsbHostControllerBaseAddress;
  PEI_URB                             *Urb;
  USBHC_MEM_POOL                      *MemPool;

  //
  // Schedule data shared between asynchronous and periodic
  // transfers:
  // ShortReadStop, as its name indicates, is used to terminate
  // the short read except the control transfer. EHCI follows
  // the alternative next QTD point when a short read happens.
  // For control transfer, even the short read happens, try the
  // status stage.
  //
  PEI_EHC_QTD                         *ShortReadStop;
  EFI_EVENT                           PollTimer;

  //
  // Asynchronous(bulk and control) transfer schedule data:
  // ReclaimHead is used as the head of the asynchronous transfer
  // list. It acts as the reclamation header.
  //
  PEI_EHC_QH                          *ReclaimHead;

  //
  // Periodic (interrupt) transfer schedule data:
  //
  VOID                                *PeriodFrame;     // Mapped as common buffer
  VOID                                *PeriodFrameMap;

  PEI_EHC_QH                          *PeriodOne;
  EFI_LIST_ENTRY                      AsyncIntTransfers;

  //
  // EHCI configuration data
  //
  UINT32                              HcStructParams; // Cache of HC structure parameter, EHC_HCSPARAMS_OFFSET
  UINT32                              HcCapParams;    // Cache of HC capability parameter, HCCPARAMS
  UINT32                              CapLen;         // Capability length
  UINT32                              High32bitAddr;
};

#define PEI_RECOVERY_USB_EHC_DEV_FROM_EHCI_THIS(a)  CR (a, PEI_USB2_HC_DEV, Usb2HostControllerPpi, USB2_HC_DEV_SIGNATURE)
#define PEI_RECOVERY_USB_EHC_DEV_FROM_THIS_NOTIFY(a) CR (a, PEI_USB2_HC_DEV, EndOfPeiNotifyList, USB2_HC_DEV_SIGNATURE)

/**
  @param  EhcDev                 EHCI Device.

  @retval EFI_SUCCESS            EHCI successfully initialized.
  @retval EFI_ABORTED            EHCI was failed to be initialized.

**/
EFI_STATUS
InitializeUsbHC (
  IN PEI_USB2_HC_DEV      *EhcDev
  );

/**
  Initialize the memory management pool for the host controller.

  @param  Ehc                   The EHCI device.
  @param  Check4G               Whether the host controller requires allocated memory
                                from one 4G address space.
  @param  Which4G               The 4G memory area each memory allocated should be from.

  @retval EFI_SUCCESS           The memory pool is initialized.
  @retval EFI_OUT_OF_RESOURCE   Fail to init the memory pool.

**/
USBHC_MEM_POOL *
UsbHcInitMemPool (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN BOOLEAN              Check4G,
  IN UINT32               Which4G
  )
;

/**
  Release the memory management pool.

  @param  Ehc                   The EHCI device.
  @param  Pool                  The USB memory pool to free.

  @retval EFI_DEVICE_ERROR      Fail to free the memory pool.
  @retval EFI_SUCCESS           The memory pool is freed.

**/
EFI_STATUS
UsbHcFreeMemPool (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN USBHC_MEM_POOL       *Pool
  )
;

/**
  Allocate some memory from the host controller's memory pool
  which can be used to communicate with host controller.

  @param  Ehc       The EHCI device.
  @param  Pool      The host controller's memory pool.
  @param  Size      Size of the memory to allocate.

  @return The allocated memory or NULL.

**/
VOID *
UsbHcAllocateMem (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN  USBHC_MEM_POOL      *Pool,
  IN  UINTN               Size
  )
;

/**
  Free the allocated memory back to the memory pool.

  @param  Ehc            The EHCI device.
  @param  Pool           The memory pool of the host controller.
  @param  Mem            The memory to free.
  @param  Size           The size of the memory to free.

**/
VOID
UsbHcFreeMem (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN USBHC_MEM_POOL       *Pool,
  IN VOID                 *Mem,
  IN UINTN                Size
  )
;

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param IoMmu                  Pointer to IOMMU PPI.
  @param Operation              Indicates if the bus master is going to read or write to system memory.
  @param HostAddress            The system memory address to map to the PCI controller.
  @param NumberOfBytes          On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param DeviceAddress          The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
IoMmuMap (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN EDKII_IOMMU_OPERATION  Operation,
  IN VOID                   *HostAddress,
  IN OUT UINTN              *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param IoMmu              Pointer to IOMMU PPI.
  @param Mapping            The mapping value returned from Map().

**/
VOID
IoMmuUnmap (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN VOID                  *Mapping
  );

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param IoMmu                  Pointer to IOMMU PPI.
  @param Pages                  The number of pages to allocate.
  @param HostAddress            A pointer to store the base system memory address of the
                                allocated range.
  @param DeviceAddress          The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateBuffer (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN UINTN                  Pages,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param IoMmu              Pointer to IOMMU PPI.
  @param Pages              The number of pages to free.
  @param HostAddress        The base system memory address of the allocated range.
  @param Mapping            The mapping value returned from Map().

**/
VOID
IoMmuFreeBuffer (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN UINTN                  Pages,
  IN VOID                   *HostAddress,
  IN VOID                   *Mapping
  );

/**
  Initialize IOMMU.

  @param IoMmu              Pointer to pointer to IOMMU PPI.

**/
VOID
IoMmuInit (
  OUT EDKII_IOMMU_PPI       **IoMmu
  );

#endif
