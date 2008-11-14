/** @file
  Provides services to access PCI Configuration Space on a platform with multiple PCI segments.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PCI_SEGMENT_LIB__
#define __PCI_SEGMENT_LIB__


/**
  Macro that converts PCI Segment, PCI Bus, PCI Device, PCI Function,
  and PCI Register to an address that can be passed to the PCI Segment Library functions.

  Computes an address that is compatible with the PCI Segment Library functions.
  The unused upper bits of Segment, Bus, Device, Function,
  and Register are stripped prior to the generation of the address.

  @param  Segment   PCI Segment number.  Range 0..65535.
  @param  Bus       PCI Bus number.  Range 0..255.
  @param  Device    PCI Device number.  Range 0..31.
  @param  Function  PCI Function number.  Range 0..7.
  @param  Register  PCI Register number.  Range 0..255 for PCI.  Range 0..4095 for PCI Express.

  @return The address that is compatible with the PCI Segment Library functions.

**/
#define PCI_SEGMENT_LIB_ADDRESS(Segment,Bus,Device,Function,Register) \
  ( ((Register) & 0xfff)              | \
    (((Function) & 0x07) << 12)       | \
    (((Device) & 0x1f) << 15)         | \
    (((Bus) & 0xff) << 20)            | \
    (LShiftU64((Segment) & 0xffff, 32)) \
  )

/**
  Reads an 8-bit PCI configuration register.

  Reads and returns the 8-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().
  
  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.

  @return The 8-bit PCI configuration register specified by Address.

**/
UINT8
EFIAPI
PciSegmentRead8 (
  IN UINT64                    Address
  );

/**
  Writes an 8-bit PCI configuration register.

  Writes the 8-bit PCI configuration register specified by Address with the value specified by Value.
  Value is returned.  This function must guarantee that all PCI read and write operations are serialized.
  If Address > 0x0FFFFFFF, then ASSERT().

  @param  Address     Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  Value       The value to write.

  @return The parameter of Value.

**/
UINT8
EFIAPI
PciSegmentWrite8 (
  IN UINT64                    Address,
  IN UINT8                     Value
  );

/**
  Performs a bitwise inclusive OR of an 8-bit PCI configuration register with an 8-bit value.

  Reads the 8-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 8-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT8
EFIAPI
PciSegmentOr8 (
  IN UINT64                    Address,
  IN UINT8                     OrData
  );

/**
  Performs a bitwise AND of an 8-bit PCI configuration register with an 8-bit value.

  Reads the 8-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  and writes the result to the 8-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  AndData   The value to AND with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT8
EFIAPI
PciSegmentAnd8 (
  IN UINT64                    Address,
  IN UINT8                     AndData
  );

/**
  Performs a bitwise AND of an 8-bit PCI configuration register with an 8-bit value,
  followed a  bitwise inclusive OR with another 8-bit value.
  
  Reads the 8-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  performs a bitwise inclusive OR between the result of the AND operation and the value specified by OrData,
  and writes the result to the 8-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  AndData    The value to AND with the PCI configuration register.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT8
EFIAPI
PciSegmentAndThenOr8 (
  IN UINT64                    Address,
  IN UINT8                     AndData,
  IN UINT8                     OrData
  );

/**
  Reads a bit field of a PCI configuration register.

  Reads the bit field in an 8-bit PCI configuration register.
  The bit field is specified by the StartBit and the EndBit.
  The value of the bit field is returned.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.

  @return The value of the bit field.

**/
UINT8
EFIAPI
PciSegmentBitFieldRead8 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit
  );

/**
  Writes a bit field to a PCI configuration register.

  Writes Value to the bit field of the PCI configuration register.
  The bit field is specified by the StartBit and the EndBit.
  All other bits in the destination PCI configuration register are preserved.
  The new value of the 8-bit register is returned.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  Value     New value of the bit field.

  @return The new value of the 8-bit register.

**/
UINT8
EFIAPI
PciSegmentBitFieldWrite8 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT8                     Value
  );

/**
  Reads the 8-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 8-bit PCI configuration register specified by Address. 

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  OrData    The value to OR with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT8
EFIAPI
PciSegmentBitFieldOr8 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT8                     OrData
  );

/**
  Reads a bit field in an 8-bit PCI configuration, performs a bitwise OR,
  and writes the result back to the bit field in the 8-bit port.

  Reads the 8-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 8-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  Extra left bits in OrData are stripped.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  AndData   The value to AND with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT8
EFIAPI
PciSegmentBitFieldAnd8 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT8                     AndData
  );

/**
  Reads a bit field in an 8-bit PCI configuration register, performs a bitwise AND,
  and writes the result back to the bit field in the 8-bit register.
 
  Reads the 8-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  and writes the result to the 8-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  Extra left bits in AndData are stripped.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  AndData   The value to AND with the read value from the PCI configuration register.
  @param  OrData    The value to OR with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT8
EFIAPI
PciSegmentBitFieldAndThenOr8 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT8                     AndData,
  IN UINT8                     OrData
  );

/**
  Reads a 16-bit PCI configuration register.

  Reads and returns the 16-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().
  
  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.

  @return The 16-bit PCI configuration register specified by Address.

**/
UINT16
EFIAPI
PciSegmentRead16 (
  IN UINT64                    Address
  );

/**
  Writes a 16-bit PCI configuration register.

  Writes the 16-bit PCI configuration register specified by Address with the value specified by Value.
  Value is returned.  This function must guarantee that all PCI read and write operations are serialized.
  If Address > 0x0FFFFFFF, then ASSERT().

  @param  Address     Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  Value       The value to write.

  @return The parameter of Value.

**/
UINT16
EFIAPI
PciSegmentWrite16 (
  IN UINT64                    Address,
  IN UINT16                    Value
  );

/**
  Performs a bitwise inclusive OR of a 16-bit PCI configuration register with a 16-bit value.

  Reads the 16-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 16-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT16
EFIAPI
PciSegmentOr16 (
  IN UINT64                    Address,
  IN UINT16                    OrData
  );

/**
  Performs a bitwise AND of a 16-bit PCI configuration register with a 16-bit value.

  Reads the 16-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  and writes the result to the 16-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  AndData   The value to AND with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT16
EFIAPI
PciSegmentAnd16 (
  IN UINT64                    Address,
  IN UINT16                    AndData
  );

/**
  Performs a bitwise AND of a 16-bit PCI configuration register with a 16-bit value,
  followed a  bitwise inclusive OR with another 16-bit value.
  
  Reads the 16-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  performs a bitwise inclusive OR between the result of the AND operation and the value specified by OrData,
  and writes the result to the 16-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  AndData    The value to AND with the PCI configuration register.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT16
EFIAPI
PciSegmentAndThenOr16 (
  IN UINT64                    Address,
  IN UINT16                    AndData,
  IN UINT16                    OrData
  );

/**
  Reads a bit field of a PCI configuration register.

  Reads the bit field in a 16-bit PCI configuration register.
  The bit field is specified by the StartBit and the EndBit.
  The value of the bit field is returned.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.

  @return The value of the bit field.

**/
UINT16
EFIAPI
PciSegmentBitFieldRead16 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit
  );

/**
  Writes a bit field to a PCI configuration register.

  Writes Value to the bit field of the PCI configuration register.
  The bit field is specified by the StartBit and the EndBit.
  All other bits in the destination PCI configuration register are preserved.
  The new value of the 16-bit register is returned.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  Value     New value of the bit field.

  @return The new value of the 16-bit register.

**/
UINT16
EFIAPI
PciSegmentBitFieldWrite16 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT16                    Value
  );

/**
  Reads the 16-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 16-bit PCI configuration register specified by Address. 

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  OrData    The value to OR with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT16
EFIAPI
PciSegmentBitFieldOr16 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT16                    OrData
  );

/**
  Reads a bit field in a 16-bit PCI configuration, performs a bitwise OR,
  and writes the result back to the bit field in the 16-bit port.

  Reads the 16-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 16-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  Extra left bits in OrData are stripped.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  AndData   The value to AND with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT16
EFIAPI
PciSegmentBitFieldAnd16 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT16                    AndData
  );

/**
  Reads a bit field in a 16-bit PCI configuration register, performs a bitwise AND,
  and writes the result back to the bit field in the 16-bit register.
 
  Reads the 16-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  and writes the result to the 16-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  Extra left bits in AndData are stripped.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  AndData   The value to AND with the read value from the PCI configuration register.
  @param  OrData    The value to OR with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT16
EFIAPI
PciSegmentBitFieldAndThenOr16 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT16                    AndData,
  IN UINT16                    OrData
  );

/**
  Reads a 32-bit PCI configuration register.

  Reads and returns the 32-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().
  
  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.

  @return The 32-bit PCI configuration register specified by Address.

**/
UINT32
EFIAPI
PciSegmentRead32 (
  IN UINT64                    Address
  );

/**
  Writes a 32-bit PCI configuration register.

  Writes the 32-bit PCI configuration register specified by Address with the value specified by Value.
  Value is returned.  This function must guarantee that all PCI read and write operations are serialized.
  If Address > 0x0FFFFFFF, then ASSERT().

  @param  Address     Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  Value       The value to write.

  @return The parameter of Value.

**/
UINT32
EFIAPI
PciSegmentWrite32 (
  IN UINT64                    Address,
  IN UINT32                    Value
  );

/**
  Performs a bitwise inclusive OR of a 32-bit PCI configuration register with a 32-bit value.

  Reads the 32-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 32-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT32
EFIAPI
PciSegmentOr32 (
  IN UINT64                    Address,
  IN UINT32                    OrData
  );

/**
  Performs a bitwise AND of a 32-bit PCI configuration register with a 32-bit value.

  Reads the 32-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  and writes the result to the 32-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  AndData   The value to AND with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT32
EFIAPI
PciSegmentAnd32 (
  IN UINT64                    Address,
  IN UINT32                    AndData
  );

/**
  Performs a bitwise AND of a 32-bit PCI configuration register with a 32-bit value,
  followed a  bitwise inclusive OR with another 32-bit value.
  
  Reads the 32-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  performs a bitwise inclusive OR between the result of the AND operation and the value specified by OrData,
  and writes the result to the 32-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  If any reserved bits in Address are set, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  AndData   The value to AND with the PCI configuration register.
  @param  OrData    The value to OR with the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT32
EFIAPI
PciSegmentAndThenOr32 (
  IN UINT64                    Address,
  IN UINT32                    AndData,
  IN UINT32                    OrData
  );

/**
  Reads a bit field of a PCI configuration register.

  Reads the bit field in a 32-bit PCI configuration register.
  The bit field is specified by the StartBit and the EndBit.
  The value of the bit field is returned.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.

  @return The value of the bit field.

**/
UINT32
EFIAPI
PciSegmentBitFieldRead32 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit
  );

/**
  Writes a bit field to a PCI configuration register.

  Writes Value to the bit field of the PCI configuration register.
  The bit field is specified by the StartBit and the EndBit.
  All other bits in the destination PCI configuration register are preserved.
  The new value of the 32-bit register is returned.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  Value     New value of the bit field.

  @return The new value of the 32-bit register.

**/
UINT32
EFIAPI
PciSegmentBitFieldWrite32 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT32                    Value
  );

/**
  Reads the 32-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 32-bit PCI configuration register specified by Address. 

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  OrData    The value to OR with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT32
EFIAPI
PciSegmentBitFieldOr32 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT32                    OrData
  );

/**
  Reads a bit field in a 32-bit PCI configuration, performs a bitwise OR,
  and writes the result back to the bit field in the 32-bit port.

  Reads the 32-bit PCI configuration register specified by Address,
  performs a bitwise inclusive OR between the read result and the value specified by OrData,
  and writes the result to the 32-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  Extra left bits in OrData are stripped.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  AndData   The value to AND with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT32
EFIAPI
PciSegmentBitFieldAnd32 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT32                    AndData
  );

/**
  Reads a bit field in a 32-bit PCI configuration register, performs a bitwise AND,
  and writes the result back to the bit field in the 32-bit register.
 
  Reads the 32-bit PCI configuration register specified by Address,
  performs a bitwise AND between the read result and the value specified by AndData,
  and writes the result to the 32-bit PCI configuration register specified by Address.
  The value written to the PCI configuration register is returned.
  This function must guarantee that all PCI read and write operations are serialized.
  Extra left bits in AndData are stripped.
  If any reserved bits in Address are set, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Address   Address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    The ordinal of the least significant bit in a byte is bit 0.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    The ordinal of the most significant bit in a byte is bit 7.
  @param  AndData   The value to AND with the read value from the PCI configuration register.
  @param  OrData    The value to OR with the read value from the PCI configuration register.

  @return The value written to the PCI configuration register.

**/
UINT32
EFIAPI
PciSegmentBitFieldAndThenOr32 (
  IN UINT64                    Address,
  IN UINTN                     StartBit,
  IN UINTN                     EndBit,
  IN UINT32                    AndData,
  IN UINT32                    OrData
  );

/**
  Reads a range of PCI configuration registers into a caller supplied buffer.

  Reads the range of PCI configuration registers specified by StartAddress
  and Size into the buffer specified by Buffer.
  This function only allows the PCI configuration registers from a single PCI function to be read.
  Size is returned.
  If any reserved bits in StartAddress are set, then ASSERT().
  If ((StartAddress & 0xFFF) + Size) > 0x1000, then ASSERT().
  If (StartAddress + Size - 1)  > 0x0FFFFFFF, then ASSERT().
  If Buffer is NULL, then ASSERT().

  @param  StartAddress  Starting address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  Size          Size in bytes of the transfer.
  @param  Buffer        Pointer to a buffer receiving the data read.

  @return The paramter of Size.

**/
UINTN
EFIAPI
PciSegmentReadBuffer (
  IN  UINT64                   StartAddress,
  IN  UINTN                    Size,
  OUT VOID                     *Buffer
  );

/**
  Copies the data in a caller supplied buffer to a specified range of PCI configuration space.

  Writes the range of PCI configuration registers specified by StartAddress
  and Size from the buffer specified by Buffer.
  This function only allows the PCI configuration registers from a single PCI function to be written.
  Size is returned.
  If any reserved bits in StartAddress are set, then ASSERT().
  If ((StartAddress & 0xFFF) + Size) > 0x1000, then ASSERT().
  If (StartAddress + Size - 1)  > 0x0FFFFFFF, then ASSERT().
  If Buffer is NULL, then ASSERT().

  @param  StartAddress  Starting address that encodes the PCI Segment, Bus, Device, Function, and Register.
  @param  Size          Size in bytes of the transfer.
  @param  Buffer        Pointer to a buffer containing the data to write.

  @return The paramter of Size.

**/
UINTN
EFIAPI
PciSegmentWriteBuffer (
  IN UINT64                    StartAddress,
  IN UINTN                     Size,
  IN VOID                      *Buffer
  );

#endif
