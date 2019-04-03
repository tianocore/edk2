/** @file
  Shell command for Displaying Performance Metrics.

  The Dp command reads performance data and presents it in several
  different formats depending upon the needs of the user.  Both
  Trace and Measured Profiling information is processed and presented.

  Dp uses the "PerformanceLib" to read the measurement records.
  The "TimerLib" provides information about the timer, such as frequency,
  beginning, and ending counter values.
  Measurement records contain identifying information (Handle, Token, Module)
  and start and end time values.
  Dp uses this information to group records in different ways.  It also uses
  timer information to calculate elapsed time for each measurement.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.
  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Dp.h"
#include "Literals.h"
#include "DpInternal.h"

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT32                       Entry;
} RSDT_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT64                       Entry;
} XSDT_TABLE;

#pragma pack()

EFI_HANDLE   mDpHiiHandle;

typedef struct {
  EFI_HANDLE    Handle;
  EFI_GUID      ModuleGuid;
} HANDLE_GUID_MAP;

HANDLE_GUID_MAP  *mCacheHandleGuidTable;
UINTN            mCachePairCount = 0;

//
/// Module-Global Variables
///@{
CHAR16           mGaugeString[DP_GAUGE_STRING_LENGTH + 1];
CHAR16           mUnicodeToken[DXE_PERFORMANCE_STRING_SIZE];
UINT64           mInterestThreshold;
BOOLEAN          mShowId = FALSE;
UINT8            *mBootPerformanceTable;
UINTN            mBootPerformanceTableSize;
BOOLEAN          mPeiPhase = FALSE;
BOOLEAN          mDxePhase = FALSE;

PERF_SUMMARY_DATA SummaryData = { 0 };    ///< Create the SummaryData structure and init. to ZERO.
MEASUREMENT_RECORD  *mMeasurementList = NULL;
UINTN               mMeasurementNum    = 0;

/// Items for which to gather cumulative statistics.
PERF_CUM_DATA CumData[] = {
  PERF_INIT_CUM_DATA (LOAD_IMAGE_TOK),
  PERF_INIT_CUM_DATA (START_IMAGE_TOK),
  PERF_INIT_CUM_DATA (DRIVERBINDING_START_TOK),
  PERF_INIT_CUM_DATA (DRIVERBINDING_SUPPORT_TOK),
  PERF_INIT_CUM_DATA (DRIVERBINDING_STOP_TOK)
};

/// Number of items for which we are gathering cumulative statistics.
UINT32 const      NumCum = sizeof(CumData) / sizeof(PERF_CUM_DATA);

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeFlag},   // -v   Verbose Mode
  {L"-A", TypeFlag},   // -A   All, Cooked
  {L"-R", TypeFlag},   // -R   RAW All
  {L"-s", TypeFlag},   // -s   Summary
  {L"-x", TypeFlag},   // -x   eXclude Cumulative Items
  {L"-i", TypeFlag},   // -i   Display Identifier
  {L"-c", TypeValue},  // -c   Display cumulative data.
  {L"-n", TypeValue},  // -n # Number of records to display for A and R
  {L"-t", TypeValue},  // -t # Threshold of interest
  {NULL, TypeMax}
  };

///@}

/**
   Display the trailing Verbose information.
**/
VOID
DumpStatistics( void )
{
  EFI_STRING                StringPtr;
  EFI_STRING                StringPtrUnknown;
  StringPtr        = HiiGetString (mDpHiiHandle, STRING_TOKEN (STR_DP_SECTION_STATISTICS), NULL);
  StringPtrUnknown = HiiGetString (mDpHiiHandle, STRING_TOKEN (STR_ALIT_UNKNOWN), NULL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_SECTION_HEADER), mDpHiiHandle,
              (StringPtr == NULL) ? StringPtrUnknown : StringPtr);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMTRACE), mDpHiiHandle,      SummaryData.NumTrace);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMINCOMPLETE), mDpHiiHandle, SummaryData.NumIncomplete);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMPHASES), mDpHiiHandle,     SummaryData.NumSummary);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMHANDLES), mDpHiiHandle,    SummaryData.NumHandles, SummaryData.NumTrace - SummaryData.NumHandles);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMPEIMS), mDpHiiHandle,      SummaryData.NumPEIMs);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMGLOBALS), mDpHiiHandle,    SummaryData.NumGlobal);
  SHELL_FREE_NON_NULL (StringPtr);
  SHELL_FREE_NON_NULL (StringPtrUnknown);
}

/**
  Get Boot performance table form Acpi table.

**/
EFI_STATUS
GetBootPerformanceTable (
  )
{
  FIRMWARE_PERFORMANCE_TABLE  *FirmwarePerformanceTable;

  FirmwarePerformanceTable = (FIRMWARE_PERFORMANCE_TABLE *) EfiLocateFirstAcpiTable (
                                                              EFI_ACPI_5_0_FIRMWARE_PERFORMANCE_DATA_TABLE_SIGNATURE
                                                              );
  if (FirmwarePerformanceTable == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_GET_ACPI_FPDT_FAIL), mDpHiiHandle);
    return EFI_NOT_FOUND;
  }

  mBootPerformanceTable = (UINT8*) (UINTN)FirmwarePerformanceTable->BootPointerRecord.BootPerformanceTablePointer;
  mBootPerformanceTableSize = ((BOOT_PERFORMANCE_TABLE *) mBootPerformanceTable)->Header.Length;

  return EFI_SUCCESS;
}

/**
  Get Handle form Module Guid.

  @param  ModuleGuid     Module Guid.
  @param  Handle         The handle to be returned.

**/
VOID
GetHandleFormModuleGuid (
  IN      EFI_GUID        *ModuleGuid,
  IN OUT  EFI_HANDLE      *Handle
  )
{
  UINTN                             Index;

  if (IsZeroGuid (ModuleGuid)) {
    *Handle = NULL;
  }
  //
  // Try to get the Handle form the caached array.
  //
  for (Index = 0; Index < mCachePairCount; Index++) {
    if (CompareGuid (ModuleGuid, &mCacheHandleGuidTable[Index].ModuleGuid)) {
      *Handle = mCacheHandleGuidTable[Index].Handle;
      break;
    }
  }
  if (Index >= mCachePairCount) {
    *Handle = NULL;
  }
}

/**
Cache the GUID and handle mapping pairs. In order to save time for searching.

**/
EFI_STATUS
BuildCachedGuidHandleTable (
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             HandleCount;
  UINTN                             Index;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_DRIVER_BINDING_PROTOCOL       *DriverBinding;
  EFI_GUID                          *TempGuid;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FvFilePath;

  Status = gBS->LocateHandleBuffer (AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_HANDLES_ERROR), mDpHiiHandle, Status);
    return Status;
  }

  mCacheHandleGuidTable = AllocateZeroPool (HandleCount * sizeof (HANDLE_GUID_MAP));
  if (mCacheHandleGuidTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    //
    // Try Handle as ImageHandle.
    //
    Status = gBS->HandleProtocol (
                  HandleBuffer[Index],
                  &gEfiLoadedImageProtocolGuid,
                  (VOID**) &LoadedImage
                  );
    if (EFI_ERROR (Status)) {
      //
      // Try Handle as Controller Handle
      //
      Status = gBS->OpenProtocol (
                    HandleBuffer[Index],
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

    if (!EFI_ERROR (Status) && LoadedImage != NULL) {
      //
      // Get Module Guid from DevicePath.
      //
      if (LoadedImage->FilePath != NULL &&
          LoadedImage->FilePath->Type == MEDIA_DEVICE_PATH &&
          LoadedImage->FilePath->SubType == MEDIA_PIWG_FW_FILE_DP
         ) {
        FvFilePath      = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LoadedImage->FilePath;
        TempGuid        = &FvFilePath->FvFileName;

        mCacheHandleGuidTable[mCachePairCount].Handle = HandleBuffer[Index];
        CopyGuid (&mCacheHandleGuidTable[mCachePairCount].ModuleGuid, TempGuid);
        mCachePairCount ++;
      }
    }
  }
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }
  return EFI_SUCCESS;
}

/**
  Get Measurement form Fpdt records.

  @param  RecordHeader        Pointer to the start record.
  @param  IsStart             Is start record or End record.
  @param  Measurement         Pointer to the measurement which need to be filled.

**/
VOID
GetMeasurementInfo (
  IN     EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER  *RecordHeader,
  IN     BOOLEAN                                      IsStart,
  IN OUT MEASUREMENT_RECORD                           *Measurement
  )
{
  VOID                                         *ModuleGuid;
  EFI_HANDLE                                   StartHandle;

  switch (RecordHeader->Type) {
  case FPDT_GUID_EVENT_TYPE:
    ModuleGuid                    = &(((FPDT_GUID_EVENT_RECORD *)RecordHeader)->Guid);
    Measurement->Identifier       = ((UINT32)((FPDT_GUID_EVENT_RECORD *)RecordHeader)->ProgressID);
    if (IsStart) {
      Measurement->StartTimeStamp = ((FPDT_GUID_EVENT_RECORD *)RecordHeader)->Timestamp;
    } else {
      Measurement->EndTimeStamp   = ((FPDT_GUID_EVENT_RECORD *)RecordHeader)->Timestamp;
    }
    switch (Measurement->Identifier) {
    case MODULE_START_ID:
    case MODULE_END_ID:
      if (mPeiPhase) {
        Measurement->Token        = ALit_PEIM;
        Measurement->Module       = ALit_PEIM;
      } else if (mDxePhase) {
        Measurement->Token        = ALit_START_IMAGE;
        Measurement->Module       = ALit_START_IMAGE;
      }
      break;
    default:
      ASSERT(FALSE);
    }

    if (Measurement->Token != NULL && AsciiStrCmp (Measurement->Token, ALit_PEIM) == 0) {
      Measurement->Handle         = &(((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->Guid);
    } else {
      GetHandleFormModuleGuid(ModuleGuid, &StartHandle);
      Measurement->Handle = StartHandle;
      //
      // When no perf entry to record the PEI and DXE phase,
      // For start image, we need detect the PEIM and non PEIM here.
      //
      if (Measurement->Token == NULL) {
        if (StartHandle == NULL && !IsZeroGuid (ModuleGuid)) {
          Measurement->Token      = ALit_PEIM;
          Measurement->Module     = ALit_PEIM;
          Measurement->Handle     = ModuleGuid;
        } else {
          Measurement->Token      = ALit_START_IMAGE;
          Measurement->Module     = ALit_START_IMAGE;
        }
      }
    }
    break;

  case FPDT_DYNAMIC_STRING_EVENT_TYPE:
    ModuleGuid                    = &(((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->Guid);
    Measurement->Identifier       = ((UINT32)((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->ProgressID);
    if (IsStart) {
      Measurement->StartTimeStamp = ((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->Timestamp;
    } else {
      Measurement->EndTimeStamp   = ((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->Timestamp;
    }
    switch (Measurement->Identifier) {
    case MODULE_START_ID:
    case MODULE_END_ID:
      if (mPeiPhase) {
        Measurement->Token        = ALit_PEIM;
      } else if (mDxePhase) {
        Measurement->Token        = ALit_START_IMAGE;
      }
      break;

    case MODULE_LOADIMAGE_START_ID:
    case MODULE_LOADIMAGE_END_ID:
      Measurement->Token          = ALit_LOAD_IMAGE;
      break;

    case MODULE_DB_START_ID:
    case MODULE_DB_END_ID:
      Measurement->Token          = ALit_DB_START;
      break;

    case MODULE_DB_SUPPORT_START_ID:
    case MODULE_DB_SUPPORT_END_ID:
      Measurement->Token          = ALit_DB_SUPPORT;
      break;

    case MODULE_DB_STOP_START_ID:
    case MODULE_DB_STOP_END_ID:
      Measurement->Token          = ALit_DB_STOP;
      break;

    default:
      Measurement->Token          = ((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->String;
      break;
    }

    Measurement->Module           = ((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->String;

    if (Measurement->Token != NULL && AsciiStrCmp (Measurement->Token, ALit_PEIM) == 0) {
      Measurement->Handle         = &(((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->Guid);
    } else {
      GetHandleFormModuleGuid(ModuleGuid, &StartHandle);
      Measurement->Handle = StartHandle;
      //
      // When no perf entry to record the PEI and DXE phase,
      // For start image, we need detect the PEIM and non PEIM here.
      //
      if (Measurement->Token == NULL  && (Measurement->Identifier == MODULE_START_ID || Measurement->Identifier == MODULE_END_ID)) {
        if (StartHandle == NULL && !IsZeroGuid (ModuleGuid)) {
          Measurement->Token      = ALit_PEIM;
          Measurement->Handle     = ModuleGuid;
        } else {
          Measurement->Token      = ALit_START_IMAGE;
        }
      }
    }
    break;

  case FPDT_GUID_QWORD_EVENT_TYPE:
    ModuleGuid                    = &(((FPDT_GUID_QWORD_EVENT_RECORD *)RecordHeader)->Guid);
    Measurement->Identifier       = ((UINT32)((FPDT_GUID_QWORD_EVENT_RECORD *)RecordHeader)->ProgressID);
    if (IsStart) {
      Measurement->StartTimeStamp = ((FPDT_GUID_QWORD_EVENT_RECORD *)RecordHeader)->Timestamp;
    } else {
      Measurement->EndTimeStamp   = ((FPDT_GUID_QWORD_EVENT_RECORD *)RecordHeader)->Timestamp;
    }
    switch (Measurement->Identifier) {
    case MODULE_DB_START_ID:
      Measurement->Token          = ALit_DB_START;
      Measurement->Module         = ALit_DB_START;
      break;

    case MODULE_DB_SUPPORT_START_ID:
    case MODULE_DB_SUPPORT_END_ID:
      Measurement->Token          = ALit_DB_SUPPORT;
      Measurement->Module         = ALit_DB_SUPPORT;
      break;

    case MODULE_DB_STOP_START_ID:
    case MODULE_DB_STOP_END_ID:
      Measurement->Token          = ALit_DB_STOP;
      Measurement->Module         = ALit_DB_STOP;
      break;

    case MODULE_LOADIMAGE_START_ID:
    case MODULE_LOADIMAGE_END_ID:
      Measurement->Token          = ALit_LOAD_IMAGE;
      Measurement->Module         = ALit_LOAD_IMAGE;
      break;

    default:
      ASSERT(FALSE);
    }
    GetHandleFormModuleGuid(ModuleGuid, &StartHandle);
    Measurement->Handle = StartHandle;
    break;

  case  FPDT_GUID_QWORD_STRING_EVENT_TYPE:
    ModuleGuid                    = &(((FPDT_GUID_QWORD_STRING_EVENT_RECORD *)RecordHeader)->Guid);
    Measurement->Identifier       = ((UINT32)((FPDT_GUID_QWORD_STRING_EVENT_RECORD *)RecordHeader)->ProgressID);
    if (IsStart) {
      Measurement->StartTimeStamp = ((FPDT_GUID_QWORD_STRING_EVENT_RECORD*)RecordHeader)->Timestamp;
    } else {
      Measurement->EndTimeStamp   = ((FPDT_GUID_QWORD_STRING_EVENT_RECORD *)RecordHeader)->Timestamp;
    }
    //
    // Currently only "DB:Start:" end record with FPDT_GUID_QWORD_STRING_EVENT_TYPE.
    //
    switch (Measurement->Identifier) {
    case MODULE_DB_END_ID:
      Measurement->Token          = ALit_DB_START;
      Measurement->Module         = ALit_DB_START;
      break;
    default:
      ASSERT(FALSE);
    }
    GetHandleFormModuleGuid(ModuleGuid, &StartHandle);
    Measurement->Handle = StartHandle;
    break;

  case FPDT_DUAL_GUID_STRING_EVENT_TYPE:
    ModuleGuid                    = &(((FPDT_DUAL_GUID_STRING_EVENT_RECORD *)RecordHeader)->Guid1);
    Measurement->Identifier       = ((UINT32)((FPDT_DUAL_GUID_STRING_EVENT_RECORD *)RecordHeader)->ProgressID);
    if (IsStart) {
      Measurement->StartTimeStamp = ((FPDT_DUAL_GUID_STRING_EVENT_RECORD *)RecordHeader)->Timestamp;
    } else {
      Measurement->EndTimeStamp   = ((FPDT_DUAL_GUID_STRING_EVENT_RECORD *)RecordHeader)->Timestamp;
    }
    Measurement->Token            = ((FPDT_DUAL_GUID_STRING_EVENT_RECORD *)RecordHeader)->String;
    Measurement->Module           = ((FPDT_DUAL_GUID_STRING_EVENT_RECORD *)RecordHeader)->String;
    GetHandleFormModuleGuid(ModuleGuid, &StartHandle);
    Measurement->Handle = StartHandle;
    break;

  default:
    break;
  }
}

/**
  Search the start measurement in the mMeasurementList for the end measurement.

  @param  EndMeasureMent        Measurement for end record.

**/
VOID
SearchMeasurement (
  IN MEASUREMENT_RECORD                           *EndMeasureMent
  )
{
  INTN                            Index;

  for (Index = mMeasurementNum - 1; Index >= 0; Index--) {
    if (AsciiStrCmp (EndMeasureMent->Token, ALit_PEIM) == 0) {
      if (mMeasurementList[Index].EndTimeStamp == 0 && EndMeasureMent->Handle!= NULL && mMeasurementList[Index].Handle != NULL&&
          CompareGuid(mMeasurementList[Index].Handle, EndMeasureMent->Handle) &&
          (AsciiStrCmp (mMeasurementList[Index].Token, EndMeasureMent->Token) == 0) &&
          (AsciiStrCmp (mMeasurementList[Index].Module, EndMeasureMent->Module) == 0)) {
        mMeasurementList[Index].EndTimeStamp = EndMeasureMent->EndTimeStamp;
        break;
      }
    } else if (EndMeasureMent->Identifier == PERF_CROSSMODULE_END_ID) {
      if (mMeasurementList[Index].EndTimeStamp == 0 &&
         (AsciiStrCmp (mMeasurementList[Index].Token, EndMeasureMent->Token) == 0) &&
         (AsciiStrCmp (mMeasurementList[Index].Module, EndMeasureMent->Module) == 0) &&
         mMeasurementList[Index].Identifier == PERF_CROSSMODULE_START_ID) {
        mMeasurementList[Index].EndTimeStamp = EndMeasureMent->EndTimeStamp;
        break;
      }
    } else {
      if (mMeasurementList[Index].EndTimeStamp == 0 && mMeasurementList[Index].Handle == EndMeasureMent->Handle &&
         (AsciiStrCmp (mMeasurementList[Index].Token, EndMeasureMent->Token) == 0) &&
         (AsciiStrCmp (mMeasurementList[Index].Module, EndMeasureMent->Module) == 0)) {
        mMeasurementList[Index].EndTimeStamp = EndMeasureMent->EndTimeStamp;
        break;
      }
    }
  }
}

/**
  Generate the measure record array.

**/
EFI_STATUS
BuildMeasurementList (
  )
{
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER  *RecordHeader;
  UINT8                                        *PerformanceTablePtr;
  UINT16                                       StartProgressId;
  UINTN                                        TableLength;
  UINT8                                        *StartRecordEvent;
  MEASUREMENT_RECORD                           MeasureMent;

  mMeasurementList = AllocateZeroPool (mBootPerformanceTableSize);
  if (mMeasurementList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TableLength         = sizeof (BOOT_PERFORMANCE_TABLE);
  PerformanceTablePtr = (mBootPerformanceTable + TableLength);

  while (TableLength < mBootPerformanceTableSize) {
    RecordHeader      = (EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER*) PerformanceTablePtr;
    StartRecordEvent  = (UINT8 *)RecordHeader;
    StartProgressId   = ((FPDT_GUID_EVENT_RECORD *)StartRecordEvent)->ProgressID;

    //
    // If the record with ProgressId 0, the record doesn't appear in pairs. The timestamp in the record is the EndTimeStamp, its StartTimeStamp is 0.
    // If the record is the start record, fill the info to the measurement in the mMeasurementList.
    // If the record is the end record, find the related start measurement in the mMeasurementList and fill the EndTimeStamp.
    //
    if (StartProgressId == 0) {
      GetMeasurementInfo (RecordHeader, FALSE, &(mMeasurementList[mMeasurementNum]));
      mMeasurementNum ++;
    } else if (((StartProgressId >= PERF_EVENTSIGNAL_START_ID && ((StartProgressId & 0x000F) == 0)) ||
        (StartProgressId < PERF_EVENTSIGNAL_START_ID && ((StartProgressId & 0x0001) != 0)))) {
      //
      // Since PEIM and StartImage has same Type and ID when PCD PcdEdkiiFpdtStringRecordEnableOnly = FALSE
      // So we need to identify these two kinds of record through different phase.
      //
      if(StartProgressId == PERF_CROSSMODULE_START_ID ){
        if (AsciiStrCmp (((FPDT_DYNAMIC_STRING_EVENT_RECORD *)StartRecordEvent)->String, ALit_PEI) == 0) {
          mPeiPhase = TRUE;
        } else if (AsciiStrCmp (((FPDT_DYNAMIC_STRING_EVENT_RECORD *)StartRecordEvent)->String, ALit_DXE) == 0) {
          mDxePhase = TRUE;
          mPeiPhase = FALSE;
        }
      }
      // Get measurement info form the start record to the mMeasurementList.
      GetMeasurementInfo (RecordHeader, TRUE, &(mMeasurementList[mMeasurementNum]));
      mMeasurementNum ++;
    } else {
      ZeroMem(&MeasureMent, sizeof(MEASUREMENT_RECORD));
      GetMeasurementInfo (RecordHeader, FALSE, &MeasureMent);
      SearchMeasurement (&MeasureMent);
    }
    TableLength         += RecordHeader->Length;
    PerformanceTablePtr += RecordHeader->Length;
  }
  return EFI_SUCCESS;
}

/**
  Initialize the cumulative data.

**/
VOID
InitCumulativeData (
  VOID
  )
{
  UINTN                             Index;

  for (Index = 0; Index < NumCum; ++Index) {
    CumData[Index].Count = 0;
    CumData[Index].MinDur = PERF_MAXDUR;
    CumData[Index].MaxDur = 0;
    CumData[Index].Duration = 0;
  }
}

/**
  Initialize the Summary data.

**/
VOID
InitSummaryData (
  VOID
  )
{
  SummaryData.NumTrace      = 0;
  SummaryData.NumIncomplete = 0;
  SummaryData.NumSummary    = 0;
  SummaryData.NumHandles    = 0;
  SummaryData.NumPEIMs      = 0;
  SummaryData.NumGlobal     = 0;
}

/**
  Dump performance data.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval SHELL_SUCCESS            Command completed successfully.
  @retval SHELL_INVALID_PARAMETER  Command usage error.
  @retval SHELL_ABORTED            The user aborts the operation.
  @retval value                    Unknown error.
**/
SHELL_STATUS
RunDp (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  LIST_ENTRY                *ParamPackage;
  CONST CHAR16              *CmdLineArg;
  EFI_STATUS                Status;

  PERFORMANCE_PROPERTY      *PerformanceProperty;
  UINTN                     Number2Display;

  EFI_STRING                StringPtr;
  BOOLEAN                   SummaryMode;
  BOOLEAN                   VerboseMode;
  BOOLEAN                   AllMode;
  BOOLEAN                   RawMode;
  BOOLEAN                   ExcludeMode;
  BOOLEAN                   CumulativeMode;
  CONST CHAR16              *CustomCumulativeToken;
  PERF_CUM_DATA             *CustomCumulativeData;
  UINTN                     NameSize;
  SHELL_STATUS              ShellStatus;
  TIMER_INFO                TimerInfo;
  UINT64                    Intermediate;

  StringPtr   = NULL;
  SummaryMode = FALSE;
  VerboseMode = FALSE;
  AllMode     = FALSE;
  RawMode     = FALSE;
  ExcludeMode = FALSE;
  CumulativeMode = FALSE;
  CustomCumulativeData = NULL;
  ShellStatus = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // Process Command Line arguments
  //
  Status = ShellCommandLineParse (ParamList, &ParamPackage, NULL, TRUE);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_INVALID_ARG), mDpHiiHandle);
    return SHELL_INVALID_PARAMETER;
  } else if (ShellCommandLineGetCount(ParamPackage) > 1){
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_TOO_MANY), mDpHiiHandle);
    return SHELL_INVALID_PARAMETER;
  }

  //
  // Boolean options
  //
  VerboseMode = ShellCommandLineGetFlag (ParamPackage, L"-v");
  SummaryMode = (BOOLEAN) (ShellCommandLineGetFlag (ParamPackage, L"-S") || ShellCommandLineGetFlag (ParamPackage, L"-s"));
  AllMode     = ShellCommandLineGetFlag (ParamPackage, L"-A");
  RawMode     = ShellCommandLineGetFlag (ParamPackage, L"-R");
  ExcludeMode = ShellCommandLineGetFlag (ParamPackage, L"-x");
  mShowId     = ShellCommandLineGetFlag (ParamPackage, L"-i");
  CumulativeMode = ShellCommandLineGetFlag (ParamPackage, L"-c");

  if (AllMode && RawMode) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_CONFLICT_ARG), mDpHiiHandle, L"-A", L"-R");
    return SHELL_INVALID_PARAMETER;
  }

  // Options with Values
  if (ShellCommandLineGetFlag (ParamPackage, L"-n")) {
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-n");
    if (CmdLineArg == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_TOO_FEW), mDpHiiHandle);
      return SHELL_INVALID_PARAMETER;
    } else {
      if (!(RawMode || AllMode)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_NO_RAW_ALL), mDpHiiHandle);
        return SHELL_INVALID_PARAMETER;
      }
      Status = ShellConvertStringToUint64(CmdLineArg, &Intermediate, FALSE, TRUE);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_INVALID_NUM_ARG), mDpHiiHandle, L"-n");
        return SHELL_INVALID_PARAMETER;
      } else {
        Number2Display = (UINTN)Intermediate;
        if (Number2Display == 0 || Number2Display > MAXIMUM_DISPLAYCOUNT) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_INVALID_RANGE), mDpHiiHandle, L"-n", 0, MAXIMUM_DISPLAYCOUNT);
          return SHELL_INVALID_PARAMETER;
        }
      }
    }
  } else {
    Number2Display = DEFAULT_DISPLAYCOUNT;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-t")) {
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-t");
    if (CmdLineArg == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_TOO_FEW), mDpHiiHandle);
      return SHELL_INVALID_PARAMETER;
    } else {
      Status = ShellConvertStringToUint64(CmdLineArg, &Intermediate, FALSE, TRUE);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_INVALID_NUM_ARG), mDpHiiHandle, L"-t");
        return SHELL_INVALID_PARAMETER;
      } else {
        mInterestThreshold = Intermediate;
      }
    }
  } else {
    mInterestThreshold = DEFAULT_THRESHOLD;  // 1ms := 1,000 us
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-c")) {
    CustomCumulativeToken = ShellCommandLineGetValue (ParamPackage, L"-c");
    if (CustomCumulativeToken == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_TOO_FEW), mDpHiiHandle);
      return SHELL_INVALID_PARAMETER;
    } else {
      CustomCumulativeData = AllocateZeroPool (sizeof (PERF_CUM_DATA));
      if (CustomCumulativeData == NULL) {
        ShellStatus = SHELL_OUT_OF_RESOURCES;
        goto Done;
      }
      CustomCumulativeData->MinDur = PERF_MAXDUR;
      CustomCumulativeData->MaxDur = 0;
      CustomCumulativeData->Count  = 0;
      CustomCumulativeData->Duration = 0;
      NameSize = StrLen (CustomCumulativeToken) + 1;
      CustomCumulativeData->Name   = AllocateZeroPool (NameSize);
      if (CustomCumulativeData->Name == NULL) {
        ShellStatus = SHELL_OUT_OF_RESOURCES;
        goto Done;
      }
      UnicodeStrToAsciiStrS (CustomCumulativeToken, CustomCumulativeData->Name, NameSize);
    }
  }

  //
  // DP dump performance data by parsing FPDT table in ACPI table.
  // Folloing 3 steps are to get the measurement form the FPDT table.
  //

  //
  //1. Get FPDT from ACPI table.
  //
  Status = GetBootPerformanceTable ();
  if (EFI_ERROR (Status)) {
    ShellStatus = Status;
    goto Done;
  }

  //
  //2. Cache the ModuleGuid and hanlde mapping table.
  //
  Status = BuildCachedGuidHandleTable();
  if (EFI_ERROR (Status)) {
    ShellStatus = Status;
    goto Done;
  }

  //
  //3. Build the measurement array form the FPDT records.
  //
  Status = BuildMeasurementList ();
  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Initialize the pre-defined cumulative data.
  //
  InitCumulativeData ();

  //
  // Initialize the Summary data.
  //
  InitSummaryData ();

  //
  // Timer specific processing
  //
  // Get the Performance counter characteristics:
  //          Freq = Frequency in Hz
  //    StartCount = Value loaded into the counter when it starts counting
  //      EndCount = Value counter counts to before it needs to be reset
  //
  Status = EfiGetSystemConfigurationTable (&gPerformanceProtocolGuid, (VOID **) &PerformanceProperty);
  if (EFI_ERROR (Status) || (PerformanceProperty == NULL)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PERF_PROPERTY_NOT_FOUND), mDpHiiHandle);
    goto Done;
  }

  TimerInfo.Frequency  = (UINT32)DivU64x32 (PerformanceProperty->Frequency, 1000);
  TimerInfo.StartCount = 0;
  TimerInfo.EndCount   = 0xFFFF;
  TimerInfo.CountUp = TRUE;

  //
  // Print header
  //
  // print DP's build version
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_BUILD_REVISION), mDpHiiHandle, DP_MAJOR_VERSION, DP_MINOR_VERSION);

  // print performance timer characteristics
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_KHZ), mDpHiiHandle, TimerInfo.Frequency);

  if (VerboseMode && !RawMode) {
    StringPtr = HiiGetString (mDpHiiHandle,
                  (EFI_STRING_ID) (TimerInfo.CountUp ? STRING_TOKEN (STR_DP_UP) : STRING_TOKEN (STR_DP_DOWN)), NULL);
    ASSERT (StringPtr != NULL);
    // Print Timer count range and direction
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_TIMER_PROPERTIES), mDpHiiHandle,
                StringPtr,
                TimerInfo.StartCount,
                TimerInfo.EndCount
                );
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_VERBOSE_THRESHOLD), mDpHiiHandle, mInterestThreshold);
  }

/****************************************************************************
****            Print Sections based on command line options
****
****  Option modes have the following priority:
****    v Verbose     --  Valid in combination with any other options
****    t Threshold   --  Modifies All, Raw, and Cooked output
****                      Default is 0 for All and Raw mode
****                      Default is DEFAULT_THRESHOLD for "Cooked" mode
****    n Number2Display  Used by All and Raw mode.  Otherwise ignored.
****    A All         --  R and S options are ignored
****    R Raw         --  S option is ignored
****    s Summary     --  Modifies "Cooked" output only
****    Cooked (Default)
****************************************************************************/
  GatherStatistics (CustomCumulativeData);
  if (CumulativeMode) {
    ProcessCumulative (CustomCumulativeData);
  } else if (AllMode) {
    Status = DumpAllTrace( Number2Display, ExcludeMode);
    if (Status == EFI_ABORTED) {
      ShellStatus = SHELL_ABORTED;
      goto Done;
    }
  } else if (RawMode) {
    Status = DumpRawTrace( Number2Display, ExcludeMode);
    if (Status == EFI_ABORTED) {
      ShellStatus = SHELL_ABORTED;
      goto Done;
    }
  } else {
    //------------- Begin Cooked Mode Processing
    ProcessPhases ();
    if ( ! SummaryMode) {
      Status = ProcessHandles ( ExcludeMode);
      if (Status == EFI_ABORTED) {
        ShellStatus = SHELL_ABORTED;
        goto Done;
      }

      Status = ProcessPeims ();
      if (Status == EFI_ABORTED) {
        ShellStatus = SHELL_ABORTED;
        goto Done;
      }

      Status = ProcessGlobal ();
      if (Status == EFI_ABORTED) {
        ShellStatus = SHELL_ABORTED;
        goto Done;
      }

       ProcessCumulative (NULL);
    }
  } //------------- End of Cooked Mode Processing
  if ( VerboseMode || SummaryMode) {
    DumpStatistics();
  }

Done:
  if (ParamPackage != NULL) {
    ShellCommandLineFreeVarList (ParamPackage);
  }
  SHELL_FREE_NON_NULL (StringPtr);
  if (CustomCumulativeData != NULL) {
    SHELL_FREE_NON_NULL (CustomCumulativeData->Name);
  }
  SHELL_FREE_NON_NULL (CustomCumulativeData);

  SHELL_FREE_NON_NULL (mMeasurementList);

  SHELL_FREE_NON_NULL (mCacheHandleGuidTable);

  mMeasurementNum = 0;
  mCachePairCount = 0;
  return ShellStatus;
}


/**
  Retrive HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HANDLE
InitializeHiiPackage (
  EFI_HANDLE                  ImageHandle
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  EFI_HANDLE                  HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  return HiiHandle;
}
