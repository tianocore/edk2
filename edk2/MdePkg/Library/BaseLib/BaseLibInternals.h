/** @file
  Declaration of internal functions in BaseLib.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BASE_LIB_INTERNALS__
#define __BASE_LIB_INTERNALS__

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>

//
// Math functions
//

/**
  Shifts a 64-bit integer left between 0 and 63 bits. The low bits
  are filled with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the left by Count bits. The
  low Count bits are set to zero. The shifted value is returned.

  @param  Operand The 64-bit operand to shift left.
  @param  Count   The number of bits to shift left.

  @return Operand << Count

**/
UINT64
EFIAPI
InternalMathLShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

/**
  Shifts a 64-bit integer right between 0 and 63 bits. This high bits
  are filled with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the right by Count bits. The
  high Count bits are set to zero. The shifted value is returned.

  @param  Operand The 64-bit operand to shift right.
  @param  Count   The number of bits to shift right.

  @return Operand >> Count

**/
UINT64
EFIAPI
InternalMathRShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

/**
  Shifts a 64-bit integer right between 0 and 63 bits. The high bits
  are filled with original integer's bit 63. The shifted value is returned.

  This function shifts the 64-bit value Operand to the right by Count bits. The
  high Count bits are set to bit 63 of Operand.  The shifted value is returned.

  @param  Operand The 64-bit operand to shift right.
  @param  Count   The number of bits to shift right.

  @return Operand arithmetically shifted right by Count

**/
UINT64
EFIAPI
InternalMathARShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

/**
  Rotates a 64-bit integer left between 0 and 63 bits, filling
  the low bits with the high bits that were rotated.

  This function rotates the 64-bit value Operand to the left by Count bits. The
  low Count bits are fill with the high Count bits of Operand. The rotated
  value is returned.

  @param  Operand The 64-bit operand to rotate left.
  @param  Count   The number of bits to rotate left.

  @return Operand <<< Count

**/
UINT64
EFIAPI
InternalMathLRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

/**
  Rotates a 64-bit integer right between 0 and 63 bits, filling
  the high bits with the high low bits that were rotated.

  This function rotates the 64-bit value Operand to the right by Count bits.
  The high Count bits are fill with the low Count bits of Operand. The rotated
  value is returned.

  @param  Operand The 64-bit operand to rotate right.
  @param  Count   The number of bits to rotate right.

  @return Operand >>> Count

**/
UINT64
EFIAPI
InternalMathRRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

/**
  Switches the endianess of a 64-bit integer.

  This function swaps the bytes in a 64-bit unsigned value to switch the value
  from little endian to big endian or vice versa. The byte swapped value is
  returned.

  @param  Operand A 64-bit unsigned value.

  @return The byte swapped Operand.

**/
UINT64
EFIAPI
InternalMathSwapBytes64 (
  IN      UINT64                    Operand
  );

/**
  Multiples a 64-bit unsigned integer by a 32-bit unsigned integer
  and generates a 64-bit unsigned result.

  This function multiples the 64-bit unsigned value Multiplicand by the 32-bit
  unsigned value Multiplier and generates a 64-bit unsigned result. This 64-
  bit unsigned result is returned.

  @param  Multiplicand  A 64-bit unsigned value.
  @param  Multiplier    A 32-bit unsigned value.

  @return Multiplicand * Multiplier

**/
UINT64
EFIAPI
InternalMathMultU64x32 (
  IN      UINT64                    Multiplicand,
  IN      UINT32                    Multiplier
  );

/**
  Multiples a 64-bit unsigned integer by a 64-bit unsigned integer
  and generates a 64-bit unsigned result.

  This function multiples the 64-bit unsigned value Multiplicand by the 64-bit
  unsigned value Multiplier and generates a 64-bit unsigned result. This 64-
  bit unsigned result is returned.

  @param  Multiplicand  A 64-bit unsigned value.
  @param  Multiplier    A 64-bit unsigned value.

  @return Multiplicand * Multiplier

**/
UINT64
EFIAPI
InternalMathMultU64x64 (
  IN      UINT64                    Multiplicand,
  IN      UINT64                    Multiplier
  );

/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and
  generates a 64-bit unsigned result.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. This
  function returns the 64-bit unsigned quotient.

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.

  @return Dividend / Divisor

**/
UINT64
EFIAPI
InternalMathDivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );

/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and
  generates a 32-bit unsigned remainder.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 32-bit remainder. This function
  returns the 32-bit unsigned remainder.

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.

  @return Dividend % Divisor

**/
UINT32
EFIAPI
InternalMathModU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );

/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and
  generates a 64-bit unsigned result and an optional 32-bit unsigned remainder.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. If Remainder
  is not NULL, then the 32-bit unsigned remainder is returned in Remainder.
  This function returns the 64-bit unsigned quotient.

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.
  @param  Remainder A pointer to a 32-bit unsigned value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
UINT64
EFIAPI
InternalMathDivRemU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor,
  OUT     UINT32                    *Remainder OPTIONAL
  );

/**
  Divides a 64-bit unsigned integer by a 64-bit unsigned integer and
  generates a 64-bit unsigned result and an optional 64-bit unsigned remainder.

  This function divides the 64-bit unsigned value Dividend by the 64-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. If Remainder
  is not NULL, then the 64-bit unsigned remainder is returned in Remainder.
  This function returns the 64-bit unsigned quotient.

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 64-bit unsigned value.
  @param  Remainder A pointer to a 64-bit unsigned value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
UINT64
EFIAPI
InternalMathDivRemU64x64 (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder OPTIONAL
  );

/**
  Divides a 64-bit signed integer by a 64-bit signed integer and
  generates a 64-bit signed result and an optional 64-bit signed remainder.

  This function divides the 64-bit signed value Dividend by the 64-bit
  signed value Divisor and generates a 64-bit signed quotient. If Remainder
  is not NULL, then the 64-bit signed remainder is returned in Remainder.
  This function returns the 64-bit signed quotient.

  @param  Dividend  A 64-bit signed value.
  @param  Divisor   A 64-bit signed value.
  @param  Remainder A pointer to a 64-bit signed value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
INT64
EFIAPI
InternalMathDivRemS64x64 (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder  OPTIONAL
 );

/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the
  new stack specified by NewStack and passing in the parameters specified
  by Context1 and Context2.  Context1 and Context2 are optional and may
  be NULL.  The function EntryPoint must never return.
  Marker will be ignored on IA-32, x64, and EBC.
  IPF CPUs expect one additional parameter of type VOID * that specifies
  the new backing store pointer.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  Marker      VA_LIST marker for the variable argument list.

**/
VOID
EFIAPI
InternalSwitchStack (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,   OPTIONAL
  IN      VOID                      *Context2,   OPTIONAL
  IN      VOID                      *NewStack,
  IN      VA_LIST                   Marker
  );


/**
  Worker function that locates the Node in the List.

  By searching the List, finds the location of the Node in List. At the same time,
  verifies the validity of this list.

  If List is NULL, then ASSERT().
  If List->ForwardLink is NULL, then ASSERT().
  If List->backLink is NULL, then ASSERT().
  If Node is NULL, then ASSERT();
  If PcdMaximumLinkedListLenth is not zero, and prior to insertion the number
  of nodes in ListHead, including the ListHead node, is greater than or
  equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  List  A pointer to a node in a linked list.
  @param  Node  A pointer to one nod.

  @retval TRUE   Node is in List
  @retval FALSE  Node isn't in List, or List is invalid

**/
BOOLEAN
EFIAPI
IsNodeInList (
  IN      CONST LIST_ENTRY      *List,
  IN      CONST LIST_ENTRY      *Node
  );


/**
  Performs an atomic increment of an 32-bit unsigned integer.

  Performs an atomic increment of the 32-bit unsigned integer specified by
  Value and returns the incremented value. The increment operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  @param  Value A pointer to the 32-bit value to increment.

  @return The incremented value.

**/
UINT32
EFIAPI
InternalSyncIncrement (
  IN      volatile UINT32           *Value
  );


/**
  Performs an atomic decrement of an 32-bit unsigned integer.

  Performs an atomic decrement of the 32-bit unsigned integer specified by
  Value and returns the decrement value. The decrement operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  @param  Value A pointer to the 32-bit value to decrement.

  @return The decrement value.

**/
UINT32
EFIAPI
InternalSyncDecrement (
  IN      volatile UINT32           *Value
  );


/**
  Performs an atomic compare exchange operation on a 32-bit unsigned integer.

  Performs an atomic compare exchange operation on the 32-bit unsigned integer
  specified by Value.  If Value is equal to CompareValue, then Value is set to
  ExchangeValue and CompareValue is returned.  If Value is not equal to CompareValue,
  then Value is returned.  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param  Value         A pointer to the 32-bit value for the compare exchange
                        operation.
  @param  CompareValue  32-bit value used in compare operation.
  @param  ExchangeValue 32-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT32
EFIAPI
InternalSyncCompareExchange32 (
  IN      volatile UINT32           *Value,
  IN      UINT32                    CompareValue,
  IN      UINT32                    ExchangeValue
  );


/**
  Performs an atomic compare exchange operation on a 64-bit unsigned integer.

  Performs an atomic compare exchange operation on the 64-bit unsigned integer specified
  by Value.  If Value is equal to CompareValue, then Value is set to ExchangeValue and
  CompareValue is returned.  If Value is not equal to CompareValue, then Value is returned.
  The compare exchange operation must be performed using MP safe mechanisms.

  @param  Value         A pointer to the 64-bit value for the compare exchange
                        operation.
  @param  CompareValue  64-bit value used in compare operation.
  @param  ExchangeValue 64-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT64
EFIAPI
InternalSyncCompareExchange64 (
  IN      volatile UINT64           *Value,
  IN      UINT64                    CompareValue,
  IN      UINT64                    ExchangeValue
  );


/**
  Worker function that returns a bit field from Operand.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
  @param  EndBit    The ordinal of the most significant bit in the bit field.

  @return The bit field read.

**/
unsigned int
EFIAPI
BitFieldReadUint (
  IN      unsigned int              Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );


/**
  Worker function that reads a bit field from Operand, performs a bitwise OR,
  and returns the result.

  Performs a bitwise OR between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new value is returned.

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
  @param  OrData    The value to OR with the read value from the value

  @return The new value.

**/
unsigned int
EFIAPI
BitFieldOrUint (
  IN      unsigned int              Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      unsigned int              OrData
  );


/**
  Worker function that reads a bit field from Operand, performs a bitwise AND,
  and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new value is returned.

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
  @param  AndData    The value to And with the read value from the value

  @return The new value.

**/
unsigned int
EFIAPI
BitFieldAndUint (
  IN      unsigned int              Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      unsigned int              AndData
  );


/**
  Worker function that checks ASSERT condition for JumpBuffer

  Checks ASSERT condition for JumpBuffer.

  If JumpBuffer is NULL, then ASSERT().
  For IPF CPUs, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().

  @param  JumpBuffer    A pointer to CPU context buffer.

**/
VOID
EFIAPI
InternalAssertJumpBuffer (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  );


/**
  Restores the CPU context that was saved with SetJump().

  Restores the CPU context from the buffer specified by JumpBuffer.
  This function never returns to the caller.
  Instead is resumes execution based on the state of JumpBuffer.

  @param  JumpBuffer    A pointer to CPU context buffer.
  @param  Value         The value to return when the SetJump() context is restored.

**/
VOID
EFIAPI
InternalLongJump (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,
  IN      UINTN                     Value
  );


//
// Ia32 and x64 specific functions
//
#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)

/**
  Reads the current Global Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current GDTR descriptor and returns it in Gdtr. This
  function is only available on IA-32 and X64.

  @param  Gdtr  Pointer to a GDTR descriptor.

**/
VOID
EFIAPI
InternalX86ReadGdtr (
  OUT     IA32_DESCRIPTOR           *Gdtr
  );

/**
  Writes the current Global Descriptor Table Register (GDTR) descriptor.

  Writes and the current GDTR descriptor specified by Gdtr. This function is
  only available on IA-32 and X64.

  @param  Gdtr  Pointer to a GDTR descriptor.

**/
VOID
EFIAPI
InternalX86WriteGdtr (
  IN      CONST IA32_DESCRIPTOR     *Gdtr
  );

/**
  Reads the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current IDTR descriptor and returns it in Idtr. This
  function is only available on IA-32 and X64.

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
InternalX86ReadIdtr (
  OUT     IA32_DESCRIPTOR           *Idtr
  );

/**
  Writes the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Writes the current IDTR descriptor and returns it in Idtr. This function is
  only available on IA-32 and X64.

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
InternalX86WriteIdtr (
  IN      CONST IA32_DESCRIPTOR     *Idtr
  );

/**
  Save the current floating point/SSE/SSE2 context to a buffer.

  Saves the current floating point/SSE/SSE2 state to the buffer specified by
  Buffer. Buffer must be aligned on a 16-byte boundary. This function is only
  available on IA-32 and X64.

  @param  Buffer  Pointer to a buffer to save the floating point/SSE/SSE2 context.

**/
VOID
EFIAPI
InternalX86FxSave (
  OUT     IA32_FX_BUFFER            *Buffer
  );

/**
  Restores the current floating point/SSE/SSE2 context from a buffer.

  Restores the current floating point/SSE/SSE2 state from the buffer specified
  by Buffer. Buffer must be aligned on a 16-byte boundary. This function is
  only available on IA-32 and X64.

  @param  Buffer  Pointer to a buffer to save the floating point/SSE/SSE2 context.

**/
VOID
EFIAPI
InternalX86FxRestore (
  IN      CONST IA32_FX_BUFFER      *Buffer
  );

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
InternalX86EnablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  );

/**
  Disables the 32-bit paging mode on the CPU.

  Disables the 32-bit paging mode on the CPU and returns to 32-bit protected
  mode. This function assumes the current execution mode is 32-paged protected
  mode. This function is only available on IA-32. After the 32-bit paging mode
  is disabled, control is transferred to the function specified by EntryPoint
  using the new stack specified by NewStack and passing in the parameters
  specified by Context1 and Context2. Context1 and Context2 are optional and
  may be NULL. The function EntryPoint must never return.

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
InternalX86DisablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  );

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
InternalX86EnablePaging64 (
  IN      UINT16                    Cs,
  IN      UINT64                    EntryPoint,
  IN      UINT64                    Context1,  OPTIONAL
  IN      UINT64                    Context2,  OPTIONAL
  IN      UINT64                    NewStack
  );

/**
  Disables the 64-bit paging mode on the CPU.

  Disables the 64-bit paging mode on the CPU and returns to 32-bit protected
  mode. This function assumes the current execution mode is 64-paging mode.
  This function is only available on X64. After the 64-bit paging mode is
  disabled, control is transferred to the function specified by EntryPoint
  using the new stack specified by NewStack and passing in the parameters
  specified by Context1 and Context2. Context1 and Context2 are optional and
  may be 0. The function EntryPoint must never return.

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
InternalX86DisablePaging64 (
  IN      UINT16                    Cs,
  IN      UINT32                    EntryPoint,
  IN      UINT32                    Context1,  OPTIONAL
  IN      UINT32                    Context2,  OPTIONAL
  IN      UINT32                    NewStack
  );


#elif defined (MDE_CPU_IPF)
//
//
// IPF specific functions
//

/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  NewBsp      A pointer to the new memory location for RSE backing
                      store.

**/
VOID
EFIAPI
AsmSwitchStackAndBackingStore (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack,
  IN      VOID                      *NewBsp
  );
#else

#endif

#endif
