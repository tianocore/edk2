/*++

Copyright (c) 2006 Intel Corporation. All rights reserved
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

  IdeController.h

Abstract:

  Header file for chipset ATA controller driver.

Revision History
--*/

#ifndef _IDE_CONTROLLER_H
#define _IDE_CONTROLLER_H

#include <PiDxe.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/IdeControllerInit.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL  gIdeControllerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gIdeControllerName;
extern EFI_COMPONENT_NAME2_PROTOCOL gIdeControllerName2;

#include <IndustryStandard/Pci22.h>

//
// Symbol definition, for PCI IDE configuration field
//
#define PCI_CLASS_MASS_STORAGE  0x01
#define PCI_SUB_CLASS_IDE       0x01

//
// Supports 2 channel max
//
#define ICH_IDE_MAX_CHANNEL 0x02
//
// Supports 2 devices max
//
#define ICH_IDE_MAX_DEVICES 0x02
#define ICH_IDE_ENUMER_ALL  FALSE

#define IDE_CONTROLLER_SIGNATURE  SIGNATURE_32 ('i', 'i', 'd', 'e')

//
// Ide controller driver private data structure
//
typedef struct {
  //
  // Standard signature used to identify Ide controller private data
  //
  UINT32                            Signature;

  //
  // Protocol instance of IDE_CONTROLLER_INIT produced by this driver
  //
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  IdeInit;

  //
  // copy of protocol pointers used by this driver
  //
  EFI_PCI_IO_PROTOCOL               *PciIo;

  //
  // The highest disqulified mode for each attached Ide device.
  // Per ATA/ATAPI spec, if a mode is not supported, the modes higher than
  // it should not be supported
  //
  EFI_ATA_COLLECTIVE_MODE           DisqulifiedModes[ICH_IDE_MAX_CHANNEL][ICH_IDE_MAX_DEVICES];

  //
  // A copy of EFI_IDENTIFY_DATA data for each attached Ide device and its flag
  //
  EFI_IDENTIFY_DATA                 IdentifyData[ICH_IDE_MAX_CHANNEL][ICH_IDE_MAX_DEVICES];
  BOOLEAN                           IdentifyValid[ICH_IDE_MAX_CHANNEL][ICH_IDE_MAX_DEVICES];

} EFI_IDE_CONTROLLER_PRIVATE_DATA;

#define IDE_CONTROLLER_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      EFI_IDE_CONTROLLER_PRIVATE_DATA, \
      IdeInit, \
      IDE_CONTROLLER_SIGNATURE \
      )

//
// Driver binding functions declaration
//
EFI_STATUS
EFIAPI
IdeControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN EFI_HANDLE                        Controller,
  IN EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath
  )
/*++

  Routine Description:
  
  Register Driver Binding protocol for this driver.
  
  Arguments:
  
    This                 -- a pointer points to the Binding Protocol instance
    Controller           -- The handle of controller to be tested. 
    *RemainingDevicePath -- A pointer to the device path. Ignored by device
                            driver but used by bus driver

  Returns:

    EFI_SUCCESS          -- Driver loaded.
    other                -- Driver not loaded.
--*/
;

EFI_STATUS
EFIAPI
IdeControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL        *This,
  IN EFI_HANDLE                         Controller,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  )
/*++

  Routine Description:
  
    This routine is called right after the .Supported() called and return 
    EFI_SUCCESS. Notes: The supported protocols are checked but the Protocols
    are closed.

  Arguments:
      
    This                 -- a pointer points to the Binding Protocol instance
    Controller           -- The handle of controller to be tested. Parameter
                            passed by the caller
    *RemainingDevicePath -- A pointer to the device path. Should be ignored by
                            device driver
--*/
;

EFI_STATUS
EFIAPI
IdeControllerStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN  EFI_HANDLE                        Controller,
  IN  UINTN                             NumberOfChildren,
  IN  EFI_HANDLE                        *ChildHandleBuffer
  )
/*++
  
  Routine Description:
    Stop this driver on Controller Handle. 

  Arguments:
    This              - Protocol instance pointer.
    Controller        - Handle of device to stop driver on 
    NumberOfChildren  - Not used
    ChildHandleBuffer - Not used

  Returns:
    EFI_SUCCESS       - This driver is removed DeviceHandle
    other             - This driver was not removed from this device
  
--*/
;

//
// IDE controller init functions declaration
//
EFI_STATUS
EFIAPI
IdeInitGetChannelInfo (
  IN   EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN   UINT8                            Channel,
  OUT  BOOLEAN                          *Enabled,
  OUT  UINT8                            *MaxDevices
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  Channel     - TODO: add argument description
  Enabled     - TODO: add argument description
  MaxDevices  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IdeInitNotifyPhase (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  EFI_IDE_CONTROLLER_ENUM_PHASE     Phase,
  OUT UINT8                             Channel
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Phase   - TODO: add argument description
  Channel - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IdeInitSubmitData (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  IN  EFI_IDENTIFY_DATA                 *IdentifyData
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This          - TODO: add argument description
  Channel       - TODO: add argument description
  Device        - TODO: add argument description
  IdentifyData  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IdeInitSubmitFailingModes (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Channel - TODO: add argument description
  Device  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IdeInitDisqualifyMode (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  IN  EFI_ATA_COLLECTIVE_MODE           *BadModes
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Channel   - TODO: add argument description
  Device    - TODO: add argument description
  BadModes  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IdeInitCalculateMode (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  IN  EFI_ATA_COLLECTIVE_MODE           **SupportedModes
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This            - TODO: add argument description
  Channel         - TODO: add argument description
  Device          - TODO: add argument description
  SupportedModes  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IdeInitSetTiming (
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  IN  EFI_ATA_COLLECTIVE_MODE           *Modes
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Channel - TODO: add argument description
  Device  - TODO: add argument description
  Modes   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
