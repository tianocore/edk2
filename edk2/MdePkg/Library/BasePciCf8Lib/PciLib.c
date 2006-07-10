/** @file
  PCI Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PciLib.c

**/

//
// Declare I/O Ports used to perform PCI Confguration Cycles
//
#define PCI_CONFIGURATION_ADDRESS_PORT  0xCF8
#define PCI_CONFIGURATION_DATA_PORT     0xCFC

//
// Declare macro to convert PCI Library formatted address to CF8 formatted address
//
// PCI Library formatted address    CF8 Formatted Address
// =============================    ======================
//    Bits 00..11  Register           Bits 00..07  Register
//    Bits 12..14  Function           Bits 08..10  Function
//    Bits 15..19  Device             Bits 11..15  Device
//    Bits 20..27  Bus                Bits 16..23  Bus
//    Bits 28..31  Reserved(MBZ)      Bits 24..30  Reserved(MBZ)
//                                    Bits 31..31  Must be 1
//

/**
  Assert the validity of a PCI address. A valid PCI address should contain 1's
  only in the low 28 bits.

  @param  A The address to validate.
  @param  M Additional bits to assert to be zero.

**/
#define ASSERT_INVALID_PCI_ADDRESS(A,M) \
  ASSERT (((A) & (~0xffff0ff | (M))) == 0)

/**
  Convert a PCI Express address to PCI CF8 address.

  @param  A The address to convert.

  @retval The coverted address.

**/
#define PCI_TO_CF8_ADDRESS(A) \
  ((UINT32) ((((A) >> 4) & 0x00ffff00) | ((A) & 0xfc) | 0x80000000))

/**
  Reads an 8-bit PCI configuration register.

  Reads and returns the 8-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.

  @return The read value from the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8Read8 (
  IN      UINTN                     Address
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoRead8 (PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3));
}

/**
  Writes an 8-bit PCI configuration register.

  Writes the 8-bit PCI configuration register specified by Address with the
  value specified by Value. Value is returned. This function must guarantee
  that all PCI read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  Value   The value to write.

  @return The value written to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8Write8 (
  IN      UINTN                     Address,
  IN      UINT8                     Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoWrite8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           Value
           );
}

/**
  Performs a bitwise inclusive OR of an 8-bit PCI configuration register with
  an 8-bit value.

  Reads the 8-bit PCI configuration register specified by Address, performs a
  bitwise inclusive OR between the read result and the value specified by
  OrData, and writes the result to the 8-bit PCI configuration register
  specified by Address. The value written to the PCI configuration register is
  returned. This function must guarantee that all PCI read and write operations
  are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  OrData  The value to OR with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8Or8 (
  IN      UINTN                     Address,
  IN      UINT8                     OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoOr8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           OrData
           );
}

/**
  Performs a bitwise AND of an 8-bit PCI configuration register with an 8-bit
  value.

  Reads the 8-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData, and
  writes the result to the 8-bit PCI configuration register specified by
  Address. The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  AndData The value to AND with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8And8 (
  IN      UINTN                     Address,
  IN      UINT8                     AndData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoAnd8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           AndData
           );
}

/**
  Performs a bitwise AND of an 8-bit PCI configuration register with an 8-bit
  value, followed a  bitwise inclusive OR with another 8-bit value.

  Reads the 8-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData,
  performs a bitwise inclusive OR between the result of the AND operation and
  the value specified by OrData, and writes the result to the 8-bit PCI
  configuration register specified by Address. The value written to the PCI
  configuration register is returned. This function must guarantee that all PCI
  read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  AndData The value to AND with the PCI configuration register.
  @param  OrData  The value to OR with the result of the AND operation.

  @return The value written back to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8AndThenOr8 (
  IN      UINTN                     Address,
  IN      UINT8                     AndData,
  IN      UINT8                     OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoAndThenOr8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           AndData,
           OrData
           );
}

/**
  Reads a bit field of a PCI configuration register.

  Reads the bit field in an 8-bit PCI configuration register. The bit field is
  specified by the StartBit and the EndBit. The value of the bit field is
  returned.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to read.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.

  @return The value of the bit field read from the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8BitFieldRead8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldRead8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           StartBit,
           EndBit
           );
}

/**
  Writes a bit field to a PCI configuration register.

  Writes Value to the bit field of the PCI configuration register. The bit
  field is specified by the StartBit and the EndBit. All other bits in the
  destination PCI configuration register are preserved. The new value of the
  8-bit register is returned.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  Value     New value of the bit field.

  @return The value written back to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8BitFieldWrite8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldWrite8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           StartBit,
           EndBit,
           Value
           );
}

/**
  Reads a bit field in an 8-bit PCI configuration, performs a bitwise OR, and
  writes the result back to the bit field in the 8-bit port.

  Reads the 8-bit PCI configuration register specified by Address, performs a
  bitwise inclusive OR between the read result and the value specified by
  OrData, and writes the result to the 8-bit PCI configuration register
  specified by Address. The value written to the PCI configuration register is
  returned. This function must guarantee that all PCI read and write operations
  are serialized. Extra left bits in OrData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8BitFieldOr8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldOr8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           StartBit,
           EndBit,
           OrData
           );
}

/**
  Reads a bit field in an 8-bit PCI configuration register, performs a bitwise
  AND, and writes the result back to the bit field in the 8-bit register.

  Reads the 8-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData, and
  writes the result to the 8-bit PCI configuration register specified by
  Address. The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are
  serialized. Extra left bits in AndData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  AndData   The value to AND with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8BitFieldAnd8 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldAnd8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           StartBit,
           EndBit,
           AndData
           );
}

/**
  Reads a bit field in an 8-bit port, performs a bitwise AND followed by a
  bitwise inclusive OR, and writes the result back to the bit field in the
  8-bit port.

  Reads the 8-bit PCI configuration register specified by Address, performs a
  bitwise AND followed by a bitwise inclusive OR between the read result and
  the value specified by AndData, and writes the result to the 8-bit PCI
  configuration register specified by Address. The value written to the PCI
  configuration register is returned. This function must guarantee that all PCI
  read and write operations are serialized. Extra left bits in both AndData and
  OrData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  AndData   The value to AND with the PCI configuration register.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The value written back to the PCI configuration register.

**/
UINT8
EFIAPI
PciCf8BitFieldAndThenOr8(
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData,
  IN      UINT8                     OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 0);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldAndThenOr8 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
           StartBit,
           EndBit,
           AndData,
           OrData
           );
}

/**
  Reads a 16-bit PCI configuration register.

  Reads and returns the 16-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.

  @return The read value from the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8Read16 (
  IN      UINTN                     Address
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoRead16 (PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2));
}

/**
  Writes a 16-bit PCI configuration register.

  Writes the 16-bit PCI configuration register specified by Address with the
  value specified by Value. Value is returned. This function must guarantee
  that all PCI read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  Value   The value to write.

  @return The value written to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8Write16 (
  IN      UINTN                     Address,
  IN      UINT16                    Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoWrite16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           Value
           );
}

/**
  Performs a bitwise inclusive OR of a 16-bit PCI configuration register with
  a 16-bit value.

  Reads the 16-bit PCI configuration register specified by Address, performs a
  bitwise inclusive OR between the read result and the value specified by
  OrData, and writes the result to the 16-bit PCI configuration register
  specified by Address. The value written to the PCI configuration register is
  returned. This function must guarantee that all PCI read and write operations
  are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  OrData  The value to OR with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8Or16 (
  IN      UINTN                     Address,
  IN      UINT16                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoOr16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           OrData
           );
}

/**
  Performs a bitwise AND of a 16-bit PCI configuration register with a 16-bit
  value.

  Reads the 16-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData, and
  writes the result to the 16-bit PCI configuration register specified by
  Address. The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  AndData The value to AND with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8And16 (
  IN      UINTN                     Address,
  IN      UINT16                    AndData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoAnd16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           AndData
           );
}

/**
  Performs a bitwise AND of a 16-bit PCI configuration register with a 16-bit
  value, followed a  bitwise inclusive OR with another 16-bit value.

  Reads the 16-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData,
  performs a bitwise inclusive OR between the result of the AND operation and
  the value specified by OrData, and writes the result to the 16-bit PCI
  configuration register specified by Address. The value written to the PCI
  configuration register is returned. This function must guarantee that all PCI
  read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  AndData The value to AND with the PCI configuration register.
  @param  OrData  The value to OR with the result of the AND operation.

  @return The value written back to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8AndThenOr16 (
  IN      UINTN                     Address,
  IN      UINT16                    AndData,
  IN      UINT16                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoAndThenOr16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           AndData,
           OrData
           );
}

/**
  Reads a bit field of a PCI configuration register.

  Reads the bit field in a 16-bit PCI configuration register. The bit field is
  specified by the StartBit and the EndBit. The value of the bit field is
  returned.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to read.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.

  @return The value of the bit field read from the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8BitFieldRead16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldRead16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           StartBit,
           EndBit
           );
}

/**
  Writes a bit field to a PCI configuration register.

  Writes Value to the bit field of the PCI configuration register. The bit
  field is specified by the StartBit and the EndBit. All other bits in the
  destination PCI configuration register are preserved. The new value of the
  16-bit register is returned.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  Value     New value of the bit field.

  @return The value written back to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8BitFieldWrite16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldWrite16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           StartBit,
           EndBit,
           Value
           );
}

/**
  Reads a bit field in a 16-bit PCI configuration, performs a bitwise OR, and
  writes the result back to the bit field in the 16-bit port.

  Reads the 16-bit PCI configuration register specified by Address, performs a
  bitwise inclusive OR between the read result and the value specified by
  OrData, and writes the result to the 16-bit PCI configuration register
  specified by Address. The value written to the PCI configuration register is
  returned. This function must guarantee that all PCI read and write operations
  are serialized. Extra left bits in OrData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8BitFieldOr16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldOr16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           StartBit,
           EndBit,
           OrData
           );
}

/**
  Reads a bit field in a 16-bit PCI configuration register, performs a bitwise
  AND, and writes the result back to the bit field in the 16-bit register.

  Reads the 16-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData, and
  writes the result to the 16-bit PCI configuration register specified by
  Address. The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are
  serialized. Extra left bits in AndData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  AndData   The value to AND with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8BitFieldAnd16 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldAnd16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           StartBit,
           EndBit,
           AndData
           );
}

/**
  Reads a bit field in a 16-bit port, performs a bitwise AND followed by a
  bitwise inclusive OR, and writes the result back to the bit field in the
  16-bit port.

  Reads the 16-bit PCI configuration register specified by Address, performs a
  bitwise AND followed by a bitwise inclusive OR between the read result and
  the value specified by AndData, and writes the result to the 16-bit PCI
  configuration register specified by Address. The value written to the PCI
  configuration register is returned. This function must guarantee that all PCI
  read and write operations are serialized. Extra left bits in both AndData and
  OrData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  AndData   The value to AND with the PCI configuration register.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The value written back to the PCI configuration register.

**/
UINT16
EFIAPI
PciCf8BitFieldAndThenOr16(
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData,
  IN      UINT16                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 1);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldAndThenOr16 (
           PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
           StartBit,
           EndBit,
           AndData,
           OrData
           );
}

/**
  Reads a 32-bit PCI configuration register.

  Reads and returns the 32-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.

  @return The read value from the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8Read32 (
  IN      UINTN                     Address
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoRead32 (PCI_CONFIGURATION_DATA_PORT);
}

/**
  Writes a 32-bit PCI configuration register.

  Writes the 32-bit PCI configuration register specified by Address with the
  value specified by Value. Value is returned. This function must guarantee
  that all PCI read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  Value   The value to write.

  @return The value written to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8Write32 (
  IN      UINTN                     Address,
  IN      UINT32                    Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoWrite32 (
           PCI_CONFIGURATION_DATA_PORT,
           Value
           );
}

/**
  Performs a bitwise inclusive OR of a 32-bit PCI configuration register with
  a 32-bit value.

  Reads the 32-bit PCI configuration register specified by Address, performs a
  bitwise inclusive OR between the read result and the value specified by
  OrData, and writes the result to the 32-bit PCI configuration register
  specified by Address. The value written to the PCI configuration register is
  returned. This function must guarantee that all PCI read and write operations
  are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  OrData  The value to OR with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8Or32 (
  IN      UINTN                     Address,
  IN      UINT32                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoOr32 (
           PCI_CONFIGURATION_DATA_PORT,
           OrData
           );
}

/**
  Performs a bitwise AND of a 32-bit PCI configuration register with a 32-bit
  value.

  Reads the 32-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData, and
  writes the result to the 32-bit PCI configuration register specified by
  Address. The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  AndData The value to AND with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8And32 (
  IN      UINTN                     Address,
  IN      UINT32                    AndData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoAnd32 (
           PCI_CONFIGURATION_DATA_PORT,
           AndData
           );
}

/**
  Performs a bitwise AND of a 32-bit PCI configuration register with a 32-bit
  value, followed a  bitwise inclusive OR with another 32-bit value.

  Reads the 32-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData,
  performs a bitwise inclusive OR between the result of the AND operation and
  the value specified by OrData, and writes the result to the 32-bit PCI
  configuration register specified by Address. The value written to the PCI
  configuration register is returned. This function must guarantee that all PCI
  read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address Address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  AndData The value to AND with the PCI configuration register.
  @param  OrData  The value to OR with the result of the AND operation.

  @return The value written back to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8AndThenOr32 (
  IN      UINTN                     Address,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoAndThenOr32 (
           PCI_CONFIGURATION_DATA_PORT,
           AndData,
           OrData
           );
}

/**
  Reads a bit field of a PCI configuration register.

  Reads the bit field in a 32-bit PCI configuration register. The bit field is
  specified by the StartBit and the EndBit. The value of the bit field is
  returned.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to read.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.

  @return The value of the bit field read from the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8BitFieldRead32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldRead32 (
           PCI_CONFIGURATION_DATA_PORT,
           StartBit,
           EndBit
           );
}

/**
  Writes a bit field to a PCI configuration register.

  Writes Value to the bit field of the PCI configuration register. The bit
  field is specified by the StartBit and the EndBit. All other bits in the
  destination PCI configuration register are preserved. The new value of the
  32-bit register is returned.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  Value     New value of the bit field.

  @return The value written back to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8BitFieldWrite32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    Value
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldWrite32 (
           PCI_CONFIGURATION_DATA_PORT,
           StartBit,
           EndBit,
           Value
           );
}

/**
  Reads a bit field in a 32-bit PCI configuration, performs a bitwise OR, and
  writes the result back to the bit field in the 32-bit port.

  Reads the 32-bit PCI configuration register specified by Address, performs a
  bitwise inclusive OR between the read result and the value specified by
  OrData, and writes the result to the 32-bit PCI configuration register
  specified by Address. The value written to the PCI configuration register is
  returned. This function must guarantee that all PCI read and write operations
  are serialized. Extra left bits in OrData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8BitFieldOr32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldOr32 (
           PCI_CONFIGURATION_DATA_PORT,
           StartBit,
           EndBit,
           OrData
           );
}

/**
  Reads a bit field in a 32-bit PCI configuration register, performs a bitwise
  AND, and writes the result back to the bit field in the 32-bit register.

  Reads the 32-bit PCI configuration register specified by Address, performs a
  bitwise AND between the read result and the value specified by AndData, and
  writes the result to the 32-bit PCI configuration register specified by
  Address. The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are
  serialized. Extra left bits in AndData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the PCI configuration register.

  @return The value written back to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8BitFieldAnd32 (
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldAnd32 (
           PCI_CONFIGURATION_DATA_PORT,
           StartBit,
           EndBit,
           AndData
           );
}

/**
  Reads a bit field in a 32-bit port, performs a bitwise AND followed by a
  bitwise inclusive OR, and writes the result back to the bit field in the
  32-bit port.

  Reads the 32-bit PCI configuration register specified by Address, performs a
  bitwise AND followed by a bitwise inclusive OR between the read result and
  the value specified by AndData, and writes the result to the 32-bit PCI
  configuration register specified by Address. The value written to the PCI
  configuration register is returned. This function must guarantee that all PCI
  read and write operations are serialized. Extra left bits in both AndData and
  OrData are stripped.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   PCI configuration register to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the PCI configuration register.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The value written back to the PCI configuration register.

**/
UINT32
EFIAPI
PciCf8BitFieldAndThenOr32(
  IN      UINTN                     Address,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  )
{
  ASSERT_INVALID_PCI_ADDRESS (Address, 3);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  return IoBitFieldAndThenOr32 (
           PCI_CONFIGURATION_DATA_PORT,
           StartBit,
           EndBit,
           AndData,
           OrData
           );
}

/**
  Reads a range of PCI configuration registers into a caller supplied buffer.

  Reads the range of PCI configuration registers specified by StartAddress and
  Size into the buffer specified by Buffer. This function only allows the PCI
  configuration registers from a single PCI function to be read. Size is
  returned. When possible 32-bit PCI configuration read cycles are used to read
  from StartAdress to StartAddress + Size. Due to alignment restrictions, 8-bit
  and 16-bit PCI configuration read cycles may be used at the beginning and the
  end of the range.

  If StartAddress > 0x0FFFFFFF, then ASSERT().
  If the register specified by StartAddress >= 0x100, then ASSERT().
  If ((StartAddress & 0xFFF) + Size) > 0x100, then ASSERT().
  If Size > 0 and Buffer is NULL, then ASSERT().

  @param  StartAddress  Starting address that encodes the PCI Bus, Device,
                        Function and Register.
  @param  Size          Size in bytes of the transfer.
  @param  Buffer        Pointer to a buffer receiving the data read.

  @return Size

**/
UINTN
EFIAPI
PciCf8ReadBuffer (
  IN      UINTN                     StartAddress,
  IN      UINTN                     Size,
  OUT     VOID                      *Buffer
  )
{
  UINTN                             ReturnValue;

  ASSERT_INVALID_PCI_ADDRESS (StartAddress, 0);
  ASSERT (((StartAddress & 0xFFF) + Size) <= 0x100);

  if (Size == 0) {
    return Size;
  }

  ASSERT (Buffer != NULL);

  //
  // Save Size for return
  //
  ReturnValue = Size;

  if ((StartAddress & 1) != 0) {
    //
    // Read a byte if StartAddress is byte aligned
    //
    *(UINT8*)Buffer = PciCf8Read8 (StartAddress);
    StartAddress += sizeof (UINT8);
    Size -= sizeof (UINT8);
    Buffer = (UINT8*)Buffer + 1;
  }

  if (Size >= sizeof (UINT16) && (StartAddress & 2) != 0) {
    //
    // Read a word if StartAddress is word aligned
    //
    *(UINT16*)Buffer = PciCf8Read16 (StartAddress);
    StartAddress += sizeof (UINT16);
    Size -= sizeof (UINT16);
    Buffer = (UINT16*)Buffer + 1;
  }

  while (Size >= sizeof (UINT32)) {
    //
    // Read as many double words as possible
    //
    *(UINT32*)Buffer = PciCf8Read32 (StartAddress);
    StartAddress += sizeof (UINT32);
    Size -= sizeof (UINT32);
    Buffer = (UINT32*)Buffer + 1;
  }

  if (Size >= sizeof (UINT16)) {
    //
    // Read the last remaining word if exist
    //
    *(UINT16*)Buffer = PciCf8Read16 (StartAddress);
    StartAddress += sizeof (UINT16);
    Size -= sizeof (UINT16);
    Buffer = (UINT16*)Buffer + 1;
  }

  if (Size >= sizeof (UINT8)) {
    //
    // Read the last remaining byte if exist
    //
    *(UINT8*)Buffer = PciCf8Read8 (StartAddress);
  }

  return ReturnValue;
}

/**
  Copies the data in a caller supplied buffer to a specified range of PCI
  configuration space.

  Writes the range of PCI configuration registers specified by StartAddress and
  Size from the buffer specified by Buffer. This function only allows the PCI
  configuration registers from a single PCI function to be written. Size is
  returned. When possible 32-bit PCI configuration write cycles are used to
  write from StartAdress to StartAddress + Size. Due to alignment restrictions,
  8-bit and 16-bit PCI configuration write cycles may be used at the beginning
  and the end of the range.

  If StartAddress > 0x0FFFFFFF, then ASSERT().
  If the register specified by StartAddress >= 0x100, then ASSERT().
  If ((StartAddress & 0xFFF) + Size) > 0x100, then ASSERT().
  If Size > 0 and Buffer is NULL, then ASSERT().

  @param  StartAddress  Starting address that encodes the PCI Bus, Device,
                        Function and Register.
  @param  Size          Size in bytes of the transfer.
  @param  Buffer        Pointer to a buffer containing the data to write.

  @return Size

**/
UINTN
EFIAPI
PciCf8WriteBuffer (
  IN      UINTN                     StartAddress,
  IN      UINTN                     Size,
  IN      VOID                      *Buffer
  )
{
  UINTN                             ReturnValue;

  ASSERT_INVALID_PCI_ADDRESS (StartAddress, 0);
  ASSERT (((StartAddress & 0xFFF) + Size) <= 0x100);

  if (Size == 0) {
    return 0;
  }

  ASSERT (Buffer != NULL);

  //
  // Save Size for return
  //
  ReturnValue = Size;

  if ((StartAddress & 1) != 0) {
    //
    // Write a byte if StartAddress is byte aligned
    //
    PciCf8Write8 (StartAddress, *(UINT8*)Buffer);
    StartAddress += sizeof (UINT8);
    Size -= sizeof (UINT8);
    Buffer = (UINT8*)Buffer + 1;
  }

  if (Size >= sizeof (UINT16) && (StartAddress & 2) != 0) {
    //
    // Write a word if StartAddress is word aligned
    //
    PciCf8Write16 (StartAddress, *(UINT16*)Buffer);
    StartAddress += sizeof (UINT16);
    Size -= sizeof (UINT16);
    Buffer = (UINT16*)Buffer + 1;
  }

  while (Size >= sizeof (UINT32)) {
    //
    // Write as many double words as possible
    //
    PciCf8Write32 (StartAddress, *(UINT32*)Buffer);
    StartAddress += sizeof (UINT32);
    Size -= sizeof (UINT32);
    Buffer = (UINT32*)Buffer + 1;
  }

  if (Size >= sizeof (UINT16)) {
    //
    // Write the last remaining word if exist
    //
    PciCf8Write16 (StartAddress, *(UINT16*)Buffer);
    StartAddress += sizeof (UINT16);
    Size -= sizeof (UINT16);
    Buffer = (UINT16*)Buffer + 1;
  }

  if (Size >= sizeof (UINT8)) {
    //
    // Write the last remaining byte if exist
    //
    PciCf8Write8 (StartAddress, *(UINT8*)Buffer);
  }

  return ReturnValue;
}
