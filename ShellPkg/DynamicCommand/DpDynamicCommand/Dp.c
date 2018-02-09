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
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
 
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
  PERF_INIT_CUM_DATA (DRIVERBINDING_SUPPORT_TOK)
};

/// Number of items for which we are gathering cumulative statistics.
UINT32 const      NumCum = sizeof(CumData) / sizeof(PERF_CUM_DATA);

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeFlag},   // -v   Verbose Mode
  {L"-A", TypeFlag},   // -A   All, Cooked
  {L"-R", TypeFlag},   // -R   RAW All
  {L"-s", TypeFlag},   // -s   Summary
#if PROFILING_IMPLEMENTED
  {L"-P", TypeFlag},   // -P   Dump Profile Data
  {L"-T", TypeFlag},   // -T   Dump Trace Data
#endif // PROFILING_IMPLEMENTED
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
#if PROFILING_IMPLEMENTED
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_STATS_NUMPROFILE), mDpHiiHandle,    SummaryData.NumProfile);
#endif // PROFILING_IMPLEMENTED
  SHELL_FREE_NON_NULL (StringPtr);
  SHELL_FREE_NON_NULL (StringPtrUnknown);
}

/**
  This function scan ACPI table in RSDT.

  @param  Rsdt        ACPI RSDT
  @param  Signature   ACPI table signature

  @return ACPI table
**/
VOID *
ScanTableInRSDT (
  IN RSDT_TABLE                   *Rsdt,
  IN UINT32                       Signature
  )
{
  UINTN                         Index;
  UINT32                        EntryCount;
  UINT32                        *EntryPtr;
  EFI_ACPI_DESCRIPTION_HEADER   *Table;

  EntryCount = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT32);

  EntryPtr = &Rsdt->Entry;
  for (Index = 0; Index < EntryCount; Index ++, EntryPtr ++) {
    Table = (EFI_ACPI_DESCRIPTION_HEADER*)((UINTN)(*EntryPtr));
    if (Table->Signature == Signature) {
      return Table;
    }
  }

  return NULL;
}

/**
  This function scan ACPI table in XSDT.

  @param  Xsdt       ACPI XSDT
  @param  Signature  ACPI table signature

  @return ACPI table
**/
VOID *
ScanTableInXSDT (
  IN XSDT_TABLE                   *Xsdt,
  IN UINT32                       Signature
  )
{
  UINTN                        Index;
  UINT32                       EntryCount;
  UINT64                       EntryPtr;
  UINTN                        BasePtr;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;

  EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);

  BasePtr = (UINTN)(&(Xsdt->Entry));
  for (Index = 0; Index < EntryCount; Index ++) {
    CopyMem (&EntryPtr, (VOID *)(BasePtr + Index * sizeof(UINT64)), sizeof(UINT64));
    Table = (EFI_ACPI_DESCRIPTION_HEADER*)((UINTN)(EntryPtr));
    if (Table->Signature == Signature) {
      return Table;
    }
  }

  return NULL;
}

/**
  This function scan ACPI table in RSDP.

  @param  Rsdp       ACPI RSDP
  @param  Signature  ACPI table signature

  @return ACPI table
**/
VOID *
FindAcpiPtr (
  IN EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp,
  IN UINT32                                       Signature
  )
{
  EFI_ACPI_DESCRIPTION_HEADER                    *AcpiTable;
  RSDT_TABLE                                     *Rsdt;
  XSDT_TABLE                                     *Xsdt;

  AcpiTable = NULL;

  //
  // Check ACPI2.0 table
  //
  Rsdt = (RSDT_TABLE *)(UINTN)Rsdp->RsdtAddress;
  Xsdt = NULL;
  if ((Rsdp->Revision >= 2) && (Rsdp->XsdtAddress < (UINT64)(UINTN)-1)) {
    Xsdt = (XSDT_TABLE *)(UINTN)Rsdp->XsdtAddress;
  }
  //
  // Check Xsdt
  //
  if (Xsdt != NULL) {
    AcpiTable = ScanTableInXSDT (Xsdt, Signature);
  }
  //
  // Check Rsdt
  //
  if ((AcpiTable == NULL) && (Rsdt != NULL)) {
    AcpiTable = ScanTableInRSDT (Rsdt, Signature);
  }

  return AcpiTable;
}

/**
  Get Boot performance table form Acpi table.

**/
EFI_STATUS
GetBootPerformanceTable (
  )
{
  EFI_STATUS                  Status;
  VOID                        *AcpiTable;
  FIRMWARE_PERFORMANCE_TABLE  *FirmwarePerformanceTable;

  AcpiTable = NULL;

  Status = EfiGetSystemConfigurationTable (
             &gEfiAcpi20TableGuid,
             &AcpiTable
             );
  if (EFI_ERROR (Status)) {
    Status = EfiGetSystemConfigurationTable (
               &gEfiAcpi10TableGuid,
               &AcpiTable
                 );
  }
  if (EFI_ERROR(Status) || AcpiTable == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_GET_ACPI_TABLE_FAIL), mDpHiiHandle);
    return Status;
  }

  FirmwarePerformanceTable = FindAcpiPtr (
                      (EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiTable,
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
  return Status;
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

    if (AsciiStrCmp (Measurement->Token, ALit_PEIM) == 0) {
      Measurement->Handle         = &(((FPDT_GUID_EVENT_RECORD *)RecordHeader)->Guid);
    } else {
      GetHandleFormModuleGuid(ModuleGuid, &StartHandle);
      Measurement->Handle         = StartHandle;
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

    if (AsciiStrCmp (Measurement->Token, ALit_PEIM) == 0) {
      Measurement->Handle         = &(((FPDT_DYNAMIC_STRING_EVENT_RECORD *)RecordHeader)->Guid);
    } else {
      GetHandleFormModuleGuid(ModuleGuid, &StartHandle);
      Measurement->Handle = StartHandle;
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
    // If the record is the start record, fill the info to the measurement in the mMeasurementList.
    // If the record is the end record, find the related start measurement in the mMeasurementList and fill the EndTimeStamp.
    //
    if (((StartProgressId >= PERF_EVENTSIGNAL_START_ID && ((StartProgressId & 0x000F) == 0)) ||
        (StartProgressId < PERF_EVENTSIGNAL_START_ID && ((StartProgressId & 0x0001) != 0)))) {
      //
      // Since PEIM and StartImage has same Type and ID when PCD PcdEdkiiFpdtStringRecordEnableOnly = FALSE
      // So we need to identify these two kinds of record through different phase.
      //
      if (AsciiStrCmp (((FPDT_DYNAMIC_STRING_EVENT_RECORD *)StartRecordEvent)->String, ALit_PEI) == 0) {
        mPeiPhase = TRUE;
      } else if (AsciiStrCmp (((FPDT_DYNAMIC_STRING_EVENT_RECORD *)StartRecordEvent)->String, ALit_DXE) == 0) {
        mDxePhase = TRUE;
        mPeiPhase = FALSE;
      }
      // Get measurement info form the start record to the mMeasurementList.
      GetMeasurementInfo (RecordHeader, TRUE, &(mMeasurementList[mMeasurementNum]));
      mMeasurementNum ++;
    } else {
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
  BOOLEAN                   TraceMode;
  BOOLEAN                   ProfileMode;
  BOOLEAN                   ExcludeMode;
  BOOLEAN                   CumulativeMode;
  CONST CHAR16              *CustomCumulativeToken;
  PERF_CUM_DATA             *CustomCumulativeData;
  UINTN                     NameSize;
  SHELL_STATUS              ShellStatus;
  TIMER_INFO                TimerInfo;

  StringPtr   = NULL;
  SummaryMode = FALSE;
  VerboseMode = FALSE;
  AllMode     = FALSE;
  RawMode     = FALSE;
  TraceMode   = FALSE;
  ProfileMode = FALSE;
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
  // DP dump performance data by parsing FPDT table in ACPI table.
  // Folloing 3 steps are to get the measurement form the FPDT table.
  //

  //
  //1. Get FPDT from ACPI table.
  //
  Status = GetBootPerformanceTable ();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  //2. Cache the ModuleGuid and hanlde mapping table.
  //
  Status = BuildCachedGuidHandleTable();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  //3. Build the measurement array form the FPDT records.
  //
  Status = BuildMeasurementList ();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Process Command Line arguments
  //
  Status = ShellCommandLineParse (ParamList, &ParamPackage, NULL, TRUE);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DP_INVALID_ARG), mDpHiiHandle);
    return SHELL_INVALID_PARAMETER;
  }

  //
  // Boolean options
  //
  VerboseMode = ShellCommandLineGetFlag (ParamPackage, L"-v");
  SummaryMode = (BOOLEAN) (ShellCommandLineGetFlag (ParamPackage, L"-S") || ShellCommandLineGetFlag (ParamPackage, L"-s"));
  AllMode     = ShellCommandLineGetFlag (ParamPackage, L"-A");
  RawMode     = ShellCommandLineGetFlag (ParamPackage, L"-R");
#if PROFILING_IMPLEMENTED
  TraceMode   = ShellCommandLineGetFlag (ParamPackage, L"-T");
  ProfileMode = ShellCommandLineGetFlag (ParamPackage, L"-P");
#endif  // PROFILING_IMPLEMENTED
  ExcludeMode = ShellCommandLineGetFlag (ParamPackage, L"-x");
  mShowId     = ShellCommandLineGetFlag (ParamPackage, L"-i");
  CumulativeMode = ShellCommandLineGetFlag (ParamPackage, L"-c");

  // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-n");
  if (CmdLineArg == NULL) {
    Number2Display = DEFAULT_DISPLAYCOUNT;
  } else {
    Number2Display = StrDecimalToUintn(CmdLineArg);
    if (Number2Display == 0) {
      Number2Display = MAXIMUM_DISPLAYCOUNT;
    }
  }

  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-t");
  if (CmdLineArg == NULL) {
    mInterestThreshold = DEFAULT_THRESHOLD;  // 1ms := 1,000 us
  } else {
    mInterestThreshold = StrDecimalToUint64(CmdLineArg);
  }

  // Handle Flag combinations and default behaviors
  // If both TraceMode and ProfileMode are FALSE, set them both to TRUE
  if ((! TraceMode) && (! ProfileMode)) {
    TraceMode   = TRUE;
#if PROFILING_IMPLEMENTED
    ProfileMode = TRUE;
#endif  // PROFILING_IMPLEMENTED
  }

  //
  // Initialize the pre-defined cumulative data.
  //
  InitCumulativeData ();

  //
  // Init the custom cumulative data.
  //
  CustomCumulativeToken = ShellCommandLineGetValue (ParamPackage, L"-c");
  if (CustomCumulativeToken != NULL) {
    CustomCumulativeData = AllocateZeroPool (sizeof (PERF_CUM_DATA));
    if (CustomCumulativeData == NULL) {
      return SHELL_OUT_OF_RESOURCES;
    }
    CustomCumulativeData->MinDur = PERF_MAXDUR;
    CustomCumulativeData->MaxDur = 0;
    CustomCumulativeData->Count  = 0;
    CustomCumulativeData->Duration = 0;
    NameSize = StrLen (CustomCumulativeToken) + 1;
    CustomCumulativeData->Name   = AllocateZeroPool (NameSize);
    if (CustomCumulativeData->Name == NULL) {
      FreePool (CustomCumulativeData);
      return SHELL_OUT_OF_RESOURCES;
    }
    UnicodeStrToAsciiStrS (CustomCumulativeToken, CustomCumulativeData->Name, NameSize);
  }

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
****
****  The All, Raw, and Cooked modes are modified by the Trace and Profile
****  options.
****    !T && !P  := (0) Default, Both are displayed
****     T && !P  := (1) Only Trace records are displayed
****    !T &&  P  := (2) Only Profile records are displayed
****     T &&  P  := (3) Same as Default, both are displayed
****************************************************************************/
  GatherStatistics (CustomCumulativeData);
  if (CumulativeMode) {                       
    ProcessCumulative (CustomCumulativeData);
  } else if (AllMode) {
    if (TraceMode) {
      Status = DumpAllTrace( Number2Display, ExcludeMode);
      if (Status == EFI_ABORTED) {
        ShellStatus = SHELL_ABORTED;
        goto Done;
      }
    }
    if (ProfileMode) {
      DumpAllProfile( Number2Display, ExcludeMode);
    }
  } else if (RawMode) {
    if (TraceMode) {
      Status = DumpRawTrace( Number2Display, ExcludeMode);
      if (Status == EFI_ABORTED) {
        ShellStatus = SHELL_ABORTED;
        goto Done;
      }
    }
    if (ProfileMode) {
      DumpRawProfile( Number2Display, ExcludeMode);
    }
  } else {
    //------------- Begin Cooked Mode Processing
    if (TraceMode) {
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
    }
    if (ProfileMode) {
      DumpAllProfile( Number2Display, ExcludeMode);
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
