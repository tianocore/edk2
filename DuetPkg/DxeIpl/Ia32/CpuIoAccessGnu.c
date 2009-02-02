UINT8
EFIAPI
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

VOID
EFIAPI
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