/** @file
Provides the definition of Usb Hc Protocol and OHCI controller
private data structure.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#ifndef _OHCI_H
#define _OHCI_H


#include <Uefi.h>

#include <Protocol/UsbHostController.h>
#include <Protocol/PciIo.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>

#include <IndustryStandard/Pci.h>


typedef struct _USB_OHCI_HC_DEV USB_OHCI_HC_DEV;

#include "UsbHcMem.h"
#include "OhciReg.h"
#include "OhciSched.h"
#include "OhciUrb.h"
#include "Descriptor.h"
#include "ComponentName.h"
#include "OhciDebug.h"

extern EFI_DRIVER_BINDING_PROTOCOL   gOhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gOhciComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gOhciComponentName2;

#define USB_OHCI_HC_DEV_SIGNATURE     SIGNATURE_32('o','h','c','i')

typedef struct _HCCA_MEMORY_BLOCK{
  UINT32                    HccaInterruptTable[32];    // 32-bit Physical Address to ED_DESCRIPTOR
  UINT16                    HccaFrameNumber;
  UINT16                    HccaPad;
  UINT32                    HccaDoneHead;              // 32-bit Physical Address to TD_DESCRIPTOR
  UINT8                     Reserved[116];
} HCCA_MEMORY_BLOCK;


struct _USB_OHCI_HC_DEV {
  UINTN                     Signature;
  EFI_USB_HC_PROTOCOL       UsbHc;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT64                    OriginalPciAttributes;

  HCCA_MEMORY_BLOCK         *HccaMemoryBlock;
  VOID                      *HccaMemoryBuf;
  VOID                      *HccaMemoryMapping;
  UINTN                     HccaMemoryPages;

  ED_DESCRIPTOR             *IntervalList[6][32];
  INTERRUPT_CONTEXT_ENTRY   *InterruptContextList;
  VOID                      *MemPool;

  UINT32                    ToggleFlag;

  EFI_EVENT                 HouseKeeperTimer;
  //
  // ExitBootServicesEvent is used to stop the OHC DMA operation
  // after exit boot service.
  //
  EFI_EVENT                  ExitBootServiceEvent;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
};

#define USB_OHCI_HC_DEV_FROM_THIS(a)    CR(a, USB_OHCI_HC_DEV, UsbHc, USB_OHCI_HC_DEV_SIGNATURE)
#define USB2_OHCI_HC_DEV_FROM_THIS(a)    CR(a, USB_OHCI_HC_DEV, Usb2Hc, USB_OHCI_HC_DEV_SIGNATURE)

//
// Func List
//

/**
  Provides software reset for the USB host controller.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPOURTED      The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.

**/
EFI_STATUS
EFIAPI
OhciReset (
  IN EFI_USB_HC_PROTOCOL  *This,
  IN UINT16               Attributes
  );
/**
  Retrieve the current state of the USB host controller.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  State                 Variable to return the current host controller
                                state.

  @retval EFI_SUCCESS           Host controller state was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to
                                retrieve the host controller's current state.

**/

EFI_STATUS
EFIAPI
OhciGetState (
  IN  EFI_USB_HC_PROTOCOL  *This,
  OUT EFI_USB_HC_STATE     *State
  );
/**
  Sets the USB host controller to a specific state.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  State                 The state of the host controller that will be set.

  @retval EFI_SUCCESS           The USB host controller was successfully placed
                                in the state specified by State.
  @retval EFI_INVALID_PARAMETER State is invalid.
  @retval EFI_DEVICE_ERROR      Failed to set the state due to device error.

**/

EFI_STATUS
EFIAPI
OhciSetState(
  IN EFI_USB_HC_PROTOCOL  *This,
  IN EFI_USB_HC_STATE     State
  );
/**

  Submits control transfer to a target USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxPaketLength        Indicates the maximum packet size that the
                                default control transfer endpoint is capable of
                                sending or receiving.
  @param  Request               A pointer to the USB device request that will be sent
                                to the USB device.
  @param  TransferDirection     Specifies the data direction for the transfer.
                                There are three values available, DataIn, DataOut
                                and NoData.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            Indicates the size, in bytes, of the data buffer
                                specified by Data.
  @param  TimeOut               Indicates the maximum time, in microseconds,
                                which the transfer is allowed to complete.
  @param  TransferResult        A pointer to the detailed result information generated
                                by this control transfer.

  @retval EFI_SUCCESS           The control transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The control transfer could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The control transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The control transfer failed due to host controller or device error.
                                Caller should check TranferResult for detailed error information.

--*/


EFI_STATUS
EFIAPI
OhciControlTransfer (
  IN     EFI_USB_HC_PROTOCOL     *This,
  IN     UINT8                   DeviceAddress,
  IN     BOOLEAN                 IsSlowDevice,
  IN     UINT8                   MaxPacketLength,
  IN     EFI_USB_DEVICE_REQUEST  *Request,
  IN     EFI_USB_DATA_DIRECTION  TransferDirection,
  IN OUT VOID                    *Data                 OPTIONAL,
  IN OUT UINTN                   *DataLength           OPTIONAL,
  IN     UINTN                   TimeOut,
  OUT    UINT32                  *TransferResult
  );
/**

  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an
                                endpoint direction of the target USB device.
                                Each endpoint address supports data transfer in
                                one direction except the control endpoint
                                (whose default endpoint address is 0).
                                It is the caller's responsibility to make sure that
                                the EndPointAddress represents a bulk endpoint.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            When input, indicates the size, in bytes, of the data buffer
                                specified by Data. When output, indicates the actually
                                transferred data size.
  @param  DataToggle            A pointer to the data toggle value. On input, it indicates
                                the initial data toggle value the bulk transfer should adopt;
                                on output, it is updated to indicate the data toggle value
                                of the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in microseconds, which the
                                transfer is allowed to complete.
  TransferResult                A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The bulk transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The bulk transfer could not be submitted due to lack of resource.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The bulk transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The bulk transfer failed due to host controller or device error.
                                Caller should check TranferResult for detailed error information.

**/


EFI_STATUS
EFIAPI
OhciBulkTransfer(
  IN     EFI_USB_HC_PROTOCOL  *This,
  IN     UINT8                DeviceAddress,
  IN     UINT8                EndPointAddress,
  IN     UINT8                MaxPacketLength,
  IN OUT VOID                 *Data,
  IN OUT UINTN                *DataLength,
  IN OUT UINT8                *DataToggle,
  IN     UINTN                TimeOut,
  OUT    UINT32               *TransferResult
  );
/**

  Submits an interrupt transfer to an interrupt endpoint of a USB device.

  @param  Ohc                   Device private data
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device. Each endpoint address
                                supports data transfer in one direction except the
                                control endpoint (whose default endpoint address is 0).
                                It is the caller's responsibility to make sure that
                                the EndPointAddress represents an interrupt endpoint.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxPacketLength       Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  IsNewTransfer         If TRUE, an asynchronous interrupt pipe is built between
                                the host and the target interrupt endpoint.
                                If FALSE, the specified asynchronous interrupt pipe
                                is canceled.
  @param  DataToggle            A pointer to the data toggle value.  On input, it is valid
                                when IsNewTransfer is TRUE, and it indicates the initial
                                data toggle value the asynchronous interrupt transfer
                                should adopt.
                                On output, it is valid when IsNewTransfer is FALSE,
                                and it is updated to indicate the data toggle value of
                                the subsequent asynchronous interrupt transfer.
  @param  PollingInterval       Indicates the interval, in milliseconds, that the
                                asynchronous interrupt transfer is polled.
                                This parameter is required when IsNewTransfer is TRUE.
  @param  UCBuffer              Uncacheable buffer
  @param  DataLength            Indicates the length of data to be received at the
                                rate specified by PollingInterval from the target
                                asynchronous interrupt endpoint.  This parameter
                                is only required when IsNewTransfer is TRUE.
  @param  CallBackFunction      The Callback function.This function is called at the
                                rate specified by PollingInterval.This parameter is
                                only required when IsNewTransfer is TRUE.
  @param  Context               The context that is passed to the CallBackFunction.
                                This is an optional parameter and may be NULL.
  @param  IsPeriodic            Periodic interrupt or not
  @param  OutputED              The correspoding ED carried out
  @param  OutputTD              The correspoding TD carried out


  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/

EFI_STATUS
OhciInterruptTransfer (
  IN     USB_OHCI_HC_DEV                  *Ohc,
  IN     UINT8                            DeviceAddress,
  IN     UINT8                            EndPointAddress,
  IN     BOOLEAN                          IsSlowDevice,
  IN     UINT8                            MaxPacketLength,
  IN     BOOLEAN                          IsNewTransfer,
  IN OUT UINT8                            *DataToggle        OPTIONAL,
  IN     UINTN                            PollingInterval    OPTIONAL,
  IN     VOID                             *UCBuffer          OPTIONAL,
  IN     UINTN                            DataLength         OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK  CallBackFunction   OPTIONAL,
  IN     VOID                             *Context           OPTIONAL,
  IN     BOOLEAN                          IsPeriodic         OPTIONAL,
  OUT    ED_DESCRIPTOR                    **OutputED         OPTIONAL,
  OUT    TD_DESCRIPTOR                    **OutputTD         OPTIONAL
  );
/**

  Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device. Each endpoint address
                                supports data transfer in one direction except the
                                control endpoint (whose default endpoint address is 0).
                                It is the caller's responsibility to make sure that
                                the EndPointAddress represents an interrupt endpoint.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxiumPacketLength    Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  IsNewTransfer         If TRUE, an asynchronous interrupt pipe is built between
                                the host and the target interrupt endpoint.
                                If FALSE, the specified asynchronous interrupt pipe
                                is canceled.
  @param  DataToggle            A pointer to the data toggle value.  On input, it is valid
                                when IsNewTransfer is TRUE, and it indicates the initial
                                data toggle value the asynchronous interrupt transfer
                                should adopt.
                                On output, it is valid when IsNewTransfer is FALSE,
                                and it is updated to indicate the data toggle value of
                                the subsequent asynchronous interrupt transfer.
  @param  PollingInterval       Indicates the interval, in milliseconds, that the
                                asynchronous interrupt transfer is polled.
                                This parameter is required when IsNewTransfer is TRUE.
  @param  DataLength            Indicates the length of data to be received at the
                                rate specified by PollingInterval from the target
                                asynchronous interrupt endpoint.  This parameter
                                is only required when IsNewTransfer is TRUE.
  @param  CallBackFunction      The Callback function.This function is called at the
                                rate specified by PollingInterval.This parameter is
                                only required when IsNewTransfer is TRUE.
  @param  Context               The context that is passed to the CallBackFunction.
                                This is an optional parameter and may be NULL.

  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/


EFI_STATUS
EFIAPI
OhciAsyncInterruptTransfer (
  IN     EFI_USB_HC_PROTOCOL              *This,
  IN     UINT8                            DeviceAddress,
  IN     UINT8                            EndPointAddress,
  IN     BOOLEAN                          IsSlowDevice,
  IN     UINT8                            MaxPacketLength,
  IN     BOOLEAN                          IsNewTransfer,
  IN OUT UINT8                            *DataToggle        OPTIONAL,
  IN     UINTN                            PollingInterval    OPTIONAL,
  IN     UINTN                            DataLength         OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK  CallBackFunction   OPTIONAL,
  IN     VOID                             *Context           OPTIONAL
  );
/**

  Submits synchronous interrupt transfer to an interrupt endpoint
  of a USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device. Each endpoint
                                address supports data transfer in one direction
                                except the control endpoint (whose default
                                endpoint address is 0). It is the caller's responsibility
                                to make sure that the EndPointAddress represents
                                an interrupt endpoint.
  @param  IsSlowDevice          Indicates whether the target device is slow device
                                or full-speed device.
  @param  MaxPacketLength       Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            On input, the size, in bytes, of the data buffer specified
                                by Data. On output, the number of bytes transferred.
  @param  DataToggle            A pointer to the data toggle value. On input, it indicates
                                the initial data toggle value the synchronous interrupt
                                transfer should adopt;
                                on output, it is updated to indicate the data toggle value
                                of the subsequent synchronous interrupt transfer.
  @param  TimeOut               Indicates the maximum time, in microseconds, which the
                                transfer is allowed to complete.
  @param  TransferResult        A pointer to the detailed result information from
                                the synchronous interrupt transfer.

  @retval EFI_UNSUPPORTED       This interface not available.
  @retval EFI_INVALID_PARAMETER Parameters not follow spec

**/


EFI_STATUS
EFIAPI
OhciSyncInterruptTransfer (
  IN     EFI_USB_HC_PROTOCOL  *This,
  IN     UINT8                DeviceAddress,
  IN     UINT8                EndPointAddress,
  IN     BOOLEAN              IsSlowDevice,
  IN     UINT8                MaxPacketLength,
  IN OUT VOID                 *Data,
  IN OUT UINTN                *DataLength,
  IN OUT UINT8                *DataToggle,
  IN     UINTN                TimeOut,
  OUT    UINT32               *TransferResult
  );
/**

  Submits isochronous transfer to a target USB device.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       End point address
  @param  MaximumPacketLength   Indicates the maximum packet size that the
                                default control transfer endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            Indicates the size, in bytes, of the data buffer
                                specified by Data.
  @param  TransferResult        A pointer to the detailed result information generated
                                by this control transfer.

  @retval EFI_UNSUPPORTED       This interface not available
  @retval EFI_INVALID_PARAMETER Data is NULL or DataLength is 0 or TransferResult is NULL

**/


EFI_STATUS
EFIAPI
OhciIsochronousTransfer (
  IN     EFI_USB_HC_PROTOCOL  *This,
  IN     UINT8                DeviceAddress,
  IN     UINT8                EndPointAddress,
  IN     UINT8                MaximumPacketLength,
  IN OUT VOID                 *Data,
  IN OUT UINTN                DataLength,
  OUT    UINT32               *TransferResult
  );
/**

  Submits Async isochronous transfer to a target USB device.

  @param  his                   A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       End point address
  @param  MaximumPacketLength   Indicates the maximum packet size that the
                                default control transfer endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  IsochronousCallBack   When the transfer complete, the call back function will be called
  @param  Context               Pass to the call back function as parameter

  @retval EFI_UNSUPPORTED       This interface not available
  @retval EFI_INVALID_PARAMETER Data is NULL or Datalength is 0

**/

EFI_STATUS
EFIAPI
OhciAsyncIsochronousTransfer (
  IN     EFI_USB_HC_PROTOCOL                *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              MaximumPacketLength,
  IN OUT VOID                               *Data,
  IN OUT UINTN                              DataLength,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    IsochronousCallBack,
  IN     VOID                               *Context OPTIONAL
  );

/**

  Retrieves the number of root hub ports.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  NumOfPorts            A pointer to the number of the root hub ports.

  @retval EFI_SUCCESS           The port number was retrieved successfully.
**/
EFI_STATUS
EFIAPI
OhciGetRootHubNumOfPorts (
  IN  EFI_USB_HC_PROTOCOL  *This,
  OUT UINT8                *NumOfPorts
  );
/**

  Retrieves the current status of a USB root hub port.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL.
  @param  PortNumber            Specifies the root hub port from which the status
                                is to be retrieved.  This value is zero-based. For example,
                                if a root hub has two ports, then the first port is numbered 0,
                                and the second port is numbered 1.
  @param  PortStatus            A pointer to the current port status bits and
                                port status change bits.

  @retval EFI_SUCCESS           The status of the USB root hub port specified by PortNumber
                                was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER Port number not valid
**/


EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
  IN  EFI_USB_HC_PROTOCOL  *This,
  IN  UINT8                PortNumber,
  OUT EFI_USB_PORT_STATUS  *PortStatus
  );

/**

  Sets a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be set.
  @param  PortFeature           Indicates the feature selector associated
                                with the feature set request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set for the
                                USB root hub port specified by PortNumber.
  @retval EFI_DEVICE_ERROR      Set feature failed because of hardware issue
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
**/
EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
  IN EFI_USB_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  );
/**

  Clears a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared for the
                                USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Some error happened when clearing feature
**/
EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
  IN EFI_USB_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  );


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

OHCIDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Starting the Usb OHCI Driver.

  @param  This                  Protocol instance pointer.
  @param  Controller            Handle of device to test.
  @param  RemainingDevicePath   Not used.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.
  @retval EFI_DEVICE_ERROR      This driver cannot be started due to device Error.
                                EFI_OUT_OF_RESOURCES- Failed due to resource shortage.

**/
EFI_STATUS
EFIAPI
OHCIDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                  Protocol instance pointer.
  @param  Controller            Handle of device to stop driver on.
  @param  NumberOfChildren      Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer     List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
EFIAPI
OHCIDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

#endif
