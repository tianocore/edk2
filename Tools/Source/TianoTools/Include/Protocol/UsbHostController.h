/** @file
  EFI_USB_HC_PROTOCOL as defined in EFI 1.10.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  UsbHostController.h

**/

#ifndef _USB_HOSTCONTROLLER_H_
#define _USB_HOSTCONTROLLER_H_

#define EFI_USB_HC_PROTOCOL_GUID \
  { \
    0xf5089266, 0x1aa0, 0x4953, {0x97, 0xd8, 0x56, 0x2f, 0x8a, 0x73, 0xb5, 0x19 } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_USB_HC_PROTOCOL EFI_USB_HC_PROTOCOL;

//
// Protocol definitions
//

/**                                                                 
  Provides software reset for the USB host controller.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.
                                
  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_UNSUPPORTED       The type of reset specified by Attributes is not currently supported
                                by the host controller hardware.                                    
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to perform the reset operation.
                                   
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_RESET) (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN UINT16                  Attributes
  );

/**                                                                 
  Retrieves current state of the USB host controller.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  State                 A pointer to the EFI_USB_HC_STATE data structure that
                                indicates current state of the USB host controller.  
                                
  @retval EFI_SUCCESS           The state information of the host controller was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to retrieve the host controller¡¯s
                                current state.                                                                 
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_GET_STATE) (
  IN  EFI_USB_HC_PROTOCOL    *This,
  OUT EFI_USB_HC_STATE       *State
  );

/**                                                                 
  Sets the USB host controller to a specific state.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  State                 Indicates the state of the host controller that will be set.                                
                                
  @retval EFI_SUCCESS           The USB host controller was successfully placed in the state specified by
                                State.                                                                   
  @retval EFI_INVALID_PARAMETER State is NULL.
  @retval EFI_DEVICE_ERROR      Failed to set the state specified by State due to device error.                                
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_SET_STATE) (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN EFI_USB_HC_STATE        State
  );

/**                                                                 
  Submits control transfer to a target USB device.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB, which is
                                assigned during USB enumeration.                                
  @param  IsSlowDevice          Indicates whether the target device is slow device or full-speed
                                device.                                                         
  @param  MaximumPacketLength   Indicates the maximum packet size that the default control 
                                transfer endpoint is capable of sending or receiving.     
  @param  Request               A pointer to the USB device request that will be sent to the USB
                                device.
  @param  TransferDirection     Specifies the data direction for the transfer.  
  @param  Data                  A pointer to the buffer of data that will be transmitted to USB  
                                device or received from USB device.                            
  @param  DataLength            On input, indicates the size, in bytes, of the data buffer specified
                                by Data. On output, indicates the amount of data actually           
                                transferred.                                                        
  @param  TimeOut               Indicates the maximum time, in milliseconds, which the transfer
                                is allowed to complete.                                        
  @param  TransferResult        A pointer to the detailed result information generated by this  
                                control transfer.
                                
  @retval EFI_SUCCESS           The control transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The control transfer could not be completed due to a lack of resources.                                
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The control transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The control transfer failed due to host controller or device error.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_CONTROL_TRANSFER) (
  IN     EFI_USB_HC_PROTOCOL       *This,
  IN     UINT8                     DeviceAddress,
  IN     BOOLEAN                   IsSlowDevice,
  IN     UINT8                     MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST    *Request,
  IN     EFI_USB_DATA_DIRECTION    TransferDirection,
  IN OUT VOID                      *Data       OPTIONAL,
  IN OUT UINTN                     *DataLength OPTIONAL,
  IN     UINTN                     TimeOut,
  OUT    UINT32                    *TransferResult
  );

/**                                                                 
  Submits bulk transfer to a bulk endpoint of a USB device.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB, which is
                                assigned during USB enumeration.                                
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device.                  
  @param  MaximumPacketLength   Indicates the maximum packet size that the default control 
                                transfer endpoint is capable of sending or receiving.     
  @param  Data                  A pointer to the buffer of data that will be transmitted to USB  
                                device or received from USB device.                            
  @param  DataLength            On input, indicates the size, in bytes, of the data buffer specified
                                by Data. On output, indicates the amount of data actually           
                                transferred.             
  @param  DataToggle            A pointer to the data toggle value.                                                                           
  @param  TimeOut               Indicates the maximum time, in milliseconds, which the transfer
                                is allowed to complete.                                        
  @param  TransferResult        A pointer to the detailed result information of the bulk transfer.
                                
  @retval EFI_SUCCESS           The bulk transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The bulk transfer could not be completed due to a lack of resources.                                
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The bulk transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The bulk transfer failed due to host controller or device error.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_BULK_TRANSFER) (
  IN     EFI_USB_HC_PROTOCOL    *This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  MaximumPacketLength,
  IN OUT VOID                   *Data,
  IN OUT UINTN                  *DataLength,
  IN OUT UINT8                  *DataToggle,
  IN     UINTN                  TimeOut,
  OUT    UINT32                 *TransferResult
  );

/**                                                                 
  Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB, which is
                                assigned during USB enumeration.                                
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device.   
  @param  IsSlowDevice          Indicates whether the target device is slow device or full-speed
                                device.                                                                          
  @param  MaximumPacketLength   Indicates the maximum packet size that the default control 
                                transfer endpoint is capable of sending or receiving.     
  @param  IsNewTransfer         If TRUE, an asynchronous interrupt pipe is built between the host                       
                                and the target interrupt endpoint. If FALSE, the specified       
  
  @param  DataToggle            A pointer to the data toggle value.      
  @param  PollingInterval       Indicates the interval, in milliseconds, that the asynchronous
                                interrupt transfer is polled.                                                                                                      asynchronous interrupt pipe is canceled.                         
  @param  DataLength            Indicates the length of data to be received at the rate specified by
                                PollingInterval from the target asynchronous interrupt              
                                endpoint.                                                             
  @param  CallBackFunction      The Callback function.                                
  @param  Context               The context that is passed to the CallBackFunction.
                                
  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.    
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.                                
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The bulk transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The bulk transfer failed due to host controller or device error.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER) (
  IN     EFI_USB_HC_PROTOCOL                                 *This,
  IN     UINT8                                               DeviceAddress,
  IN     UINT8                                               EndPointAddress,
  IN     BOOLEAN                                             IsSlowDevice,
  IN     UINT8                                               MaxiumPacketLength,
  IN     BOOLEAN                                             IsNewTransfer,
  IN OUT UINT8                                               *DataToggle,
  IN     UINTN                                               PollingInterval  OPTIONAL,
  IN     UINTN                                               DataLength       OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK                     CallBackFunction OPTIONAL,
  IN     VOID                                                *Context         OPTIONAL
  );

/**                                                                 
  Submits synchronous interrupt transfer to an interrupt endpoint of a USB device.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB, which is
                                assigned during USB enumeration.                                
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device.   
  @param  IsSlowDevice          Indicates whether the target device is slow device or full-speed
                                device.                                                                          
  @param  MaximumPacketLength   Indicates the maximum packet size that the default control 
                                transfer endpoint is capable of sending or receiving.       
  @param  Data                  A pointer to the buffer of data that will be transmitted to USB
                                device or received from USB device.                                                                                            asynchronous interrupt pipe is canceled.                         
  @param  DataLength            On input, the size, in bytes, of the data buffer specified by Data.
                                On output, the number of bytes transferred.                          
  @param  DataToggle            A pointer to the data toggle value.                                
  @param  TimeOut               Indicates the maximum time, in milliseconds, which the transfer    
                                is allowed to complete.                                            
  @param  TransferResult        A pointer to the detailed result information from the synchronous
                                interrupt transfer.
                                
  @retval EFI_SUCCESS           The synchronous interrupt transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.                                  
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The synchronous interrupt transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The synchronous interrupt transfer failed due to host controller or device error.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER) (
  IN     EFI_USB_HC_PROTOCOL    *This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     BOOLEAN                IsSlowDevice,
  IN     UINT8                  MaximumPacketLength,
  IN OUT VOID                   *Data,
  IN OUT UINTN                  *DataLength,
  IN OUT UINT8                  *DataToggle,
  IN     UINTN                  TimeOut,
  OUT    UINT32                 *TransferResult
  );

/**                                                                 
  Submits isochronous transfer to an isochronous endpoint of a USB device.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB, which is
                                assigned during USB enumeration.                                
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device.                  
  @param  MaximumPacketLength   Indicates the maximum packet size that the default control 
                                transfer endpoint is capable of sending or receiving.       
  @param  Data                  A pointer to the buffer of data that will be transmitted to USB
                                device or received from USB device.                                                                                            asynchronous interrupt pipe is canceled.                         
  @param  DataLength            Specifies the length, in bytes, of the data to be sent to or received
                                from the USB device.                                                 
  @param  TransferResult        A pointer to the detailed result information from the isochronous
                                transfer.
                                
  @retval EFI_SUCCESS           The isochronous transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The isochronous could not be completed due to a lack of resources.                                  
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The isochronous transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The isochronous transfer failed due to host controller or device error.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ISOCHRONOUS_TRANSFER) (
  IN     EFI_USB_HC_PROTOCOL    *This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  MaximumPacketLength,
  IN OUT VOID                   *Data,
  IN     UINTN                  DataLength,
  OUT    UINT32                 *TransferResult
  );

/**                                                                 
  Submits nonblocking isochronous transfer to an isochronous endpoint of a USB device.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB, which is
                                assigned during USB enumeration.                                
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device.                  
  @param  MaximumPacketLength   Indicates the maximum packet size that the default control 
                                transfer endpoint is capable of sending or receiving.       
  @param  Data                  A pointer to the buffer of data that will be transmitted to USB
                                device or received from USB device.                                                                                            asynchronous interrupt pipe is canceled.                         
  @param  DataLength            Specifies the length, in bytes, of the data to be sent to or received
                                from the USB device.                                                 
  @param  IsochronousCallback   The Callback function.
  @param  Context               Data passed to the IsochronousCallback function. This is
                                an optional parameter and may be NULL.
                                
  @retval EFI_SUCCESS           The asynchronous isochronous transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The asynchronous isochronous could not be completed due to a lack of resources.                                  
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER) (
  IN     EFI_USB_HC_PROTOCOL                *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              MaximumPacketLength,
  IN OUT VOID                               *Data,
  IN     UINTN                              DataLength,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    IsochronousCallBack,
  IN     VOID                               *Context OPTIONAL
  );

/**                                                                 
  Retrieves the number of root hub ports.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  PortNumber            A pointer to the number of the root hub ports.                                
                                
  @retval EFI_SUCCESS           The port number was retrieved successfully.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to retrieve the port number.
  @retval EFI_INVALID_PARAMETER PortNumber is NULL.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_NUMBER) (
  IN EFI_USB_HC_PROTOCOL    *This,
  OUT UINT8                 *PortNumber
  );

/**                                                                 
  Retrieves the current status of a USB root hub port.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port from which the status is to be retrieved.
                                This value is zero based.
  @param  PortStatus            A pointer to the current port status bits and port status change bits.                                
                                
  @retval EFI_SUCCESS           The status of the USB root hub port specified by PortNumber
                                was returned in PortStatus.                                
  @retval EFI_INVALID_PARAMETER PortNumber is invalid.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS) (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                  PortNumber,
  OUT EFI_USB_PORT_STATUS    *PortStatus
  );

/**                                                                 
  Sets a feature for the specified root hub port.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port from which the status is to be retrieved.
                                This value is zero based.
  @param  PortFeature           Indicates the feature selector associated with the feature set
                                request.
                                
  @retval EFI_SUCCESS           The feature specified by PortFeature was set for the USB
                                root hub port specified by PortNumber.                  
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid for this function.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE) (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  );

/**                                                                 
  Clears a feature for the specified root hub port.
    
  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port from which the status is to be cleared.
                                This value is zero based.
  @param  PortFeature           Indicates the feature selector associated with the feature clear
                                request.
                                
  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared for the USB
                                root hub port specified by PortNumber.                  
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid for this function.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE) (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  );

struct _EFI_USB_HC_PROTOCOL {
  EFI_USB_HC_PROTOCOL_RESET                       Reset;
  EFI_USB_HC_PROTOCOL_GET_STATE                   GetState;
  EFI_USB_HC_PROTOCOL_SET_STATE                   SetState;
  EFI_USB_HC_PROTOCOL_CONTROL_TRANSFER            ControlTransfer;
  EFI_USB_HC_PROTOCOL_BULK_TRANSFER               BulkTransfer;
  EFI_USB_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER    AsyncInterruptTransfer;
  EFI_USB_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER     SyncInterruptTransfer;
  EFI_USB_HC_PROTOCOL_ISOCHRONOUS_TRANSFER        IsochronousTransfer;
  EFI_USB_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER  AsyncIsochronousTransfer;
  EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_NUMBER     GetRootHubPortNumber;
  EFI_USB_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS     GetRootHubPortStatus;
  EFI_USB_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE    SetRootHubPortFeature;
  EFI_USB_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE  ClearRootHubPortFeature;
  UINT16                                          MajorRevision;
  UINT16                                          MinorRevision;
};

extern EFI_GUID gEfiUsbHcProtocolGuid;

#endif
