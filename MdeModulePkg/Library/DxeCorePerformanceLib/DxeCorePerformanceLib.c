/** @file
  Performance library instance mainly used by DxeCore.

  This library provides the performance measurement interfaces and initializes performance
  logging for DXE phase. It first initializes its private global data structure for
  performance logging and saves the performance GUIDed HOB passed from PEI phase. 
  It initializes DXE phase performance logging by publishing the Performance and PerformanceEx Protocol,
  which are consumed by DxePerformanceLib to logging performance data in DXE phase.

  This library is mainly used by DxeCore to start performance logging to ensure that
  Performance Protocol is installed at the very beginning of DXE phase.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "DxeCorePerformanceLibInternal.h"

//
// Data for FPDT performance records.
//
#define SMM_BOOT_RECORD_COMM_SIZE (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data) + sizeof(SMM_BOOT_RECORD_COMMUNICATE))
#define STRING_SIZE             (FPDT_STRING_EVENT_RECORD_NAME_LENGTH * sizeof (CHAR8))
#define FIRMWARE_RECORD_BUFFER  0x10000
#define CACHE_HANDLE_GUID_COUNT 0x800

BOOT_PERFORMANCE_TABLE          *mAcpiBootPerformanceTable = NULL;
BOOT_PERFORMANCE_TABLE          mBootPerformanceTableTemplate = {
  {
    EFI_ACPI_5_0_FPDT_BOOT_PERFORMANCE_TABLE_SIGNATURE,
    sizeof (BOOT_PERFORMANCE_TABLE)
  },
  {
    {
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_TYPE_FIRMWARE_BASIC_BOOT,    // Type
      sizeof (EFI_ACPI_5_0_FPDT_FIRMWARE_BASIC_BOOT_RECORD),        // Length
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_REVISION_FIRMWARE_BASIC_BOOT // Revision
    },
    0,  // Reserved
    //
    // These values will be updated at runtime.
    //
    0,  // ResetEnd
    0,  // OsLoaderLoadImageStart
    0,  // OsLoaderStartImageStart
    0,  // ExitBootServicesEntry
    0   // ExitBootServicesExit
  }
};

typedef struct {
  EFI_HANDLE    Handle;
  CHAR8         NameString[FPDT_STRING_EVENT_RECORD_NAME_LENGTH];
  EFI_GUID      ModuleGuid;
} HANDLE_GUID_MAP;

HANDLE_GUID_MAP mCacheHandleGuidTable[CACHE_HANDLE_GUID_COUNT];
UINTN           mCachePairCount = 0;

UINT32  mLoadImageCount       = 0;
UINT32  mPerformanceLength    = 0;
UINT32  mMaxPerformanceLength = 0;
UINT32  mBootRecordSize       = 0;
UINT32  mBootRecordMaxSize    = 0;

BOOLEAN mFpdtBufferIsReported = FALSE;
BOOLEAN mLackSpaceIsReported  = FALSE;
CHAR8   *mPlatformLanguage    = NULL;
UINT8   *mPerformancePointer  = NULL;
UINT8   *mBootRecordBuffer    = NULL;
BOOLEAN  mLockInsertRecord    = FALSE;

EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *mDevicePathToText = NULL;

//
// Interfaces for Performance Protocol.
//
PERFORMANCE_PROTOCOL mPerformanceInterface = {
  StartGauge,
  EndGauge,
  GetGauge
  };

//
// Interfaces for PerformanceEx Protocol.
//
PERFORMANCE_EX_PROTOCOL mPerformanceExInterface = {
  StartGaugeEx,
  EndGaugeEx,
  GetGaugeEx
  };

PERFORMANCE_PROPERTY  mPerformanceProperty;

/**
Check whether the Token is a known one which is uesed by core.

@param  Token      Pointer to a Null-terminated ASCII string

@retval TRUE       Is a known one used by core.
@retval FALSE      Not a known one.

**/
BOOLEAN
IsKnownTokens (
  IN CONST CHAR8  *Token
  )
{
  if (Token == NULL) {
    return FALSE;
  }

  if (AsciiStrCmp (Token, SEC_TOK) == 0 ||
      AsciiStrCmp (Token, PEI_TOK) == 0 ||
      AsciiStrCmp (Token, DXE_TOK) == 0 ||
      AsciiStrCmp (Token, BDS_TOK) == 0 ||
      AsciiStrCmp (Token, DRIVERBINDING_START_TOK) == 0 ||
      AsciiStrCmp (Token, DRIVERBINDING_SUPPORT_TOK) == 0 ||
      AsciiStrCmp (Token, DRIVERBINDING_STOP_TOK) == 0 ||
      AsciiStrCmp (Token, LOAD_IMAGE_TOK) == 0 ||
      AsciiStrCmp (Token, START_IMAGE_TOK) == 0 ||
      AsciiStrCmp (Token, PEIM_TOK) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
Check whether the ID is a known one which map to the known Token.

@param  Identifier  32-bit identifier.

@retval TRUE        Is a known one used by core.
@retval FALSE       Not a known one.

**/
BOOLEAN
IsKnownID (
  IN UINT32       Identifier
  )
{
  if (Identifier == MODULE_START_ID ||
      Identifier == MODULE_END_ID ||
      Identifier == MODULE_LOADIMAGE_START_ID ||
      Identifier == MODULE_LOADIMAGE_END_ID ||
      Identifier == MODULE_DB_START_ID ||
      Identifier == MODULE_DB_END_ID ||
      Identifier == MODULE_DB_SUPPORT_START_ID ||
      Identifier == MODULE_DB_SUPPORT_END_ID ||
      Identifier == MODULE_DB_STOP_START_ID ||
      Identifier == MODULE_DB_STOP_END_ID) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Allocate EfiReservedMemoryType below 4G memory address.

  This function allocates EfiReservedMemoryType below 4G memory address.

  @param[in]  Size   Size of memory to allocate.

  @return Allocated address for output.

**/
VOID *
FpdtAllocateReservedMemoryBelow4G (
  IN UINTN       Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID                  *Buffer;

  Buffer  = NULL;
  Pages   = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  if (!EFI_ERROR (Status)) {
    Buffer = (VOID *) (UINTN) Address;
    ZeroMem (Buffer, Size);
  }

  return Buffer;
}

/**
  Allocate buffer for Boot Performance table.

  @return Status code.

**/
EFI_STATUS
AllocateBootPerformanceTable (
  )
{
  EFI_STATUS                              Status;
  UINTN                                   Size;
  UINT8                                   *SmmBootRecordCommBuffer;
  EFI_SMM_COMMUNICATE_HEADER              *SmmCommBufferHeader;
  SMM_BOOT_RECORD_COMMUNICATE             *SmmCommData;
  UINTN                                   CommSize;
  UINTN                                   BootPerformanceDataSize;
  UINT8                                   *BootPerformanceData;
  EFI_SMM_COMMUNICATION_PROTOCOL          *Communication;
  FIRMWARE_PERFORMANCE_VARIABLE           PerformanceVariable;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE *SmmCommRegionTable;
  EFI_MEMORY_DESCRIPTOR                   *SmmCommMemRegion;
  UINTN                                   Index;
  VOID                                    *SmmBootRecordData;
  UINTN                                   SmmBootRecordDataSize;
  UINTN                                   ReservedMemSize;

  //
  // Collect boot records from SMM drivers.
  //
  SmmBootRecordCommBuffer = NULL;
  SmmCommData             = NULL;
  SmmBootRecordData       = NULL;
  SmmBootRecordDataSize   = 0;
  ReservedMemSize         = 0;
  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &Communication);
  if (!EFI_ERROR (Status)) {
    //
    // Initialize communicate buffer
    // Get the prepared Reserved Memory Range
    //
    Status = EfiGetSystemConfigurationTable (
              &gEdkiiPiSmmCommunicationRegionTableGuid,
              (VOID **) &SmmCommRegionTable
              );
    if (!EFI_ERROR (Status)) {
      ASSERT (SmmCommRegionTable != NULL);
      SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *) (SmmCommRegionTable + 1);
      for (Index = 0; Index < SmmCommRegionTable->NumberOfEntries; Index ++) {
        if (SmmCommMemRegion->Type == EfiConventionalMemory) {
          break;
        }
        SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) SmmCommMemRegion + SmmCommRegionTable->DescriptorSize);
      }
      ASSERT (Index < SmmCommRegionTable->NumberOfEntries);
      ASSERT (SmmCommMemRegion->PhysicalStart > 0);
      ASSERT (SmmCommMemRegion->NumberOfPages > 0);
      ReservedMemSize = (UINTN) SmmCommMemRegion->NumberOfPages * EFI_PAGE_SIZE;

      //
      // Check enough reserved memory space
      //
      if (ReservedMemSize > SMM_BOOT_RECORD_COMM_SIZE) {
        SmmBootRecordCommBuffer = (VOID *) (UINTN) SmmCommMemRegion->PhysicalStart;
        SmmCommBufferHeader = (EFI_SMM_COMMUNICATE_HEADER*)SmmBootRecordCommBuffer;
        SmmCommData = (SMM_BOOT_RECORD_COMMUNICATE*)SmmCommBufferHeader->Data;
        ZeroMem((UINT8*)SmmCommData, sizeof(SMM_BOOT_RECORD_COMMUNICATE));

        CopyGuid (&SmmCommBufferHeader->HeaderGuid, &gEfiFirmwarePerformanceGuid);
        SmmCommBufferHeader->MessageLength = sizeof(SMM_BOOT_RECORD_COMMUNICATE);
        CommSize = SMM_BOOT_RECORD_COMM_SIZE;

        //
        // Get the size of boot records.
        //
        SmmCommData->Function       = SMM_FPDT_FUNCTION_GET_BOOT_RECORD_SIZE;
        SmmCommData->BootRecordData = NULL;
        Status = Communication->Communicate (Communication, SmmBootRecordCommBuffer, &CommSize);

        if (!EFI_ERROR (Status) && !EFI_ERROR (SmmCommData->ReturnStatus) && SmmCommData->BootRecordSize != 0) {
          //
          // Get all boot records
          //
          SmmCommData->Function       = SMM_FPDT_FUNCTION_GET_BOOT_RECORD_DATA_BY_OFFSET;
          SmmBootRecordDataSize       = SmmCommData->BootRecordSize;
          SmmBootRecordData           = AllocateZeroPool(SmmBootRecordDataSize);
          ASSERT (SmmBootRecordData  != NULL);
          SmmCommData->BootRecordOffset = 0;
          SmmCommData->BootRecordData   = (VOID *) ((UINTN) SmmCommMemRegion->PhysicalStart + SMM_BOOT_RECORD_COMM_SIZE);
          SmmCommData->BootRecordSize   = ReservedMemSize - SMM_BOOT_RECORD_COMM_SIZE;
          while (SmmCommData->BootRecordOffset < SmmBootRecordDataSize) {
            Status = Communication->Communicate (Communication, SmmBootRecordCommBuffer, &CommSize);
            ASSERT_EFI_ERROR (Status);
            ASSERT_EFI_ERROR(SmmCommData->ReturnStatus);
            if (SmmCommData->BootRecordOffset + SmmCommData->BootRecordSize > SmmBootRecordDataSize) {
              CopyMem ((UINT8 *) SmmBootRecordData + SmmCommData->BootRecordOffset, SmmCommData->BootRecordData, SmmBootRecordDataSize - SmmCommData->BootRecordOffset);
            } else {
              CopyMem ((UINT8 *) SmmBootRecordData + SmmCommData->BootRecordOffset, SmmCommData->BootRecordData, SmmCommData->BootRecordSize);
            }
            SmmCommData->BootRecordOffset = SmmCommData->BootRecordOffset + SmmCommData->BootRecordSize;
          }
        }
      }
    }
  }

  //
  // Prepare memory for Boot Performance table.
  // Boot Performance table includes BasicBoot record, and one or more appended Boot Records.
  //
  BootPerformanceDataSize = sizeof (BOOT_PERFORMANCE_TABLE) + mPerformanceLength + PcdGet32 (PcdExtFpdtBootRecordPadSize);
  if (SmmCommData != NULL && SmmBootRecordData != NULL) {
    BootPerformanceDataSize += SmmBootRecordDataSize;
  }

  //
  // Try to allocate the same runtime buffer as last time boot.
  //
  ZeroMem (&PerformanceVariable, sizeof (PerformanceVariable));
  Size = sizeof (PerformanceVariable);
  Status = gRT->GetVariable (
                  EFI_FIRMWARE_PERFORMANCE_VARIABLE_NAME,
                  &gEfiFirmwarePerformanceGuid,
                  NULL,
                  &Size,
                  &PerformanceVariable
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->AllocatePages (
                    AllocateAddress,
                    EfiReservedMemoryType,
                    EFI_SIZE_TO_PAGES (BootPerformanceDataSize),
                    &PerformanceVariable.BootPerformanceTablePointer
                    );
    if (!EFI_ERROR (Status)) {
      mAcpiBootPerformanceTable = (BOOT_PERFORMANCE_TABLE *) (UINTN) PerformanceVariable.BootPerformanceTablePointer;
    }
  }

  if (mAcpiBootPerformanceTable == NULL) {
    //
    // Fail to allocate at specified address, continue to allocate at any address.
    //
    mAcpiBootPerformanceTable = (BOOT_PERFORMANCE_TABLE *) FpdtAllocateReservedMemoryBelow4G (BootPerformanceDataSize);
  }
  DEBUG ((DEBUG_INFO, "DxeCorePerformanceLib: ACPI Boot Performance Table address = 0x%x\n", mAcpiBootPerformanceTable));

  if (mAcpiBootPerformanceTable == NULL) {
    if (SmmCommData != NULL && SmmBootRecordData != NULL) {
      FreePool (SmmBootRecordData);
    }
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Prepare Boot Performance Table.
  //
  BootPerformanceData = (UINT8 *) mAcpiBootPerformanceTable;
  //
  // Fill Basic Boot record to Boot Performance Table.
  //
  CopyMem (mAcpiBootPerformanceTable, &mBootPerformanceTableTemplate, sizeof (mBootPerformanceTableTemplate));
  BootPerformanceData = BootPerformanceData + mAcpiBootPerformanceTable->Header.Length;
  //
  // Fill Boot records from boot drivers.
  //
  if (mPerformancePointer != NULL) {
    CopyMem (BootPerformanceData, mPerformancePointer, mPerformanceLength);
    mAcpiBootPerformanceTable->Header.Length += mPerformanceLength;
    BootPerformanceData = BootPerformanceData + mPerformanceLength;
    FreePool (mPerformancePointer);
    mPerformancePointer   = NULL;
    mPerformanceLength    = 0;
    mMaxPerformanceLength = 0;
  }
  if (SmmCommData != NULL && SmmBootRecordData != NULL) {
    //
    // Fill Boot records from SMM drivers.
    //
    CopyMem (BootPerformanceData, SmmBootRecordData, SmmBootRecordDataSize);
    FreePool (SmmBootRecordData);
    mAcpiBootPerformanceTable->Header.Length = (UINT32) (mAcpiBootPerformanceTable->Header.Length + SmmBootRecordDataSize);
    BootPerformanceData = BootPerformanceData + SmmBootRecordDataSize;
  }

  mBootRecordBuffer  = (UINT8 *) mAcpiBootPerformanceTable;
  mBootRecordSize    = mAcpiBootPerformanceTable->Header.Length;
  mBootRecordMaxSize = mBootRecordSize + PcdGet32 (PcdExtFpdtBootRecordPadSize);

  return EFI_SUCCESS;
}

/**
  Get a human readable module name and module guid for the given image handle.
  If module name can't be found, "" string will return.
  If module guid can't be found, Zero Guid will return.

  @param    Handle        Image handle or Controller handle.
  @param    NameString    The ascii string will be filled into it. If not found, null string will return.
  @param    BufferSize    Size of the input NameString buffer.
  @param    ModuleGuid    Point to the guid buffer to store the got module guid value.

  @retval EFI_SUCCESS     Successfully get module name and guid.
  @retval EFI_INVALID_PARAMETER  The input parameter NameString is NULL.
  @retval other value  Module Name can't be got.
**/
EFI_STATUS
GetModuleInfoFromHandle (
  IN EFI_HANDLE  Handle,
  OUT CHAR8            *NameString,
  IN UINTN             BufferSize,
  OUT EFI_GUID         *ModuleGuid OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  CHAR8                       *PdbFileName;
  EFI_GUID                    *TempGuid;
  UINTN                       StartIndex;
  UINTN                       Index;
  INTN                        Count;
  BOOLEAN                     ModuleGuidIsGet;
  UINTN                       StringSize;
  CHAR16                      *StringPtr;
  EFI_COMPONENT_NAME2_PROTOCOL      *ComponentName2;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FvFilePath;

  if (NameString == NULL || BufferSize == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Try to get the ModuleGuid and name string form the caached array.
  //
  if (mCachePairCount > 0) {
    for (Count = mCachePairCount -1; Count >= 0; Count--) {
      if (Handle == mCacheHandleGuidTable[Count].Handle) {
        CopyGuid (ModuleGuid, &mCacheHandleGuidTable[Count].ModuleGuid);
        AsciiStrCpyS (NameString, FPDT_STRING_EVENT_RECORD_NAME_LENGTH, mCacheHandleGuidTable[Count].NameString);
        return EFI_SUCCESS;
      }
    }
  }

  Status = EFI_INVALID_PARAMETER;
  LoadedImage     = NULL;
  ModuleGuidIsGet = FALSE;

  //
  // Initialize GUID as zero value.
  //
  TempGuid    = &gZeroGuid;
  //
  // Initialize it as "" string.
  //
  NameString[0] = 0;

  if (Handle != NULL) {
    //
    // Try Handle as ImageHandle.
    //
    Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID**) &LoadedImage
                  );

    if (EFI_ERROR (Status)) {
      //
      // Try Handle as Controller Handle
      //
      Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBinding,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
      if (!EFI_ERROR (Status)) {
        //
        // Get Image protocol from ImageHandle
        //
        Status = gBS->HandleProtocol (
                      DriverBinding->ImageHandle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID**) &LoadedImage
                      );
      }
    }
  }

  if (!EFI_ERROR (Status) && LoadedImage != NULL) {
    //
    // Get Module Guid from DevicePath.
    //
    if (LoadedImage->FilePath != NULL &&
        LoadedImage->FilePath->Type == MEDIA_DEVICE_PATH &&
        LoadedImage->FilePath->SubType == MEDIA_PIWG_FW_FILE_DP
       ) {
      //
      // Determine GUID associated with module logging performance
      //
      ModuleGuidIsGet = TRUE;
      FvFilePath      = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LoadedImage->FilePath;
      TempGuid        = &FvFilePath->FvFileName;
    }

    //
    // Method 1 Get Module Name from PDB string.
    //
    PdbFileName = PeCoffLoaderGetPdbPointer (LoadedImage->ImageBase);
    if (PdbFileName != NULL && BufferSize > 0) {
      StartIndex = 0;
      for (Index = 0; PdbFileName[Index] != 0; Index++) {
        if ((PdbFileName[Index] == '\\') || (PdbFileName[Index] == '/')) {
          StartIndex = Index + 1;
        }
      }
      //
      // Copy the PDB file name to our temporary string.
      // If the length is bigger than BufferSize, trim the redudant characters to avoid overflow in array boundary.
      //
      for (Index = 0; Index < BufferSize - 1; Index++) {
        NameString[Index] = PdbFileName[Index + StartIndex];
        if (NameString[Index] == 0 || NameString[Index] == '.') {
          NameString[Index] = 0;
          break;
        }
      }

      if (Index == BufferSize - 1) {
        NameString[Index] = 0;
      }
      //
      // Module Name is got.
      //
      goto Done;
    }
  }

  //
  // Method 2: Get the name string from ComponentName2 protocol
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **) &ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Get the current platform language setting
    //
    if (mPlatformLanguage == NULL) {
      GetEfiGlobalVariable2 (L"PlatformLang", (VOID **) &mPlatformLanguage, NULL);
    }
    if (mPlatformLanguage != NULL) {
      Status = ComponentName2->GetDriverName (
                                 ComponentName2,
                                 mPlatformLanguage != NULL ? mPlatformLanguage : "en-US",
                                 &StringPtr
                                 );
      if (!EFI_ERROR (Status)) {
        for (Index = 0; Index < BufferSize - 1 && StringPtr[Index] != 0; Index++) {
          NameString[Index] = (CHAR8) StringPtr[Index];
        }
        NameString[Index] = 0;
        //
        // Module Name is got.
        //
        goto Done;
      }
    }
  }

  if (ModuleGuidIsGet) {
    //
    // Method 3 Try to get the image's FFS UI section by image GUID
    //
    StringPtr  = NULL;
    StringSize = 0;
    Status = GetSectionFromAnyFv (
              TempGuid,
              EFI_SECTION_USER_INTERFACE,
              0,
              (VOID **) &StringPtr,
              &StringSize
              );

    if (!EFI_ERROR (Status)) {
      //
      // Method 3. Get the name string from FFS UI section
      //
      for (Index = 0; Index < BufferSize - 1 && StringPtr[Index] != 0; Index++) {
        NameString[Index] = (CHAR8) StringPtr[Index];
      }
      NameString[Index] = 0;
      FreePool (StringPtr);
    }
  }

Done:
  //
  // Copy Module Guid
  //
  if (ModuleGuid != NULL) {
    CopyGuid (ModuleGuid, TempGuid);
    if (IsZeroGuid(TempGuid) && (Handle != NULL) && !ModuleGuidIsGet) {
        // Handle is GUID
      CopyGuid (ModuleGuid, (EFI_GUID *) Handle);
    }
  }

  //
  // Cache the Handle and Guid pairs.
  //
  if (mCachePairCount < CACHE_HANDLE_GUID_COUNT) {
    mCacheHandleGuidTable[mCachePairCount].Handle = Handle;
    CopyGuid (&mCacheHandleGuidTable[mCachePairCount].ModuleGuid, ModuleGuid);
    AsciiStrCpyS (mCacheHandleGuidTable[mCachePairCount].NameString, FPDT_STRING_EVENT_RECORD_NAME_LENGTH, NameString);
    mCachePairCount ++;
  }

  return Status;
}

/**
  Get the FPDT record info.

  @param  IsStart                 TRUE if the performance log is start log.
  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  RecordInfo              On return, pointer to the info of the record.
  @param  UseModuleName           Only useful for FPDT_DYNAMIC_STRING_EVENT_TYPE, indicate that whether need use
                                  Module name to fill the string field in the FPDT_DYNAMIC_STRING_EVENT_RECORD.

  @retval EFI_SUCCESS          Get record info successfully.
  @retval EFI_UNSUPPORTED      No matched FPDT record.

**/
EFI_STATUS
GetFpdtRecordInfo (
  IN BOOLEAN                  IsStart,
  IN CONST VOID               *Handle,
  IN CONST CHAR8              *Token,
  IN CONST CHAR8              *Module,
  OUT FPDT_BASIC_RECORD_INFO  *RecordInfo,
  IN OUT BOOLEAN              *UseModuleName
  )
{
  UINT16              RecordType;
  UINTN               StringSize;

  RecordType    = FPDT_DYNAMIC_STRING_EVENT_TYPE;

  //
  // Token to Type and Id.
  //
  if (Token != NULL) {
    if (AsciiStrCmp (Token, START_IMAGE_TOK) == 0) {                // "StartImage:"
      *UseModuleName = TRUE;
      RecordType     = FPDT_GUID_EVENT_TYPE;
      if (IsStart) {
        RecordInfo->ProgressID  = MODULE_START_ID;
      } else {
        RecordInfo->ProgressID  = MODULE_END_ID;
      }
    } else if (AsciiStrCmp (Token, LOAD_IMAGE_TOK) == 0) {          // "LoadImage:"
      *UseModuleName = TRUE;
      RecordType = FPDT_GUID_QWORD_EVENT_TYPE;
      if (IsStart) {
        RecordInfo->ProgressID  = MODULE_LOADIMAGE_START_ID;
      } else {
        RecordInfo->ProgressID  = MODULE_LOADIMAGE_END_ID;
      }
    } else if (AsciiStrCmp (Token, DRIVERBINDING_START_TOK) == 0) {  // "DB:Start:"
      *UseModuleName = TRUE;
      if (IsStart) {
        RecordInfo->ProgressID  = MODULE_DB_START_ID;
        RecordType   = FPDT_GUID_QWORD_EVENT_TYPE;
      } else {
        RecordInfo->ProgressID  = MODULE_DB_END_ID;
        RecordType   = FPDT_GUID_QWORD_STRING_EVENT_TYPE;
      }
    } else if (AsciiStrCmp (Token, DRIVERBINDING_SUPPORT_TOK) == 0) { // "DB:Support:"
      *UseModuleName = TRUE;
      if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        return RETURN_UNSUPPORTED;
      }
      RecordType     = FPDT_GUID_QWORD_EVENT_TYPE;
      if (IsStart) {
        RecordInfo->ProgressID  = MODULE_DB_SUPPORT_START_ID;
      } else {
        RecordInfo->ProgressID  = MODULE_DB_SUPPORT_END_ID;
      }
    } else if (AsciiStrCmp (Token, DRIVERBINDING_STOP_TOK) == 0) {    // "DB:Stop:"
      *UseModuleName = TRUE;
      if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
         return RETURN_UNSUPPORTED;
      }
      RecordType     = FPDT_GUID_QWORD_EVENT_TYPE;
      if (IsStart) {
        RecordInfo->ProgressID  = MODULE_DB_STOP_START_ID;
      } else {
        RecordInfo->ProgressID  = MODULE_DB_STOP_END_ID;
      }
    } else if (AsciiStrCmp (Token, PEI_TOK) == 0 ||                   // "PEI"
               AsciiStrCmp (Token, DXE_TOK) == 0 ||                   // "DXE"
               AsciiStrCmp (Token, BDS_TOK) == 0) {                   // "BDS"
      if (IsStart) {
        RecordInfo->ProgressID  = PERF_CROSSMODULE_START_ID;
      } else {
        RecordInfo->ProgressID  = PERF_CROSSMODULE_END_ID;
      }
    } else {                                                          // Pref used in Modules.
      if (IsStart) {
        RecordInfo->ProgressID  = PERF_INMODULE_START_ID;
      } else {
        RecordInfo->ProgressID  = PERF_INMODULE_END_ID;
      }
    }
  } else if (Handle!= NULL || Module != NULL) {                       // Pref used in Modules.
    if (IsStart) {
      RecordInfo->ProgressID    = PERF_INMODULE_START_ID;
    } else {
      RecordInfo->ProgressID    = PERF_INMODULE_END_ID;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Get Record size baesed on the record type.
  // When PcdEdkiiFpdtStringRecordEnableOnly is TRUE, all records are with type of FPDT_DYNAMIC_STRING_EVENT_TYPE.
  //
  if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
    RecordType               = FPDT_DYNAMIC_STRING_EVENT_TYPE;
    RecordInfo->RecordSize   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD) + STRING_SIZE;
  } else {
    switch (RecordType) {
    case FPDT_GUID_EVENT_TYPE:
      RecordInfo->RecordSize = sizeof (FPDT_GUID_EVENT_RECORD);
      break;

    case FPDT_DYNAMIC_STRING_EVENT_TYPE:
      if (*UseModuleName) {
        StringSize  = STRING_SIZE;
      } else if (Token  != NULL) {
        StringSize  = AsciiStrSize (Token);
      } else if (Module != NULL) {
        StringSize  = AsciiStrSize (Module);
      } else {
        StringSize  = STRING_SIZE;
      }
      if (StringSize > STRING_SIZE) {
        StringSize   = STRING_SIZE;
      }
      RecordInfo->RecordSize = (UINT8)(sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD) + StringSize);
      break;

    case FPDT_GUID_QWORD_EVENT_TYPE:
      RecordInfo->RecordSize = (UINT8)sizeof (FPDT_GUID_QWORD_EVENT_RECORD);
      break;

    case FPDT_GUID_QWORD_STRING_EVENT_TYPE:
      RecordInfo->RecordSize = (UINT8)sizeof (FPDT_GUID_QWORD_STRING_EVENT_RECORD);
      break;

    default:
      //
      // Record is unsupported yet, return EFI_UNSUPPORTED
      //
      return EFI_UNSUPPORTED;
    }
  }

  RecordInfo->Type = RecordType;
  return EFI_SUCCESS;
}

/**
  Add performance log to FPDT boot record table.

  @param  IsStart                 TRUE if the performance log is start log.
  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  Ticker                  64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the created record
                                  is same as the one created by StartGauge of PERFORMANCE_PROTOCOL.

  @retval EFI_SUCCESS             Add FPDT boot record.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to record the measurement.
  @retval EFI_UNSUPPORTED         No matched FPDT record.

**/
EFI_STATUS
InsertFpdtMeasurement (
  IN BOOLEAN      IsStart,
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       Ticker,
  IN UINT32       Identifier
  )
{
  EFI_GUID                     ModuleGuid;
  CHAR8                        ModuleName[FPDT_STRING_EVENT_RECORD_NAME_LENGTH];
  EFI_STATUS                   Status;
  FPDT_RECORD_PTR              FpdtRecordPtr;
  FPDT_BASIC_RECORD_INFO       RecordInfo;
  UINT64                       TimeStamp;
  UINTN                        DestMax;
  UINTN                        StrLength;
  CONST CHAR8                  *StringPtr;
  BOOLEAN                      UseModuleName;

  StringPtr     = NULL;
  UseModuleName = FALSE;
  ZeroMem (ModuleName, sizeof (ModuleName));

  if (mLockInsertRecord) {
    return EFI_UNSUPPORTED;
  }

  mLockInsertRecord = TRUE;

  //
  // Get record info (type, size, ProgressID and Module Guid).
  //
  Status = GetFpdtRecordInfo (IsStart, Handle, Token, Module, &RecordInfo, &UseModuleName);
  if (EFI_ERROR (Status)) {
    mLockInsertRecord = FALSE;
    return Status;
  }

  //
  // If PERF_START()/PERF_END() have specified the ProgressID,it has high priority.
  // !!! Note: If the Perf is not the known Token used in the core but have same
  // ID with the core Token, this case will not be supported.
  // And in currtnt usage mode, for the unkown ID, there is a general rule:
  // If it is start pref: the lower 4 bits of the ID should be 0.
  // If it is end pref: the lower 4 bits of the ID should not be 0.
  // If input ID doesn't follow the rule, we will adjust it.
  //
  if ((Identifier != 0) && (IsKnownID (Identifier)) && (!IsKnownTokens (Token))) {
    mLockInsertRecord = FALSE;
    return EFI_UNSUPPORTED;
  } else if ((Identifier != 0) && (!IsKnownID (Identifier)) && (!IsKnownTokens (Token))) {
    if (IsStart && ((Identifier & 0x000F) != 0)) {
      Identifier &= 0xFFF0;
    } else if ((!IsStart) && ((Identifier & 0x000F) == 0)) {
      Identifier += 1;
    }
    RecordInfo.ProgressID = (UINT16)Identifier;
  }

  if (mFpdtBufferIsReported) {
    //
    // Append Boot records to the boot performance table.
    //
    if (mBootRecordSize + RecordInfo.RecordSize > mBootRecordMaxSize) {
      if (!mLackSpaceIsReported) {
        DEBUG ((DEBUG_INFO, "DxeCorePerformanceLib: No enough space to save boot records\n"));
        mLackSpaceIsReported = TRUE;
      }
      mLockInsertRecord = FALSE;
      return EFI_OUT_OF_RESOURCES;
    } else {
      //
      // Save boot record into BootPerformance table
      //
      FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mBootRecordBuffer + mBootRecordSize);
      mBootRecordSize += RecordInfo.RecordSize;
      mAcpiBootPerformanceTable->Header.Length += RecordInfo.RecordSize;
    }
  } else {
    //
    // Check if pre-allocated buffer is full
    //
    if (mPerformanceLength + RecordInfo.RecordSize > mMaxPerformanceLength) {
      mPerformancePointer = ReallocatePool (
                              mPerformanceLength,
                              mPerformanceLength + RecordInfo.RecordSize + FIRMWARE_RECORD_BUFFER,
                              mPerformancePointer
                              );

      if (mPerformancePointer == NULL) {
        mLockInsertRecord = FALSE;
        return EFI_OUT_OF_RESOURCES;
      }
      mMaxPerformanceLength = mPerformanceLength + RecordInfo.RecordSize + FIRMWARE_RECORD_BUFFER;
    }
    //
    // Covert buffer to FPDT Ptr Union type.
    //
    FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mPerformancePointer + mPerformanceLength);
    mPerformanceLength += RecordInfo.RecordSize;
  }

  //
  // Get the TimeStamp.
  //
  if (Ticker == 0) {
    Ticker    = GetPerformanceCounter ();
    TimeStamp = GetTimeInNanoSecond (Ticker);
  } else if (Ticker == 1) {
    TimeStamp = 0;
  } else {
    TimeStamp = GetTimeInNanoSecond (Ticker);
  }

  //
  // Get the ModuleName and ModuleGuid form the handle.
  //
  GetModuleInfoFromHandle ((EFI_HANDLE *)Handle, ModuleName, sizeof (ModuleName), &ModuleGuid);

  //
  // Fill in the record information.
  //
  switch (RecordInfo.Type) {
  case FPDT_GUID_EVENT_TYPE:
    FpdtRecordPtr.GuidEvent->Header.Type                = FPDT_GUID_EVENT_TYPE;
    FpdtRecordPtr.GuidEvent->Header.Length              = RecordInfo.RecordSize;
    FpdtRecordPtr.GuidEvent->Header.Revision            = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.GuidEvent->ProgressID                 = RecordInfo.ProgressID;
    FpdtRecordPtr.GuidEvent->Timestamp                  = TimeStamp;
    CopyMem (&FpdtRecordPtr.GuidEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidEvent->Guid));
    break;

  case FPDT_DYNAMIC_STRING_EVENT_TYPE:
    FpdtRecordPtr.DynamicStringEvent->Header.Type       = FPDT_DYNAMIC_STRING_EVENT_TYPE;
    FpdtRecordPtr.DynamicStringEvent->Header.Length     = RecordInfo.RecordSize;
    FpdtRecordPtr.DynamicStringEvent->Header.Revision   = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.DynamicStringEvent->ProgressID        = RecordInfo.ProgressID;
    FpdtRecordPtr.DynamicStringEvent->Timestamp         = TimeStamp;
    CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.DynamicStringEvent->Guid));

    if (UseModuleName) {
      StringPtr     = ModuleName;
    } else if (Token != NULL) {
      StringPtr     = Token;
    } else if (Module != NULL) {
      StringPtr     = Module;
    } else if (ModuleName != NULL) {
      StringPtr     = ModuleName;
    }
    if (StringPtr != NULL && AsciiStrLen (StringPtr) != 0) {
      StrLength     = AsciiStrLen (StringPtr);
      DestMax       = (RecordInfo.RecordSize - sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD)) / sizeof (CHAR8);
      if (StrLength >= DestMax) {
        StrLength   = DestMax -1;
      }
      AsciiStrnCpyS (FpdtRecordPtr.DynamicStringEvent->String, DestMax, StringPtr, StrLength);
    } else {
      AsciiStrCpyS (FpdtRecordPtr.DynamicStringEvent->String, FPDT_STRING_EVENT_RECORD_NAME_LENGTH, "unknown name");
    }
    break;

  case FPDT_GUID_QWORD_EVENT_TYPE:
    FpdtRecordPtr.GuidQwordEvent->Header.Type           = FPDT_GUID_QWORD_EVENT_TYPE;
    FpdtRecordPtr.GuidQwordEvent->Header.Length         = RecordInfo.RecordSize;
    FpdtRecordPtr.GuidQwordEvent->Header.Revision       = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.GuidQwordEvent->ProgressID            = RecordInfo.ProgressID;
    FpdtRecordPtr.GuidQwordEvent->Timestamp             = TimeStamp;
    CopyMem (&FpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidQwordEvent->Guid));
    if ((MODULE_LOADIMAGE_START_ID == RecordInfo.ProgressID) && AsciiStrCmp (Token, LOAD_IMAGE_TOK) == 0) {
      mLoadImageCount++;
      FpdtRecordPtr.GuidQwordEvent->Qword               = mLoadImageCount;
    }
    break;

  case FPDT_GUID_QWORD_STRING_EVENT_TYPE:
    FpdtRecordPtr.GuidQwordStringEvent->Header.Type     = FPDT_GUID_QWORD_STRING_EVENT_TYPE;
    FpdtRecordPtr.GuidQwordStringEvent->Header.Length   = RecordInfo.RecordSize;
    FpdtRecordPtr.GuidQwordStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.GuidQwordStringEvent->ProgressID      = RecordInfo.ProgressID;
    FpdtRecordPtr.GuidQwordStringEvent->Timestamp       = TimeStamp;
    CopyMem (&FpdtRecordPtr.GuidQwordStringEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidQwordStringEvent->Guid));
    break;

  default:
    //
    // Record is not supported in current DXE phase, return EFI_ABORTED
    //
    mLockInsertRecord = FALSE;
    return EFI_UNSUPPORTED;
  }

  mLockInsertRecord = FALSE;
  return EFI_SUCCESS;
}

/**
  Dumps all the PEI performance.

  @param  HobStart      A pointer to a Guid.

  This internal function dumps all the PEI performance log to the DXE performance gauge array.
  It retrieves the optional GUID HOB for PEI performance and then saves the performance data
  to DXE performance data structures.

**/
VOID
InternalGetPeiPerformance (
  VOID  *HobStart
  )
{
  UINT8                                *FirmwarePerformanceHob;
  FPDT_PEI_EXT_PERF_HEADER             *PeiPerformanceLogHeader;
  UINT8                                *EventRec;
  EFI_HOB_GUID_TYPE                    *GuidHob;

  GuidHob = GetNextGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid, HobStart);
  while (GuidHob != NULL) {
    FirmwarePerformanceHob  = GET_GUID_HOB_DATA (GuidHob);
    PeiPerformanceLogHeader = (FPDT_PEI_EXT_PERF_HEADER *)FirmwarePerformanceHob;

    if (mPerformanceLength + PeiPerformanceLogHeader->SizeOfAllEntries > mMaxPerformanceLength) {
      mPerformancePointer = ReallocatePool (
                              mPerformanceLength,
                              mPerformanceLength +
                              (UINTN)PeiPerformanceLogHeader->SizeOfAllEntries +
                              FIRMWARE_RECORD_BUFFER,
                              mPerformancePointer
                              );
      ASSERT (mPerformancePointer != NULL);
      mMaxPerformanceLength = mPerformanceLength +
                              (UINTN)(PeiPerformanceLogHeader->SizeOfAllEntries) +
                              FIRMWARE_RECORD_BUFFER;
    }

    EventRec = mPerformancePointer + mPerformanceLength;
    CopyMem (EventRec, FirmwarePerformanceHob + sizeof (FPDT_PEI_EXT_PERF_HEADER), (UINTN)(PeiPerformanceLogHeader->SizeOfAllEntries));
    //
    // Update the used buffer size.
    //
    mPerformanceLength += (UINTN)(PeiPerformanceLogHeader->SizeOfAllEntries);
    mLoadImageCount    += PeiPerformanceLogHeader->LoadImageCount;

    //
    // Get next performance guid hob
    //
    GuidHob = GetNextGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid, GET_NEXT_HOB (GuidHob));
  }
}

/**
  Report Boot Perforamnce table address as report status code.

  @param  Event    The event of notify protocol.
  @param  Context  Notify event context.

**/
VOID
EFIAPI
ReportFpdtRecordBuffer (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  EFI_STATUS      Status;
  UINT64          BPDTAddr;

  if (!mFpdtBufferIsReported) {
    Status = AllocateBootPerformanceTable ();
    if (!EFI_ERROR(Status)) {
      BPDTAddr = (UINT64)(UINTN)mAcpiBootPerformanceTable;
      REPORT_STATUS_CODE_EX (
          EFI_PROGRESS_CODE,
          EFI_SOFTWARE_DXE_BS_DRIVER,
          0,
          NULL,
          &gEdkiiFpdtExtendedFirmwarePerformanceGuid,
          &BPDTAddr,
          sizeof (UINT64)
          );
    }
    //
    // Set FPDT report state to TRUE.
    //
    mFpdtBufferIsReported = TRUE;
  }
}

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  Adds a record to the end of the performance measurement log
  that contains the Handle, Token, Module and Identifier.
  The end time of the new record must be set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the created record
                                  is same as the one created by StartGauge of PERFORMANCE_PROTOCOL.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to record the measurement.

**/
EFI_STATUS
EFIAPI
StartGaugeEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  return InsertFpdtMeasurement (TRUE, Handle, Token, Module, TimeStamp, Identifier);
}

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, Module and Identifier and has an end time value of zero.
  If the record can not be found then return EFI_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the found record
                                  is same as the one found by EndGauge of PERFORMANCE_PROTOCOL.

  @retval EFI_SUCCESS             The end of  the measurement was recorded.
  @retval EFI_NOT_FOUND           The specified measurement record could not be found.

**/
EFI_STATUS
EFIAPI
EndGaugeEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  return InsertFpdtMeasurement (FALSE, Handle, Token, Module, TimeStamp, Identifier);
}

/**
  Retrieves a previously logged performance measurement.
  It can also retrieve the log created by StartGauge and EndGauge of PERFORMANCE_PROTOCOL,
  and then assign the Identifier with 0.

    !!! Not support!!!

  Retrieves the performance log entry from the performance log specified by LogEntryKey.
  If it stands for a valid entry, then EFI_SUCCESS is returned and
  GaugeDataEntryEx stores the pointer to that entry.

  @param  LogEntryKey             The key for the previous performance measurement log entry.
                                  If 0, then the first performance measurement log entry is retrieved.
  @param  GaugeDataEntryEx        The indirect pointer to the extended gauge data entry specified by LogEntryKey
                                  if the retrieval is successful.

  @retval EFI_SUCCESS             The GuageDataEntryEx is successfully found based on LogEntryKey.
  @retval EFI_NOT_FOUND           The LogEntryKey is the last entry (equals to the total entry number).
  @retval EFI_INVALIDE_PARAMETER  The LogEntryKey is not a valid entry (greater than the total entry number).
  @retval EFI_INVALIDE_PARAMETER  GaugeDataEntryEx is NULL.

**/
EFI_STATUS
EFIAPI
GetGaugeEx (
  IN  UINTN                 LogEntryKey,
  OUT GAUGE_DATA_ENTRY_EX   **GaugeDataEntryEx
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  Adds a record to the end of the performance measurement log
  that contains the Handle, Token, and Module.
  The end time of the new record must be set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to record the measurement.

**/
EFI_STATUS
EFIAPI
StartGauge (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  return StartGaugeEx (Handle, Token, Module, TimeStamp, 0);
}

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, and Module and has an end time value of zero.
  If the record can not be found then return EFI_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval EFI_SUCCESS             The end of  the measurement was recorded.
  @retval EFI_NOT_FOUND           The specified measurement record could not be found.

**/
EFI_STATUS
EFIAPI
EndGauge (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  return EndGaugeEx (Handle, Token, Module, TimeStamp, 0);
}

/**
  Retrieves a previously logged performance measurement.
  It can also retrieve the log created by StartGaugeEx and EndGaugeEx of PERFORMANCE_EX_PROTOCOL,
  and then eliminate the Identifier.

    !!! Not support!!!

  Retrieves the performance log entry from the performance log specified by LogEntryKey.
  If it stands for a valid entry, then EFI_SUCCESS is returned and
  GaugeDataEntry stores the pointer to that entry.

  @param  LogEntryKey             The key for the previous performance measurement log entry.
                                  If 0, then the first performance measurement log entry is retrieved.
  @param  GaugeDataEntry          The indirect pointer to the gauge data entry specified by LogEntryKey
                                  if the retrieval is successful.

  @retval EFI_SUCCESS             The GuageDataEntry is successfully found based on LogEntryKey.
  @retval EFI_NOT_FOUND           The LogEntryKey is the last entry (equals to the total entry number).
  @retval EFI_INVALIDE_PARAMETER  The LogEntryKey is not a valid entry (greater than the total entry number).
  @retval EFI_INVALIDE_PARAMETER  GaugeDataEntry is NULL.

**/
EFI_STATUS
EFIAPI
GetGauge (
  IN  UINTN               LogEntryKey,
  OUT GAUGE_DATA_ENTRY    **GaugeDataEntry
  )
{
  return EFI_UNSUPPORTED;
}


/**
  The constructor function initializes Performance infrastructure for DXE phase.

  The constructor function publishes Performance and PerformanceEx protocol, allocates memory to log DXE performance
  and merges PEI performance data to DXE performance log.
  It will ASSERT() if one of these operations fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeCorePerformanceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_EVENT                 ReadyToBootEvent;
  PERFORMANCE_PROPERTY      *PerformanceProperty;

  if (!PerformanceMeasurementEnabled ()) {
    //
    // Do not initialize performance infrastructure if not required.
    //
    return EFI_SUCCESS;
  }

  //
  // Dump normal PEI performance records
  //
  InternalGetPeiPerformance (GetHobList());

  //
  // Install the protocol interfaces for DXE performance library instance.
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gPerformanceProtocolGuid,
                  &mPerformanceInterface,
                  &gPerformanceExProtocolGuid,
                  &mPerformanceExInterface,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register ReadyToBoot event to report StatusCode data
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ReportFpdtRecordBuffer,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );

  ASSERT_EFI_ERROR (Status);

  Status = EfiGetSystemConfigurationTable (&gPerformanceProtocolGuid, (VOID **) &PerformanceProperty);
  if (EFI_ERROR (Status)) {
    //
    // Install configuration table for performance property.
    //
    mPerformanceProperty.Revision  = PERFORMANCE_PROPERTY_REVISION;
    mPerformanceProperty.Reserved  = 0;
    mPerformanceProperty.Frequency = GetPerformanceCounterProperties (
                                       &mPerformanceProperty.TimerStartValue,
                                       &mPerformanceProperty.TimerEndValue
                                       );
    Status = gBS->InstallConfigurationTable (&gPerformanceProtocolGuid, &mPerformanceProperty);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  Adds a record to the end of the performance measurement log
  that contains the Handle, Token, Module and Identifier.
  The end time of the new record must be set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the created record
                                  is same as the one created by StartPerformanceMeasurement.

  @retval RETURN_SUCCESS          The start of the measurement was recorded.
  @retval RETURN_OUT_OF_RESOURCES There are not enough resources to record the measurement.

**/
RETURN_STATUS
EFIAPI
StartPerformanceMeasurementEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  return InsertFpdtMeasurement (TRUE, Handle, Token, Module, TimeStamp, Identifier);
}

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, Module and Identifier and has an end time value of zero.
  If the record can not be found then return RETURN_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the found record
                                  is same as the one found by EndPerformanceMeasurement.

  @retval RETURN_SUCCESS          The end of  the measurement was recorded.
  @retval RETURN_NOT_FOUND        The specified measurement record could not be found.

**/
RETURN_STATUS
EFIAPI
EndPerformanceMeasurementEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  return InsertFpdtMeasurement (FALSE, Handle, Token, Module, TimeStamp, Identifier);
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurement and EndPerformanceMeasurement,
  and then assign the Identifier with 0.

    !!! Not support!!!

  Attempts to retrieve the performance log entry specified by LogEntryKey.  If LogEntryKey is
  zero on entry, then an attempt is made to retrieve the first entry from the performance log,
  and the key for the second entry in the log is returned.  If the performance log is empty,
  then no entry is retrieved and zero is returned.  If LogEntryKey is not zero, then the performance
  log entry associated with LogEntryKey is retrieved, and the key for the next entry in the log is
  returned.  If LogEntryKey is the key for the last entry in the log, then the last log entry is
  retrieved and an implementation specific non-zero key value that specifies the end of the performance
  log is returned.  If LogEntryKey is equal this implementation specific non-zero key value, then no entry
  is retrieved and zero is returned.  In the cases where a performance log entry can be returned,
  the log entry is returned in Handle, Token, Module, StartTimeStamp, EndTimeStamp and Identifier.
  If LogEntryKey is not a valid log entry key for the performance measurement log, then ASSERT().
  If Handle is NULL, then ASSERT().
  If Token is NULL, then ASSERT().
  If Module is NULL, then ASSERT().
  If StartTimeStamp is NULL, then ASSERT().
  If EndTimeStamp is NULL, then ASSERT().
  If Identifier is NULL, then ASSERT().

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance log entry.
  @param  Handle                  Pointer to environment specific context used to identify the component
                                  being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string that identifies the component
                                  being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string that identifies the module
                                  being measured.
  @param  StartTimeStamp          Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was started.
  @param  EndTimeStamp            Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was ended.
  @param  Identifier              Pointer to the 32-bit identifier that was recorded when the measurement
                                  was ended.

  @return The key for the next performance log entry (in general case).

**/
UINTN
EFIAPI
GetPerformanceMeasurementEx (
  IN  UINTN       LogEntryKey,
  OUT CONST VOID  **Handle,
  OUT CONST CHAR8 **Token,
  OUT CONST CHAR8 **Module,
  OUT UINT64      *StartTimeStamp,
  OUT UINT64      *EndTimeStamp,
  OUT UINT32      *Identifier
  )
{
  return 0;
}

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  Adds a record to the end of the performance measurement log
  that contains the Handle, Token, and Module.
  The end time of the new record must be set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval RETURN_SUCCESS          The start of the measurement was recorded.
  @retval RETURN_OUT_OF_RESOURCES There are not enough resources to record the measurement.

**/
RETURN_STATUS
EFIAPI
StartPerformanceMeasurement (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  return InsertFpdtMeasurement (TRUE, Handle, Token, Module, TimeStamp, 0);
}

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, and Module and has an end time value of zero.
  If the record can not be found then return RETURN_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval RETURN_SUCCESS          The end of  the measurement was recorded.
  @retval RETURN_NOT_FOUND        The specified measurement record could not be found.

**/
RETURN_STATUS
EFIAPI
EndPerformanceMeasurement (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  return InsertFpdtMeasurement (FALSE, Handle, Token, Module, TimeStamp, 0);
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurementEx and EndPerformanceMeasurementEx,
  and then eliminate the Identifier.

  !!! Not support!!!

  Attempts to retrieve the performance log entry specified by LogEntryKey.  If LogEntryKey is
  zero on entry, then an attempt is made to retrieve the first entry from the performance log,
  and the key for the second entry in the log is returned.  If the performance log is empty,
  then no entry is retrieved and zero is returned.  If LogEntryKey is not zero, then the performance
  log entry associated with LogEntryKey is retrieved, and the key for the next entry in the log is
  returned.  If LogEntryKey is the key for the last entry in the log, then the last log entry is
  retrieved and an implementation specific non-zero key value that specifies the end of the performance
  log is returned.  If LogEntryKey is equal this implementation specific non-zero key value, then no entry
  is retrieved and zero is returned.  In the cases where a performance log entry can be returned,
  the log entry is returned in Handle, Token, Module, StartTimeStamp, and EndTimeStamp.
  If LogEntryKey is not a valid log entry key for the performance measurement log, then ASSERT().
  If Handle is NULL, then ASSERT().
  If Token is NULL, then ASSERT().
  If Module is NULL, then ASSERT().
  If StartTimeStamp is NULL, then ASSERT().
  If EndTimeStamp is NULL, then ASSERT().

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance log entry.
  @param  Handle                  Pointer to environment specific context used to identify the component
                                  being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string that identifies the component
                                  being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string that identifies the module
                                  being measured.
  @param  StartTimeStamp          Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was started.
  @param  EndTimeStamp            Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was ended.

  @return The key for the next performance log entry (in general case).

**/
UINTN
EFIAPI
GetPerformanceMeasurement (
  IN  UINTN       LogEntryKey,
  OUT CONST VOID  **Handle,
  OUT CONST CHAR8 **Token,
  OUT CONST CHAR8 **Module,
  OUT UINT64      *StartTimeStamp,
  OUT UINT64      *EndTimeStamp
  )
{
  return 0;
}

/**
  Returns TRUE if the performance measurement macros are enabled.

  This function returns TRUE if the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of
  PcdPerformanceLibraryPropertyMask is set.  Otherwise FALSE is returned.

  @retval TRUE                    The PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of
                                  PcdPerformanceLibraryPropertyMask is set.
  @retval FALSE                   The PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of
                                  PcdPerformanceLibraryPropertyMask is clear.

**/
BOOLEAN
EFIAPI
PerformanceMeasurementEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);
}
