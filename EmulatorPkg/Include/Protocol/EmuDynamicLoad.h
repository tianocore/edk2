/*++
  This file contains a 'Sample Driver' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may be modified by the user, subject to
  the additional terms of the license agreement
--*/
/*++

Copyright (c) 2016, Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  EmuDynamicLoad.h

Abstract:
--*/

#ifndef _EMU_DYNAMIC_LOAD_PROTOCOL_H_
#define _EMU_DYNAMIC_LOAD_PROTOCOL_H_

#define EMU_DYNAMIC_LOAD_PROTOCOL_GUID  \
 { 0xb41452e6, 0x66f5, 0x4c24, { 0xba, 0x7, 0x6f, 0x9a, 0x4, 0x52, 0x3e, 0xa1 }}

typedef struct _EMU_DYNAMIC_LOAD_PROTOCOL  EMU_DYNAMIC_LOAD_PROTOCOL;

typedef
VOID *
(EFIAPI *EMU_DLOPEN) (
  IN CONST CHAR8 *FileName,
  IN INT16 Flag
  );

typedef
CHAR8 *
(EFIAPI *EMU_DLERROR) (
  VOID
  );

typedef
VOID *
(EFIAPI *EMU_DLSYM) (
  IN VOID* Handle,
  IN CONST CHAR8* Symbol
  );

typedef
INT16
(EFIAPI *EMU_DLCLOSE) (
  IN VOID* Handle
  );



struct _EMU_DYNAMIC_LOAD_PROTOCOL {
  EMU_DLOPEN  Dlopen;
  EMU_DLERROR Dlerror;
  EMU_DLSYM   Dlsym;
  EMU_DLCLOSE Dlclose;
};

extern EFI_GUID gEmuDynamicLoadProtocolGuid;

#endif // _EMU_DYNAMIC_LOAD_PROTOCOL_H_
