/*++

Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscPhysicalArrayFunction.c

Abstract:

  BIOS system Physical Array boot time changes.
  SMBIOS type 16.

--*/


#include "CommonHeader.h"
#include "MiscSubclassDriver.h"



/**
  This function makes boot time changes to the contents of the
  MiscPhysicalArrayFunction (Type 16).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/

MISC_SMBIOS_TABLE_FUNCTION(MiscPhysicalMemoryArray)
{
    EFI_STATUS                      Status;
    EFI_SMBIOS_HANDLE               SmbiosHandle;
    SMBIOS_TABLE_TYPE16             *SmbiosRecord;
    EFI_MEMORY_ARRAY_LOCATION_DATA  *ForType16InputData;
    UINT32                           TopOfMemory = 8 * 1024 * 1024;

    //
    // First check for invalid parameters.
    //
    if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
    }

    ForType16InputData = (EFI_MEMORY_ARRAY_LOCATION_DATA *)RecordData;

    //
    // Two zeros following the last string.
    //
    SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE16)  + 1);
    ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE16)  + 1);

    SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE16);

    //
    // Make handle chosen by smbios protocol.add automatically.
    //
    SmbiosRecord->Hdr.Handle = 0;

    //
    // ReleaseDate will be the 3rd optional string following the formatted structure.
    //
    SmbiosRecord->Location = *(UINT8 *) &ForType16InputData ->MemoryArrayLocation;
    SmbiosRecord->Use = *(UINT8 *) &ForType16InputData ->MemoryArrayUse;
    SmbiosRecord->MemoryErrorCorrection = *(UINT8 *) &ForType16InputData->MemoryErrorCorrection;

    //
    // System does not provide the error information structure
    //
    SmbiosRecord->MemoryErrorInformationHandle = 0xFFFE;

    //
    // Maximum memory capacity
    //
    SmbiosRecord-> MaximumCapacity = TopOfMemory;
    SmbiosRecord-> NumberOfMemoryDevices= 0x02;

    //
    // Now we have got the full smbios record, call smbios protocol to add this record.
    //
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = Smbios-> Add(
                        Smbios,
                        NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                       );
    FreePool(SmbiosRecord);
    return Status;

}
