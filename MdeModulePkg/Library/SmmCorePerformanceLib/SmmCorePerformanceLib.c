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

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "SmmCorePerformanceLibInternal.h"

#define STRING_SIZE                       (FPDT_STRING_EVENT_RECORD_NAME_LENGTH * sizeof (CHAR8))
#define FIRMWARE_RECORD_BUFFER            0x1000
#define CACHE_HANDLE_GUID_COUNT           0x100

SMM_BOOT_PERFORMANCE_TABLE    *mSmmBootPerformanceTable = NULL;

typedef struct {
  EFI_HANDLE    Handle;
  CHAR8         NameString[FPDT_STRING_EVENT_RECORD_NAME_LENGTH];
  EFI_GUID      ModuleGuid;
} HANDLE_GUID_MAP;

HANDLE_GUID_MAP      mCacheHandleGuidTable[CACHE_HANDLE_GUID_COUNT];
UINTN                mCachePairCount = 0;

UINT32               mPerformanceLength    = 0;
UINT32               mMaxPerformanceLength = 0;
BOOLEAN              mFpdtDataIsReported   = FALSE;
BOOLEAN              mLackSpaceIsReport    = FALSE;
CHAR8                *mPlatformLanguage    = NULL;
SPIN_LOCK            mSmmFpdtLock;
PERFORMANCE_PROPERTY  mPerformanceProperty;

//
// Interfaces for SMM Performance Protocol.
//
PERFORMANCE_PROTOCOL mPerformanceInterface = {
  StartGauge,
  EndGauge,
  GetGauge
};

//
// Interfaces for SMM PerformanceEx Protocol.
//
PERFORMANCE_EX_PROTOCOL mPerformanceExInterface = {
  StartGaugeEx,
  EndGaugeEx,
  GetGaugeEx
};

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

  @retval EFI_SUCCESS             Get record info successfully.
  @retval EFI_UNSUPPORTED         No matched FPDT record.

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

  RecordType = FPDT_DYNAMIC_STRING_EVENT_TYPE;

  //
  // Token to Type and Id.
  //
  if (Token != NULL) {
    if (AsciiStrCmp (Token, START_IMAGE_TOK) == 0) {              // "StartImage:"
      *UseModuleName = TRUE;
      RecordType     = FPDT_GUID_EVENT_TYPE;
      if (IsStart) {
        RecordInfo->ProgressID  = MODULE_START_ID;
      } else {
        RecordInfo->ProgressID  = MODULE_END_ID;
      }
    } else if (AsciiStrCmp (Token, LOAD_IMAGE_TOK) == 0) {        // "LoadImage:"
      *UseModuleName = TRUE;
      RecordType     = FPDT_GUID_QWORD_EVENT_TYPE;
      if (IsStart) {
        RecordInfo->ProgressID  = MODULE_LOADIMAGE_START_ID;
      } else {
        RecordInfo->ProgressID  = MODULE_LOADIMAGE_END_ID;
      }
    } else {                                                      // Pref used in Modules
      if (IsStart) {
        RecordInfo->ProgressID  = PERF_INMODULE_START_ID;
      } else {
        RecordInfo->ProgressID  = PERF_INMODULE_END_ID;
      }
    }
  } else if (Handle != NULL || Module != NULL) {                 // Pref used in Modules
    if (IsStart) {
      RecordInfo->ProgressID    = PERF_INMODULE_START_ID;
    } else {
      RecordInfo->ProgressID    = PERF_INMODULE_END_ID;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
    RecordType               = FPDT_DYNAMIC_STRING_EVENT_TYPE;
    RecordInfo->RecordSize   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD) + STRING_SIZE;
  } else {
    switch (RecordType) {
    case FPDT_GUID_EVENT_TYPE:
      RecordInfo->RecordSize  = sizeof (FPDT_GUID_EVENT_RECORD);
      break;

    case FPDT_DYNAMIC_STRING_EVENT_TYPE:
      if (*UseModuleName) {
        StringSize   = STRING_SIZE;
      } else if (Token != NULL) {
        StringSize  = AsciiStrSize (Token);
      } else if (Module != NULL) {
        StringSize  = AsciiStrSize (Module);
      } else {
        StringSize  = STRING_SIZE;
      }
      if (StringSize > STRING_SIZE) {
        StringSize  = STRING_SIZE;
      }
      RecordInfo->RecordSize  = (UINT8)(sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD) + StringSize);
      break;

    case FPDT_GUID_QWORD_EVENT_TYPE:
      RecordInfo->RecordSize  = (UINT8)sizeof (FPDT_GUID_QWORD_EVENT_RECORD);
      break;

    default:
      //
      // Record type is unsupported in SMM phase.
      //
      return EFI_UNSUPPORTED;
    }
  }

  RecordInfo->Type = RecordType;
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
  IN EFI_HANDLE        Handle,
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
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FvFilePath;

  if (NameString == NULL || BufferSize == 0) {
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

  if (ModuleGuidIsGet) {
    //
    // Method 2 Try to get the image's FFS UI section by image GUID
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
  UINT64                       TimeStamp;
  FPDT_BASIC_RECORD_INFO       RecordInfo;
  UINTN                        DestMax;
  UINTN                        StrLength;
  CONST CHAR8                  *StringPtr;
  BOOLEAN                      UseModuleName;

  StringPtr     = NULL;
  UseModuleName = FALSE;
  ZeroMem (ModuleName, sizeof (ModuleName));

  //
  // Get record info includes type, size, ProgressID.
  //
  Status = GetFpdtRecordInfo (IsStart, Handle, Token, Module, &RecordInfo, &UseModuleName);
  if (EFI_ERROR (Status)) {
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
    return EFI_UNSUPPORTED;
  } else if ((Identifier != 0) && (!IsKnownID (Identifier)) && (!IsKnownTokens (Token))) {
    if (IsStart && ((Identifier & 0x000F) != 0)) {
      Identifier &= 0xFFF0;
    } else if ((!IsStart) && ((Identifier & 0x000F) == 0)) {
      Identifier += 1;
    }
    RecordInfo.ProgressID = (UINT16)Identifier;
  }

  if (mFpdtDataIsReported) {
    //
    // Append Boot records after Smm boot performance records have been reported.
    //
    if (mPerformanceLength + RecordInfo.RecordSize > mMaxPerformanceLength) {
      if (!mLackSpaceIsReport) {
        DEBUG ((DEBUG_INFO, "SmmCorePerformanceLib: No enough space to save boot records\n"));
        mLackSpaceIsReport = TRUE;
      }
      return EFI_OUT_OF_RESOURCES;
    } else {
      //
      // Covert buffer to FPDT Ptr Union type.
      //
      FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8*)mSmmBootPerformanceTable + mSmmBootPerformanceTable->Header.Length);
    }
  } else {
    //
    // Check if pre-allocated buffer is full
    //
    if (mPerformanceLength + RecordInfo.RecordSize > mMaxPerformanceLength) {
      mSmmBootPerformanceTable = ReallocatePool (
                                   mPerformanceLength,
                                   mPerformanceLength + sizeof (SMM_BOOT_PERFORMANCE_TABLE) + RecordInfo.RecordSize + FIRMWARE_RECORD_BUFFER,
                                   mSmmBootPerformanceTable
                              );

      if (mSmmBootPerformanceTable == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      mSmmBootPerformanceTable->Header.Length = sizeof (SMM_BOOT_PERFORMANCE_TABLE) + mPerformanceLength;
      mMaxPerformanceLength = mPerformanceLength + sizeof (SMM_BOOT_PERFORMANCE_TABLE) + RecordInfo.RecordSize + FIRMWARE_RECORD_BUFFER;
    }
    //
    // Covert buffer to FPDT Ptr Union type.
    //
    FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)((UINT8*)mSmmBootPerformanceTable + mSmmBootPerformanceTable->Header.Length);
  }
  FpdtRecordPtr.RecordHeader->Length = 0;

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
    break;

  default:
    //
    // Record is not supported in current SMM phase, return EFI_UNSUPPORTED
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Update the cached FPDT record buffer.
  //
  mPerformanceLength += FpdtRecordPtr.RecordHeader->Length;
  mSmmBootPerformanceTable->Header.Length += FpdtRecordPtr.RecordHeader->Length;

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
  EFI_STATUS    Status;

  AcquireSpinLock (&mSmmFpdtLock);

  Status = InsertFpdtMeasurement (TRUE, Handle, Token, Module, TimeStamp, Identifier);

  ReleaseSpinLock (&mSmmFpdtLock);

  return Status;
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
  EFI_STATUS     Status;

  AcquireSpinLock (&mSmmFpdtLock);

  Status = InsertFpdtMeasurement (FALSE, Handle, Token, Module, TimeStamp, Identifier);

  ReleaseSpinLock (&mSmmFpdtLock);

  return Status;
}

/**
  Retrieves a previously logged performance measurement.
  It can also retrieve the log created by StartGauge and EndGauge of PERFORMANCE_PROTOCOL,
  and then assign the Identifier with 0.

  !!! Not Support!!!

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

  !!! Not Support!!!

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
  SmmReadyToBoot protocol notification event handler.

  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   SmmReadyToBootCallback runs successfully

**/
EFI_STATUS
EFIAPI
SmmReportFpdtRecordData (
  IN CONST EFI_GUID                       *Protocol,
  IN VOID                                 *Interface,
  IN EFI_HANDLE                           Handle
  )
{
  UINT64          SmmBPDTddr;

  if (!mFpdtDataIsReported && mSmmBootPerformanceTable != NULL) {
    SmmBPDTddr = (UINT64)(UINTN)mSmmBootPerformanceTable;
    REPORT_STATUS_CODE_EX (
        EFI_PROGRESS_CODE,
        EFI_SOFTWARE_SMM_DRIVER,
        0,
        NULL,
        &gEdkiiFpdtExtendedFirmwarePerformanceGuid,
        &SmmBPDTddr,
        sizeof (UINT64)
        );
    //
    // Set FPDT report state to TRUE.
    //
    mFpdtDataIsReported = TRUE;
  }
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
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  EFI_HANDLE                Handle;
  EFI_STATUS                Status;
  VOID                      *SmmReadyToBootRegistration;
  PERFORMANCE_PROPERTY      *PerformanceProperty;

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
                    &gSmmPerformanceProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPerformanceInterface
                    );
  ASSERT_EFI_ERROR (Status);
  Status = gSmst->SmmInstallProtocolInterface (
                    &Handle,
                    &gSmmPerformanceExProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPerformanceExInterface
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEdkiiSmmReadyToBootProtocolGuid,
                    SmmReportFpdtRecordData,
                    &SmmReadyToBootRegistration
                    );
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
  return (RETURN_STATUS) StartGaugeEx (Handle, Token, Module, TimeStamp, Identifier);
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
  return (RETURN_STATUS) EndGaugeEx (Handle, Token, Module, TimeStamp, Identifier);
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
  return StartGaugeEx (Handle, Token, Module, TimeStamp, 0);
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
  return EndGaugeEx (Handle, Token, Module, TimeStamp, 0);
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
