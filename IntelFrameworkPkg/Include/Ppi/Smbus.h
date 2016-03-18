/** @file
  This file declares the Smbus PPI, which provides the basic I/O interfaces that a PEIM 
  uses to access its SMBus controller and the slave devices attached to it.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This PPI is defined in Framework of EFI SmBus PPI spec.
  Version 0.9.

**/

#ifndef _PEI_SMBUS_PPI_H_
#define _PEI_SMBUS_PPI_H_

#include <Ppi/Smbus2.h>

#define EFI_PEI_SMBUS_PPI_GUID \
  { \
    0xabd42895, 0x78cf, 0x4872, {0x84, 0x44, 0x1b, 0x5c, 0x18, 0xb, 0xfb, 0xda } \
  }

typedef struct _EFI_PEI_SMBUS_PPI EFI_PEI_SMBUS_PPI;

/**
  Executes an SMBus operation to an SMBus controller.

  @param[in]      PeiServices   A pointer to the system PEI Services Table.
  @param[in]      This          A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param[in]      SlaveAddress  The SMBUS hardware address to which the SMBUS
                                device is preassigned or allocated.
  @param[in]      Command       This command is transmitted by the SMBus host
                                controller to the SMBus slave device, and the 
                                interpretation is SMBus slave device specific.
  @param[in]      Operation     Signifies which particular SMBus hardware protocol
                                instance to use to execute the SMBus transactions.
  @param[in]      PecCheck      Defines if Packet Error Code (PEC) checking is 
                                required for this operation.
  @param[in, out] Length        The number of bytes for this operation.
  @param[in, out] Buffer        Contains the value of data to execute to the SMBus 
                                slave device.

  @retval EFI_SUCCESS           The last data that was returned from the access
                                matched the poll exit criteria.
  @retval EFI_CRC_ERROR         The checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT           Timeout expired before the operation was completed.
                                Timeout is determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed
                                due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The request was not completed because a failure
                                was recorded in the Host Status Register bit.
  @retval EFI_INVALID_PARAMETER The operation is not defined in EFI_SMBUS_OPERATION.
  @retval EFI_INVALID_PARAMETER Length/Buffer is NULL for operations except for 
                                EfiSmbusQuickRead and EfiSmbusQuickWrite. Length 
                                is outside the range of valid values.
  @retval EFI_UNSUPPORTED       The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL  Buffer is not sufficient for this operation.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_EXECUTE_OPERATION)(
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_SMBUS_PPI         *This,
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN      EFI_SMBUS_DEVICE_COMMAND  Command,
  IN      EFI_SMBUS_OPERATION       Operation,
  IN      BOOLEAN                   PecCheck,
  IN OUT  UINTN                     *Length,
  IN OUT  VOID                      *Buffer
  );

/**
  This function is user-defined, and is called when the SlaveAddress/Data pair happens.

  @param[in]  PeiServices    A pointer to the system PEI Services Table.
  @param[in]  This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param[in]  SlaveAddress   The SMBUS hardware address to which the SMBUS
                             device is preassigned or allocated.
  @param[in]  Data           Data of the SMBus host notify command, which denotes that
                             the caller wants to be called.

  @return Status Code returned by callback function.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_NOTIFY_FUNCTION)(
  IN      EFI_PEI_SERVICES              **PeiServices,
  IN      EFI_PEI_SMBUS_PPI             *SmbusPpi,
  IN      EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress,
  IN      UINTN                         Data
  );

/**
  The ArpDevice() function enumerates either the entire bus or a specific
  device identified by SmbusUdid.

  @param[in]      PeiServices   A pointer to the system PEI Services Table.
  @param[in]      This          A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param[in]      ArpAll        A Boolean expression that indicates if the host 
                                drivers need to enumerate all the devices or to 
                                enumerate only the device that is identified
                                by SmbusUdid. If ArpAll is TRUE, SmbusUdid and 
                                SlaveAddress are optional and ignored if entered.
                                If ArpAll is FALSE, ArpDevice will enumerate 
                                SmbusUdid, and the address will be at SlaveAddress.
  @param[in]      SmbusUdid     The targeted SMBus Unique Device Identifier (UDID).
                                The UDID may not exist for SMBus devices with fixed 
                                addresses.
  @param[in, out] SlaveAddress  The new SMBus address for the slave device for
                                which the operation is targeted.
                                This address may be NULL.

  @retval EFI_SUCCESS           The SMBus slave device address was set.
  @retval EFI_INVALID_PARAMETER SlaveAddress is NULL.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed
                                due to a lack of resources.
  @retval EFI_TIMEOUT           The SMBus slave device did not respond.
  @retval EFI_DEVICE_ERROR      The request was not completed because the transaction failed.
  @retval EFI_UNSUPPORTED       ArpDevice() is not implemented by this PEIM. 
                                This return value is not defined in the Framework Specification.
                                This return value was introduced in the PI Specification.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_ARP_DEVICE)(
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_SMBUS_PPI         *This,
  IN      BOOLEAN                   ArpAll,
  IN      EFI_SMBUS_UDID            *SmbusUdid, OPTIONAL
  IN OUT  EFI_SMBUS_DEVICE_ADDRESS  *SlaveAddress OPTIONAL
  );

/**
  The GetArpMap() function returns the mapping of all the SMBus devices
  that are enumerated by the SMBus host driver.

  @param[in]      PeiServices    A pointer to the system PEI Services Table.
  @param[in]      This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param[in, out] Length         The size of the buffer that contains the SMBus device map.
  @param[in, out] SmbusDeviceMap The pointer to the device map as enumerated
                                 by the SMBus controller driver.

  @retval EFI_SUCCESS       The device map was returned correctly in the buffer.
  @retval EFI_UNSUPPORTED   GetArpMap() are not implemented by this PEIM. 
                            This return value was not defined in the Framework Specification.
                            This return value was introduced in the PI Specification.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_GET_ARP_MAP)(
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      EFI_PEI_SMBUS_PPI         *This,
  IN OUT  UINTN                     *Length,
  IN OUT  EFI_SMBUS_DEVICE_MAP      **SmbusDeviceMap
  );

/**
  Allows a device driver to register for a callback when the bus driver detects a state that it needs to 
  propagate to other PEIMs that are registered for a callback.

  The Notify() function registers all the callback functions to allow the
  bus driver to call these functions when the SlaveAddress/Data pair occur.
  All functions to be registered with EFI_PEI_SMBUS_PPI_NOTIFY must be of type
  EFI_PEI_SMBUS_NOTIFY_FUNCTION.

  @param[in] PeiServices    A pointer to the system PEI Services Table.
  @param[in] This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param[in] SlaveAddress   The address that the host controller detects as
                            sending a message and triggers all the registered functions.
  @param[in] Data           Data that the host controller detects as sending a message
                            and triggers all the registered functions.
  @param[in] NotifyFunction The function to call when the bus driver
                            detects the SlaveAddress and Data pair.

  @retval EFI_SUCCESS       NotifyFunction has been registered.
  @retval EFI_UNSUPPORTED   Notify() are not implemented by this PEIM. 
                            This return value is not defined in  the Framework Specification.
                            This return value was introduced in the PI Specification.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_NOTIFY)(
  IN      EFI_PEI_SERVICES              **PeiServices,
  IN      EFI_PEI_SMBUS_PPI             *This,
  IN      EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress,
  IN      UINTN                         Data,
  IN      EFI_PEI_SMBUS_NOTIFY_FUNCTION NotifyFunction
  );

///
/// Provides the basic I/O interfaces that a PEIM uses to access
/// its SMBus controller and the slave devices attached to it.
///
struct _EFI_PEI_SMBUS_PPI {
  ///
  /// Executes the SMBus operation to an SMBus slave device.
  ///
  EFI_PEI_SMBUS_PPI_EXECUTE_OPERATION Execute;
  
  ///
  /// Allows an SMBus 2.0 device(s) to be Address Resolution Protocol (ARP)
  ///
  EFI_PEI_SMBUS_PPI_ARP_DEVICE        ArpDevice;
  
  ///
  /// Allows a PEIM to retrieve the address that was allocated by the SMBus
  /// host controller during enumeration/ARP.  
  ///
  EFI_PEI_SMBUS_PPI_GET_ARP_MAP       GetArpMap;
  
  ///
  /// Allows a driver to register for a callback to the SMBus host
  /// controller driver when the bus issues a notification to the bus controller PEIM.  
  ///
  EFI_PEI_SMBUS_PPI_NOTIFY            Notify;
};

extern EFI_GUID gEfiPeiSmbusPpiGuid;

#endif
