/** @file
  CxlDxe driver utility file
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"

inline UINT64 min2(UINT64 a, UINT64 b)
{
  if (a <= b) {
    return a;
  } else {
    return b;
  }
}

inline size_t min2size(size_t a, size_t b)
{
  if (a <= b) {
    return a;
  } else {
    return b;
  }
}

inline UINT64 min3(UINT64 a, UINT64 b, UINT64 c)
{
  if (a <= b && a <= c) {
    return a;
  } else if (b <= a && b <= c) {
    return b;
  } else {
    return c;
  }
}

void getChunkCnt(int filesize, int maxPayloadSize, int *chunkCnt, int *chunkSize) {

  /*******************************************************************************
  *   FW Transfer should take less time, so we made chunk size maximum
  *   else chunk size can be CXL_FW_TRANSFER_ALIGNMENT
  ********************************************************************************/

  int cnt = 0, size = 0;
  if (maxPayloadSize % CXL_FW_TRANSFER_ALIGNMENT == 0) {
      size = maxPayloadSize;
  } else {
      size = (maxPayloadSize - (maxPayloadSize % CXL_FW_TRANSFER_ALIGNMENT));
  }

  cnt = (filesize / size);
  if (filesize % size) {
      //Add 1 to cnt for remaining chunk
      cnt = cnt + 1;
  }

  *chunkCnt = cnt;
  *chunkSize = size;
}

UINT64 field_get(UINT64 reg, UINT32 p1, UINT32 p2)
{
  UINT32 X = p1 - p2 + 1;        //Num of bits
  reg = reg >> p2;               //Right shift to make P2 position as 0 bit position
  UINT64 mask = (1 << X) - 1;    //Crate mask
  UINT64 lastXbits = reg & mask;
  return lastXbits;
}

void strCpy(CHAR16 *st1, char *st2)
{
  if (st2[0] == '\0') {
    return;
  }

  int i = 0;
  for (i = 0; st2[i] != '\0'; i++) {
    st1[i] = st2[i];
  }
  st1[i] = '\0';
}

void strCpy_c16(CHAR16 *st1, CHAR16 *st2)
{
  int i = 0;
  for (i = 0; st2[i] != '\0'; i++) {
    st1[i] = st2[i];
  }
  st1[i] = '\0';
}

void strCpy_const16(CHAR16 *st1, CONST CHAR16 *st2)
{
  int i = 0;
  for (i = 0; st2[i] != '\0'; i++) {
      st1[i] = st2[i];
  }
  st1[i] = '\0';
}

int strCmp(CHAR16 *str1, CHAR16 *str2) {

    int i = 0;
    if (str1 == NULL) {
        return 1;
    }

    while (str1[i] == str2[i]) {
      if (str1[i] == '\0' || str2[i] == '\0') {
        break;
      }
      i++;
    }

    if (str1[i] == '\0' && str2[i] == '\0') {
      return 0;
    } else {
        return str1[i] - str2[i];
    }
}

void
InitializeFwImageDescriptor(CXL_CONTROLLER_PRIVATE_DATA *Private)
{
  for (int i = 0; i < Private->slotInfo.num_slots; i++) {
    Private->slotInfo.FwImageDescriptor[i].ImageIndex = i;    /* This should always be between 1 and CXL_FW_IMAGE_DESCRIPTOR_COUNT */
    Private->slotInfo.FwImageDescriptor[i].ImageId = CXL_FW_IMAGE_ID;
    Private->slotInfo.FwImageDescriptor[i].ImageIdName = CXL_FIRMWARE_IMAGE_ID_NAME;
    Private->slotInfo.FwImageDescriptor[i].Version = CXL_FW_VERSION;

    if (Private->slotInfo.firmware_version[i][0] != '\0') {
      Private->slotInfo.FwImageDescriptor[i].VersionName = AllocateZeroPool(CXL_STRING_BUFFER_WIDTH);
      if (Private->slotInfo.FwImageDescriptor[i].VersionName == NULL) {
        DEBUG((EFI_D_ERROR, "InitializeFwImageDescriptor: AllocateZeroPool failed!\n"));
        return;
      }
      strCpy(Private->slotInfo.FwImageDescriptor[i].VersionName, Private->slotInfo.firmware_version[i]);
    }

    Private->slotInfo.FwImageDescriptor[i].Size = Private->slotInfo.imageFileSize[i];
    Private->slotInfo.FwImageDescriptor[i].AttributesSupported = 1;
    Private->slotInfo.FwImageDescriptor[i].AttributesSetting = IMAGE_ATTRIBUTE_IMAGE_UPDATABLE | IMAGE_ATTRIBUTE_RESET_REQUIRED;
    Private->slotInfo.FwImageDescriptor[i].Compatibilities = IMAGE_COMPATIBILITY_CHECK_SUPPORTED;
    Private->slotInfo.FwImageDescriptor[i].LowestSupportedImageVersion = 0;
    Private->slotInfo.FwImageDescriptor[i].LastAttemptVersion = 0;
    Private->slotInfo.FwImageDescriptor[i].LastAttemptStatus = EFI_SUCCESS;
  }
}

EFI_STATUS pci_uefi_read_config_word(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT32 *val) {
  EFI_STATUS  Status;
  UINT32 Offset = start;

  Status = Private->PciIo->Pci.Read(
             Private->PciIo,
             EfiPciIoWidthUint32,
             Offset,
             1,
             val
             );

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: Failed to read PCI IO for Ext. capability\n", __func__));
  }
  return Status;
}

EFI_STATUS pci_uefi_mem_read_32(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT32 *val) {

  EFI_STATUS  Status;
  UINT32 BarIndex = Private->map.bar;
  UINT32 v32 = 0;

  Status = Private->PciIo->Mem.Read(
             Private->PciIo,
             EfiPciIoWidthUint32,
             BarIndex,
             start,
             1,
             &v32
             );

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: Failed to read PCI Mem\n", __func__));
    return Status;
  }

  *val = v32;
  return Status;
}

EFI_STATUS pci_uefi_mem_read_64(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT64 *val) {

  EFI_STATUS Status;
  UINT32 BarIndex = Private->map.bar;
  UINT64 v64 = 0;

  Status = Private->PciIo->Mem.Read(
             Private->PciIo,
             EfiPciIoWidthUint64,
             BarIndex,
             start,
             1,
             &v64
             );

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: Failed to read PCI Mem\n", __func__));
    return Status;
  }

  *val = v64;
  return Status;
}

EFI_STATUS pci_uefi_mem_read_n(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, CHAR8 Buffer[], UINT32 Size) {

  EFI_STATUS  Status;
  UINT32 BarIndex = Private->map.bar;
  UINT32 offset = start;

  for (int i = 0; i < Size; i++) {
    Status = Private->PciIo->Mem.Read(
               Private->PciIo,
               EfiPciIoWidthUint8,
               BarIndex,
               offset,
               1,
               &Buffer[i]
               );

    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "[%a]: Read err in Buffer[%d] \n", __func__, i));
      break;
    }
    offset += 1;
  }
  return Status;
}

EFI_STATUS pci_uefi_mem_write_32(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT32 *val) {

  EFI_STATUS  Status;
  UINT32 BarIndex = Private->map.bar;
  UINT32 v32 = *val;

  Status = Private->PciIo->Mem.Write(
             Private->PciIo,
             EfiPciIoWidthUint32,
             BarIndex,
             start,
             1,
             &v32
             );

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: Failed to write PCI Mem\n", __func__));
  }
  return Status;
}

EFI_STATUS pci_uefi_mem_write_64(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT64 *val) {

  EFI_STATUS  Status;
  UINT32 BarIndex = Private->map.bar;
  UINT64 v64 = *val;

  Status = Private->PciIo->Mem.Write(
             Private->PciIo,
             EfiPciIoWidthUint64,
             BarIndex,
             start,
             1,
             &v64
             );

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "[%a]: Failed to write PCI Mem\n", __func__));
  }
  return Status;
}

EFI_STATUS pci_uefi_mem_write_n(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, CHAR8 Buffer[], UINT32 Size) {

  EFI_STATUS Status;
  UINT32 BarIndex = Private->map.bar;
  UINT32 offset = start;

  for (int i = 0; i < Size; i++) {
    Status = Private->PciIo->Mem.Write(
               Private->PciIo,
               EfiPciIoWidthUint8,
               BarIndex,
               offset,
               1,
               &Buffer[i]
               );

    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "[%a]: Read err in Buffer[%d] \n", __func__, i));
        break;
    }
    offset += 1;
  }
  return Status;
}
