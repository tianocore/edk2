/** @file

  Defines data types and constants introduced in UEFI.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UEFI_BASETYPE_H__
#define __UEFI_BASETYPE_H__

#include "Base.h"

//
// Basical data type definitions introduced in UEFI.
// 
typedef GUID                      EFI_GUID;
///
/// Function return status for EFI API
///
typedef RETURN_STATUS             EFI_STATUS;
typedef VOID                      *EFI_HANDLE;

typedef VOID                      *EFI_EVENT;

typedef UINTN                     EFI_TPL;


typedef UINT64                    EFI_LBA;


typedef UINT16                    STRING_REF;

typedef UINT64                    EFI_PHYSICAL_ADDRESS;
typedef UINT64                    EFI_VIRTUAL_ADDRESS;

//
// EFI Time Abstraction:
//  Year:       2000 - 20XX
//  Month:      1 - 12
//  Day:        1 - 31
//  Hour:       0 - 23
//  Minute:     0 - 59
//  Second:     0 - 59
//  Nanosecond: 0 - 999,999,999
//  TimeZone:   -1440 to 1440 or 2047
//
typedef struct {
  UINT16  Year;
  UINT8   Month;
  UINT8   Day;
  UINT8   Hour;
  UINT8   Minute;
  UINT8   Second;
  UINT8   Pad1;
  UINT32  Nanosecond;
  INT16   TimeZone;
  UINT8   Daylight;
  UINT8   Pad2;
} EFI_TIME;


//
// Networking Definitions
//
typedef struct {
  UINT8 Addr[4];
} EFI_IPv4_ADDRESS;

typedef struct {
  UINT8 Addr[16];
} EFI_IPv6_ADDRESS;

typedef struct {
  UINT8 Addr[32];
} EFI_MAC_ADDRESS;

typedef union {
  UINT32            Addr[4];
  EFI_IPv4_ADDRESS  v4;
  EFI_IPv6_ADDRESS  v6;
} EFI_IP_ADDRESS;


//
// Enumeration of EFI_STATUS.
// 
#define EFI_SUCCESS               RETURN_SUCCESS              
#define EFI_LOAD_ERROR            RETURN_LOAD_ERROR           
#define EFI_INVALID_PARAMETER     RETURN_INVALID_PARAMETER    
#define EFI_UNSUPPORTED           RETURN_UNSUPPORTED          
#define EFI_BAD_BUFFER_SIZE       RETURN_BAD_BUFFER_SIZE      
#define EFI_BUFFER_TOO_SMALL      RETURN_BUFFER_TOO_SMALL     
#define EFI_NOT_READY             RETURN_NOT_READY            
#define EFI_DEVICE_ERROR          RETURN_DEVICE_ERROR         
#define EFI_WRITE_PROTECTED       RETURN_WRITE_PROTECTED      
#define EFI_OUT_OF_RESOURCES      RETURN_OUT_OF_RESOURCES     
#define EFI_VOLUME_CORRUPTED      RETURN_VOLUME_CORRUPTED     
#define EFI_VOLUME_FULL           RETURN_VOLUME_FULL          
#define EFI_NO_MEDIA              RETURN_NO_MEDIA             
#define EFI_MEDIA_CHANGED         RETURN_MEDIA_CHANGED        
#define EFI_NOT_FOUND             RETURN_NOT_FOUND            
#define EFI_ACCESS_DENIED         RETURN_ACCESS_DENIED        
#define EFI_NO_RESPONSE           RETURN_NO_RESPONSE          
#define EFI_NO_MAPPING            RETURN_NO_MAPPING           
#define EFI_TIMEOUT               RETURN_TIMEOUT              
#define EFI_NOT_STARTED           RETURN_NOT_STARTED          
#define EFI_ALREADY_STARTED       RETURN_ALREADY_STARTED      
#define EFI_ABORTED               RETURN_ABORTED              
#define EFI_ICMP_ERROR            RETURN_ICMP_ERROR           
#define EFI_TFTP_ERROR            RETURN_TFTP_ERROR           
#define EFI_PROTOCOL_ERROR        RETURN_PROTOCOL_ERROR       
#define EFI_INCOMPATIBLE_VERSION  RETURN_INCOMPATIBLE_VERSION 
#define EFI_SECURITY_VIOLATION    RETURN_SECURITY_VIOLATION   
#define EFI_CRC_ERROR             RETURN_CRC_ERROR   
#define EFI_END_OF_MEDIA          RETURN_END_OF_MEDIA
#define EFI_END_OF_FILE           RETURN_END_OF_FILE

#define EFI_WARN_UNKNOWN_GLYPH    RETURN_WARN_UNKNOWN_GLYPH   
#define EFI_WARN_DELETE_FAILURE   RETURN_WARN_DELETE_FAILURE  
#define EFI_WARN_WRITE_FAILURE    RETURN_WARN_WRITE_FAILURE   
#define EFI_WARN_BUFFER_TOO_SMALL RETURN_WARN_BUFFER_TOO_SMALL


#define NULL_HANDLE               ((VOID *) 0)

//
// Define macro to encode the status code.
// 
#define EFIERR(_a)                ENCODE_ERROR(_a)

#define EFI_ERROR(A)              RETURN_ERROR(A)

//
// Define macros to build data structure signatures from characters.
//
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))
#define EFI_SIGNATURE_64(A, B, C, D, E, F, G, H) \
    (EFI_SIGNATURE_32 (A, B, C, D) | ((UINT64) (EFI_SIGNATURE_32 (E, F, G, H)) << 32))


//
//  Returns the byte offset to a field within a structure
//
#define EFI_FIELD_OFFSET(TYPE,Field) ((UINTN)(&(((TYPE *) 0)->Field)))

//
// The EFI memory allocation functions work in units of EFI_PAGEs that are
// 4K. This should in no way be confused with the page size of the processor.
// An EFI_PAGE is just the quanta of memory in EFI.
//
#define EFI_PAGE_SIZE             0x1000
#define EFI_PAGE_MASK             0xFFF
#define EFI_PAGE_SHIFT            12

#define EFI_SIZE_TO_PAGES(a)  (((a) >> EFI_PAGE_SHIFT) + (((a) & EFI_PAGE_MASK) ? 1 : 0))

#define EFI_PAGES_TO_SIZE(a)   ( (a) << EFI_PAGE_SHIFT)


#define EFI_MAX_BIT               MAX_BIT
#define EFI_MAX_ADDRESS           MAX_ADDRESS

#endif
