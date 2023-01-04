/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include "MipiSyst.h"
#include "MipiSystApi.h"

/**
 * This array record the operation code for a member in MIPI_SYST_MSGDSC structure.
 * and how many times the data stored in that member needs to be processed.
**/
CONST MIPI_SYST_SCATTER_PROG  mScatterOps[] = {
  {
    MipiSystScatterOp64bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdGuid),
    2
  }
  ,
  {
    MipiSystScatterOp16bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdLen),
    1
  }
  ,
  {
    MipiSystScatterOpBlob,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdPld.DataVar),
    0
  }
  ,
  {
    MipiSystScatterOp32bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdChk),
    1
  }
  ,
  {
    MipiSystScatterOp32bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdPld.CatId.Id.CatId32),
    1
  }
  ,
  {
    MipiSystScatterOp64bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdPld.CatId.Id.CatId64),
    1
  }
  ,
  {
    MipiSystScatterOpBlob,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdPld.CatId.Param),
    0
  }
  ,
  {
    MipiSystScatterOp64bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdPld.DataClock),
    2
  }
  ,
  {
    MipiSystScatterOp64bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdTs),
    1
  }
  ,
  {
    MipiSystScatterOp64bit,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdPld.Version.Id),
    1
  }
  ,
  {
    MipiSystScatterOpBlob,
    OFFSET_OF (MIPI_SYST_MSGDSC, EdPld.Version.Text),
    0
  }
  ,
  {
    MipiSystScatterOpEnd,
    0,
    0
  }
};

/**
  Initialize MIPI_SYST_HANDLE structure.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.

  @retval MIPI_SYST_HANDLE    A initialized pointer to MIPI_SYST_HANDLE structure.
**/
MIPI_SYST_HANDLE *
EFIAPI
InitMipiSystHandle (
  IN MIPI_SYST_HANDLE  *SystHandle
  )
{
  ZeroMem (SystHandle, sizeof (MIPI_SYST_HANDLE));
  SystHandle->SysthVersion = MIPI_SYST_VERSION_CODE;

  return SystHandle;
}

/**
  Insert optional item into MIPI_SYST_SCATTER_PROG array.

  @param[in]  SystHandle         A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Len                Length of MIPI_SYST_MSGDSC buffer.
  @param[in]  Desc               A pointer to MIPI_SYST_MSGDSC buffer.
  @param[in, out]  Progptr       A pointer to a pointer to MIPI_SYST_SCATTER_PROG buffer.
**/
VOID
EFIAPI
InsertOptionalMsgComponents (
  IN      MIPI_SYST_HANDLE        *SystHandle,
  IN      UINT16                  Len,
  IN      MIPI_SYST_MSGDSC        *Desc,
  IN OUT  MIPI_SYST_SCATTER_PROG  **Progptr
  )
{
  MIPI_SYST_SCATTER_PROG  *Prog;

  Prog = *Progptr;

  if (Desc->EdTag.EtGuid) {
    Desc->EdGuid = SystHandle->SystHandleGuid;
    *Prog++      = mScatterOps[ScatterOpGuid];
  }

  if (Desc->EdTag.EtLength) {
    Desc->EdLen = Len;
    *Prog++     = mScatterOps[ScatterOpLength];
  }

  if (Desc->EdTag.EtTimestamp) {
    Desc->EdTs = MipiSystGetEpochUs ();
    *Prog++    = mScatterOps[ScatterOpTs];
  }

  *Progptr = Prog;
}

/**
  To set operation code and descriptor for debug string print.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Type            String type.
  @param[in]  Severity        An error level to decide whether to enable Trace Hub data.
  @param[in]  Len             Length of data buffer.
  @param[in]  Str             A pointer to data buffer.
**/
VOID
MipiSystWriteDebugString (
  IN        MIPI_SYST_HANDLE          *SystHandle,
  IN        MIPI_SYST_SUBTYPE_STRING  Type,
  IN        MIPI_SYST_SEVERITY        Severity,
  IN        UINT16                    Len,
  IN CONST  CHAR8                     *Str
  )
{
  MIPI_SYST_MSGDSC        Desc;
  MIPI_SYST_SCATTER_PROG  Prog[MIPI_SYST_SCATTER_PROG_LEN];
  MIPI_SYST_SCATTER_PROG  *ProgPtr;
  UINT64                  ErrMsg;

  if (SystHandle == NULL) {
    return;
  }

  ProgPtr = Prog;

  ZeroMem (&Desc, sizeof (MIPI_SYST_MSGDSC));
  Desc.EdTag            = SystHandle->SystHandleTag;
  Desc.EdTag.EtType     = MipiSystTypeString;
  Desc.EdTag.EtSubtype  = Type;
  Desc.EdTag.EtSeverity = Severity;

  if (Str == NULL) {
    Desc.EdTag.EtSubtype = MipiSystStringInvalidParam;
    ErrMsg               = 0x0000296c6c756e28ull; /* == "(null)\0\0" */
    Str                  = (CHAR8 *)&ErrMsg;
    Len                  = 7;
  }

  InsertOptionalMsgComponents (SystHandle, Len, &Desc, &ProgPtr);

  Desc.EdPld.DataVar = (CONST CHAR8 *)Str;
  *ProgPtr           = mScatterOps[ScatterOpPayldVar];
  ProgPtr->SsoLength = Len;
  ++ProgPtr;

  *ProgPtr = mScatterOps[ScatterOpEnd];

  ASSERT (ProgPtr < &Prog[MIPI_SYST_SCATTER_PROG_LEN]);

  MipiSystScatterWrite (SystHandle, Prog, &Desc);
}

/**
  To set operation code and descriptor for catalog or status code debug print.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Severity        An error level to decide whether to enable Trace Hub data.
  @param[in]  CatId           Cat id.
**/
VOID
EFIAPI
MipiSystWriteCatalogMessage (
  IN MIPI_SYST_HANDLE    *SystHandle,
  IN MIPI_SYST_SEVERITY  Severity,
  IN UINT64              CatId
  )
{
  MIPI_SYST_MSGDSC        Desc;
  MIPI_SYST_SCATTER_PROG  Prog[MIPI_SYST_SCATTER_PROG_LEN];
  MIPI_SYST_SCATTER_PROG  *ProgPtr;
  UINT16                  ParamLen;

  if (SystHandle == NULL) {
    return;
  }

  ProgPtr = Prog;

  ZeroMem (&Desc, sizeof (MIPI_SYST_MSGDSC));
  Desc.EdTag            = SystHandle->SystHandleTag;
  Desc.EdTag.EtType     = MipiSystTypeCatalog;
  Desc.EdTag.EtSubtype  = MipiSystCatalogId64P32;
  Desc.EdTag.EtSeverity = Severity;

  ParamLen = (UINT16)(SystHandle->SystHandleParamCount * sizeof (UINT32));

  InsertOptionalMsgComponents (SystHandle, sizeof (CatId) + ParamLen, &Desc, &ProgPtr);

  Desc.EdPld.CatId.Id.CatId64 = CatId;
  *ProgPtr++                  = mScatterOps[ScatterOpCatId64];

  if (ParamLen) {
    Desc.EdPld.CatId.Param = SystHandle->SystHandleParam;
    *ProgPtr               = mScatterOps[ScatterOpCatIdArgs];
    ProgPtr->SsoLength     = ParamLen;
    ++ProgPtr;
  }

  *ProgPtr = mScatterOps[ScatterOpEnd];

  ASSERT (ProgPtr < &Prog[MIPI_SYST_SCATTER_PROG_LEN]);

  MipiSystScatterWrite (SystHandle, Prog, &Desc);
}

/**
  Write data to specified MMIO address according to MIPI_SYST_MSGDSC and MIPI_SYST_SCATTER_PROG.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  ScatterProg     A pointer to MIPI_SYST_SCATTER_PROG array.
  @param[in]  Pdesc           A pointer to MIPI_SYST_MSGDSC structure.
**/
VOID
EFIAPI
MipiSystScatterWrite (
  IN MIPI_SYST_HANDLE        *SystHandle,
  IN MIPI_SYST_SCATTER_PROG  *ScatterProg,
  IN CONST VOID              *Pdesc
  )
{
  UINT16      Repeat;
  CONST VOID  *DataPtr;

  MipiSystWriteD32Ts (SystHandle, *(UINT32 *)Pdesc);

  while (ScatterProg->SsoOpcode != MipiSystScatterOpEnd) {
    //
    // Times to process a data.
    //
    Repeat = ScatterProg->SsoLength;

    DataPtr = Pdesc;

    //
    // A pointer to arbitrary member of MIPI_SYST_MSGDSC structure.
    //
    DataPtr = (UINT8 *)DataPtr + ScatterProg->SsoOffset;

    switch (ScatterProg->SsoOpcode) {
      case MipiSystScatterOp8bit:
        do {
          MipiSystWriteD8 (SystHandle, *(UINT8 *)DataPtr);
          DataPtr = (UINT8 *)DataPtr + 1;
        } while (--Repeat);

        break;
      case MipiSystScatterOp16bit:
        do {
          MipiSystWriteD16 (SystHandle, *(UINT16 *)DataPtr);
          DataPtr = (UINT16 *)DataPtr + 1;
        } while (--Repeat);

        break;
      case MipiSystScatterOp32bit:
        do {
          MipiSystWriteD32 (SystHandle, *(UINT32 *)DataPtr);
          DataPtr = (UINT32 *)DataPtr + 1;
        } while (--Repeat);

        break;
      case MipiSystScatterOp64bit:
        do {
          if (sizeof (UINTN) == 8) {
            MipiSystWriteD64 (SystHandle, *(UINT64 *)DataPtr);
            DataPtr = (UINT64 *)DataPtr + 1;
          } else if (sizeof (UINTN) == 4) {
            MipiSystWriteD32 (SystHandle, *(UINT32 *)DataPtr);
            DataPtr = (UINT32 *)DataPtr + 1;
            MipiSystWriteD32 (SystHandle, *(UINT32 *)DataPtr);
            DataPtr = (UINT32 *)DataPtr + 1;
          }
        } while (--Repeat);

        break;
      case MipiSystScatterOpBlob:
        DataPtr = *(VOID **)DataPtr;

        //
        // 64 bits memory addressing
        //
        if (sizeof (UINTN) == 8) {
          //
          // Process 8 bytes in data buffer at a time until remaining bytes < 8.
          //
          while (Repeat >= sizeof (UINT64)) {
            MipiSystWriteD64 (SystHandle, *(UINT64 *)DataPtr);
            DataPtr = (UINT64 *)DataPtr + 1;
            Repeat -= sizeof (UINT64);
          }

          //
          // Process 4 bytes in data buffer at a time until remaining bytes < 4
          //
          if (Repeat >= sizeof (UINT32)) {
            MipiSystWriteD32 (SystHandle, *(UINT32 *)DataPtr);
            DataPtr = (UINT32 *)DataPtr + 1;
            Repeat -= sizeof (UINT32);
          }
        } else if (sizeof (UINTN) == 4) {
          //
          // 32 bits memory addressing
          //

          //
          // Process 4 bytes in data buffer at a time until remaining bytes < 4
          //
          while (Repeat >= sizeof (UINT32)) {
            MipiSystWriteD32 (SystHandle, *(UINT32 *)DataPtr);
            DataPtr = (UINT32 *)DataPtr + 1;
            Repeat -= sizeof (UINT32);
          }
        }

        //
        // Process 2 bytes in data buffer at a time until remaining bytes < 2
        //
        if (Repeat >= sizeof (UINT16)) {
          MipiSystWriteD16 (SystHandle, *(UINT16 *)DataPtr);
          DataPtr = (UINT16 *)DataPtr + 1;
          Repeat -= sizeof (UINT16);
        }

        //
        // Process the last 1 byte.
        //
        if (Repeat) {
          MipiSystWriteD8 (SystHandle, *(UINT8 *)DataPtr);
        }
    }

    //
    // Pointer to next record.
    //
    ++ScatterProg;
  }

  MipiSystWriteFlag (SystHandle);
}

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x10.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD32Ts (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT32            Data
  )
{
  *((UINT32 *)(SystHandle->ThDebugMmioAddress + 0x10)) = Data;
}

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x18.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD32Mts (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT32            Data
  )
{
  *((UINT32 *)(SystHandle->ThDebugMmioAddress + 0x18)) = Data;
}

/**
  Write 8 bytes to Trace Hub MMIO addr + 0x18.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD64Mts (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT64            Data
  )
{
  *((UINT64 *)(SystHandle->ThDebugMmioAddress + 0x18)) = Data;
}

/**
  Write 1 byte to Trace Hub MMIO addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD8 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT8             Data
  )
{
  *((UINT8 *)(SystHandle->ThDebugMmioAddress + 0x0)) = Data;
}

/**
  Write 2 bytes to Trace Hub MMIO mmio addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD16 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT16            Data
  )
{
  *((UINT16 *)(SystHandle->ThDebugMmioAddress + 0x0)) = Data;
}

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD32 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT32            Data
  )
{
  *((UINT32 *)(SystHandle->ThDebugMmioAddress + 0x0)) = Data;
}

/**
  Write 8 bytes to Trace Hub MMIO addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD64 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT64            Data
  )
{
  *((UINT64 *)(SystHandle->ThDebugMmioAddress + 0x0)) = Data;
}

/**
  Clear data in Trace Hub MMIO addr + 0x30.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
**/
VOID
EFIAPI
MipiSystWriteFlag (
  IN  MIPI_SYST_HANDLE  *SystHandle
  )
{
  UINT32  *Flag;

  Flag  = (UINT32 *)(SystHandle->ThDebugMmioAddress + 0x30);
  *Flag = 0;
}

/**
  Get Epoch time.

  @retval UINT64    A numeric number for timestamp.
**/
UINT64
EFIAPI
MipiSystGetEpochUs (
  VOID
  )
{
  UINT64  Epoch;

  Epoch = 1000;

  return Epoch;
}
