/** @file
  I/O Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  IoLib.c

**/

/**
  Reads an 8-bit I/O port.

  Reads the 8-bit I/O port specified by Port. The 8-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT8
EFIAPI
IoRead8 (
  IN      UINTN                     Port
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->IoRead8 (PeiServices, CpuIo, (UINT64) Port);
}

/**
  Writes an 8-bit I/O port.

  Writes the 8-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT8
EFIAPI
IoWrite8 (
  IN      UINTN                     Port,
  IN      UINT8                     Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->IoWrite8 (PeiServices, CpuIo, (UINT64) Port, Value);
  return Value;
}

/**
  Reads a 16-bit I/O port.

  Reads the 16-bit I/O port specified by Port. The 16-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT16
EFIAPI
IoRead16 (
  IN      UINTN                     Port
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->IoRead16 (PeiServices, CpuIo, (UINT64) Port);
}

/**
  Writes a 16-bit I/O port.

  Writes the 16-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT16
EFIAPI
IoWrite16 (
  IN      UINTN                     Port,
  IN      UINT16                    Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->IoWrite16 (PeiServices, CpuIo, (UINT64) Port, Value);
  return Value;
}

/**
  Reads a 32-bit I/O port.

  Reads the 32-bit I/O port specified by Port. The 32-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT32
EFIAPI
IoRead32 (
  IN      UINTN                     Port
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->IoRead32 (PeiServices, CpuIo, (UINT64) Port);
}

/**
  Writes a 32-bit I/O port.

  Writes the 32-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT32
EFIAPI
IoWrite32 (
  IN      UINTN                     Port,
  IN      UINT32                    Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->IoWrite32 (PeiServices, CpuIo, (UINT64) Port, Value);
  return Value;
}

/**
  Reads a 64-bit I/O port.

  Reads the 64-bit I/O port specified by Port. The 64-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT64
EFIAPI
IoRead64 (
  IN      UINTN                     Port
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->IoRead64 (PeiServices, CpuIo, (UINT64) Port);
}

/**
  Writes a 64-bit I/O port.

  Writes the 64-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT64
EFIAPI
IoWrite64 (
  IN      UINTN                     Port,
  IN      UINT64                    Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->IoWrite64 (PeiServices, CpuIo, (UINT64) Port, Value);
  return Value;;
}

/**
  Reads an 8-bit MMIO register.

  Reads the 8-bit MMIO register specified by Address. The 8-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 8-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT8
EFIAPI
MmioRead8 (
  IN      UINTN                     Address
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->MemRead8 (PeiServices, CpuIo, (UINT64) Address);
}

/**
  Writes an 8-bit MMIO register.

  Writes the 8-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 8-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT8
EFIAPI
MmioWrite8 (
  IN      UINTN                     Address,
  IN      UINT8                     Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->MemWrite8 (PeiServices, CpuIo, (UINT64) Address, Value);
  return Value;
}

/**
  Reads a 16-bit MMIO register.

  Reads the 16-bit MMIO register specified by Address. The 16-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 16-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT16
EFIAPI
MmioRead16 (
  IN      UINTN                     Address
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->MemRead16 (PeiServices, CpuIo, (UINT64) Address);

}

/**
  Writes a 16-bit MMIO register.

  Writes the 16-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 16-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT16
EFIAPI
MmioWrite16 (
  IN      UINTN                     Address,
  IN      UINT16                    Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->MemWrite16 (PeiServices, CpuIo, (UINT64) Address, Value);
  return Value;
}

/**
  Reads a 32-bit MMIO register.

  Reads the 32-bit MMIO register specified by Address. The 32-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 32-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT32
EFIAPI
MmioRead32 (
  IN      UINTN                     Address
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->MemRead32 (PeiServices, CpuIo, (UINT64) Address);

}

/**
  Writes a 32-bit MMIO register.

  Writes the 32-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 32-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT32
EFIAPI
MmioWrite32 (
  IN      UINTN                     Address,
  IN      UINT32                    Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->MemWrite32 (PeiServices, CpuIo, (UINT64) Address, Value);
  return Value;
}

/**
  Reads a 64-bit MMIO register.

  Reads the 64-bit MMIO register specified by Address. The 64-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 64-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT64
EFIAPI
MmioRead64 (
  IN      UINTN                     Address
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  return CpuIo->MemRead64 (PeiServices, CpuIo, (UINT64) Address);

}

/**
  Writes a 64-bit MMIO register.

  Writes the 64-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  If 64-bit MMIO register operations are not supported, then ASSERT().

  @param  Address The MMIO register to write.
  @param  Value   The value to write to the MMIO register.

**/
UINT64
EFIAPI
MmioWrite64 (
  IN      UINTN                     Address,
  IN      UINT64                    Value
  )
{
  EFI_PEI_SERVICES                  **PeiServices;
  EFI_PEI_CPU_IO_PPI                *CpuIo;

  PeiServices = GetPeiServicesTablePointer ();
  CpuIo       = (*PeiServices)->CpuIo;

  ASSERT (CpuIo != NULL);

  CpuIo->MemWrite64 (PeiServices, CpuIo, (UINT64) Address, Value);
  return Value;
}
