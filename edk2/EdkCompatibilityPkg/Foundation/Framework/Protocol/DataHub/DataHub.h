/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    DataHub.h

Abstract:

    The data hub protocol is used both by agents wishing to log
    data and those wishing to be made aware of all information that
    has been logged.

    For more information please look at Intel Platform Innovation 
    Framework for EFI Data Hub Specification.

--*/

#ifndef __DATA_HUB_H__
#define __DATA_HUB_H__

#define EFI_DATA_HUB_PROTOCOL_GUID \
  { \
    0xae80d021, 0x618e, 0x11d4, 0xbc, 0xd7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 \
  }

//
// EFI generic Data Hub Header
//
// A Data Record is an EFI_DATA_RECORD_HEADER followed by RecordSize bytes of
//  data. The format of the data is defined by the DataRecordGuid.
//
// If EFI_DATA_RECORD_HEADER is extended in the future the Version number must
//  change and the HeaderSize will change if the definition of
//  EFI_DATA_RECORD_HEADER is extended.
//
// The logger is responcible for initializing:
//  Version, HeaderSize, RecordSize, DataRecordGuid, DataRecordClass
//
// The Data Hub driver is responcible for initializing:
//   LogTime and LogMonotonicCount.
//
#define EFI_DATA_RECORD_HEADER_VERSION  0x0100
typedef struct {
  UINT16    Version;
  UINT16    HeaderSize;
  UINT32    RecordSize;
  EFI_GUID  DataRecordGuid;
  EFI_GUID  ProducerName;
  UINT64    DataRecordClass;
  EFI_TIME  LogTime;
  UINT64    LogMonotonicCount;
} EFI_DATA_RECORD_HEADER;

//
// Definition of DataRecordClass. These are used to filter out class types
// at a very high level. The DataRecordGuid still defines the format of
// the data. See DateHub.doc for rules on what can and can not be a
// new DataRecordClass
//
#define EFI_DATA_RECORD_CLASS_DEBUG         0x0000000000000001
#define EFI_DATA_RECORD_CLASS_ERROR         0x0000000000000002
#define EFI_DATA_RECORD_CLASS_DATA          0x0000000000000004
#define EFI_DATA_RECORD_CLASS_PROGRESS_CODE 0x0000000000000008

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_DATA_HUB_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_LOG_DATA) (
  IN  EFI_DATA_HUB_PROTOCOL   * This,
  IN  EFI_GUID                * DataRecordGuid,
  IN  EFI_GUID                * ProducerName,
  IN  UINT64                  DataRecordClass,
  IN  VOID                    *RawData,
  IN  UINT32                  RawDataSize
  );

typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_GET_NEXT_RECORD) (
  IN EFI_DATA_HUB_PROTOCOL    * This,
  IN OUT  UINT64              *MonotonicCount,
  IN  EFI_EVENT               * FilterDriver OPTIONAL,
  OUT EFI_DATA_RECORD_HEADER  **Record
  );

typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_REGISTER_FILTER_DRIVER) (
  IN EFI_DATA_HUB_PROTOCOL    * This,
  IN EFI_EVENT                FilterEvent,
  IN EFI_TPL                  FilterTpl,
  IN UINT64                   FilterClass,
  IN EFI_GUID                 * FilterDataRecordGuid OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_UNREGISTER_FILTER_DRIVER) (
  IN EFI_DATA_HUB_PROTOCOL    * This,
  IN EFI_EVENT                FilterEvent
  );

typedef struct _EFI_DATA_HUB_PROTOCOL {
  EFI_DATA_HUB_LOG_DATA                 LogData;
  EFI_DATA_HUB_GET_NEXT_RECORD          GetNextRecord;
  EFI_DATA_HUB_REGISTER_FILTER_DRIVER   RegisterFilterDriver;
  EFI_DATA_HUB_UNREGISTER_FILTER_DRIVER UnregisterFilterDriver;
} EFI_DATA_HUB_PROTOCOL;

extern EFI_GUID gEfiDataHubProtocolGuid;

#endif
