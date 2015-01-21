/*++

Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscProcessorCacheFunction.c

Abstract:

  BIOS processor cache details.
  Misc. subclass type 7.
  SMBIOS type 7.

--*/
#include "CommonHeader.h"
#include "MiscSubclassDriver.h"
#include <Protocol/DataHub.h>
#include <Guid/DataHubRecords.h>


extern  SMBIOS_TABLE_TYPE7            *SmbiosRecordL1;
extern  SMBIOS_TABLE_TYPE7            *SmbiosRecordL2;
extern  SMBIOS_TABLE_TYPE7            *SmbiosRecordL3;


UINT32
ConvertBase2ToRaw (
  IN  EFI_EXP_BASE2_DATA             *Data)
{
  UINTN         Index;
  UINT32        RawData;

  RawData = Data->Value;
  for (Index = 0; Index < (UINTN) Data->Exponent; Index++) {
     RawData <<= 1;
  }

  return  RawData;
}


MISC_SMBIOS_TABLE_FUNCTION(MiscProcessorCache)
{
	EFI_SMBIOS_HANDLE     SmbiosHandle;
	SMBIOS_TABLE_TYPE7            *SmbiosRecordL1;
	SMBIOS_TABLE_TYPE7            *SmbiosRecordL2;

	EFI_CACHE_SRAM_TYPE_DATA      CacheSramType;
	CHAR16                          *SocketDesignation;
	CHAR8                           *OptionalStrStart;
	UINTN                           SocketStrLen;
	STRING_REF                      TokenToGet;
	EFI_DATA_HUB_PROTOCOL           *DataHub;
	UINT64                          MonotonicCount;
	EFI_DATA_RECORD_HEADER          *Record;
	EFI_SUBCLASS_TYPE1_HEADER       *DataHeader;
	UINT8                           *SrcData;
	UINT32                          SrcDataSize;
	EFI_STATUS                      Status;

	//
	// Memory Device LOcator
	//
	DEBUG ((EFI_D_ERROR, "type 7\n"));

	TokenToGet = STRING_TOKEN (STR_SOCKET_DESIGNATION);
	SocketDesignation = SmbiosMiscGetString (TokenToGet);
	SocketStrLen = StrLen(SocketDesignation);
	if (SocketStrLen > SMBIOS_STRING_MAX_LENGTH) {
	return EFI_UNSUPPORTED;
	}

	SmbiosRecordL1 = AllocatePool(sizeof (SMBIOS_TABLE_TYPE7) + 7 + 1 + 1);
	ASSERT (SmbiosRecordL1 != NULL);
	ZeroMem(SmbiosRecordL1, sizeof (SMBIOS_TABLE_TYPE7) + 7 + 1 + 1);

	SmbiosRecordL2 = AllocatePool(sizeof (SMBIOS_TABLE_TYPE7) + 7 + 1 + 1);
	ASSERT (SmbiosRecordL2 != NULL);
	ZeroMem(SmbiosRecordL2, sizeof (SMBIOS_TABLE_TYPE7) + 7 + 1 + 1);

	//
	// Get the Data Hub Protocol. Assume only one instance
	//
	Status = gBS->LocateProtocol (
	                &gEfiDataHubProtocolGuid,
	                NULL,
	                (VOID **)&DataHub
	                );
	ASSERT_EFI_ERROR(Status);

	MonotonicCount = 0;
	Record = NULL;

	do {
	Status = DataHub->GetNextRecord (
	                    DataHub,
	                    &MonotonicCount,
	                    NULL,
	                    &Record
	                    );
		if (!EFI_ERROR(Status)) {
			if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA) {
				DataHeader  = (EFI_SUBCLASS_TYPE1_HEADER *)(Record + 1);
				SrcData     = (UINT8  *)(DataHeader + 1);
				SrcDataSize = Record->RecordSize - Record->HeaderSize - sizeof (EFI_SUBCLASS_TYPE1_HEADER);
				if (CompareGuid(&Record->DataRecordGuid, &gEfiCacheSubClassGuid) && (DataHeader->RecordType == CacheSizeRecordType)) {
          			if (DataHeader->SubInstance == EFI_CACHE_L1) {
						SmbiosRecordL1->InstalledSize += (UINT16) (ConvertBase2ToRaw((EFI_EXP_BASE2_DATA *)SrcData) >> 10);
						SmbiosRecordL1->MaximumCacheSize = SmbiosRecordL1->InstalledSize;
          			}
         			 else if (DataHeader->SubInstance == EFI_CACHE_L2) {
						SmbiosRecordL2->InstalledSize += (UINT16) (ConvertBase2ToRaw((EFI_EXP_BASE2_DATA *)SrcData) >> 10);
						SmbiosRecordL2->MaximumCacheSize = SmbiosRecordL2->InstalledSize;
          			} else {
           				 continue;
          			}
		  		}
	      	}
    	}
	} while (!EFI_ERROR(Status) && (MonotonicCount != 0));

	//
	//Filling SMBIOS type 7 information for different cache levels.
	//

	SmbiosRecordL1->Hdr.Type = EFI_SMBIOS_TYPE_CACHE_INFORMATION;
	SmbiosRecordL1->Hdr.Length = (UINT8) sizeof (SMBIOS_TABLE_TYPE7);
	SmbiosRecordL1->Hdr.Handle = 0;

	SmbiosRecordL1->Associativity = CacheAssociativity8Way;
	SmbiosRecordL1->SystemCacheType = CacheTypeUnknown;
	SmbiosRecordL1->SocketDesignation = 0x01;
	SmbiosRecordL1->CacheSpeed = 0;
	SmbiosRecordL1->CacheConfiguration = 0x0180;
	ZeroMem (&CacheSramType, sizeof (EFI_CACHE_SRAM_TYPE_DATA));
	CacheSramType.Synchronous = 1;
	CopyMem(&SmbiosRecordL1->SupportedSRAMType, &CacheSramType, 2);
	CopyMem(&SmbiosRecordL1->CurrentSRAMType, &CacheSramType, 2);
	SmbiosRecordL1->ErrorCorrectionType = EfiCacheErrorSingleBit;


	SmbiosRecordL2->Hdr.Type = EFI_SMBIOS_TYPE_CACHE_INFORMATION;
	SmbiosRecordL2->Hdr.Length = (UINT8) sizeof (SMBIOS_TABLE_TYPE7);
	SmbiosRecordL2->Hdr.Handle = 0;

	SmbiosRecordL2->Associativity = CacheAssociativity16Way;
	SmbiosRecordL2->SystemCacheType = CacheTypeInstruction;
	SmbiosRecordL2->SocketDesignation = 0x01;
	SmbiosRecordL2->CacheSpeed = 0;
	SmbiosRecordL2->CacheConfiguration = 0x0281;
	ZeroMem (&CacheSramType, sizeof (EFI_CACHE_SRAM_TYPE_DATA));
	CacheSramType.Synchronous = 1;
	CopyMem(&SmbiosRecordL2->SupportedSRAMType, &CacheSramType, 2);
	CopyMem(&SmbiosRecordL2->CurrentSRAMType, &CacheSramType, 2);
	SmbiosRecordL2->ErrorCorrectionType = EfiCacheErrorSingleBit;



	//
	//Adding SMBIOS type 7 records to SMBIOS table.
	//
	SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
	OptionalStrStart = (CHAR8 *)(SmbiosRecordL1 + 1);
	UnicodeStrToAsciiStr(SocketDesignation, OptionalStrStart);

	Smbios-> Add(
	           Smbios,
	           NULL,
	           &SmbiosHandle,
	           (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecordL1
	           );

	//
	//VLV2 incorporates two SLM modules (quad cores) in the SoC. 2 cores share BIU/L2 cache
	//
	SmbiosRecordL2->InstalledSize = (SmbiosRecordL2->InstalledSize)/2;
	SmbiosRecordL2->MaximumCacheSize = SmbiosRecordL2->InstalledSize;
	SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

	OptionalStrStart = (CHAR8 *)(SmbiosRecordL2 + 1);
	UnicodeStrToAsciiStr(SocketDesignation, OptionalStrStart);

	Smbios-> Add(
	           Smbios,
	           NULL,
	           &SmbiosHandle,
	           (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecordL2
	           );

	return EFI_SUCCESS;
}
