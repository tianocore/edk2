/** @file
  Main file for Cxl shell Debug1 function.

  Copyright 2026 Google LLC<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/CxlIo.h>
#include <Protocol/PciIo.h>
#include <Library/ShellLib.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>

#pragma pack(1)
typedef struct {
  EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE_HEADER    DoeHeader;
  CXL_CDAT_STRUCTURE                                     Cdat;
} CXL_LIB_DOE_CDAT_HEADER_RESP;
#pragma pack()

//
// Global Variables
//
STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-s", TypeValue },
  { NULL,  TypeMax   }
};

EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_REQUEST  mCdatRequest = {
  .Header           = {
    .VendorId       = CXL_DVSEC_VENDOR_ID,
    .DataObjectType = EfiCxlDoeTableAccess,
    .Length         = sizeof (EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_REQUEST) / sizeof (UINT32)
  },
  .ReqCode     = 0,
  .TableType   = 0,
  .EntryHandle = 0,
};

/**
  Prints all CDAT entris for the provided device.

  @param[in] Subtable       Pointer to the subtable's data.

**/
VOID
PrintCdatSubtable (
  IN CXL_CDAT_COMMON_SUBTABLE_HEADER  *Subtable
  )
{
  CXL_CDAT_DEVICE_SCOPED_MEMORY_AFFINITY_STRUCTURE         *Dsmas;
  CXL_CDAT_DEVICE_SCOPED_LAT_BW_STRUCTURE                  *Dslbis;
  CXL_CDAT_DEVICE_SCOPED_MEMORY_SIDE_CACHE_INFO_STRUCTURE  *Dsmscis;
  CXL_CDAT_DEVICE_SCOPED_INITIATOR_STRUCTURE               *Dsis;
  CXL_CDAT_DEVICE_SCOPED_EFI_MEMORY_TYPE_STRUCTURE         *Dsemts;

  switch (Subtable->Type) {
    case CxlCdatTypeDsmas:
      Dsmas = (CXL_CDAT_DEVICE_SCOPED_MEMORY_AFFINITY_STRUCTURE *)Subtable;
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_CDAT_DSMAS),
        gShellDebug1HiiHandle,
        Subtable->Type,
        Dsmas->DsmadHandle,
        Dsmas->Flags,
        Dsmas->DpaBase,
        Dsmas->DpaLength
        );
      break;
    case CxlCdatTypeDslbis:
      Dslbis = (CXL_CDAT_DEVICE_SCOPED_LAT_BW_STRUCTURE *)Subtable;
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_CDAT_DSLBIS),
        gShellDebug1HiiHandle,
        Subtable->Type,
        Dslbis->Handle,
        Dslbis->Flags,
        Dslbis->DataType,
        Dslbis->EntryBaseUnit,
        Dslbis->Entry[0],
        Dslbis->Entry[1],
        Dslbis->Entry[2]
        );
      break;
    case CxlCdatTypeDsmscis:
      Dsmscis = (CXL_CDAT_DEVICE_SCOPED_MEMORY_SIDE_CACHE_INFO_STRUCTURE *)Subtable;
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_CDAT_DSMSCIS),
        gShellDebug1HiiHandle,
        Dsmscis->Header.Type
        );
      break;
    case CxlCdatTypeDsis:
      Dsis = (CXL_CDAT_DEVICE_SCOPED_INITIATOR_STRUCTURE *)Subtable;
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_CDAT_DSIS),
        gShellDebug1HiiHandle,
        Dsis->Header.Type
        );
      break;
    case CxlCdatTypeDsemts:
      Dsemts = (CXL_CDAT_DEVICE_SCOPED_EFI_MEMORY_TYPE_STRUCTURE *)Subtable;
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_CDAT_DSEMTS),
        gShellDebug1HiiHandle,
        Dsemts->Header.Type,
        Dsemts->DsmasHandle
        );
      break;
    case CxlCdatTypeSslbis:
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_CDAT_SSLBIS),
        gShellDebug1HiiHandle,
        Subtable->Type
        );
      break;
    default:
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_CDAT_UNKNOWN),
        gShellDebug1HiiHandle,
        Subtable->Type
        );
      break;
  }
}

/**
  Prints all CDAT entris for the provided device.

  @param[in] CxlIo        Pointer to the device's CxlIo protocol.

**/
VOID
PrintCdatInfo (
  IN EDKII_CXL_IO_PROTOCOL  *CxlIo
  )
{
  EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_REQUEST   CdatRequest;
  EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE  *Response;
  CXL_LIB_DOE_CDAT_HEADER_RESP                  DoeCdatHeader;
  CXL_CDAT_COMMON_SUBTABLE_HEADER               *SubtableHeader;
  UINTN                                         BufferSize;
  VOID                                          *Buffer;
  EFI_STATUS                                    Status;

  Buffer = NULL;

  ZeroMem (&DoeCdatHeader, sizeof (DoeCdatHeader));
  BufferSize  = sizeof (DoeCdatHeader);
  CdatRequest = mCdatRequest;
  Status      = CxlIo->DoeTransact (
                         CxlIo,
                         &CdatRequest,
                         sizeof (CdatRequest),
                         (VOID *)&DoeCdatHeader,
                         &BufferSize
                         );

  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (
      STRING_TOKEN (STR_CXL_DOE_ERROR),
      gShellDebug1HiiHandle,
      Status
      );
    return;
  }

  Status = gBS->AllocatePool (EfiBootServicesData, SIZE_1KB, &Buffer);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (
      STRING_TOKEN (STR_GEN_NO_MEM),
      gShellDebug1HiiHandle,
      L"cxl"
      );
    return;
  }

  CdatRequest.EntryHandle = DoeCdatHeader.DoeHeader.EntryHandle;
  do {
    BufferSize = SIZE_1KB;
    Status     = CxlIo->DoeTransact (
                          CxlIo,
                          &CdatRequest,
                          sizeof (CdatRequest),
                          (VOID *)Buffer,
                          &BufferSize
                          );
    if (EFI_ERROR (Status)) {
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_CXL_DOE_ERROR),
        gShellDebug1HiiHandle,
        Status
        );
      goto OnExit;
    }

    Response = (EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE *)Buffer;

    SubtableHeader = (CXL_CDAT_COMMON_SUBTABLE_HEADER *)Response->Data;
    PrintCdatSubtable (SubtableHeader);

    CdatRequest.EntryHandle = Response->Header.EntryHandle;
  } while (CdatRequest.EntryHandle != 0xFFFF);

OnExit:
  if (Buffer) {
    gBS->FreePool (Buffer);
  }
}

/**
  Discovers all CXL devices and gets their handles.

  NOTE: The caller must free the handles when done using them.

  @param[out] Handles                Pointer a handle buffer.
  @param[out] NumHandles             Number of handles discoverd

  @retval     EFI_SUCCESS             Success.
**/
EFI_STATUS
CxlFindEndpoints (
  EFI_HANDLE  **Handles,
  UINTN       *NumHandles
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *PciHandles;
  UINTN       NumPciHandles;
  UINTN       Idx;

  /* Probe each PCI device to attempt to bind the CXL driver. */
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL /* SearchKey */,
                  &NumPciHandles,
                  &PciHandles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Idx = 0; Idx < NumPciHandles; Idx++) {
    gBS->ConnectController (
           PciHandles[Idx],
           NULL,
           NULL,
           FALSE
           );
  }

  gBS->FreePool (PciHandles);

  /* Now, get the handles of all detected CXL devices. */
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEdkiiCxlIoProtocolGuid,
                  NULL /* SearchKey */,
                  NumHandles,
                  Handles
                  );
  return Status;
}

/**
  Function for 'cxl' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunCxl (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                          Segment;
  UINTN                          Bus;
  UINTN                          Device;
  UINTN                          Func;
  EFI_STATUS                     Status;
  UINTN                          Index;
  EFI_HANDLE                     *HandleBuf;
  UINTN                          HandleCount;
  LIST_ENTRY                     *Package;
  CHAR16                         *ProblemParam;
  SHELL_STATUS                   ShellStatus;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EDKII_CXL_IO_PROTOCOL          *CxlIo;
  PCI_DEVICE_INDEPENDENT_REGION  PciHeader;
  CONST CHAR16                   *Temp;
  UINT64                         RetVal;
  UINT16                         TargetSegment;
  UINT16                         TargetBus;
  UINT16                         TargetDevice;
  UINT16                         TargetFunc;

  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;
  HandleBuf   = NULL;
  Package     = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"cxl", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // Argument Count == 1(no other argument): enumerate all CXL functions
    //
    if (ShellCommandLineGetCount (Package) == 1) {
      Status = CxlFindEndpoints (&HandleBuf, &HandleCount);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol (HandleBuf[Index], &gEdkiiCxlIoProtocolGuid, (VOID **)&CxlIo);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_PCI_HANDLE_CFG_ERR), gShellDebug1HiiHandle, L"cxl");
          ShellStatus = SHELL_NOT_FOUND;
          goto Done;
        }

        PciIo  = CxlIo->PciIo;
        Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Func);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Status = PciIo->Pci.Read (
                              PciIo,
                              EfiPciIoWidthFifoUint32,
                              0,
                              sizeof (PciHeader) / sizeof (UINT32),
                              &PciHeader
                              );
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        ShellPrintHiiDefaultEx (
          STRING_TOKEN (STR_CXL_LINE_P1),
          gShellDebug1HiiHandle,
          Segment,
          Bus,
          Device,
          Func
          );

        ShellPrintHiiDefaultEx (
          STRING_TOKEN (STR_CXL_LINE_P2),
          gShellDebug1HiiHandle,
          PciHeader.VendorId,
          PciHeader.DeviceId
          );
      }

      Status = EFI_SUCCESS;
      goto Done;
    } else {
      // Dump extended information
      TargetSegment = 0;
      TargetBus     = 0;
      TargetDevice  = 0;
      TargetFunc    = 0;
      if ((ShellCommandLineGetCount (Package) < 4) || (ShellCommandLineGetCount (Package) == 5)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"cxl");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      if (ShellCommandLineGetCount (Package) > 6) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"cxl");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      if (ShellCommandLineGetFlag (Package, L"-s") && (ShellCommandLineGetValue (Package, L"-s") == NULL)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"cxl", L"-s");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      Temp = ShellCommandLineGetValue (Package, L"-s");
      if (Temp != NULL) {
        //
        // Input converted to hexadecimal number.
        //
        if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
          TargetSegment = (UINT16)RetVal;
        } else {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"cxl", Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      }

      //
      // The first Argument is assumed to be Bus number, second
      // to be Device number, and third to be Func number.
      //
      Temp = ShellCommandLineGetRawValue (Package, 1);
      if (Temp != NULL) {
        //
        // Input converted to hexadecimal number.
        //
        if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
          TargetBus = (UINT16)RetVal;
        } else {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"cxl", Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }

        if (TargetBus > PCI_MAX_BUS) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"cxl", Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      }

      Temp = ShellCommandLineGetRawValue (Package, 2);
      if (Temp != NULL) {
        //
        // Input converted to hexadecimal number.
        //
        if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
          TargetDevice = (UINT16)RetVal;
        } else {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"cxl", Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }

        if (TargetDevice > PCI_MAX_DEVICE) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"cxl", Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      }

      Temp = ShellCommandLineGetRawValue (Package, 3);
      if (Temp != NULL) {
        //
        // Input converted to hexadecimal number.
        //
        if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
          TargetFunc = (UINT16)RetVal;
        } else {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"cxl", Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }

        if (TargetFunc > PCI_MAX_FUNC) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"cxl", Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      }

      Status = CxlFindEndpoints (&HandleBuf, &HandleCount);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol (HandleBuf[Index], &gEdkiiCxlIoProtocolGuid, (VOID **)&CxlIo);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_PCI_HANDLE_CFG_ERR), gShellDebug1HiiHandle, L"cxl");
          ShellStatus = SHELL_NOT_FOUND;
          goto Done;
        }

        PciIo  = CxlIo->PciIo;
        Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Func);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if ((Segment != TargetSegment) ||
            (Bus != TargetBus) ||
            (Device != TargetDevice) ||
            (Func != TargetFunc))
        {
          continue;
        }

        PrintCdatInfo (CxlIo);
        goto Done;
      }
    }
  }

Done:
  if (HandleBuf != NULL) {
    FreePool (HandleBuf);
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }

  return ShellStatus;
}
