/*++

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 NtLoadAsDll.h

Abstract:

  Nt service Ppi that is used to load PE32s in the NT emulation environment.

--*/

#ifndef _NT_LOAD_AS_DLL_H_
#define _NT_LOAD_AS_DLL_H_

#include "Tiano.h"

#define EFI_NT_LOAD_AS_DLL_PPI_GUID \
  { \
    0xccc53f6b, 0xa03a, 0x4ed8, {0x83, 0x9a, 0x3, 0xd9, 0x9c, 0x2, 0xb4, 0xe3} \
  }

typedef
EFI_STATUS
(EFIAPI *EFI_NT_LOAD_AS_DLL) (
  IN CHAR8    *PdbFileName,
  IN VOID     **ImageEntryPoint,
  OUT VOID    **ModHandle
  );

/*++

Routine Description:
  Loads the .DLL file is present when a PE/COFF file is loaded.  This provides source level
  debugging for drivers that have cooresponding .DLL files on the local system.

Arguments:
  PdbFileName     - The name of the .PDB file.  This was found from the PE/COFF
                    file's debug directory entry.
  ImageEntryPoint - A pointer to the DLL entry point of the .DLL file was loaded.

Returns:
  EFI_SUCCESS     - The .DLL file was loaded, and the DLL entry point is returned in ImageEntryPoint
  EFI_NOT_FOUND   - The .DLL file could not be found
  EFI_UNSUPPORTED - The .DLL file was loaded, but the entry point to the .DLL file could not
                    determined.

--*/
typedef
EFI_STATUS
(EFIAPI *EFI_NT_FREE_LIBRARY) (
  IN VOID     *ModHandle
  );

/*++

Routine Description:
  Free resources allocated by Entry (). ModHandle was returned by
  Entry ().

Arguments:
  MohHandle  - Handle of the resources to free to undo the work.

Returns:
  EFI_SUCCESS - 

--*/
typedef struct {
  EFI_NT_LOAD_AS_DLL  Entry;
  EFI_NT_FREE_LIBRARY FreeLibrary;
} EFI_NT_LOAD_AS_DLL_PPI;

extern EFI_GUID gEfiNtLoadAsDllPpiGuid;

#endif
