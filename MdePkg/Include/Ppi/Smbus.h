/** @file
  This file declares Smbus PPI.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Smbus.h

  @par Revision Reference:
  This PPI is defined in Framework of EFI SmBus PPI spec.
  Version 0.9

**/

#ifndef _PEI_SMBUS_PPI_H
#define _PEI_SMBUS_PPI_H

#define EFI_PEI_SMBUS_PPI_GUID \
  { \
    0xabd42895, 0x78cf, 0x4872, {0x84, 0x44, 0x1b, 0x5c, 0x18, 0xb, 0xfb, 0xda } \
  }

typedef struct _EFI_PEI_SMBUS_PPI EFI_PEI_SMBUS_PPI;

/**
  Executes an SMBus operation to an SMBus controller.

  @param  PeiServices    A pointer to the system PEI Services Table.
  @param  This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param  SlaveAddress   The SMBUS hardware address to which the SMBUS
                         device is preassigned or allocated.
  @param  Command        This command is transmitted by the SMBus host
                         controller to the SMBus slave device and the interpretation is
                         SMBus slave device specific.
  @param  Operation      Signifies which particular SMBus hardware protocol
                         instance that it will use to execute the SMBus transactions.
  @param  PecCheck       Defines if Packet Error Code (PEC) checking is required
                         for this operation.
  @param  Length         Signifies the number of bytes that this operation will do.
  @param  Buffer         Contains the value of data to execute to the SMBus slave device.

  @retval EFI_SUCCESS           The last data that was returned from the access
                                matched the poll exit criteria.
  @retval EFI_CRC_ERROR         The checksum is not correct (PEC is incorrect)
  @retval EFI_TIMEOUT           Timeout expired before the operation was completed.
                                Timeout is determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed
                                due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The request was not completed because
                                a failure reflected in the Host Status Register bit.
  @retval EFI_INVALID_PARAMETER Operation is not defined in EFI_SMBUS_OPERATION.
                                Or Length/Buffer is NULL for operations except for EfiSmbusQuickRead and
                                EfiSmbusQuickWrite. Length is outside the range of valid values.
  @retval EFI_UNSUPPORTED       The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL  Buffer is not sufficient for this operation.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_EXECUTE_OPERATION) (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_SMBUS_PPI              *This,
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN      EFI_SMBUS_DEVICE_COMMAND  Command,
  IN      EFI_SMBUS_OPERATION       Operation,
  IN      BOOLEAN                   PecCheck,
  IN OUT  UINTN                     *Length,
  IN OUT  VOID                      *Buffer
  );

typedef struct {
  UINT32  VendorSpecificId;
  UINT16  SubsystemDeviceId;
  UINT16  SubsystemVendorId;
  UINT16  Interface;
  UINT16  DeviceId;
  UINT16  VendorId;
  UINT8   VendorRevision;
  UINT8   DeviceCapabilities;
} EFI_SMBUS_UDID;

/**
  CallBack function can be registered in EFI_PEI_SMBUS_PPI_NOTIFY.

  @param  PeiServices    A pointer to the system PEI Services Table.
  @param  This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param  SlaveAddress   The SMBUS hardware address to which the SMBUS
                         device is preassigned or allocated.
  @param  Data           Data of the SMBus host notify command that
                         the caller wants to be called.

  @return Status Code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_NOTIFY_FUNCTION) (
  IN      EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_SMBUS_PPI                  *SmbusPpi,
  IN      EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress,
  IN      UINTN                         Data
  );

/**
  The ArpDevice() function enumerates the entire bus or enumerates a specific 
  device that is identified by SmbusUdid. 

  @param  PeiServices    A pointer to the system PEI Services Table.
  @param  This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param  ArpAll         A Boolean expression that indicates if the host drivers need
                         to enumerate all the devices or enumerate only the device that is identified
                         by SmbusUdid. If ArpAll is TRUE, SmbusUdid and SlaveAddress are optional.
                         If ArpAll is FALSE, ArpDevice will enumerate SmbusUdid and the address
                         will be at SlaveAddress.
  @param  SmbusUdid      The targeted SMBus Unique Device Identifier (UDID).
                         The UDID may not exist for SMBus devices with fixed addresses.
  @param  SlaveAddress   The new SMBus address for the slave device for
                         which the operation is targeted.

  @retval EFI_SUCCESS           The SMBus slave device address was set.
  @retval EFI_INVALID_PARAMETER SlaveAddress is NULL.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed
                                due to a lack of resources.
  @retval EFI_TIMEOUT           The SMBus slave device did not respond.
  @retval EFI_DEVICE_ERROR      The request was not completed because the transaction failed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_ARP_DEVICE) (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_SMBUS_PPI              *This,
  IN      BOOLEAN                   ArpAll,
  IN      EFI_SMBUS_UDID            *SmbusUdid, OPTIONAL
  IN OUT  EFI_SMBUS_DEVICE_ADDRESS  *SlaveAddress OPTIONAL
  );

typedef struct {
  EFI_SMBUS_DEVICE_ADDRESS  SmbusDeviceAddress;
  EFI_SMBUS_UDID            SmbusDeviceUdid;
} EFI_SMBUS_DEVICE_MAP;

/**
  The GetArpMap() function returns the mapping of all the SMBus devices 
  that are enumerated by the SMBus host driver. 

  @param  PeiServices    A pointer to the system PEI Services Table.
  @param  This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param  Length         Size of the buffer that contains the SMBus device map.
  @param  SmbusDeviceMap The pointer to the device map as enumerated
                         by the SMBus controller driver.

  @retval EFI_SUCCESS           The device map was returned correctly in the buffer.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_GET_ARP_MAP) (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_SMBUS_PPI              *This,
  IN OUT  UINTN                     *Length,
  IN OUT  EFI_SMBUS_DEVICE_MAP      **SmbusDeviceMap
  );

/**
  The Notify() function registers all the callback functions to allow the 
  bus driver to call these functions when the SlaveAddress/Data pair happens.

  @param  PeiServices    A pointer to the system PEI Services Table.
  @param  This           A pointer to the EFI_PEI_SMBUS_PPI instance.
  @param  SlaveAddress   Address that the host controller detects as
                         sending a message and calls all the registered functions.
  @param  Data           Data that the host controller detects as sending a message
                         and calls all the registered functions.
  @param  NotifyFunction The function to call when the bus driver
                         detects the SlaveAddress and Data pair.

  @retval EFI_SUCCESS           NotifyFunction has been registered.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMBUS_PPI_NOTIFY) (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_SMBUS_PPI              *This,
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN      UINTN                     Data,
  IN  EFI_PEI_SMBUS_NOTIFY_FUNCTION NotifyFunction
  );

/**
  @par Ppi Description:
  Provides the basic I/O interfaces that a PEIM uses to access 
  its SMBus controller and the slave devices attached to it.

  @param Execute
  Executes the SMBus operation to an SMBus slave device.

  @param ArpDevice
  Allows an SMBus 2.0 device(s) to be Address Resolution Protocol (ARP)

  @param GetArpMap
  Allows a PEIM to retrieve the address that was allocated by the SMBus 
  host controller during enumeration/ARP.

  @param Notify
  Allows a driver to register for a callback to the SMBus host 
  controller driver when the bus issues a notification to the bus controller PEIM.

**/
struct _EFI_PEI_SMBUS_PPI {
  EFI_PEI_SMBUS_PPI_EXECUTE_OPERATION Execute;
  EFI_PEI_SMBUS_PPI_ARP_DEVICE        ArpDevice;
  EFI_PEI_SMBUS_PPI_GET_ARP_MAP       GetArpMap;
  EFI_PEI_SMBUS_PPI_NOTIFY            Notify;
};

extern EFI_GUID gEfiPeiSmbusPpiGuid;

#endif
