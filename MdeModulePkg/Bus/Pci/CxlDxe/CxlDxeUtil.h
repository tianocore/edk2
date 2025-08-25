/** @file
  Header file for CxlDxe driver
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _EFI_CXLDXE_UTILS_H_
#define _EFI_CXLDXE_UTILS_H_

#include <stddef.h>

/**
  Returns minimum among the input values

  @param[in] ValOne                   Input value one
  @param[in] ValTwo                   Input value Two
  @param[in] ValThree                 Input value Three

  @retval Minimum                     Returns minimum value among the given input values

  **/
UINT64
MinimumOfThreeValues (
  UINT64  a,
  UINT64  b,
  UINT64  c
  );

/**
  Returns the number of chunk from firmware file, FW Transfer should take less time, therefore chunk Size is maximum

  @param[in] FileSize                   Size of Firmware file
  @param[in] MaxPayloadSize             Maximum Payload Size supported by mailbox

  @retval ChunkCount                    Number of Chunks of perticular size
  @retval ChunkSize                     Chunks size to be transferred

  **/
void
GetChunkCount (
  UINT32  FileSize,
  UINT32  MaxPayloadSize,
  UINT32  *ChunkCount,
  UINT32  *ChunkSize
  );

/**
  Returns bits value from input value

  @param[in] RegisterValue               Input register value from where bits has to extracted
  @param[in] StartingBit                 starting bits position
  @param[in] EndingBit                   ending bits position

  @retval LastPositionBits               Value of bits from position one to two

  **/
UINT64
GetFieldValues (
  UINT64  RegisterValue,
  UINT32  StartingBit,
  UINT32  EndingBit
  );

/**
  Initialize Firmware Image Descriptor with default values, which were to be updated in later function calls

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval void                   No value is returned

  **/
void
InitializeFwImageDescriptor (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  );

/**
  Reads EFI PCI i/o protocol values

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start               starting bits position

  @retval                        Value of PCI IO for Extended capability

  **/
EFI_STATUS
PciUefiReadConfigWord (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  UINT32                       *Value
  );

/**
  Reads EFI PCI i/o protocol values of thirty two bits

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start               starting bits position

  @retval                        Value of PCI IO for Extended capability

  **/
EFI_STATUS
PciUefiMemRead32 (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  UINT32                       *Value
  );

/**
  Reads EFI PCI i/o protocol values of sixty four bits

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start               starting bits position

  @retval                        Value of PCI IO for Extended capability

  **/
EFI_STATUS
PciUefiMemRead64 (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  UINT64                       *Value
  );

/**
  Reads EFI PCI i/o protocol values of N bits

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start               starting bits position

  @retval                        Value of PCI IO for Extended capability

  **/
EFI_STATUS
PciUefiMemReadNBits (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  CHAR8                        Buffer[],
  UINT32                       Size
  );

/**
  Write EFI PCI i/o protocol values of thirty two bits

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start               starting bits position

  @retval                        Value of PCI IO for Extended capability

  **/
EFI_STATUS
PciUefiMemWrite32 (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  UINT32                       *Value
  );

/**
  Write EFI PCI i/o protocol values of sixty four bits

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start               starting bits position

  @retval                        Value of PCI IO for Extended capability

  **/
EFI_STATUS
PciUefiMemWrite64 (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  UINT64                       *Value
  );

/**
  Write EFI PCI i/o protocol values of N bits

  @param  Private                The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in] Start               starting bits position

  @retval                        Value of PCI IO for Extended capability

  **/
EFI_STATUS
PciUefiMemWriteNBits (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Start,
  CHAR8                        Buffer[],
  UINT32                       Size
  );

#endif // _EFI_CXLDXE_UTILS_H_


