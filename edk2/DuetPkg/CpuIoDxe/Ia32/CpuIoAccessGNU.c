/*  This file is only used when not able to compile the MASM CpuIoAccess.asm
    NOTE: Compiling with -fomit-frame-pointer would get you to roughly the exact
    same code as the MASM file although GCC will typically include movzbl %al, %eax
    or movzwl %ax, %eax instructions on the read functions such that the entire
    eax result register will be valid, not just the lowest 8 or 16 bits.
 */
#ifdef __GNUC__

/*  A quick note about GCC inline asm and the GNU assembler:
    When gas encounters an instruction with a suffix (e.g. inb, inw, or inl vs. just in) it will
    warn if the operand corresponding to the suffix is not of the correct size and will assume you
    meant what you said when you specified the suffix.

    Because GCC does not enable us to see whether it is replacing %0 with %al, %ax, or %eax it is
    helpful to have the assembler warn us that GCC is making an incorrect assumption.  The actual
    in or out instruction will always be generated correctly in this case since the assembler is
    correct in assuming we meant what we said when we specified the suffix.  However, GCC might
    generate incorrect surrounding code.  For example, if we were to incorrectly specify the
    output size of an in instruction as UINT32, GCC would potentially fail to issue movz(b|w)l after
    it under the assumption that the in instruction filled the entire eax register and not just
    the al or ax portion.

    GCC determines which size of register to use based on the C data type.  So for in instructions
    the interesting type is that of the automatic variable named Data which is specified as an
    output operand to the inline assembly statement.  For example:

      UINT8     Data;
      asm ( "inb %1, %0"
          : "=a"(Data)
          : "d"(Port)
          );
      return Data;

    In this case, GCC will replace %0 with %al.  If Data had been specified as UINT16, it would replace
    %0 with %ax, and for UINT32 with %eax.

    Likewise in the case of IA32 out instructions, GCC will replace %0 with the appropriately sized
    register based on the size of the input operand.  There is one gotcha though.  The CpuIoWrite
    series of functions all use UINT32 as the type of the second (Data) argument.  This means that
    for GCC to output the correct register size we must cast it appropriately.

    The Port number is always a UINT16 so GCC will always ouput %dx.
 */

#include "CpuIoAccess.h"

UINT8
IA32API
CpuIoRead8 (
  IN  UINT16  Port
  )
{
  UINT8     Data;
  asm ( "inb %1, %0"
      : "=a"(Data)
      : "d"(Port)
      );
  return Data;
}

UINT16
IA32API
CpuIoRead16 (
  IN  UINT16  Port
  )
{
  UINT16    Data;
  asm ( "inw %1, %0"
      : "=a"(Data)
      : "d"(Port)
      );
  return Data;
}

UINT32
IA32API
CpuIoRead32 (
  IN  UINT16  Port
  )
{
  UINT32    Data;
  asm ( "inl %1, %0"
      : "=a"(Data)
      : "d"(Port)
      );
  return Data;
}

VOID
IA32API
CpuIoWrite8 (
  IN  UINT16  Port,
  IN  UINT32  Data
  )
{
  asm ( "outb %1, %0"
      : /* No outputs */
      : "d"(Port)
      , "a"((UINT8)Data)
      );
}

VOID
IA32API
CpuIoWrite16 (
  IN  UINT16  Port,
  IN  UINT32  Data
  )
{
  asm ( "outw %1, %0"
      : /* No outputs */
      : "d"(Port)
      , "a"((UINT16)Data)
      );
}

VOID
IA32API
CpuIoWrite32 (
  IN  UINT16  Port,
  IN  UINT32  Data
  )
{
  asm ( "outl %1, %0"
      : /* No outputs */
      : "d"(Port)
      /*  NOTE: Cast is technically unnecessary but we use it to illustrate
          that we always want to output a UINT32 and never anything else.
       */
      , "a"((UINT32)Data) 
      );
}

#endif /* def __GNUC__ */
