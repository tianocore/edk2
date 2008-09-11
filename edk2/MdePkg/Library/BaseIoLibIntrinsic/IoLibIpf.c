/** @file
  Common I/O Library routines.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "BaseIoLibIntrinsicInternal.h"
#include <Library/PcdLib.h>

#define MAP_PORT_BASE_TO_MEM(_Port) \
    ((((_Port) & 0xfffc) << 10) | ((_Port) & 0x0fff))

/**
  Translates I/O port address to memory address.

  This function translates I/O port address to memory address by adding the 64MB
  aligned I/O Port space to the I/O address.
  If I/O Port space base is not 64MB aligned, then ASSERT ().  

  @param  Port  The I/O port to read.

  @return The memory address.

**/
UINTN
InternalGetMemoryMapAddress (
  IN UINTN                  Port
  )
{
  UINTN                     Address;
  UINTN                     IoBlockBaseAddress;

  Address            = MAP_PORT_BASE_TO_MEM (Port);
  IoBlockBaseAddress = PcdGet64(PcdIoBlockBaseAddressForIpf);

  //
  // Make sure that the I/O Port space base is 64MB aligned.
  // 
  ASSERT ((IoBlockBaseAddress & 0x3ffffff) == 0);
  Address += IoBlockBaseAddress;

  return Address;
}

/**
  Reads a 8-bit I/O port.

  Reads the 8-bit I/O port specified by Port. The 8-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  @param  Port  The I/O port to read.

  @return The value read from Port.

**/
UINT8
EFIAPI
IoRead8 (
  IN  UINT64                 Port
  )
{
  return MmioRead8 (InternalGetMemoryMapAddress (Port));
}

/**
  Reads a 16-bit I/O port.

  Reads the 16-bit I/O port specified by Port. The 16-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  @param  Port  The I/O port to read.

  @return The value read from Port.

**/
UINT16
EFIAPI
IoRead16 (
  IN  UINT64                 Port
  )
{
  return MmioRead16 (InternalGetMemoryMapAddress (Port));
}

/**
  Reads a 32-bit I/O port.

  Reads the 32-bit I/O port specified by Port. The 32-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  @param  Port  The I/O port to read.

  @return The value read from Port.

**/
UINT32
EFIAPI
IoRead32 (
  IN  UINT64                 Port
  )
{
  return MmioRead32 (InternalGetMemoryMapAddress (Port));
}

/**
  Reads a 64-bit I/O port.

  Reads the 64-bit I/O port specified by Port. The 64-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read from Port.

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
  @param  Data  The value to write to the I/O port.

  @return The value written to the I/O port. It equals to the
          input Value instead of the actual value read back from
          the I/O port.

**/
UINT8
EFIAPI
IoWrite8 (
  IN  UINT64                 Port,
  IN  UINT8                  Data
  )
{
  return MmioWrite8 (InternalGetMemoryMapAddress (Port), Data);
}

/**
  Writes a 16-bit I/O port.

  Writes the 16-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  @param  Port  The I/O port to write.
  @param  Data  The value to write to the I/O port.

  @return The value written to the I/O port. It equals to the
          input Value instead of the actual value read back from
          the I/O port.

**/
UINT16
EFIAPI
IoWrite16 (
  IN  UINT64                 Port,
  IN  UINT16                 Data
  )
{
  return MmioWrite16 (InternalGetMemoryMapAddress (Port), Data);
}

/**
  Writes a 32-bit I/O port.

  Writes the 32-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  @param  Port  The I/O port to write.
  @param  Data  The value to write to the I/O port.

  @return The value written to the I/O port. It equals to the
          input Value instead of the actual value read back from
          the I/O port.

**/
UINT32
EFIAPI
IoWrite32 (
  IN  UINT64                 Port,
  IN  UINT32                 Data
  )
{
  return MmioWrite32 (InternalGetMemoryMapAddress (Port), Data);
}

/**
  Writes a 64-bit I/O port.

  Writes the 64-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written to the I/O port. It equals to the
          input Value instead of the actual value read back from
          the I/O port.

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

  @return The value read from Address.

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

  @return The value read from Address.

**/
UINT16
EFIAPI
MmioRead16 (
  IN  UINT64                 Address
  )
{
  UINT16           Data;

  //
  // Make sure that Address is 16-bit aligned.
  // 
  ASSERT ((Address & 1) == 0);

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

  @return The value read from Address.

**/
UINT32
EFIAPI
MmioRead32 (
  IN  UINT64                 Address
  )
{
  UINT32           Data;

  //
  // Make sure that Address is 32-bit aligned.
  // 
  ASSERT ((Address & 3) == 0);

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

  @return The value read from Address.

**/
UINT64
EFIAPI
MmioRead64 (
  IN  UINT64                 Address
  )
{
  UINT64           Data;

  //
  // Make sure that Address is 64-bit aligned.
  // 
  ASSERT ((Address & 7) == 0);

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

  @return The value written to the Mmio. It equals to the
          input Value instead of the actual value read back from
          the Mmio.

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

  @return The value written to the Mmio. It equals to the
          input Value instead of the actual value read back from
          the Mmio.

**/
UINT16
EFIAPI
MmioWrite16 (
  IN  UINT64                 Address,
  IN  UINT16                 Data
  )
{
  //
  // Make sure that Address is 16-bit aligned.
  // 
  ASSERT ((Address & 1) == 0);

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

  @return The value written to the Mmio. It equals to the
          input Value instead of the actual value read back from
          the Mmio.

**/
UINT32
EFIAPI
MmioWrite32 (
  IN  UINT64                 Address,
  IN  UINT32                 Data
  )
{
  //
  // Make sure that Address is 32-bit aligned.
  // 
  ASSERT ((Address & 3) == 0);

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

  @return The value written to the Mmio. It equals to the
          input Value instead of the actual value read back from
          the Mmio.

**/
UINT64
EFIAPI
MmioWrite64 (
  IN  UINT64                 Address,
  IN  UINT64                 Data
  )
{
  //
  // Make sure that Address is 64-bit aligned.
  // 
  ASSERT ((Address & 7) == 0);

  Address |= BIT63;

  MemoryFence ();
  *((volatile UINT64 *) Address) = Data;
  MemoryFence ();

  return Data;
}
