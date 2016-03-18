/** @file
  The header file for ISA driver
  
Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISA_DRIVER_H_
#define _ISA_DRIVER_H_


#include <Uefi.h>

#include <Protocol/PciIo.h>
#include <Protocol/SuperIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/IsaIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/GenericMemoryTest.h>
#include <Guid/StatusCodeDataTypeId.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/Acpi.h>

#include "ComponentName.h"

//
// 8237 DMA registers
//
#define R_8237_DMA_BASE_CA_CH0                    0x00
#define R_8237_DMA_BASE_CA_CH1                    0x02
#define R_8237_DMA_BASE_CA_CH2                    0x04
#define R_8237_DMA_BASE_CA_CH3                    0xd6
#define R_8237_DMA_BASE_CA_CH5                    0xc4
#define R_8237_DMA_BASE_CA_CH6                    0xc8
#define R_8237_DMA_BASE_CA_CH7                    0xcc

#define R_8237_DMA_BASE_CC_CH0                    0x01
#define R_8237_DMA_BASE_CC_CH1                    0x03
#define R_8237_DMA_BASE_CC_CH2                    0x05
#define R_8237_DMA_BASE_CC_CH3                    0xd7
#define R_8237_DMA_BASE_CC_CH5                    0xc6
#define R_8237_DMA_BASE_CC_CH6                    0xca
#define R_8237_DMA_BASE_CC_CH7                    0xce

#define R_8237_DMA_MEM_LP_CH0                     0x87
#define R_8237_DMA_MEM_LP_CH1                     0x83
#define R_8237_DMA_MEM_LP_CH2                     0x81
#define R_8237_DMA_MEM_LP_CH3                     0x82
#define R_8237_DMA_MEM_LP_CH5                     0x8B
#define R_8237_DMA_MEM_LP_CH6                     0x89
#define R_8237_DMA_MEM_LP_CH7                     0x8A


#define R_8237_DMA_COMMAND_CH0_3                  0x08
#define R_8237_DMA_COMMAND_CH4_7                  0xd0
#define   B_8237_DMA_COMMAND_GAP                  0x10
#define   B_8237_DMA_COMMAND_CGE                  0x04


#define R_8237_DMA_STA_CH0_3                      0xd8
#define R_8237_DMA_STA_CH4_7                      0xd0

#define R_8237_DMA_WRSMSK_CH0_3                   0x0a
#define R_8237_DMA_WRSMSK_CH4_7                   0xd4
#define   B_8237_DMA_WRSMSK_CMS                   0x04


#define R_8237_DMA_CHMODE_CH0_3                   0x0b
#define R_8237_DMA_CHMODE_CH4_7                   0xd6
#define   V_8237_DMA_CHMODE_DEMAND                0x00
#define   V_8237_DMA_CHMODE_SINGLE                0x40
#define   V_8237_DMA_CHMODE_CASCADE               0xc0
#define   B_8237_DMA_CHMODE_DECREMENT             0x20
#define   B_8237_DMA_CHMODE_INCREMENT             0x00
#define   B_8237_DMA_CHMODE_AE                    0x10
#define   V_8237_DMA_CHMODE_VERIFY                0
#define   V_8237_DMA_CHMODE_IO2MEM                0x04
#define   V_8237_DMA_CHMODE_MEM2IO                0x08

#define R_8237_DMA_CBPR_CH0_3                     0x0c
#define R_8237_DMA_CBPR_CH4_7                     0xd8

#define R_8237_DMA_MCR_CH0_3                      0x0d
#define R_8237_DMA_MCR_CH4_7                      0xda

#define R_8237_DMA_CLMSK_CH0_3                    0x0e
#define R_8237_DMA_CLMSK_CH4_7                    0xdc

#define R_8237_DMA_WRMSK_CH0_3                    0x0f
#define R_8237_DMA_WRMSK_CH4_7                    0xde

typedef enum {
  IsaAccessTypeUnknown,
  IsaAccessTypeIo,
  IsaAccessTypeMem,
  IsaAccessTypeMaxType
} ISA_ACCESS_TYPE;

typedef struct {
  UINT8 Address;
  UINT8 Page;
  UINT8 Count;
} EFI_ISA_DMA_REGISTERS;

//
// ISA I/O Device Structure
//
#define ISA_IO_DEVICE_SIGNATURE SIGNATURE_32 ('i', 's', 'a', 'i')

typedef struct {
  UINT32                                    Signature;
  EFI_HANDLE                                Handle;
  EFI_ISA_IO_PROTOCOL                       IsaIo;
  EFI_PCI_IO_PROTOCOL                       *PciIo;
} ISA_IO_DEVICE;

#define ISA_IO_DEVICE_FROM_ISA_IO_THIS(a) CR (a, ISA_IO_DEVICE, IsaIo, ISA_IO_DEVICE_SIGNATURE)

//
// Mapping structure for performing ISA DMA to a buffer above 16 MB
//
typedef struct {
  EFI_ISA_IO_PROTOCOL_OPERATION Operation;
  UINTN                         NumberOfBytes;
  UINTN                         NumberOfPages;
  EFI_PHYSICAL_ADDRESS          HostAddress;
  EFI_PHYSICAL_ADDRESS          MappedHostAddress;
} ISA_MAP_INFO;

//
// EFI Driver Binding Protocol Interface Functions
//

/** 
  Tests to see if a controller can be managed by the ISA Driver.

  How the Start() function of a driver is implemented can affect how the Supported() function is implemented.

  @param[in] This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.  
  @param[in] Controller           The handle of the controller to test.
  @param[in] RemainingDevicePath  A pointer to the remaining portion of a device path.
  
  @retval EFI_SUCCESS             The device is supported by this driver.
  @retval EFI_ALREADY_STARTED     The device is already being managed by this driver.
  @retval EFI_ACCESS_DENIED       The device is already being managed by a different driver 
                                  or an application that requires exclusive access.
  @retval EFI_UNSUPPORTED         The device is is not supported by this driver.

**/
EFI_STATUS
EFIAPI
IsaIoDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. 
  
  The Start() function is designed to be invoked from the EFI boot service ConnectController(). 
  As a result, much of the error checking on the parameters to Start() has been moved into this 
  common boot service. It is legal to call Start() from other locations, but the following calling 
  restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.  

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.
**/
EFI_STATUS
EFIAPI
IsaIoDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle. 
  
  The Stop() function is designed to be invoked from the EFI boot service DisconnectController(). 
  As a result, much of the error checking on the parameters to Stop() has been moved 
  into this common boot service. It is legal to call Stop() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  
  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must 
                                support a bus specific I/O protocol for the driver 
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.
**/
EFI_STATUS
EFIAPI
IsaIoDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   * ChildHandleBuffer OPTIONAL
  );

//
// Function Prototypes
//

/**
  Initializes an ISA I/O Instance

  @param[in] IsaIoDevice            The isa device to be initialized.
  @param[in] DevicePath             The device path of the isa device.
  @param[in] Resources              The ACPI resource list.
  
**/
VOID
InitializeIsaIoInstance (
  IN ISA_IO_DEVICE               *IsaIoDevice,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN ACPI_RESOURCE_HEADER_PTR    Resources
  );

#endif

