/** @file
  Performance library instance used by SMM Core.

  This library provides the performance measurement interfaces and initializes performance
  logging for the SMM phase.
  It initializes SMM phase performance logging by publishing the SMM Performance and PerformanceEx Protocol,
  which is consumed by SmmPerformanceLib to logging performance data in SMM phase.

  This library is mainly used by SMM Core to start performance logging to ensure that
  SMM Performance and PerformanceEx Protocol are installed at the very beginning of SMM phase.

 Caution: This module requires additional review when modified.
 This driver will have external input - performance data and communicate buffer in SMM mode.
 This external input must be validated carefully to avoid security issue like
 buffer overflow, integer overflow.

 SmmPerformanceHandlerEx(), SmmPerformanceHandler() will receive untrusted input and do basic validation.

Copyright (c) 2011 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmmCorePerformanceLibInternal.h"

#define STRING_SIZE              (FPDT_STRING_EVENT_RECORD_NAME_LENGTH * sizeof (CHAR8))
#define FIRMWARE_RECORD_BUFFER   0x1000
#define CACHE_HANDLE_GUID_COUNT  0x100

SMM_BOOT_PERFORMANCE_TABLE  *mSmmBootPerformanceTable = NULL;

typedef struct {
  EFI_HANDLE    Handle;
  CHAR8         NameString[FPDT_STRING_EVENT_RECORD_NAME_LENGTH];
  EFI_GUID      ModuleGuid;
} HANDLE_GUID_MAP;

HANDLE_GUID_MAP  mCacheHandleGuidTable[CACHE_HANDLE_GUID_COUNT];
UINTN            mCachePairCount = 0;

UINT32                mPerformanceLength    = sizeof (SMM_BOOT_PERFORMANCE_TABLE);
UINT32                mMaxPerformanceLength = 0;
UINT32                mLoadImageCount       = 0;
BOOLEAN               mFpdtDataIsReported   = FALSE;
BOOLEAN               mLackSpaceIsReport    = FALSE;
CHAR8                 *mPlatformLanguage    = NULL;
SPIN_LOCK             mSmmFpdtLock;
PERFORMANCE_PROPERTY  mPerformanceProperty;
UINT32                mCachedLength   = 0;
UINT32                mBootRecordSize = 0;

//
// Interfaces for SMM PerformanceMeasurement Protocol.
//
EDKII_PERFORMANCE_MEASUREMENT_PROTOCOL  mPerformanceMeasurementInterface = {
  CreatePerformanceMeasurement,
};

/**
  Return the pointer to the FPDT record in the allocated memory.

  @param  RecordSize             The size of FPDT record.
  @param  FpdtRecordPtr          Pointer the FPDT record in the allocated memory.

  @retval EFI_SUCCESS            Successfully get the pointer to the FPDT record.
  @retval EFI_OUT_OF_RESOURCES   Ran out of space to store the records.
**/
EFI_STATUS
GetFpdtRecordPtr (
  IN     UINT8            RecordSize,
  IN OUT FPDT_RECORD_PTR  *FpdtRecordPtr
  )
{
  if (mFpdtDataIsReported) {
    //
    // Append Boot records after Smm boot performance records have been reported.
    //
    if (mPerformanceLength + RecordSize > mMaxPerformanceLength) {
      if (!mLackSpaceIsReport) {
        DEBUG ((DEBUG_INFO, "SmmCorePerformanceLib: No enough space to save boot records\n"));
        mLackSpaceIsReport = TRUE;
      }

      return EFI_OUT_OF_RESOURCES;
    } else {
      //
      // Covert buffer to FPDT Ptr Union type.
      //
      FpdtRecordPtr->RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8 *)mSmmBootPerformanceTable + mSmmBootPerformanceTable->Header.Length);
    }
  } else {
    //
    // Check if pre-allocated buffer is full
    //
    if (mPerformanceLength + RecordSize > mMaxPerformanceLength) {
      mSmmBootPerformanceTable = ReallocatePool (
                                   mPerformanceLength,
                                   mPerformanceLength + RecordSize + FIRMWARE_RECORD_BUFFER,
                                   mSmmBootPerformanceTable
                                   );

      if (mSmmBootPerformanceTable == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      mSmmBootPerformanceTable->Header.Length = mPerformanceLength;
      mMaxPerformanceLength                   = mPerformanceLength + RecordSize + FIRMWARE_RECORD_BUFFER;
    }

    //
    // Covert buffer to FPDT Ptr Union type.
    //
    FpdtRecordPtr->RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8 *)mSmmBootPerformanceTable + mSmmBootPerformanceTable->Header.Length);
  }

  FpdtRecordPtr->RecordHeader->Length = 0;
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

  if ((AsciiStrCmp (Token, SEC_TOK) == 0) ||
      (AsciiStrCmp (Token, PEI_TOK) == 0) ||
      (AsciiStrCmp (Token, DXE_TOK) == 0) ||
      (AsciiStrCmp (Token, BDS_TOK) == 0) ||
      (AsciiStrCmp (Token, DRIVERBINDING_START_TOK) == 0) ||
      (AsciiStrCmp (Token, DRIVERBINDING_SUPPORT_TOK) == 0) ||
      (AsciiStrCmp (Token, DRIVERBINDING_STOP_TOK) == 0) ||
      (AsciiStrCmp (Token, LOAD_IMAGE_TOK) == 0) ||
      (AsciiStrCmp (Token, START_IMAGE_TOK) == 0) ||
      (AsciiStrCmp (Token, PEIM_TOK) == 0))
  {
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
  IN UINT32  Identifier
  )
{
  if ((Identifier == MODULE_START_ID) ||
      (Identifier == MODULE_END_ID) ||
      (Identifier == MODULE_LOADIMAGE_START_ID) ||
      (Identifier == MODULE_LOADIMAGE_END_ID) ||
      (Identifier == MODULE_DB_START_ID) ||
      (Identifier == MODULE_DB_END_ID) ||
      (Identifier == MODULE_DB_SUPPORT_START_ID) ||
      (Identifier == MODULE_DB_SUPPORT_END_ID) ||
      (Identifier == MODULE_DB_STOP_START_ID) ||
      (Identifier == MODULE_DB_STOP_END_ID))
  {
    return TRUE;
  } else {
    return FALSE;
  }
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
  IN CONST VOID                        *Handle,
  IN CONST CHAR8                       *String,
  OUT UINT16                           *ProgressID
  )
{
  //
  // Token to Id.
  //
  if (String != NULL) {
    if (AsciiStrCmp (String, START_IMAGE_TOK) == 0) {
      // "StartImage:"
      if (Attribute == PerfStartEntry) {
        *ProgressID = MODULE_START_ID;
      } else {
        *ProgressID = MODULE_END_ID;
      }
    } else if (AsciiStrCmp (String, LOAD_IMAGE_TOK) == 0) {
      // "LoadImage:"
      if (Attribute == PerfStartEntry) {
        *ProgressID = MODULE_LOADIMAGE_START_ID;
      } else {
        *ProgressID = MODULE_LOADIMAGE_END_ID;
      }
    } else {
      // Pref used in Modules
      if (Attribute == PerfStartEntry) {
        *ProgressID = PERF_INMODULE_START_ID;
      } else {
        *ProgressID = PERF_INMODULE_END_ID;
      }
    }
  } else if (Handle != NULL) {
    // Pref used in Modules
    if (Attribute == PerfStartEntry) {
      *ProgressID = PERF_INMODULE_START_ID;
    } else {
      *ProgressID = PERF_INMODULE_END_ID;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

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
EFIAPI
GetModuleInfoFromHandle (
  IN EFI_HANDLE  Handle,
  OUT CHAR8      *NameString,
  IN UINTN       BufferSize,
  OUT EFI_GUID   *ModuleGuid OPTIONAL
  )
{
  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  EFI_DRIVER_BINDING_PROTOCOL        *DriverBinding;
  CHAR8                              *PdbFileName;
  EFI_GUID                           *TempGuid;
  UINTN                              StartIndex;
  UINTN                              Index;
  INTN                               Count;
  BOOLEAN                            ModuleGuidIsGet;
  UINTN                              StringSize;
  CHAR16                             *StringPtr;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvFilePath;

  if ((NameString == NULL) || (BufferSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Try to get the ModuleGuid and name string form the caached array.
  //
  if (mCachePairCount > 0) {
    for (Count = mCachePairCount - 1; Count >= 0; Count--) {
      if (Handle == mCacheHandleGuidTable[Count].Handle) {
        CopyGuid (ModuleGuid, &mCacheHandleGuidTable[Count].ModuleGuid);
        AsciiStrCpyS (NameString, FPDT_STRING_EVENT_RECORD_NAME_LENGTH, mCacheHandleGuidTable[Count].NameString);
        return EFI_SUCCESS;
      }
    }
  }

  Status          = EFI_INVALID_PARAMETER;
  LoadedImage     = NULL;
  ModuleGuidIsGet = FALSE;

  //
  // Initialize GUID as zero value.
  //
  TempGuid = &gZeroGuid;
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
                    (VOID **)&LoadedImage
                    );

    if (EFI_ERROR (Status)) {
      //
      // Try Handle as Controller Handle
      //
      Status = gBS->OpenProtocol (
                      Handle,
                      &gEfiDriverBindingProtocolGuid,
                      (VOID **)&DriverBinding,
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
                        (VOID **)&LoadedImage
                        );
      }
    }
  }

  if (!EFI_ERROR (Status) && (LoadedImage != NULL)) {
    //
    // Get Module Guid from DevicePath.
    //
    if ((LoadedImage->FilePath != NULL) &&
        (LoadedImage->FilePath->Type == MEDIA_DEVICE_PATH) &&
        (LoadedImage->FilePath->SubType == MEDIA_PIWG_FW_FILE_DP)
        )
    {
      //
      // Determine GUID associated with module logging performance
      //
      ModuleGuidIsGet = TRUE;
      FvFilePath      = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)LoadedImage->FilePath;
      TempGuid        = &FvFilePath->FvFileName;
    }

    //
    // Method 1 Get Module Name from PDB string.
    //
    PdbFileName = PeCoffLoaderGetPdbPointer (LoadedImage->ImageBase);
    if ((PdbFileName != NULL) && (BufferSize > 0)) {
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
        if ((NameString[Index] == 0) || (NameString[Index] == '.')) {
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

  if (ModuleGuidIsGet) {
    //
    // Method 2 Try to get the image's FFS UI section by image GUID
    //
    StringPtr  = NULL;
    StringSize = 0;
    Status     = GetSectionFromAnyFv (
                   TempGuid,
                   EFI_SECTION_USER_INTERFACE,
                   0,
                   (VOID **)&StringPtr,
                   &StringSize
                   );

    if (!EFI_ERROR (Status)) {
      //
      // Method 3. Get the name string from FFS UI section
      //
      for (Index = 0; Index < BufferSize - 1 && StringPtr[Index] != 0; Index++) {
        NameString[Index] = (CHAR8)StringPtr[Index];
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
    if (IsZeroGuid (TempGuid) && (Handle != NULL) && !ModuleGuidIsGet) {
      // Handle is GUID
      CopyGuid (ModuleGuid, (EFI_GUID *)Handle);
    }
  }

  //
  // Cache the Handle and Guid pairs.
  //
  if (mCachePairCount < CACHE_HANDLE_GUID_COUNT) {
    mCacheHandleGuidTable[mCachePairCount].Handle = Handle;
    CopyGuid (&mCacheHandleGuidTable[mCachePairCount].ModuleGuid, ModuleGuid);
    AsciiStrCpyS (mCacheHandleGuidTable[mCachePairCount].NameString, FPDT_STRING_EVENT_RECORD_NAME_LENGTH, NameString);
    mCachePairCount++;
  }

  return Status;
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
  IN OUT CHAR8        *Destination,
  IN     CONST CHAR8  *Source,
  IN OUT UINT8        *Length
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

  AsciiStrnCpyS (Destination, DestMax, Source, StringLen);
  *Length += (UINT8)DestMax;

  return;
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
  IN CONST VOID                        *CallerIdentifier   OPTIONAL,
  IN CONST VOID                        *Guid     OPTIONAL,
  IN CONST CHAR8                       *String   OPTIONAL,
  IN       UINT64                      Ticker,
  IN       UINT64                      Address   OPTIONAL,
  IN       UINT16                      PerfId,
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute
  )

{
  EFI_STATUS       Status;
  EFI_GUID         ModuleGuid;
  CHAR8            ModuleName[FPDT_STRING_EVENT_RECORD_NAME_LENGTH];
  FPDT_RECORD_PTR  FpdtRecordPtr;
  FPDT_RECORD_PTR  CachedFpdtRecordPtr;
  UINT64           TimeStamp;
  CONST CHAR8      *StringPtr;
  UINTN            DestMax;
  UINTN            StringLen;
  UINT16           ProgressId;

  StringPtr = NULL;
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
    }

    if (PerfId == 0) {
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
  // 3. Get the TimeStamp.
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
      if ((PerfId == MODULE_START_ID) && (Attribute == PerfEntry)) {
        mCachedLength = mSmmBootPerformanceTable->Header.Length;
      }

      if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        FpdtRecordPtr.GuidEvent->Header.Type     = FPDT_GUID_EVENT_TYPE;
        FpdtRecordPtr.GuidEvent->Header.Length   = sizeof (FPDT_GUID_EVENT_RECORD);
        FpdtRecordPtr.GuidEvent->Header.Revision = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.GuidEvent->ProgressID      = PerfId;
        FpdtRecordPtr.GuidEvent->Timestamp       = TimeStamp;
        CopyMem (&FpdtRecordPtr.GuidEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidEvent->Guid));
        if ((CallerIdentifier == NULL) && (PerfId == MODULE_END_ID) && (mCachedLength != 0)) {
          CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8 *)mSmmBootPerformanceTable + mCachedLength);
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
        mLoadImageCount++;
        //
        // Cache the offset of load image start record and use to be updated by the load image end record if needed.
        //
        if ((CallerIdentifier == NULL) && (Attribute == PerfEntry)) {
          mCachedLength = mSmmBootPerformanceTable->Header.Length;
        }
      }

      if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        FpdtRecordPtr.GuidQwordEvent->Header.Type     = FPDT_GUID_QWORD_EVENT_TYPE;
        FpdtRecordPtr.GuidQwordEvent->Header.Length   = sizeof (FPDT_GUID_QWORD_EVENT_RECORD);
        FpdtRecordPtr.GuidQwordEvent->Header.Revision = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.GuidQwordEvent->ProgressID      = PerfId;
        FpdtRecordPtr.GuidQwordEvent->Timestamp       = TimeStamp;
        FpdtRecordPtr.GuidQwordEvent->Qword           = mLoadImageCount;
        CopyMem (&FpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof (FpdtRecordPtr.GuidQwordEvent->Guid));
        if ((PerfId == MODULE_LOADIMAGE_END_ID) && (mCachedLength != 0)) {
          CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8 *)mSmmBootPerformanceTable + mCachedLength);
          CopyMem (&CachedFpdtRecordPtr.GuidQwordEvent->Guid, &ModuleGuid, sizeof (CachedFpdtRecordPtr.GuidQwordEvent->Guid));
          mCachedLength = 0;
        }
      }

      break;

    case PERF_EVENTSIGNAL_START_ID:
    case PERF_EVENTSIGNAL_END_ID:
    case PERF_CALLBACK_START_ID:
    case PERF_CALLBACK_END_ID:
      if ((String == NULL) || (Guid == NULL)) {
        return EFI_INVALID_PARAMETER;
      }

      StringPtr = String;
      if (AsciiStrLen (String) == 0) {
        StringPtr = "unknown name";
      }

      if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        FpdtRecordPtr.DualGuidStringEvent->Header.Type     = FPDT_DUAL_GUID_STRING_EVENT_TYPE;
        FpdtRecordPtr.DualGuidStringEvent->Header.Length   = sizeof (FPDT_DUAL_GUID_STRING_EVENT_RECORD);
        FpdtRecordPtr.DualGuidStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.DualGuidStringEvent->ProgressID      = PerfId;
        FpdtRecordPtr.DualGuidStringEvent->Timestamp       = TimeStamp;
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
        FpdtRecordPtr.DynamicStringEvent->Header.Type     = FPDT_DYNAMIC_STRING_EVENT_TYPE;
        FpdtRecordPtr.DynamicStringEvent->Header.Length   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
        FpdtRecordPtr.DynamicStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.DynamicStringEvent->ProgressID      = PerfId;
        FpdtRecordPtr.DynamicStringEvent->Timestamp       = TimeStamp;
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
          FpdtRecordPtr.DynamicStringEvent->Header.Type     = FPDT_DYNAMIC_STRING_EVENT_TYPE;
          FpdtRecordPtr.DynamicStringEvent->Header.Length   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
          FpdtRecordPtr.DynamicStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
          FpdtRecordPtr.DynamicStringEvent->ProgressID      = PerfId;
          FpdtRecordPtr.DynamicStringEvent->Timestamp       = TimeStamp;
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
    if (StringPtr == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    FpdtRecordPtr.DynamicStringEvent->Header.Type     = FPDT_DYNAMIC_STRING_EVENT_TYPE;
    FpdtRecordPtr.DynamicStringEvent->Header.Length   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
    FpdtRecordPtr.DynamicStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.DynamicStringEvent->ProgressID      = PerfId;
    FpdtRecordPtr.DynamicStringEvent->Timestamp       = TimeStamp;
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

    if (((PerfId == MODULE_LOADIMAGE_END_ID) || (PerfId == MODULE_END_ID)) && (mCachedLength != 0)) {
      CachedFpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8 *)mSmmBootPerformanceTable + mCachedLength);
      if (PerfId == MODULE_LOADIMAGE_END_ID) {
        DestMax   = CachedFpdtRecordPtr.DynamicStringEvent->Header.Length - sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
        StringLen = AsciiStrLen (StringPtr);
        if (StringLen >= DestMax) {
          StringLen = DestMax -1;
        }

        CopyMem (&CachedFpdtRecordPtr.DynamicStringEvent->Guid, &ModuleGuid, sizeof (CachedFpdtRecordPtr.DynamicStringEvent->Guid));
        AsciiStrnCpyS (CachedFpdtRecordPtr.DynamicStringEvent->String, DestMax, StringPtr, StringLen);
      } else if (PerfId == MODULE_END_ID) {
        DestMax   = FpdtRecordPtr.DynamicStringEvent->Header.Length - sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
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
  mPerformanceLength                      += FpdtRecordPtr.RecordHeader->Length;
  mSmmBootPerformanceTable->Header.Length += FpdtRecordPtr.RecordHeader->Length;

  return EFI_SUCCESS;
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for report MM boot records.

  Caution: This function may receive untrusted input.
  Communicate buffer and buffer size are external input, so this function will do basic validation.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will
                                 be conveyed from a non-MM environment into an MM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.

**/
EFI_STATUS
EFIAPI
FpdtSmiHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *RegisterContext,
  IN OUT VOID        *CommBuffer,
  IN OUT UINTN       *CommBufferSize
  )
{
  EFI_STATUS                   Status;
  SMM_BOOT_RECORD_COMMUNICATE  *SmmCommData;
  UINTN                        BootRecordOffset;
  UINTN                        BootRecordSize;
  VOID                         *BootRecordData;
  UINTN                        TempCommBufferSize;
  UINT8                        *BootRecordBuffer;

  //
  // If input is invalid, stop processing this SMI
  //
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if (TempCommBufferSize < sizeof (SMM_BOOT_RECORD_COMMUNICATE)) {
    return EFI_SUCCESS;
  }

  if (!SmmIsBufferOutsideSmmValid ((UINTN)CommBuffer, TempCommBufferSize)) {
    DEBUG ((DEBUG_ERROR, "FpdtSmiHandler: MM communication data buffer in MMRAM or overflow!\n"));
    return EFI_SUCCESS;
  }

  SmmCommData = (SMM_BOOT_RECORD_COMMUNICATE *)CommBuffer;

  Status = EFI_SUCCESS;

  switch (SmmCommData->Function) {
    case SMM_FPDT_FUNCTION_GET_BOOT_RECORD_SIZE:
      if (mSmmBootPerformanceTable != NULL) {
        mBootRecordSize = mSmmBootPerformanceTable->Header.Length - sizeof (SMM_BOOT_PERFORMANCE_TABLE);
      }

      SmmCommData->BootRecordSize = mBootRecordSize;
      break;

    case SMM_FPDT_FUNCTION_GET_BOOT_RECORD_DATA:
      Status = EFI_UNSUPPORTED;
      break;

    case SMM_FPDT_FUNCTION_GET_BOOT_RECORD_DATA_BY_OFFSET:
      BootRecordOffset = SmmCommData->BootRecordOffset;
      BootRecordData   = SmmCommData->BootRecordData;
      BootRecordSize   = SmmCommData->BootRecordSize;
      if ((BootRecordData == NULL) || (BootRecordOffset >= mBootRecordSize)) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      //
      // Sanity check
      //
      if (BootRecordSize > mBootRecordSize - BootRecordOffset) {
        BootRecordSize = mBootRecordSize - BootRecordOffset;
      }

      SmmCommData->BootRecordSize = BootRecordSize;
      if (!SmmIsBufferOutsideSmmValid ((UINTN)BootRecordData, BootRecordSize)) {
        DEBUG ((DEBUG_ERROR, "FpdtSmiHandler: MM Data buffer in MMRAM or overflow!\n"));
        Status = EFI_ACCESS_DENIED;
        break;
      }

      BootRecordBuffer = ((UINT8 *)(mSmmBootPerformanceTable)) + sizeof (SMM_BOOT_PERFORMANCE_TABLE);
      CopyMem (
        (UINT8 *)BootRecordData,
        BootRecordBuffer + BootRecordOffset,
        BootRecordSize
        );
      mFpdtDataIsReported = TRUE;
      break;

    default:
      Status = EFI_UNSUPPORTED;
  }

  SmmCommData->ReturnStatus = Status;

  return EFI_SUCCESS;
}

/**
  SmmBase2 protocol notify callback function, when SMST and SMM memory service get initialized
  this function is callbacked to initialize the Smm Performance Lib

  @param  Event    The event of notify protocol.
  @param  Context  Notify event context.

**/
VOID
EFIAPI
InitializeSmmCorePerformanceLib (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_HANDLE            Handle;
  EFI_HANDLE            SmiHandle;
  EFI_STATUS            Status;
  PERFORMANCE_PROPERTY  *PerformanceProperty;

  //
  // Initialize spin lock
  //
  InitializeSpinLock (&mSmmFpdtLock);

  //
  // Install the protocol interfaces for SMM performance library instance.
  //
  Handle = NULL;
  Status = gSmst->SmmInstallProtocolInterface (
                    &Handle,
                    &gEdkiiSmmPerformanceMeasurementProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPerformanceMeasurementInterface
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Register SMI handler.
  //
  SmiHandle = NULL;
  Status    = gSmst->SmiHandlerRegister (FpdtSmiHandler, &gEfiFirmwarePerformanceGuid, &SmiHandle);
  ASSERT_EFI_ERROR (Status);

  Status = EfiGetSystemConfigurationTable (&gPerformanceProtocolGuid, (VOID **)&PerformanceProperty);
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
}

/**
  The constructor function initializes the Performance Measurement Enable flag and
  registers SmmBase2 protocol notify callback.
  It will ASSERT() if one of these operations fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCorePerformanceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;

  if (!PerformanceMeasurementEnabled ()) {
    //
    // Do not initialize performance infrastructure if not required.
    //
    return EFI_SUCCESS;
  }

  //
  // Create the events to do the library init.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InitializeSmmCorePerformanceLib,
                  NULL,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifications on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  &gEfiSmmBase2ProtocolGuid,
                  Event,
                  &Registration
                  );

  ASSERT_EFI_ERROR (Status);

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
  IN CONST VOID                        *CallerIdentifier  OPTIONAL,
  IN CONST VOID                        *Guid      OPTIONAL,
  IN CONST CHAR8                       *String    OPTIONAL,
  IN       UINT64                      TimeStamp  OPTIONAL,
  IN       UINT64                      Address    OPTIONAL,
  IN       UINT32                      Identifier,
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  AcquireSpinLock (&mSmmFpdtLock);
  Status = InsertFpdtRecord (CallerIdentifier, Guid, String, TimeStamp, Address, (UINT16)Identifier, Attribute);
  ReleaseSpinLock (&mSmmFpdtLock);
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
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  CONST CHAR8  *String;

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
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  CONST CHAR8  *String;

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

  !!! Not Support!!!

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
  @param  Identifier              Pointer to the 32-bit identifier that was recorded.

  @return The key for the next performance log entry (in general case).

**/
UINTN
EFIAPI
GetPerformanceMeasurementEx (
  IN  UINTN        LogEntryKey,
  OUT CONST VOID   **Handle,
  OUT CONST CHAR8  **Token,
  OUT CONST CHAR8  **Module,
  OUT UINT64       *StartTimeStamp,
  OUT UINT64       *EndTimeStamp,
  OUT UINT32       *Identifier
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
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
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
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
  IN UINT64       TimeStamp
  )
{
  return EndPerformanceMeasurementEx (Handle, Token, Module, TimeStamp, 0);
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurementEx and EndPerformanceMeasurementEx,
  and then eliminate the Identifier.

  !!! Not Support!!!

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
  IN  UINTN        LogEntryKey,
  OUT CONST VOID   **Handle,
  OUT CONST CHAR8  **Token,
  OUT CONST CHAR8  **Module,
  OUT UINT64       *StartTimeStamp,
  OUT UINT64       *EndTimeStamp
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
  return (BOOLEAN)((PcdGet8 (PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);
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
  IN CONST VOID   *Guid     OPTIONAL,
  IN CONST CHAR8  *String   OPTIONAL,
  IN UINT64       Address  OPTIONAL,
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
  IN  CONST UINTN  Type
  )
{
  //
  // When Performance measurement is enabled and the type is not filtered, the performance can be logged.
  //
  if (PerformanceMeasurementEnabled () && ((PcdGet8 (PcdPerformanceLibraryPropertyMask) & Type) == 0)) {
    return TRUE;
  }

  return FALSE;
}
