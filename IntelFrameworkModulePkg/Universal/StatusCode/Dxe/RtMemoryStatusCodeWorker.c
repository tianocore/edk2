/** @file
  Runtime memory status code worker in DXE.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "DxeStatusCode.h"

/**
  Initialize runtime memory status code.
 
  @return  The function always return EFI_SUCCESS

**/
EFI_STATUS
RtMemoryStatusCodeInitializeWorker (
  VOID
  )
{
  RUNTIME_MEMORY_STATUSCODE_HEADER  *RtMemoryStatusCodeTable;

  //
  // Allocate runtime memory status code pool.
  //
  RtMemoryStatusCodeTable = 
    (RUNTIME_MEMORY_STATUSCODE_HEADER *) AllocateRuntimePool (
                                           sizeof (RUNTIME_MEMORY_STATUSCODE_HEADER) +
                                           PcdGet16 (PcdStatusCodeRuntimeMemorySize) *
                                           1024
                                           );

  ASSERT (NULL != RtMemoryStatusCodeTable);

  RtMemoryStatusCodeTable->RecordIndex                  = 0;
  RtMemoryStatusCodeTable->NumberOfRecords              = 0;
  RtMemoryStatusCodeTable->MaxRecordsNumber             = 
    (PcdGet16 (PcdStatusCodeRuntimeMemorySize) * 1024) / sizeof (MEMORY_STATUSCODE_RECORD);

  gDxeStatusCode.RtMemoryStatusCodeTable[PHYSICAL_MODE] = RtMemoryStatusCodeTable;
  return EFI_SUCCESS;
}


/**
  Report status code into runtime memory. If the runtime pool is full, roll back to the 
  first record and overwrite it.
 
  @param  RtMemoryStatusCodeTable      
                        Point to Runtime memory table header.

  @param  CodeType      Indicates the type of status code being reported.  Type EFI_STATUS_CODE_TYPE is defined in "Related Definitions" below.
 
  @param  Value         Describes the current status of a hardware or software entity.  
                        This included information about the class and subclass that is used to classify the entity 
                        as well as an operation.  For progress codes, the operation is the current activity. 
                        For error codes, it is the exception.  For debug codes, it is not defined at this time. 
                        Type EFI_STATUS_CODE_VALUE is defined in "Related Definitions" below.  
                        Specific values are discussed in the Intel? Platform Innovation Framework for EFI Status Code Specification.
 
  @param  Instance      The enumeration of a hardware or software entity within the system.  
                        A system may contain multiple entities that match a class/subclass pairing. 
                        The instance differentiates between them.  An instance of 0 indicates that instance information is unavailable, 
                        not meaningful, or not relevant.  Valid instance numbers start with 1.
 
  @return               The function always return EFI_SUCCESS.

**/
EFI_STATUS
RtMemoryStatusCodeReportWorker (
  RUNTIME_MEMORY_STATUSCODE_HEADER      *RtMemoryStatusCodeTable,
  IN EFI_STATUS_CODE_TYPE               CodeType,
  IN EFI_STATUS_CODE_VALUE              Value,
  IN UINT32                             Instance
  )
{
  MEMORY_STATUSCODE_RECORD              *Record;

  ASSERT (NULL != RtMemoryStatusCodeTable);

  //
  // Locate current record buffer.
  //
  Record                  = (MEMORY_STATUSCODE_RECORD *) (RtMemoryStatusCodeTable + 1);
  Record                  = &Record[RtMemoryStatusCodeTable->RecordIndex++];

  //
  // Save status code.
  //
  Record->CodeType        = CodeType;
  Record->Value           = Value;
  Record->Instance        = Instance;

  //
  // Record total number of records, we compare the number with max records number,
  // if it is bigger than the max number, then the roll back had happened, the record index points to 
  // the first record. if it is less then max number, then the zero index is the first record.
  //
  RtMemoryStatusCodeTable->NumberOfRecords++;
  if (RtMemoryStatusCodeTable->RecordIndex == RtMemoryStatusCodeTable->MaxRecordsNumber) {
    //
    // Roll back record index.
    //
    RtMemoryStatusCodeTable->RecordIndex = 0;
  }

  return EFI_SUCCESS;
}



