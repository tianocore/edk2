/** @file
Header file for the defintions used in SMBus DXE driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _DXE_QNC_SMBUS_H_
#define _DXE_QNC_SMBUS_H_
#include "CommonHeader.h"

#include "QNCSmbus.h"

#define MAX_SMBUS_DEVICES   107     // Max number of SMBus devices (7 bit
                                    //   address yields 128 combinations but 21
                                    //   of those are reserved)

#define MICROSECOND     10
#define MILLISECOND     (1000 * MICROSECOND)
#define ONESECOND       (1000 * MILLISECOND)

#define STALL_TIME          1000000 // 1,000,000 microseconds = 1 second
#define BUS_TRIES           3       // How many times to retry on Bus Errors
#define SMBUS_NUM_RESERVED  21      // Number of device addresses that are
                                    //   reserved by the SMBus spec.
#define SMBUS_ADDRESS_ARP   0xC2 >> 1
#define   SMBUS_DATA_PREPARE_TO_ARP   0x01
#define   SMBUS_DATA_RESET_DEVICE     0x02
#define   SMBUS_DATA_GET_UDID_GENERAL 0x03
#define   SMBUS_DATA_ASSIGN_ADDRESS   0x04
#define SMBUS_GET_UDID_LENGTH 17    // 16 byte UDID + 1 byte address

/**
  Executes an SMBus operation to an SMBus controller. Returns when either the command has been
  executed or an error is encountered in doing the operation.

  The Execute() function provides a standard way to execute an operation as defined in the System
  Management Bus (SMBus) Specification. The resulting transaction will be either that the SMBus
  slave devices accept this transaction or that this function returns with error.

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  SlaveAddress            The SMBus slave address of the device with which to communicate.
  @param  Command                 This command is transmitted by the SMBus host controller to the
                                  SMBus slave device and the interpretation is SMBus slave device
                                  specific. It can mean the offset to a list of functions inside an
                                  SMBus slave device. Not all operations or slave devices support
                                  this command's registers.
  @param  Operation               Signifies which particular SMBus hardware protocol instance that
                                  it will use to execute the SMBus transactions. This SMBus
                                  hardware protocol is defined by the SMBus Specification and is
                                  not related to EFI.
  @param  PecCheck                Defines if Packet Error Code (PEC) checking is required for this
                                  operation.
  @param  Length                  Signifies the number of bytes that this operation will do. The
                                  maximum number of bytes can be revision specific and operation
                                  specific. This field will contain the actual number of bytes that
                                  are executed for this operation. Not all operations require this
                                  argument.
  @param  Buffer                  Contains the value of data to execute to the SMBus slave device.
                                  Not all operations require this argument. The length of this
                                  buffer is identified by Length.

  @retval EFI_SUCCESS             The last data that was returned from the access matched the poll
                                  exit criteria.
  @retval EFI_CRC_ERROR           Checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT             Timeout expired before the operation was completed. Timeout is
                                  determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR        The request was not completed because a failure that was
                                  reflected in the Host Status Register bit. Device errors are a
                                  result of a transaction collision, illegal command field,
                                  unclaimed cycle (host initiated), or bus errors (collisions).
  @retval EFI_INVALID_PARAMETER   Operation is not defined in EFI_SMBUS_OPERATION.
  @retval EFI_INVALID_PARAMETER   Length/Buffer is NULL for operations except for EfiSmbusQuickRead
                                  and EfiSmbusQuickWrite. Length is outside the range of valid
                                  values.
  @retval EFI_UNSUPPORTED         The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL    Buffer is not sufficient for this operation.

**/
EFI_STATUS
EFIAPI
SmbusExecute (
  IN CONST  EFI_SMBUS_HC_PROTOCOL     *This,
  IN CONST  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN CONST  EFI_SMBUS_DEVICE_COMMAND  Command,
  IN CONST  EFI_SMBUS_OPERATION       Operation,
  IN CONST  BOOLEAN                   PecCheck,
  IN OUT    UINTN                     *Length,
  IN OUT    VOID                      *Buffer
  );

/**
  Sets the SMBus slave device addresses for the device with a given unique ID or enumerates the
  entire bus.

  The ArpDevice() function provides a standard way for a device driver to enumerate the entire
  SMBus or specific devices on the bus.

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  ArpAll                  A Boolean expression that indicates if the host drivers need to
                                  enumerate all the devices or enumerate only the device that is
                                  identified by SmbusUdid. If ArpAll is TRUE, SmbusUdid and
                                  SlaveAddress are optional. If ArpAll is FALSE, ArpDevice will
                                  enumerate SmbusUdid and the address will be at SlaveAddress.
  @param  SmbusUdid               The Unique Device Identifier (UDID) that is associated with this
                                  device.
  @param  SlaveAddress            The SMBus slave address that is associated with an SMBus UDID.

  @retval EFI_SUCCESS             The last data that was returned from the access matched the poll
                                  exit criteria.
  @retval EFI_CRC_ERROR           Checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT             Timeout expired before the operation was completed. Timeout is
                                  determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR        The request was not completed because a failure that was
                                  reflected in the Host Status Register bit. Device errors are a
                                  result of a transaction collision, illegal command field,
                                  unclaimed cycle (host initiated), or bus errors (collisions).
  @retval EFI_UNSUPPORTED         The corresponding operation is not supported.

**/
EFI_STATUS
EFIAPI
SmbusArpDevice (
  IN CONST  EFI_SMBUS_HC_PROTOCOL     *This,
  IN        BOOLEAN                   ArpAll,
  IN        EFI_SMBUS_UDID            *SmbusUdid,   OPTIONAL
  IN OUT    EFI_SMBUS_DEVICE_ADDRESS  *SlaveAddress OPTIONAL
  );

/**
  Returns a pointer to the Address Resolution Protocol (ARP) map that contains the ID/address pair
  of the slave devices that were enumerated by the SMBus host controller driver.

  The GetArpMap() function returns the mapping of all the SMBus devices that were enumerated by the
  SMBus host driver.

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  Length                  Size of the buffer that contains the SMBus device map.
  @param  SmbusDeviceMap          The pointer to the device map as enumerated by the SMBus
                                  controller driver.

  @retval EFI_SUCCESS             The SMBus returned the current device map.
  @retval EFI_UNSUPPORTED         The corresponding operation is not supported.

**/
EFI_STATUS
EFIAPI
SmbusGetArpMap (
  IN CONST  EFI_SMBUS_HC_PROTOCOL   *This,
  IN OUT    UINTN                   *Length,
  IN OUT    EFI_SMBUS_DEVICE_MAP    **SmbusDeviceMap
  );

/**
  Allows a device driver to register for a callback when the bus driver detects a state that it
  needs to propagate to other drivers that are registered for a callback.

  The Notify() function registers all the callback functions to allow the bus driver to call these
  functions when the SlaveAddress/Data pair happens.
  If NotifyFunction is NULL, then ASSERT ().

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  SlaveAddress            The SMBUS hardware address to which the SMBUS device is
                                  preassigned or allocated.
  @param  Data                    Data of the SMBus host notify command that the caller wants to be
                                  called.
  @param  NotifyFunction          The function to call when the bus driver detects the SlaveAddress
                                  and Data pair.

  @retval EFI_SUCCESS             NotifyFunction was registered.
  @retval EFI_UNSUPPORTED         The corresponding operation is not supported.

**/
EFI_STATUS
EFIAPI
SmbusNotify (
  IN CONST  EFI_SMBUS_HC_PROTOCOL     *This,
  IN CONST  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN CONST  UINTN                     Data,
  IN CONST  EFI_SMBUS_NOTIFY_FUNCTION NotifyFunction
  );

/**
  Entry point to the DXE Driver that produces the SMBus Host Controller Protocol.

  @param  ImageHandle      ImageHandle of the loaded driver.
  @param  SystemTable      Pointer to the EFI System Table.

  @retval EFI_SUCCESS      The entry point of SMBus DXE driver is executed successfully.
  @retval !EFI_SUCESS      Some error occurs in the entry point of SMBus DXE driver.

**/
EFI_STATUS
EFIAPI
InitializeQNCSmbus (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  );

#endif
