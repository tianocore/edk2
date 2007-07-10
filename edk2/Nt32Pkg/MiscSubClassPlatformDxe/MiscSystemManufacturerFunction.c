/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscSystemManufacturerFunction.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

--*/

#include "MiscSubclassDriver.h"

//
//
//
MISC_SUBCLASS_TABLE_FUNCTION (
  MiscSystemManufacturer
  )
/*++
Description:

  This function makes boot time changes to the contents of the
  MiscSystemManufacturer (Type 13).

Parameters:

  RecordType
    Type of record to be processed from the Data Table.
    mMiscSubclassDataTable[].RecordType

  RecordLen
    Size of static RecordData from the Data Table.
    mMiscSubclassDataTable[].RecordLen

  RecordData
    Pointer to copy of RecordData from the Data Table.  Changes made
    to this copy will be written to the Data Hub but will not alter
    the contents of the static Data Table.

  LogRecordData
    Set *LogRecordData to TRUE to log RecordData to Data Hub.
    Set *LogRecordData to FALSE when there is no more data to log.

Returns:

  EFI_SUCCESS
    All parameters were valid and *RecordData and *LogRecordData have
    been set.

  EFI_UNSUPPORTED
    Unexpected RecordType value.

  EFI_INVALID_PARAMETER
    One of the following parameter conditions was true:
      RecordLen was zero.
      RecordData was NULL.
      LogRecordData was NULL.
--*/
{
  STATIC BOOLEAN  Done = FALSE;

  //
  // First check for invalid parameters.
  //
  if (*RecordLen == 0 || RecordData == NULL || LogRecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Then check for unsupported RecordType.
  //
  if (RecordType != EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER) {
    return EFI_UNSUPPORTED;
  }
  //
  // Is this the first time through this function?
  //
  if (!Done) {
    //
    // Yes, this is the first time.  Inspect/Change the contents of the
    // RecordData structure.
    //
    //
    // Set system GUID.
    //
    // ((EFI_MISC_SYSTEM_MANUFACTURER_DATA *)RecordData)->SystemUuid = %%TBD
    //
    // Set power-on type.
    //
    // ((EFI_MISC_SYSTEM_MANUFACTURER_DATA *)RecordData)->SystemWakeupType = %%TBD
    //
    // Set Done flag to TRUE for next pass through this function.
    // Set *LogRecordData to TRUE so data will get logged to Data Hub.
    //
    Done            = TRUE;
    *LogRecordData  = TRUE;
  } else {
    //
    // No, this is the second time.  Reset the state of the Done flag
    // to FALSE and tell the data logger that there is no more data
    // to be logged for this record type.  If any memory allocations
    // were made by earlier passes, they must be released now.
    //
    Done            = FALSE;
    *LogRecordData  = FALSE;
  }

  return EFI_SUCCESS;
}

/* eof - MiscSystemManufacturerFunction.c */
