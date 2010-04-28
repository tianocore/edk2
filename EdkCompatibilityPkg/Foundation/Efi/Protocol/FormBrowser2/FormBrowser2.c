/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FormBrowser2.c

Abstract:

  The EFI_FORM_BROWSER2_PROTOCOL is the interface to the UEFI configuration driver.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (FormBrowser2)

EFI_GUID  gEfiFormBrowser2ProtocolGuid = EFI_FORM_BROWSER2_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiFormBrowser2ProtocolGuid, "Form Browser2 Protocol", "Form Browser 2.1 protocol");
