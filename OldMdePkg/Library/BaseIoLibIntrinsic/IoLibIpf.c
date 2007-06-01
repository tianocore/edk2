/** @file
  Common I/O Library routines.

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  IoLibIpf.c

**/

#define MAP_PORT_BASE_TO_MEM(_Port) \
    ((((_Port) & 0xfffc) << 10) | ((_Port) & 0x0fff))

/**
  Reads a 8-bit I/O port.

  Reads the 8-bit I/O port specified by Port. The 8-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT8
EFIAPI
IoRead8 (
  IN  UINT64                 Port
  )
{
  UINT64           Address;

  //
  // Add the 64MB aligned IO Port space to the IO address
  //
  Address = MAP_PORT_BASE_TO_MEM (Port);
  Address += PcdGet64(PcdIoBlockBaseAddressForIpf);

  return MmioRead8 (Address);
}

/**
  Reads a 16-bit I/O port.

  Reads the 16-bit I/O port specified by Port. The 16-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT16
EFIAPI
IoRead16 (
  IN  UINT64                 Port
  )
{
  UINT64           Address;

  //
  // Add the 64MB aligned IO Port space to the IO address
  //
  Address = MAP_PORT_BASE_TO_MEM (Port);
  Address += PcdGet64(PcdIoBlockBaseAddressForIpf);

  return MmioRead16 (Address);
}

/**
  Reads a 32-bit I/O port.

  Reads the 32-bit I/O port specified by Port. The 32-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT32
EFIAPI
IoRead32 (
  IN  UINT64                 Port
  )
{
  UINT64           Address;

  //
  // Add the 64MB aligned IO Port space to the IO address
  //
  Address = MAP_PORT_BASE_TO_MEM (Port);
  Address += PcdGet64(PcdIoBlockBaseAddressForIpf);

  return MmioRead32 (Address);
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
  ASSERT (FALSE);
  return 0;
}

/**
  Writes a 8-bit I/O port.

  Writes the 8-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT8
EFIAPI
IoWrite8 (
  IN  UINT64                 Port,
  IN  UINT8                  Data
  )
{
  UINT64           Address;

  //
  // Add the 64MB aligned IO Port space to the IO address
  //
  Address = MAP_PORT_BASE_TO_MEM (Port);
  Address += PcdGet64(PcdIoBlockBaseAddressForIpf);

  return MmioWrite8 (Address, Data);
}

/**
  Writes a 16-bit I/O port.

  Writes the 16-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT16
EFIAPI
IoWrite16 (
  IN  UINT64                 Port,
  IN  UINT16                 Data
  )
{
  UINT64           Address;

  //
  // Add the 64MB aligned IO Port space to the IO address
  //
  Address = MAP_PORT_BASE_TO_MEM (Port);
  Address += PcdGet64(PcdIoBlockBaseAddressForIpf);

  return MmioWrite16 (Address, Data);
}

/**
  Writes a 32-bit I/O port.

  Writes the 32-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT32
EFIAPI
IoWrite32 (
  IN  UINT64                 Port,
  IN  UINT32                 Data
  )
{
  UINT64           Address;

  //
  // Add the 64MB aligned IO Port space to the IO address
  //
  Address = MAP_PORT_BASE_TO_MEM (Port);
  Address += PcdGet64(PcdIoBlockBaseAddressForIpf);

  return MmioWrite32 (Address, Data);
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
  ASSERT (FALSE);
  return 0;
}

/**
  Reads a 8-bit MMIO register.

  Reads the 8-bit MMIO register specified by Address. The 8-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT8
EFIAPI
MmioRead8 (
  IN  UINT64                 Address
  )
{
  UINT8            Data;

  Address |= BIT63;

  MemoryFence ();
  Data = *((volatile UINT8 *) Address);
  MemoryFence ();

  return Data;
}

/**
  Reads a 16-bit MMIO register.

  Reads the 16-bit MMIO register specified by Address. The 16-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT16
EFIAPI
MmioRead16 (
  IN  UINT64                 Address
  )
{
  UINT16           Data;

  Address |= BIT63;

  MemoryFence ();
  Data = *((volatile UINT16 *) Address);
  MemoryFence ();

  return Data;
}

/**
  Reads a 32-bit MMIO register.

  Reads the 32-bit MMIO register specified by Address. The 32-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT32
EFIAPI
MmioRead32 (
  IN  UINT64                 Address
  )
{
  UINT32           Data;

  Address |= BIT63;

  MemoryFence ();
  Data = *((volatile UINT32 *) Address);
  MemoryFence ();

  return Data;
}

/**
  Reads a 64-bit MMIO register.

  Reads the 64-bit MMIO register specified by Address. The 64-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  @param  Address The MMIO register to read.

  @return The value read.

**/
UINT64
EFIAPI
MmioRead64 (
  IN  UINT64                 Address
  )
{
  UINT64           Data;

  Address |= BIT63;

  MemoryFence ();
  Data = *((volatile UINT64 *) Address);
  MemoryFence ();

  return Data;

}

/**
  Writes a 8-bit MMIO register.

  Writes the 8-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  @param  Address The MMIO register to write.
  @param  Data    The value to write to the MMIO register.

  @return The value written the memory address.

**/
UINT8
EFIAPI
MmioWrite8 (
  IN  UINT64                 Address,
  IN  UINT8                  Data
  )
{
  Address |= BIT63;

  MemoryFence ();
  *((volatile UINT8 *) Address) = Data;
  MemoryFence ();

  return Data;
}

/**
  Writes a 16-bit MMIO register.

  Writes the 16-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  @param  Address The MMIO register to write.
  @param  Data    The value to write to the MMIO register.

  @return The value written the memory address.

**/
UINT16
EFIAPI
MmioWrite16 (
  IN  UINT64                 Address,
  IN  UINT16                 Data
  )
{
  Address |= BIT63;

  MemoryFence ();
  *((volatile UINT16 *) Address) = Data;
  MemoryFence ();

  return Data;
}

/**
  Writes a 32-bit MMIO register.

  Writes the 32-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  @param  Address The MMIO register to write.
  @param  Data    The value to write to the MMIO register.

  @return The value written the memory address.

**/
UINT32
EFIAPI
MmioWrite32 (
  IN  UINT64                 Address,
  IN  UINT32                 Data
  )
{
  Address |= BIT63;

  MemoryFence ();
  *((volatile UINT32 *) Address) = Data;
  MemoryFence ();

  return Data;
}

/**
  Writes a 64-bit MMIO register.

  Writes the 64-bit MMIO register specified by Address with the value specified
  by Value and returns Value. This function must guarantee that all MMIO read
  and write operations are serialized.

  @param  Address The MMIO register to write.
  @param  Data    The value to write to the MMIO register.

  @return The value written the memory address.

**/
UINT64
EFIAPI
MmioWrite64 (
  IN  UINT64                 Address,
  IN  UINT64                 Data
  )
{
  Address |= BIT63;

  MemoryFence ();
  *((volatile UINT64 *) Address) = Data;
  MemoryFence ();

  return Data;
}
