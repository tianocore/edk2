/*++

Copyright (c) 2001 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BootScriptExecuter.h
    
Abstract:

  Boot Script Executer PPI as defined in EFI 2.0

--*/

#ifndef _PEI_BOOT_SCRIPT_EXECUTER_PPI_H
#define _PEI_BOOT_SCRIPT_EXECUTER_PPI_H

#define PEI_BOOT_SCRIPT_EXECUTER_PPI_GUID \
  { \
    0xabd42895, 0x78cf, 0x4872, {0x84, 0x44, 0x1b, 0x5c, 0x18, 0x0b, 0xfb, 0xff} \
  }

EFI_FORWARD_DECLARATION (PEI_BOOT_SCRIPT_EXECUTER_PPI);

#define PEI_BOOT_SCRIPT_EXECUTER_PPI_REVISION 0x00000001

typedef
EFI_STATUS
(EFIAPI *PEI_BOOT_SCRIPT_EXECUTE) (
  IN     EFI_PEI_SERVICES                        **PeiServices,
  IN PEI_BOOT_SCRIPT_EXECUTER_PPI                * This,
  IN     EFI_PHYSICAL_ADDRESS                    Address,
  IN     EFI_GUID                                * FvFile OPTIONAL
  );

struct _PEI_BOOT_SCRIPT_EXECUTER_PPI {
  UINT64                  Revision;
  PEI_BOOT_SCRIPT_EXECUTE Execute;
};

extern EFI_GUID gPeiBootScriptExecuterPpiGuid;

#endif
