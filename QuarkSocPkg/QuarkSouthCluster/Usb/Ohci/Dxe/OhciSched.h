/** @file
This file contains the definination for host controller schedule routines.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#ifndef _OHCI_SCHED_H
#define _OHCI_SCHED_H

#include "Descriptor.h"

#define HCCA_MEM_SIZE     256
#define GRID_SIZE         16
#define GRID_SHIFT        4

typedef struct _INTERRUPT_CONTEXT_ENTRY INTERRUPT_CONTEXT_ENTRY;

struct _INTERRUPT_CONTEXT_ENTRY{
  UINT8                               DeviceAddress;
  UINT8                               EndPointAddress;
  ED_DESCRIPTOR                       *Ed;
  TD_DESCRIPTOR                       *DataTd;
  BOOLEAN                             IsSlowDevice;
  UINT8                               MaxPacketLength;
  UINTN                               PollingInterval;
  EFI_ASYNC_USB_TRANSFER_CALLBACK     CallBackFunction;
  VOID                                *Context;
  BOOLEAN                             IsPeriodic;
  VOID                                *Buffer;
  UINTN                               DataLength;
  VOID                                *UCBuffer;
  VOID                                *UCBufferMapping;
  UINT8                               *Toggle;
  INTERRUPT_CONTEXT_ENTRY      *NextEntry;
};


typedef struct {
  UINT32                  ErrorCode;
  UINT8                   NextToggle;
} OHCI_ED_RESULT;

/**

  Add an item of interrupt context

  @param  Ohc                   UHC private data
  @param  NewEntry              New entry to add

  @retval EFI_SUCCESS           Item successfully added

**/
EFI_STATUS
OhciAddInterruptContextEntry (
  IN  USB_OHCI_HC_DEV          *Ohc,
  IN  INTERRUPT_CONTEXT_ENTRY  *NewEntry
  );

/**

  Free a interrupt context entry

  @param  Ohc                   UHC private data
  @param  Entry                 Pointer to an interrupt context entry

  @retval EFI_SUCCESS           Entry freed
  @retval EFI_INVALID_PARAMETER Entry is NULL

**/
EFI_STATUS
OhciFreeInterruptContextEntry (
  IN USB_OHCI_HC_DEV          *Ohc,
  IN INTERRUPT_CONTEXT_ENTRY  *Entry
  );

/**

  Free entries match the device address and endpoint address

  @Param  Ohc                   UHC private date
  @Param  DeviceAddress         Item to free must match this device address
  @Param  EndPointAddress       Item to free must match this end point address
  @Param  DataToggle            DataToggle for output

  @retval  EFI_SUCCESS          Items match the requirement removed

**/
EFI_STATUS
OhciFreeInterruptContext(
  IN  USB_OHCI_HC_DEV     *Ohc,
  IN  UINT8               DeviceAddress,
  IN  UINT8               EndPointAddress,
  OUT UINT8               *DataToggle
  );


/**

  Convert Error code from OHCI format to EFI format

  @Param  ErrorCode             ErrorCode in OHCI format

  @retval                       ErrorCode in EFI format

**/
UINT32
ConvertErrorCode (
  IN  UINT32              ErrorCode
  );


/**

  Check TDs Results

  @Param  Ohc                   UHC private data
  @Param  Td                    TD_DESCRIPTOR
  @Param  Result                Result to return

  @retval TRUE                  means OK
  @retval FLASE                 means Error or Short packet

**/
BOOLEAN
OhciCheckTDsResults (
  IN  USB_OHCI_HC_DEV     *Ohc,
  IN  TD_DESCRIPTOR       *Td,
  OUT UINT32              *Result
  );
/**

  Check the task status on an ED

  @Param  Ed                    Pointer to the ED task that TD hooked on
  @Param  HeadTd                TD header for current transaction

  @retval                       Task Status Code

**/

UINT32
CheckEDStatus (
  IN  ED_DESCRIPTOR       *Ed,
  IN  TD_DESCRIPTOR       *HeadTd,
  OUT OHCI_ED_RESULT      *EdResult
  );
/**

  Check the task status

  @Param  Ohc                   UHC private data
  @Param  ListType              Pipe type
  @Param  Ed                    Pointer to the ED task hooked on
  @Param  HeadTd                Head of TD corresponding to the task
  @Param  ErrorCode             return the ErrorCode

  @retval  EFI_SUCCESS          Task done
  @retval  EFI_NOT_READY        Task on processing
  @retval  EFI_DEVICE_ERROR     Some error occured

**/
EFI_STATUS
CheckIfDone (
  IN  USB_OHCI_HC_DEV       *Ohc,
  IN  DESCRIPTOR_LIST_TYPE  ListType,
  IN  ED_DESCRIPTOR         *Ed,
  IN  TD_DESCRIPTOR         *HeadTd,
  OUT OHCI_ED_RESULT        *EdResult
  );

/**

  Convert TD condition code to Efi Status

  @Param  ConditionCode         Condition code to convert

  @retval  EFI_SUCCESS          No error occured
  @retval  EFI_NOT_READY        TD still on processing
  @retval  EFI_DEVICE_ERROR     Error occured in processing TD

**/

EFI_STATUS
OhciTDConditionCodeToStatus (
  IN  UINT32              ConditionCode
  );

/**

  Invoke callbacks hooked on done TDs

  @Param  Entry                 Interrupt transfer transaction information data structure
  @Param  Context               Ohc private data

**/

VOID
OhciInvokeInterruptCallBack(
  IN  INTERRUPT_CONTEXT_ENTRY  *Entry,
  IN  UINT32                   Result
);


/**

  Timer to submit periodic interrupt transfer, and invoke callbacks hooked on done TDs

  @param  Event                 Event handle
  @param  Context               Device private data

**/

VOID
EFIAPI
OhciHouseKeeper (
  IN  EFI_EVENT           Event,
  IN  VOID                *Context
  );

#endif
