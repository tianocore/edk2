/**@file
  This contains the installation function for the driver.
  
Copyright (c) 2005 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "8259.h"

//
// Global for the Legacy 8259 Protocol that is prodiced by this driver
//
EFI_LEGACY_8259_PROTOCOL  m8259 = {
  Interrupt8259SetVectorBase,
  Interrupt8259GetMask,
  Interrupt8259SetMask,
  Interrupt8259SetMode,
  Interrupt8259GetVector,
  Interrupt8259EnableIrq,
  Interrupt8259DisableIrq,
  Interrupt8259GetInterruptLine,
  Interrupt8259EndOfInterrupt
};

//
// Global for the handle that the Legacy 8259 Protocol is installed
//
EFI_HANDLE                m8259Handle             = NULL;

UINT8                     mMasterBase             = 0xff;
UINT8                     mSlaveBase              = 0xff;
EFI_8259_MODE             mMode                   = Efi8259ProtectedMode;
UINT16                    mProtectedModeMask      = 0xffff;
UINT16                    mLegacyModeMask         = 0x06b8;
UINT16                    mProtectedModeEdgeLevel = 0x0000;
UINT16                    mLegacyModeEdgeLevel    = 0x0000;

//
// Worker Functions
//
VOID
Interrupt8259WriteMask (
  IN UINT16  Mask,
  IN UINT16  EdgeLevel
  )
/**

  Routine Description:
    Sets the 8250 mask to the valud specified by Mask

  Arguments:
    Mask       - A 16 bit valute that represents the master and slave mask values

  Returns:
    None

**/
// TODO:    EdgeLevel - add argument and description to function comment
{
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, (UINT8) Mask);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, (UINT8) (Mask >> 8));
  IoWrite8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_MASTER, (UINT8) EdgeLevel);
  IoWrite8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_SLAVE, (UINT8) (EdgeLevel >> 8));
}

VOID
Interrupt8259ReadMask (
  IN UINT16  *Mask,
  IN UINT16  *EdgeLevel
  )
/**

  Routine Description:
    Sets the 8250 mask to the valud specified by Mask

  Arguments:
    Mask       - A 16 bit valute that represents the master and slave mask values

  Returns:
    None

**/
// TODO:    EdgeLevel - add argument and description to function comment
{
  UINT16  MasterValue;
  UINT16  SlaveValue;

  if (Mask != NULL) {
    MasterValue = IoRead8 (LEGACY_8259_MASK_REGISTER_MASTER);
    SlaveValue  = IoRead8 (LEGACY_8259_MASK_REGISTER_SLAVE);

    *Mask = (UINT16) (MasterValue | (SlaveValue << 8));
  }

  if (EdgeLevel != NULL) {
    MasterValue = IoRead8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_MASTER);
    SlaveValue  = IoRead8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_SLAVE);

    *EdgeLevel = (UINT16) (MasterValue | (SlaveValue << 8));
  }
}
//
// Legacy 8259 Protocol Interface Function
//
EFI_STATUS
EFIAPI
Interrupt8259SetVectorBase (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  UINT8                     MasterBase,
  IN  UINT8                     SlaveBase
  )
/**

  Routine Description:
    Sets the base vector for the 8250 Master and Slave interrupt controllers

  Arguments:
    This       - Protocol instance pointer.
    MasterBase - Base vector of the 8259 Master
    SlaveBase  - Base vector of the 8259 Slave

  Returns:
    EFI_SUCCESS       - 8259 programmed

**/
{
  UINT8 Mask;

  if (SlaveBase != mSlaveBase) {
    mSlaveBase = SlaveBase;

    //
    // Initialize Slave interrupt controller.
    //
    Mask = IoRead8 (LEGACY_8259_MASK_REGISTER_SLAVE);
    IoWrite8 (LEGACY_8259_CONTROL_REGISTER_SLAVE, 0x11);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, mSlaveBase);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0x02);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0x01);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, Mask);
  }

  if (MasterBase != mMasterBase) {
    mMasterBase = MasterBase;

    //
    // Initialize Master interrupt controller.
    //
    Mask = IoRead8 (LEGACY_8259_MASK_REGISTER_MASTER);
    IoWrite8 (LEGACY_8259_CONTROL_REGISTER_MASTER, 0x11);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, mMasterBase);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0x04);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0x01);
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, Mask);
  }

  IoWrite8 (LEGACY_8259_CONTROL_REGISTER_SLAVE, 0x20);
  IoWrite8 (LEGACY_8259_CONTROL_REGISTER_MASTER, 0x20);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Interrupt8259GetMask (
  IN  EFI_LEGACY_8259_PROTOCOL  * This,
  OUT UINT16                    *LegacyMask, OPTIONAL
  OUT UINT16                    *LegacyEdgeLevel, OPTIONAL
  OUT UINT16                    *ProtectedMask, OPTIONAL
  OUT UINT16                    *ProtectedEdgeLevel OPTIONAL
  )
/**

  Routine Description:
    Get the 8259 master and slave address that maps IRQ to processor interrupt 
    vector number. Get the Context of the device including the state of the
    interrupt mask.

  Arguments:
    This       - Protocol instance pointer.

  Returns:
    EFI_SUCCESS       - 8259 programmed
    EFI_DEVICE_ERROR  - Error writting to 8259

**/
// TODO:    LegacyMask - add argument and description to function comment
// TODO:    LegacyEdgeLevel - add argument and description to function comment
// TODO:    ProtectedMask - add argument and description to function comment
// TODO:    ProtectedEdgeLevel - add argument and description to function comment
{
  if (LegacyMask != NULL) {
    *LegacyMask = mLegacyModeMask;
  }

  if (LegacyEdgeLevel != NULL) {
    *LegacyEdgeLevel = mLegacyModeEdgeLevel;
  }

  if (ProtectedMask != NULL) {
    *ProtectedMask = mProtectedModeMask;
  }

  if (ProtectedEdgeLevel != NULL) {
    *ProtectedEdgeLevel = mProtectedModeEdgeLevel;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Interrupt8259SetMask (
  IN  EFI_LEGACY_8259_PROTOCOL  * This,
  IN  UINT16                    *LegacyMask, OPTIONAL
  IN  UINT16                    *LegacyEdgeLevel, OPTIONAL
  IN  UINT16                    *ProtectedMask, OPTIONAL
  IN  UINT16                    *ProtectedEdgeLevel OPTIONAL
  )
/**

  Routine Description:
    Set the 8259 interrupt and edge/level masks for legacy and/or protected 
    mode operation. This routine does not touch the hardware but only the
    RAM copies of the masks.

  Arguments:
    This       - Protocol instance pointer.

  Returns:
    EFI_SUCCESS       - 8259 masks updated

**/
// TODO:    LegacyMask - add argument and description to function comment
// TODO:    LegacyEdgeLevel - add argument and description to function comment
// TODO:    ProtectedMask - add argument and description to function comment
// TODO:    ProtectedEdgeLevel - add argument and description to function comment
{
  if (LegacyMask != NULL) {
    mLegacyModeMask = *LegacyMask;
  }

  if (LegacyEdgeLevel != NULL) {
    mLegacyModeEdgeLevel = *LegacyEdgeLevel;
  }

  if (ProtectedMask != NULL) {
    mProtectedModeMask = *ProtectedMask;
  }

  if (ProtectedEdgeLevel != NULL) {
    mProtectedModeEdgeLevel = *ProtectedEdgeLevel;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Interrupt8259SetMode (
  IN  EFI_LEGACY_8259_PROTOCOL  * This,
  IN  EFI_8259_MODE             Mode,
  IN  UINT16                    *Mask, OPTIONAL
  IN  UINT16                    *EdgeLevel OPTIONAL
  )
/**

  Routine Description:
    Set the 8259 master and slave address that maps IRQ to processor interrupt 
    vector number. Restore the Context of the device, so that the interrupt
    mask is put back in it's previous mode.

  Arguments:
    This  - Protocol instance pointer.
    Mode  - 
    Mask  -

  Returns:
    EFI_SUCCESS       - 8259 programmed
    EFI_DEVICE_ERROR  - Error writting to 8259

**/
// TODO:    EdgeLevel - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  if (Mode == mMode) {
    return EFI_SUCCESS;
  }

  if (Mode == Efi8259LegacyMode) {
    //
    // Save the protected mode mask
    //
    Interrupt8259ReadMask (&mProtectedModeMask, &mProtectedModeEdgeLevel);

    if (Mask != NULL) {
      //
      // Update the Mask for the new mode
      //
      mLegacyModeMask = *Mask;
    }

    if (EdgeLevel != NULL) {
      //
      // Update the Edge/Level triggered mask for the new mode
      //
      mLegacyModeEdgeLevel = *EdgeLevel;
    }

    mMode = Mode;

    //
    // Set 8259 Vector Base
    //
    //
    Interrupt8259SetVectorBase (This, LEGACY_MODE_BASE_VECTOR_MASTER, LEGACY_MODE_BASE_VECTOR_SLAVE);

    //
    // Enable Interrupts
    //
    Interrupt8259WriteMask (mLegacyModeMask, mLegacyModeEdgeLevel);

    return EFI_SUCCESS;
  }

  if (Mode == Efi8259ProtectedMode) {
    //
    // Save the legacy mode mask
    //
    Interrupt8259ReadMask (&mLegacyModeMask, &mLegacyModeEdgeLevel);
    //
    // Always force Timer to be enabled after return from 16-bit code.
    // This always insures that on next entry, timer is counting.
    //
    mLegacyModeMask &= 0xFFFE;

    if (Mask != NULL) {
      //
      // Update the Mask for the new mode
      //
      mProtectedModeMask = *Mask;
    }

    if (EdgeLevel != NULL) {
      //
      // Update the Edge/Level triggered mask for the new mode
      //
      mProtectedModeEdgeLevel = *EdgeLevel;
    }

    mMode = Mode;

    //
    // Set 8259 Vector Base
    //
    //
    Interrupt8259SetVectorBase (This, PROTECTED_MODE_BASE_VECTOR_MASTER, PROTECTED_MODE_BASE_VECTOR_SLAVE);

    //
    // Enable Interrupts
    //
    Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
Interrupt8259GetVector (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq,
  OUT UINT8                     *Vector
  )
/**

  Routine Description:
    Convert from IRQ to processor interrupt vector number.

  Arguments:
    This    - Protocol instance pointer.
    Irq     - 8259 IRQ0 - IRQ15
    Vector  - Processor vector number that matches Irq

  Returns:
    EFI_SUCCESS           - The Vector matching Irq is returned
    EFI_INVALID_PARAMETER - Irq not valid

**/
{
  if (Irq < Efi8259Irq0 || Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  if (Irq <= Efi8259Irq7) {
    *Vector = (UINT8) (mMasterBase + Irq);
  } else {
    *Vector = (UINT8) (mSlaveBase + (Irq - Efi8259Irq8));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Interrupt8259EnableIrq (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq,
  IN  BOOLEAN                   LevelTriggered
  )
/**

  Routine Description:
    Enable Irq by unmasking interrupt in 8259

  Arguments:
    This    - Protocol instance pointer.
    Irq     - 8259 IRQ0 - IRQ15

  Returns:
    EFI_SUCCESS           - Irq enabled on 8259
    EFI_INVALID_PARAMETER - Irq not valid

**/
// TODO:    LevelTriggered - add argument and description to function comment
{
  if (Irq < Efi8259Irq0 || Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  mProtectedModeMask = (UINT16) (mProtectedModeMask & ~(1 << Irq));
  if (LevelTriggered) {
    mProtectedModeEdgeLevel = (UINT16) (mProtectedModeEdgeLevel | (1 << Irq));
  } else {
    mProtectedModeEdgeLevel = (UINT16) (mProtectedModeEdgeLevel & ~(1 << Irq));
  }

  Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Interrupt8259DisableIrq (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq
  )
/**

  Routine Description:
    Disable Irq by masking interrupt in 8259

  Arguments:
    This    - Protocol instance pointer.
    Irq     - 8259 IRQ0 - IRQ15

  Returns:
    EFI_SUCCESS           - Irq disabled on 8259
    EFI_INVALID_PARAMETER - Irq not valid

**/
{
  if (Irq < Efi8259Irq0 || Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  mProtectedModeMask      = (UINT16) (mProtectedModeMask | (1 << Irq));
  mProtectedModeEdgeLevel = (UINT16) (mProtectedModeEdgeLevel & ~(1 << Irq));

  Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Interrupt8259GetInterruptLine (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_HANDLE                PciHandle,
  OUT UINT8                     *Vector
  )
/**

  Routine Description:
    PciHandle represents a PCI config space of a PCI function. Vector 
    represents Interrupt Pin (from PCI config space) and it is the data
    that is programmed into the Interrupt Line (from the PCI config space)
    register.

  Arguments:
    This      - Protocol instance pointer.
    PciHandle - PCI function to return vector for 
    Vector    - Vector for fucntion that matches 

  Returns:
    EFI_SUCCESS           - A valid Vector is returned
    EFI_INVALID_PARAMETER - PciHandle not valid

**/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
Interrupt8259EndOfInterrupt (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq
  )
/**

  Routine Description:
    Send an EOI to 8259

  Arguments:
    This    - Protocol instance pointer.
    Irq     - 8259 IRQ0 - IRQ15

  Returns:
    EFI_SUCCESS           - EOI successfully sent to 8259
    EFI_INVALID_PARAMETER - Irq not valid

**/
{
  if (Irq < Efi8259Irq0 || Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  if (Irq >= Efi8259Irq8) {
    IoWrite8 (LEGACY_8259_CONTROL_REGISTER_SLAVE, LEGACY_8259_EOI);
  }

  IoWrite8 (LEGACY_8259_CONTROL_REGISTER_MASTER, LEGACY_8259_EOI);

  return EFI_SUCCESS;
}

//
// Legacy 8259 Driver Entry Point
//
EFI_STATUS
EFIAPI
Install8259 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/**

Routine Description:
  

Arguments:

  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCCESS - Legacy 8259  Protocol Installed

**/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS   Status;
  EFI_8259_IRQ Irq;

  //
  // Clear all pending interrupt
  //
  for (Irq = Efi8259Irq0; Irq <= Efi8259Irq15; Irq++) {
    Interrupt8259EndOfInterrupt (&m8259, Irq);
  }

  //
  // Set the 8259 Master base to 0x68 and the 8259 Slave base to 0x70
  //
  Status = Interrupt8259SetVectorBase (&m8259, PROTECTED_MODE_BASE_VECTOR_MASTER, PROTECTED_MODE_BASE_VECTOR_SLAVE);

  //
  // Set all 8259 interrupts to edge triggered and disabled
  //
  Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

  //
  // Install 8259 Protocol onto a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &m8259Handle,
                  &gEfiLegacy8259ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &m8259
                  );
  return Status;
}

