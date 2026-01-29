/** @file
  Header file for PciSioSerial Driver

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <Uefi.h>

#include <IndustryStandard/Pci.h>

#include <Protocol/SuperIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/SerialIo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL   gSerialControllerDriver;
extern EFI_COMPONENT_NAME_PROTOCOL   gPciSioSerialComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gPciSioSerialComponentName2;

#define SIO_SERIAL_PORT_NAME  L"SIO Serial Port #%d"
#define PCI_SERIAL_PORT_NAME  L"PCI Serial Port #%d"
#define SERIAL_PORT_NAME_LEN  (sizeof (SIO_SERIAL_PORT_NAME) / sizeof (CHAR16) + MAXIMUM_VALUE_CHARACTERS)

//
// Internal Data Structures
//
#define TIMEOUT_STALL_INTERVAL  10

#pragma pack(1)
///
/// PcdPciSerialParameters contains zero or more instances of the below structure.
/// If a PCI device contains multiple UARTs, PcdPciSerialParameters needs to contain
/// two instances of the below structure, with the VendorId and DeviceId equals to the
/// device ID and vendor ID of the device. If the PCI device uses the first two BARs
/// to support multiple UARTs, BarIndex of first instance equals to 0 and BarIndex of
/// second one equals to 1; if the PCI device uses the first BAR to support multiple
/// UARTs, BarIndex of both instance equals to 0 and Offset of first instance equals
/// to 0 while Offset of second one equals to some value bigger or equal to 8.
/// For certain UART whose register needs to be accessed in DWORD aligned address,
/// RegisterStride equals to 4.
///
typedef struct {
  UINT16    VendorId;          ///< Vendor ID to match the PCI device.  The value 0xFFFF terminates the list of entries.
  UINT16    DeviceId;          ///< Device ID to match the PCI device
  UINT32    ClockRate;         ///< UART clock rate.  Set to 0 for default clock rate of 1843200 Hz
  UINT64    Offset;            ///< The byte offset into to the BAR
  UINT8     BarIndex;          ///< Which BAR to get the UART base address
  UINT8     RegisterStride;    ///< UART register stride in bytes.  Set to 0 for default register stride of 1 byte.
  UINT16    ReceiveFifoDepth;  ///< UART receive FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  UINT16    TransmitFifoDepth; ///< UART transmit FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  UINT8     Reserved[2];
} PCI_SERIAL_PARAMETER;
#pragma pack()

#define SERIAL_MAX_FIFO_SIZE  17      ///< Actual FIFO size is 16. FIFO based on circular wastes one unit.
typedef struct {
  UINT16    Head;                       ///< Head pointer of the FIFO. Empty when (Head == Tail).
  UINT16    Tail;                       ///< Tail pointer of the FIFO. Full when ((Tail + 1) % SERIAL_MAX_FIFO_SIZE == Head).
  UINT8     Data[SERIAL_MAX_FIFO_SIZE]; ///< Store the FIFO data.
} SERIAL_DEV_FIFO;

typedef union {
  EFI_PCI_IO_PROTOCOL    *PciIo;
  EFI_SIO_PROTOCOL       *Sio;
} PARENT_IO_PROTOCOL_PTR;

typedef struct {
  EFI_PCI_IO_PROTOCOL    *PciIo;          // Pointer to parent PciIo instance.
  UINTN                  ChildCount;      // Count of child SerialIo instance.
  UINT64                 PciAttributes;   // Original PCI attributes.
} PCI_DEVICE_INFO;

typedef struct {
  UINT32                      Signature;
  EFI_HANDLE                  Handle;
  EFI_SERIAL_IO_PROTOCOL      SerialIo;
  EFI_SERIAL_IO_MODE          SerialMode;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath;
  UART_DEVICE_PATH            UartDevicePath;

  EFI_PHYSICAL_ADDRESS        BaseAddress;    ///< UART base address
  BOOLEAN                     MmioAccess;     ///< TRUE for MMIO, FALSE for IO
  UINT8                       RegisterStride; ///< UART Register Stride
  UINT32                      ClockRate;      ///< UART clock rate

  UINT16                      ReceiveFifoDepth; ///< UART receive FIFO depth in bytes.
  SERIAL_DEV_FIFO             Receive;          ///< The FIFO used to store received data

  UINT16                      TransmitFifoDepth; ///< UART transmit FIFO depth in bytes.
  SERIAL_DEV_FIFO             Transmit;          ///< The FIFO used to store to-transmit data

  BOOLEAN                     SoftwareLoopbackEnable;
  BOOLEAN                     HardwareFlowControl;
  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;
  BOOLEAN                     ContainsControllerNode; ///< TRUE if the device produced contains Controller node
  UINT32                      Instance;
  PCI_DEVICE_INFO             *PciDeviceInfo;
} SERIAL_DEV;

#define SERIAL_DEV_SIGNATURE  SIGNATURE_32 ('s', 'e', 'r', 'd')
#define SERIAL_DEV_FROM_THIS(a)  CR (a, SERIAL_DEV, SerialIo, SERIAL_DEV_SIGNATURE)

//
// Serial Driver Defaults
//
#define SERIAL_PORT_DEFAULT_TIMEOUT       1000000
#define SERIAL_PORT_SUPPORT_CONTROL_MASK  (EFI_SERIAL_CLEAR_TO_SEND                |       \
                                                 EFI_SERIAL_DATA_SET_READY               | \
                                                 EFI_SERIAL_RING_INDICATE                | \
                                                 EFI_SERIAL_CARRIER_DETECT               | \
                                                 EFI_SERIAL_REQUEST_TO_SEND              | \
                                                 EFI_SERIAL_DATA_TERMINAL_READY          | \
                                                 EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE     | \
                                                 EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE     | \
                                                 EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE | \
                                                 EFI_SERIAL_OUTPUT_BUFFER_EMPTY          | \
                                                 EFI_SERIAL_INPUT_BUFFER_EMPTY)

#define SERIAL_PORT_MIN_TIMEOUT  1                    // 1 uS
#define SERIAL_PORT_MAX_TIMEOUT  100000000            // 100 seconds
//
// UART Registers
//
#define SERIAL_REGISTER_THR  0 ///< WO   Transmit Holding Register
#define SERIAL_REGISTER_RBR  0 ///< RO   Receive Buffer Register
#define SERIAL_REGISTER_DLL  0 ///< R/W  Divisor Latch LSB
#define SERIAL_REGISTER_DLM  1 ///< R/W  Divisor Latch MSB
#define SERIAL_REGISTER_IER  1 ///< R/W  Interrupt Enable Register
#define SERIAL_REGISTER_IIR  2 ///< RO   Interrupt Identification Register
#define SERIAL_REGISTER_FCR  2 ///< WO   FIFO Cotrol Register
#define SERIAL_REGISTER_LCR  3 ///< R/W  Line Control Register
#define SERIAL_REGISTER_MCR  4 ///< R/W  Modem Control Register
#define SERIAL_REGISTER_LSR  5 ///< R/W  Line Status Register
#define SERIAL_REGISTER_MSR  6 ///< R/W  Modem Status Register
#define SERIAL_REGISTER_SCR  7 ///< R/W  Scratch Pad Register
#pragma pack(1)

///
/// Interrupt Enable Register
///
typedef union {
  struct {
    UINT8    Ravie    : 1; ///< Receiver Data Available Interrupt Enable
    UINT8    Theie    : 1; ///< Transmistter Holding Register Empty Interrupt Enable
    UINT8    Rie      : 1; ///< Receiver Interrupt Enable
    UINT8    Mie      : 1; ///< Modem Interrupt Enable
    UINT8    Reserved : 4;
  } Bits;
  UINT8    Data;
} SERIAL_PORT_IER;

///
/// FIFO Control Register
///
typedef union {
  struct {
    UINT8    TrFIFOE  : 1; ///< Transmit and Receive FIFO Enable
    UINT8    ResetRF  : 1; ///< Reset Reciever FIFO
    UINT8    ResetTF  : 1; ///< Reset Transmistter FIFO
    UINT8    Dms      : 1; ///< DMA Mode Select
    UINT8    Reserved : 1;
    UINT8    TrFIFO64 : 1; ///< Enable 64 byte FIFO
    UINT8    Rtb      : 2; ///< Receive Trigger Bits
  } Bits;
  UINT8    Data;
} SERIAL_PORT_FCR;

///
/// Line Control Register
///
typedef union {
  struct {
    UINT8    SerialDB : 2; ///< Number of Serial Data Bits
    UINT8    StopB    : 1; ///< Number of Stop Bits
    UINT8    ParEn    : 1; ///< Parity Enable
    UINT8    EvenPar  : 1; ///< Even Parity Select
    UINT8    SticPar  : 1; ///< Sticky Parity
    UINT8    BrCon    : 1; ///< Break Control
    UINT8    DLab     : 1; ///< Divisor Latch Access Bit
  } Bits;
  UINT8    Data;
} SERIAL_PORT_LCR;

///
/// Modem Control Register
///
typedef union {
  struct {
    UINT8    DtrC     : 1; ///< Data Terminal Ready Control
    UINT8    Rts      : 1; ///< Request To Send Control
    UINT8    Out1     : 1; ///< Output1
    UINT8    Out2     : 1; ///< Output2, used to disable interrupt
    UINT8    Lme      : 1; ///< Loopback Mode Enable
    UINT8    Reserved : 3;
  } Bits;
  UINT8    Data;
} SERIAL_PORT_MCR;

///
/// Line Status Register
///
typedef union {
  struct {
    UINT8    Dr    : 1; ///< Receiver Data Ready Status
    UINT8    Oe    : 1; ///< Overrun Error Status
    UINT8    Pe    : 1; ///< Parity Error Status
    UINT8    Fe    : 1; ///< Framing Error Status
    UINT8    Bi    : 1; ///< Break Interrupt Status
    UINT8    Thre  : 1; ///< Transmistter Holding Register Status
    UINT8    Temt  : 1; ///< Transmitter Empty Status
    UINT8    FIFOe : 1; ///< FIFO Error Status
  } Bits;
  UINT8    Data;
} SERIAL_PORT_LSR;

///
/// Modem Status Register
///
typedef union {
  struct {
    UINT8    DeltaCTS       : 1; ///< Delta Clear To Send Status
    UINT8    DeltaDSR       : 1; ///< Delta Data Set Ready Status
    UINT8    TrailingEdgeRI : 1; ///< Trailing Edge of Ring Indicator Status
    UINT8    DeltaDCD       : 1; ///< Delta Data Carrier Detect Status
    UINT8    Cts            : 1; ///< Clear To Send Status
    UINT8    Dsr            : 1; ///< Data Set Ready Status
    UINT8    Ri             : 1; ///< Ring Indicator Status
    UINT8    Dcd            : 1; ///< Data Carrier Detect Status
  } Bits;
  UINT8    Data;
} SERIAL_PORT_MSR;

#pragma pack()
//
// Define serial register I/O macros
//
#define READ_RBR(S)  SerialReadRegister (S, SERIAL_REGISTER_RBR)
#define READ_DLL(S)  SerialReadRegister (S, SERIAL_REGISTER_DLL)
#define READ_DLM(S)  SerialReadRegister (S, SERIAL_REGISTER_DLM)
#define READ_IER(S)  SerialReadRegister (S, SERIAL_REGISTER_IER)
#define READ_IIR(S)  SerialReadRegister (S, SERIAL_REGISTER_IIR)
#define READ_LCR(S)  SerialReadRegister (S, SERIAL_REGISTER_LCR)
#define READ_MCR(S)  SerialReadRegister (S, SERIAL_REGISTER_MCR)
#define READ_LSR(S)  SerialReadRegister (S, SERIAL_REGISTER_LSR)
#define READ_MSR(S)  SerialReadRegister (S, SERIAL_REGISTER_MSR)
#define READ_SCR(S)  SerialReadRegister (S, SERIAL_REGISTER_SCR)

#define WRITE_THR(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_THR, D)
#define WRITE_DLL(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_DLL, D)
#define WRITE_DLM(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_DLM, D)
#define WRITE_IER(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_IER, D)
#define WRITE_FCR(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_FCR, D)
#define WRITE_LCR(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_LCR, D)
#define WRITE_MCR(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_MCR, D)
#define WRITE_LSR(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_LSR, D)
#define WRITE_MSR(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_MSR, D)
#define WRITE_SCR(S, D)  SerialWriteRegister (S, SERIAL_REGISTER_SCR, D)

//
// Prototypes
// Driver model protocol interface
//

/**
  Check to see if this driver supports the given controller

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller           The handle of the controller to test.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path.

  @return EFI_SUCCESS          This driver can support the given controller

**/
EFI_STATUS
EFIAPI
SerialControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Start to management the controller passed in

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller           The handle of the controller to test.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path.

  @return EFI_SUCCESS          Driver is started successfully
**/
EFI_STATUS
EFIAPI
SerialControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Disconnect this driver with the controller, uninstall related protocol instance

  @param  This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller            The handle of the controller to test.
  @param  NumberOfChildren      Number of child device.
  @param  ChildHandleBuffer     A pointer to the remaining portion of a device path.

  @retval EFI_SUCCESS           Operation successfully
  @retval EFI_DEVICE_ERROR      Cannot stop the driver successfully

**/
EFI_STATUS
EFIAPI
SerialControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// Serial I/O Protocol Interface
//

/**
  Reset serial device.

  @param This               Pointer to EFI_SERIAL_IO_PROTOCOL

  @retval EFI_SUCCESS        Reset successfully
  @retval EFI_DEVICE_ERROR   Failed to reset

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  );

/**
  Set new attributes to a serial device.

  @param This                     Pointer to EFI_SERIAL_IO_PROTOCOL
  @param  BaudRate                 The baudrate of the serial device
  @param  ReceiveFifoDepth         The depth of receive FIFO buffer
  @param  Timeout                  The request timeout for a single char
  @param  Parity                   The type of parity used in serial device
  @param  DataBits                 Number of databits used in serial device
  @param  StopBits                 Number of stopbits used in serial device

  @retval  EFI_SUCCESS              The new attributes were set
  @retval  EFI_INVALID_PARAMETERS   One or more attributes have an unsupported value
  @retval  EFI_UNSUPPORTED          Data Bits can not set to 5 or 6
  @retval  EFI_DEVICE_ERROR         The serial device is not functioning correctly (no return)

**/
EFI_STATUS
EFIAPI
SerialSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT64                  BaudRate,
  IN UINT32                  ReceiveFifoDepth,
  IN UINT32                  Timeout,
  IN EFI_PARITY_TYPE         Parity,
  IN UINT8                   DataBits,
  IN EFI_STOP_BITS_TYPE      StopBits
  );

/**
  Set Control Bits.

  @param This              Pointer to EFI_SERIAL_IO_PROTOCOL
  @param Control           Control bits that can be settable

  @retval EFI_SUCCESS       New Control bits were set successfully
  @retval EFI_UNSUPPORTED   The Control bits wanted to set are not supported

**/
EFI_STATUS
EFIAPI
SerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  );

/**
  Get ControlBits.

  @param This          Pointer to EFI_SERIAL_IO_PROTOCOL
  @param Control       Control signals of the serial device

  @retval EFI_SUCCESS   Get Control signals successfully

**/
EFI_STATUS
EFIAPI
SerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  );

/**
  Write the specified number of bytes to serial device.

  @param This                Pointer to EFI_SERIAL_IO_PROTOCOL
  @param  BufferSize         On input the size of Buffer, on output the amount of
                             data actually written
  @param  Buffer             The buffer of data to write

  @retval EFI_SUCCESS        The data were written successfully
  @retval EFI_DEVICE_ERROR   The device reported an error
  @retval EFI_TIMEOUT        The write operation was stopped due to timeout

**/
EFI_STATUS
EFIAPI
SerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  );

/**
  Read the specified number of bytes from serial device.

  @param This               Pointer to EFI_SERIAL_IO_PROTOCOL
  @param BufferSize         On input the size of Buffer, on output the amount of
                            data returned in buffer
  @param Buffer             The buffer to return the data into

  @retval EFI_SUCCESS        The data were read successfully
  @retval EFI_DEVICE_ERROR   The device reported an error
  @retval EFI_TIMEOUT        The read operation was stopped due to timeout

**/
EFI_STATUS
EFIAPI
SerialRead (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  );

//
// Internal Functions
//

/**
  Use scratchpad register to test if this serial port is present.

  @param SerialDevice   Pointer to serial device structure

  @return if this serial port is present
**/
BOOLEAN
SerialPresent (
  IN SERIAL_DEV  *SerialDevice
  );

/**
  Detect whether specific FIFO is full or not.

  @param Fifo    A pointer to the Data Structure SERIAL_DEV_FIFO

  @return whether specific FIFO is full or not

**/
BOOLEAN
SerialFifoFull (
  IN SERIAL_DEV_FIFO  *Fifo
  );

/**
  Detect whether specific FIFO is empty or not.

  @param  Fifo    A pointer to the Data Structure SERIAL_DEV_FIFO

  @return whether specific FIFO is empty or not

**/
BOOLEAN
SerialFifoEmpty (
  IN SERIAL_DEV_FIFO  *Fifo
  );

/**
  Add data to specific FIFO.

  @param Fifo                  A pointer to the Data Structure SERIAL_DEV_FIFO
  @param Data                  the data added to FIFO

  @retval EFI_SUCCESS           Add data to specific FIFO successfully
  @retval EFI_OUT_OF_RESOURCE   Failed to add data because FIFO is already full

**/
EFI_STATUS
SerialFifoAdd (
  IN SERIAL_DEV_FIFO  *Fifo,
  IN UINT8            Data
  );

/**
  Remove data from specific FIFO.

  @param Fifo                  A pointer to the Data Structure SERIAL_DEV_FIFO
  @param Data                  the data removed from FIFO

  @retval EFI_SUCCESS           Remove data from specific FIFO successfully
  @retval EFI_OUT_OF_RESOURCE   Failed to remove data because FIFO is empty

**/
EFI_STATUS
SerialFifoRemove (
  IN  SERIAL_DEV_FIFO  *Fifo,
  OUT UINT8            *Data
  );

/**
  Reads and writes all available data.

  @param SerialDevice           The device to flush

  @retval EFI_SUCCESS           Data was read/written successfully.
  @retval EFI_OUT_OF_RESOURCE   Failed because software receive FIFO is full.  Note, when
                                this happens, pending writes are not done.

**/
EFI_STATUS
SerialReceiveTransmit (
  IN SERIAL_DEV  *SerialDevice
  );

/**
  Read serial port.

  @param SerialDev     Pointer to serial device
  @param Offset        Offset in register group

  @return Data read from serial port
**/
UINT8
SerialReadRegister (
  IN SERIAL_DEV  *SerialDev,
  IN UINT32      Offset
  );

/**
  Write serial port.

  @param  SerialDev     Pointer to serial device
  @param  Offset        Offset in register group
  @param  Data          data which is to be written to some serial port register
**/
VOID
SerialWriteRegister (
  IN SERIAL_DEV  *SerialDev,
  IN UINT32      Offset,
  IN UINT8       Data
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

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
SerialComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
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

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
SerialComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

/**
  Add the component name for the serial io device

  @param SerialDevice     A pointer to the SERIAL_DEV instance.
  @param Uid              Unique ID for the serial device.
**/
VOID
AddName (
  IN  SERIAL_DEV  *SerialDevice,
  IN  UINT32      Uid
  );

/**
  Checks whether the UART parameters are valid and computes the Divisor.

  @param  ClockRate      The clock rate of the serial device used to verify
                         the BaudRate. Do not verify the BaudRate if it's 0.
  @param  BaudRate       The requested baudrate of the serial device.
  @param  DataBits       Number of databits used in serial device.
  @param  Parity         The type of parity used in serial device.
  @param  StopBits       Number of stopbits used in serial device.
  @param  Divisor        Return the divisor if ClockRate is not 0.
  @param  ActualBaudRate Return the actual supported baudrate without
                         exceeding BaudRate. NULL means baudrate degradation
                         is not allowed.
                         If the requested BaudRate is not supported, the routine
                         returns TRUE and the Actual Baud Rate when ActualBaudRate
                         is not NULL, returns FALSE when ActualBaudRate is NULL.

  @retval TRUE   The UART parameters are valid.
  @retval FALSE  The UART parameters are not valid.
**/
BOOLEAN
VerifyUartParameters (
  IN     UINT32              ClockRate,
  IN     UINT64              BaudRate,
  IN     UINT8               DataBits,
  IN     EFI_PARITY_TYPE     Parity,
  IN     EFI_STOP_BITS_TYPE  StopBits,
  OUT UINT64                 *Divisor,
  OUT UINT64                 *ActualBaudRate
  );

/**
  Skip the optional Controller device path node and return the
  pointer to the next device path node.

  @param DevicePath             Pointer to the device path.
  @param ContainsControllerNode Returns TRUE if the Controller device path exists.
  @param ControllerNumber       Returns the Controller Number if Controller device path exists.

  @return     Pointer to the next device path node.
**/
UART_DEVICE_PATH *
SkipControllerDevicePathNode (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  BOOLEAN                   *ContainsControllerNode,
  UINT32                    *ControllerNumber
  );

/**
  Check the device path node whether it's the Flow Control node or not.

  @param[in] FlowControl    The device path node to be checked.

  @retval TRUE              It's the Flow Control node.
  @retval FALSE             It's not.

**/
BOOLEAN
IsUartFlowControlDevicePathNode (
  IN UART_FLOW_CONTROL_DEVICE_PATH  *FlowControl
  );

#endif
