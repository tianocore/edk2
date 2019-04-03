/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


  \section I2cDriverStack       I2C Driver Stack

  The following is a representation of the I<sup>2</sup>C (I2C)
  driver stack and an I2C bus layout.

  <code><pre>
              +-----------------+
              |   Application   |
              +-----------------+
                       |
                       | Third Party or UEFI
                       |
                       V
 +--------+   +-----------------+
 | Slave  |   |   Third Party   |
 | Driver |   |   I2C Device    |
 |        |   |     Driver      |
 +--------+   +-----------------+
      |                |
      |          BUS   |
      |                |
      |                V
      |       +-----------------+
      |       | I2C Bus Driver  |------------------.
      |       +-----------------+                  |
      |                |                           |
      |         HOST   |          BUS              |
      |                |          CONFIGURATION    |
SLAVE |                V          MANAGEMENT       | ACPI
      |       +-----------------+                  |
      |       | I2C Host Driver |----------.       |
      |       +-----------------+          |       |
      |                |                   |       |
      |        MASTER  |                   V       V
      |                |               +-------=-------------+
      |                V               | I2C Platform Driver |
      |       +-----------------+      +---------------------+
      `------>| I2C Port Driver |               |      |
              +-----------------+               |      |
                       |                        |      |
            Software   |                        |      |
            --------------------------------------------------
            Hardware   |                        |      |
                       |                        |      |
                       V                        |      |
              +-----------------+               |      |
              | I2C Controller  |               |      |
              +-----------------+               |      |
                       |                        |      |
            -----------------------             |      |
            I2C Bus    |                        |      |
                       |    +------------+      |      |
                       +----| High speed |      |      |
                       |    | I2C device |      |      |
                       |    |    0x01    |      |      |
                       |    +------------+      |      |
                       |                        |      |
                  +---------+  0                |      |
                  | Switch  |<------------------`      |
                  +---------+  1                       |
                       |                               |
                       |    +------------+             |
                       +----| Fast speed |             |
                       |    | I2C device |             |
                       |    |    0x02    |             |
                       |    +------------+             |
                       |                               |
                +-------------+                        |
                | Multiplexer |<-----------------------`
                +-------------+
                 0 |       | 1
                   |       |
                   |       |
                   |       |    +-------------+
                   |       +----| Third Party |
                   |       |    | I2C Device  |
                   |       |    |  0x03, 0x04 |
                   |       |    +-------------+
                   |       |
                   |
                   |            +-------------+
                   +------------| Third Party |
                   |            | I2C Device  |
                   |            |  0x03, 0x04 |
                   |            +-------------+
                   |
  </pre></code>

  The platform hardware designer chooses the bus layout based upon
  the platform, I2C chip and software requirements.  The design uses
  switches to truncate the bus to enable higher speed operation for a
  subset of devices which are placed closer to the controller.  When the
  switch is on, the extended bus must operate at a lower speed.  The
  design uses multiplexer to create separate address spaces enabling
  the use of multiple devices which would otherwise have conflicting
  addresses. See the
  <a href="http://www.nxp.com/documents/user_manual/UM10204.pdf">I<sup>2</sup>C
  Specification</a> for more details.

  N.B. Some operating systems may prohibit the changing of switches
  and multiplexers in the I2C bus.  In this case the platform hardware
  and software designers must select a single I2C bus configuration
  consisting of constant input values for the switches and multiplexers.
  The platform software designer must then ensure that this I2C bus
  configuration is enabled prior to passing control to the operating
  system.

  The platform hardware designer needs to provide the platform software
  designer the following data for each I2C bus:

  1.  Which controller controls this bus

  2.  A list of logic blocks contained in one or more I2C devices:

      a.  I2C device which contains this logic block

      b.  Logic block slave address

      c.  Logic block name

  3.  For each configuration of the switches and multiplexer

      a.  What is the maximum speed of operation

      b.  What devices are accessible

  4.  The settings for the switches and multiplexers when control is
      given to the operating system.

  \section ThirdPartyI2cDrivers   Third Party I2C Drivers

  This layer is I2C chip specific but platform and host controller
  independent.

  Third party I2C driver writers, typically silicon vendors, need
  to provide:

  1.  The device path node data that is used to select their
      driver.

  2.  The order for the blocks of logic that get referenced
      by the entries in the slave address array.

  3.  The hardware version of the I2C device, this value is passed
      to the third party I2C driver to enable it to perform work
      arounds for the specific hardware version.  This value should
      match the value in the ACPI _HRV tag.

  The third party I2C driver uses relative addressing to abstract
  the platform specific details of the I2C device.  Using an
  example I2C device containing an accelerometer and a magnetometer
  which consumes two slave addresses, one for each logic block.  The
  third party I2C driver writer may choose to write two drivers, one
  for each block of logic, in which case each driver refers to the
  single I2C slave address using the relative value of zero (0).
  However if the third party I2C driver writer chooses to write a
  single driver which consumes multiple slave addresses then the
  third party I2C driver writer needs to convey the order of the
  I2C slave address entries in the slave address array to the
  platform software designer.  For the example:

      0: Accelerometer

      1: Magnetometer

  The platform hardware designer picks the actual slave addresses
  from the I2C device's data sheet and provides this information
  to the platform software designer.  The platform software designer
  then places the slave addresses into the slave address array in the
  order specified by the third party I2C driver writer.  The third
  party driver I2C writer then indirectly references this array by
  specifying the index value as the relative slave address.  The
  relative value always starts at zero (0) and its maximum value is
  the number of entries in slave address array minus one.

  The slave address is specified as a 32-bit integer to allow room
  for future slave address expansion.  Only the port driver knows
  the maximum slave address value.  All other drivers and
  applications must look for the EFI_NOT_FOUND status for the
  indication that the maximum slave address was exceeded.

  \section I2cBusDriver         I2C Bus Driver

  This layer is platform, host controller, and I2C chip independent.

  The I2C bus driver creates a handle for each of the I2C devices
  described within the platform driver.  The I2C controller's device
  path is extended with the device path node provided by the platform
  driver and attached to the handle.  The third party I2C device driver
  uses the device path to determine if it may connect.  For ACPI nodes,
  the third party I2C driver should use the CID or CidString value.

  The I2C bus driver validates the relative address for the I2C device
  and then converts the relative value to an actual I2C slave address.
  The request is then passed to the I2C host driver.

  \section I2cHostDriver        I2C Host Driver

  This layer is platform, host controller, and I2C chip independent.

  N.B. For proper operation of the I2C bus, only the I2C bus driver
  and the I2C test application should connect to the I2C host driver
  via the EFI_I2C_HOST_DRIVER_PROTOCOL.

  The I2C host driver may access any device on the I2C bus.  The I2C
  host driver has the following responsibilities:

  1.  Limits the number of requests to the I2C port driver to one.
      The I2C host driver holds on to additional requests until the
      I2C port driver is available to process the request.  The I2C
      requests are issued in FIFO order to the I2C port driver.

  2.  Enable the proper I2C bus configuration before starting the
      I2C request on the I2C port driver

  I2C devices are addressed as the tuple: BusConfiguration:SlaveAddress.
  I2C bus configuration zero (0) is the portion of the I2C bus that
  connects to the host controller.  The bus configuration specifies
  the control values for the switches and multiplexers in the I2C bus.
  After the switches and multiplexers are properly configured, the I2C
  controller uses the slave address to access the requested I2C device.

  Since the I2C driver stack supports asynchronous operations this
  layer maintains a queue of I2C requests until the I2C controller
  is available them.  When a request reaches the head of the queue
  the necessary bus configuration is enabled and then the request
  is sent to the I2C port driver.

  \section I2cPortDriver        I2C Port Driver

  This layer is I2C controller specific but platform independent.

  This layer manipulates the I2C controller to perform an operation
  on the I2C bus.  This layer does not configure the I2C bus so it
  is up to the caller to ensure that the I2C bus is in the proper
  configuration before issuing the I2C request.

  This layer typically needs the following information:

  1.  Host controller address
  2.  Controller's input clock frequency

  Depending upon the I2C controller, more data may be necessary.
  This layer may use any method to get these values: hard coded
  values, PCD values, or may choose to communicate with the platform
  layer using an undefined mechanism to get these values.

  If the I2C port driver requires data from the platform driver then
  the I2C port driver writer needs to provide the platform interface
  details to the platform software designer.

  \section I2cPlatformDriver    I2C Platform Driver

  When enabling access to I2C devices within UEFI, this driver
  installs the EFI_I2C_ACPI_PROTOCOL to provide the I2C device
  descriptions to the I2C bus driver using the EFI_I2C_DEVICE
  structure.  These descriptions include the bus configuration
  number required for the I2C device, the slave address array
  and the device path.

  The EFI_I2C_BUS_CONFIGURATION_MANAGEMENT protocol is optional.
  This protocol needs to be specified under the following conditions:

  1.  The I2C bus must operate at a frequency greater than 100 KHz
  2.  The I2C bus contains switches or multiplexers.

  The EFI_I2C_BUS_CONFIGURATION_MANAGEMENT protocol enables the
  I2C host driver to call into the I2C platform driver to enable
  a specific I2C bus configuration and set its maximum clock speed.

  The platform software designer collects the data requirements
  from third party I2C driver writers, the I2C controller
  driver writer, the EFI_I2C_ACPI_PROTOCOL and
  EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL.  The platform
  software designer gets the necessary data from the platform
  hardware designer.  The platform software designer then builds
  the data structures and implements the necessary routines to
  construct the I2C platform driver.

  \section I2cSwitches          Switches and Multiplexers

  There are some I2C switches and I2C multiplexers where the control
  is done via I2C commands.  When the control inputs come via the
  same I2C bus that is being configured then the platform driver must
  use the EFI_I2C_MASTER_PROTOCOL that is passed to the platform
  driver.  While the I2C host driver makes the call to the I2C
  platform driver to configure the bus, the host driver keeps the
  I2C port driver idle, to allow the I2C platform driver preform
  the necessary configuration operations.

  If however the configuration control is done via and I2C device
  connected to a different I2C bus (host controller), then it is
  possible for the platform software designer may choose between
  the following:

  1.  Call into a third party I2C driver to manipulate the I2C
      bus control device.
  2.  Call into the EFI_I2C_BUS_PROTOCOL if no third party I2C
      driver exists for the I2C bus control device
  3.  Call into the EFI_I2C_HOST_PROTOCOL if the platform does
      not expose the I2C bus control device.

**/

#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

/**
  Declare the forward references

**/
typedef struct _EFI_I2C_MASTER_PROTOCOL   EFI_I2C_MASTER_PROTOCOL;  ///<  I2C master protocol

///
/// I2C device operation
///
/// This structure provides the information necessary for an operation
/// on an I2C device
///
typedef struct {
  ///
  /// Number of bytes to send to the I2C device
  ///
  UINT32 WriteBytes;

  ///
  /// Number of bytes to read, set to zero for write only operations
  ///
  UINT32 ReadBytes;

  ///
  /// Address of the buffer containing the data to send to the I2C device.
  /// The WriteBuffer must be at least WriteBytes in length.
  ///
  UINT8 *WriteBuffer;

  ///
  /// Address of the buffer to receive data from the I2C device. Use NULL
  /// for write only operations.  The ReadBuffer must be at least ReadBytes
  /// in length.
  ///
  UINT8 *ReadBuffer;

  ///
  /// Timeout for the I2C operation in 100 ns units
  ///
  UINT32 Timeout;
} EFI_I2C_REQUEST_PACKET;


/**
  Set the I2C controller bus clock frequency.

  This routine must be called at or below TPL_NOTIFY.

  The software and controller do a best case effort of using the specified
  frequency for the I2C bus.  If the frequency does not match exactly then
  the controller will use a slightly lower frequency to avoid
  exceeding the operating conditions for any of the I2C devices on the bus.
  For example if 400 KHz was specified and the controller's divide network
  only supports 402 KHz or 398 KHz then the controller would be set to 398
  KHz.  However if the desired frequency is 400 KHz and the controller only
  supports 1 MHz and 100 KHz then this routine would return EFI_UNSUPPORTED.

  @param[in] This           Address of an EFI_I2C_MASTER_PROTOCOL
                            structure
  @param[in] BusClockHertz  New I2C bus clock frequency in Hertz

  @retval EFI_SUCCESS       The bus frequency was set successfully.
  @retval EFI_UNSUPPORTED   The controller does not support this frequency.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_MASTER_BUS_FREQUENCY_SET) (
  IN CONST EFI_I2C_MASTER_PROTOCOL *This,
  IN UINTN BusClockHertz
  );

/**
  Reset the I2C controller and configure it for use

  This routine must be called at or below TPL_NOTIFY.

  The I2C controller is reset and the I2C bus frequency is set to 100 KHz.

  @param[in]     This       Address of an EFI_I2C_MASTER_PROTOCOL
                            structure

**/
typedef
VOID
(EFIAPI *EFI_I2C_MASTER_RESET) (
  IN CONST EFI_I2C_MASTER_PROTOCOL *This
  );

/**
  Start an I2C operation on the host controller

  This routine must be called at or below TPL_NOTIFY.  For synchronous
  requests this routine must be called at or below TPL_CALLBACK.

  This function initiates an I2C operation on the controller.

  The operation is performed by selecting the I2C device with its slave
  address and then sending all write data to the I2C device.  If read data
  is requested, a restart is sent followed by the slave address and then
  the read data is clocked into the I2C controller and placed in the read
  buffer.  When the operation completes, the status value is returned and
  then the event is set.

  N.B. The typical consumer of this API is the I2C host driver.
  Extreme care must be taken by other consumers of this API to
  prevent confusing the third party I2C drivers due to a state
  change at the I2C device which the third party I2C drivers did
  not initiate.  I2C platform drivers may use this API within
  these guidelines.

  N.B. This API supports only one operation, no queuing support
  exists at this layer.  This API assumes that the I2C bus is in
  the correct configuration for the I2C request.

  @param[in] This           Address of an EFI_I2C_MASTER_PROTOCOL
                            structure
  @param[in] SlaveAddress   Address of the device on the I2C bus.
  @param[in] Event          Event to set for asynchronous operations,
                            NULL for synchronous operations
  @param[in] RequestPacket  Address of an EFI_I2C_REQUEST_PACKET
                            structure describing the I2C operation
  @param[out] I2cStatus     Optional buffer to receive the I2C operation
                            completion status

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_ABORTED           The request did not complete because the driver
                                was shutdown.
  @retval EFI_BAD_BUFFER_SIZE   The WriteBytes or ReadBytes buffer size is too large.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the operation.
                                This could indicate the slave device is not present.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL
  @retval EFI_INVALID_PARAMETER TPL is too high
  @retval EFI_NOT_FOUND         SlaveAddress exceeds maximum address
  @retval EFI_NOT_READY         I2C bus is busy or operation pending, wait for
                                the event and then read status pointed to by
                                the request packet.
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the
                                slave address.  EFI_DEVICE_ERROR may also be
                                returned if the controller cannot distinguish
                                when the NACK occurred.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for I2C operation
  @retval EFI_TIMEOUT           The transaction did not complete within an internally
                                specified timeout period.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_MASTER_START_REQUEST) (
  IN CONST EFI_I2C_MASTER_PROTOCOL *This,
  IN UINTN SlaveAddress,
  IN EFI_EVENT Event OPTIONAL,
  IN CONST EFI_I2C_REQUEST_PACKET *RequestPacket,
  OUT EFI_STATUS *I2cStatus OPTIONAL
  );

///
/// I2C master mode protocol
///
/// This protocol manipulates the I2C host controller to perform transactions as a
/// master on the I2C bus using the current state of any switches or multiplexers
/// in the I2C bus.
///
struct _EFI_I2C_MASTER_PROTOCOL {
  ///
  /// Set the clock frequency for the I2C bus
  ///
  EFI_I2C_MASTER_BUS_FREQUENCY_SET BusFrequencySet;

  ///
  /// Reset the I2C host controller
  ///
  EFI_I2C_MASTER_RESET Reset;

  ///
  /// Start an I2C transaction in master mode on the host controller
  ///
  EFI_I2C_MASTER_START_REQUEST StartRequest;

  ///
  /// The maximum number of bytes the I2C host controller
  /// is able to receive from the I2C bus.
  ///
  UINT32 MaximumReceiveBytes;

  ///
  /// The maximum number of bytes the I2C host controller
  /// is able to send on the I2C bus.
  ///
  UINT32 MaximumTransmitBytes;

  ///
  /// The maximum number of bytes in the I2C bus transaction.
  ///
  UINT32 MaximumTotalBytes;
};

///
/// GUID for the EFI_I2C_MASTER_PROTOCOL
///
extern EFI_GUID gEfiI2cMasterProtocolGuid;

#endif  //  __I2C_MASTER_H__
