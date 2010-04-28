/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  GenFvImageLib.h

Abstract:

  This file contains describes the public interfaces to the GenFvImage Library.
  The basic purpose of the library is to create Firmware Volume images.

--*/

#ifndef _EFI_GEN_FV_IMAGE_LIB_H
#define _EFI_GEN_FV_IMAGE_LIB_H

//
// Include files
//
#include <windows.h>
#include "ParseInf.h"

//
// Following definition is used for FIT in IPF
//
#define COMP_TYPE_FIT_PEICORE 0x10
#define COMP_TYPE_FIT_UNUSED  0x7F

#define FIT_TYPE_MASK         0x7F
#define CHECKSUM_BIT_MASK     0x80

#pragma pack(1)

typedef struct {
  UINT64  CompAddress;
  UINT32  CompSize;
  UINT16  CompVersion;
  UINT8   CvAndType;
  UINT8   CheckSum;
} FIT_TABLE;

#pragma pack()
//
// Exported function prototypes
//
EFI_STATUS
GenerateFvImage (
  IN CHAR8    *InfFileImage,
  IN UINTN    InfFileSize,
  OUT UINT8   **FvImage,
  OUT UINTN   *FvImageSize,
  OUT CHAR8   **FvFileName,
  OUT UINT8   **SymImage,
  OUT UINTN   *SymImageSize,
  OUT CHAR8   **SymFileName
  );

/*++

Routine Description:

  This is the main function which will be called from application.

Arguments:

  InfFileImage  Buffer containing the INF file contents.
  InfFileSize   Size of the contents of the InfFileImage buffer.
  FvImage       Pointer to the FV image created.
  FvImageSize   Size of the FV image created and pointed to by FvImage.
  FvFileName    Requested name for the FV file.
  SymImage      Pointer to the Sym image created.
  SymImageSize  Size of the Sym image created and pointed to by SymImage.
  SymFileName   Requested name for the Sym file.
    
Returns:
 
  EFI_SUCCESS             Function completed successfully.
  EFI_OUT_OF_RESOURCES    Could not allocate required resources.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.

--*/
EFI_STATUS
UpdatePeiCoreEntryInFit (
  IN FIT_TABLE     *FitTablePtr,
  IN UINT64        PeiCorePhysicalAddress
  );

/*++

Routine Description:

  This function is used to update the Pei Core address in FIT, this can be used by Sec core to pass control from
  Sec to Pei Core

Arguments:

  FitTablePtr             - The pointer of FIT_TABLE.
  PeiCorePhysicalAddress  - The address of Pei Core entry.

Returns:

  EFI_SUCCESS             - The PEI_CORE FIT entry was updated successfully.
  EFI_NOT_FOUND           - Not found the PEI_CORE FIT entry.

--*/
VOID
UpdateFitCheckSum (
  IN FIT_TABLE   *FitTablePtr
  );

/*++

Routine Description:

  This function is used to update the checksum for FIT.


Arguments:

  FitTablePtr             - The pointer of FIT_TABLE.

Returns:

  None.

--*/
#endif
