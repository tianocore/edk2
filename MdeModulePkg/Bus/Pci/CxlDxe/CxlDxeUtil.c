/** @file
  CxlDxe driver utility file
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"
#include "CxlDxeUtil.h"

/**
  Returns minimum among the input values

  @param[in] ValOne                   Input value one
  @param[in] ValTwo                   Input value Two
  @param[in] ValThree                 Input value Three

  @retval Minimum                     Returns minimum value among the given input values

  **/
UINT64
MinimumOfThreeValues (
  UINT64  ValOne,
  UINT64  ValTwo,
  UINT64  ValThree
  )
{
  UINT64  Minimum = MIN (ValOne, MIN (ValTwo, ValThree));

  return Minimum;
}

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
  )
{
  UINT32  Position = StartingBit - EndingBit + 1;     // Num of bits

  RegisterValue = RegisterValue >> EndingBit;         // Right shift to make P2 position as 0 bit position
  UINT64  Mask             = (1 << Position) - 1;     // Crate mask
  UINT64  LastPositionBits = RegisterValue & Mask;

  return LastPositionBits;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      Offset;

  Offset = Start;

  Status = Private->PciIo->Pci.Read (
                                 Private->PciIo,
                                 EfiPciIoWidthUint32,
                                 Offset,
                                 1,
                                 Value
                                 );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: Failed to read PCI IO for Ext. capability\n", __func__));
  }

  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      BarIndex;
  UINT32      ReadValue;

  BarIndex  = Private->RegisterMap.BaseAddressRegister;
  ReadValue = 0;

  Status = Private->PciIo->Mem.Read (
                                 Private->PciIo,
                                 EfiPciIoWidthUint32,
                                 (UINT8)BarIndex,
                                 Start,
                                 1,
                                 &ReadValue
                                 );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: Failed to read PCI Mem\n", __func__));
    return Status;
  }

  *Value = ReadValue;
  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      BarIndex;
  UINT64      ReadValue;

  BarIndex  = Private->RegisterMap.BaseAddressRegister;
  ReadValue = 0;

  Status = Private->PciIo->Mem.Read (
                                 Private->PciIo,
                                 EfiPciIoWidthUint64,
                                 (UINT8)BarIndex,
                                 Start,
                                 1,
                                 &ReadValue
                                 );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: Failed to read PCI Mem\n", __func__));
    return Status;
  }

  *Value = ReadValue;
  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      BarIndex;
  UINT32      Offset;

  BarIndex = Private->RegisterMap.BaseAddressRegister;
  Offset   = Start;

  for (UINT32 Index = 0; Index < Size; Index++) {
    Status = Private->PciIo->Mem.Read (
                                   Private->PciIo,
                                   EfiPciIoWidthUint8,
                                   (UINT8)BarIndex,
                                   Offset,
                                   1,
                                   &Buffer[Index]
                                   );

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[%a]: Read err in Buffer[%d] \n", __func__, Index));
      break;
    }

    Offset += 1;
  }

  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      BarIndex;
  UINT32      WriteValue;

  BarIndex   = Private->RegisterMap.BaseAddressRegister;
  WriteValue = *Value;

  Status = Private->PciIo->Mem.Write (
                                 Private->PciIo,
                                 EfiPciIoWidthUint32,
                                 (UINT8)BarIndex,
                                 Start,
                                 1,
                                 &WriteValue
                                 );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: Failed to write PCI Mem\n", __func__));
  }

  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      BarIndex;
  UINT64      WriteValue;

  BarIndex   = Private->RegisterMap.BaseAddressRegister;
  WriteValue = *Value;

  Status = Private->PciIo->Mem.Write (
                                 Private->PciIo,
                                 EfiPciIoWidthUint64,
                                 (UINT8)BarIndex,
                                 Start,
                                 1,
                                 &WriteValue
                                 );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: Failed to write PCI Mem\n", __func__));
  }

  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      BarIndex;
  UINT32      Offset;

  BarIndex = Private->RegisterMap.BaseAddressRegister;
  Offset   = Start;

  for (UINT32 Index = 0; Index < Size; Index++) {
    Status = Private->PciIo->Mem.Write (
                                   Private->PciIo,
                                   EfiPciIoWidthUint8,
                                   (UINT8)BarIndex,
                                   Offset,
                                   1,
                                   &Buffer[Index]
                                   );

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[%a]: Read err in Buffer[%d] \n", __func__, Index));
      break;
    }

    Offset += 1;
  }

  return Status;
}

