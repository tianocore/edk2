/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

    LpcIsaAcpi.h

Abstract:

    Isa Acpi interface



--*/

#ifndef _LPC_ISA_ACPI_H
#define _LPC_ISA_ACPI_H



#include "Protocol/IsaAcpi.h"
#include "Library/DevicePathLib.h"


typedef struct {
  UINT8  Register;
  UINT8  Value;
} ICH_DMA_INIT;

//
// Prototypes for the ISA ACPI protocol interface
//
EFI_STATUS
EFIAPI
IsaDeviceEnumerate (
  IN     EFI_ISA_ACPI_PROTOCOL       *This,
  OUT    EFI_ISA_ACPI_DEVICE_ID      **Device
  );

EFI_STATUS
EFIAPI
IsaDeviceSetPower (
  IN     EFI_ISA_ACPI_PROTOCOL       *This,
  IN     EFI_ISA_ACPI_DEVICE_ID      *Device,
  IN     BOOLEAN                     OnOff
  );

EFI_STATUS
EFIAPI
IsaGetCurrentResource (
  IN     EFI_ISA_ACPI_PROTOCOL       *This,
  IN     EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT    EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );

EFI_STATUS
EFIAPI
IsaGetPossibleResource (
  IN     EFI_ISA_ACPI_PROTOCOL       *This,
  IN     EFI_ISA_ACPI_DEVICE_ID      *Device,
  OUT    EFI_ISA_ACPI_RESOURCE_LIST  **ResourceList
  );

EFI_STATUS
EFIAPI
IsaSetResource (
  IN     EFI_ISA_ACPI_PROTOCOL       *This,
  IN     EFI_ISA_ACPI_DEVICE_ID      *Device,
  IN     EFI_ISA_ACPI_RESOURCE_LIST  *ResourceList
  );

EFI_STATUS
EFIAPI
IsaEnableDevice (
  IN    EFI_ISA_ACPI_PROTOCOL        *This,
  IN    EFI_ISA_ACPI_DEVICE_ID       *Device,
  IN    BOOLEAN                      Enable
  );

EFI_STATUS
EFIAPI
IsaInitDevice (
  IN    EFI_ISA_ACPI_PROTOCOL        *This,
  IN    EFI_ISA_ACPI_DEVICE_ID       *Device
  );

EFI_STATUS
EFIAPI
LpcInterfaceInit (
  IN    EFI_ISA_ACPI_PROTOCOL        *This
);

VOID
EmptyResourceList (
  IN    UINT32      DeviceHid
);

#endif
