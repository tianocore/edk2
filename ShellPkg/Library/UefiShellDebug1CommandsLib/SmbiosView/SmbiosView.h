/** @file
  Tools of clarify the content of the smbios table.

  Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMBIOS_VIEW_H_
#define _SMBIOS_VIEW_H_

#define STRUCTURE_TYPE_RANDOM   (UINT8) 0xFE
#define STRUCTURE_TYPE_INVALID  (UINT8) 0xFF

typedef struct {
  UINT16    Index;
  UINT8     Type;
  UINT16    Handle;
  UINT16    Addr;   // offset from table head
  UINT16    Length; // total structure length
} STRUCTURE_STATISTICS;

/**
  Query all structures Data from SMBIOS table and Display
  the information to users as required display option.

  @param[in] QueryType      Structure type to view.
  @param[in] QueryHandle    Structure handle to view.
  @param[in] Option         Display option: none,outline,normal,detail.
  @param[in] RandomView     Support for -h parameter.

  @retval EFI_SUCCESS           print is successful.
  @retval EFI_BAD_BUFFER_SIZE   structure is out of the range of SMBIOS table.
**/
EFI_STATUS
SMBiosView (
  IN  UINT8    QueryType,
  IN  UINT16   QueryHandle,
  IN  UINT8    Option,
  IN  BOOLEAN  RandomView
  );

/**
  Query all structures Data from SMBIOS table and Display
  the information to users as required display option.

  @param[in] QueryType      Structure type to view.
  @param[in] QueryHandle    Structure handle to view.
  @param[in] Option         Display option: none,outline,normal,detail.
  @param[in] RandomView     Support for -h parameter.

  @retval EFI_SUCCESS           print is successful.
  @retval EFI_BAD_BUFFER_SIZE   structure is out of the range of SMBIOS table.
**/
EFI_STATUS
SMBios64View (
  IN  UINT8    QueryType,
  IN  UINT16   QueryHandle,
  IN  UINT8    Option,
  IN  BOOLEAN  RandomView
  );

/**
  Function to initialize the global mStatisticsTable object.

  @retval EFI_SUCCESS           print is successful.
**/
EFI_STATUS
InitSmbiosTableStatistics (
  VOID
  );

/**
  Function to initialize the global mSmbios64BitStatisticsTable object.

  @retval EFI_SUCCESS           print is successful.
**/
EFI_STATUS
InitSmbios64BitTableStatistics (
  VOID
  );

/**
  Function to display the global mStatisticsTable object.

  @param[in] Option             ECHO, NORMAL, or DETAIL control the amount of detail displayed.

  @retval EFI_SUCCESS           print is successful.
**/
EFI_STATUS
DisplayStatisticsTable (
  IN UINT8  Option
  );

/**
  Function to display the global mSmbios64BitStatisticsTable object.

  @param[in] Option             ECHO, NORMAL, or DETAIL control the amount of detail displayed.

  @retval EFI_SUCCESS           print is successful.
**/
EFI_STATUS
DisplaySmbios64BitStatisticsTable (
  IN UINT8  Option
  );

/**
  function to return a string of the detail level.

  @param[in] ShowType         The detail level whose name is desired in clear text.

  @return   A pointer to a string representing the ShowType (or 'undefined type' if not known).
**/
CHAR16 *
GetShowTypeString (
  UINT8  ShowType
  );

extern UINT8  gShowType;

extern UINTN  mSmbios64BitTableLength;

#endif
