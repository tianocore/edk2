/*++

Copyright (c)  2004 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  FlashDefFile.h

Abstract:

  Header file for flash management utility in the Intel Platform 
  Innovation Framework for EFI build environment.

--*/

#ifndef _FLASH_DEF_FILE_H_
#define _FLASH_DEF_FILE_H_

#ifdef __cplusplus
extern "C"
{
#endif

void
FDFConstructor (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

void
FDFDestructor (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFParseFile (
  char    *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FileName  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFCreateCIncludeFile (
  char      *FlashDeviceName,
  char      *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FlashDeviceName - GC_TODO: add argument description
  FileName        - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFCreateCFlashMapDataFile (
  char      *FlashDeviceName,
  char      *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FlashDeviceName - GC_TODO: add argument description
  FileName        - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFCreateAsmIncludeFile (
  char      *FlashDeviceName,
  char      *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FlashDeviceName - GC_TODO: add argument description
  FileName        - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFParseFile (
  char      *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FileName  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFCreateImage (
  char      *FlashDeviceName,
  char      *ImageName,
  char      *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FlashDeviceName - GC_TODO: add argument description
  ImageName       - GC_TODO: add argument description
  FileName        - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFCreateDscFile (
  char      *FlashDeviceName,
  char      *FileName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FlashDeviceName - GC_TODO: add argument description
  FileName        - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDFCreateSymbols (
  char      *FlashDeviceName
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FlashDeviceName - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

STATUS
FDDiscover (
  char          *FDFileName,
  unsigned int  BaseAddr
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FDFileName  - GC_TODO: add argument description
  BaseAddr    - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#ifdef __cplusplus
}
#endif

#endif // #ifndef _FLASH_DEF_FILE_H_
