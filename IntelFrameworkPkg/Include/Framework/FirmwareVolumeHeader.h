/** @file
  Defines the data structure that is the volume header found at the beginning of
  all firmware volumes that are either memory mapped or have an
  associated FirmwareVolumeBlock protocol.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  These definitions are from the Firmware Volume Block Spec 0.9.

**/

#ifndef __EFI_FIRMWARE_VOLUME_HEADER_H__
#define __EFI_FIRMWARE_VOLUME_HEADER_H__

///
/// Firmware Volume Block Attributes bit definitions.
///@{
#define EFI_FVB_READ_DISABLED_CAP   0x00000001
#define EFI_FVB_READ_ENABLED_CAP    0x00000002
#define EFI_FVB_READ_STATUS         0x00000004

#define EFI_FVB_WRITE_DISABLED_CAP  0x00000008
#define EFI_FVB_WRITE_ENABLED_CAP   0x00000010
#define EFI_FVB_WRITE_STATUS        0x00000020

#define EFI_FVB_LOCK_CAP            0x00000040
#define EFI_FVB_LOCK_STATUS         0x00000080

#define EFI_FVB_STICKY_WRITE        0x00000200
#define EFI_FVB_MEMORY_MAPPED       0x00000400
#define EFI_FVB_ERASE_POLARITY      0x00000800

#define EFI_FVB_ALIGNMENT_CAP       0x00008000
#define EFI_FVB_ALIGNMENT_2         0x00010000
#define EFI_FVB_ALIGNMENT_4         0x00020000
#define EFI_FVB_ALIGNMENT_8         0x00040000
#define EFI_FVB_ALIGNMENT_16        0x00080000
#define EFI_FVB_ALIGNMENT_32        0x00100000
#define EFI_FVB_ALIGNMENT_64        0x00200000
#define EFI_FVB_ALIGNMENT_128       0x00400000
#define EFI_FVB_ALIGNMENT_256       0x00800000
#define EFI_FVB_ALIGNMENT_512       0x01000000
#define EFI_FVB_ALIGNMENT_1K        0x02000000
#define EFI_FVB_ALIGNMENT_2K        0x04000000
#define EFI_FVB_ALIGNMENT_4K        0x08000000
#define EFI_FVB_ALIGNMENT_8K        0x10000000
#define EFI_FVB_ALIGNMENT_16K       0x20000000
#define EFI_FVB_ALIGNMENT_32K       0x40000000
#define EFI_FVB_ALIGNMENT_64K       0x80000000
///@}

/// This is a simple macro defined as the set of all FV Block Attributes signifying capabilities.
#define EFI_FVB_CAPABILITIES  ( EFI_FVB_READ_DISABLED_CAP  | \
                                EFI_FVB_READ_ENABLED_CAP   | \
                                EFI_FVB_WRITE_DISABLED_CAP | \
                                EFI_FVB_WRITE_ENABLED_CAP  | \
                                EFI_FVB_LOCK_CAP \
                              )

/** A parameterized macro defining a boolean expression that tests the state of a particular bit.
  *
  * @param FvbAttributes  Indicates a test for CLEAR if EFI_FVB_ERASE_POLARITY is 1, else test for SET.
  *
  * @param TestAttributes The set of bits to test.
  *
  * @param Bit            A value indicating the bit(s) to test.
  *                       If multiple bits are set, the logical OR of their tests is the expression's value.
**/
#define EFI_TEST_FFS_ATTRIBUTES_BIT( FvbAttributes, TestAttributes, Bit) \
    ((BOOLEAN) \
      ((FvbAttributes & EFI_FVB_ERASE_POLARITY) ? (((~TestAttributes) & Bit) == Bit) : ((TestAttributes & Bit) == Bit)) \
    )

/// A simple macro defined as the set of all FV Block Attribute bits that indicate status.
#define EFI_FVB_STATUS    (EFI_FVB_READ_STATUS | EFI_FVB_WRITE_STATUS | EFI_FVB_LOCK_STATUS)

#endif  /* __EFI_FIRMWARE_VOLUME_HEADER_H__ */
