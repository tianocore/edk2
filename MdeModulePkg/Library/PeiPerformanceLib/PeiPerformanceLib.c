/** @file
  Performance library instance used in PEI phase.

  This file implements all APIs in Performance Library class in MdePkg. It creates
  performance logging GUIDed HOB on the first performance logging and then logs the
  performance data to the GUIDed HOB. Due to the limitation of temporary RAM, the maximum
  number of performance logging entry is specified by PcdMaxPeiPerformanceLogEntries or 
  PcdMaxPeiPerformanceLogEntries16.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiPei.h>

#include <Guid/ExtendedFirmwarePerformance.h>

#include <Library/PerformanceLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>

#define  STRING_SIZE            (FPDT_STRING_EVENT_RECORD_NAME_LENGTH * sizeof (CHAR8))
#define  MAX_RECORD_SIZE        (sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD) + STRING_SIZE)

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

  @retval EFI_SUCCESS             Get record info successfully.
  @retval EFI_UNSUPPORTED         No matched FPDT record.

**/
EFI_STATUS
GetFpdtRecordInfo (
  IN BOOLEAN                 IsStart,
  IN CONST VOID              *Handle,
  IN CONST CHAR8             *Token,
  IN CONST CHAR8             *Module,
  OUT FPDT_BASIC_RECORD_INFO *RecordInfo
  )
{
  UINTN     StringSize;
  UINT16    RecordType;

  RecordType = FPDT_DYNAMIC_STRING_EVENT_TYPE;

  //
  // Get the ProgressID based on the Token.
  // When PcdEdkiiFpdtStringRecordEnableOnly is TRUE, all records are with type of FPDT_DYNAMIC_STRING_EVENT_TYPE.
  //
  if (Token != NULL) {
    if (AsciiStrCmp (Token, LOAD_IMAGE_TOK) == 0) {               // "LoadImage:"
      if (IsStart) {
        RecordInfo->ProgressID = MODULE_LOADIMAGE_START_ID;
      } else {
        RecordInfo->ProgressID = MODULE_LOADIMAGE_END_ID;
      }
      if(!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        RecordType = FPDT_GUID_QWORD_EVENT_TYPE;
      }
    } else if (AsciiStrCmp (Token, SEC_TOK) == 0 ||               // "SEC"
               AsciiStrCmp (Token, PEI_TOK) == 0) {               // "PEI"
      if (IsStart) {
        RecordInfo->ProgressID = PERF_CROSSMODULE_START_ID;
      } else {
        RecordInfo->ProgressID = PERF_CROSSMODULE_END_ID;
      }
    } else if (AsciiStrCmp (Token, PEIM_TOK) == 0) {              // "PEIM"
      if (IsStart) {
        RecordInfo->ProgressID = MODULE_START_ID;
      } else {
        RecordInfo->ProgressID = MODULE_END_ID;
      }
      if(!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        RecordType = FPDT_GUID_EVENT_TYPE;
      }
    } else {                                                      //Pref used in Modules.
      if (IsStart) {
        RecordInfo->ProgressID = PERF_INMODULE_START_ID;
      } else {
        RecordInfo->ProgressID = PERF_INMODULE_END_ID;
      }
    }
  } else if (Module != NULL || Handle != NULL) {                  //Pref used in Modules.
    if (IsStart) {
      RecordInfo->ProgressID = PERF_INMODULE_START_ID;
    } else {
      RecordInfo->ProgressID = PERF_INMODULE_END_ID;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Get the Guid and string.
  //
  if(PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
    RecordInfo->RecordSize = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD) + STRING_SIZE;
  } else {
    switch (RecordType) {
    case FPDT_GUID_EVENT_TYPE:
      RecordInfo->RecordSize = sizeof (FPDT_GUID_EVENT_RECORD);
      break;

    case FPDT_GUID_QWORD_EVENT_TYPE:
      RecordInfo->RecordSize = sizeof (FPDT_GUID_QWORD_EVENT_RECORD);
      break;

    case FPDT_DYNAMIC_STRING_EVENT_TYPE:
      if (Token != NULL) {
        StringSize = AsciiStrSize (Token);
      } else if (Module != NULL) {
        StringSize = AsciiStrSize (Module);
      } else {
        StringSize = STRING_SIZE;
      }
      RecordInfo->RecordSize  = (UINT8)(sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD) + StringSize);
      break;

    default:
      //
      // Other type is unsupported in PEI phase yet, return EFI_UNSUPPORTED
      //
      return EFI_UNSUPPORTED;
    }
  }
  RecordInfo->Type = RecordType;
  return EFI_SUCCESS;
}

/**
  Convert PEI performance log to FPDT String boot record.

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

  @retval EFI_SUCCESS              Add FPDT boot record.
  @retval EFI_OUT_OF_RESOURCES     There are not enough resources to record the measurement.
  @retval EFI_UNSUPPORTED          No matched FPDT record.

**/
EFI_STATUS
InsertPeiFpdtMeasurement (
  IN BOOLEAN      IsStart,
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       Ticker,
  IN UINT32       Identifier
  )
{
  EFI_HOB_GUID_TYPE                     *GuidHob;
  UINTN                                 PeiPerformanceSize;
  UINT8                                 *PeiFirmwarePerformance;
  FPDT_PEI_EXT_PERF_HEADER              *PeiPerformanceLogHeader;
  FPDT_RECORD_PTR                       FpdtRecordPtr;
  FPDT_BASIC_RECORD_INFO                RecordInfo;
  CONST VOID                            *ModuleGuid;
  UINTN                                 DestMax;
  UINTN                                 StrLength;
  CONST CHAR8                           *StringPtr;
  EFI_STATUS                            Status;
  UINT16                                PeiPerformanceLogEntries;
  UINT64                                TimeStamp;

  StringPtr = NULL;
  FpdtRecordPtr.RecordHeader = NULL;
  PeiPerformanceLogHeader = NULL;

  //
  // Get record info (type, size, ProgressID and Module Guid).
  //
  Status = GetFpdtRecordInfo (IsStart, Handle, Token, Module, &RecordInfo);
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

  //
  // Get the number of PeiPerformanceLogEntries form PCD.
  //
  PeiPerformanceLogEntries = (UINT16) (PcdGet16 (PcdMaxPeiPerformanceLogEntries16) != 0 ?
                                       PcdGet16 (PcdMaxPeiPerformanceLogEntries16) :
                                       PcdGet8 (PcdMaxPeiPerformanceLogEntries));

  //
  // Create GUID HOB Data.
  //
  GuidHob = GetFirstGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid);
  PeiFirmwarePerformance = NULL;
  while (GuidHob != NULL) {
    //
    // PEI Performance HOB was found, then return the existing one.
    //
    PeiFirmwarePerformance  = (UINT8*)GET_GUID_HOB_DATA (GuidHob);
    PeiPerformanceLogHeader = (FPDT_PEI_EXT_PERF_HEADER *)PeiFirmwarePerformance;
    if (!PeiPerformanceLogHeader->HobIsFull && PeiPerformanceLogHeader->SizeOfAllEntries + RecordInfo.RecordSize > PeiPerformanceLogEntries * MAX_RECORD_SIZE) {
      PeiPerformanceLogHeader->HobIsFull = TRUE;
    }
    if (!PeiPerformanceLogHeader->HobIsFull && PeiPerformanceLogHeader->SizeOfAllEntries + RecordInfo.RecordSize <= PeiPerformanceLogEntries * MAX_RECORD_SIZE) {
      FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(PeiFirmwarePerformance + sizeof (FPDT_PEI_EXT_PERF_HEADER) + PeiPerformanceLogHeader->SizeOfAllEntries);
      break;
    }
    //
    // Previous HOB is used, then find next one.
    //
    GuidHob = GetNextGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid, GET_NEXT_HOB (GuidHob));
  }

  if (GuidHob == NULL) {
    //
    // PEI Performance HOB was not found, then build one.
    //
    PeiPerformanceSize      = sizeof (FPDT_PEI_EXT_PERF_HEADER) +
                              MAX_RECORD_SIZE * PeiPerformanceLogEntries;
    PeiFirmwarePerformance  = (UINT8*)BuildGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid, PeiPerformanceSize);
    if (PeiFirmwarePerformance != NULL) {
      ZeroMem (PeiFirmwarePerformance, PeiPerformanceSize);
    }
    PeiPerformanceLogHeader = (FPDT_PEI_EXT_PERF_HEADER *)PeiFirmwarePerformance;
    FpdtRecordPtr.RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(PeiFirmwarePerformance + sizeof (FPDT_PEI_EXT_PERF_HEADER));
  }

  if (PeiFirmwarePerformance == NULL) {
    //
    // there is no enough resource to store performance data
    //
    return EFI_OUT_OF_RESOURCES;
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
  // Get the ModuleGuid.
  //
  if (Handle != NULL) {
    ModuleGuid = Handle;
  } else {
    ModuleGuid = &gEfiCallerIdGuid;
  }

  switch (RecordInfo.Type) {
  case FPDT_GUID_EVENT_TYPE:
    FpdtRecordPtr.GuidEvent->Header.Type       = FPDT_GUID_EVENT_TYPE;
    FpdtRecordPtr.GuidEvent->Header.Length     = RecordInfo.RecordSize;;
    FpdtRecordPtr.GuidEvent->Header.Revision   = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.GuidEvent->ProgressID        = RecordInfo.ProgressID;
    FpdtRecordPtr.GuidEvent->Timestamp         = TimeStamp;
    CopyMem (&FpdtRecordPtr.GuidEvent->Guid, ModuleGuid, sizeof (EFI_GUID));
    PeiPerformanceLogHeader->SizeOfAllEntries += RecordInfo.RecordSize;
    break;

  case FPDT_GUID_QWORD_EVENT_TYPE:
    FpdtRecordPtr.GuidQwordEvent->Header.Type     = FPDT_GUID_QWORD_EVENT_TYPE;
    FpdtRecordPtr.GuidQwordEvent->Header.Length   = RecordInfo.RecordSize;;
    FpdtRecordPtr.GuidQwordEvent->Header.Revision = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.GuidQwordEvent->ProgressID      = RecordInfo.ProgressID;
    FpdtRecordPtr.GuidQwordEvent->Timestamp       = TimeStamp;
    PeiPerformanceLogHeader->LoadImageCount++;
    FpdtRecordPtr.GuidQwordEvent->Qword           = PeiPerformanceLogHeader->LoadImageCount;
    CopyMem (&FpdtRecordPtr.GuidQwordEvent->Guid, ModuleGuid, sizeof (EFI_GUID));
    PeiPerformanceLogHeader->SizeOfAllEntries += RecordInfo.RecordSize;
    break;

  case FPDT_DYNAMIC_STRING_EVENT_TYPE:
    FpdtRecordPtr.DynamicStringEvent->Header.Type       = FPDT_DYNAMIC_STRING_EVENT_TYPE;
    FpdtRecordPtr.DynamicStringEvent->Header.Length     = RecordInfo.RecordSize;
    FpdtRecordPtr.DynamicStringEvent->Header.Revision   = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.DynamicStringEvent->ProgressID        = RecordInfo.ProgressID;
    FpdtRecordPtr.DynamicStringEvent->Timestamp         = TimeStamp;
    CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, ModuleGuid, sizeof (EFI_GUID));
    PeiPerformanceLogHeader->SizeOfAllEntries += RecordInfo.RecordSize;

    if (Token != NULL) {
      StringPtr                     = Token;
    } else if (Module != NULL) {
      StringPtr                     = Module;
    }
    if (StringPtr != NULL && AsciiStrLen (StringPtr) != 0) {
      DestMax                       = (RecordInfo.RecordSize - sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD)) / sizeof (CHAR8);
      StrLength                     = AsciiStrLen (StringPtr);
      if (StrLength >= DestMax) {
        StrLength                   = DestMax -1;
      }
      AsciiStrnCpyS (FpdtRecordPtr.DynamicStringEvent->String, DestMax, StringPtr, StrLength);
    } else {
      AsciiStrCpyS (FpdtRecordPtr.DynamicStringEvent->String, FPDT_STRING_EVENT_RECORD_NAME_LENGTH, "unknown name");
    }
    break;

  default:
    //
    // Record is not supported in current PEI phase, return EFI_ABORTED
    //
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Creates a record for the beginning of a performance measurement.

  If TimeStamp is zero, then this function reads the current time stamp
  and adds that time stamp value to the record as the start time.

  If TimeStamp is one, then this function reads 0 as the start time.

  If TimeStamp is other value, then TimeStamp is added to the record as the start time.

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
  return InsertPeiFpdtMeasurement (TRUE, Handle, Token, Module, TimeStamp, Identifier);
}

/**

  Creates a record for the end of a performance measurement.

  If the TimeStamp is not zero or one, then TimeStamp is added to the record as the end time.
  If the TimeStamp is zero, then this function reads the current time stamp and adds that time stamp value to the record as the end time.
  If the TimeStamp is one, then this function reads 0 as the end time.

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
  return InsertPeiFpdtMeasurement (FALSE, Handle, Token, Module, TimeStamp, Identifier);
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurement and EndPerformanceMeasurement,
  and then assign the Identifier with 0.

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

  !!!NOT Support yet!!!

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance of entry entry.
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
  Creates a record for the beginning of a performance measurement.

  If TimeStamp is zero, then this function reads the current time stamp
  and adds that time stamp value to the record as the start time.

  If TimeStamp is one, then this function reads 0 as the start time.

  If TimeStamp is other value, then TimeStamp is added to the record as the start time.


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
  return InsertPeiFpdtMeasurement (TRUE, Handle, Token, Module, TimeStamp, 0);
}

/**

  Creates a record for the end of a performance measurement.

  If the TimeStamp is not zero or one, then TimeStamp is added to the record as the end time.
  If the TimeStamp is zero, then this function reads the current time stamp and adds that time stamp value to the record as the end time.
  If the TimeStamp is one, then this function reads 0 as the end time.

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
  return InsertPeiFpdtMeasurement (FALSE, Handle, Token, Module, TimeStamp, 0);
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurementEx and EndPerformanceMeasurementEx,
  and then eliminate the Identifier.

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

  NOT Support yet.

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance of entry entry.
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
