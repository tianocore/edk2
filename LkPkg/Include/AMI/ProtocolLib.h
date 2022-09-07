#ifndef OC_MISC_LIB_H
#define OC_MISC_LIB_H

#include <Uefi.h>
UINTN
CountProtocolInstances (
  IN EFI_GUID  *Protocol
);

#endif // OC_MISC_LIB_H