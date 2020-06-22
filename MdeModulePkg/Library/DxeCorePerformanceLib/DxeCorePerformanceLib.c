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
SPDX-License-Identifier: BSD-2-Clause-Patent

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
UINT32  mCachedLength         = 0;

BOOLEAN mFpdtBufferIsReported = FALSE;
BOOLEAN mLackSpaceIsReported  = FALSE;
CHAR8   *mPlatformLanguage    = NULL;
UINT8   *mPerformancePointer  = NULL;
UINT8   *mBootRecordBuffer    = NULL;
BOOLEAN  mLockInsertRecord    = FALSE;
CHAR8   *mDevicePathString    = NULL;

EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *mDevicePathToText = NULL;

//
// Interfaces for PerformanceMeasurement Protocol.
//
EDKII_PERFORMANCE_MEASUREMENT_PROTOCOL mPerformanceMeasurementInterface = {
  CreatePerformanceMeasurement,
  };

PERFORMANCE_PROPERTY  mPerformanceProperty;

/**
  Return the pointer to the FPDT record in the allocated memory.

  @param  RecordSize             The size of FPDT record.
  @param  FpdtRecordPtr          Pointer the FPDT record in the allocated memory.

  @retval EFI_SUCCESS            Successfully get the pointer to the FPDT record.
  @retval EFI_OUT_OF_RESOURCES   Ran out of space to store the records.
**/
EFI_STATUS
GetFpdtRecordPtr (
  IN     UINT8               RecordSize,
  IN OUT FPDT_RECORD_PTR     *FpdtRecordPtr
)
{
  if (mFpdtBufferIsReported) {
    //
    // Append Boot records to the boot performance table.
    //
    if (mBootRecordSize + RecordSize > mBootRecordMaxSize) {
      if (!mLackSpaceIsReported) {
        DEBUG ((DEBUG_INFO, "DxeCorePerformanceLib: No enough space to save boot records\n"));
        mLackSpaceIsReported = TRUE;
      }
      return EFI_OUT_OF_RESOURCES;
    } else {
      //
      // Save boot record into BootPerformance table
      //
      FpdtRecordPtr->RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mBootRecordBuffer + mBootRecordSize);
    }
  } else {
    //
    // Check if pre-allocated buffer is full
    //
    if (mPerformanceLength + RecordSize > mMaxPerformanceLength) {
      mPerformancePointer = ReallocatePool (
                              mPerformanceLength,
                              mPerformanceLength + RecordSize + FIRMWARE_RECORD_BUFFER,
                              mPerformancePointer
                              );
      if (mPerformancePointer == NULL) {
         return EFI_OUT_OF_RESOURCES;
       }
      mMaxPerformanceLength = mPerformanceLength + RecordSize + FIRMWARE_RECORD_BUFFER;
    }
    //
    // Covert buffer to FPDT Ptr Union type.
    //
    FpdtRecordPtr->RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mPerformancePointer + mPerformanceLength);
  }
  return EFI_SUCCESS;
}

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
    mAcpiBootPerformanceTable = (BOOT_PERFORMANCE_TABLE *) AllocatePeiAccessiblePages (
                                                             EfiReservedMemoryType,
                                                             EFI_SIZE_TO_PAGES (BootPerformanceDataSize)
                                                             );
    if (mAcpiBootPerformanceTable != NULL) {
      ZeroMem (mAcpiBootPerformanceTable, BootPerformanceDataSize);
    }
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
  Get the FPDT record identifier.

  @param Attribute                The attribute of the Record.
                                  PerfStartEntry: Start Record.
                                  PerfEndEntry: End Record.
  @param  Handle                  Pointer to environment specific context used to identify the component being measured.
  @param  String                  Pointer to a Null-terminated ASCII string that identifies the component being measured.
  @param  ProgressID              On return, pointer to the ProgressID.

  @retval EFI_SUCCESS              Get record info successfully.
  @retval EFI_INVALID_PARAMETER    No matched FPDT record.

**/
EFI_STATUS
GetFpdtRecordId (
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute,
  IN CONST VOID                       *Handle,
  IN CONST CHAR8                      *String,
  OUT UINT16                          *ProgressID
  )
{
  //
  // Token to PerfId.
  //
  if (String != NULL) {
    if (AsciiStrCmp (String, START_IMAGE_TOK) == 0) {                // "StartImage:"
      if (Attribute == PerfStartEntry) {
        *ProgressID  = MODULE_START_ID;
      } else {
        *ProgressID  = MODULE_END_ID;
      }
    } else if (AsciiStrCmp (String, LOAD_IMAGE_TOK) == 0) {          // "LoadImage:"
      if (Attribute == PerfStartEntry) {
        *ProgressID  = MODULE_LOADIMAGE_START_ID;
      } else {
        *ProgressID  = MODULE_LOADIMAGE_END_ID;
      }
    } else if (AsciiStrCmp (String, DRIVERBINDING_START_TOK) == 0) {  // "DB:Start:"
      if (Attribute == PerfStartEntry) {
        *ProgressID  = MODULE_DB_START_ID;
      } else {
        *ProgressID  = MODULE_DB_END_ID;
      }
    } else if (AsciiStrCmp (String, DRIVERBINDING_SUPPORT_TOK) == 0) { // "DB:Support:"
      if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        return RETURN_UNSUPPORTED;
      }
      if (Attribute == PerfStartEntry) {
        *ProgressID  = MODULE_DB_SUPPORT_START_ID;
      } else {
        *ProgressID  = MODULE_DB_SUPPORT_END_ID;
      }
    } else if (AsciiStrCmp (String, DRIVERBINDING_STOP_TOK) == 0) {    // "DB:Stop:"
      if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
         return RETURN_UNSUPPORTED;
      }
      if (Attribute == PerfStartEntry) {
        *ProgressID  = MODULE_DB_STOP_START_ID;
      } else {
        *ProgressID  = MODULE_DB_STOP_END_ID;
      }
    } else if (AsciiStrCmp (String, PEI_TOK) == 0 ||                   // "PEI"
               AsciiStrCmp (String, DXE_TOK) == 0 ||                   // "DXE"
               AsciiStrCmp (String, BDS_TOK) == 0) {                   // "BDS"
      if (Attribute == PerfStartEntry) {
        *ProgressID  = PERF_CROSSMODULE_START_ID;
      } else {
        *ProgressID  = PERF_CROSSMODULE_END_ID;
      }
    } else {                                                          // Pref used in Modules.
      if (Attribute == PerfStartEntry) {
        *ProgressID  = PERF_INMODULE_START_ID;
      } else {
        *ProgressID  = PERF_INMODULE_END_ID;
      }
    }
  } else if (Handle!= NULL) {                                         // Pref used in Modules.
    if (Attribute == PerfStartEntry) {
      *ProgressID    = PERF_INMODULE_START_ID;
    } else {
      *ProgressID    = PERF_INMODULE_END_ID;
    }
  } else {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

/**
  Copies the string from Source into Destination and updates Length with the
  size of the string.

  @param Destination - destination of the string copy
  @param Source      - pointer to the source string which will get copied
  @param Length      - pointer to a length variable to be updated

**/
VOID
CopyStringIntoPerfRecordAndUpdateLength (
  IN OUT CHAR8  *Destination,
  IN     CONST CHAR8  *Source,
  IN OUT UINT8  *Length
  )
{
  UINTN  StringLen;
  UINTN  DestMax;

  ASSERT (Source != NULL);

  if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
    DestMax = STRING_SIZE;
  } else {
    DestMax = AsciiStrSize (Source);
    if (DestMax > STRING_SIZE) {
      DestMax = STRING_SIZE;
    }
  }
  StringLen = AsciiStrLen (Source);
  if (StringLen >= DestMax) {
    StringLen = DestMax -1;
  }

  AsciiStrnCpyS(Destination, DestMax, Source, StringLen);
  *Length += (UINT8)DestMax;

  return;
}

/**
  Get a string description for device for the given controller handle and update record
  length. If ComponentName2 GetControllerName is supported, the value is included in the string,
  followed by device path, otherwise just device path.

  @param Handle              - Image handle
  @param ControllerHandle    - Controller handle.
  @param ComponentNameString - Pointer to a location where the string will be saved
  @param Length              - Pointer to record length to be updated

  @retval EFI_SUCCESS     - Successfully got string description for device
  @retval EFI_UNSUPPORTED - Neither ComponentName2 ControllerName nor DevicePath were found

**/
EFI_STATUS
GetDeviceInfoFromHandleAndUpdateLength (
  IN CONST VOID        *Handle,
  IN EFI_HANDLE        ControllerHandle,
  OUT CHAR8            *ComponentNameString,
  IN OUT UINT8         *Length
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *DevicePathProtocol;
  EFI_COMPONENT_NAME2_PROTOCOL  *ComponentName2;
  EFI_STATUS                    Status;
  CHAR16                        *StringPtr;
  CHAR8                         *AsciiStringPtr;
  UINTN                         ControllerNameStringSize;
  UINTN                         DevicePathStringSize;

  ControllerNameStringSize = 0;

  Status = gBS->HandleProtocol (
                  (EFI_HANDLE) Handle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **) &ComponentName2
                  );

  if (!EFI_ERROR(Status)) {
    //
    // Get the current platform language setting
    //
    if (mPlatformLanguage == NULL) {
      GetEfiGlobalVariable2 (L"PlatformLang", (VOID **)&mPlatformLanguage, NULL);
    }

    Status = ComponentName2->GetControllerName (
                               ComponentName2,
                               ControllerHandle,
                               NULL,
                               mPlatformLanguage != NULL ? mPlatformLanguage : "en-US",
                               &StringPtr
                               );
  }

  if (!EFI_ERROR (Status)) {
    //
    // This will produce the size of the unicode string, which is twice as large as the ASCII one
    // This must be an even number, so ok to divide by 2
    //
    ControllerNameStringSize = StrSize(StringPtr) / 2;

    //
    // The + 1 is because we want to add a space between the ControllerName and the device path
    //
    if ((ControllerNameStringSize + (*Length) + 1) > FPDT_MAX_PERF_RECORD_SIZE) {
      //
      // Only copy enough to fill FPDT_MAX_PERF_RECORD_SIZE worth of the record
      //
      ControllerNameStringSize = FPDT_MAX_PERF_RECORD_SIZE - (*Length) - 1;
    }

    UnicodeStrnToAsciiStrS(StringPtr, ControllerNameStringSize - 1, ComponentNameString, ControllerNameStringSize, &ControllerNameStringSize);

    //
    // Add a space in the end of the ControllerName
    //
    AsciiStringPtr = ComponentNameString + ControllerNameStringSize - 1;
    *AsciiStringPtr = 0x20;
    AsciiStringPtr++;
    *AsciiStringPtr = 0;
    ControllerNameStringSize++;

    *Length += (UINT8)ControllerNameStringSize;
  }

  //
  // This function returns the device path protocol from the handle specified by Handle.  If Handle is
  // NULL or Handle does not contain a device path protocol, then NULL is returned.
  //
  DevicePathProtocol = DevicePathFromHandle(ControllerHandle);

  if (DevicePathProtocol != NULL) {
    StringPtr = ConvertDevicePathToText (DevicePathProtocol, TRUE, FALSE);
    if (StringPtr != NULL) {
      //
      // This will produce the size of the unicode string, which is twice as large as the ASCII one
      // This must be an even number, so ok to divide by 2
      //
      DevicePathStringSize = StrSize(StringPtr) / 2;

      if ((DevicePathStringSize + (*Length)) > FPDT_MAX_PERF_RECORD_SIZE) {
        //
        // Only copy enough to fill FPDT_MAX_PERF_RECORD_SIZE worth of the record
        //
        DevicePathStringSize = FPDT_MAX_PERF_RECORD_SIZE - (*Length);
      }

      if (ControllerNameStringSize != 0) {
        AsciiStringPtr = ComponentNameString + ControllerNameStringSize - 1;
      } else {
        AsciiStringPtr = ComponentNameString;
      }

      UnicodeStrnToAsciiStrS(StringPtr, DevicePathStringSize - 1, AsciiStringPtr, DevicePathStringSize, &DevicePathStringSize);
      *Length += (UINT8)DevicePathStringSize;
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  Create performance record with event description and a timestamp.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID.
  @param Guid              - Pointer to a GUID.
  @param String            - Pointer to a string describing the measurement.
  @param Ticker            - 64-bit time stamp.
  @param Address           - Pointer to a location in memory relevant to the measurement.
  @param PerfId            - Performance identifier describing the type of measurement.
  @param Attribute         - The attribute of the measurement. According to attribute can create a start
                             record for PERF_START/PERF_START_EX, or a end record for PERF_END/PERF_END_EX,
                             or a general record for other Perf macros.

  @retval EFI_SUCCESS           - Successfully created performance record.
  @retval EFI_OUT_OF_RESOURCES  - Ran out of space to store the records.
  @retval EFI_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                  pointer or invalid PerfId.

  @retval EFI_SUCCESS           - Successfully created performance record
  @retval EFI_OUT_OF_RESOURCES  - Ran out of space to store the records
  @retval EFI_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                  pointer or invalid PerfId

**/
EFI_STATUS
InsertFpdtRecord (
  IN CONST VOID                        *CallerIdentifier,  OPTIONAL
  IN CONST VOID                        *Guid,    OPTIONAL
  IN CONST CHAR8                       *String,  OPTIONAL
  IN       UINT64                      Ticker,
  IN       UINT64                      Address,  OPTIONAL
  IN       UINT16                      PerfId,
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute
  )
{
  EFI_GUID                     ModuleGuid;
  CHAR8                        ModuleName[FPDT_STRING_EVENT_RECORD_NAME_LENGTH];
  FPDT_RECORD_PTR              FpdtRecordPtr;
  FPDT_RECORD_PTR              CachedFpdtRecordPtr;
  UINT64                       TimeStamp;
  CONST CHAR8                  *StringPtr;
  UINTN                        DestMax;
  UINTN                        StringLen;
  EFI_STATUS                   Status;
  UINT16                       ProgressId;

  StringPtr     = NULL;
  ProgressId    = 0;
  ZeroMem (ModuleName, sizeof (ModuleName));

  //
  // 1. Get the Perf Id for records from PERF_START/PERF_END, PERF_START_EX/PERF_END_EX.
  //    notes: For other Perf macros (Attribute == PerfEntry), their Id is known.
  //
  if (Attribute != PerfEntry) {
    //
    // If PERF_START_EX()/PERF_END_EX() have specified the ProgressID,it has high priority.
    // !!! Note: If the Perf is not the known Token used in the core but have same
    // ID with the core Token, this case will not be supported.
    // And in currtnt usage mode, for the unkown ID, there is a general rule:
    // If it is start pref: the lower 4 bits of the ID should be 0.
    // If it is end pref: the lower 4 bits of the ID should not be 0.
    // If input ID doesn't follow the rule, we will adjust it.
    //
    if ((PerfId != 0) && (IsKnownID (PerfId)) && (!IsKnownTokens (String))) {
      return EFI_INVALID_PARAMETER;
    } else if ((PerfId != 0) && (!IsKnownID (PerfId)) && (!IsKnownTokens (String))) {
      if ((Attribute == PerfStartEntry) && ((PerfId & 0x000F) != 0)) {
        PerfId &= 0xFFF0;
      } else if ((Attribute == PerfEndEntry) && ((PerfId & 0x000F) == 0)) {
        PerfId += 1;
      }
    } else if (PerfId == 0) {
      //
      // Get ProgressID form the String Token.
      //
      Status = GetFpdtRecordId (Attribute, CallerIdentifier, String, &ProgressId);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      PerfId = ProgressId;
    }
  }

  //
  // 2. Get the buffer to store the FPDT record.
  //
  Status = GetFpdtRecordPtr (FPDT_MAX_PERF_RECORD_SIZE, &FpdtRecordPtr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //3. Get the TimeStamp.
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
  // 4. Fill in the FPDT record according to different Performance Identifier.
  //
  switch (PerfId) {
  case MODULE_START_ID:
  case MODULE_END_ID:
    GetModuleInfoFromHandle ((EFI_HANDLE)CallerIdentifier, ModuleName, sizeof (ModuleName), &ModuleGuid);
    StringPtr = ModuleName;
    //
    // Cache the offset of start image start record and use to update the start image end record if needed.
    //
    if (Attribute == PerfEntry && PerfId == MODULE_START_ID) {
      if (mFpdtBufferIsReported) {
        mCachedLength = mBootRecordSize;
      } else {
        mCachedLength = mPerformanceLength;
      }
    }
    if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
      FpdtRecordPtr.GuidEvent->Header.Type                = FPDT_GUID_EVENT_TYPE;
      FpdtRecordPtr.GuidEvent->Header.Length              = sizeof (FPDT_GUID_EVENT_RECORD);
      FpdtRecordPtr.GuidEvent->Header.Revision            = FPDT_RECORD_REVISION_1;
      FpdtRecordPtr.GuidEvent->ProgressID                 = PerfId;
      FpdtRecordPtr.GuidEvent->Timestamp                  = TimeStamp;
      CopyMem (&FpdtRecordPtr.GuidEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidEvent->Guid));
      if (CallerIdentifier == NULL && PerfId == MODULE_END_ID && mCachedLength != 0) {
        if (mFpdtBufferIsReported) {
          CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mBootRecordBuffer + mCachedLength);
        } else {
          CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mPerformancePointer + mCachedLength);
        }
        CopyMem (&FpdtRecordPtr.GuidEvent->Guid, &CachedFpdtRecordPtr.GuidEvent->Guid, sizeof (FpdtRecordPtr.GuidEvent->Guid));
        mCachedLength = 0;
      }
    }
    break;

  case MODULE_LOADIMAGE_START_ID:
  case MODULE_LOADIMAGE_END_ID:
    GetModuleInfoFromHandle ((EFI_HANDLE)CallerIdentifier, ModuleName, sizeof (ModuleName), &ModuleGuid);
    StringPtr = ModuleName;
    if (PerfId == MODULE_LOADIMAGE_START_ID) {
      mLoadImageCount ++;
      //
      // Cache the offset of load image start record and use to be updated by the load image end record if needed.
      //
      if (CallerIdentifier == NULL && Attribute == PerfEntry) {
        if (mFpdtBufferIsReported) {
          mCachedLength = mBootRecordSize;
        } else {
          mCachedLength = mPerformanceLength;
        }
      }
    }
    if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
      FpdtRecordPtr.GuidQwordEvent->Header.Type           = FPDT_GUID_QWORD_EVENT_TYPE;
      FpdtRecordPtr.GuidQwordEvent->Header.Length         = sizeof (FPDT_GUID_QWORD_EVENT_RECORD);
      FpdtRecordPtr.GuidQwordEvent->Header.Revision       = FPDT_RECORD_REVISION_1;
      FpdtRecordPtr.GuidQwordEvent->ProgressID            = PerfId;
      FpdtRecordPtr.GuidQwordEvent->Timestamp             = TimeStamp;
      FpdtRecordPtr.GuidQwordEvent->Qword                 = mLoadImageCount;
      CopyMem (&FpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidQwordEvent->Guid));
      if (PerfId == MODULE_LOADIMAGE_END_ID && mCachedLength != 0) {
        if (mFpdtBufferIsReported) {
          CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mBootRecordBuffer + mCachedLength);
        } else {
          CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mPerformancePointer + mCachedLength);
        }
        CopyMem (&CachedFpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof (CachedFpdtRecordPtr.GuidQwordEvent->Guid));
        mCachedLength = 0;
      }
    }
    break;

  case MODULE_DB_START_ID:
  case MODULE_DB_SUPPORT_START_ID:
  case MODULE_DB_SUPPORT_END_ID:
  case MODULE_DB_STOP_START_ID:
  case MODULE_DB_STOP_END_ID:
    GetModuleInfoFromHandle ((EFI_HANDLE)CallerIdentifier, ModuleName, sizeof (ModuleName), &ModuleGuid);
    StringPtr = ModuleName;
    if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
      FpdtRecordPtr.GuidQwordEvent->Header.Type           = FPDT_GUID_QWORD_EVENT_TYPE;
      FpdtRecordPtr.GuidQwordEvent->Header.Length         = sizeof (FPDT_GUID_QWORD_EVENT_RECORD);
      FpdtRecordPtr.GuidQwordEvent->Header.Revision       = FPDT_RECORD_REVISION_1;
      FpdtRecordPtr.GuidQwordEvent->ProgressID            = PerfId;
      FpdtRecordPtr.GuidQwordEvent->Timestamp             = TimeStamp;
      FpdtRecordPtr.GuidQwordEvent->Qword                 = Address;
      CopyMem (&FpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidQwordEvent->Guid));
    }
    break;

  case MODULE_DB_END_ID:
    GetModuleInfoFromHandle ((EFI_HANDLE)CallerIdentifier, ModuleName, sizeof (ModuleName), &ModuleGuid);
    StringPtr = ModuleName;
    if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
      FpdtRecordPtr.GuidQwordStringEvent->Header.Type     = FPDT_GUID_QWORD_STRING_EVENT_TYPE;
      FpdtRecordPtr.GuidQwordStringEvent->Header.Length   = sizeof (FPDT_GUID_QWORD_STRING_EVENT_RECORD);;
      FpdtRecordPtr.GuidQwordStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
      FpdtRecordPtr.GuidQwordStringEvent->ProgressID      = PerfId;
      FpdtRecordPtr.GuidQwordStringEvent->Timestamp       = TimeStamp;
      FpdtRecordPtr.GuidQwordStringEvent->Qword           = Address;
      CopyMem (&FpdtRecordPtr.GuidQwordStringEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidQwordStringEvent->Guid));
      if (Address != 0) {
        GetDeviceInfoFromHandleAndUpdateLength(CallerIdentifier, (EFI_HANDLE)(UINTN)Address, FpdtRecordPtr.GuidQwordStringEvent->String, &FpdtRecordPtr.GuidQwordStringEvent->Header.Length);
      }
    }
    break;

  case PERF_EVENTSIGNAL_START_ID:
  case PERF_EVENTSIGNAL_END_ID:
  case PERF_CALLBACK_START_ID:
  case PERF_CALLBACK_END_ID:
    if (String == NULL || Guid == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    StringPtr = String;
    if (AsciiStrLen (String) == 0) {
      StringPtr = "unknown name";
    }
    if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
      FpdtRecordPtr.DualGuidStringEvent->Header.Type      = FPDT_DUAL_GUID_STRING_EVENT_TYPE;
      FpdtRecordPtr.DualGuidStringEvent->Header.Length    = sizeof (FPDT_DUAL_GUID_STRING_EVENT_RECORD);
      FpdtRecordPtr.DualGuidStringEvent->Header.Revision  = FPDT_RECORD_REVISION_1;
      FpdtRecordPtr.DualGuidStringEvent->ProgressID       = PerfId;
      FpdtRecordPtr.DualGuidStringEvent->Timestamp        = TimeStamp;
      CopyMem (&FpdtRecordPtr.DualGuidStringEvent->Guid1, CallerIdentifier, sizeof (FpdtRecordPtr.DualGuidStringEvent->Guid1));
      CopyMem (&FpdtRecordPtr.DualGuidStringEvent->Guid2, Guid, sizeof (FpdtRecordPtr.DualGuidStringEvent->Guid2));
      CopyStringIntoPerfRecordAndUpdateLength (FpdtRecordPtr.DualGuidStringEvent->String, StringPtr, &FpdtRecordPtr.DualGuidStringEvent->Header.Length);
    }
    break;

  case PERF_EVENT_ID:
  case PERF_FUNCTION_START_ID:
  case PERF_FUNCTION_END_ID:
  case PERF_INMODULE_START_ID:
  case PERF_INMODULE_END_ID:
  case PERF_CROSSMODULE_START_ID:
  case PERF_CROSSMODULE_END_ID:
    GetModuleInfoFromHandle ((EFI_HANDLE)CallerIdentifier, ModuleName, sizeof (ModuleName), &ModuleGuid);
    if (String != NULL) {
      StringPtr = String;
    } else {
      StringPtr = ModuleName;
    }
    if (AsciiStrLen (StringPtr) == 0) {
      StringPtr = "unknown name";
    }
    if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
      FpdtRecordPtr.DynamicStringEvent->Header.Type       = FPDT_DYNAMIC_STRING_EVENT_TYPE;
      FpdtRecordPtr.DynamicStringEvent->Header.Length     = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
      FpdtRecordPtr.DynamicStringEvent->Header.Revision   = FPDT_RECORD_REVISION_1;
      FpdtRecordPtr.DynamicStringEvent->ProgressID        = PerfId;
      FpdtRecordPtr.DynamicStringEvent->Timestamp         = TimeStamp;
      CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.DynamicStringEvent->Guid));
      CopyStringIntoPerfRecordAndUpdateLength (FpdtRecordPtr.DynamicStringEvent->String, StringPtr, &FpdtRecordPtr.DynamicStringEvent->Header.Length);
    }
    break;

  default:
    if (Attribute != PerfEntry) {
      GetModuleInfoFromHandle ((EFI_HANDLE)CallerIdentifier, ModuleName, sizeof (ModuleName), &ModuleGuid);
      if (String != NULL) {
        StringPtr = String;
      } else {
        StringPtr = ModuleName;
      }
      if (AsciiStrLen (StringPtr) == 0) {
        StringPtr = "unknown name";
      }
      if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        FpdtRecordPtr.DynamicStringEvent->Header.Type       = FPDT_DYNAMIC_STRING_EVENT_TYPE;
        FpdtRecordPtr.DynamicStringEvent->Header.Length     = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
        FpdtRecordPtr.DynamicStringEvent->Header.Revision   = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.DynamicStringEvent->ProgressID        = PerfId;
        FpdtRecordPtr.DynamicStringEvent->Timestamp         = TimeStamp;
        CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.DynamicStringEvent->Guid));
        CopyStringIntoPerfRecordAndUpdateLength (FpdtRecordPtr.DynamicStringEvent->String, StringPtr, &FpdtRecordPtr.DynamicStringEvent->Header.Length);
      }
    } else {
      return EFI_INVALID_PARAMETER;
    }
    break;
  }

  //
  // 4.2 When PcdEdkiiFpdtStringRecordEnableOnly==TRUE, create string record for all Perf entries.
  //
  if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
    if (StringPtr == NULL ||PerfId == MODULE_DB_SUPPORT_START_ID || PerfId == MODULE_DB_SUPPORT_END_ID) {
      return EFI_INVALID_PARAMETER;
    }
    FpdtRecordPtr.DynamicStringEvent->Header.Type       = FPDT_DYNAMIC_STRING_EVENT_TYPE;
    FpdtRecordPtr.DynamicStringEvent->Header.Length     = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
    FpdtRecordPtr.DynamicStringEvent->Header.Revision   = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.DynamicStringEvent->ProgressID        = PerfId;
    FpdtRecordPtr.DynamicStringEvent->Timestamp         = TimeStamp;
    if (Guid != NULL) {
      //
      // Cache the event guid in string event record.
      //
      CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, Guid, sizeof (FpdtRecordPtr.DynamicStringEvent->Guid));
    } else {
      CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.DynamicStringEvent->Guid));
    }
    if (AsciiStrLen (StringPtr) == 0) {
      StringPtr = "unknown name";
    }
    CopyStringIntoPerfRecordAndUpdateLength (FpdtRecordPtr.DynamicStringEvent->String, StringPtr, &FpdtRecordPtr.DynamicStringEvent->Header.Length);

    if ((PerfId == MODULE_LOADIMAGE_START_ID) || (PerfId == MODULE_END_ID)) {
      FpdtRecordPtr.DynamicStringEvent->Header.Length = (UINT8)(sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD)+ STRING_SIZE);
    }
    if ((PerfId == MODULE_LOADIMAGE_END_ID || PerfId == MODULE_END_ID) && mCachedLength != 0) {
      if (mFpdtBufferIsReported) {
        CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mBootRecordBuffer + mCachedLength);
      } else {
        CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(mPerformancePointer + mCachedLength);
      }
      if (PerfId == MODULE_LOADIMAGE_END_ID) {
        DestMax = CachedFpdtRecordPtr.DynamicStringEvent->Header.Length - sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
        StringLen = AsciiStrLen (StringPtr);
        if (StringLen >= DestMax) {
          StringLen = DestMax -1;
        }
        CopyMem (&CachedFpdtRecordPtr.DynamicStringEvent->Guid, &ModuleGuid, sizeof (CachedFpdtRecordPtr.DynamicStringEvent->Guid));
        AsciiStrnCpyS (CachedFpdtRecordPtr.DynamicStringEvent->String, DestMax, StringPtr, StringLen);
      } else if (PerfId == MODULE_END_ID) {
        DestMax = FpdtRecordPtr.DynamicStringEvent->Header.Length - sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
        StringLen = AsciiStrLen (CachedFpdtRecordPtr.DynamicStringEvent->String);
        if (StringLen >= DestMax) {
          StringLen = DestMax -1;
        }
        CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, &CachedFpdtRecordPtr.DynamicStringEvent->Guid, sizeof (CachedFpdtRecordPtr.DynamicStringEvent->Guid));
        AsciiStrnCpyS (FpdtRecordPtr.DynamicStringEvent->String, DestMax, CachedFpdtRecordPtr.DynamicStringEvent->String, StringLen);
      }
      mCachedLength = 0;
    }
  }

  //
  // 5. Update the length of the used buffer after fill in the record.
  //
  if (mFpdtBufferIsReported) {
    mBootRecordSize += FpdtRecordPtr.RecordHeader->Length;
    mAcpiBootPerformanceTable->Header.Length += FpdtRecordPtr.RecordHeader->Length;
  } else {
    mPerformanceLength += FpdtRecordPtr.RecordHeader->Length;
  }
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
                  &gEdkiiPerformanceMeasurementProtocolGuid,
                  &mPerformanceMeasurementInterface,
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
  Create performance record with event description and a timestamp.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID.
  @param Guid              - Pointer to a GUID.
  @param String            - Pointer to a string describing the measurement.
  @param TimeStamp         - 64-bit time stamp.
  @param Address           - Pointer to a location in memory relevant to the measurement.
  @param Identifier        - Performance identifier describing the type of measurement.
  @param Attribute         - The attribute of the measurement. According to attribute can create a start
                             record for PERF_START/PERF_START_EX, or a end record for PERF_END/PERF_END_EX,
                             or a general record for other Perf macros.

  @retval EFI_SUCCESS           - Successfully created performance record.
  @retval EFI_OUT_OF_RESOURCES  - Ran out of space to store the records.
  @retval EFI_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                  pointer or invalid PerfId.
**/
EFI_STATUS
EFIAPI
CreatePerformanceMeasurement (
  IN CONST VOID                        *CallerIdentifier,
  IN CONST VOID                        *Guid,   OPTIONAL
  IN CONST CHAR8                       *String, OPTIONAL
  IN       UINT64                      TimeStamp,
  IN       UINT64                      Address,  OPTIONAL
  IN       UINT32                      Identifier,
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute
  )
{
  EFI_STATUS   Status;

  Status = EFI_SUCCESS;

  if (mLockInsertRecord) {
    return EFI_INVALID_PARAMETER;
  }
  mLockInsertRecord = TRUE;

  Status = InsertFpdtRecord (CallerIdentifier, Guid, String, TimeStamp, Address, (UINT16)Identifier, Attribute);

  mLockInsertRecord = FALSE;

  return Status;
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
  CONST CHAR8     *String;

  if (Token != NULL) {
    String = Token;
  } else if (Module != NULL) {
    String = Module;
  } else {
    String = NULL;
  }

  return (RETURN_STATUS)CreatePerformanceMeasurement (Handle, NULL, String, TimeStamp, 0, Identifier, PerfStartEntry);
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
  CONST CHAR8     *String;

  if (Token != NULL) {
    String = Token;
  } else if (Module != NULL) {
    String = Module;
  } else {
    String = NULL;
  }

  return (RETURN_STATUS)CreatePerformanceMeasurement (Handle, NULL, String, TimeStamp, 0, Identifier, PerfEndEntry);
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
  return StartPerformanceMeasurementEx (Handle, Token, Module, TimeStamp, 0);
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
  return EndPerformanceMeasurementEx (Handle, Token, Module, TimeStamp, 0);
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

/**
  Create performance record with event description and a timestamp.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID
  @param Guid              - Pointer to a GUID
  @param String            - Pointer to a string describing the measurement
  @param Address           - Pointer to a location in memory relevant to the measurement
  @param Identifier        - Performance identifier describing the type of measurement

  @retval RETURN_SUCCESS           - Successfully created performance record
  @retval RETURN_OUT_OF_RESOURCES  - Ran out of space to store the records
  @retval RETURN_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                     pointer or invalid PerfId

**/
RETURN_STATUS
EFIAPI
LogPerformanceMeasurement (
  IN CONST VOID   *CallerIdentifier,
  IN CONST VOID   *Guid,    OPTIONAL
  IN CONST CHAR8  *String,  OPTIONAL
  IN UINT64       Address, OPTIONAL
  IN UINT32       Identifier
  )
{
  return (RETURN_STATUS)CreatePerformanceMeasurement (CallerIdentifier, Guid, String, 0, Address, Identifier, PerfEntry);
}

/**
  Check whether the specified performance measurement can be logged.

  This function returns TRUE when the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of PcdPerformanceLibraryPropertyMask is set
  and the Type disable bit in PcdPerformanceLibraryPropertyMask is not set.

  @param Type        - Type of the performance measurement entry.

  @retval TRUE         The performance measurement can be logged.
  @retval FALSE        The performance measurement can NOT be logged.

**/
BOOLEAN
EFIAPI
LogPerformanceMeasurementEnabled (
  IN  CONST UINTN        Type
  )
{
  //
  // When Performance measurement is enabled and the type is not filtered, the performance can be logged.
  //
  if (PerformanceMeasurementEnabled () && (PcdGet8(PcdPerformanceLibraryPropertyMask) & Type) == 0) {
    return TRUE;
  }
  return FALSE;
}
