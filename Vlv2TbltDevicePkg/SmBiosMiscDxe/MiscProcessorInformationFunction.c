/*++

Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscProcessorInformationFunction.c

Abstract:

  Onboard processor information boot time changes.
  SMBIOS type 4.

--*/

#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

#include <Protocol/MpService.h>
#include <Protocol/DataHub.h>
#include <Guid/DataHubRecords.h>
#include <Library/CpuIA32.h>

#define EfiProcessorFamilyIntelAtomProcessor    0x2B

EFI_GUID                        mProcessorProducerGuid;


/**
  Get cache SMBIOS record handle.

  @param  Smbios        Pointer to SMBIOS protocol instance.
  @param  CacheLevel    Level of cache, starting from one.
  @param  Handle        Returned record handle.

**/

VOID
GetCacheHandle (
  IN  EFI_SMBIOS_PROTOCOL      *Smbios,
  IN  UINT8                    CacheLevel,
  OUT  EFI_SMBIOS_HANDLE       *Handle
  )
{
  UINT16                     CacheConfig;
  EFI_STATUS                 Status;
  EFI_SMBIOS_TYPE            RecordType;
  EFI_SMBIOS_TABLE_HEADER    *Buffer;

  *Handle = 0;
  RecordType = EFI_SMBIOS_TYPE_CACHE_INFORMATION;

  do {
    Status = Smbios->GetNext (
                       Smbios,
                       Handle,
                       &RecordType,
                       &Buffer,
                       NULL
                       );
    if (!EFI_ERROR(Status)) {
      CacheConfig = *(UINT16*)((UINT8*)Buffer + 5);
      if ((CacheConfig & 0x7) == (CacheLevel -1) ) {
        return;
      }
    }
  } while (!EFI_ERROR(Status));

  *Handle = 0xFFFF;
}


/**
  This function makes boot time changes to the contents of the
  MiscProcessorInformation (Type 4).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
UINT32
ConvertBase10ToRaw (
  IN  EFI_EXP_BASE10_DATA             *Data)
{
  UINTN         Index;
  UINT32        RawData;

  RawData = Data->Value;
  for (Index = 0; Index < (UINTN) Data->Exponent; Index++) {
     RawData *= 10;
  }

  return  RawData;
}

#define BSEL_CR_OVERCLOCK_CONTROL	0xCD
#define	FUSE_BSEL_MASK				0x03



UINT16 miFSBFrequencyTable[4] = {
  83,          	// 83.3MHz
  100,          // 100MHz
  133,          // 133MHz
  117           // 116.7MHz
};

/**
  Determine the processor core frequency

  @param None

  @retval Processor core frequency multiplied by 3


**/
UINT16
DetermineiFsbFromMsr (
  VOID
  )
{
  //
  // Determine the processor core frequency
  //
  UINT64	Temp;
  Temp = (EfiReadMsr (BSEL_CR_OVERCLOCK_CONTROL)) & FUSE_BSEL_MASK;
  return miFSBFrequencyTable[(UINT32)(Temp)];

}

MISC_SMBIOS_TABLE_FUNCTION (MiscProcessorInformation)
{
    CHAR8                           *OptionalStrStart;
    EFI_STRING                      SerialNumber;
    CHAR16                          *Version=NULL;
    CHAR16                          *Manufacturer=NULL;
    CHAR16                          *Socket=NULL;
    CHAR16                          *AssetTag=NULL;
    CHAR16                          *PartNumber=NULL;
    UINTN                           SerialNumberStrLen=0;
    UINTN                           VersionStrLen=0;
    UINTN                           ManufacturerStrLen=0;
    UINTN                           SocketStrLen=0;
    UINTN                           AssetTagStrLen=0;
    UINTN                           PartNumberStrLen=0;
    UINTN                           ProcessorVoltage=0xAE;
    UINT32                          Eax01;
    UINT32                          Ebx01;
    UINT32                          Ecx01;
    UINT32                          Edx01;
    STRING_REF                      TokenToGet;
    EFI_STATUS                      Status;
    EFI_SMBIOS_HANDLE               SmbiosHandle;
    SMBIOS_TABLE_TYPE4              *SmbiosRecord;
    EFI_CPU_DATA_RECORD             *ForType4InputData;
    UINT16                          L1CacheHandle=0;
    UINT16                          L2CacheHandle=0;
    UINT16                          L3CacheHandle=0;
    UINTN                           NumberOfEnabledProcessors=0 ;
    UINTN                           NumberOfProcessors=0;
    UINT64                          Frequency = 0;
    EFI_MP_SERVICES_PROTOCOL        *MpService;
    EFI_DATA_HUB_PROTOCOL           *DataHub;
    UINT64                          MonotonicCount;
    EFI_DATA_RECORD_HEADER          *Record;
    EFI_SUBCLASS_TYPE1_HEADER       *DataHeader;
    UINT8                           *SrcData;
    UINT32                          SrcDataSize;
    EFI_PROCESSOR_VERSION_DATA      *ProcessorVersion;
    CHAR16                          *NewStringToken;
    STRING_REF                      TokenToUpdate;
    PROCESSOR_ID_DATA               *ProcessorId = NULL;


    //
    // First check for invalid parameters.
    //
    if (RecordData == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    ForType4InputData = (EFI_CPU_DATA_RECORD *)RecordData;

    ProcessorId = AllocateZeroPool(sizeof(PROCESSOR_ID_DATA));
    if (ProcessorId == NULL) {
      return EFI_INVALID_PARAMETER;
    }

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

            //
            // Processor
            //
            if (CompareGuid(&Record->DataRecordGuid, &gEfiProcessorSubClassGuid)) {
              CopyMem (&mProcessorProducerGuid, &Record->ProducerName, sizeof(EFI_GUID));
              switch (DataHeader->RecordType) {
                case ProcessorVoltageRecordType:
                  ProcessorVoltage = (((EFI_EXP_BASE10_DATA *)SrcData)->Value)/100 + 0x80;
                  break;
                case ProcessorCoreFrequencyRecordType:
                  DEBUG ((EFI_D_ERROR, "ProcessorCoreFrequencyRecordType SrcData1 =%d\n", ConvertBase10ToRaw((EFI_EXP_BASE10_DATA *)SrcData)/1000000));
                  Frequency = (ConvertBase10ToRaw((EFI_EXP_BASE10_DATA *)SrcData)/1000000);
                  break;
                case ProcessorVersionRecordType:
                  ProcessorVersion = (EFI_PROCESSOR_VERSION_DATA *)SrcData;
                  NewStringToken = HiiGetPackageString(&mProcessorProducerGuid, *ProcessorVersion, NULL);
                  TokenToUpdate = (STRING_REF)STR_MISC_PROCESSOR_VERSION;
                  HiiSetString(mHiiHandle, TokenToUpdate, NewStringToken, NULL);
                  break;
                default:
                  break;
              }
            }
          }
        }
    } while (!EFI_ERROR(Status) && (MonotonicCount != 0));

    //
    // Token to get for Socket Name
    //
    TokenToGet = STRING_TOKEN (STR_MISC_SOCKET_NAME);
    Socket = SmbiosMiscGetString (TokenToGet);
    SocketStrLen = StrLen(Socket);
    if (SocketStrLen > SMBIOS_STRING_MAX_LENGTH) {
         return EFI_UNSUPPORTED;
    }

    //
    // Token to get for Processor Manufacturer
    //
    TokenToGet = STRING_TOKEN (STR_MISC_PROCESSOR_MAUFACTURER);
    Manufacturer = SmbiosMiscGetString (TokenToGet);
    ManufacturerStrLen = StrLen(Manufacturer);
    if (ManufacturerStrLen > SMBIOS_STRING_MAX_LENGTH) {
      return EFI_UNSUPPORTED;
    }

    //
    // Token to get for Processor Version
    //
    TokenToGet = STRING_TOKEN (STR_MISC_PROCESSOR_VERSION);
    Version = SmbiosMiscGetString (TokenToGet);
    VersionStrLen = StrLen(Version);
    if (VersionStrLen > SMBIOS_STRING_MAX_LENGTH) {
        return EFI_UNSUPPORTED;
    }

    //
    // Token to get for Serial Number
    //
    TokenToGet = STRING_TOKEN (STR_MISC_PROCESSOR_SERIAL_NUMBER);
    SerialNumber = SmbiosMiscGetString (TokenToGet);
    SerialNumberStrLen = StrLen(SerialNumber);
    if (SerialNumberStrLen > SMBIOS_STRING_MAX_LENGTH) {
        return EFI_UNSUPPORTED;
    }

    //
    // Token to get for Assert Tag Information
    //
    TokenToGet = STRING_TOKEN (STR_MISC_ASSERT_TAG_DATA);
    AssetTag = SmbiosMiscGetString (TokenToGet);
    AssetTagStrLen = StrLen(AssetTag);
    if (AssetTagStrLen > SMBIOS_STRING_MAX_LENGTH) {
        return EFI_UNSUPPORTED;
    }

    //
    // Token to get for part number Information
    //
    TokenToGet = STRING_TOKEN (STR_MISC_PART_NUMBER);
    PartNumber = SmbiosMiscGetString (TokenToGet);
    PartNumberStrLen = StrLen(PartNumber);
    if (PartNumberStrLen > SMBIOS_STRING_MAX_LENGTH) {
         return EFI_UNSUPPORTED;
    }

    //
    // Two zeros following the last string.
    //
    SmbiosRecord = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE4) + AssetTagStrLen + 1 + SocketStrLen + 1+ ManufacturerStrLen +1 + VersionStrLen+ 1+ SerialNumberStrLen + 1 + PartNumberStrLen+ 1 + 1);

    SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE4);

    //
    // Make handle chosen by smbios protocol.add automatically.
    //
    SmbiosRecord->Hdr.Handle = 0;

    SmbiosRecord-> Socket= 1;
    SmbiosRecord -> ProcessorManufacture = 2;
    SmbiosRecord -> ProcessorVersion = 3;
    SmbiosRecord ->SerialNumber =4;

    SmbiosRecord-> AssetTag= 5;
    SmbiosRecord-> PartNumber= 6;

    //
    // Processor Type
    //
    ForType4InputData-> VariableRecord.ProcessorType= EfiCentralProcessor;
    SmbiosRecord -> ProcessorType = ForType4InputData-> VariableRecord.ProcessorType;

    //
    // Processor Family
    //
    ForType4InputData-> VariableRecord.ProcessorFamily= EfiProcessorFamilyIntelAtomProcessor; //0x2B;;
    SmbiosRecord -> ProcessorFamily = ForType4InputData-> VariableRecord.ProcessorFamily;
    SmbiosRecord -> ExternalClock = DetermineiFsbFromMsr();

    //
    // Processor ID
    //
    AsmCpuid(0x001, &Eax01, &Ebx01, &Ecx01, &Edx01);
    ProcessorId->Signature = *(PROCESSOR_SIGNATURE *)&Eax01;
    ProcessorId->FeatureFlags = *(PROCESSOR_FEATURE_FLAGS *)&Edx01;
    SmbiosRecord -> ProcessorId = *(PROCESSOR_ID_DATA *)ProcessorId;

    //
    // Processor Voltage
    //
    ForType4InputData-> VariableRecord.ProcessorVoltage= *(EFI_PROCESSOR_VOLTAGE_DATA *)&ProcessorVoltage;
    SmbiosRecord -> Voltage = *(PROCESSOR_VOLTAGE *) &(ForType4InputData-> VariableRecord.ProcessorVoltage);

    //
    // Status
    //
    ForType4InputData-> VariableRecord.ProcessorHealthStatus= 0x41;//0x41;
    SmbiosRecord -> Status = ForType4InputData-> VariableRecord.ProcessorHealthStatus;

    //
    // Processor Upgrade
    //
    SmbiosRecord -> ProcessorUpgrade = 0x008;

    //
    // Processor Family 2
    //
    SmbiosRecord -> ProcessorFamily2 = ForType4InputData-> VariableRecord.ProcessorFamily;

    //
    // Processor speed
    //
    SmbiosRecord-> CurrentSpeed = *(UINT16*) & Frequency;
    SmbiosRecord-> MaxSpeed = *(UINT16*) & Frequency;

    //
    // Processor Characteristics
    //
    AsmCpuid(0x8000000, NULL, NULL, NULL, &Edx01);
    Edx01= Edx01 >> 28;
    Edx01 &= 0x01;
    SmbiosRecord-> ProcessorCharacteristics= (UINT16)Edx01;

    //
    // Processor Core Count and Enabled core count
    //
    Status = gBS->LocateProtocol (
                    &gEfiMpServiceProtocolGuid,
                    NULL,
                    (void **)&MpService
                    );
    if (!EFI_ERROR (Status)) {
    //
    // Determine the number of processors
    //
    MpService->GetNumberOfProcessors (
                 MpService,
                 &NumberOfProcessors,
                 &NumberOfEnabledProcessors
                 );
    }
    SmbiosRecord-> CoreCount= (UINT8)NumberOfProcessors;
    SmbiosRecord-> EnabledCoreCount= (UINT8)NumberOfEnabledProcessors;
    SmbiosRecord-> ThreadCount= (UINT8)NumberOfEnabledProcessors;
    SmbiosRecord-> ProcessorCharacteristics = 0x2; // Unknown

    //
    // Processor Cache Handle
    //
    GetCacheHandle( Smbios,1, &L1CacheHandle);
    GetCacheHandle( Smbios,2, &L2CacheHandle);
    GetCacheHandle( Smbios,3, &L3CacheHandle);

    //
    // Updating Cache Handle Information
    //
    SmbiosRecord->L1CacheHandle  = L1CacheHandle;
    SmbiosRecord->L2CacheHandle  = L2CacheHandle;
    SmbiosRecord->L3CacheHandle  = L3CacheHandle;

    OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
    UnicodeStrToAsciiStr(Socket, OptionalStrStart);
    UnicodeStrToAsciiStr(Manufacturer, OptionalStrStart + SocketStrLen + 1);
    UnicodeStrToAsciiStr(Version, OptionalStrStart + SocketStrLen + 1 + ManufacturerStrLen+ 1);
    UnicodeStrToAsciiStr(SerialNumber, OptionalStrStart + SocketStrLen + 1 + VersionStrLen + 1 + ManufacturerStrLen + 1);
    UnicodeStrToAsciiStr(AssetTag, OptionalStrStart + SerialNumberStrLen + 1 + VersionStrLen + 1 + ManufacturerStrLen + 1 + SocketStrLen + 1);
    UnicodeStrToAsciiStr(PartNumber, OptionalStrStart + SerialNumberStrLen + 1 + VersionStrLen + 1 + ManufacturerStrLen + 1 + SocketStrLen + 1 + AssetTagStrLen + 1 );

    //
    // Now we have got the full Smbios record, call Smbios protocol to add this record.
    //
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = Smbios-> Add(
                        Smbios,
                        NULL,
                        &SmbiosHandle,
                        (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                        );
    if (EFI_ERROR (Status)) return Status;
    FreePool(SmbiosRecord);
    return Status;

}

