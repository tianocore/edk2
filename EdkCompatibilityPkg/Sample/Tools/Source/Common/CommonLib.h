/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CommonLib.h

Abstract:

  Common library assistance routines.

--*/

#ifndef _EFI_COMMON_LIB_H
#define _EFI_COMMON_LIB_H

#include "TianoCommon.h"

#define PRINTED_GUID_BUFFER_SIZE  37  // including null-termination
//
// Function declarations
//
VOID
PeiZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  );

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  );

VOID
ZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  );

VOID
CopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  );

INTN
CompareGuid (
  IN EFI_GUID     *Guid1,
  IN EFI_GUID     *Guid2
  );

EFI_STATUS
GetFileImage (
  IN CHAR8    *InputFileName,
  OUT CHAR8   **InputFileImage,
  OUT UINT32  *BytesRead
  );

UINT8
CalculateChecksum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  );

UINT8
CalculateSum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  );

UINT16
CalculateChecksum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  );

UINT16
CalculateSum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  );

EFI_STATUS
PrintGuid (
  IN EFI_GUID                     *Guid
  );

#define PRINTED_GUID_BUFFER_SIZE  37  // including null-termination
EFI_STATUS
PrintGuidToBuffer (
  IN EFI_GUID     *Guid,
  IN OUT UINT8    *Buffer,
  IN UINT32       BufferLen,
  IN BOOLEAN      Uppercase
  );

#endif
