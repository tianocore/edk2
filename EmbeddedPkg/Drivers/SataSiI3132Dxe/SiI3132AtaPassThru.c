/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "SataSiI3132.h"

#include <IndustryStandard/Atapi.h>
#include <Library/DevicePathLib.h>

SATA_SI3132_DEVICE*
GetSataDevice (
  IN  SATA_SI3132_INSTANCE* SataInstance,
  IN  UINT16 Port,
  IN  UINT16 PortMultiplierPort
) {
  LIST_ENTRY              *List;
  SATA_SI3132_PORT        *SataPort;
  SATA_SI3132_DEVICE      *SataDevice;

  if (Port >= SATA_SII3132_MAXPORT) {
    return NULL;
  }

  SataPort = &(SataInstance->Ports[Port]);
  List = SataPort->Devices.ForwardLink;

  while (List != &SataPort->Devices) {
    SataDevice = (SATA_SI3132_DEVICE*)List;
    if (SataDevice->Index == PortMultiplierPort) {
      return SataDevice;
    }
    List = List->ForwardLink;
  }
  return NULL;
}

EFI_STATUS
SiI3132AtaPassThruCommand (
  IN     SATA_SI3132_INSTANCE             *SataSiI3132Instance,
  IN     SATA_SI3132_PORT                 *SataPort,
  IN     UINT16                           PortMultiplierPort,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET *Packet,
  IN     EFI_EVENT                        Event OPTIONAL
  )
{
  SATA_SI3132_DEVICE      *SataDevice;
  EFI_PHYSICAL_ADDRESS    PhysInDataBuffer;
  UINTN                   InDataBufferLength = 0;
  EFI_PHYSICAL_ADDRESS    PhysOutDataBuffer;
  UINTN                   OutDataBufferLength;
  CONST UINTN             EmptySlot = 0;
  UINTN                   Control = PRB_CTRL_ATA;
  UINTN                   Protocol = 0;
  UINT32                  Value32, Error, Timeout = 0;
  CONST UINT32            IrqMask = (SII3132_PORT_INT_CMDCOMPL | SII3132_PORT_INT_CMDERR) << 16;
  EFI_STATUS              Status;
  VOID*                   PciAllocMapping = NULL;
  EFI_PCI_IO_PROTOCOL     *PciIo;

  PciIo = SataSiI3132Instance->PciIo;
  ZeroMem (SataPort->HostPRB, sizeof (SATA_SI3132_PRB));

  // Construct Si3132 PRB
  switch (Packet->Protocol) {
  case EFI_ATA_PASS_THRU_PROTOCOL_ATA_HARDWARE_RESET:
    ASSERT (0); //TODO: Implement me!
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_ATA_SOFTWARE_RESET:
    SATA_TRACE ("SiI3132AtaPassThru() EFI_ATA_PASS_THRU_PROTOCOL_ATA_SOFTWARE_RESET");
    Control = PRB_CTRL_SRST;

    if (FeaturePcdGet (PcdSataSiI3132FeaturePMPSupport)) {
        SataPort->HostPRB->Fis.Control = 0x0F;
    }
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_ATA_NON_DATA:
    ASSERT (0); //TODO: Implement me!
    break;

  // There is no difference for SiI3132 between PIO and DMA invokation
  case EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_IN:
  case EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN:
    // Fixup the size for block transfer. Following UEFI Specification, 'InTransferLength' should
    // be in number of bytes. But for most data transfer commands, the value is in number of blocks
    if (Packet->Acb->AtaCommand == ATA_CMD_IDENTIFY_DRIVE) {
      InDataBufferLength = Packet->InTransferLength;
    } else {
      SataDevice = GetSataDevice (SataSiI3132Instance, SataPort->Index, PortMultiplierPort);
      if (!SataDevice || (SataDevice->BlockSize == 0)) {
        return EFI_INVALID_PARAMETER;
      }

      InDataBufferLength = Packet->InTransferLength * SataDevice->BlockSize;
    }

    Status = PciIo->Map (
               PciIo, EfiPciIoOperationBusMasterRead,
               Packet->InDataBuffer, &InDataBufferLength, &PhysInDataBuffer, &PciAllocMapping
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Construct SGEs (32-bit system)
    SataPort->HostPRB->Sge[0].DataAddressLow = (UINT32)PhysInDataBuffer;
    SataPort->HostPRB->Sge[0].DataAddressHigh = (UINT32)(PhysInDataBuffer >> 32);
    SataPort->HostPRB->Sge[0].Attributes = SGE_TRM; // Only one SGE
    SataPort->HostPRB->Sge[0].DataCount = InDataBufferLength;

    // Copy the Ata Command Block
    CopyMem (&SataPort->HostPRB->Fis, Packet->Acb, sizeof (EFI_ATA_COMMAND_BLOCK));

    // Fixup the FIS
    SataPort->HostPRB->Fis.FisType = 0x27; // Register - Host to Device FIS
    SataPort->HostPRB->Fis.Control = 1 << 7; // Is a command
    if (FeaturePcdGet (PcdSataSiI3132FeaturePMPSupport)) {
      SataPort->HostPRB->Fis.Control |= PortMultiplierPort & 0xFF;
    }
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_UDMA_DATA_OUT:
  case EFI_ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT:
    SataDevice = GetSataDevice (SataSiI3132Instance, SataPort->Index, PortMultiplierPort);
    if (!SataDevice || (SataDevice->BlockSize == 0)) {
      return EFI_INVALID_PARAMETER;
    }

    // Fixup the size for block transfer. Following UEFI Specification, 'InTransferLength' should
    // be in number of bytes. But for most data transfer commands, the value is in number of blocks
    OutDataBufferLength = Packet->OutTransferLength * SataDevice->BlockSize;

    Status = PciIo->Map (
               PciIo, EfiPciIoOperationBusMasterWrite,
               Packet->OutDataBuffer, &OutDataBufferLength, &PhysOutDataBuffer, &PciAllocMapping
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Construct SGEs (32-bit system)
    SataPort->HostPRB->Sge[0].DataAddressLow  = (UINT32)PhysOutDataBuffer;
    SataPort->HostPRB->Sge[0].DataAddressHigh = (UINT32)(PhysOutDataBuffer >> 32);
    SataPort->HostPRB->Sge[0].Attributes      = SGE_TRM; // Only one SGE
    SataPort->HostPRB->Sge[0].DataCount       = OutDataBufferLength;

    // Copy the Ata Command Block
    CopyMem (&SataPort->HostPRB->Fis, Packet->Acb, sizeof (EFI_ATA_COMMAND_BLOCK));

    // Fixup the FIS
    SataPort->HostPRB->Fis.FisType = 0x27; // Register - Host to Device FIS
    SataPort->HostPRB->Fis.Control = 1 << 7; // Is a command
    if (FeaturePcdGet (PcdSataSiI3132FeaturePMPSupport)) {
      SataPort->HostPRB->Fis.Control |= PortMultiplierPort & 0xFF;
    }
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_DMA:
    ASSERT (0); //TODO: Implement me!
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_DMA_QUEUED:
    ASSERT (0); //TODO: Implement me!
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_DEVICE_DIAGNOSTIC:
    ASSERT (0); //TODO: Implement me!
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_DEVICE_RESET:
    ASSERT (0); //TODO: Implement me!
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_FPDMA:
    ASSERT (0); //TODO: Implement me!
    break;
  case EFI_ATA_PASS_THRU_PROTOCOL_RETURN_RESPONSE:
    ASSERT (0); //TODO: Implement me!
    break;
  default:
    ASSERT (0);
    break;
  }

  SataPort->HostPRB->Control = Control;
  SataPort->HostPRB->ProtocolOverride = Protocol;

  // Clear IRQ
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, IrqMask);

  if (!FeaturePcdGet (PcdSataSiI3132FeatureDirectCommandIssuing)) {
    // Indirect Command Issuance

    //TODO: Find which slot is free (maybe use the Cmd FIFO)
    //SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_CMDEXECFIFO_REG, &EmptySlot);

    SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_CMDACTIV_REG + (EmptySlot * 8),
                     (UINT32)(SataPort->PhysAddrHostPRB & 0xFFFFFFFF));
    SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_CMDACTIV_REG + (EmptySlot * 8) + 4,
                     (UINT32)((SataPort->PhysAddrHostPRB >> 32) & 0xFFFFFFFF));
  } else {
    // Direct Command Issuance
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 1, // Bar 1
        SataPort->RegBase + (EmptySlot * 0x80),
        sizeof (SATA_SI3132_PRB) / 4,
        SataPort->HostPRB);
    ASSERT_EFI_ERROR (Status);

    SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_CMDEXECFIFO_REG, EmptySlot);
  }

#if 0
  // Could need to be implemented if we run multiple command in parallel to know which slot has been completed
  SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_SLOTSTATUS_REG, &Value32);
  Timeout = Packet->Timeout;
  while (!Timeout && !Value32) {
    gBS->Stall (1);
    SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_SLOTSTATUS_REG, &Value32);
    Timeout--;
  }
#else
  SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, &Value32);
  if (!Packet->Timeout) {
    while (!(Value32 & IrqMask)) {
      gBS->Stall (1);
      SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, &Value32);
    }
  } else {
    Timeout = Packet->Timeout;
    while (Timeout && !(Value32 & IrqMask)) {
      gBS->Stall (1);
      SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, &Value32);
      Timeout--;
    }
  }
#endif
  // Fill Packet Ata Status Block
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, 1, // Bar 1
      SataPort->RegBase + 0x08,
      sizeof (EFI_ATA_STATUS_BLOCK) / 4,
      Packet->Asb);
  ASSERT_EFI_ERROR (Status);


  if ((Packet->Timeout != 0) && (Timeout == 0)) {
    DEBUG ((EFI_D_ERROR, "SiI3132AtaPassThru() Err:Timeout\n"));
    //ASSERT (0);
    return EFI_TIMEOUT;
  } else if (Value32 & (SII3132_PORT_INT_CMDERR << 16)) {
    SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_CMDERROR_REG, &Error);
    DEBUG ((EFI_D_ERROR, "SiI3132AtaPassThru() CmdErr:0x%X (SiI3132 Err:0x%X)\n", Value32, Error));
    ASSERT (0);
    return EFI_DEVICE_ERROR;
  } else if (Value32 & (SII3132_PORT_INT_CMDCOMPL << 16)) {
    // Clear Command Complete
    SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, SII3132_PORT_INT_CMDCOMPL << 16);

    if (PciAllocMapping) {
      Status = PciIo->Unmap (PciIo, PciAllocMapping);
      ASSERT (!EFI_ERROR (Status));
    }

    // If the command was ATA_CMD_IDENTIFY_DRIVE then we need to update the BlockSize
    if (Packet->Acb->AtaCommand == ATA_CMD_IDENTIFY_DRIVE) {
      ATA_IDENTIFY_DATA *IdentifyData = (ATA_IDENTIFY_DATA*)Packet->InDataBuffer;

      // Get the corresponding Block Device
      SataDevice = GetSataDevice (SataSiI3132Instance, SataPort->Index, PortMultiplierPort);

      // Check logical block size
      if ((IdentifyData->phy_logic_sector_support & BIT12) != 0) {
        ASSERT (SataDevice != NULL);
        SataDevice->BlockSize = (UINT32) (((IdentifyData->logic_sector_size_hi << 16) |
                                            IdentifyData->logic_sector_size_lo) * sizeof (UINT16));
      } else {
        SataDevice->BlockSize = 0x200;
      }
    }
    return EFI_SUCCESS;
  } else {
    ASSERT (0);
    return EFI_DEVICE_ERROR;
  }
}

/**
  Sends an ATA command to an ATA device that is attached to the ATA controller. This function
  supports both blocking I/O and non-blocking I/O. The blocking I/O functionality is required,
  and the non-blocking I/O functionality is optional.

  @param[in]     This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]     Port                The port number of the ATA device to send the command.
  @param[in]     PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                     If there is no port multiplier, then specify 0.
  @param[in,out] Packet              A pointer to the ATA command to send to the ATA device specified by Port
                                     and PortMultiplierPort.
  @param[in]     Event               If non-blocking I/O is not supported then Event is ignored, and blocking
                                     I/O is performed. If Event is NULL, then blocking I/O is performed. If
                                     Event is not NULL and non blocking I/O is supported, then non-blocking
                                     I/O is performed, and Event will be signaled when the ATA command completes.

  @retval EFI_SUCCESS                The ATA command was sent by the host. For bi-directional commands,
                                     InTransferLength bytes were transferred from InDataBuffer. For write and
                                     bi-directional commands, OutTransferLength bytes were transferred by OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE        The ATA command was not executed. The number of bytes that could be transferred
                                     is returned in InTransferLength. For write and bi-directional commands,
                                     OutTransferLength bytes were transferred by OutDataBuffer.
  @retval EFI_NOT_READY              The ATA command could not be sent because there are too many ATA commands
                                     already queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR           A device error occurred while attempting to send the ATA command.
  @retval EFI_INVALID_PARAMETER      Port, PortMultiplierPort, or the contents of Acb are invalid. The ATA
                                     command was not sent, so no additional status information is available.

**/
EFI_STATUS
SiI3132AtaPassThru (
  IN     EFI_ATA_PASS_THRU_PROTOCOL       *This,
  IN     UINT16                           Port,
  IN     UINT16                           PortMultiplierPort,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET *Packet,
  IN     EFI_EVENT                        Event OPTIONAL
  )
{
  SATA_SI3132_INSTANCE    *SataSiI3132Instance;
  SATA_SI3132_DEVICE      *SataDevice;
  SATA_SI3132_PORT        *SataPort;

  SataSiI3132Instance = INSTANCE_FROM_ATAPASSTHRU_THIS (This);
  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  SataDevice = GetSataDevice (SataSiI3132Instance, Port, PortMultiplierPort);
  if (!SataDevice) {
    return EFI_INVALID_PARAMETER;
  }
  SataPort = SataDevice->Port;

  DEBUG ((EFI_D_INFO, "SiI3132AtaPassThru(%d,%d) : AtaCmd:0x%X Prot:%d\n", Port, PortMultiplierPort,
         Packet->Acb->AtaCommand, Packet->Protocol));

  return SiI3132AtaPassThruCommand (SataSiI3132Instance, SataPort, PortMultiplierPort, Packet, Event);
}

/**
  Used to retrieve the list of legal port numbers for ATA devices on an ATA controller.
  These can either be the list of ports where ATA devices are actually present or the
  list of legal port numbers for the ATA controller. Regardless, the caller of this
  function must probe the port number returned to see if an ATA device is actually
  present at that location on the ATA controller.

  The GetNextPort() function retrieves the port number on an ATA controller. If on input
  Port is 0xFFFF, then the port number of the first port on the ATA controller is returned
  in Port and EFI_SUCCESS is returned.

  If Port is a port number that was returned on a previous call to GetNextPort(), then the
  port number of the next port on the ATA controller is returned in Port, and EFI_SUCCESS
  is returned. If Port is not 0xFFFF and Port was not returned on a previous call to
  GetNextPort(), then EFI_INVALID_PARAMETER is returned.

  If Port is the port number of the last port on the ATA controller, then EFI_NOT_FOUND is
  returned.

  @param[in]     This           A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in,out] Port           On input, a pointer to the port number on the ATA controller.
                                On output, a pointer to the next port number on the ATA
                                controller. An input value of 0xFFFF retrieves the first port
                                number on the ATA controller.

  @retval EFI_SUCCESS           The next port number on the ATA controller was returned in Port.
  @retval EFI_NOT_FOUND         There are no more ports on this ATA controller.
  @retval EFI_INVALID_PARAMETER Port is not 0xFFFF and Port was not returned on a previous call
                                to GetNextPort().

**/
EFI_STATUS
SiI3132GetNextPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN OUT UINT16                 *Port
  )
{
  SATA_SI3132_INSTANCE    *SataSiI3132Instance;
  UINTN                   PrevPort;
  EFI_STATUS              Status = EFI_SUCCESS;

  SataSiI3132Instance = INSTANCE_FROM_ATAPASSTHRU_THIS (This);
  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  PrevPort = *Port;

  if (PrevPort == 0xFFFF) {
    *Port = 0;
  } else {
    if (PrevPort < SATA_SII3132_MAXPORT) {
        *Port = PrevPort + 1;
    } else {
        Status = EFI_NOT_FOUND;
    }
  }
  return Status;
}

/**
  Used to retrieve the list of legal port multiplier port numbers for ATA devices on a port of an ATA
  controller. These can either be the list of port multiplier ports where ATA devices are actually
  present on port or the list of legal port multiplier ports on that port. Regardless, the caller of this
  function must probe the port number and port multiplier port number returned to see if an ATA
  device is actually present.

  The GetNextDevice() function retrieves the port multiplier port number of an ATA device
  present on a port of an ATA controller.

  If PortMultiplierPort points to a port multiplier port number value that was returned on a
  previous call to GetNextDevice(), then the port multiplier port number of the next ATA device
  on the port of the ATA controller is returned in PortMultiplierPort, and EFI_SUCCESS is
  returned.

  If PortMultiplierPort points to 0xFFFF, then the port multiplier port number of the first
  ATA device on port of the ATA controller is returned in PortMultiplierPort and
  EFI_SUCCESS is returned.

  If PortMultiplierPort is not 0xFFFF and the value pointed to by PortMultiplierPort
  was not returned on a previous call to GetNextDevice(), then EFI_INVALID_PARAMETER
  is returned.

  If PortMultiplierPort is the port multiplier port number of the last ATA device on the port of
  the ATA controller, then EFI_NOT_FOUND is returned.

  @param[in]     This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]     Port                The port number present on the ATA controller.
  @param[in,out] PortMultiplierPort  On input, a pointer to the port multiplier port number of an
                                     ATA device present on the ATA controller.
                                     If on input a PortMultiplierPort of 0xFFFF is specified,
                                     then the port multiplier port number of the first ATA device
                                     is returned. On output, a pointer to the port multiplier port
                                     number of the next ATA device present on an ATA controller.

  @retval EFI_SUCCESS                The port multiplier port number of the next ATA device on the port
                                     of the ATA controller was returned in PortMultiplierPort.
  @retval EFI_NOT_FOUND              There are no more ATA devices on this port of the ATA controller.
  @retval EFI_INVALID_PARAMETER      PortMultiplierPort is not 0xFFFF, and PortMultiplierPort was not
                                     returned on a previous call to GetNextDevice().

**/
EFI_STATUS
SiI3132GetNextDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port,
  IN OUT UINT16                 *PortMultiplierPort
  )
{
  SATA_SI3132_INSTANCE    *SataSiI3132Instance;
  SATA_SI3132_PORT        *SataPort;
  SATA_SI3132_DEVICE      *SataDevice;
  LIST_ENTRY              *List;
  EFI_STATUS              Status = EFI_SUCCESS;

  SataSiI3132Instance = INSTANCE_FROM_ATAPASSTHRU_THIS (This);
  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  if (Port >= SATA_SII3132_MAXPORT) {
    return EFI_INVALID_PARAMETER;
  }

  SataPort = &(SataSiI3132Instance->Ports[Port]);

  if (*PortMultiplierPort == 0xFFFF) {
    List = SataPort->Devices.ForwardLink;
    if (List != &SataPort->Devices) {
      // The list is not empty, return the first device
      *PortMultiplierPort = ((SATA_SI3132_DEVICE*)List)->Index;
    } else {
      Status = EFI_NOT_FOUND;
    }
  } else {
    SataDevice = GetSataDevice (SataSiI3132Instance, Port, *PortMultiplierPort);
    if (SataDevice != NULL) {
      // We have found the previous port multiplier, return the next one
      List = SataDevice->Link.ForwardLink;
      if (List != &SataPort->Devices) {
        *PortMultiplierPort = ((SATA_SI3132_DEVICE*)List)->Index;
      } else {
        Status = EFI_NOT_FOUND;
      }
    } else {
      Status = EFI_NOT_FOUND;
    }
  }
  return Status;
}

/**
  Used to allocate and build a device path node for an ATA device on an ATA controller.

  The BuildDevicePath() function allocates and builds a single device node for the ATA
  device specified by Port and PortMultiplierPort. If the ATA device specified by Port and
  PortMultiplierPort is not present on the ATA controller, then EFI_NOT_FOUND is returned.
  If DevicePath is NULL, then EFI_INVALID_PARAMETER is returned. If there are not enough
  resources to allocate the device path node, then EFI_OUT_OF_RESOURCES is returned.

  Otherwise, DevicePath is allocated with the boot service AllocatePool(), the contents of
  DevicePath are initialized to describe the ATA device specified by Port and PortMultiplierPort,
  and EFI_SUCCESS is returned.

  @param[in]     This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]     Port                Port specifies the port number of the ATA device for which a
                                     device path node is to be allocated and built.
  @param[in]     PortMultiplierPort  The port multiplier port number of the ATA device for which a
                                     device path node is to be allocated and built. If there is no
                                     port multiplier, then specify 0.
  @param[in,out] DevicePath          A pointer to a single device path node that describes the ATA
                                     device specified by Port and PortMultiplierPort. This function
                                     is responsible for allocating the buffer DevicePath with the
                                     boot service AllocatePool(). It is the caller's responsibility
                                     to free DevicePath when the caller is finished with DevicePath.
  @retval EFI_SUCCESS                The device path node that describes the ATA device specified by
                                     Port and PortMultiplierPort was allocated and returned in DevicePath.
  @retval EFI_NOT_FOUND              The ATA device specified by Port and PortMultiplierPort does not
                                     exist on the ATA controller.
  @retval EFI_INVALID_PARAMETER      DevicePath is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources to allocate DevicePath.

**/
EFI_STATUS
SiI3132BuildDevicePath (
  IN     EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN     UINT16                     Port,
  IN     UINT16                     PortMultiplierPort,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath
  )
{
  SATA_SI3132_INSTANCE        *SataSiI3132Instance;
  SATA_SI3132_DEVICE          *SataDevice;
  EFI_DEVICE_PATH_PROTOCOL    *SiI3132DevicePath;

  SATA_TRACE ("SiI3132BuildDevicePath()");

  SataSiI3132Instance = INSTANCE_FROM_ATAPASSTHRU_THIS (This);
  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  SataDevice = GetSataDevice (SataSiI3132Instance, Port, PortMultiplierPort);
  if (SataDevice == NULL) {
    return EFI_NOT_FOUND;
  }

  SiI3132DevicePath = CreateDeviceNode (MESSAGING_DEVICE_PATH, MSG_SATA_DP, sizeof (SATA_DEVICE_PATH));
  if (SiI3132DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((SATA_DEVICE_PATH*)SiI3132DevicePath)->HBAPortNumber = Port;
  if (FeaturePcdGet (PcdSataSiI3132FeaturePMPSupport)) {
    ((SATA_DEVICE_PATH*)SiI3132DevicePath)->PortMultiplierPortNumber = PortMultiplierPort;
  } else {
    //Temp:((SATA_DEVICE_PATH*)SiI3132DevicePath)->PortMultiplierPortNumber = SATA_HBA_DIRECT_CONNECT_FLAG;
    ((SATA_DEVICE_PATH*)SiI3132DevicePath)->PortMultiplierPortNumber = 0;
  }
  ((SATA_DEVICE_PATH*)SiI3132DevicePath)->Lun = Port; //TODO: Search information how to define properly LUN (Logical Unit Number)

  *DevicePath = SiI3132DevicePath;
  return EFI_SUCCESS;
}

/**
  Used to translate a device path node to a port number and port multiplier port number.

  The GetDevice() function determines the port and port multiplier port number associated with
  the ATA device described by DevicePath. If DevicePath is a device path node type that the
  ATA Pass Thru driver supports, then the ATA Pass Thru driver will attempt to translate the contents
  DevicePath into a port number and port multiplier port number.

  If this translation is successful, then that port number and port multiplier port number are returned
  in Port and PortMultiplierPort, and EFI_SUCCESS is returned.

  If DevicePath, Port, or PortMultiplierPort are NULL, then EFI_INVALID_PARAMETER is returned.

  If DevicePath is not a device path node type that the ATA Pass Thru driver supports, then
  EFI_UNSUPPORTED is returned.

  If DevicePath is a device path node type that the ATA Pass Thru driver supports, but there is not
  a valid translation from DevicePath to a port number and port multiplier port number, then
  EFI_NOT_FOUND is returned.

  @param[in]  This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]  DevicePath          A pointer to the device path node that describes an ATA device on the
                                  ATA controller.
  @param[out] Port                On return, points to the port number of an ATA device on the ATA controller.
  @param[out] PortMultiplierPort  On return, points to the port multiplier port number of an ATA device
                                  on the ATA controller.

  @retval EFI_SUCCESS             DevicePath was successfully translated to a port number and port multiplier
                                  port number, and they were returned in Port and PortMultiplierPort.
  @retval EFI_INVALID_PARAMETER   DevicePath is NULL.
  @retval EFI_INVALID_PARAMETER   Port is NULL.
  @retval EFI_INVALID_PARAMETER   PortMultiplierPort is NULL.
  @retval EFI_UNSUPPORTED         This driver does not support the device path node type in DevicePath.
  @retval EFI_NOT_FOUND           A valid translation from DevicePath to a port number and port multiplier
                                  port number does not exist.
**/
EFI_STATUS
SiI3132GetDevice (
  IN  EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  OUT UINT16                     *Port,
  OUT UINT16                     *PortMultiplierPort
  )
{
  SATA_SI3132_INSTANCE        *SataSiI3132Instance;

  SATA_TRACE ("SiI3132GetDevice()");

  if (!DevicePath || !Port || !PortMultiplierPort) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePath->Type != MESSAGING_DEVICE_PATH) || (DevicePath->SubType != MSG_SATA_DP)) {
    return EFI_UNSUPPORTED;
  }

  SataSiI3132Instance = INSTANCE_FROM_ATAPASSTHRU_THIS (This);
  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  if (((SATA_DEVICE_PATH*)DevicePath)->Lun >= SATA_SII3132_MAXPORT) {
    return EFI_NOT_FOUND;
  }

  if (FeaturePcdGet (PcdSataSiI3132FeaturePMPSupport)) {
    ASSERT (0); //TODO: Implement me!
    return EFI_UNSUPPORTED;
  } else {
    *Port = ((SATA_DEVICE_PATH*)DevicePath)->Lun;
    // Return the first Sata Sevice as there should be only one directly connected
    *PortMultiplierPort = ((SATA_SI3132_DEVICE*)SataSiI3132Instance->Ports[*Port].Devices.ForwardLink)->Index;
    return EFI_SUCCESS;
  }
}

EFI_STATUS
SiI3132HwResetPort (
  IN SATA_SI3132_PORT *SataPort
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT32              Value32;
  UINTN               Timeout;

  SATA_TRACE ("SiI3132HwResetPort()");

  PciIo = SataPort->Instance->PciIo;

  // Clear Port Reset
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_CONTROLCLEAR_REG, SII3132_PORT_CONTROL_RESET);

  SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_STATUS_REG, &Value32);
  ASSERT (!(Value32 & SII3132_PORT_CONTROL_RESET));

  // Initialize error counters
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_ERRCOUNTDECODE, 0);
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_ERRCOUNTCRC, 0);
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_ERRCOUNTHANDSHAKE, 0);

  // Enable interrupts for command completion and command errors
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_ENABLEINT_REG, SII3132_PORT_INT_CMDCOMPL | SII3132_PORT_INT_CMDERR);

  // Clear IRQ
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_ENABLEINT_REG, SII3132_PORT_INT_CMDCOMPL | SII3132_PORT_INT_CMDERR | SII3132_PORT_INT_PORTRDY | (1 << 3));

  // Wait until Port Ready
  SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, &Value32);
  Timeout = 1000;
  while ((Timeout > 0) && ((Value32 & SII3132_PORT_INT_PORTRDY) == 0)) {
    gBS->Stall (1);
    Timeout--;
    SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, &Value32);
  }
  // Clear IRQ
  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_INTSTATUS_REG, SII3132_PORT_INT_PORTRDY);

  if (Timeout == 0) {
    SATA_TRACE ("SiI3132HwResetPort(): Timeout");
    return EFI_TIMEOUT;
  } else if ((Value32 & SII3132_PORT_INT_PORTRDY) == 0) {
    SATA_TRACE ("SiI3132HwResetPort(): Port Not Ready");
    return EFI_DEVICE_ERROR;
  } else {
    return EFI_SUCCESS;
  }
}

/**
  Resets a specific port on the ATA controller. This operation also resets all the ATA devices
  connected to the port.

  The ResetChannel() function resets an a specific port on an ATA controller. This operation
  resets all the ATA devices connected to that port. If this ATA controller does not support
  a reset port operation, then EFI_UNSUPPORTED is returned.

  If a device error occurs while executing that port reset operation, then EFI_DEVICE_ERROR is
  returned.

  If a timeout occurs during the execution of the port reset operation, then EFI_TIMEOUT is returned.

  If the port reset operation is completed, then EFI_SUCCESS is returned.

  @param[in]  This          A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]  Port          The port number on the ATA controller.

  @retval EFI_SUCCESS       The ATA controller port was reset.
  @retval EFI_UNSUPPORTED   The ATA controller does not support a port reset operation.
  @retval EFI_DEVICE_ERROR  A device error occurred while attempting to reset the ATA port.
  @retval EFI_TIMEOUT       A timeout occurred while attempting to reset the ATA port.

**/
EFI_STATUS
SiI3132ResetPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port
  )
{
  SATA_SI3132_INSTANCE    *SataSiI3132Instance;
  SATA_SI3132_PORT        *SataPort;

  SATA_TRACE ("SiI3132ResetPort()");

  SataSiI3132Instance = INSTANCE_FROM_ATAPASSTHRU_THIS (This);
  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  if (Port >= SATA_SII3132_MAXPORT) {
    return EFI_UNSUPPORTED;
  }

  SataPort = &(SataSiI3132Instance->Ports[Port]);
  return SiI3132HwResetPort (SataPort);
}

/**
  Resets an ATA device that is connected to an ATA controller.

  The ResetDevice() function resets the ATA device specified by Port and PortMultiplierPort.
  If this ATA controller does not support a device reset operation, then EFI_UNSUPPORTED is
  returned.

  If Port or PortMultiplierPort are not in a valid range for this ATA controller, then
  EFI_INVALID_PARAMETER is returned.

  If a device error occurs while executing that device reset operation, then EFI_DEVICE_ERROR
  is returned.

  If a timeout occurs during the execution of the device reset operation, then EFI_TIMEOUT is
  returned.

  If the device reset operation is completed, then EFI_SUCCESS is returned.

  @param[in] This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in] Port                Port represents the port number of the ATA device to be reset.
  @param[in] PortMultiplierPort  The port multiplier port number of the ATA device to reset.
                                 If there is no port multiplier, then specify 0.
  @retval EFI_SUCCESS            The ATA device specified by Port and PortMultiplierPort was reset.
  @retval EFI_UNSUPPORTED        The ATA controller does not support a device reset operation.
  @retval EFI_INVALID_PARAMETER  Port or PortMultiplierPort are invalid.
  @retval EFI_DEVICE_ERROR       A device error occurred while attempting to reset the ATA device
                                 specified by Port and PortMultiplierPort.
  @retval EFI_TIMEOUT            A timeout occurred while attempting to reset the ATA device
                                 specified by Port and PortMultiplierPort.

**/
EFI_STATUS
SiI3132ResetDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port,
  IN UINT16                     PortMultiplierPort
  )
{
  EFI_PCI_IO_PROTOCOL     *PciIo;
  SATA_SI3132_INSTANCE    *SataSiI3132Instance;
  SATA_SI3132_PORT        *SataPort;
  SATA_SI3132_DEVICE      *SataDevice;
  UINTN                   Timeout;
  UINT32                  Value32;

  SATA_TRACE ("SiI3132ResetDevice()");

  SataSiI3132Instance = INSTANCE_FROM_ATAPASSTHRU_THIS (This);
  if (!SataSiI3132Instance) {
    return EFI_INVALID_PARAMETER;
  }

  PciIo = SataSiI3132Instance->PciIo;

  SataDevice = GetSataDevice (SataSiI3132Instance, Port, PortMultiplierPort);
  if (!SataDevice) {
    return EFI_INVALID_PARAMETER;
  }
  SataPort = SataDevice->Port;

  SATA_PORT_WRITE32 (SataPort->RegBase + SII3132_PORT_CONTROLSET_REG, SII3132_PORT_DEVICE_RESET);

  Timeout = 100;
  SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_STATUS_REG, &Value32);
  while ((Timeout > 0) && ((Value32 & SII3132_PORT_DEVICE_RESET) != 0)) {
    gBS->Stall (1);
    SATA_PORT_READ32 (SataPort->RegBase + SII3132_PORT_STATUS_REG, &Value32);
    Timeout--;
  }

  if (Timeout == 0) {
    SATA_TRACE ("SiI3132ResetDevice(): Timeout");
    return EFI_TIMEOUT;
  } else {
    return EFI_SUCCESS;
  }
}
