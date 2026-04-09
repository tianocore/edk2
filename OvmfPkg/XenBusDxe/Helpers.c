#include "XenBusDxe.h"

CHAR8 *
AsciiStrDup (
  IN CONST CHAR8  *Str
  )
{
  return AllocateCopyPool (AsciiStrSize (Str), Str);
}
