/** @file
 Define the GUID gEdkiiFaultTolerantWriteGuid that will be used to build
 FAULT_TOLERANT_WRITE_LAST_WRITE_DATA GUID hob and install PPI to inform the check
 for FTW last write data has been done. The GUID hob will be only built if FTW last write was
 still in progress with SpareComplete set and DestinationComplete not set.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/

#ifndef _FAULT_TOLERANT_WRITE_H_
#define _FAULT_TOLERANT_WRITE_H_

#define EDKII_FAULT_TOLERANT_WRITE_GUID \
  { \
    0x1d3e9cb8, 0x43af, 0x490b, { 0x83,  0xa, 0x35, 0x16, 0xaa, 0x53, 0x20, 0x47 } \
  }

//
// FTW Last write data. It will be used as gEdkiiFaultTolerantWriteGuid GUID hob data.
//
typedef struct {
  ///
  /// Target address to be updated in FTW last write.
  ///
  EFI_PHYSICAL_ADDRESS      TargetAddress;
  ///
  /// Spare address to back up the updated buffer.
  ///
  EFI_PHYSICAL_ADDRESS      SpareAddress;
  ///
  /// The length of data that have been backed up in spare block.
  /// It is also the length of target block that has been erased.
  ///
  UINT64                    Length;
} FAULT_TOLERANT_WRITE_LAST_WRITE_DATA;

//
// This GUID will be used to install PPI to inform the check for FTW last write data has been done.
// The related FAULT_TOLERANT_WRITE_LAST_WRITE_DATA GUID hob will be only built if
// FTW last write was still in progress with SpareComplete set and DestinationComplete not set.
// It means the target buffer has been backed up in spare block, then target block has been erased,
// but the target buffer has not been writen in target block from spare block.
//
extern EFI_GUID gEdkiiFaultTolerantWriteGuid;

#endif
