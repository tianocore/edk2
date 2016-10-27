/** @file
  This library will parse the coreboot table in memory and extract those required
  information.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Guid/FrameBufferInfoGuid.h>

typedef RETURN_STATUS \
        (*CB_MEM_INFO_CALLBACK) (UINT64 Base, UINT64 Size, UINT32 Type, VOID *Param);

/**
  Find coreboot record with given Tag from the memory Start in 4096
  bytes range.

  @param  Start              The start memory to be searched in
  @param  Tag                The tag id to be found

  @retval NULL              The Tag is not found.
  @retval Others            The poiter to the record found.

**/
VOID *
EFIAPI
FindCbTag (
  IN  VOID     *Start,
  IN  UINT32   Tag
  );

/**
  Acquire the memory information from the coreboot table in memory.

  @param  MemInfoCallback     The callback routine
  @param  pParam              Pointer to the callback routine parameter

  @retval RETURN_SUCCESS     Successfully find out the memory information.
  @retval RETURN_NOT_FOUND   Failed to find the memory information.

**/
RETURN_STATUS
EFIAPI
CbParseMemoryInfo (
  IN  CB_MEM_INFO_CALLBACK  MemInfoCallback,
  IN  VOID                  *pParam
  );

/**
  Acquire the coreboot memory table with the given table id

  @param  TableId            Table id to be searched
  @param  pMemTable          Pointer to the base address of the memory table
  @param  pMemTableSize      Pointer to the size of the memory table

  @retval RETURN_SUCCESS     Successfully find out the memory table.
  @retval RETURN_INVALID_PARAMETER  Invalid input parameters.
  @retval RETURN_NOT_FOUND   Failed to find the memory table.

**/
RETURN_STATUS
EFIAPI
CbParseCbMemTable (
  IN UINT32     TableId,
  IN VOID**     pMemTable,
  IN UINT32*    pMemTableSize
  );

/**
  Acquire the acpi table from coreboot

  @param  pMemTable          Pointer to the base address of the memory table
  @param  pMemTableSize      Pointer to the size of the memory table

  @retval RETURN_SUCCESS     Successfully find out the memory table.
  @retval RETURN_INVALID_PARAMETER  Invalid input parameters.
  @retval RETURN_NOT_FOUND   Failed to find the memory table.

**/
RETURN_STATUS
EFIAPI
CbParseAcpiTable (
  IN VOID**     pMemTable,
  IN UINT32*    pMemTableSize
  );

/**
  Acquire the smbios table from coreboot

  @param  pMemTable          Pointer to the base address of the memory table
  @param  pMemTableSize      Pointer to the size of the memory table

  @retval RETURN_SUCCESS     Successfully find out the memory table.
  @retval RETURN_INVALID_PARAMETER  Invalid input parameters.
  @retval RETURN_NOT_FOUND   Failed to find the memory table.

**/
RETURN_STATUS
EFIAPI
CbParseSmbiosTable (
  IN VOID**     pMemTable,
  IN UINT32*    pMemTableSize
  );

/**
  Find the required fadt information

  @param  pPmCtrlReg         Pointer to the address of power management control register
  @param  pPmTimerReg        Pointer to the address of power management timer register
  @param  pResetReg          Pointer to the address of system reset register
  @param  pResetValue        Pointer to the value to be writen to the system reset register
  @param  pPmEvtReg          Pointer to the address of power management event register
  @param  pPmGpeEnReg        Pointer to the address of power management GPE enable register

  @retval RETURN_SUCCESS     Successfully find out all the required fadt information.
  @retval RETURN_NOT_FOUND   Failed to find the fadt table.

**/
RETURN_STATUS
EFIAPI
CbParseFadtInfo (
  IN UINTN*     pPmCtrlReg,
  IN UINTN*     pPmTimerReg,
  IN UINTN*     pResetReg,
  IN UINTN*     pResetValue,
  IN UINTN*     pPmEvtReg,
  IN UINTN*     pPmGpeEnReg
  );

/**
  Find the serial port information

  @param  pRegBase           Pointer to the base address of serial port registers
  @param  pRegAccessType     Pointer to the access type of serial port registers
  @param  pRegWidth          Pointer to the register width in bytes
  @param  pBaudrate          Pointer to the serial port baudrate
  @param  pInputHertz        Pointer to the input clock frequency
  @param  pUartPciAddr       Pointer to the UART PCI bus, dev and func address

  @retval RETURN_SUCCESS     Successfully find the serial port information.
  @retval RETURN_NOT_FOUND   Failed to find the serial port information .

**/
RETURN_STATUS
EFIAPI
CbParseSerialInfo (
  OUT UINT32     *pRegBase,
  OUT UINT32     *pRegAccessType,
  OUT UINT32     *pRegWidth,
  OUT UINT32     *pBaudrate,
  OUT UINT32     *pInputHertz,
  OUT UINT32     *pUartPciAddr
  );

/**
  Search for the coreboot table header

  @param  Level              Level of the search depth
  @param  HeaderPtr          Pointer to the pointer of coreboot table header

  @retval RETURN_SUCCESS     Successfully find the coreboot table header .
  @retval RETURN_NOT_FOUND   Failed to find the coreboot table header .

**/
RETURN_STATUS
EFIAPI
CbParseGetCbHeader (
  IN UINTN  Level,
  IN VOID** HeaderPtr
  );

/**
  Find the video frame buffer information

  @param  pFbInfo            Pointer to the FRAME_BUFFER_INFO structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information .

**/
RETURN_STATUS
EFIAPI
CbParseFbInfo (
  IN FRAME_BUFFER_INFO*     pFbInfo
  );

