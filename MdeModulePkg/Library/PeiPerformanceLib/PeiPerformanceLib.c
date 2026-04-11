/** @file
  Performance library instance used in PEI phase.

  This file implements all APIs in Performance Library class in MdePkg. It creates
  performance logging GUIDed HOB on the first performance logging and then logs the
  performance data to the GUIDed HOB. Due to the limitation of temporary RAM, the maximum
  number of performance logging entry is specified by PcdMaxPeiPerformanceLogEntries or
  PcdMaxPeiPerformanceLogEntries16.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/ExtendedFirmwarePerformance.h>
#include <Guid/PerformanceMeasurement.h>

#include <Library/PerformanceLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>

#define  STRING_SIZE          (FPDT_STRING_EVENT_RECORD_NAME_LENGTH * sizeof (CHAR8))
#define  PEI_MAX_RECORD_SIZE  (sizeof (FPDT_DUAL_GUID_STRING_EVENT_RECORD) + STRING_SIZE)

/**
  Return the pointer to the FPDT record in the allocated memory.

  @param  RecordSize                The size of FPDT record.
  @param  FpdtRecordPtr             Pointer the FPDT record in the allocated memory.
  @param  PeiPerformanceLogHeader   Pointer to the header of the PEI Performance records in the GUID Hob.

  @retval EFI_SUCCESS               Successfully get the pointer to the FPDT record.
  @retval EFI_OUT_OF_RESOURCES      Ran out of space to store the records.
**/
EFI_STATUS
GetFpdtRecordPtr (
  IN     UINT8                     RecordSize,
  IN OUT FPDT_RECORD_PTR           *FpdtRecordPtr,
  IN OUT FPDT_PEI_EXT_PERF_HEADER  **PeiPerformanceLogHeader
  )
{
  UINT16             PeiPerformanceLogEntries;
  UINTN              PeiPerformanceSize;
  UINT8              *PeiFirmwarePerformance;
  EFI_HOB_GUID_TYPE  *GuidHob;

  //
  // Get the number of PeiPerformanceLogEntries form PCD.
  //
  PeiPerformanceLogEntries = (UINT16)(PcdGet16 (PcdMaxPeiPerformanceLogEntries16) != 0 ?
                                      PcdGet16 (PcdMaxPeiPerformanceLogEntries16) :
                                      PcdGet8 (PcdMaxPeiPerformanceLogEntries));

  //
  // Create GUID HOB Data.
  //
  GuidHob                = GetFirstGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid);
  PeiFirmwarePerformance = NULL;
  while (GuidHob != NULL) {
    //
    // PEI Performance HOB was found, then return the existing one.
    //
    PeiFirmwarePerformance   = (UINT8 *)GET_GUID_HOB_DATA (GuidHob);
    *PeiPerformanceLogHeader = (FPDT_PEI_EXT_PERF_HEADER *)PeiFirmwarePerformance;
    if (!(*PeiPerformanceLogHeader)->HobIsFull && ((*PeiPerformanceLogHeader)->SizeOfAllEntries + RecordSize > (PeiPerformanceLogEntries * PEI_MAX_RECORD_SIZE))) {
      (*PeiPerformanceLogHeader)->HobIsFull = TRUE;
    }

    if (!(*PeiPerformanceLogHeader)->HobIsFull && ((*PeiPerformanceLogHeader)->SizeOfAllEntries + RecordSize <= (PeiPerformanceLogEntries * PEI_MAX_RECORD_SIZE))) {
      FpdtRecordPtr->RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(PeiFirmwarePerformance + sizeof (FPDT_PEI_EXT_PERF_HEADER) + (*PeiPerformanceLogHeader)->SizeOfAllEntries);
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
    PeiPerformanceSize = sizeof (FPDT_PEI_EXT_PERF_HEADER) +
                         PEI_MAX_RECORD_SIZE * PeiPerformanceLogEntries;
    PeiFirmwarePerformance = (UINT8 *)BuildGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid, PeiPerformanceSize);
    if (PeiFirmwarePerformance != NULL) {
      ZeroMem (PeiFirmwarePerformance, PeiPerformanceSize);
      (*PeiPerformanceLogHeader)  = (FPDT_PEI_EXT_PERF_HEADER *)PeiFirmwarePerformance;
      FpdtRecordPtr->RecordHeader = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER *)(PeiFirmwarePerformance + sizeof (FPDT_PEI_EXT_PERF_HEADER));
    }
  }

  if (PeiFirmwarePerformance == NULL) {
    //
    // there is no enough resource to store performance data
    //
    return EFI_OUT_OF_RESOURCES;
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
  IN BOOLEAN      Attribute,
  IN CONST VOID   *Handle,
  IN CONST CHAR8  *String,
  OUT UINT16      *ProgressID
  )
{
  //
  // Get the ProgressID based on the Token.
  // When PcdEdkiiFpdtStringRecordEnableOnly is TRUE, all records are with type of FPDT_DYNAMIC_STRING_EVENT_TYPE.
  //
  if (String != NULL) {
    if (AsciiStrCmp (String, LOAD_IMAGE_TOK) == 0) {
      // "LoadImage:"
      if (Attribute == PerfStartEntry) {
        *ProgressID = MODULE_LOADIMAGE_START_ID;
      } else {
        *ProgressID = MODULE_LOADIMAGE_END_ID;
      }
    } else if ((AsciiStrCmp (String, SEC_TOK) == 0) ||             // "SEC"
               (AsciiStrCmp (String, PEI_TOK) == 0))               // "PEI"
    {
      if (Attribute == PerfStartEntry) {
        *ProgressID = PERF_CROSSMODULE_START_ID;
      } else {
        *ProgressID = PERF_CROSSMODULE_END_ID;
      }
    } else if (AsciiStrCmp (String, PEIM_TOK) == 0) {
      // "PEIM"
      if (Attribute == PerfStartEntry) {
        *ProgressID = MODULE_START_ID;
      } else {
        *ProgressID = MODULE_END_ID;
      }
    } else {
      // Pref used in Modules.
      if (Attribute == PerfStartEntry) {
        *ProgressID = PERF_INMODULE_START_ID;
      } else {
        *ProgressID = PERF_INMODULE_END_ID;
      }
    }
  } else if (Handle != NULL) {
    // Pref used in Modules.
    if (Attribute == PerfStartEntry) {
      *ProgressID = PERF_INMODULE_START_ID;
    } else {
      *ProgressID = PERF_INMODULE_END_ID;
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
  Convert PEI performance log to FPDT String boot record.

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
  FPDT_RECORD_PTR           FpdtRecordPtr;
  CONST VOID                *ModuleGuid;
  CONST CHAR8               *StringPtr;
  EFI_STATUS                Status;
  UINT64                    TimeStamp;
  FPDT_PEI_EXT_PERF_HEADER  *PeiPerformanceLogHeader;

  StringPtr                  = NULL;
  FpdtRecordPtr.RecordHeader = NULL;
  PeiPerformanceLogHeader    = NULL;

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
      return EFI_UNSUPPORTED;
    } else if ((PerfId != 0) && (!IsKnownID (PerfId)) && (!IsKnownTokens (String))) {
      if ((Attribute == PerfStartEntry) && ((PerfId & 0x000F) != 0)) {
        PerfId &= 0xFFF0;
      } else if ((Attribute == PerfEndEntry) && ((PerfId & 0x000F) == 0)) {
        PerfId += 1;
      }
    } else if (PerfId == 0) {
      Status = GetFpdtRecordId (Attribute, CallerIdentifier, String, &PerfId);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // 2. Get the buffer to store the FPDT record.
  //
  Status = GetFpdtRecordPtr (PEI_MAX_RECORD_SIZE, &FpdtRecordPtr, &PeiPerformanceLogHeader);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // 3 Get the TimeStamp.
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
  // 4.Get the ModuleGuid.
  //
  if (CallerIdentifier != NULL) {
    ModuleGuid = CallerIdentifier;
  } else {
    ModuleGuid = &gEfiCallerIdGuid;
  }

  //
  // 5. Fill in the FPDT record according to different Performance Identifier.
  //
  switch (PerfId) {
    case MODULE_START_ID:
    case MODULE_END_ID:
      StringPtr = PEIM_TOK;
      if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        FpdtRecordPtr.GuidEvent->Header.Type     = FPDT_GUID_EVENT_TYPE;
        FpdtRecordPtr.GuidEvent->Header.Length   = sizeof (FPDT_GUID_EVENT_RECORD);
        FpdtRecordPtr.GuidEvent->Header.Revision = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.GuidEvent->ProgressID      = PerfId;
        FpdtRecordPtr.GuidEvent->Timestamp       = TimeStamp;
        CopyMem (&FpdtRecordPtr.GuidEvent->Guid, ModuleGuid, sizeof (EFI_GUID));
      }

      break;

    case MODULE_LOADIMAGE_START_ID:
    case MODULE_LOADIMAGE_END_ID:
      StringPtr = LOAD_IMAGE_TOK;
      if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        FpdtRecordPtr.GuidQwordEvent->Header.Type     = FPDT_GUID_QWORD_EVENT_TYPE;
        FpdtRecordPtr.GuidQwordEvent->Header.Length   = sizeof (FPDT_GUID_QWORD_EVENT_RECORD);
        FpdtRecordPtr.GuidQwordEvent->Header.Revision = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.GuidQwordEvent->ProgressID      = PerfId;
        FpdtRecordPtr.GuidQwordEvent->Timestamp       = TimeStamp;
        if (PerfId == MODULE_LOADIMAGE_START_ID) {
          PeiPerformanceLogHeader->LoadImageCount++;
        }

        FpdtRecordPtr.GuidQwordEvent->Qword = PeiPerformanceLogHeader->LoadImageCount;
        CopyMem (&FpdtRecordPtr.GuidQwordEvent->Guid, ModuleGuid, sizeof (EFI_GUID));
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
        CopyMem (&FpdtRecordPtr.DualGuidStringEvent->Guid1, ModuleGuid, sizeof (FpdtRecordPtr.DualGuidStringEvent->Guid1));
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
      if ((String != NULL) && (AsciiStrLen (String) != 0)) {
        StringPtr = String;
      } else {
        StringPtr = "unknown name";
      }

      if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
        FpdtRecordPtr.DynamicStringEvent->Header.Type     = FPDT_DYNAMIC_STRING_EVENT_TYPE;
        FpdtRecordPtr.DynamicStringEvent->Header.Length   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
        FpdtRecordPtr.DynamicStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
        FpdtRecordPtr.DynamicStringEvent->ProgressID      = PerfId;
        FpdtRecordPtr.DynamicStringEvent->Timestamp       = TimeStamp;
        CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, ModuleGuid, sizeof (EFI_GUID));
        CopyStringIntoPerfRecordAndUpdateLength (FpdtRecordPtr.DynamicStringEvent->String, StringPtr, &FpdtRecordPtr.DynamicStringEvent->Header.Length);
      }

      break;

    default:
      if (Attribute != PerfEntry) {
        if ((String != NULL) && (AsciiStrLen (String) != 0)) {
          StringPtr = String;
        } else {
          StringPtr = "unknown name";
        }

        if (!PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
          FpdtRecordPtr.DynamicStringEvent->Header.Type     = FPDT_DYNAMIC_STRING_EVENT_TYPE;
          FpdtRecordPtr.DynamicStringEvent->Header.Length   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
          FpdtRecordPtr.DynamicStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
          FpdtRecordPtr.DynamicStringEvent->ProgressID      = PerfId;
          FpdtRecordPtr.DynamicStringEvent->Timestamp       = TimeStamp;
          CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, ModuleGuid, sizeof (FpdtRecordPtr.DynamicStringEvent->Guid));
          CopyStringIntoPerfRecordAndUpdateLength (FpdtRecordPtr.DynamicStringEvent->String, StringPtr, &FpdtRecordPtr.DynamicStringEvent->Header.Length);
        }
      } else {
        return EFI_INVALID_PARAMETER;
      }

      break;
  }

  //
  // 5.2 When PcdEdkiiFpdtStringRecordEnableOnly==TRUE, create string record for all Perf entries.
  //
  if (PcdGetBool (PcdEdkiiFpdtStringRecordEnableOnly)) {
    FpdtRecordPtr.DynamicStringEvent->Header.Type     = FPDT_DYNAMIC_STRING_EVENT_TYPE;
    FpdtRecordPtr.DynamicStringEvent->Header.Length   = sizeof (FPDT_DYNAMIC_STRING_EVENT_RECORD);
    FpdtRecordPtr.DynamicStringEvent->Header.Revision = FPDT_RECORD_REVISION_1;
    FpdtRecordPtr.DynamicStringEvent->ProgressID      = PerfId;
    FpdtRecordPtr.DynamicStringEvent->Timestamp       = TimeStamp;
    if (Guid != NULL) {
      //
      // Cache the event guid in string event record.
      //
      CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, Guid, sizeof (EFI_GUID));
    } else {
      CopyMem (&FpdtRecordPtr.DynamicStringEvent->Guid, ModuleGuid, sizeof (EFI_GUID));
    }

    CopyStringIntoPerfRecordAndUpdateLength (FpdtRecordPtr.DynamicStringEvent->String, StringPtr, &FpdtRecordPtr.DynamicStringEvent->Header.Length);
  }

  //
  // 6. Update the length of the used buffer after fill in the record.
  //
  PeiPerformanceLogHeader->SizeOfAllEntries += FpdtRecordPtr.RecordHeader->Length;

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

  return (RETURN_STATUS)InsertFpdtRecord (Handle, NULL, String, TimeStamp, 0, (UINT16)Identifier, PerfStartEntry);
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

  return (RETURN_STATUS)InsertFpdtRecord (Handle, NULL, String, TimeStamp, 0, (UINT16)Identifier, PerfEndEntry);
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
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
  IN UINT64       TimeStamp
  )
{
  return StartPerformanceMeasurementEx (Handle, Token, Module, TimeStamp, 0);
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
  return (RETURN_STATUS)InsertFpdtRecord (CallerIdentifier, Guid, String, 0, Address, (UINT16)Identifier, PerfEntry);
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
