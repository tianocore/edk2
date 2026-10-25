/** @file
  I/O library for platforms that map legacy I/O port accesses to MMIO.

  If PcdPciIoTranslation is non-zero, legacy I/O port accesses are mapped to
  the firmware-visible MMIO translation window. If PcdPciIoTranslation is zero,
  I/O port accesses keep the same ASSERT behavior as IoLibNoIo.c.

  This source is currently selected for LoongArch, and can be reused by other
  architectures that have the same platform-provided translation model.

  Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>
  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Include common header file for this module.
//
#include "BaseIoLibIntrinsicInternal.h"
#include <Library/PcdLib.h>

/**
  Translates an I/O port to an MMIO address.

  If PcdPciIoTranslation is zero, then legacy I/O port accesses are not
  supported. In that case this function asserts and returns FALSE so callers
  can preserve the same no-I/O behavior as IoLibNoIo.c without touching MMIO
  address 0.

  @param[in]  Port     The I/O port to translate.
  @param[out] Address  The translated MMIO address.

  @retval TRUE   Port was translated and Address is valid.
  @retval FALSE  No I/O port MMIO translation window is available.

**/
STATIC
BOOLEAN
IoPortToMmioAddress (
  IN  UINTN  Port,
  OUT UINTN  *Address
  )
{
  UINT64  Translation;
  UINT64  MmioAddress;

  Translation = PcdGet64 (PcdPciIoTranslation);
  if (Translation == 0) {
    ASSERT (FALSE);
    return FALSE;
  }

  MmioAddress = Translation + Port;
  ASSERT (MmioAddress >= Translation);
  ASSERT (MmioAddress <= MAX_UINTN);

  *Address = (UINTN)MmioAddress;
  return TRUE;
}

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
  IN      UINTN  Port
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return 0;
  }

  return MmioRead8 (Address);
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
  IN      UINTN  Port,
  IN      UINT8  Value
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return Value;
  }

  return MmioWrite8 (Address, Value);
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
  IN      UINTN  Port
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return 0;
  }

  return MmioRead16 (Address);
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
  IN      UINTN   Port,
  IN      UINT16  Value
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return Value;
  }

  return MmioWrite16 (Address, Value);
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
  IN      UINTN  Port
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return 0;
  }

  return MmioRead32 (Address);
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
  IN      UINTN   Port,
  IN      UINT32  Value
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return Value;
  }

  return MmioWrite32 (Address, Value);
}

/**
  Reads a 64-bit I/O port.

  Reads the 64-bit I/O port specified by Port. The 64-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().
  If Port is not aligned on a 64-bit boundary, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT64
EFIAPI
IoRead64 (
  IN      UINTN  Port
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return 0;
  }

  return MmioRead64 (Address);
}

/**
  Writes a 64-bit I/O port.

  Writes the 64-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().
  If Port is not aligned on a 64-bit boundary, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written to the I/O port.

**/
UINT64
EFIAPI
IoWrite64 (
  IN      UINTN   Port,
  IN      UINT64  Value
  )
{
  UINTN  Address;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return 0;
  }

  return MmioWrite64 (Address, Value);
}

/**
  Reads an 8-bit I/O port fifo into a block of memory.

  Reads the 8-bit I/O fifo port specified by Port.
  The port is read Count times, and the read data is
  stored in the provided Buffer.

  This function must guarantee that all I/O read and write operations are
  serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to read.
  @param  Count   The number of times to read I/O port.
  @param  Buffer  The buffer to store the read data into.

**/
VOID
EFIAPI
IoReadFifo8 (
  IN      UINTN  Port,
  IN      UINTN  Count,
  OUT     VOID   *Buffer
  )
{
  UINTN  Address;
  UINT8  *Buffer8;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return;
  }

  Buffer8 = Buffer;
  while (Count-- > 0) {
    *Buffer8++ = MmioRead8 (Address);
  }
}

/**
  Writes a block of memory into an 8-bit I/O port fifo.

  Writes the 8-bit I/O fifo port specified by Port.
  The port is written Count times, and the write data is
  retrieved from the provided Buffer.

  This function must guarantee that all I/O write and write operations are
  serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to write.
  @param  Count   The number of times to write I/O port.
  @param  Buffer  The buffer to retrieve the write data from.

**/
VOID
EFIAPI
IoWriteFifo8 (
  IN      UINTN  Port,
  IN      UINTN  Count,
  IN      VOID   *Buffer
  )
{
  UINTN  Address;
  UINT8  *Buffer8;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return;
  }

  Buffer8 = (UINT8 *)Buffer;
  while (Count-- > 0) {
    MmioWrite8 (Address, *Buffer8++);
  }
}

/**
  Reads a 16-bit I/O port fifo into a block of memory.

  Reads the 16-bit I/O fifo port specified by Port.
  The port is read Count times, and the read data is
  stored in the provided Buffer.

  This function must guarantee that all I/O read and write operations are
  serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to read.
  @param  Count   The number of times to read I/O port.
  @param  Buffer  The buffer to store the read data into.

**/
VOID
EFIAPI
IoReadFifo16 (
  IN      UINTN  Port,
  IN      UINTN  Count,
  OUT     VOID   *Buffer
  )
{
  UINTN   Address;
  UINT16  *Buffer16;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return;
  }

  Buffer16 = Buffer;
  while (Count-- > 0) {
    *Buffer16++ = MmioRead16 (Address);
  }
}

/**
  Writes a block of memory into a 16-bit I/O port fifo.

  Writes the 16-bit I/O fifo port specified by Port.
  The port is written Count times, and the write data is
  retrieved from the provided Buffer.

  This function must guarantee that all I/O write and write operations are
  serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to write.
  @param  Count   The number of times to write I/O port.
  @param  Buffer  The buffer to retrieve the write data from.

**/
VOID
EFIAPI
IoWriteFifo16 (
  IN      UINTN  Port,
  IN      UINTN  Count,
  IN      VOID   *Buffer
  )
{
  UINTN   Address;
  UINT16  *Buffer16;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return;
  }

  Buffer16 = (UINT16 *)Buffer;
  while (Count-- > 0) {
    MmioWrite16 (Address, *Buffer16++);
  }
}

/**
  Reads a 32-bit I/O port fifo into a block of memory.

  Reads the 32-bit I/O fifo port specified by Port.
  The port is read Count times, and the read data is
  stored in the provided Buffer.

  This function must guarantee that all I/O read and write operations are
  serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to read.
  @param  Count   The number of times to read I/O port.
  @param  Buffer  The buffer to store the read data into.

**/
VOID
EFIAPI
IoReadFifo32 (
  IN      UINTN  Port,
  IN      UINTN  Count,
  OUT     VOID   *Buffer
  )
{
  UINTN   Address;
  UINT32  *Buffer32;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return;
  }

  Buffer32 = Buffer;
  while (Count-- > 0) {
    *Buffer32++ = MmioRead32 (Address);
  }
}

/**
  Writes a block of memory into a 32-bit I/O port fifo.

  Writes the 32-bit I/O fifo port specified by Port.
  The port is written Count times, and the write data is
  retrieved from the provided Buffer.

  This function must guarantee that all I/O write and write operations are
  serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().

  @param  Port    The I/O port to write.
  @param  Count   The number of times to write I/O port.
  @param  Buffer  The buffer to retrieve the write data from.

**/
VOID
EFIAPI
IoWriteFifo32 (
  IN      UINTN  Port,
  IN      UINTN  Count,
  IN      VOID   *Buffer
  )
{
  UINTN   Address;
  UINT32  *Buffer32;

  if (!IoPortToMmioAddress (Port, &Address)) {
    return;
  }

  Buffer32 = (UINT32 *)Buffer;
  while (Count-- > 0) {
    MmioWrite32 (Address, *Buffer32++);
  }
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
  IN      UINTN  Address
  )
{
  UINT8    Value;
  BOOLEAN  Flag;

  Flag = FilterBeforeMmIoRead (FilterWidth8, Address, &Value);
  if (Flag) {
    Value = *(volatile UINT8 *)Address;
  }

  FilterAfterMmIoRead (FilterWidth8, Address, &Value);

  return Value;
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
  IN      UINTN  Address,
  IN      UINT8  Value
  )
{
  BOOLEAN  Flag;

  Flag = FilterBeforeMmIoWrite (FilterWidth8, Address, &Value);
  if (Flag) {
    *(volatile UINT8 *)Address = Value;
  }

  FilterAfterMmIoWrite (FilterWidth8, Address, &Value);

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
  IN      UINTN  Address
  )
{
  UINT16   Value;
  BOOLEAN  Flag;

  ASSERT ((Address & 1) == 0);

  Flag = FilterBeforeMmIoRead (FilterWidth16, Address, &Value);
  if (Flag) {
    Value = *(volatile UINT16 *)Address;
  }

  FilterAfterMmIoRead (FilterWidth16, Address, &Value);

  return Value;
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
  IN      UINTN   Address,
  IN      UINT16  Value
  )
{
  BOOLEAN  Flag;

  ASSERT ((Address & 1) == 0);

  Flag = FilterBeforeMmIoWrite (FilterWidth16, Address, &Value);
  if (Flag) {
    *(volatile UINT16 *)Address = Value;
  }

  FilterAfterMmIoWrite (FilterWidth16, Address, &Value);

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
  IN      UINTN  Address
  )
{
  UINT32   Value;
  BOOLEAN  Flag;

  ASSERT ((Address & 3) == 0);

  Flag = FilterBeforeMmIoRead (FilterWidth32, Address, &Value);
  if (Flag) {
    Value = *(volatile UINT32 *)Address;
  }

  FilterAfterMmIoRead (FilterWidth32, Address, &Value);

  return Value;
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
  IN      UINTN   Address,
  IN      UINT32  Value
  )
{
  BOOLEAN  Flag;

  ASSERT ((Address & 3) == 0);

  Flag = FilterBeforeMmIoWrite (FilterWidth32, Address, &Value);
  if (Flag) {
    *(volatile UINT32 *)Address = Value;
  }

  FilterAfterMmIoWrite (FilterWidth32, Address, &Value);

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
  IN      UINTN  Address
  )
{
  UINT64   Value;
  BOOLEAN  Flag;

  ASSERT ((Address & 7) == 0);

  Flag = FilterBeforeMmIoRead (FilterWidth64, Address, &Value);
  if (Flag) {
    Value = *(volatile UINT64 *)Address;
  }

  FilterAfterMmIoRead (FilterWidth64, Address, &Value);

  return Value;
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
  IN      UINTN   Address,
  IN      UINT64  Value
  )
{
  BOOLEAN  Flag;

  ASSERT ((Address & 7) == 0);

  Flag = FilterBeforeMmIoWrite (FilterWidth64, Address, &Value);
  if (Flag) {
    *(volatile UINT64 *)Address = Value;
  }

  FilterAfterMmIoWrite (FilterWidth64, Address, &Value);

  return Value;
}
