/** @file
  Driver implementing the Tiano Legacy 8259 Protocol

Copyright (c) 2005, 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _8259_H__
#define _8259_H__

#include <FrameworkDxe.h>

#include <Protocol/Legacy8259.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>

//
// 8259 Hardware definitions
//
#define LEGACY_MODE_BASE_VECTOR_MASTER                    0x08
#define LEGACY_MODE_BASE_VECTOR_SLAVE                     0x70

#define PROTECTED_MODE_BASE_VECTOR_MASTER                 0x68
#define PROTECTED_MODE_BASE_VECTOR_SLAVE                  0x70

#define LEGACY_8259_CONTROL_REGISTER_MASTER               0x20
#define LEGACY_8259_MASK_REGISTER_MASTER                  0x21
#define LEGACY_8259_CONTROL_REGISTER_SLAVE                0xA0
#define LEGACY_8259_MASK_REGISTER_SLAVE                   0xA1
#define LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_MASTER  0x4D0
#define LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_SLAVE   0x4D1

#define LEGACY_8259_EOI                                   0x20

//
// Protocol Function Prototypes
//
EFI_STATUS
EFIAPI
Interrupt8259SetVectorBase (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN UINT8                     MasterBase,
  IN UINT8                     SlaveBase
  );

EFI_STATUS
EFIAPI
Interrupt8259GetMask (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  OUT UINT16                    *LegacyMask, OPTIONAL
  OUT UINT16                    *LegacyEdgeLevel, OPTIONAL
  OUT UINT16                    *ProtectedMask, OPTIONAL
  OUT UINT16                    *ProtectedEdgeLevel OPTIONAL
  );

EFI_STATUS
EFIAPI
Interrupt8259SetMask (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN UINT16                    *LegacyMask, OPTIONAL
  IN UINT16                    *LegacyEdgeLevel, OPTIONAL
  IN UINT16                    *ProtectedMask, OPTIONAL
  IN UINT16                    *ProtectedEdgeLevel OPTIONAL
  );

EFI_STATUS
EFIAPI
Interrupt8259SetMode (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN EFI_8259_MODE             Mode,
  IN UINT16                    *Mask, OPTIONAL
  IN UINT16                    *EdgeLevel OPTIONAL
  );

EFI_STATUS
EFIAPI
Interrupt8259GetVector (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq,
  OUT UINT8                     *Vector
  );

EFI_STATUS
EFIAPI
Interrupt8259EnableIrq (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN EFI_8259_IRQ              Irq,
  IN BOOLEAN                   LevelTriggered
  );

EFI_STATUS
EFIAPI
Interrupt8259DisableIrq (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN EFI_8259_IRQ              Irq
  );

EFI_STATUS
EFIAPI
Interrupt8259GetInterruptLine (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_HANDLE                PciHandle,
  OUT UINT8                     *Vector
  );

EFI_STATUS
EFIAPI
Interrupt8259EndOfInterrupt (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq
  );

#endif
