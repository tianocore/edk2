/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscSubclassDriver.h

Abstract:

  Header file for MiscSubclass Driver.

--*/

#ifndef _MISC_SUBCLASS_DRIVER_H
#define _MISC_SUBCLASS_DRIVER_H


//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include <MiscDevicePath.h>

//
// Autogen string file
//
#include <MiscSubclassStrDefs.h>

//
// Data table entry update function.
//
typedef 
EFI_STATUS 
(EFIAPI EFI_MISC_SUBCLASS_DATA_FUNCTION) (
  IN UINT16                         RecordType,
  IN UINT32                         *RecordLen,
  IN OUT EFI_MISC_SUBCLASS_RECORDS  *RecordData,
  OUT BOOLEAN                       *LogRecordData
  );

//
// Data table entry definition.
//
typedef struct {
  UINT16                          RecordType;
  UINT32                          RecordLen;
  VOID                            *RecordData;
  EFI_MISC_SUBCLASS_DATA_FUNCTION *Function;
} EFI_MISC_SUBCLASS_DATA_TABLE;

//
// Data Table extern definitions.
//
#define MISC_SUBCLASS_TABLE_EXTERNS(NAME1, NAME2) \
  extern NAME1 NAME2 ## Data; \
  extern EFI_MISC_SUBCLASS_DATA_FUNCTION NAME2 ## Function

//
// Data Table entries
//
#define MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(NAME1, NAME2) { \
	  NAME1 ##  _RECORD_NUMBER, sizeof (NAME1 ## _DATA), &NAME2 ## Data, NULL \
  }

#define MISC_SUBCLASS_TABLE_ENTRY_FUNCTION_ONLY(NAME1, NAME2) \
  { \
    NAME1 ##  _RECORD_NUMBER, 0, NULL, &NAME2 ## Function \
  }

#define MISC_SUBCLASS_TABLE_ENTRY_DATA_AND_FUNCTION(NAME1, NAME2, NAME3) \
  { \
    NAME1 ##  _RECORD_NUMBER, sizeof (NAME1 ## _DATA), &NAME2 ## Data, &NAME3 ## Function \
  }

//
// Global definition macros.
//
#define MISC_SUBCLASS_TABLE_DATA(NAME1, NAME2)  NAME1 NAME2 ## Data

#define MISC_SUBCLASS_TABLE_FUNCTION(NAME2) \
  EFI_STATUS EFIAPI NAME2 ## Function ( \
  IN UINT16 RecordType, \
  IN UINT32 *RecordLen, \
  IN OUT EFI_MISC_SUBCLASS_RECORDS * RecordData, \
  OUT BOOLEAN *LogRecordData \
  )

//
// Data Table Array
//
extern EFI_MISC_SUBCLASS_DATA_TABLE mMiscSubclassDataTable[];

//
// Data Table Array Entries
//
extern UINTN  mMiscSubclassDataTableEntries;

#endif /* _MISC_SUBCLASS_DRIVER_H */

/* eof - MiscSubclassDriver.h */
