/** @file
  This file defines the hob structure for board related information from acpi table
  
  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ACPI_BOARD_INFO_GUID_H__
#define __ACPI_BOARD_INFO_GUID_H__

///
/// Board information GUID
///
extern EFI_GUID gUefiAcpiBoardInfoGuid;

typedef struct {
  UINT64             PmEvtBase;
  UINT64             PmGpeEnBase;
  UINT64             PmCtrlRegBase;
  UINT64             PmTimerRegBase;
  UINT64             ResetRegAddress;
  UINT8              ResetValue;      
} ACPI_BOARD_INFO;
  
#endif
