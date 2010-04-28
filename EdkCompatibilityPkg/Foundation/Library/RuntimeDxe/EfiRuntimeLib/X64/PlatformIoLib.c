/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PlatformIoLib.c

Abstract:

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"
#include EFI_PROTOCOL_DEFINITION (CpuIo)

#define PCI_CONFIG_INDEX_PORT    0xcf8
#define PCI_CONFIG_DATA_PORT     0xcfc
#define REFRESH_CYCLE_TOGGLE_BIT 0x10

UINT32
GetPciAddress (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register
  )
/*++

Routine Description:
  Constructs PCI Address 32 bits
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register

Returns:
  PciAddress to be written to Config Port

--*/
{
  UINT32  Data;

  Data  = 0;

  Data  = (((UINT32) Segment) << 24);
  Data |= (((UINT32) Bus) << 16);
  Data |= (((UINT32) DevFunc) << 8);
  Data |= (UINT32) Register;

  return Data;

}

UINT8
PciRead8 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register
  )
/*++

Routine Description:
  Perform an one byte PCI config cycle read
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register

Returns:
  Data read from PCI config space

--*/
{
  EFI_STATUS  Status;
  UINT32      PciAddress;
  UINT32      PciAddress1;
  UINT8       Data;

  PciAddress = GetPciAddress (Segment, Bus, DevFunc, Register);
  //
  // Set bit 31 for PCI config access
  //
  PciAddress1 = PciAddress;
  PciAddress  = ((PciAddress & 0xFFFFFFFC) | (0x80000000));

  Status      = EfiIoWrite (EfiCpuIoWidthUint32, PCI_CONFIG_INDEX_PORT, 1, &PciAddress);

  if (EFI_ERROR (Status)) {
    return 0;
  }

  EfiIoRead (EfiCpuIoWidthUint8, (PCI_CONFIG_DATA_PORT + (PciAddress1 & 0x3)), 1, &Data);

  return Data;
}

UINT16
PciRead16 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register
  )
/*++

Routine Description:
  Perform an two byte PCI config cycle read
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register

Returns:
  Data read from PCI config space

--*/
{
  EFI_STATUS  Status;
  UINT32      PciAddress;
  UINT32      PciAddress1;
  UINT16      Data;

  PciAddress = GetPciAddress (Segment, Bus, DevFunc, Register);
  //
  // Set bit 31 for PCI config access
  //
  PciAddress1 = PciAddress;
  PciAddress  = ((PciAddress & 0xFFFFFFFC) | (0x80000000));

  Status      = EfiIoWrite (EfiCpuIoWidthUint32, PCI_CONFIG_INDEX_PORT, 1, &PciAddress);

  if (EFI_ERROR (Status)) {
    return 0;
  }

  EfiIoRead (EfiCpuIoWidthUint16, (PCI_CONFIG_DATA_PORT + (PciAddress1 & 0x3)), 1, &Data);

  return Data;
}

UINT32
PciRead32 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register
  )
/*++

Routine Description:
  Perform an four byte PCI config cycle read
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register

Returns:
  Data read from PCI config space

--*/
{
  EFI_STATUS  Status;
  UINT32      PciAddress;
  UINT32      PciAddress1;
  UINT32      Data;

  PciAddress = GetPciAddress (Segment, Bus, DevFunc, Register);
  //
  // Set bit 31 for PCI config access
  //
  PciAddress1 = PciAddress;
  PciAddress  = ((PciAddress & 0xFFFFFFFC) | (0x80000000));

  Status      = EfiIoWrite (EfiCpuIoWidthUint32, PCI_CONFIG_INDEX_PORT, 1, &PciAddress);

  if (EFI_ERROR (Status)) {
    return 0;
  }

  EfiIoRead (EfiCpuIoWidthUint32, (PCI_CONFIG_DATA_PORT + (PciAddress1 & 0x3)), 1, &Data);

  return Data;
}

VOID
PciWrite8 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register,
  UINT8   Data
  )
/*++

Routine Description:
  Perform an one byte PCI config cycle write
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register
  Data      - Data to write

Returns:
  NONE

--*/
{
  EFI_STATUS  Status;
  UINT32      PciAddress;
  UINT32      PciAddress1;

  PciAddress = GetPciAddress (Segment, Bus, DevFunc, Register);
  //
  // Set bit 31 for PCI config access
  //
  PciAddress1 = PciAddress;
  PciAddress  = ((PciAddress & 0xFFFFFFFC) | (0x80000000));

  Status      = EfiIoWrite (EfiCpuIoWidthUint32, PCI_CONFIG_INDEX_PORT, 1, &PciAddress);

  if (EFI_ERROR (Status)) {
    return ;
  }

  EfiIoWrite (EfiCpuIoWidthUint8, (PCI_CONFIG_DATA_PORT + (PciAddress1 & 0x3)), 1, &Data);
}

VOID
PciWrite16 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register,
  UINT16  Data
  )
/*++

Routine Description:
  Perform an two byte PCI config cycle write
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register
  Data      - Data to write

Returns:
  NONE

--*/
{
  EFI_STATUS  Status;
  UINT32      PciAddress;
  UINT32      PciAddress1;

  PciAddress = GetPciAddress (Segment, Bus, DevFunc, Register);
  //
  // Set bit 31 for PCI config access
  //
  PciAddress1 = PciAddress;
  PciAddress  = ((PciAddress & 0xFFFFFFFC) | (0x80000000));

  Status      = EfiIoWrite (EfiCpuIoWidthUint32, PCI_CONFIG_INDEX_PORT, 1, &PciAddress);

  if (EFI_ERROR (Status)) {
    return ;
  }

  EfiIoWrite (EfiCpuIoWidthUint16, (PCI_CONFIG_DATA_PORT + (PciAddress1 & 0x3)), 1, &Data);
}

VOID
PciWrite32 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register,
  UINT32  Data
  )
/*++

Routine Description:
  Perform an four byte PCI config cycle write
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register
  Data      - Data to write

Returns:
  NONE

--*/
{
  EFI_STATUS  Status;
  UINT32      PciAddress;
  UINT32      PciAddress1;

  PciAddress = GetPciAddress (Segment, Bus, DevFunc, Register);
  //
  // Set bit 31 for PCI config access
  //
  PciAddress1 = PciAddress;
  PciAddress  = ((PciAddress & 0xFFFFFFFC) | (0x80000000));

  Status      = EfiIoWrite (EfiCpuIoWidthUint32, PCI_CONFIG_INDEX_PORT, 1, &PciAddress);

  if (EFI_ERROR (Status)) {
    return ;
  }

  EfiIoWrite (EfiCpuIoWidthUint32, (PCI_CONFIG_DATA_PORT + (PciAddress1 & 0x3)), 1, &Data);
}
//
// Delay Primative
//
VOID
EfiStall (
  IN  UINTN   Microseconds
  )
/*++

Routine Description:
 Delay for at least the request number of microseconds
    
Arguments:
  Microseconds - Number of microseconds to delay.

Returns:
  NONE

--*/
{
  UINT8 Data;
  UINT8 InitialState;
  UINTN CycleIterations;

  CycleIterations = 0;
  Data            = 0;
  InitialState    = 0;

  if (EfiAtRuntime ()) {
    //
    // The time-source is 30 us granular, so calibrate the timing loop
    // based on this baseline
    // Error is possible 30us.
    //
    CycleIterations = (Microseconds - 1) / 30 + 1;

    //
    // Use the DMA Refresh timer in port 0x61.  Cheap but effective.
    // The only issue is that the granularity is 30us, and we want to
    // guarantee "at least" one full transition to avoid races.
    //
    //
    //   _____________/----------\__________/--------
    //
    //                |<--15us-->|<--15us-->|
    //
    // --------------------------------------------------> Time (us)
    //
    while (CycleIterations--) {
      EfiIoRead (EfiCpuIoWidthUint8, 0x61, 1, &Data);
      Data &= REFRESH_CYCLE_TOGGLE_BIT;
      InitialState = Data;

      //
      // Capture first transition (strictly less than one period)
      //
      while (InitialState == Data) {
        EfiIoRead (EfiCpuIoWidthUint8, 0x61, 1, &Data);
        Data &= REFRESH_CYCLE_TOGGLE_BIT;
      }

      InitialState = Data;
      //
      // Capture next transition (guarantee at least one full pulse)
      //
      while (InitialState == Data) {
        EfiIoRead (EfiCpuIoWidthUint8, 0x61, 1, &Data);
        Data &= REFRESH_CYCLE_TOGGLE_BIT;
      }
    }
  } else {
    gBS->Stall (Microseconds);
  }
}
