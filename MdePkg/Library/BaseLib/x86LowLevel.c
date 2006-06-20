/** @file
  IA-32/x64 specific functions.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  x86LowLevel.c

**/

#include "BaseLibInternals.h"

//
// Bit-wise MSR operations
//

/**
  Reads a 64-bit MSR, performs a bitwise inclusive OR on the lower 32-bits, and
  writes the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the lower 32-bits of the read result and the value specified by
  OrData, and writes the result to the 64-bit MSR specified by Index. The lower
  32-bits of the value written to the MSR is returned. No parameter checking is
  performed on Index or OrData, and some of these may cause CPU exceptions. The
  caller must either guarantee that Index and OrData are valid, or the caller
  must establish proper exception handlers. This function is only available on
  IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  OrData  The value to OR with the read value from the MSR.

  @return The lower 32-bit value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrOr32 (
  IN      UINT32                    Index,
  IN      UINT32                    OrData
  )
{
  return (UINT32)AsmMsrOr64 (Index, OrData);
}

/**
  Reads a 64-bit MSR, performs a bitwise AND on the lower 32-bits, and writes
  the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  lower 32-bits of the read result and the value specified by AndData, and
  writes the result to the 64-bit MSR specified by Index. The lower 32-bits of
  the value written to the MSR is returned. No parameter checking is performed
  on Index or AndData, and some of these may cause CPU exceptions. The caller
  must either guarantee that Index and AndData are valid, or the caller must
  establish proper exception handlers. This function is only available on IA-32
  and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.

  @return The lower 32-bit value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrAnd32 (
  IN      UINT32                    Index,
  IN      UINT32                    AndData
  )
{
  return (UINT32)AsmMsrAnd64 (Index, AndData);
}

/**
  Reads a 64-bit MSR, performs a bitwise AND followed by a bitwise inclusive OR
  on the lower 32-bits, and writes the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  lower 32-bits of the read result and the value specified by AndData
  preserving the upper 32-bits, performs a bitwise inclusive OR between the
  result of the AND operation and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Address. The lower 32-bits of the value
  written to the MSR is returned. No parameter checking is performed on Index,
  AndData, or OrData, and some of these may cause CPU exceptions. The caller
  must either guarantee that Index, AndData, and OrData are valid, or the
  caller must establish proper exception handlers. This function is only
  available on IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.
  @param  OrData  The value to OR with the result of the AND operation.

  @return The lower 32-bit value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrAndThenOr32 (
  IN      UINT32                    Index,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  )
{
  return (UINT32)AsmMsrAndThenOr64 (Index, AndData, OrData);
}

/**
  Reads a bit field of an MSR.

  Reads the bit field in the lower 32-bits of a 64-bit MSR. The bit field is
  specified by the StartBit and the EndBit. The value of the bit field is
  returned. The caller must either guarantee that Index is valid, or the caller
  must set up exception handlers to catch the exceptions. This function is only
  available on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to read.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.

  @return The bit field read from the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldRead32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  return BitFieldRead32 (AsmReadMsr32 (Index), StartBit, EndBit);
}

/**
  Writes a bit field to an MSR.

  Writes Value to a bit field in the lower 32-bits of a  64-bit MSR. The bit
  field is specified by the StartBit and the EndBit. All other bits in the
  destination MSR are preserved. The lower 32-bits of the MSR written is
  returned. Extra left bits in Value are stripped. The caller must either
  guarantee that Index and the data written is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  Value     New value of the bit field.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldWrite32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    Value
  )
{
  ASSERT (EndBit < sizeof (Value) * 8);
  ASSERT (StartBit <= EndBit);
  return (UINT32)AsmMsrBitFieldWrite64 (Index, StartBit, EndBit, Value);
}

/**
  Reads a bit field in a 64-bit MSR, performs a bitwise OR, and writes the
  result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the read result and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Index. The lower 32-bits of the value
  written to the MSR are returned. Extra left bits in OrData are stripped. The
  caller must either guarantee that Index and the data written is valid, or
  the caller must set up exception handlers to catch the exceptions. This
  function is only available on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  OrData    The value to OR with the read value from the MSR.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldOr32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    OrData
  )
{
  ASSERT (EndBit < sizeof (OrData) * 8);
  ASSERT (StartBit <= EndBit);
  return (UINT32)AsmMsrBitFieldOr64 (Index, StartBit, EndBit, OrData);
}

/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND, and writes the
  result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  read result and the value specified by AndData, and writes the result to the
  64-bit MSR specified by Index. The lower 32-bits of the value written to the
  MSR are returned. Extra left bits in AndData are stripped. The caller must
  either guarantee that Index and the data written is valid, or the caller must
  set up exception handlers to catch the exceptions. This function is only
  available on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the MSR.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldAnd32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData
  )
{
  ASSERT (EndBit < sizeof (AndData) * 8);
  ASSERT (StartBit <= EndBit);
  return (UINT32)AsmMsrBitFieldAnd64 (Index, StartBit, EndBit, AndData);
}

/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND followed by a
  bitwise inclusive OR, and writes the result back to the bit field in the
  64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND followed by a
  bitwise inclusive OR between the read result and the value specified by
  AndData, and writes the result to the 64-bit MSR specified by Index. The
  lower 32-bits of the value written to the MSR are returned. Extra left bits
  in both AndData and OrData are stripped. The caller must either guarantee
  that Index and the data written is valid, or the caller must set up exception
  handlers to catch the exceptions. This function is only available on IA-32
  and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the MSR.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldAndThenOr32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  )
{
  ASSERT (EndBit < sizeof (AndData) * 8);
  ASSERT (StartBit <= EndBit);
  return (UINT32)AsmMsrBitFieldAndThenOr64 (
                   Index,
                   StartBit,
                   EndBit,
                   AndData,
                   OrData
                   );
}

/**
  Reads a 64-bit MSR, performs a bitwise inclusive OR, and writes the result
  back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the read result and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Index. The value written to the MSR is
  returned. No parameter checking is performed on Index or OrData, and some of
  these may cause CPU exceptions. The caller must either guarantee that Index
  and OrData are valid, or the caller must establish proper exception handlers.
  This function is only available on IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  OrData  The value to OR with the read value from the MSR.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrOr64 (
  IN      UINT32                    Index,
  IN      UINT64                    OrData
  )
{
  return AsmWriteMsr64 (Index, AsmReadMsr64 (Index) | OrData);
}

/**
  Reads a 64-bit MSR, performs a bitwise AND, and writes the result back to the
  64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  read result and the value specified by OrData, and writes the result to the
  64-bit MSR specified by Index. The value written to the MSR is returned. No
  parameter checking is performed on Index or OrData, and some of these may
  cause CPU exceptions. The caller must either guarantee that Index and OrData
  are valid, or the caller must establish proper exception handlers. This
  function is only available on IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrAnd64 (
  IN      UINT32                    Index,
  IN      UINT64                    AndData
  )
{
  return AsmWriteMsr64 (Index, AsmReadMsr64 (Index) & AndData);
}

/**
  Reads a 64-bit MSR, performs a bitwise AND followed by a bitwise inclusive
  OR, and writes the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between read
  result and the value specified by AndData, performs a bitwise inclusive OR
  between the result of the AND operation and the value specified by OrData,
  and writes the result to the 64-bit MSR specified by Index. The value written
  to the MSR is returned. No parameter checking is performed on Index, AndData,
  or OrData, and some of these may cause CPU exceptions. The caller must either
  guarantee that Index, AndData, and OrData are valid, or the caller must
  establish proper exception handlers. This function is only available on IA-32
  and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.
  @param  OrData  The value to OR with the result of the AND operation.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrAndThenOr64 (
  IN      UINT32                    Index,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  )
{
  return AsmWriteMsr64 (Index, (AsmReadMsr64 (Index) & AndData) | OrData);
}

/**
  Reads a bit field of an MSR.

  Reads the bit field in the 64-bit MSR. The bit field is specified by the
  StartBit and the EndBit. The value of the bit field is returned. The caller
  must either guarantee that Index is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to read.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldRead64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  return BitFieldRead64 (AsmReadMsr64 (Index), StartBit, EndBit);
}

/**
  Writes a bit field to an MSR.

  Writes Value to a bit field in a 64-bit MSR. The bit field is specified by
  the StartBit and the EndBit. All other bits in the destination MSR are
  preserved. The MSR written is returned. Extra left bits in Value are
  stripped. The caller must either guarantee that Index and the data written is
  valid, or the caller must set up exception handlers to catch the exceptions.
  This function is only available on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  Value     New value of the bit field.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldWrite64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    Value
  )
{
  return AsmWriteMsr64 (
           Index,
           BitFieldWrite64 (AsmReadMsr64 (Index), StartBit, EndBit, Value)
           );
}

/**
  Reads a bit field in a 64-bit MSR, performs a bitwise inclusive OR, and
  writes the result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the read result and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Index. The value written to the MSR is
  returned. Extra left bits in OrData are stripped. The caller must either
  guarantee that Index and the data written is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  OrData    The value to OR with the read value from the bit field.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldOr64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    OrData
  )
{
  return AsmWriteMsr64 (
           Index,
           BitFieldOr64 (AsmReadMsr64 (Index), StartBit, EndBit, OrData)
           );
}

/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND, and writes the
  result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  read result and the value specified by AndData, and writes the result to the
  64-bit MSR specified by Index. The value written to the MSR is returned.
  Extra left bits in AndData are stripped. The caller must either guarantee
  that Index and the data written is valid, or the caller must set up exception
  handlers to catch the exceptions. This function is only available on IA-32
  and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the bit field.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldAnd64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData
  )
{
  return AsmWriteMsr64 (
           Index,
           BitFieldAnd64 (AsmReadMsr64 (Index), StartBit, EndBit, AndData)
           );
}

/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND followed by a
  bitwise inclusive OR, and writes the result back to the bit field in the
  64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND followed by
  a bitwise inclusive OR between the read result and the value specified by
  AndData, and writes the result to the 64-bit MSR specified by Index. The
  value written to the MSR is returned. Extra left bits in both AndData and
  OrData are stripped. The caller must either guarantee that Index and the data
  written is valid, or the caller must set up exception handlers to catch the
  exceptions. This function is only available on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the bit field.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldAndThenOr64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  )
{
  return AsmWriteMsr64 (
           Index,
           BitFieldAndThenOr64 (
             AsmReadMsr64 (Index),
             StartBit,
             EndBit,
             AndData,
             OrData
             )
           );
}

//
// Base Library CPU Functions
//

/**
  Retrieves the current CPU interrupt state.

  Retrieves the current CPU interrupt state. Returns TRUE is interrupts are
  currently enabled. Otherwise returns FALSE.

  @retval TRUE  CPU interrupts are enabled.
  @retval FALSE CPU interrupts are disabled.

**/
BOOLEAN
EFIAPI
GetInterruptState (
  VOID
  )
{
  IA32_EFLAGS32                     EFlags;

  EFlags.UintN = AsmReadEflags ();
  return (BOOLEAN)(EFlags.Bits.IF == 1);
}

//
// Ia32 and x64 specific functions
//

/**
  Reads the current Global Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current GDTR descriptor and returns it in Gdtr. This
  function is only available on IA-32 and X64.

  If Gdtr is NULL, then ASSERT().

  @param  Gdtr  Pointer to a GDTR descriptor.

**/
VOID
EFIAPI
AsmReadGdtr (
  OUT     IA32_DESCRIPTOR           *Gdtr
  )
{
  ASSERT (Gdtr != NULL);
  InternalX86ReadGdtr (Gdtr);
}

/**
  Writes the current Global Descriptor Table Register (GDTR) descriptor.

  Writes and the current GDTR descriptor specified by Gdtr. This function is
  only available on IA-32 and X64.

  If Gdtr is NULL, then ASSERT().

  @param  Gdtr  Pointer to a GDTR descriptor.

**/
VOID
EFIAPI
AsmWriteGdtr (
  IN      CONST IA32_DESCRIPTOR     *Gdtr
  )
{
  ASSERT (Gdtr != NULL);
  InternalX86WriteGdtr (Gdtr);
}

/**
  Reads the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current IDTR descriptor and returns it in Idtr. This
  function is only available on IA-32 and X64.

  If Idtr is NULL, then ASSERT().

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
AsmReadIdtr (
  OUT     IA32_DESCRIPTOR           *Idtr
  )
{
  ASSERT (Idtr != NULL);
  InternalX86ReadIdtr (Idtr);
}

/**
  Writes the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Writes the current IDTR descriptor and returns it in Idtr. This function is
  only available on IA-32 and X64.

  If Idtr is NULL, then ASSERT().

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
AsmWriteIdtr (
  IN      CONST IA32_DESCRIPTOR     *Idtr
  )
{
  ASSERT (Idtr != NULL);
  InternalX86WriteIdtr (Idtr);
}

/**
  Save the current floating point/SSE/SSE2 context to a buffer.

  Saves the current floating point/SSE/SSE2 state to the buffer specified by
  Buffer. Buffer must be aligned on a 16-byte boundary. This function is only
  available on IA-32 and X64.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-byte boundary, then ASSERT().

  @param  Buffer  Pointer to a buffer to save the floating point/SSE/SSE2 context.

**/
VOID
EFIAPI
AsmFxSave (
  OUT     IA32_FX_BUFFER            *Buffer
  )
{
  ASSERT (Buffer != NULL);
  ASSERT (((UINTN)Buffer & 0xf) == 0);

  InternalX86FxSave (Buffer);
  
  //
  // Mark one flag at end of Buffer, it will be check by AsmFxRestor()
  //
  *(UINT32 *) (&Buffer[sizeof (IA32_FX_BUFFER) - 4]) = 0xAA5555AA; 
}

/**
  Restores the current floating point/SSE/SSE2 context from a buffer.

  Restores the current floating point/SSE/SSE2 state from the buffer specified
  by Buffer. Buffer must be aligned on a 16-byte boundary. This function is
  only available on IA-32 and X64.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-byte boundary, then ASSERT().
  If Buffer was not saved with AsmFxSave(), then ASSERT().

  @param  Buffer  Pointer to a buffer to save the floating point/SSE/SSE2 context.

**/
VOID
EFIAPI
AsmFxRestore (
  IN CONST IA32_FX_BUFFER  *Buffer
  )
{
  ASSERT (Buffer != NULL);
  ASSERT (((UINTN)Buffer & 0xf) == 0);

  //
  // Check the flag recorded by AsmFxSave()
  //
  ASSERT (*(UINT32 *) (&Buffer[sizeof (IA32_FX_BUFFER) - 4]) == 0xAA5555AA);

  InternalX86FxRestore (Buffer);
}

/**
  Enables the 32-bit paging mode on the CPU.

  Enables the 32-bit paging mode on the CPU. CR0, CR3, CR4, and the page tables
  must be properly initialized prior to calling this service. This function
  assumes the current execution mode is 32-bit protected mode. This function is
  only available on IA-32. After the 32-bit paging mode is enabled, control is
  transferred to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If the current execution mode is not 32-bit protected mode, then ASSERT().
  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  There are a number of constraints that must be followed before calling this
  function:
  1)  Interrupts must be disabled.
  2)  The caller must be in 32-bit protected mode with flat descriptors. This
      means all descriptors must have a base of 0 and a limit of 4GB.
  3)  CR0 and CR4 must be compatible with 32-bit protected mode with flat
      descriptors.
  4)  CR3 must point to valid page tables that will be used once the transition
      is complete, and those page tables must guarantee that the pages for this
      function and the stack are identity mapped.

  @param  EntryPoint  A pointer to function to call with the new stack after
                      paging is enabled.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function as the first parameter after paging is enabled.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function as the second parameter after paging is enabled.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function after paging is enabled.

**/
VOID
EFIAPI
AsmEnablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  )
{
  ASSERT (EntryPoint != NULL);
  ASSERT (NewStack != NULL);
  InternalX86EnablePaging32 (EntryPoint, Context1, Context2, NewStack);
}

/**
  Disables the 32-bit paging mode on the CPU.

  Disables the 32-bit paging mode on the CPU and returns to 32-bit protected
  mode. This function assumes the current execution mode is 32-paged protected
  mode. This function is only available on IA-32. After the 32-bit paging mode
  is disabled, control is transferred to the function specified by EntryPoint
  using the new stack specified by NewStack and passing in the parameters
  specified by Context1 and Context2. Context1 and Context2 are optional and
  may be NULL. The function EntryPoint must never return.

  If the current execution mode is not 32-bit paged mode, then ASSERT().
  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  There are a number of constraints that must be followed before calling this
  function:
  1)  Interrupts must be disabled.
  2)  The caller must be in 32-bit paged mode.
  3)  CR0, CR3, and CR4 must be compatible with 32-bit paged mode.
  4)  CR3 must point to valid page tables that guarantee that the pages for
      this function and the stack are identity mapped.

  @param  EntryPoint  A pointer to function to call with the new stack after
                      paging is disabled.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function as the first parameter after paging is disabled.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function as the second parameter after paging is
                      disabled.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function after paging is disabled.

**/
VOID
EFIAPI
AsmDisablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  )
{
  ASSERT (EntryPoint != NULL);
  ASSERT (NewStack != NULL);
  InternalX86DisablePaging32 (EntryPoint, Context1, Context2, NewStack);
}

/**
  Enables the 64-bit paging mode on the CPU.

  Enables the 64-bit paging mode on the CPU. CR0, CR3, CR4, and the page tables
  must be properly initialized prior to calling this service. This function
  assumes the current execution mode is 32-bit protected mode with flat
  descriptors. This function is only available on IA-32. After the 64-bit
  paging mode is enabled, control is transferred to the function specified by
  EntryPoint using the new stack specified by NewStack and passing in the
  parameters specified by Context1 and Context2. Context1 and Context2 are
  optional and may be 0. The function EntryPoint must never return.

  If the current execution mode is not 32-bit protected mode with flat
  descriptors, then ASSERT().
  If EntryPoint is 0, then ASSERT().
  If NewStack is 0, then ASSERT().

  @param  Cs          The 16-bit selector to load in the CS before EntryPoint
                      is called. The descriptor in the GDT that this selector
                      references must be setup for long mode.
  @param  EntryPoint  The 64-bit virtual address of the function to call with
                      the new stack after paging is enabled.
  @param  Context1    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the first parameter after
                      paging is enabled.
  @param  Context2    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the second parameter after
                      paging is enabled.
  @param  NewStack    The 64-bit virtual address of the new stack to use for
                      the EntryPoint function after paging is enabled.

**/
VOID
EFIAPI
AsmEnablePaging64 (
  IN      UINT16                    Cs,
  IN      UINT64                    EntryPoint,
  IN      UINT64                    Context1,  OPTIONAL
  IN      UINT64                    Context2,  OPTIONAL
  IN      UINT64                    NewStack
  )
{
  ASSERT (EntryPoint != 0);
  ASSERT (NewStack != 0);
  InternalX86EnablePaging64 (Cs, EntryPoint, Context1, Context2, NewStack);
}

/**
  Disables the 64-bit paging mode on the CPU.

  Disables the 64-bit paging mode on the CPU and returns to 32-bit protected
  mode. This function assumes the current execution mode is 64-paging mode.
  This function is only available on X64. After the 64-bit paging mode is
  disabled, control is transferred to the function specified by EntryPoint
  using the new stack specified by NewStack and passing in the parameters
  specified by Context1 and Context2. Context1 and Context2 are optional and
  may be 0. The function EntryPoint must never return.

  If the current execution mode is not 64-bit paged mode, then ASSERT().
  If EntryPoint is 0, then ASSERT().
  If NewStack is 0, then ASSERT().

  @param  Cs          The 16-bit selector to load in the CS before EntryPoint
                      is called. The descriptor in the GDT that this selector
                      references must be setup for 32-bit protected mode.
  @param  EntryPoint  The 64-bit virtual address of the function to call with
                      the new stack after paging is disabled.
  @param  Context1    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the first parameter after
                      paging is disabled.
  @param  Context2    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the second parameter after
                      paging is disabled.
  @param  NewStack    The 64-bit virtual address of the new stack to use for
                      the EntryPoint function after paging is disabled.

**/
VOID
EFIAPI
AsmDisablePaging64 (
  IN      UINT16                    Cs,
  IN      UINT32                    EntryPoint,
  IN      UINT32                    Context1,  OPTIONAL
  IN      UINT32                    Context2,  OPTIONAL
  IN      UINT32                    NewStack
  )
{
  ASSERT (EntryPoint != 0);
  ASSERT (NewStack != 0);
  InternalX86DisablePaging64 (Cs, EntryPoint, Context1, Context2, NewStack);
}

//
// x86 version of MemoryFence()
//

/**
  Used to serialize load and store operations.

  All loads and stores that proceed calls to this function are guaranteed to be
  globally visible when this function returns.

**/
VOID
EFIAPI
MemoryFence (
  VOID
  )
{
}
