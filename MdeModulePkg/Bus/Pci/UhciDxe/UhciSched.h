/** @file

  The definition for EHCI register operation routines.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_UHCI_SCHED_H_
#define _EFI_UHCI_SCHED_H_


#define UHCI_ASYNC_INT_SIGNATURE  SIGNATURE_32 ('u', 'h', 'c', 'a')
//
// The failure mask for USB transfer return status. If any of
// these bit is set, the transfer failed. EFI_USB_ERR_NOEXECUTE
// and EFI_USB_ERR_NAK are not considered as error condition:
// the transfer is still going on.
//
#define USB_ERR_FAIL_MASK  (EFI_USB_ERR_STALL   | EFI_USB_ERR_BUFFER | \
                            EFI_USB_ERR_BABBLE  | EFI_USB_ERR_CRC    | \
                            EFI_USB_ERR_TIMEOUT | EFI_USB_ERR_BITSTUFF | \
                            EFI_USB_ERR_SYSTEM)


//
// Structure to return the result of UHCI QH execution.
// Result is the final result of the QH's QTD. NextToggle
// is the next data toggle to use. Complete is the actual
// length of data transferred.
//
typedef struct {
  UINT32                  Result;
  UINT8                   NextToggle;
  UINTN                   Complete;
} UHCI_QH_RESULT;

typedef struct _UHCI_ASYNC_REQUEST  UHCI_ASYNC_REQUEST;

//
// Structure used to manager the asynchronous interrupt transfers.
//
struct _UHCI_ASYNC_REQUEST{
  UINTN                           Signature;
  LIST_ENTRY                      Link;
  UHCI_ASYNC_REQUEST              *Recycle;

  //
  // Endpoint attributes
  //
  UINT8                           DevAddr;
  UINT8                           EndPoint;
  BOOLEAN                         IsLow;
  UINTN                           Interval;

  //
  // Data and UHC structures
  //
  UHCI_QH_SW                      *QhSw;
  UHCI_TD_SW                      *FirstTd;
  UINT8                           *Data;      // Allocated host memory, not mapped memory
  UINTN                           DataLen;
  VOID                            *Mapping;

  //
  // User callback and its context
  //
  EFI_ASYNC_USB_TRANSFER_CALLBACK Callback;
  VOID                            *Context;
};

#define UHCI_ASYNC_INT_FROM_LINK(a) \
          CR (a, UHCI_ASYNC_REQUEST, Link, UHCI_ASYNC_INT_SIGNATURE)


/**
  Create Frame List Structure.

  @param  Uhc                    The UHCI device.

  @return EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @return EFI_UNSUPPORTED        Map memory fail.
  @return EFI_SUCCESS            Success.

**/
EFI_STATUS
UhciInitFrameList (
  IN USB_HC_DEV         *Uhc
  );

/**
  Destory FrameList buffer.

  @param  Uhc                    The UHCI device.

  @return None.

**/
VOID
UhciDestoryFrameList (
  IN USB_HC_DEV           *Uhc
  );


/**
  Convert the poll rate to the maxium 2^n that is smaller
  than Interval.

  @param  Interval               The poll rate to convert.

  @return The converted poll rate.

**/
UINTN
UhciConvertPollRate (
  IN  UINTN               Interval
  );


/**
  Link a queue head (for asynchronous interrupt transfer) to
  the frame list.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head to link into.

**/
VOID
UhciLinkQhToFrameList (
  USB_HC_DEV              *Uhc,
  UHCI_QH_SW              *Qh
  );


/**
  Unlink QH from the frame list is easier: find all
  the precedence node, and pointer there next to QhSw's
  next.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head to unlink.

**/
VOID
UhciUnlinkQhFromFrameList (
  USB_HC_DEV              *Uhc,
  UHCI_QH_SW              *Qh
  );


/**
  Check the result of the transfer.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head of the transfer.
  @param  Td                     The first TDs of the transfer.
  @param  TimeOut                TimeOut value in milliseconds.
  @param  IsLow                  Is Low Speed Device.
  @param  QhResult               The variable to return result.

  @retval EFI_SUCCESS            The transfer finished with success.
  @retval EFI_DEVICE_ERROR       Transfer failed.

**/
EFI_STATUS
UhciExecuteTransfer (
  IN  USB_HC_DEV          *Uhc,
  IN  UHCI_QH_SW          *Qh,
  IN  UHCI_TD_SW          *Td,
  IN  UINTN               TimeOut,
  IN  BOOLEAN             IsLow,
  OUT UHCI_QH_RESULT      *QhResult
  );


/**
  Create Async Request node, and Link to List.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head of the transfer.
  @param  FirstTd                First TD of the transfer.
  @param  DevAddr                Device Address.
  @param  EndPoint               EndPoint Address.
  @param  DataLen                Data length.
  @param  Interval               Polling Interval when inserted to frame list.
  @param  Data                   Data buffer, unmapped.
  @param  Callback               Callback after interrupt transfeer.
  @param  Context                Callback Context passed as function parameter.
  @param  IsLow                  Is Low Speed.

  @retval EFI_SUCCESS            An asynchronous transfer is created.
  @retval EFI_INVALID_PARAMETER  Paremeter is error.
  @retval EFI_OUT_OF_RESOURCES   Failed because of resource shortage.

**/
EFI_STATUS
UhciCreateAsyncReq (
  IN USB_HC_DEV                       *Uhc,
  IN UHCI_QH_SW                       *Qh,
  IN UHCI_TD_SW                       *FirstTd,
  IN UINT8                            DevAddr,
  IN UINT8                            EndPoint,
  IN UINTN                            DataLen,
  IN UINTN                            Interval,
  IN UINT8                            *Data,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,
  IN VOID                             *Context,
  IN BOOLEAN                          IsLow
  );


/**
  Delete Async Interrupt QH and TDs.

  @param  Uhc                    The UHCI device.
  @param  DevAddr                Device Address.
  @param  EndPoint               EndPoint Address.
  @param  Toggle                 The next data toggle to use.

  @retval EFI_SUCCESS            The request is deleted.
  @retval EFI_INVALID_PARAMETER  Paremeter is error.
  @retval EFI_NOT_FOUND          The asynchronous isn't found.

**/
EFI_STATUS
UhciRemoveAsyncReq (
  IN  USB_HC_DEV          *Uhc,
  IN  UINT8               DevAddr,
  IN  UINT8               EndPoint,
  OUT UINT8               *Toggle
  );


/**
  Release all the asynchronous transfers on the lsit.

  @param  Uhc                    The UHCI device.

  @return None.

**/
VOID
UhciFreeAllAsyncReq (
  IN USB_HC_DEV           *Uhc
  );


/**
  Interrupt transfer periodic check handler.

  @param  Event                  The event of the time.
  @param  Context                Context of the event, pointer to USB_HC_DEV.

  @return None.

**/
VOID
EFIAPI
UhciMonitorAsyncReqList (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  );

#endif
