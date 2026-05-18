/** @file
  Main file for CxlDxe driver that implements CxlIo procol.

Copyright 2026 Google LLC<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CxlDxe.h"
#include "Base.h"
#include "IndustryStandard/Cxl20.h"
#include "ProcessorBind.h"
#include "Protocol/CxlIo.h"
#include "Uefi/UefiBaseType.h"
#include "IndustryStandard/PciExpress60.h"

//
// CXL Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gCxlDriverBinding = {
  CxlDriverBindingSupported,
  CxlDriverBindingStart,
  CxlDriverBindingStop,
  0x10,
  NULL,
  NULL
};

/*
 * The PCIe spec regarding Data Object Exchange uses "dword", aka double word
 * aka uint32_t as its fundamental data type. Sizes are often specified in
 * dwords, not in bytes.
 */
#define SIZEOF_IN_DWORDS(thing)  (sizeof(thing) / sizeof(UINT32))
#define CXL_PCI_EXT_CAP_ID_DVSEC  PCI_EXPRESS_EXTENDED_CAPABILITY_DESIGNATED_VENDOR_SPECIFIC_ID

#define CXL_DOE_DATA_OBJECT_READY_TIMEOUT_MICROSECONDS        (1 * 1000 * 1000)
#define CXL_DOE_DATA_OBJECT_READY_POLL_INTERVAL_MICROSECONDS  (10 * 1000)

/**
  Reads an array of UINT32s via the EFI PCI IO protocol.

  @param[in]  Private           The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]  BarIndex          Base Address Register to read data from.
  @param[in]  Start             Start position to read from.
  @param[out] Buffer            The buffer to read into.
  @param[in]  SizeInBytes       The number of *bytes* to read.

  @retval EFI_SUCCESS           Value of PCI IO for Extended capability.

  **/
STATIC
EFI_STATUS
PciUefiMemReadUInt32Array (
  IN CXL_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT8                        BarIndex,
  IN UINT64                       Start,
  OUT UINT32                      *Buffer,
  IN UINTN                        SizeInBytes
  )
{
  EFI_STATUS  Status;
  UINT64      BufferIndex;
  UINT64      Offset;

  ASSERT ((SizeInBytes % 4) == 0);
  if ((SizeInBytes % 4) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = Start;

  for (BufferIndex = 0; BufferIndex < (SizeInBytes / 4); BufferIndex++) {
    Status = Private->PciIo->Mem.Read (
                                   Private->PciIo,
                                   EfiPciIoWidthUint32,
                                   BarIndex,
                                   Offset,
                                   1,
                                   &Buffer[BufferIndex]
                                   );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: PciIo read error at 0x%lx: %r \n", __func__, BufferIndex, Status));
      break;
    }

    Offset += sizeof (UINT32);
  }

  return Status;
}

/**
  Write an array of UINT32s via the EFI PCI i/o protocol.

  @param[in]  Private             The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]  BarIndex            Base Address Register to read data from.
  @param[in]  Start               Start position to read from.
  @param[in]  Buffer              The buffer with the data that should be written.
  @param[in]  SizeInBytes         The number of *bytes* to write.

  @retval EFI_SUCCESS             The data was written successfully.
  @retval other                   Error happenned.

  **/
STATIC
EFI_STATUS
PciUefiMemWriteUInt32Array (
  IN CXL_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT8                        BarIndex,
  IN UINT64                       Start,
  IN UINT32                       *Buffer,
  IN UINTN                        SizeInBytes
  )
{
  EFI_STATUS  Status;
  UINT64      BufferIndex;
  UINT64      Offset;

  ASSERT ((SizeInBytes % 4) == 0);
  if ((SizeInBytes % 4) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = Start;

  for (BufferIndex = 0; BufferIndex < (SizeInBytes / 4); BufferIndex++) {
    Status = Private->PciIo->Mem.Write (
                                   Private->PciIo,
                                   EfiPciIoWidthUint32,
                                   BarIndex,
                                   Offset,
                                   1,
                                   &Buffer[BufferIndex]
                                   );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: PciIo write error at 0x%lx: %r \n", __func__, BufferIndex, Status));
      break;
    }

    Offset += sizeof (UINT32);
  }

  return Status;
}

/**
  Finds PCI Express Designated Vendor-Specific Extended Capability starting from
  postition Position

  @param[in]  Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]  Capability               Extended capability ID.
  @param[in]  Position                 Start position.
  @param[out] Offset                   Pointer to store capability offset to if found.

  @retval EFI_SUCCSESS                 Requested capability was found.
  @retval Other                        Requested capability was not found or error happened.

**/
STATIC
EFI_STATUS
CxlFindNextExtendedCapability (
  IN CXL_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT32                       Capability,
  IN UINT32                       Position,
  OUT UINT32                      *Offset
  )
{
  PCI_EXP_EXT_HDR  Header;
  EFI_STATUS       Status;

  while (Position >= EFI_PCIE_CAPABILITY_BASE_OFFSET) {
    Status = Private->PciIo->Pci.Read (
                                   Private->PciIo,
                                   EfiPciIoWidthUint8,
                                   Position,
                                   sizeof (Header),
                                   &Header
                                   );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Header.CapabilityId == 0) {
      return EFI_NOT_FOUND;
    }

    if (Header.CapabilityId == Capability) {
      *Offset = Position;
      return EFI_SUCCESS;
    }

    Position = Header.NextCapabilityOffset;
  }

  return EFI_NOT_FOUND;
}

/**
  Finds PCI Express Designated Vendor-Specific Extended Capability

  @param[in]  Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]  Capability               Extended capability ID.
  @param[out] Offset                   Pointer to the location of the requested ext. capability.

  @retval EFI_SUCCESS                  Requested capability was found, location is stored at the Offset pointer.
  @retval EFI_NOT_FOUND                Requested capability was not found.
**/
STATIC
EFI_STATUS
CxlFindExtendedCapability (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       Capability,
  UINT32                       *Offset
  )
{
  return CxlFindNextExtendedCapability (Private, Capability, EFI_PCIE_CAPABILITY_BASE_OFFSET, Offset);
}

/**
  Finds Offset of the requested Dvsec for Dvsec with Vendor ID field set to
  1E98h to indicate these Capability structures are defined by the CXL
  specification.

  @param[in]  Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]  DvsecId                  Requested DvsecId for vendor 1E98h.
  @param[out] Offset                   Pointer where the location of the required Dvsec is stored.

  @retval EFI_SUCCESS                  Requested DvsecId was found.
  @retval Other                        Requested DvsecId was not found or error happened.

**/
STATIC
EFI_STATUS
CxlFindDvsecCapability (
  CXL_CONTROLLER_PRIVATE_DATA  *Private,
  UINT32                       DvsecId,
  UINT32                       *Offset
  )
{
  UINT32                                                        Position;
  EFI_STATUS                                                    Status;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_DESIGNATED_VENDOR_SPECIFIC  DvsecHeader;

  Position = 0;
  Status   = CxlFindExtendedCapability (Private, CXL_PCI_EXT_CAP_ID_DVSEC, &Position);

  while (!EFI_ERROR (Status)) {
    Status = Private->PciIo->Pci.Read (
                                   Private->PciIo,
                                   EfiPciIoWidthUint8,
                                   Position,
                                   sizeof (DvsecHeader),
                                   &DvsecHeader
                                   );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((DvsecHeader.DesignatedVendorSpecificHeader1.Bits.DvsecVendorId == CXL_DVSEC_VENDOR_ID) &&
        (DvsecHeader.DesignatedVendorSpecificHeader2.Bits.DvsecId == DvsecId))
    {
      *Offset = Position;
      return EFI_SUCCESS;
    }

    Position = DvsecHeader.Header.NextCapabilityOffset;
    Status   = CxlFindNextExtendedCapability (Private, CXL_PCI_EXT_CAP_ID_DVSEC, Position, &Position);
  }

  return Status;
}

/**
  Check if the proposed register block matches the requested type and if it does

  @param[in]  Private                   The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]  Block                     Register block to be check for.
  @param[in]  Type                      Register block type to check against.
  @param[out] RegisterMap               Pointer to CXL_REGISTER_MAP structure to be filled with regblock's data.

  @retval TRUE                          Regblock is valid and is fo request type.
  @retval FALSE                         Regblock type does not match.
**/
BOOLEAN
CxlDecodeRegblock (
  IN CXL_CONTROLLER_PRIVATE_DATA                *Private,
  IN CXL_DVSEC_REGISTER_LOCATOR_REGISTER_BLOCK  *Block,
  IN UINT32                                     Type,
  OUT CXL_REGISTER_MAP                          *RegisterMap
  )
{
  UINT8   Bar;
  UINT64  Offset;

  if (Type != Block->OffsetLow.Bits.RegisterBlockIdentifier) {
    return FALSE;
  }

  /* Only support 64-bit BARs */
  Bar = (UINT8)(Block->OffsetLow.Bits.RegisterBir / 2);

  Offset =
    ((UINT64)Block->OffsetHigh.Bits.RegisterBlockOffsetHigh << 32) |
    (Block->OffsetLow.Bits.RegisterBlockOffsetLow << 16);

  RegisterMap->RegisterType        = Block->OffsetLow.Bits.RegisterBlockIdentifier;
  RegisterMap->BaseAddressRegister = Bar;
  RegisterMap->RegisterOffset      = Offset;

  return TRUE;
}

/**
  Finds Register low and high based on Register block identifier and calls CxlDecodeRegblock

  @param[in]  Private                  The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]  Type                     Register block identifier type.
  @param[out] RegisterMap              Pointer to CXL_REGISTER_MAP structure to be filled with regblock's data.

  @retval EFI_SUCCESS                 A valid regblock was found.
  @retval other                       An error happened.

**/
EFI_STATUS
CxlFindRegblockInstance (
  IN CXL_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT32                       Type,
  OUT CXL_REGISTER_MAP            *RegisterMap
  )
{
  EFI_STATUS                                 Status;
  UINT32                                     RegisterLocatorSize;
  UINT32                                     RegisterLocator;
  UINT32                                     RegisterLocatorEnd;
  EFI_PCI_IO_PROTOCOL                        *PciIo;
  CXL_DVSEC_REGISTER_LOCATOR                 PcieDvsecHeader;
  CXL_DVSEC_REGISTER_LOCATOR_REGISTER_BLOCK  RegisterBlock;

  RegisterLocatorSize = 0;
  RegisterLocator     = 0;

  Status = CxlFindDvsecCapability (Private, CXL_DVSEC_ID_REGISTER_LOCATOR, &RegisterLocator);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  PciIo  = Private->PciIo;
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        RegisterLocator,
                        sizeof (PcieDvsecHeader),
                        &PcieDvsecHeader
                        );

  RegisterLocatorSize = PcieDvsecHeader.DvsecHeader1.Bits.DvsecLength;
  RegisterLocatorEnd  = RegisterLocator + RegisterLocatorSize;
  RegisterLocator    += sizeof (CXL_DVSEC_REGISTER_LOCATOR);

  /* Loop for each Reg block and get Register Offset Low and high */
  while (RegisterLocator < RegisterLocatorEnd) {
    Status = Private->PciIo->Pci.Read (
                                   Private->PciIo,
                                   EfiPciIoWidthUint8,
                                   RegisterLocator,
                                   sizeof (RegisterBlock),
                                   &RegisterBlock
                                   );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (CxlDecodeRegblock (Private, &RegisterBlock, Type, RegisterMap)) {
      return EFI_SUCCESS;
    }

    RegisterLocator += sizeof (CXL_DVSEC_REGISTER_LOCATOR_REGISTER_BLOCK);
  }

  return EFI_NOT_FOUND;
}

/**
  Finds and sets up the Data Object Exchange (DOE) capability.

  @param[in] Private    The pointer to the CXL_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS   DOE capability was found and set up.
  @retval other         Either capability not found or error happened.
**/
STATIC
EFI_STATUS
EFIAPI
CxlDoeSetup (
  CXL_CONTROLLER_PRIVATE_DATA  *Private
  )
{
  UINT32      Position;
  EFI_STATUS  Status;

  Status = CxlFindExtendedCapability (Private, PCI_EXPRESS_EXTENDED_CAPABILITY_DATA_OBJECT_EXCHANGE_ID, &Position);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to find DOE capability: %r\n", __func__, Status));
    return Status;
  }

  Private->DoeExtCapBase = Position;
  return EFI_SUCCESS;
}

/**
  Check whether device is ready to receive new data through DOE request.

  @param[in]  Pci      PCI IO Protocol handle.
  @param[in]  DoeBase  Base offset of DOE status registers in PCIe device
                       config space.
  @param[out] IsBusy   Pointer to store the busy state: true if busy, false otherwise.

  @retval  EFI_SUCCESS   Successful read operation.
  @retval  Other         Device not ready or failed to check device status.
**/
STATIC
EFI_STATUS
CxlIsDoeBusy (
  IN EFI_PCI_IO_PROTOCOL  *Pci,
  IN UINT32               DoeBase,
  OUT BOOLEAN             *IsBusy
  )
{
  EFI_STATUS                       Status;
  PCI_EXPRESS_DOE_STATUS_REGISTER  DoeStatusReg;
  UINT32                           Reg;

  if ((Pci == NULL) || (IsBusy == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Reg = DoeBase + OFFSET_OF (PCI_EXPRESS_DOE_EXTENDED_CAPABILITY, Status);

  Status = Pci->Pci.Read (Pci, EfiPciIoWidthUint32, Reg, 1, &DoeStatusReg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *IsBusy = DoeStatusReg.Bits.Busy ? TRUE : FALSE;
  return EFI_SUCCESS;
}

/**
  Send DOE request using PCI IO protocol.

  @param[in]  Pci           PCI IO Protocol handle.
  @param[in]  DoeBase       Base offset of DOE status registers in PCIe device
                            config space.
  @param[in]  DwordBuffer   Pointer to the DOE request data.
  @param[in]  NumDwords     Number of DWORDs in the DOE request.

  @retval  EFI_SUCCESS   Successful read operation.
  @retval  Other         Device not ready or failed to check device status.
**/
STATIC
EFI_STATUS
CxlDoeSendRequest (
  IN EFI_PCI_IO_PROTOCOL  *Pci,
  IN UINT32               DoeBase,
  IN UINT32               *DwordBuffer,
  IN UINT64               NumDwords
  )
{
  EFI_STATUS                        Status;
  UINT32                            Index;
  UINT32                            Reg;
  PCI_EXPRESS_DOE_CONTROL_REGISTER  ControlReg;
  BOOLEAN                           IsBusy;
  UINT32                            MaxRetries;

  Status = EFI_SUCCESS;

  /* Wait until DOE is not busy.  */

  IsBusy     = TRUE;
  MaxRetries = 100;
  while (IsBusy && (--MaxRetries > 0)) {
    Status = CxlIsDoeBusy (
               Pci,
               DoeBase,
               &IsBusy
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to query busy status: %r\n", __func__, Status));
      return Status;
    }
  }

  if (MaxRetries == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Device failed to become available\n", __func__));
    return EFI_PROTOCOL_ERROR;
  }

  /* Write the request into the TX mailbox. */

  Index = 0;
  Reg   = DoeBase + OFFSET_OF (PCI_EXPRESS_DOE_EXTENDED_CAPABILITY, WriteDataMailbox);
  while (Index < NumDwords) {
    Status = Pci->Pci.Write (Pci, EfiPciIoWidthUint32, Reg, 1, &DwordBuffer[Index]);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: Error while making DOE request\n", __func__));
      return Status;
    }

    Index++;
  }

  /* Set the Go bit to finish the request. */

  Reg    = DoeBase + OFFSET_OF (PCI_EXPRESS_DOE_EXTENDED_CAPABILITY, Control);
  Status = Pci->Pci.Read (Pci, EfiPciIoWidthUint32, Reg, 1, &ControlReg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: Error while reading DOE control reg\n", __func__));
    return Status;
  }

  ControlReg.Bits.Go = 1;
  Status             = Pci->Pci.Write (Pci, EfiPciIoWidthUint32, Reg, 1, &ControlReg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: Error while writing into DOE control reg\n", __func__));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Reads a single DWORD from the DOE ReadMailbox.

  On success, the UINT32 pointed to by Destination will contain the read word.

  @param[in]  Pci          PCI IO Protocol handle.
  @param[in]  DoeBase      Base offset of DOE registers in PCIe device config
                           space.
  @param[out] Destination  DWORD to populate on success.

  @retval  EFI_SUCCESS     Successful read of a single DOE DWORD.
  @retval  Other           Failed receiving of DOE response.
**/
STATIC
EFI_STATUS
CxlPciDoeReadDword (
  IN EFI_PCI_IO_PROTOCOL  *Pci,
  IN UINT32               DoeBase,
  OUT UINT32              *Destination
  )
{
  EFI_STATUS  Status;
  UINT32      Reg;
  UINT32      DoeReadMbData;

  if ((Pci == NULL) || (Destination == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Reg    = DoeBase + OFFSET_OF (PCI_EXPRESS_DOE_EXTENDED_CAPABILITY, ReadDataMailbox);
  Status = Pci->Pci.Read (Pci, EfiPciIoWidthUint32, Reg, 1, Destination);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read: %r\n", __func__, Status));
    return Status;
  }

  /* Write 1 to DOE Read Data Mailbox to indicate successful Read. */
  DoeReadMbData = 1;
  Status        = Pci->Pci.Write (Pci, EfiPciIoWidthUint32, Reg, 1, &DoeReadMbData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to ack read: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Receive DOE response.

  For CXL, DOE responses carry CDAT structures that hold information about
  remote memory ranges and it's attributes.

  @param[in]      Pci              PCI IO Protocol handle.
  @param[in]      DoeBase          Base offset of DOE registers in PCIe device config
                                   space.
  @param[out]     DoeResp          DOE response buffer.
  @param[in, out] DoeRespSizeBytes As an in-parameter, the max size of DoeResp buffer.
                                   As an out-parameter, the actual size of DoeResp.

  @retval  EFI_SUCCESS     Successful receiving of DOE response.
  @retval  Other           Failed receiving of DOE response.
**/
STATIC
EFI_STATUS
CxlDoeReceiveResponse (
  IN EFI_PCI_IO_PROTOCOL                            *Pci,
  IN UINT32                                         DoeBase,
  OUT EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE  *DoeResp,
  IN OUT UINTN                                      *DoeRespSizeBytes
  )
{
  EFI_STATUS                       Status;
  PCI_EXPRESS_DOE_STATUS_REGISTER  DoeStatusReg;
  UINT32                           Index;
  UINT32                           Length;
  UINT32                           Reg;
  UINTN                            MaxLength;
  UINT32                           *DataObjectReadPtr;
  UINTN                            ElapsedTimeMicroseconds;

  if ((Pci == NULL) || (DoeResp == NULL) || (DoeRespSizeBytes == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MaxLength = *DoeRespSizeBytes / sizeof (UINT32);

  if (MaxLength < SIZEOF_IN_DWORDS (EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }

  DoeStatusReg.Uint32     = 0;
  ElapsedTimeMicroseconds = 0;

  while (ElapsedTimeMicroseconds < CXL_DOE_DATA_OBJECT_READY_TIMEOUT_MICROSECONDS) {
    Reg    = DoeBase + OFFSET_OF (PCI_EXPRESS_DOE_EXTENDED_CAPABILITY, Status);
    Status = Pci->Pci.Read (Pci, EfiPciIoWidthUint32, Reg, 1, &DoeStatusReg);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (DoeStatusReg.Bits.Error == 1) {
      return EFI_DEVICE_ERROR;
    }

    if (DoeStatusReg.Bits.DataObjectReady == 1) {
      break;
    }

    gBS->Stall (CXL_DOE_DATA_OBJECT_READY_POLL_INTERVAL_MICROSECONDS);
    ElapsedTimeMicroseconds += CXL_DOE_DATA_OBJECT_READY_POLL_INTERVAL_MICROSECONDS;
  }

  if (DoeStatusReg.Bits.DataObjectReady == 0) {
    DEBUG ((DEBUG_WARN, "%a: Data object not ready\n", __func__));
    return EFI_TIMEOUT;
  }

  DataObjectReadPtr = (UINT32 *)DoeResp;
  Status            = CxlPciDoeReadDword (Pci, DoeBase, DataObjectReadPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read of header dword: %r\n", __func__, Status));
    return Status;
  }

  DataObjectReadPtr++;

  /* Read the DOE Header 2 for data length. */
  Status = CxlPciDoeReadDword (Pci, DoeBase, DataObjectReadPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read header length: %r\n", __func__, Status));
    return Status;
  }

  DataObjectReadPtr++;

  Length = DoeResp->Header.Header.Length;
  if (Length < SIZEOF_IN_DWORDS (EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "%a: Length too short: %u\n", Length, __func__));
    return EFI_PROTOCOL_ERROR;
  }

  if (Length > MaxLength) {
    DEBUG ((DEBUG_ERROR, "%a: CDAT table too big for response buffer\n", __func__));
    return EFI_PROTOCOL_ERROR;
  }

  /* Read DOE read entry response header. */
  Status = CxlPciDoeReadDword (Pci, DoeBase, DataObjectReadPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read response header: %r\n", __func__, Status));
    return Status;
  }

  DataObjectReadPtr++;

  Index   = 0;
  Length -= SIZEOF_IN_DWORDS (EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE_HEADER);
  while (Index < Length) {
    /* Read dword from CDAT response body. */
    Status = CxlPciDoeReadDword (Pci, DoeBase, DataObjectReadPtr);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to read resp body dword %u: %r\n", __func__, Index, Status));
      return Status;
    }

    DataObjectReadPtr++;
    Index++;
  }

  *DoeRespSizeBytes = (UINTN)DataObjectReadPtr - (UINTN)DoeResp;

  return Status;
}

/**
  Make DOE request to fetch CDAT structures and receive DOE response.

  @param[in]      Pci              PCI IO Protocol handle.
  @param[in]      DoeBase          Base offset of DOE registers in PCIe device config space.
  @param[in]      DoeReq           The DOE table access request.
  @param[out]     DoeResp          The DOE table access response buffer.
  @param[in, out] DoeRespSizeBytes As an in-parameter, the max size of the DoeResp buffer.
                                   As an out-paramter, the actual size of the DoeResp.

  @retval  EFI_SUCCESS   Successful DOE request and response receive.
  @retval  Other         Failed DOE request or response receive.
**/
STATIC
EFI_STATUS
CxlPerformCdatDoeTransaction (
  IN EFI_PCI_IO_PROTOCOL                            *Pci,
  IN UINT32                                         DoeBase,
  IN EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_REQUEST    *DoeReq,
  OUT EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE  *DoeResp,
  IN OUT UINTN                                      *DoeRespSizeBytes
  )
{
  EFI_STATUS  Status;

  if ((Pci == NULL) || (DoeReq == NULL) || (DoeResp == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  /* Send the request for the next CDAT object. */

  Status = CxlDoeSendRequest (Pci, DoeBase, (UINT32 *)DoeReq, SIZEOF_IN_DWORDS (*DoeReq));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error while sending DOE request\n", __func__));
    return Status;
  }

  /* Receive the response of the CDAT object. */

  Status = CxlDoeReceiveResponse (Pci, DoeBase, DoeResp, DoeRespSizeBytes);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: Error while receiving DOE response\n", __func__));
    return Status;
  }

  return Status;
}

/**
  The function performs a DOE transaction: sends a DOE request and reads back a response.

  @param[in]      This                Pointer to EDKII_CXL_IO_PROTOCOL.
  @param[in]      RequestBuffer       Pointer to a buffer with the DOE request.
  @param[in]      RequestBufferSize   The size of DOE requst in bytes.
  @param[out]     ResponseBuffer      Pointer to a buffer to store DOE response.
  @param[in, out] ResponseBufferSize  As an in-parameter, the max size of the DOE response,
                                      as an out-paramter, the actual size of the DOE response.

  @retval  EFI_SUCCESS   Successful DOE request and response receive.
  @retval  Other         Failed DOE request or response receive.
**/
STATIC
EFI_STATUS
EFIAPI
CxlDoeTransact (
  IN     EDKII_CXL_IO_PROTOCOL  *This,
  IN CONST VOID                 *RequestBuffer,
  IN     UINTN                  RequestBufferSize,
  OUT VOID                      *ResponseBuffer,
  IN OUT UINTN                  *ResponseBufferSize
  )
{
  CXL_CONTROLLER_PRIVATE_DATA               *Private;
  EFI_STATUS                                Status;
  CONST PCI_EXPRESS_DOE_DATA_OBJECT_HEADER  *DoeHeader;

  if ((This == NULL) || (ResponseBuffer == NULL) || (ResponseBufferSize == NULL) || (RequestBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RequestBufferSize < sizeof (PCI_EXPRESS_DOE_DATA_OBJECT_HEADER)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  DoeHeader = RequestBuffer;

  if ((DoeHeader->DataObjectType != EfiCxlDoeTableAccess) || (DoeHeader->VendorId != CXL_DVSEC_VENDOR_ID)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: DOE (vendor id: %x, type: %u) not supported on this device\n",
      __func__,
      (UINT32)DoeHeader->VendorId,
      (UINT32)DoeHeader->DataObjectType
      ));
    return EFI_UNSUPPORTED;
  }

  if (*ResponseBufferSize < sizeof (EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_RESPONSE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "%a: Output buffer too small\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Private = CXL_CONTROLLER_PRIVATE_FROM_CXL_IO (This);

  if (Private->DoeExtCapBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: DOE not supported on this device\n", __func__));
    return EFI_UNSUPPORTED;
  }

  Status = CxlPerformCdatDoeTransaction (
             Private->PciIo,
             Private->DoeExtCapBase,
             (EFI_CXL_DOE_TABLE_ACCESS_READ_ENTRY_REQUEST *)RequestBuffer,
             ResponseBuffer,
             ResponseBufferSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to execute DOE transaction: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Reads a set of regblock registers.

  @param[in]  This           A pointer to the EDKII_CXL_IO_PROTOCOL instance.
  @param[in]  RegBlock       The type of regblock regs.
  @param[in]  Offset         Offset from the start of the regblock.
  @param[out] Buf            The buf to read regblock registers into.
  @param[in]  Size           The number of bytes to read.

  @retval EFI_SUCCESS All bytes were read successfully.
  @retval other       Something went wrong.
**/
STATIC
EFI_STATUS
EFIAPI
CxlReadRegblockRegisters (
  IN EDKII_CXL_IO_PROTOCOL  *This,
  IN UINT32                 RegBlock,
  IN UINT64                 Offset,
  OUT VOID                  *Buf,
  IN UINTN                  Size
  )
{
  CXL_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_STATUS                   Status;
  CXL_REGISTER_MAP             RegisterMap;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = CXL_CONTROLLER_PRIVATE_FROM_CXL_IO (This);

  Status = CxlFindRegblockInstance (Private, RegBlock, &RegisterMap);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  return PciUefiMemReadUInt32Array (
           Private,
           RegisterMap.BaseAddressRegister,
           RegisterMap.RegisterOffset + Offset,
           (UINT32 *)Buf,
           Size
           );
}

/**
  Reads a set of DVSEC registers.

  @param[in]  This           A pointer to the EDKII_CXL_IO_PROTOCOL instance.
  @param[in]  CxlDvsecId     The DVSEC type to read.
  @param[in]  Offset         Offset from the start of the DVSEC.
  @param[out] Buf            The buf to read DVSEC registers into.
  @param[in]  Size           The number of bytes to read.

  @retval EFI_SUCCESS All bytes were read successfully.
  @retval other       Something went wrong.
**/
STATIC
EFI_STATUS
EFIAPI
CxlReadDvsecRegisters (
  IN EDKII_CXL_IO_PROTOCOL  *This,
  IN UINT32                 CxlDvsecId,
  IN UINT32                 Offset,
  OUT VOID                  *Buf,
  IN UINTN                  Size
  )
{
  CXL_CONTROLLER_PRIVATE_DATA  *Private;
  UINT32                       DvsecAddress;
  EFI_STATUS                   Status;
  UINTN                        SizeInWords;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Size % sizeof (UINT16)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Private = CXL_CONTROLLER_PRIVATE_FROM_CXL_IO (This);

  Status = CxlFindDvsecCapability (Private, CxlDvsecId, &DvsecAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SizeInWords = Size / sizeof (UINT16);

  return Private->PciIo->Pci.Read (
                               Private->PciIo,
                               EfiPciIoWidthUint16,
                               DvsecAddress + Offset,
                               SizeInWords,
                               Buf
                               );
}

/**
  Writes a set of regblock registers.

  @param[in]  This           A pointer to the EDKII_CXL_IO_PROTOCOL instance.
  @param[in]  RegBlock       The type of regblock regs.
  @param[in]  Offset         Offset from the start of the regblock.
  @param[in]  Buf            The buffer to write to the regblock registers.
  @param[in]  Size           The number of bytes to write.

  @retval EFI_SUCCESS All bytes were written successfully.
  @retval other       Something went wrong.
**/
STATIC
EFI_STATUS
EFIAPI
CxlWriteRegblockRegisters (
  IN EDKII_CXL_IO_PROTOCOL  *This,
  IN UINT32                 RegBlock,
  IN UINT64                 Offset,
  IN VOID                   *Buf,
  IN UINTN                  Size
  )
{
  CXL_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_STATUS                   Status;
  CXL_REGISTER_MAP             RegisterMap;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = CXL_CONTROLLER_PRIVATE_FROM_CXL_IO (This);

  Status = CxlFindRegblockInstance (Private, RegBlock, &RegisterMap);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  return PciUefiMemWriteUInt32Array (
           Private,
           RegisterMap.BaseAddressRegister,
           RegisterMap.RegisterOffset + Offset,
           (UINT32 *)Buf,
           Size
           );
}

/**
  Writes a set of DVSEC registers.

  @param[in]  This           A pointer to the EDKII_CXL_IO_PROTOCOL instance.
  @param[in]  CxlDvsecId     The DVSEC type to write.
  @param[in]  Offset         Offset from the start of the DVSEC area.
  @param[in]  Buf            The buf to write to the DVSEC registers.
  @param[in]  Size           The number of bytes to write.

  @retval EFI_SUCCESS All bytes were written successfully.
  @retval other       Something went wrong.
**/
STATIC
EFI_STATUS
EFIAPI
CxlWriteDvsecRegisters (
  IN EDKII_CXL_IO_PROTOCOL  *This,
  IN UINT32                 CxlDvsecId,
  IN UINT32                 Offset,
  IN VOID                   *Buf,
  IN UINTN                  Size
  )
{
  CXL_CONTROLLER_PRIVATE_DATA  *Private;
  UINT32                       DvsecAddress;
  EFI_STATUS                   Status;
  UINTN                        SizeInWords;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Size % sizeof (UINT16)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Private = CXL_CONTROLLER_PRIVATE_FROM_CXL_IO (This);

  Status = CxlFindDvsecCapability (Private, CxlDvsecId, &DvsecAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SizeInWords = Size / sizeof (UINT16);

  return Private->PciIo->Pci.Write (
                               Private->PciIo,
                               EfiPciIoWidthUint16,
                               DvsecAddress + Offset,
                               SizeInWords,
                               Buf
                               );
}

/**
  Enables the device associated with PciIo.

  @param[in] PciIo      The device to enable.

  @retval EFI_SUCCESS   The device was enabled.
  @retval other         Device enabling failed, error code.
**/
STATIC
EFI_STATUS
EFIAPI
CxlEnableDevice (
  EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  EFI_STATUS  Status;
  UINT64      Supports;

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
  Status    = PciIo->Attributes (
                       PciIo,
                       EfiPciIoAttributeOperationEnable,
                       Supports,
                       NULL
                       );

  return Status;
}

/**
  Tests to see if this driver supports a given controller.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
CxlDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT8                ClassCode[3];

  /* Ensure driver won't be started multiple times */
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiCallerIdGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    /* Driver already started */
    return EFI_ALREADY_STARTED;
  }

  /* Attempt to open PCI I/O Protocol */
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_ALREADY_STARTED;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (ClassCode),
                        ClassCode
                        );

  if (EFI_ERROR (Status)) {
    goto OnExit;
  }

  if ((ClassCode[0] != CXL_MEMORY_PROGIF) || (ClassCode[1] != CXL_MEMORY_SUB_CLASS) || (ClassCode[2] != CXL_MEMORY_CLASS)) {
    Status = EFI_UNSUPPORTED;
  }

OnExit:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
}

/**
  Stop the CXL device handled by this driver.

  @param  This                   The CxlIo keyboard driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
CxlDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  CXL_CONTROLLER_PRIVATE_DATA  *Private;
  EDKII_CXL_IO_PROTOCOL        *CxlIo;
  EFI_STATUS                   Status;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEdkiiCxlIoProtocolGuid,
                  (VOID **)&CxlIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = CXL_CONTROLLER_PRIVATE_FROM_CXL_IO (CxlIo);
  //
  // Uninstall child handle
  //
  Status = gBS->UninstallProtocolInterface (
                  Private->ControllerHandle,
                  &gEdkiiCxlIoProtocolGuid,
                  &Private->CxlIo
                  );
  FreePool ((VOID *)Private);

  if (!EFI_ERROR (Status)) {
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
  }

  return Status;
}

STATIC
EDKII_CXL_IO_PROTOCOL  gEdkiiCxlIoInterface = {
  .PciIo          = NULL,         /* To be populated at time of driver instantiation. */
  .DoeTransact    = CxlDoeTransact,
  .Regblock.Read  = CxlReadRegblockRegisters,
  .Regblock.Write = CxlWriteRegblockRegisters,
  .Dvsec.Read     = CxlReadDvsecRegisters,
  .Dvsec.Write    = CxlWriteDvsecRegisters
};

/**
  Starts the CXL device

  @param  This                   The CxlDxe driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The device can be controlled by the CxlIo protocol.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
CxlDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_PCI_IO_PROTOCOL          *PciIo;
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Private = NULL;
  Status  = gBS->OpenProtocol (
                   Controller,
                   &gEfiPciIoProtocolGuid,
                   (VOID **)&PciIo,
                   This->DriverBindingHandle,
                   Controller,
                   EFI_OPEN_PROTOCOL_BY_DRIVER
                   );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = AllocateZeroPool (sizeof (CXL_CONTROLLER_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto OnExit;
  }

  Private->Signature           = CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE;
  Private->ControllerHandle    = Controller;
  Private->ImageHandle         = This->DriverBindingHandle;
  Private->DriverBindingHandle = This->DriverBindingHandle;
  Private->PciIo               = PciIo;

  /* Initialize CXL protocol. */
  CopyMem (&Private->CxlIo, &gEdkiiCxlIoInterface, sizeof (gEdkiiCxlIoInterface));
  Private->CxlIo.PciIo = Private->PciIo;

  Status = PciIo->GetLocation (PciIo, &Private->Seg, &Private->Bus, &Private->Device, &Private->Function);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: PciIo GetLocation failed: %r\n", __func__, Status));
    goto OnExit;
  }

  Status = CxlEnableDevice (PciIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to enable %04llx:%02llx:%02llx.02llx: %r\n",
      Private->Seg,
      Private->Bus,
      Private->Device,
      Private->Function,
      Status
      ));
    goto OnExit;
  }

  Status = CxlDoeSetup (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to setup DOE registers: %r\n", __func__, Status));
    goto OnExit;
  }

  Status = gBS->InstallProtocolInterface (
                  &Private->ControllerHandle,
                  &gEdkiiCxlIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->CxlIo
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CxlIo protocol install failed: %r\n", __func__, Status));
    goto OnExit;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: CXL driver binding succeeded for %04llx:%02llx:%02llx.%02llx\n",
    __func__,
    Private->Seg,
    Private->Bus,
    Private->Device,
    Private->Function
    ));

  return EFI_SUCCESS;

OnExit:
  if (Private != NULL) {
    FreePool (Private);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  CxlDxeComponentName = {
  CxlDxeComponentNameGetDriverName,
  CxlDxeComponentNameGetControllerName,
  "eng"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  CxlDxeComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)CxlDxeComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)CxlDxeComponentNameGetControllerName,
  "en"
};

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  CxlDxeDriverNameTable[] = {
  { "eng;en", L"UEFI CXL Driver" },
  { NULL,     NULL               }
};

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
CxlDxeComponentNameGetDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN CHAR8                        *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           CxlDxeDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &CxlDxeComponentName)
           );
}

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName        A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
CxlDxeComponentNameGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN CHAR8                        *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

/**
  The entry point for CXL driver, used to install CXL driver on the ImageHandle.

  @param  ImageHandle   The firmware allocated handle for this driver image.
  @param  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   Driver loaded.
  @retval other         Driver not loaded.

**/
EFI_STATUS
EFIAPI
CxlDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INIT, "%a: Enter\n", __func__));

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gCxlDriverBinding,
             ImageHandle,
             &CxlDxeComponentName,
             &CxlDxeComponentName2
             );

  return Status;
}
