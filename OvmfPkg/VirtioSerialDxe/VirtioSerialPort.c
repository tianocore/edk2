/** @file

  Driver for virtio-serial devices.

  Helper functions to manage virtio serial ports.
  Console ports will be registered as SerialIo UARTs.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include "VirtioSerial.h"

ACPI_HID_DEVICE_PATH  mAcpiSerialDevNode = {
  {
    ACPI_DEVICE_PATH,
    ACPI_DP,
    {
      (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
      (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
    },
  },
  EISA_PNP_ID (0x0501),
  0
};

UART_DEVICE_PATH  mUartDevNode = {
  {
    MESSAGING_DEVICE_PATH,
    MSG_UART_DP,
    {
      (UINT8)(sizeof (UART_DEVICE_PATH)),
      (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
    }
  },
  0,      // Reserved
  115200, // Speed
  8, 1, 1 // 8n1
};

STATIC
UINT16
PortRx (
  IN UINT32  PortId
  )
{
  ASSERT (PortId < MAX_PORTS);

  if (PortId >= 1) {
    return (UINT16)(VIRTIO_SERIAL_Q_RX_BASE + (PortId - 1) * 2);
  }

  return VIRTIO_SERIAL_Q_RX_PORT0;
}

STATIC
UINT16
PortTx (
  IN UINT32  PortId
  )
{
  ASSERT (PortId < MAX_PORTS);

  if (PortId >= 1) {
    return (UINT16)(VIRTIO_SERIAL_Q_TX_BASE + (PortId - 1) * 2);
  }

  return VIRTIO_SERIAL_Q_TX_PORT0;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialIoReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  )
{
  DEBUG ((DEBUG_VERBOSE, "%a:%d:\n", __func__, __LINE__));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialIoSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT64                  BaudRate,
  IN UINT32                  ReceiveFifoDepth,
  IN UINT32                  Timeout,
  IN EFI_PARITY_TYPE         Parity,
  IN UINT8                   DataBits,
  IN EFI_STOP_BITS_TYPE      StopBits
  )
{
  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%d: Rate %ld, Fifo %d, Bits %d\n",
    __func__,
    __LINE__,
    BaudRate,
    ReceiveFifoDepth,
    DataBits
    ));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialIoSetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  )
{
  DEBUG ((DEBUG_INFO, "%a:%d: Control 0x%x\n", __func__, __LINE__, Control));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialIoGetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  )
{
  DEBUG ((DEBUG_VERBOSE, "%a:%d: Control 0x%x\n", __func__, __LINE__, *Control));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialIoWrite (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  )
{
  VIRTIO_SERIAL_IO_PROTOCOL  *SerialIo = (VIRTIO_SERIAL_IO_PROTOCOL *)This;
  VIRTIO_SERIAL_PORT         *Port     = SerialIo->Dev->Ports + SerialIo->PortId;
  UINT32                     Length;
  EFI_TPL                    OldTpl;

  if (!Port->DeviceOpen) {
    *BufferSize = 0;
    return EFI_SUCCESS;
  }

  VirtioSerialRingClearTx (SerialIo->Dev, PortTx (SerialIo->PortId));

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (SerialIo->WriteOffset &&
      (SerialIo->WriteOffset + *BufferSize > PORT_TX_BUFSIZE))
  {
    DEBUG ((DEBUG_VERBOSE, "%a:%d: WriteFlush %d\n", __func__, __LINE__, SerialIo->WriteOffset));
    VirtioSerialRingSendBuffer (
      SerialIo->Dev,
      PortTx (SerialIo->PortId),
      SerialIo->WriteBuffer,
      SerialIo->WriteOffset,
      TRUE
      );
    SerialIo->WriteOffset = 0;
  }

  Length = MIN ((UINT32)(*BufferSize), PORT_TX_BUFSIZE - SerialIo->WriteOffset);
  CopyMem (SerialIo->WriteBuffer + SerialIo->WriteOffset, Buffer, Length);
  SerialIo->WriteOffset += Length;
  *BufferSize            = Length;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialIoRead (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  )
{
  VIRTIO_SERIAL_IO_PROTOCOL  *SerialIo = (VIRTIO_SERIAL_IO_PROTOCOL *)This;
  VIRTIO_SERIAL_PORT         *Port     = SerialIo->Dev->Ports + SerialIo->PortId;
  BOOLEAN                    HasData;
  UINT32                     Length;
  EFI_TPL                    OldTpl;

  if (!Port->DeviceOpen) {
    goto NoData;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (SerialIo->WriteOffset) {
    DEBUG ((DEBUG_VERBOSE, "%a:%d: WriteFlush %d\n", __func__, __LINE__, SerialIo->WriteOffset));
    VirtioSerialRingSendBuffer (
      SerialIo->Dev,
      PortTx (SerialIo->PortId),
      SerialIo->WriteBuffer,
      SerialIo->WriteOffset,
      TRUE
      );
    SerialIo->WriteOffset = 0;
  }

  gBS->RestoreTPL (OldTpl);

  if (SerialIo->ReadOffset == SerialIo->ReadSize) {
    HasData = VirtioSerialRingGetBuffer (
                SerialIo->Dev,
                PortRx (SerialIo->PortId),
                &SerialIo->ReadBuffer,
                &SerialIo->ReadSize
                );
    if (!HasData) {
      goto NoData;
    }

    SerialIo->ReadOffset = 0;
  }

  if (SerialIo->ReadOffset < SerialIo->ReadSize) {
    Length = SerialIo->ReadSize - SerialIo->ReadOffset;
    if (Length > *BufferSize) {
      Length = (UINT32)(*BufferSize);
    }

    CopyMem (Buffer, SerialIo->ReadBuffer + SerialIo->ReadOffset, Length);
    SerialIo->ReadOffset += Length;
    *BufferSize           = Length;
    return EFI_SUCCESS;
  }

NoData:
  *BufferSize = 0;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtioSerialIoInit (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId
  )
{
  VIRTIO_SERIAL_PORT         *Port = Dev->Ports + PortId;
  VIRTIO_SERIAL_IO_PROTOCOL  *SerialIo;
  EFI_STATUS                 Status;

  SerialIo       = (VIRTIO_SERIAL_IO_PROTOCOL *)AllocateZeroPool (sizeof *SerialIo);
  Port->SerialIo = SerialIo;

  SerialIo->SerialIo.Revision      = EFI_SERIAL_IO_PROTOCOL_REVISION;
  SerialIo->SerialIo.Reset         = VirtioSerialIoReset;
  SerialIo->SerialIo.SetAttributes = VirtioSerialIoSetAttributes;
  SerialIo->SerialIo.SetControl    = VirtioSerialIoSetControl;
  SerialIo->SerialIo.GetControl    = VirtioSerialIoGetControl;
  SerialIo->SerialIo.Write         = VirtioSerialIoWrite;
  SerialIo->SerialIo.Read          = VirtioSerialIoRead;
  SerialIo->SerialIo.Mode          = &SerialIo->SerialIoMode;
  SerialIo->Dev                    = Dev;
  SerialIo->PortId                 = PortId;

  SerialIo->DevicePath   = DuplicateDevicePath (Dev->DevicePath);
  mAcpiSerialDevNode.UID = PortId;
  SerialIo->DevicePath   = AppendDevicePathNode (
                             SerialIo->DevicePath,
                             (EFI_DEVICE_PATH_PROTOCOL *)&mAcpiSerialDevNode
                             );
  SerialIo->DevicePath = AppendDevicePathNode (
                           SerialIo->DevicePath,
                           (EFI_DEVICE_PATH_PROTOCOL *)&mUartDevNode
                           );

  LogDevicePath (DEBUG_INFO, __func__, L"UART", SerialIo->DevicePath);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &SerialIo->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  SerialIo->DevicePath,
                  &gEfiSerialIoProtocolGuid,
                  &SerialIo->SerialIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a:%d: ERROR: %r\n", __func__, __LINE__, Status));
    goto FreeSerialIo;
  }

  Status = gBS->OpenProtocol (
                  Dev->DeviceHandle,
                  &gVirtioDeviceProtocolGuid,
                  (VOID **)&Dev->VirtIo,
                  Dev->DriverBindingHandle,
                  SerialIo->DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a:%d: ERROR: %r\n", __func__, __LINE__, Status));
    goto UninstallProtocol;
  }

  return EFI_SUCCESS;

UninstallProtocol:
  gBS->UninstallMultipleProtocolInterfaces (
         SerialIo->DeviceHandle,
         &gEfiDevicePathProtocolGuid,
         SerialIo->DevicePath,
         &gEfiSerialIoProtocolGuid,
         &SerialIo->SerialIo,
         NULL
         );

FreeSerialIo:
  FreePool (Port->SerialIo);
  Port->SerialIo = NULL;
  return Status;
}

STATIC
VOID
EFIAPI
VirtioSerialIoUninit (
  VIRTIO_SERIAL_IO_PROTOCOL  *SerialIo
  )
{
  VIRTIO_SERIAL_DEV   *Dev  = SerialIo->Dev;
  VIRTIO_SERIAL_PORT  *Port = Dev->Ports + SerialIo->PortId;

  DEBUG ((DEBUG_INFO, "%a:%d: %s\n", __func__, __LINE__, Port->Name));

  gBS->CloseProtocol (
         Dev->DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         Dev->DriverBindingHandle,
         SerialIo->DeviceHandle
         );

  gBS->UninstallMultipleProtocolInterfaces (
         SerialIo->DeviceHandle,
         &gEfiDevicePathProtocolGuid,
         SerialIo->DevicePath,
         &gEfiSerialIoProtocolGuid,
         &SerialIo->SerialIo,
         NULL
         );

  FreePool (SerialIo);
  Port->SerialIo = NULL;
}

EFI_STATUS
EFIAPI
VirtioSerialPortAdd (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId
  )
{
  VIRTIO_SERIAL_PORT  *Port = Dev->Ports + PortId;
  EFI_STATUS          Status;

  if (Port->Ready) {
    return EFI_SUCCESS;
  }

  Status = VirtioSerialInitRing (Dev, PortRx (PortId), PORT_RX_BUFSIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = VirtioSerialInitRing (Dev, PortTx (PortId), PORT_TX_BUFSIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  UnicodeSPrint (Port->Name, sizeof (Port->Name), L"Port #%d", PortId);
  VirtioSerialRingFillRx (Dev, PortRx (PortId));
  Port->Ready = TRUE;

  return EFI_SUCCESS;

Failed:
  VirtioSerialUninitRing (Dev, PortRx (PortId));
  return Status;
}

VOID
EFIAPI
VirtioSerialPortSetConsole (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId
  )
{
  VIRTIO_SERIAL_PORT  *Port = Dev->Ports + PortId;

  Port->Console = TRUE;
  UnicodeSPrint (Port->Name, sizeof (Port->Name), L"Console #%d", PortId);
  VirtioSerialIoInit (Dev, PortId);
}

VOID
EFIAPI
VirtioSerialPortSetName (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId,
  IN     UINT8              *Name
  )
{
  VIRTIO_SERIAL_PORT  *Port = Dev->Ports + PortId;

  DEBUG ((DEBUG_INFO, "%a:%d: \"%a\"\n", __func__, __LINE__, Name));
  UnicodeSPrint (Port->Name, sizeof (Port->Name), L"NamedPort #%d (%a)", PortId, Name);
}

VOID
EFIAPI
VirtioSerialPortSetDeviceOpen (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId,
  IN     UINT16             Value
  )
{
  VIRTIO_SERIAL_PORT  *Port = Dev->Ports + PortId;

  Port->DeviceOpen = (BOOLEAN)Value;
  if (Port->DeviceOpen) {
    VirtioSerialTxControl (Dev, PortId, VIRTIO_SERIAL_PORT_OPEN, 1);
  }
}

VOID
EFIAPI
VirtioSerialPortRemove (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId
  )
{
  VIRTIO_SERIAL_PORT  *Port = Dev->Ports + PortId;

  if (!Port->Ready) {
    return;
  }

  if (Port->SerialIo) {
    VirtioSerialIoUninit (Port->SerialIo);
    Port->SerialIo = NULL;
  }

  VirtioSerialUninitRing (Dev, PortRx (PortId));
  VirtioSerialUninitRing (Dev, PortTx (PortId));
  Port->Ready = FALSE;
}
