#ifndef OC_MISC_LIB_H
#define OC_MISC_LIB_H

#include <Uefi.h>
UINTN
CountProtocolInstances (
  IN  EFI_GUID  *Protocol,
  OUT EFI_HANDLE  **HandleBuffer
);

#endif // OC_MISC_LIB_H