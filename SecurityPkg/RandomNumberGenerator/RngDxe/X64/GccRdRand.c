/** @file
  RDRAND Support Routines for GCC environment.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

/**
  Generates a 16-bit random number through RDRAND instruction.

  @param[out]  Rand          Buffer pointer to store the random result.

  @retval TRUE               RDRAND call was successful.
  @retval FALSE              Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
RdRand16Step (
  OUT UINT16       *Rand
  )
{
  UINT8  Carry;

  //
  // Uses byte code for RDRAND instruction,
  // in case that GCC version has no direct support on RDRAND assembly.
  //
  __asm__ __volatile__ (
    ".byte 0x66; .byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
    :"=a"  (*Rand),
     "=qm" (Carry)
    ); 

  return (BOOLEAN) Carry;
}

/**
  Generates a 32-bit random number through RDRAND instruction.

  @param[out]  Rand          Buffer pointer to store the random result.

  @retval TRUE               RDRAND call was successful.
  @retval FALSE              Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
RdRand32Step (
  OUT UINT32       *Rand
  )
{
  UINT8  Carry;

  __asm__ __volatile__ (
    ".byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
    :"=a"  (*Rand), 
     "=qm" (Carry)
    );

  return (BOOLEAN) Carry;
}

/**
  Generates a 64-bit random number through RDRAND instruction.

  @param[out]  Rand          Buffer pointer to store the random result.

  @retval TRUE               RDRAND call was successful.
  @retval FALSE              Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
RdRand64Step  (
  OUT UINT64   *Rand
  )
{
  UINT8  Carry;

  __asm__ __volatile__ (
    ".byte 0x48; .byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
    :"=a"  (*Rand), 
     "=qm" (Carry)
    );
  
  return (BOOLEAN) Carry;
}
