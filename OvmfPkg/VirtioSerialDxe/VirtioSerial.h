/** @file

  Private definitions of the VirtioRng RNG driver

  Copyright (C) 2016, Linaro Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_SERIAL_DXE_H_
#define _VIRTIO_SERIAL_DXE_H_

#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SerialIo.h>

#include <IndustryStandard/Virtio.h>
#include <IndustryStandard/VirtioSerial.h>

#define VIRTIO_SERIAL_SIG  SIGNATURE_32 ('V', 'S', 'I', 'O')

#define MAX_PORTS  8
#define MAX_RINGS  (MAX_PORTS * 2 + 2)

#define CTRL_RX_BUFSIZE  128
#define CTRL_TX_BUFSIZE  sizeof(VIRTIO_SERIAL_CONTROL)
#define PORT_RX_BUFSIZE  128
#define PORT_TX_BUFSIZE  128

//
// Data structures
//

typedef struct _VIRTIO_SERIAL_DEV          VIRTIO_SERIAL_DEV;
typedef struct _VIRTIO_SERIAL_RING         VIRTIO_SERIAL_RING;
typedef struct _VIRTIO_SERIAL_PORT         VIRTIO_SERIAL_PORT;
typedef struct _VIRTIO_SERIAL_IO_PROTOCOL  VIRTIO_SERIAL_IO_PROTOCOL;

struct _VIRTIO_SERIAL_RING {
  VRING                   Ring;
  VOID                    *RingMap;
  DESC_INDICES            Indices;        /* Avail Ring */
  UINT16                  LastUsedIdx;    /* Used Ring */

  UINT32                  BufferSize;
  UINT32                  BufferCount;
  UINT32                  BufferPages;
  UINT8                   *Buffers;
  VOID                    *BufferMap;
  EFI_PHYSICAL_ADDRESS    DeviceAddress;

  BOOLEAN                 Ready;
};

struct _VIRTIO_SERIAL_PORT {
  BOOLEAN                      Ready;
  BOOLEAN                      Console;
  BOOLEAN                      DeviceOpen;

  CHAR16                       Name[32];

  VIRTIO_SERIAL_IO_PROTOCOL    *SerialIo;
};

struct _VIRTIO_SERIAL_DEV {
  UINT32                      Signature;
  LIST_ENTRY                  Link;

  EFI_HANDLE                  DriverBindingHandle;
  EFI_HANDLE                  DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  VIRTIO_DEVICE_PROTOCOL      *VirtIo;
  EFI_EVENT                   ExitBoot;
  VIRTIO_SERIAL_CONFIG        Config;
  VIRTIO_SERIAL_PORT          Ports[MAX_PORTS];
  VIRTIO_SERIAL_RING          Rings[MAX_RINGS];
  EFI_EVENT                   Timer;

  UINT32                      NumPorts;
  UINT32                      NumConsoles;
  UINT32                      NumNamedPorts;
};

struct _VIRTIO_SERIAL_IO_PROTOCOL {
  EFI_SERIAL_IO_PROTOCOL      SerialIo;
  EFI_SERIAL_IO_MODE          SerialIoMode;

  EFI_HANDLE                  DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  VIRTIO_SERIAL_DEV           *Dev;
  UINT32                      PortId;

  UINT8                       ReadBuffer[PORT_RX_BUFSIZE];
  UINT32                      ReadOffset;
  UINT32                      ReadSize;

  UINT8                       WriteBuffer[PORT_TX_BUFSIZE];
  UINT32                      WriteOffset;
};

//
// VirtioSerial.c
//

VOID
EFIAPI
LogDevicePath (
  UINT32                    Level,
  const CHAR8               *Func,
  CHAR16                    *Note,
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

EFI_STATUS
EFIAPI
VirtioSerialTxControl (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             Id,
  IN     UINT16             Event,
  IN     UINT16             Value
  );

//
// VirtioSerialRing.c
//

EFI_STATUS
EFIAPI
VirtioSerialInitRing (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index,
  IN     UINT32             BufferSize
  );

VOID
EFIAPI
VirtioSerialUninitRing (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  );

VOID
EFIAPI
VirtioSerialRingFillRx (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  );

VOID
EFIAPI
VirtioSerialRingClearTx (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  );

EFI_STATUS
EFIAPI
VirtioSerialRingSendBuffer (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index,
  IN     VOID               *Data,
  IN     UINT32             DataSize,
  IN     BOOLEAN            Notify
  );

BOOLEAN
EFIAPI
VirtioSerialRingHasBuffer (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  );

BOOLEAN
EFIAPI
VirtioSerialRingGetBuffer (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index,
  OUT    VOID               *Data,
  OUT    UINT32             *DataSize
  );

//
// VirtioSerialPort.c
//

EFI_STATUS
EFIAPI
VirtioSerialPortAdd (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId
  );

VOID
EFIAPI
VirtioSerialPortSetConsole (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId
  );

VOID
EFIAPI
VirtioSerialPortSetName (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId,
  IN     UINT8              *Name
  );

VOID
EFIAPI
VirtioSerialPortSetDeviceOpen (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId,
  IN     UINT16             Value
  );

VOID
EFIAPI
VirtioSerialPortRemove (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT32             PortId
  );

#endif
