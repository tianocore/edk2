/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PpisNeededByDxeCore.c

Abstract:
  
Revision History:

**/

#include "PpisNeededByDxeCore.h"
#include "HobGeneration.h"
#include "SerialStatusCode.h"

EFI_STATUS
EFIAPI
PreparePpisNeededByDxeCore (
  IN HOB_TEMPLATE  *Hob
  )
/*++

Routine Description:

  This routine adds the PPI/Protocol Hobs that are consumed by the DXE Core.
  Normally these come from PEI, but since our PEI was 32-bit we need an
  alternate source. That is this driver.

  This driver does not consume PEI or DXE services and thus updates the 
  Phit (HOB list) directly

Arguments:

  HobStart - Pointer to the beginning of the HOB List from PEI

Returns:

  This function should after it has add it's HOBs

--*/
{
  //EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeCoffLoader;
  //EFI_DECOMPRESS_PROTOCOL                   *EfiDecompress;
  //EFI_TIANO_DECOMPRESS_PROTOCOL             *TianoDecompress;
  EFI_REPORT_STATUS_CODE                    ReportStatusCode;

  //InstallEfiPeiFlushInstructionCache (&FlushInstructionCache);
  //Hob->FlushInstructionCache.Interface = FlushInstructionCache;

  // R9 do not need this protocol.
  // InstallEfiPeiTransferControl (&TransferControl);
  // Hob->TransferControl.Interface = TransferControl;

  //InstallEfiPeiPeCoffLoader (NULL, &PeCoffLoader, NULL);
  //Hob->PeCoffLoader.Interface = PeCoffLoader;

  //InstallEfiDecompress (&EfiDecompress);
  //Hob->EfiDecompress.Interface = EfiDecompress;

  //InstallTianoDecompress (&TianoDecompress);
  //Hob->TianoDecompress.Interface = TianoDecompress;

  InstallSerialStatusCode (&ReportStatusCode);
  Hob->SerialStatusCode.Interface = (VOID *)(UINTN)ReportStatusCode;

  return EFI_SUCCESS;
}


