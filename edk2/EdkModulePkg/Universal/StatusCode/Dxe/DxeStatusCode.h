/*++

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DxeStatusCode.h

Abstract:

  Header file of EFI DXE/RT Status Code.

--*/

#ifndef __DXE_STATUS_CODE_H__
#define __DXE_STATUS_CODE_H__

#include <Common/StatusCode.h>

//
// Data hub worker definition 
//
#define MAX_NUMBER_DATAHUB_RECORDS                1000
#define DATAHUB_BYTES_PER_RECORD                  EFI_STATUS_CODE_DATA_MAX_SIZE
#define EMPTY_RECORD_TAG                          0xFF
#define DATAHUB_STATUS_CODE_SIGNATURE             EFI_SIGNATURE_32 ('B', 'D', 'H', 'S')

//
// Address type of pointer.
// The point type always equal to PHYSICAL_MODE on IA32/X64/EBC architecture
// Otherwise, VIRTUAL_MODE/PHYSICAL_MODE would be used on Ipf architecture, 
// 
typedef enum {
  PHYSICAL_MODE,
  VIRTUAL_MODE
} PROCESSOR_MODE;

typedef struct {
  UINTN       Signature;
  LIST_ENTRY  Node;

  UINT8       Data[sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD) + EFI_STATUS_CODE_DATA_MAX_SIZE];
} DATAHUB_STATUSCODE_RECORD;


//
// Runtime memory status code worker definition 
// 
typedef struct {
  UINT32   RecordIndex;
  UINT32   NumberOfRecords;
  UINT32   MaxRecordsNumber;
} RUNTIME_MEMORY_STATUSCODE_HEADER;


typedef struct {
  //
  // Report operation nest status. 
  // If it is set, then the report operation has nested.
  // 
  UINT32                            StatusCodeNestStatus;
  //
  // Runtime status code management header, the records buffer is following it.
  // 
  RUNTIME_MEMORY_STATUSCODE_HEADER  *RtMemoryStatusCodeTable[2];
} DXE_STATUS_CODE_CONTROLLER;


/**
  
  Dispatch initialization request to sub status code devices based on 
  customized feature flags.
 
**/
VOID
InitializationDispatcherWorker (
  VOID
  );


/**
  Initialize serial status code worker.
 
  @return  The function always return EFI_SUCCESS

**/
EFI_STATUS
EfiSerialStatusCodeInitializeWorker (
  VOID
  );


/**
  Convert status code value and extended data to readable ASCII string, send string to serial I/O device.
 
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


  @param  CallerId      This optional parameter may be used to identify the caller. 
                        This parameter allows the status code driver to apply different rules to different callers. 
                        Type EFI_GUID is defined in InstallProtocolInterface() in the EFI 1.10 Specification.


  @param  Data          This optional parameter may be used to pass additional data
 
  @retval EFI_SUCCESS         Success to report status code to serial I/O.
  @retval EFI_DEVICE_ERROR    EFI serial device can not work after ExitBootService() is called .

**/
EFI_STATUS
SerialStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );

/**
  Initialize runtime memory status code.
 
  @return  The function always return EFI_SUCCESS

**/
EFI_STATUS
RtMemoryStatusCodeInitializeWorker (
  VOID
  );

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
  );

/**
  Initialize data hubstatus code.
  Create a data hub listener.
 
  @return  The function always return EFI_SUCCESS

**/
EFI_STATUS
DataHubStatusCodeInitializeWorker (
  VOID
  );


/**
  Report status code into DataHub.
 
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


  @param  CallerId      This optional parameter may be used to identify the caller. 
                        This parameter allows the status code driver to apply different rules to different callers. 
                        Type EFI_GUID is defined in InstallProtocolInterface() in the EFI 1.10 Specification.


  @param  Data          This optional parameter may be used to pass additional data
 
  @retval EFI_OUT_OF_RESOURCES   Can not acquire record buffer.
  @retval EFI_DEVICE_ERROR       EFI serial device can not work after ExitBootService() is called .
  @retval EFI_SUCCESS            Success to cache status code and signal log data event.

**/
EFI_STATUS
DataHubStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );

//
// declaration of DXE status code controller.
// 
extern DXE_STATUS_CODE_CONTROLLER gDxeStatusCode;

#endif
