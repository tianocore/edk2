/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  BiosIdLib.h

Abstract:

  BIOS ID library definitions.

  This library provides functions to get BIOS ID, VERSION, DATE and TIME

--*/

#ifndef _BIOS_ID_LIB_H_
#define _BIOS_ID_LIB_H_

//
// BIOS ID string format:
//
// $(BOARD_ID)$(BOARD_REV).$(OEM_ID).$(VERSION_MAJOR).$(BUILD_TYPE)$(VERSION_MINOR).YYMMDDHHMM
//
// Example: "TRFTCRB1.86C.0008.D03.0506081529"
//
#pragma pack(1)

typedef struct {
  CHAR16  BoardId[7];               // "TRFTCRB"
  CHAR16  BoardRev;                 // "1"
  CHAR16  Dot1;                     // "."
  CHAR16  OemId[3];                 // "86C"
  CHAR16  Dot2;                     // "."
  CHAR16  VersionMajor[4];          // "0008"
  CHAR16  Dot3;                     // "."
  CHAR16  BuildType;                // "D"
  CHAR16  VersionMinor[2];          // "03"
  CHAR16  Dot4;                     // "."
  CHAR16  TimeStamp[10];            // "YYMMDDHHMM"
  CHAR16  NullTerminator;           // 0x0000
} BIOS_ID_STRING;

#define MEM_IFWIVER_START           0x7E0000
#define MEM_IFWIVER_LENGTH          0x1000

typedef struct _MANIFEST_OEM_DATA{
  UINT32         Signature;
  unsigned char  FillNull[0x39];
  UINT32         IFWIVersionLen;
  unsigned char  IFWIVersion[32];
}MANIFEST_OEM_DATA;

//
// A signature precedes the BIOS ID string in the FV to enable search by external tools.
//
typedef struct {
  UINT8           Signature[8];     // "$IBIOSI$"
  BIOS_ID_STRING  BiosIdString;     // "TRFTCRB1.86C.0008.D03.0506081529"
} BIOS_ID_IMAGE;

#pragma pack()

/**
  This function returns BIOS ID by searching HOB or FV.

  @param[in]  BiosIdImage         The BIOS ID got from HOB or FV

  @retval  EFI_SUCCESS            All parameters were valid and BIOS ID has been got.
  @retval  EFI_NOT_FOUND          BiosId image is not found, and no parameter will be modified.
  @retval  EFI_INVALID_PARAMETER  The parameter is NULL.

**/
EFI_STATUS
GetBiosId (
  OUT BIOS_ID_IMAGE     *BiosIdImage
  );

/**
  This function returns the Version & Release Date and Time by getting and converting
  BIOS ID.

  @param[in] BiosVersion         The Bios Version out of the conversion.
  @param[in] BiosReleaseDate     The Bios Release Date out of the conversion.
  @param[in] BiosReleaseTime     The Bios Release Time out of the conversion.

  @retval EFI_SUCCESS            BIOS Version & Release Date and Time have been got successfully.
  @retval EFI_NOT_FOUND          BiosId image is not found, and no parameter will be modified.
  @retval EFI_INVALID_PARAMETER  All the parameters are NULL.

**/
EFI_STATUS
GetBiosVersionDateTime (
  OUT CHAR16    *BiosVersion, OPTIONAL
  OUT CHAR16    *BiosReleaseDate, OPTIONAL
  OUT CHAR16    *BiosReleaseTime OPTIONAL
  );

#endif
