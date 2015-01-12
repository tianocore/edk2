/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


**/

#ifndef __I2C_ACPI_H__
#define __I2C_ACPI_H__

#include <Protocol/DevicePath.h>

//
// I2C ACPI protocol
//
typedef struct _EFI_I2C_ACPI_PROTOCOL   EFI_I2C_ACPI_PROTOCOL;

//
// I2C device description
//
// This structure provides the platform specific information which
// describes an I2C device.
//
typedef struct {
  //
  // Hardware revision - ACPI _HRV value
  //
  UINT32 HardwareRevision;

  //
  // Device path node for the I2C device.
  //
  CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  //
  // I2C bus configuration for the I2C device.
  //
  UINT32 I2cBusConfiguration;

  //
  // Number of slave addresses for the I2C device.
  //
  UINT32 SlaveAddressCount;

  //
  // Address of the array of slave addresses for the I2C device.
  //
  CONST UINT32 *SlaveAddressArray;
}EFI_I2C_DEVICE;


/**
  Enumerate the I2C devices

  This routine must be called at or below TPL_NOTIFY.

  This function walks the platform specific data to enumerates the
  I2C devices on an I2C bus.

  @param[in]  This              Address of an EFI_I2C_ENUM_PROTOCOL
                                structure.
  @param[in, out] Device        Buffer containing the address of an
                                EFI_I2C_DEVICE structure.  Enumeration
                                is started by setting the initial
                                EFI_I2C_DEVICE structure address to NULL.
                                The buffer receives an EFI_I2C_DEVICE
                                structure address for the next I2C device.

  @retval EFI_SUCCESS           The platform data for the next device
                                on the I2C bus was returned successfully.
  @retval EFI_INVALID_PARAMETER NextDevice was NULL
  @retval EFI_NO_MAPPING        PreviousDevice does not point to a valid
                                EFI_I2C_DEVICE structure.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_ACPI_ENUMERATE) (
  IN CONST EFI_I2C_ACPI_PROTOCOL *This,
  IN OUT CONST EFI_I2C_DEVICE **Device
  );

//
// I2C device description
//
// This structure provides the platform specific information which
// describes an I2C device.
//
struct _EFI_I2C_ACPI_PROTOCOL {
  //
  // Walk the platform's list of I2C devices on the bus.  This
  // routine returns the next I2C device in the platform's list
  // for this I2C bus.
  //
  EFI_I2C_ACPI_ENUMERATE Enumerate;
};

//
// Variable containing the GUID for the I2C device enumeration protocol
//
extern EFI_GUID gEfiI2cAcpiProtocolGuid;

#endif  //  __I2C_ACPI_H__
