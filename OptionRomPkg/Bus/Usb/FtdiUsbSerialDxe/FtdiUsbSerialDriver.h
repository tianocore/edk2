/** @file
  Header file for USB Serial Driver's Data Structures.

Copyright (c) 2004 - 2013, Intel Corporation. All rights reserved.
Portions Copyright 2012 Ashley DeSimone
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FTDI_USB_SERIAL_DRIVER_H_
#define _FTDI_USB_SERIAL_DRIVER_H_

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/UsbIo.h>
#include <Protocol/SerialIo.h>

//
// US English LangID
//
#define USB_US_LANG_ID  0x0409

//
// Supported Vendor Ids
//
#define VID_FTDI    0x0403

//
// Supported product ids
//
#define DID_FTDI_FT232    0x6001

//
// FTDI Commands
//
#define FTDI_COMMAND_RESET_PORT          0
#define FTDI_COMMAND_MODEM_CTRL          1
#define FTDI_COMMAND_SET_FLOW_CTRL       2
#define FTDI_COMMAND_SET_BAUDRATE        3
#define FTDI_COMMAND_SET_DATA            4
#define FTDI_COMMAND_GET_MODEM_STATUS    5
#define FTDI_COMMAND_SET_EVENT_CHAR      6
#define FTDI_COMMAND_SET_ERROR_CHAR      7
#define FTDI_COMMAND_SET_LATENCY_TIMER   9
#define FTDI_COMMAND_GET_LATENCY_TIMER   10

//
// FTDI_PORT_IDENTIFIER
// Used in the usb control transfers that issue FTDI commands as the index value.
//
#define FTDI_PORT_IDENTIFIER    0x1 // For FTDI USB serial adapter the port
                                    // identifier is always 1.

//
// RESET_PORT
//
#define RESET_PORT_RESET        0x0 // Purges RX and TX, clears DTR and RTS sets
                                    // flow control to none, disables event
                                    // trigger, sets the event char to 0x0d and
                                    // does nothing to baudrate or data settings
#define RESET_PORT_PURGE_RX     0x1
#define RESET_PORT_PURGE_TX     0x2

//
// SET_FLOW_CONTROL
//
#define NO_FLOW_CTRL                     0x0
#define XON_XOFF_CTRL                    0x4

//
// SET_BAUD_RATE
// To set baud rate, one must calculate an encoding of the baud rate from
// UINT32 to UINT16.See EncodeBaudRateForFtdi() for details
//
#define FTDI_UART_FREQUENCY              3000000
#define FTDI_MIN_DIVISOR                 0x20
#define FTDI_MAX_DIVISOR                 0x3FFF8
//
// Special case baudrate values
// 300,000 and 200,000 are special cases for calculating the encoded baudrate
//
#define FTDI_SPECIAL_CASE_300_MIN        (3000000 * 100) / 103 // minimum adjusted
                                                               // value for 300,000
#define FTDI_SPECIAL_CASE_300_MAX        (3000000 * 100) / 97  // maximum adjusted
                                                               // value for 300,000
#define FTDI_SPECIAL_CASE_200_MIN        (2000000 * 100) / 103 // minimum adjusted
                                                               // value for 200,000
#define FTDI_SPECIAL_CASE_200_MAX        (2000000 * 100) / 97  // maximum adjusted
                                                               // value for 200,000
//
// Min and max frequency values that the FTDI chip can attain
//.all generated frequencies must be between these values
//
#define FTDI_MIN_FREQUENCY              46601941 // (3MHz * 1600) / 103 = 46601941
#define FTDI_MAX_FREQUENCY              49484536 // (3MHz * 1600) / 97 = 49484536

//
// SET_DATA_BITS
//
#define SET_DATA_BITS(n)                 (n)

//
// SET_PARITY
//
#define SET_PARITY_NONE                   0x0
#define SET_PARITY_ODD                    BIT8 // (0x1 << 8)
#define SET_PARITY_EVEN                   BIT9 // (0x2 << 8)
#define SET_PARITY_MARK                   BIT9 | BIT8 // (0x3 << 8)
#define SET_PARITY_SPACE                  BIT10 // (0x4 << 8)

//
// SET_STOP_BITS
//
#define SET_STOP_BITS_1                   0x0
#define SET_STOP_BITS_15                  BIT11 // (0x1 << 11)
#define SET_STOP_BITS_2                   BIT12 // (0x2 << 11)

//
// SET_MODEM_CTRL
// SET_DTR_HIGH = (1 | (1 << 8)), SET_DTR_LOW = (0 | (1 << 8)
// SET_RTS_HIGH = (2 | (2 << 8)), SET_RTS_LOW = (0 | (2 << 8)
//
#define SET_DTR_HIGH                     (BIT8 | BIT0)
#define SET_DTR_LOW                      (BIT8)
#define SET_RTS_HIGH                     (BIT9 | BIT1)
#define SET_RTS_LOW                      (BIT9)

//
// MODEM_STATUS
//
#define CTS_MASK                         BIT4
#define DSR_MASK                         BIT5
#define RI_MASK                          BIT6
#define SD_MASK                          BIT7
#define MSR_MASK                         (CTS_MASK | DSR_MASK | RI_MASK | SD_MASK)

//
// Macro used to check for USB transfer errors
//
#define USB_IS_ERROR(Result, Error)           (((Result) & (Error)) != 0)

//
// USB request timeouts
//
#define WDR_TIMEOUT        5000  // default urb timeout in ms
#define WDR_SHORT_TIMEOUT  1000  // shorter urb timeout in ms

//
// FTDI timeout
//
#define FTDI_TIMEOUT       16

//
// FTDI FIFO depth
//
#define FTDI_MAX_RECEIVE_FIFO_DEPTH  384

//
// FTDI Endpoint Descriptors
//
#define FTDI_ENDPOINT_ADDRESS_IN   0x81 //the endpoint address for the in enpoint generated by the device
#define FTDI_ENDPOINT_ADDRESS_OUT  0x02 //the endpoint address for the out endpoint generated by the device

//
// Max buffer size for USB transfers
//
#define SW_FIFO_DEPTH 1024

//
// struct to define a usb device as a vendor and product id pair
//
typedef struct {
  UINTN     VendorId;
  UINTN     DeviceId;
} USB_DEVICE;

//
//struct to describe the control bits of the device
//true indicates enabled
//false indicates disabled
// 
typedef struct {
  BOOLEAN    HardwareFlowControl;
  BOOLEAN    DtrState;
  BOOLEAN    RtsState;
  BOOLEAN    HardwareLoopBack;
  BOOLEAN    SoftwareLoopBack;
} CONTROL_BITS;

//
//struct to describe the status bits of the device 
//true indicates enabled
//false indicated disabled
//
typedef struct {
  BOOLEAN    CtsState;
  BOOLEAN    DsrState;
  BOOLEAN    RiState;
  BOOLEAN    SdState;
} STATUS_BITS;

//
// Structure to describe the last attributes of the Usb Serial device
//
typedef struct {
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  UINT32              Timeout;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;
} PREVIOUS_ATTRIBUTES;

//
// Structure to describe USB serial device
//
#define USB_SER_DEV_SIGNATURE  SIGNATURE_32 ('u', 's', 'b', 's')

typedef struct {
  UINTN                         Signature;
  EFI_HANDLE                    ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *ParentDevicePath;
  UART_DEVICE_PATH              UartDevicePath;
  UART_FLOW_CONTROL_DEVICE_PATH FlowControlDevicePath;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   InEndpointDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   OutEndpointDescriptor;
  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;
  UINT32                        DataBufferHead;
  UINT32                        DataBufferTail;
  UINT8                         *DataBuffer;
  EFI_SERIAL_IO_PROTOCOL        SerialIo;
  BOOLEAN                       Shutdown;
  EFI_EVENT                     PollingLoop;
  UINT32                        ControlBits;
  PREVIOUS_ATTRIBUTES           LastSettings;
  CONTROL_BITS                  ControlValues;
  STATUS_BITS                   StatusValues;
  UINT8                         ReadBuffer[512];
} USB_SER_DEV;

#define USB_SER_DEV_FROM_THIS(a) \
  CR(a, USB_SER_DEV, SerialIo, USB_SER_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gUsbSerialDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbSerialComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbSerialComponentName2;

//
// Functions of Driver Binding Protocol
//
/**
  Check whether USB Serial driver supports this device.

  @param  This[in]                   The USB Serial driver binding protocol.
  @param  Controller[in]             The controller handle to check.
  @param  RemainingDevicePath[in]    The remaining device path.

  @retval EFI_SUCCESS                The driver supports this controller.
  @retval other                      This device isn't supported.

**/
EFI_STATUS
EFIAPI
UsbSerialDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Starts the Serial device with this driver.

  This function produces Serial IO Protocol and initializes the USB
  Serial device to manage this USB Serial device.

  @param  This[in]                   The USB Serial driver binding instance.
  @param  Controller[in]             Handle of device to bind driver to.
  @param  RemainingDevicePath[in]    Optional parameter use to pick a specific
                                     child device to start.

  @retval EFI_SUCCESS                The controller is controlled by the USB
                                     Serial driver.
  @retval EFI_UNSUPPORTED            No interrupt endpoint can be found.
  @retval Other                      This controller cannot be started.

**/
EFI_STATUS
EFIAPI
UsbSerialDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop the USB Serial device handled by this driver.

  @param  This[in]                   The USB Serial driver binding protocol.
  @param  Controller[in]             The controller to release.
  @param  NumberOfChildren[in]       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer[in]      The array of child handle.

  @retval EFI_SUCCESS                The device was stopped.
  @retval EFI_UNSUPPORTED            Simple Text In Protocol or Simple Text In Ex
                                     Protocol is not installed on Controller.
  @retval EFI_DEVICE_ERROR           The device could not be stopped due to a
                                     device error.
  @retval Others                     Fail to uninstall protocols attached on the
                                     device.

**/
EFI_STATUS
EFIAPI
UsbSerialDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// Serial IO Member Functions
//

/**
  Writes data to a serial device.

  @param  This[in]                   Protocol instance pointer.
  @param  BufferSize[in, out]        On input, the size of the Buffer. On output,
                                     the amount of data actually written.
  @param  Buffer[in]                 The buffer of data to write

  @retval EFI_SUCCESS                The data was written.
  @retval EFI_DEVICE_ERROR           The device reported an error.
  @retval EFI_TIMEOUT                The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
WriteSerialIo (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  );

/**
  Reads data from a serial device.

  @param  This[in]                   Protocol instance pointer.
  @param  BufferSize[in, out]        On input, the size of the Buffer. On output,
                                     the amount of data returned in Buffer.
  @param  Buffer[out]                The buffer to return the data into.

  @retval EFI_SUCCESS                The data was read.
  @retval EFI_DEVICE_ERROR           The device reported an error.
  @retval EFI_TIMEOUT                The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
ReadSerialIo (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Retrieves the status of the control bits on a serial device.

  @param  This[in]               Protocol instance pointer.
  @param  Control[out]           A pointer to return the current Control signals
                                 from the serial device.

  @retval EFI_SUCCESS            The control bits were read from the serial
                                 device.
  @retval EFI_DEVICE_ERROR       The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
GetControlBits (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  );

/**
  Set the control bits on a serial device.

  @param  This[in]             Protocol instance pointer.
  @param  Control[in]          Set the bits of Control that are settable.

  @retval EFI_SUCCESS          The new control bits were set on the serial device.
  @retval EFI_UNSUPPORTED      The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR     The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SetControlBits (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  );

/**
  Calls SetAttributesInternal() to set the baud rate, receive FIFO depth,
  transmit/receice time out, parity, data buts, and stop bits on a serial device.

  @param  This[in]             Protocol instance pointer.
  @param  BaudRate[in]         The requested baud rate. A BaudRate value of 0
                               will use the device's default interface speed.
  @param  ReveiveFifoDepth[in] The requested depth of the FIFO on the receive
                               side of the serial interface. A ReceiveFifoDepth
                               value of 0 will use the device's default FIFO
                               depth.
  @param  Timeout[in]          The requested time out for a single character in
                               microseconds.This timeout applies to both the
                               transmit and receive side of the interface.A
                               Timeout value of 0 will use the device's default
                               time out value.
  @param  Parity[in]           The type of parity to use on this serial device.A
                               Parity value of DefaultParity will use the
                               device's default parity value.
  @param  DataBits[in]         The number of data bits to use on the serial
                               device. A DataBits value of 0 will use the
                               device's default data bit setting.
  @param  StopBits[in]         The number of stop bits to use on this serial
                               device. A StopBits value of DefaultStopBits will
                               use the device's default number of stop bits.

  @retval EFI_SUCCESS          The attributes were set
  @retval EFI_DEVICE_ERROR     The attributes were not able to be

**/
EFI_STATUS
EFIAPI
SetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT64                  BaudRate,
  IN UINT32                  ReceiveFifoDepth,
  IN UINT32                  Timeout,
  IN EFI_PARITY_TYPE         Parity,
  IN UINT8                   DataBits,
  IN EFI_STOP_BITS_TYPE      StopBits
  );

/**
  Reset the serial device.

  @param  This              Protocol instance pointer.

  @retval EFI_SUCCESS       The device was reset.
  @retval EFI_DEVICE_ERROR  The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  );

//
// EFI Component Name Functions
//

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]                   A pointer to the EFI_COMPONENT_NAME2_PROTOCOL
                                     or EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language[in]               A pointer to a Null-terminated ASCII string
                                     array indicating the language. This is the
                                     language of the driver name that the caller
                                     is requesting, and it must match one of the
                                     languages specified in SupportedLanguages.
                                     The number of languages supported by a
                                     driver is up to the driver writer. Language
                                     is specified in RFC 4646 or ISO 639-2
                                     language code format.
  @param  DriverName[out]            A pointer to the Unicode string to return.
                                     This Unicode string is the name of the
                                     driver specified by This in the language
                                     specified by Language.

  @retval EFI_SUCCESS                The Unicode string for the Driver specified
                                     by This and the language specified by
                                     Language was returned in DriverName.
  @retval EFI_INVALID_PARAMETER      Language is NULL.
  @retval EFI_INVALID_PARAMETER      DriverName is NULL.
  @retval EFI_UNSUPPORTED            The driver specified by This does not
                                     support the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbSerialComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]                   A pointer to the EFI_COMPONENT_NAME2_PROTOCOL
                                     or EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle[in]       The handle of a controller that the driver
                                     specified by This is managing.  This handle
                                     specifies the controller whose name is to
                                     be returned.
  @param  ChildHandle[in]            The handle of the child controller to
                                     retrieve the name of. This is an optional
                                     parameter that may be NULL. It will be NULL
                                     for device drivers. It will also be NULL
                                     for a bus drivers that wish to retrieve the
                                     name of the bus controller. It will not be
                                     NULL for a bus driver that wishes to
                                     retrieve the name of a child controller.
  @param  Language[in]               A pointer to a Null-terminated ASCII string
                                     array indicating the language.  This is the
                                     language of the driver name that the caller
                                     is requesting, and it must match one of the
                                     languages specified in SupportedLanguages.
                                     The number of languages supported by a
                                     driver is up to the driver writer. Language
                                     is specified in RFC 4646 or ISO 639-2
                                     language code format.
  @param  ControllerName[out]        A pointer to the Unicode string to return.
                                     This Unicode string is the name of the
                                     controller specified by ControllerHandle
                                     and ChildHandle in the language specified
                                     by Language from the point of view of the
                                     driver specified by This.

  @retval EFI_SUCCESS                The Unicode string for the user readable
                                     name in the language specified by Language
                                     for the driver specified by This was
                                     returned in DriverName.
  @retval EFI_INVALID_PARAMETER      ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER      ChildHandle is not NULL and it is not a
                                     valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER      Language is NULL.
  @retval EFI_INVALID_PARAMETER      ControllerName is NULL.
  @retval EFI_UNSUPPORTED            The driver specified by This is not
                                     currently managing the controller specified
                                     by ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED            The driver specified by This does not
                                     support the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbSerialComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle      OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  );

#endif
