/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:

    usbbus.h

  Abstract:

    Header file for USB bus driver Interface

  Revision History



--*/

#ifndef _EFI_USB_BUS_H
#define _EFI_USB_BUS_H


#include <IndustryStandard/Usb.h>
#include "hub.h"
#include "usbutil.h"


extern UINTN  gUSBDebugLevel;
extern UINTN  gUSBErrorLevel;


#define MICROSECOND       10000
#define ONESECOND         (1000 * MICROSECOND)
#define BUSPOLLING_PERIOD ONESECOND
//
// We define some maximun value here
//
#define USB_MAXCONFIG               8
#define USB_MAXALTSETTING           4
#define USB_MAXINTERFACES           32
#define USB_MAXENDPOINTS            16
#define USB_MAXSTRINGS              16
#define USB_MAXLANID                16
#define USB_MAXCHILDREN             8
#define USB_MAXCONTROLLERS          4

#define USB_IO_CONTROLLER_SIGNATURE EFI_SIGNATURE_32 ('u', 's', 'b', 'd')

typedef struct {
  LIST_ENTRY      Link;
  UINT16          StringIndex;
  CHAR16          *String;
} STR_LIST_ENTRY;

typedef struct {
  LIST_ENTRY                  Link;
  UINT16                      Toggle;
  EFI_USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
} ENDPOINT_DESC_LIST_ENTRY;

typedef struct {
  LIST_ENTRY                    Link;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  LIST_ENTRY                    EndpointDescListHead;
} INTERFACE_DESC_LIST_ENTRY;

typedef struct {
  LIST_ENTRY                Link;
  EFI_USB_CONFIG_DESCRIPTOR CongfigDescriptor;
  LIST_ENTRY                InterfaceDescListHead;
  UINTN                     ActiveInterface;
} CONFIG_DESC_LIST_ENTRY;

//
// Forward declaring
//
struct usb_io_device;

//
// This is used to form the USB Controller Handle
//
typedef struct usb_io_controller_device {
  UINTN                           Signature;
  EFI_HANDLE                      Handle;
  EFI_USB_IO_PROTOCOL             UsbIo;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_HANDLE                      HostController;
  UINT8                           CurrentConfigValue;
  UINT8                           InterfaceNumber;
  struct usb_io_device            *UsbDevice;

  BOOLEAN                         IsUsbHub;
  BOOLEAN                         IsManagedByDriver;

  //
  // Fields specified for USB Hub
  //
  EFI_EVENT                       HubNotify;
  UINT8                           HubEndpointAddress;
  UINT8                           StatusChangePort;
  UINT8                           DownstreamPorts;

  UINT8                           ParentPort;
  struct usb_io_controller_device *Parent;
  struct usb_io_device            *Children[USB_MAXCHILDREN];
} USB_IO_CONTROLLER_DEVICE;

#define USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS(a) \
    CR(a, USB_IO_CONTROLLER_DEVICE, UsbIo, USB_IO_CONTROLLER_SIGNATURE)

//
// This is used to keep the topology of USB bus
//
struct _usb_bus_controller_device;

typedef struct usb_io_device {
  UINT8                               DeviceAddress;
  BOOLEAN                             IsConfigured;
  BOOLEAN                             IsSlowDevice;
  UINT8                               DeviceSpeed;
  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator;
  EFI_USB_DEVICE_DESCRIPTOR           DeviceDescriptor;
  LIST_ENTRY                          ConfigDescListHead;
  CONFIG_DESC_LIST_ENTRY              *ActiveConfig;
  UINT16                              LangID[USB_MAXLANID];

  struct _usb_bus_controller_device   *BusController;

  //
  // Track the controller handle
  //
  UINT8                               NumOfControllers;
  USB_IO_CONTROLLER_DEVICE            *UsbController[USB_MAXCONTROLLERS];

} USB_IO_DEVICE;

//
// Usb Bus Controller device strcuture
//
#define EFI_USB_BUS_PROTOCOL_GUID \
 { 0x2B2F68CC, 0x0CD2, 0x44cf, { 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75  } }

typedef struct _EFI_USB_BUS_PROTOCOL {
  UINT64  Reserved;
} EFI_USB_BUS_PROTOCOL;

#define USB_BUS_DEVICE_SIGNATURE  EFI_SIGNATURE_32 ('u', 'b', 'u', 's')

typedef struct _usb_bus_controller_device {
  UINTN                     Signature;

  EFI_USB_BUS_PROTOCOL      BusIdentify;
  EFI_USB2_HC_PROTOCOL      *Usb2HCInterface;
  EFI_USB_HC_PROTOCOL       *UsbHCInterface;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT8                     AddressPool[16];
  USB_IO_DEVICE             *Root;
  BOOLEAN                   Hc2ProtocolSupported;
} USB_BUS_CONTROLLER_DEVICE;

#define USB_BUS_CONTROLLER_DEVICE_FROM_THIS(a) \
    CR(a, USB_BUS_CONTROLLER_DEVICE, BusIdentify, USB_BUS_DEVICE_SIGNATURE)


//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gUsbBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gUsbBusComponentName;

//
// EFI_DRIVER_BINDING_PROTOCOL Protocol Interface
//
EFI_STATUS
EFIAPI
UsbBusControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbBusControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbBusControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
UsbBusComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  CHAR8                           *Language,
  OUT CHAR16                          **DriverName
  );

EFI_STATUS
EFIAPI
UsbBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  EFI_HANDLE                      ControllerHandle,
  IN  EFI_HANDLE                      ChildHandle, OPTIONAL
  IN  CHAR8                           *Language,
  OUT CHAR16                          **ControllerName
  );

//
// Usb Device Configuration functions
//
BOOLEAN
IsHub (
  IN USB_IO_CONTROLLER_DEVICE     *Dev
  )
/*++
  
  Routine Description:
    Tell if a usb controller is a hub controller.
    
  Arguments:
    Dev - UsbIoController device structure.
    
  Returns:
    TRUE/FALSE
--*/
;

EFI_STATUS
UsbGetStringtable (
  IN  USB_IO_DEVICE     *UsbIoDevice
  )
/*++
  
  Routine Description:
    Get the string table stored in a usb device.
    
  Arguments:
    Dev     -     UsbIoController device structure.
    
  Returns:
    EFI_SUCCESS
    EFI_UNSUPPORTED
    EFI_OUT_OF_RESOURCES
    
--*/
;

EFI_STATUS
UsbGetAllConfigurations (
  IN  USB_IO_DEVICE     *UsbIoDevice
  )
/*++

  Routine Description:
    This function is to parse all the configuration descriptor.
    
  Arguments:
    UsbIoDevice  -  USB_IO_DEVICE device structure.
    
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_OUT_OF_RESOURCES  

--*/
;

EFI_STATUS
UsbSetConfiguration (
  IN  USB_IO_DEVICE     *Dev,
  IN  UINTN             ConfigurationValue
  )
/*++

  Routine Description:
    Set the device to a configuration value.
    
  Arguments:
    UsbIoDev            -   USB_IO_DEVICE to be set configuration
    ConfigrationValue   -   The configuration value to be set to that device
    
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    
--*/
;

EFI_STATUS
UsbSetDefaultConfiguration (
  IN  USB_IO_DEVICE     *Dev
  )
/*++

  Routine Description:
    Set the device to a default configuration value.
    
  Arguments:
    UsbIoDev       -    USB_IO_DEVICE to be set configuration
    
  Returns
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    
--*/
;

//
// Device Deconfiguration functions
//
VOID
UsbDestroyAllConfiguration (
  IN USB_IO_DEVICE     *UsbIoDevice
  )
/*++

  Routine Description:
    Delete all configuration data when device is not used.
    
  Arguments:
    UsbIoDevice  - USB_IO_DEVICE to be set configuration
  
  Returns:
    VOID
    
--*/
;

EFI_STATUS
DoHubConfig (
  IN USB_IO_CONTROLLER_DEVICE     *HubIoDevice
  )
/*++
  
  Routine Description:
    Configure the hub
  
  Arguments:
    HubController         -   Indicating the hub controller device that
                              will be configured
                                
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    
--*/

;

VOID
GetDeviceEndPointMaxPacketLength (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN  UINT8                 EndpointAddr,
  OUT UINTN                 *MaxPacketLength
  )
/*++

  Routine Description:
    Get the Max Packet Length of the speified Endpoint.

  Arguments:
    UsbIo           -     Given Usb Controller device.
    EndpointAddr    -     Given Endpoint address.
    MaxPacketLength -     The max packet length of that endpoint

  Returns:
    N/A

--*/
;

VOID
GetDataToggleBit (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN  UINT8                 EndpointAddr,
  OUT UINT8                 *DataToggle
  )
/*++

  Routine Description:
    Get the datatoggle of a specified endpoint.

  Arguments:
    UsbIo         -     Given Usb Controller device.
    EndpointAddr  -     Given Endpoint address.
    DataToggle    -     The current data toggle of that endpoint

  Returns:
    VOID
    
--*/
;

VOID
SetDataToggleBit (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN UINT8                  EndpointAddr,
  IN UINT8                  DataToggle
  )
/*++

  Routine Description:
    Set the datatoggle of a specified endpoint

  Arguments:
    UsbIo         -     Given Usb Controller device.
    EndpointAddr  -     Given Endpoint address.
    DataToggle    -     The current data toggle of that endpoint to be set

  Returns:
    VOID

--*/
;

INTERFACE_DESC_LIST_ENTRY           *
FindInterfaceListEntry (
  IN EFI_USB_IO_PROTOCOL    *This
  )
/*++

  Routine Description:
    Find Interface ListEntry.

  Arguments:
    This         -  EFI_USB_IO_PROTOCOL   
  
  Returns:
    INTERFACE_DESC_LIST_ENTRY pointer

--*/
;

ENDPOINT_DESC_LIST_ENTRY            *
FindEndPointListEntry (
  IN EFI_USB_IO_PROTOCOL    *This,
  IN UINT8                  EndPointAddress
  )
/*++

  Routine Description:
    Find EndPoint ListEntry.

  Arguments:
    This         -  EFI_USB_IO_PROTOCOL   
    EndpointAddr -  Endpoint address.
 
  Returns:
    ENDPOINT_DESC_LIST_ENTRY pointer

--*/
;

EFI_STATUS
IsDeviceDisconnected (
  IN USB_IO_CONTROLLER_DEVICE    *UsbIoController,
  IN OUT BOOLEAN                 *Disconnected
  )
/*++

  Routine Description:
    Reset if the device is disconencted or not

  Arguments:
    UsbIoController   -   Indicating the Usb Controller Device.
    Disconnected      -   Indicate whether the device is disconencted or not

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
;

EFI_STATUS
UsbDeviceDeConfiguration (
  IN USB_IO_DEVICE     *UsbIoDevice
  )
/*++

  Routine Description:
    Remove Device, Device Handles, Uninstall Protocols.

  Arguments:
    UsbIoDevice     -   The device to be deconfigured.

  Returns: 
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcGetCapability (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  OUT UINT8                     *MaxSpeed,
  OUT UINT8                     *PortNumber,
  OUT UINT8                     *Is64BitCapable
  )
/*++
  
  Routine Description:
  
    Virtual interface to Retrieves the capablility of root hub ports 
    for both Hc2 and Hc protocol.
    
  Arguments:
  
    UsbBusDev       - A pointer to bus controller of the device.
    MaxSpeed        - A pointer to the number of the host controller.
    PortNumber      - A pointer to the number of the root hub ports.
    Is64BitCapable  - A pointer to the flag for whether controller supports 
                      64-bit memory addressing.
    
  Returns:
  
    EFI_SUCCESS 
          The host controller capability were retrieved successfully.
    EFI_INVALID_PARAMETER 
          MaxSpeed or PortNumber or Is64BitCapable is NULL.
    EFI_DEVICE_ERROR  
          An error was encountered while attempting to retrieve the capabilities.  
          
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcReset (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN UINT16                     Attributes
  )
/*++
  
  Routine Description:
  
    Virtual interface to provides software reset for the USB host controller
    for both Hc2 and Hc protocol.
  
  Arguments:
  
    UsbBusDev   - A pointer to bus controller of the device.
    Attributes  - A bit mask of the reset operation to perform. 
                See below for a list of the supported bit mask values.
  
  #define EFI_USB_HC_RESET_GLOBAL  0x0001               // Hc2 and Hc
  #define EFI_USB_HC_RESET_HOST_CONTROLLER  0x0002      // Hc2 and Hc
  #define EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG  0x0004    // Hc2
  #define EFI_USB_HC_RESET_HOST_WITH_DEBUG  0x0008      // Hc2

  EFI_USB_HC_RESET_GLOBAL 
        If this bit is set, a global reset signal will be sent to the USB bus.
        This resets all of the USB bus logic, including the USB host 
        controller hardware and all the devices attached on the USB bus.
  EFI_USB_HC_RESET_HOST_CONTROLLER  
        If this bit is set, the USB host controller hardware will be reset. 
        No reset signal will be sent to the USB bus.
  EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG
        If this bit is set, a global reset signal will be sent to the USB bus.
        This resets all of the USB bus logic, including the USB host 
        controller hardware and all the devices attached on the USB bus. 
        If this is an EHCI controller and the debug port has configured, then 
        this is will still reset the host controller.
  EFI_USB_HC_RESET_HOST_WITH_DEBUG
        If this bit is set, the USB host controller hardware will be reset. 
        If this is an EHCI controller and the debug port has been configured,
        then this will still reset the host controller.
        
  Returns:
  
    EFI_SUCCESS 
        The reset operation succeeded.
    EFI_INVALID_PARAMETER 
        Attributes is not valid.
    EFI_UNSUPPOURTED
        The type of reset specified by Attributes is not currently supported by
        the host controller hardware.
    EFI_ACCESS_DENIED
        Reset operation is rejected due to the debug port being configured and 
        active; only EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG or 
        EFI_USB_HC_RESET_HOST_WITH_DEBUG reset Atrributes can be used to
        perform reset operation for this host controller.
    EFI_DEVICE_ERROR  
        An error was encountered while attempting to perform 
        the reset operation.
        
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcGetState (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  OUT EFI_USB_HC_STATE          *State
  )
/*++
  
  Routine Description:
  
    Virtual interface to retrieves current state of the USB host controller
    for both Hc2 and Hc protocol.
  
  Arguments:
    
    UsbBusDev - A pointer to bus controller of the device.
    State     - A pointer to the EFI_USB_HC_STATE data structure that 
              indicates current state of the USB host controller.  
              Type EFI_USB_HC_STATE is defined below.
              
    typedef enum {
      EfiUsbHcStateHalt,
      EfiUsbHcStateOperational,
      EfiUsbHcStateSuspend,
      EfiUsbHcStateMaximum
    } EFI_USB_HC_STATE;
  
  Returns:
  
    EFI_SUCCESS 
            The state information of the host controller was returned in State.
    EFI_INVALID_PARAMETER 
            State is NULL.
    EFI_DEVICE_ERROR  
            An error was encountered while attempting to retrieve the 
            host controller's current state.  
            
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcSetState (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN EFI_USB_HC_STATE           State
  )
/*++
  
  Routine Description:
  
    Virtual interface to sets the USB host controller to a specific state
    for both Hc2 and Hc protocol.
  
  Arguments:
    
    UsbBusDev   - A pointer to bus controller of the device.
    State       - Indicates the state of the host controller that will be set.
  
  Returns:
  
    EFI_SUCCESS 
          The USB host controller was successfully placed in the state 
          specified by State.
    EFI_INVALID_PARAMETER 
          State is invalid.
    EFI_DEVICE_ERROR  
          Failed to set the state specified by State due to device error.  
          
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcGetRootHubPortStatus (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN  UINT8                     PortNumber,
  OUT EFI_USB_PORT_STATUS       *PortStatus
  )
/*++
  
  Routine Description:
  
    Virtual interface to retrieves the current status of a USB root hub port
    both for Hc2 and Hc protocol.
  
  Arguments:
  
    UsbBusDev   - A pointer to bus controller of the device.
    PortNumber  - Specifies the root hub port from which the status 
                is to be retrieved.  This value is zero-based. For example, 
                if a root hub has two ports, then the first port is numbered 0,
                and the second port is numbered 1.
    PortStatus  - A pointer to the current port status bits and 
                port status change bits.  
  
  Returns:
  
    EFI_SUCCESS  The status of the USB root hub port specified by PortNumber 
                 was returned in PortStatus.
    EFI_INVALID_PARAMETER PortNumber is invalid. 
    EFI_DEVICE_ERROR      Can't read register     
    
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcSetRootHubPortFeature (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN  UINT8                     PortNumber,
  IN  EFI_USB_PORT_FEATURE      PortFeature
  )
/*++
  
  Routine Description:
    Virual interface to sets a feature for the specified root hub port
    for both Hc2 and Hc protocol.
  
  Arguments:
  
    UsbBusDev   - A pointer to bus controller of the device.
    PortNumber  - Specifies the root hub port whose feature 
                is requested to be set.
    PortFeature - Indicates the feature selector associated 
                with the feature set request. 
  
  Returns:
  
    EFI_SUCCESS 
        The feature specified by PortFeature was set for the 
        USB root hub port specified by PortNumber.
    EFI_INVALID_PARAMETER 
        PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR
        Can't read register
        
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcClearRootHubPortFeature (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN  UINT8                     PortNumber,
  IN  EFI_USB_PORT_FEATURE      PortFeature
  )
/*++
  
  Routine Description:
  
    Virtual interface to clears a feature for the specified root hub port
    for both Hc2 and Hc protocol.
  
  Arguments:
  
    UsbBusDev   - A pointer to bus controller of the device.
    PortNumber  - Specifies the root hub port whose feature 
                is requested to be cleared.
    PortFeature - Indicates the feature selector associated with the 
                feature clear request.
                  
  Returns:
  
    EFI_SUCCESS 
        The feature specified by PortFeature was cleared for the 
        USB root hub port specified by PortNumber.
    EFI_INVALID_PARAMETER 
        PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR
        Can't read register
        
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcControlTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE            *UsbBusDev,
  IN  UINT8                                DeviceAddress,
  IN  UINT8                                DeviceSpeed,
  IN  UINTN                                MaximumPacketLength,
  IN  EFI_USB_DEVICE_REQUEST               *Request,
  IN  EFI_USB_DATA_DIRECTION               TransferDirection,
  IN  OUT VOID                             *Data,
  IN  OUT UINTN                            *DataLength,
  IN  UINTN                                TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
  OUT UINT32                               *TransferResult
  )
/*++
  
  Routine Description:
  
    Virtual interface to submits control transfer to a target USB device
    for both Hc2 and Hc protocol.
  
  Arguments:
    
    UsbBusDev     - A pointer to bus controller of the device.
    DeviceAddress - Represents the address of the target device on the USB,
                  which is assigned during USB enumeration.
    DeviceSpeed   - Indicates target device speed.
    MaximumPacketLength - Indicates the maximum packet size that the 
                        default control transfer endpoint is capable of 
                        sending or receiving.
    Request       - A pointer to the USB device request that will be sent 
                  to the USB device. 
    TransferDirection - Specifies the data direction for the transfer.
                      There are three values available, DataIn, DataOut 
                      and NoData.
    Data          - A pointer to the buffer of data that will be transmitted 
                  to USB device or received from USB device.
    DataLength    - Indicates the size, in bytes, of the data buffer 
                  specified by Data.
    TimeOut       - Indicates the maximum time, in microseconds, 
                  which the transfer is allowed to complete.
    Translator      - A pointr to the transaction translator data.
    TransferResult  - A pointer to the detailed result information generated 
                    by this control transfer.
                    
  Returns:
  
    EFI_SUCCESS 
        The control transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  
        The control transfer could not be completed due to a lack of resources.
    EFI_INVALID_PARAMETER 
        Some parameters are invalid.
    EFI_TIMEOUT 
        The control transfer failed due to timeout.
    EFI_DEVICE_ERROR  
        The control transfer failed due to host controller or device error. 
        Caller should check TranferResult for detailed error information.

--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcBulkTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE           *UsbBusDev,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
/*++
  
  Routine Description:
  
    Virtual interface to submits bulk transfer to a bulk endpoint of a USB device
    both for Hc2 and Hc protocol.
    
  Arguments:
    
    UsbBusDev         - A pointer to bus controller of the device.
    DeviceAddress     - Represents the address of the target device on the USB,
                      which is assigned during USB enumeration.               
    EndPointAddress   - The combination of an endpoint number and an 
                      endpoint direction of the target USB device. 
                      Each endpoint address supports data transfer in 
                      one direction except the control endpoint 
                      (whose default endpoint address is 0). 
                      It is the caller's responsibility to make sure that 
                      the EndPointAddress represents a bulk endpoint.                  
    DeviceSpeed       - Indicates device speed. The supported values are EFI_USB_SPEED_FULL
                      and EFI_USB_SPEED_HIGH.
    MaximumPacketLength - Indicates the maximum packet size the target endpoint
                        is capable of sending or receiving.                 
    DataBuffersNumber - Number of data buffers prepared for the transfer.
    Data              - Array of pointers to the buffers of data that will be transmitted 
                      to USB device or received from USB device.              
    DataLength        - When input, indicates the size, in bytes, of the data buffer
                      specified by Data. When output, indicates the actually 
                      transferred data size.              
    DataToggle        - A pointer to the data toggle value. On input, it indicates 
                      the initial data toggle value the bulk transfer should adopt;
                      on output, it is updated to indicate the data toggle value 
                      of the subsequent bulk transfer. 
    Translator        - A pointr to the transaction translator data. 
    TimeOut           - Indicates the maximum time, in microseconds, which the 
                      transfer is allowed to complete.              
    TransferResult    - A pointer to the detailed result information of the 
                      bulk transfer.

  Returns:
  
    EFI_SUCCESS 
        The bulk transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  
        The bulk transfer could not be submitted due to lack of resource.
    EFI_INVALID_PARAMETER 
        Some parameters are invalid.
    EFI_TIMEOUT 
        The bulk transfer failed due to timeout.
    EFI_DEVICE_ERROR  
        The bulk transfer failed due to host controller or device error.
        Caller should check TranferResult for detailed error information.

--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcAsyncInterruptTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE             * UsbBusDev,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  BOOLEAN                               IsNewTransfer,
  IN OUT UINT8                              *DataToggle,
  IN  UINTN                                 PollingInterval,
  IN  UINTN                                 DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR * Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
  IN  VOID                                  *Context OPTIONAL
  )
/*++
  
  Routine Description:
  
    Virtual interface to submits an asynchronous interrupt transfer to an 
    interrupt endpoint of a USB device for both Hc2 and Hc protocol.
  
  Arguments:
    
    UsbBusDev       - A pointer to bus controller of the device.
    DeviceAddress   - Represents the address of the target device on the USB,
                    which is assigned during USB enumeration.                
    EndPointAddress - The combination of an endpoint number and an endpoint 
                    direction of the target USB device. Each endpoint address 
                    supports data transfer in one direction except the 
                    control endpoint (whose default endpoint address is 0). 
                    It is the caller's responsibility to make sure that 
                    the EndPointAddress represents an interrupt endpoint.              
    DeviceSpeed     - Indicates device speed.
    MaximumPacketLength  - Indicates the maximum packet size the target endpoint
                         is capable of sending or receiving.                   
    IsNewTransfer   - If TRUE, an asynchronous interrupt pipe is built between
                    the host and the target interrupt endpoint. 
                    If FALSE, the specified asynchronous interrupt pipe 
                    is canceled.               
    DataToggle      - A pointer to the data toggle value.  On input, it is valid 
                    when IsNewTransfer is TRUE, and it indicates the initial 
                    data toggle value the asynchronous interrupt transfer 
                    should adopt.  
                    On output, it is valid when IsNewTransfer is FALSE, 
                    and it is updated to indicate the data toggle value of 
                    the subsequent asynchronous interrupt transfer.              
    PollingInterval - Indicates the interval, in milliseconds, that the 
                    asynchronous interrupt transfer is polled.  
                    This parameter is required when IsNewTransfer is TRUE.               
    DataLength      - Indicates the length of data to be received at the 
                    rate specified by PollingInterval from the target 
                    asynchronous interrupt endpoint.  This parameter 
                    is only required when IsNewTransfer is TRUE.             
    Translator      - A pointr to the transaction translator data.
    CallBackFunction  - The Callback function.This function is called at the 
                      rate specified by PollingInterval.This parameter is 
                      only required when IsNewTransfer is TRUE.               
    Context         - The context that is passed to the CallBackFunction.
                    - This is an optional parameter and may be NULL.
  
  Returns:
  
    EFI_SUCCESS 
        The asynchronous interrupt transfer request has been successfully 
        submitted or canceled.
    EFI_INVALID_PARAMETER 
        Some parameters are invalid.
    EFI_OUT_OF_RESOURCES  
        The request could not be completed due to a lack of resources.  
    EFI_DEVICE_ERROR
        Can't read register
        
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcSyncInterruptTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE             *UsbBusDev,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN OUT VOID                               *Data,
  IN OUT UINTN                              *DataLength,
  IN OUT UINT8                              *DataToggle,
  IN  UINTN                                 TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT UINT32                                *TransferResult
  )
/*++
  
  Routine Description:
  
    Vitual interface to submits synchronous interrupt transfer to an interrupt endpoint 
    of a USB device for both Hc2 and Hc protocol.
  
  Arguments:
    
    UsbBusDev       - A pointer to bus controller of the device.
    DeviceAddress   - Represents the address of the target device on the USB, 
                    which is assigned during USB enumeration.
    EndPointAddress   - The combination of an endpoint number and an endpoint 
                      direction of the target USB device. Each endpoint 
                      address supports data transfer in one direction 
                      except the control endpoint (whose default 
                      endpoint address is 0). It is the caller's responsibility
                      to make sure that the EndPointAddress represents 
                      an interrupt endpoint. 
    DeviceSpeed     - Indicates device speed.
    MaximumPacketLength - Indicates the maximum packet size the target endpoint 
                        is capable of sending or receiving.
    Data            - A pointer to the buffer of data that will be transmitted 
                    to USB device or received from USB device.
    DataLength      - On input, the size, in bytes, of the data buffer specified 
                    by Data. On output, the number of bytes transferred.
    DataToggle      - A pointer to the data toggle value. On input, it indicates
                    the initial data toggle value the synchronous interrupt 
                    transfer should adopt; 
                    on output, it is updated to indicate the data toggle value 
                    of the subsequent synchronous interrupt transfer. 
    TimeOut         - Indicates the maximum time, in microseconds, which the 
                    transfer is allowed to complete.
    Translator      - A pointr to the transaction translator data.
    TransferResult  - A pointer to the detailed result information from 
                    the synchronous interrupt transfer.  

  Returns:
  
    EFI_SUCCESS 
        The synchronous interrupt transfer was completed successfully.
    EFI_OUT_OF_RESOURCES  
        The synchronous interrupt transfer could not be submitted due 
        to lack of resource.
    EFI_INVALID_PARAMETER 
        Some parameters are invalid.
    EFI_TIMEOUT 
        The synchronous interrupt transfer failed due to timeout.
    EFI_DEVICE_ERROR  
        The synchronous interrupt transfer failed due to host controller 
        or device error. Caller should check TranferResult for detailed 
        error information.  
        
--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcIsochronousTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE             *UsbBusDev,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  UINT8                                 DataBuffersNumber,
  IN  OUT VOID                              *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                                 DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator,
  OUT UINT32                                *TransferResult
  )
/*++
  
  Routine Description:
  
    Virtual interface to submits isochronous transfer to a target USB device
    for both Hc2 and Hc protocol.
  
  Arguments:
    
    UsbBusDev        - A pointer to bus controller of the device.
    DeviceAddress    - Represents the address of the target device on the USB,
                     which is assigned during USB enumeration.
    EndPointAddress  - End point address
    DeviceSpeed      - Indicates device speed.
    MaximumPacketLength    - Indicates the maximum packet size that the 
                           default control transfer endpoint is capable of 
                           sending or receiving.
    DataBuffersNumber - Number of data buffers prepared for the transfer.
    Data              - Array of pointers to the buffers of data that will be 
                      transmitted to USB device or received from USB device.
    DataLength        - Indicates the size, in bytes, of the data buffer 
                      specified by Data.
    Translator        - A pointr to the transaction translator data.
    TransferResult    - A pointer to the detailed result information generated 
                      by this control transfer.               
                      
  Returns:
  
    EFI_UNSUPPORTED 

--*/
;

EFI_STATUS
EFIAPI
UsbVirtualHcAsyncIsochronousTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE           *UsbBusDev,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN OUT VOID                             *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN  VOID                                *Context
  )
/*++
  
  Routine Description:
  
    Vitual interface to submits Async isochronous transfer to a target USB device
    for both Hc2 and Hc protocol.
  
  Arguments:
  
    UsbBusDev           - A pointer to bus controller of the device.
    DeviceAddress       - Represents the address of the target device on the USB,
                        which is assigned during USB enumeration.
    EndPointAddress     - End point address
    DeviceSpeed         - Indicates device speed.
    MaximumPacketLength - Indicates the maximum packet size that the 
                        default control transfer endpoint is capable of 
                        sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data that will be transmitted 
                        to USB device or received from USB device.
    DataLength          - Indicates the size, in bytes, of the data buffer 
                        specified by Data.
    Translator          - A pointr to the transaction translator data.
    IsochronousCallBack - When the transfer complete, the call back function will be called
    Context             - Pass to the call back function as parameter
                    
  Returns:
  
    EFI_UNSUPPORTED 

--*/
;

EFI_STATUS
EFIAPI
UsbPortReset (
  IN EFI_USB_IO_PROTOCOL     *This
  )
/*++

  Routine Description:
    Resets and reconfigures the USB controller.  This function will
    work for all USB devices except USB Hub Controllers.

  Arguments:
    This          -   Indicates the calling context.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_DEVICE_ERROR

--*/
;

VOID
InitializeUsbIoInstance (
  IN USB_IO_CONTROLLER_DEVICE     *UsbIoController
  )
/*++

Routine Description:

  Initialize the instance of UsbIo controller

Arguments:

  UsbIoController - A pointer to controller structure of UsbIo

Returns:

--*/
;

#endif
