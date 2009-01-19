/** @file
  GUID used to identify status code records HOB that originate from the PEI status code    
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __MEMORY_STATUS_CODE_RECORD_H__
#define __MEMORY_STATUS_CODE_RECORD_H__

#define MEMORY_STATUS_CODE_RECORD_GUID \
  { \
    0x60cc026, 0x4c0d, 0x4dda, {0x8f, 0x41, 0x59, 0x5f, 0xef, 0x0, 0xa5, 0x2} \
  }

/**
  Memory status code records packet structure :
  +---------------+----------+----------+-----+----------+-----+----------+
  | Packet Header | Record 1 | Record 2 | ... + Record n | ... | Record m |
  +---------------+----------+----------+-----+----------+-----+----------+
                  ^                                 ^                     ^
                  +--------- RecordIndex -----------+                     |
                  +---------------- MaxRecordsNumber----------------------+
**/
typedef struct {
  UINT16                  PacketIndex;          // Index of the Packet.
  UINT16                  RecordIndex;          // Index of record in the packet.
  UINT32                  MaxRecordsNumber;     // Max number of records in the packet.
} MEMORY_STATUSCODE_PACKET_HEADER;

typedef struct {
  EFI_STATUS_CODE_TYPE    CodeType;
  EFI_STATUS_CODE_VALUE   Value;
  UINT32                  Instance;
} MEMORY_STATUSCODE_RECORD;


extern EFI_GUID gMemoryStatusCodeRecordGuid;

#endif
